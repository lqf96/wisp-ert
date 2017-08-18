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
    uint16_t pkt_buf_size,
    uint16_t msg_buf_size
) {
    //Sequence number
    self->_seq_num = 0;
    //Window size
    self->_window_size = window_size;
    //Tineout
    self->_timeout = timeout;

    //Packet buffer
    uint8_t* pkt_mem = malloc(pkt_buf_size);
    if (!pkt_mem)
        return WIO_ERR_NO_MEMORY;
    WIO_TRY(wio_buf_init(&self->_pkt_buf, pkt_mem, pkt_buf_size))
    //Message buffer
    uint8_t* msg_mem = malloc(msg_buf_size);
    if (!msg_mem)
        return WIO_ERR_NO_MEMORY;
    WIO_TRY(wio_buf_init(&self->_msg_buf, msg_mem, msg_buf_size))

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
wtp_status_t wtp_begin_pkt(
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
 * End construction of WTP packet.
 *
 * @param self WTP endpoint instance
 */
wtp_status_t wtp_end_pkt(
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
    uint8_t* msg_data_mem = malloc(msg_data_size);
    if (!msg_data_mem)
        return WIO_ERR_NO_MEMORY;
    WIO_TRY(wio_buf_init(&self->_msg_data_buf, msg_data_mem, msg_data_size))

    //Data fragments buffer
    uint8_t* fragments_mem = malloc(fragments_size);
    if (!fragments_mem)
        return WIO_ERR_NO_MEMORY;
    WIO_TRY(wio_buf_init(&self->_fragments_buf, fragments_mem, fragments_size))
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
