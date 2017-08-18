#include <stdlib.h>
#include <string.h>
#include "transmission.h"

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_tx_init(
    wtp_tx_ctrl_t* self,
    uint16_t window_size,
    uint16_t timeout,
    uint16_t read_size,
    uint16_t pkt_buf_size,
    uint16_t msg_buf_size,
    uint8_t n_fragments,
    uint8_t n_msgs
) {
    //Sequence number
    self->_seq_num = 0;
    //Window size
    self->_window_size = window_size;
    //Tineout
    self->_timeout = timeout;
    //Read size
    self->_read_size = read_size;

    //Packet buffer
    WIO_TRY(wio_buf_alloc_init(&self->_pkt_buf, pkt_buf_size))
    //Message buffer
    WIO_TRY(wio_buf_alloc_init(&self->_msg_buf, msg_buf_size))

    //Begin position of next message (Sequence number)
    self->_msg_begin_seq = 0;
    //Begin position of next message (Buffer position)
    self->_msg_begin_pos = 0;
    //Fragmented position of next message
    self->_msg_fragmented = 0;

    //Data fragments queue
    WIO_TRY(wio_queue_init(&self->_fragments_queue, sizeof(wtp_tx_fragment_t), n_fragments))
    //READ OpSpec information queue
    WIO_TRY(wio_queue_init(&self->_read_info_queue, sizeof(wtp_tx_read_info_t), n_msgs))
    //Message ends sequence number queue
    WIO_TRY(wio_queue_init(&self->_msg_ends_queue, sizeof(uint16_t), n_msgs))

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_tx_fini(
    wtp_tx_ctrl_t* self
) {
    //Packet buffer
    free(self->_pkt_buf.buffer);
    //Message buffer
    free(self->_msg_buf.buffer);

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_tx_begin_packet(
    wtp_tx_ctrl_t* self,
    wtp_pkt_t pkt_type
) {
    wio_buf_t* pkt_buf = &self->_pkt_buf;

    //Allocate current packet size memory
    WIO_TRY(wio_alloc(pkt_buf, 1, &self->_pkt_size))
    //Packet begin position
    self->_pkt_begin = pkt_buf->pos_b;
    //Write packet type
    WIO_TRY(wio_write(pkt_buf, &pkt_type, 1))

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_tx_end_packet(
    wtp_tx_ctrl_t* self
) {
    //Packet size
    uint16_t pkt_size = self->_pkt_buf.pos_b-self->_pkt_begin;
    //Write packet size
    *self->_pkt_size = (uint8_t)pkt_size;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_tx_add_msg(
    wtp_tx_ctrl_t* self,
    uint8_t* data,
    uint16_t size,
    wtp_tx_read_info_t** _read_info
) {
    wio_buf_t* msg_buf = &self->_msg_buf;

    //Write message size
    WIO_TRY(wio_write(msg_buf, &size, 2))
    //Allocate memory for message data
    uint8_t* msg_data;
    WIO_TRY(wio_alloc(msg_buf, size, &msg_data))
    //Copy message data
    memcpy(msg_data, data, size);

    //READ information queue
    wio_queue_t* read_info_queue = &self->_read_info_queue;

    //Number of READ OpSpecs needed
    uint8_t n_reads = (uint8_t)(size/self->_read_size+1);
    //Create READ OpSpec information
    wtp_tx_read_info_t read_info;
    read_info._size = self->_read_size;
    read_info._n_reads = n_reads;
    //Push into READ information queue
    WIO_TRY(wio_queue_push(read_info_queue, &read_info))

    //Return READ information
    WIO_RETURN(_read_info, WIO_QUEUE_BEGIN(read_info_queue, wtp_tx_read_info_t))

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_tx_make_fragment(
    wtp_tx_ctrl_t* self,
    uint8_t avail_size,
    wtp_tx_fragment_t** _fragment
) {
    //Message buffer
    wio_buf_t* msg_buf = WIO_INST_PTR(wio_buf_t);
    //Copy message buffer (Shallow copy)
    *msg_buf = self->_msg_buf;
    //Set read cursor
    msg_buf->pos_a = self->_msg_begin_pos;

    //No messages to make fragment from
    if (msg_buf->pos_a==msg_buf->pos_b)
        return WIO_OK;
    //Read message size
    uint16_t msg_size;
    WIO_TRY(wio_read(msg_buf, &msg_size, 2))

    //Message ends queue
    wio_queue_t* msg_ends_queue = &self->_msg_ends_queue;

    //Load next message data into fragment
    if (self->_msg_fragmented>=msg_size) {
        //Update message begin
        self->_msg_begin_seq += msg_size;
        //Update fragmented position
        self->_msg_fragmented = 0;

        //Add message end
        WIO_TRY(wio_queue_push(msg_ends_queue, &self->_msg_begin_seq))

        //Skip previous message data
        WIO_TRY(wio_free(msg_buf, msg_size))
        //Update next message buffer position
        self->_msg_begin_pos = msg_buf->pos_a;

        //No messages to make fragment from
        if (msg_buf->pos_a==msg_buf->pos_b)
            return WIO_OK;
        //Read message size
        WIO_TRY(wio_read(msg_buf, &msg_size, 2))
    }

    //Sequence number and fragmented position
    uint16_t msg_fragmented = self->_msg_fragmented;
    //Sequence number
    uint16_t seq_num = self->_msg_begin_seq+msg_fragmented;

    //Maximum packet data size using different criterion
    //(Header size is 6 for WTP_PKT_BEGIN_MSG and 4 for WTP_PKT_CONT_MSG)
    uint16_t max_avail = avail_size-(msg_fragmented==0)?6:4;
    uint16_t max_msg = msg_size-msg_fragmented;
    uint16_t max_window = self->_seq_num+self->_window_size-seq_num;
    //Fragment data size
    uint16_t fragment_data_size = WIO_MIN3(max_avail, max_msg, max_window);
    if (fragment_data_size==0)
        return WIO_OK;

    //Data fragments queue
    wio_queue_t* fragments_queue = &self->_fragments_queue;

    //Update fragmented position
    self->_msg_fragmented += fragment_data_size;
    //Get fragment data position
    uint8_t* fragment_data = msg_buf->buffer+msg_buf->pos_a+msg_fragmented;
    //Make fragment
    wtp_tx_fragment_t fragment;
    fragment._seq_num = seq_num;
    fragment._msg_size = (msg_fragmented==0)?msg_size:0;
    fragment._data = fragment_data;
    fragment._size = fragment_data_size;
    fragment._need_send = false;
    //Push fragment into queue
    WIO_TRY(wio_queue_push(fragments_queue, &fragment))

    //Return fragment
    WIO_RETURN(_fragment, WIO_QUEUE_BEGIN(fragments_queue, wtp_tx_fragment_t))

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_tx_handle_ack(
    wtp_tx_ctrl_t* self,
    uint16_t seq_num,
    uint8_t* _n_msgs
) {
    //Number of messages sent
    uint8_t n_sent_msgs = 0;

    //Invalid sequence number; drop acknowledgement
    if (seq_num>self->_msg_begin_seq+self->_msg_fragmented)
        return WIO_ERR_INVALID;

    //Fragments queue
    wio_queue_t* fragments_queue = &self->_fragments_queue;
    //Number of fragments to remove
    uint8_t n_fragments;
    //Current queue index
    uint8_t queue_index = fragments_queue->end;

    for (uint8_t i=0;i<fragments_queue->size;i++) {
        //Current fragment
        wtp_tx_fragment_t* fragment = WIO_QUEUE_AT(fragments_queue, wtp_tx_fragment_t, queue_index);
        //Fragment end
        uint16_t fragment_end = fragment->_seq_num+fragment->_size;

        //No fragment to acknowledge or sequence number not at fragments border
        if (fragment_end>seq_num)
            return WIO_ERR_INVALID;
        //Update number of fragments
        n_fragments++;
        //End of acknowledged range
        if (fragment_end==seq_num)
            break;

        //Update queue index
        queue_index++;
        if (queue_index==fragments_queue->size)
            queue_index = 0;
    }

    //Message ends queue
    wio_queue_t* msg_ends_queue = &self->_msg_ends_queue;
    //Message buffer
    wio_buf_t* msg_buf = &self->_msg_buf;

    //Remove acknowledged fragments
    for (uint8_t i=0;i<n_fragments;i++) {
        //Next fragment
        wtp_tx_fragment_t fragment;
        //Pop fragment from queue
        WIO_TRY(wio_queue_pop(msg_ends_queue, &fragment))
        //Fragment end
        uint16_t fragment_end = fragment._seq_num+fragment._size;

        if (msg_ends_queue->size) {
            //Get next message end
            uint16_t* msg_end = WIO_QUEUE_END(msg_ends_queue, uint16_t);

            //Whole message sent
            if (*msg_end<=fragment_end) {
                //Update number of sent messages
                n_sent_msgs++;

                //Pop message end
                WIO_TRY(wio_queue_pop(msg_ends_queue, NULL))
                //Read message memory size
                uint16_t msg_size;
                WIO_TRY(wio_read(msg_buf, &msg_size, 2))
                //Release message memory
                WIO_TRY(wio_free(msg_buf, msg_size))
            }
        }

        //TODO: Resolve fragment timer
    }

    //Update sequence number
    self->_seq_num = seq_num;
    //Return number of messages sent
    WIO_RETURN(_n_msgs, n_sent_msgs)

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_rx_init(
    wtp_rx_ctrl_t* self,
    uint16_t window_size,
    uint16_t msg_data_size,
    uint16_t fragments_size,
    uint8_t n_msg_info
) {
    //Sequence number
    self->_seq_num = 0;
    //Window size
    self->_window_size = window_size;

    //Message data buffer
    WIO_TRY(wio_buf_alloc_init(&self->_msg_data_buf, msg_data_size))

    //Data fragments buffer
    WIO_TRY(wio_buf_alloc_init(&self->_fragments_buf, fragments_size))
    //Begin of data fragments linked list
    self->_fragments_begin = NULL;

    //Message information begin and size
    self->_msg_info_size = self->_msg_info_begin = n_msg_info;
    //Message information linked list
    wtp_rx_msg_info_t* msg_info_store = malloc(n_msg_info*sizeof(wtp_rx_msg_info_t));
    if (!msg_info_store)
        return WIO_ERR_NO_MEMORY;
    self->_msg_info_store = msg_info_store;
    //Initialize linked list items
    for (uint8_t i=0;i<n_msg_info;i++)
        msg_info_store[i]._in_use = false;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_rx_fini(
    wtp_rx_ctrl_t* self
) {
    //Message data buffer
    free(self->_msg_data_buf.buffer);
    //Data fragments buffer
    free(self->_fragments_buf.buffer);
    //Message information linked list
    free(self->_msg_info_store);

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_rx_handle_packet(
    wtp_rx_ctrl_t* self,
    uint16_t seq_num,
    uint8_t* data,
    uint16_t size,
    uint16_t new_msg_size,
    uint8_t* _n_msgs
) {
    //Packet data range and sliding window
    uint16_t rel_pkt_begin = seq_num-self->_seq_num;
    uint16_t rel_pkt_end = rel_pkt_begin+size;
    //Packet data range must be within sliding window, or the packet is dropped
    if (!((rel_pkt_begin<rel_pkt_end)&&(rel_pkt_end<=self->_window_size)))
        return WIO_ERR_INVALID;

    wtp_rx_msg_info_t* msg_info_store = self->_msg_info_store;
    uint8_t msg_info_size = self->_msg_info_size;
    //Begin of message
    if (new_msg_size) {
        //Message information before and after insertion position
        uint8_t before_msg_info = self->_msg_info_size;
        uint8_t after_msg_info = self->_msg_info_begin;

        //Find position for insertion
        while ((after_msg_info<msg_info_size)&&(msg_info_store[after_msg_info]._begin<seq_num)) {
            before_msg_info = after_msg_info;
            after_msg_info = msg_info_store[after_msg_info]._next;
        }
        //Drop new message packet if it overlaps with declared messages
        if ((after_msg_info<msg_info_size)&&(seq_num+new_msg_size>msg_info_store[after_msg_info]._begin))
            return WIO_ERR_INVALID;

        //Look for spare message information item
        uint8_t index;
        for (index=0;index<msg_info_size;index++)
            if (!msg_info_store[index]._in_use)
                break;
        //No item available
        if (index>=msg_info_size)
            return WIO_ERR_NO_MEMORY;
        //Initialize message information item
        msg_info_store[index]._in_use = true;
        msg_info_store[index]._begin = seq_num;
        msg_info_store[index]._size = new_msg_size;
        //Insert message information
        if (before_msg_info<msg_info_size)
            msg_info_store[before_msg_info]._next = index;
        else
            self->_msg_info_begin = index;
        msg_info_store[index]._next = after_msg_info;
    }

    //Data fragments buffer
    wio_buf_t* fragments_buf = &self->_fragments_buf;
    //Data fragments before and after insertion position
    wtp_rx_fragment_t* fragment_a = NULL;
    wtp_rx_fragment_t* fragment_b = self->_fragments_begin;
    //New data fragment
    wtp_rx_fragment_t* new_fragment;
    uint8_t* new_fragment_data;

    //Find position for insertion
    while (fragment_b&&(fragment_b->_seq_num<seq_num)) {
        fragment_a = fragment_b;
        fragment_b = fragment_b->_next;
    }
    //Drop data packet if it overlaps with other data fragments
    if (fragment_b&&(seq_num+size>fragment_b->_seq_num))
        return WIO_ERR_INVALID;

    //Allocate memory for new data fragment
    WIO_TRY(wio_alloc(
        fragments_buf,
        sizeof(wtp_rx_fragment_t)+size,
        &new_fragment
    ))
    //New fragment data memory
    new_fragment_data = (uint8_t*)new_fragment+sizeof(wtp_rx_fragment_t);
    //Initialize data fragment
    new_fragment->_seq_num = seq_num;
    new_fragment->_data = new_fragment_data;
    new_fragment->_size = size;
    new_fragment->_assembled = false;
    //Copy fragment data
    memcpy(new_fragment_data, data, size);
    //Insert data fragment
    if (fragment_a)
        fragment_a->_next = new_fragment;
    else
        self->_fragments_begin = new_fragment;
    new_fragment->_next = fragment_b;

    //Message data buffer
    wio_buf_t* msg_data_buf = &self->_msg_data_buf;
    //Number of new messages
    uint8_t n_msgs = 0;
    //Data fragments linked list item
    fragment_a = self->_fragments_begin;

    //Move acknowledged message data
    memmove(
        msg_data_buf->buffer,
        msg_data_buf->buffer+msg_data_buf->pos_a,
        msg_data_buf->pos_b-msg_data_buf->pos_a
    );
    //Update read and write cursor position
    msg_data_buf->pos_b -= msg_data_buf->pos_a;
    msg_data_buf->pos_a = 0;

    while (fragment_a) {
        //Message begin check
        if (self->_msg_info_begin<msg_info_size) {
            wtp_rx_msg_info_t* current_msg_info = msg_info_store+self->_msg_info_begin;

            if (self->_seq_num==current_msg_info->_begin) {
                //Write size of next message
                WIO_TRY(wio_write(msg_data_buf, &current_msg_info->_size, 2))
            }
        }

        //Append consecutive data fragments
        if (fragment_a->_seq_num==self->_seq_num) {
            //Copy fragment data
            WIO_TRY(wio_write(msg_data_buf, fragment_a->_data, fragment_a->_size))
            //Update sequence number
            self->_seq_num += fragment_a->_size;

            //Set assembled flag
            fragment_a->_assembled = true;
        } else
            break;

        //Message end check
        if (self->_msg_info_begin<msg_info_size) {
            wtp_rx_msg_info_t* current_msg_info = msg_info_store+self->_msg_info_begin;
            uint16_t current_msg_end = current_msg_info->_begin+current_msg_info->_size;

            if (self->_seq_num==current_msg_end) {
                //Release current item
                current_msg_info->_in_use = false;
                //Update begin of linked list
                self->_msg_info_begin = current_msg_info->_next;
                //Update number of messages fully received
                n_msgs++;
            }
        }

        //Move to next fragment
        fragment_a = fragment_a->_next;
    }
    //Update begin of data fragments linked list
    self->_fragments_begin = fragment_a;

    //Remove assembled data fragments
    while (fragments_buf->pos_a!=fragments_buf->pos_b) {
        //Next data fragment in buffer
        fragment_a = (wtp_rx_fragment_t*)(fragments_buf->buffer+fragments_buf->pos_a);

        //Already assembled; release fragment memory
        if (fragment_a->_assembled)
            WIO_TRY(wio_free(fragments_buf, sizeof(wtp_rx_fragment_t)+fragment_a->_size))
        else
            break;
    }

    //Return value
    WIO_RETURN(_n_msgs, n_msgs)

    return WIO_OK;
}
