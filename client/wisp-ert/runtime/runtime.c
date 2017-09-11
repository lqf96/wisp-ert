#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <wisp-base.h>
#include <ert/rpc.h>
#include <ert/runtime.h>

//WISP ID
uint16_t ert_wisp_id = 0x5101;

//WTP endpoint
wtp_t* ert_wtp_ep = WIO_INST_PTR(wtp_t);
//u-RPC endpoint
urpc_t* ert_rpc_ep = WIO_INST_PTR(urpc_t);

//Blockwrite data buffer
static uint8_t blockwrite_buffer[_ERT_BW_SIZE] = {0};
//WISP data
static WISP_dataStructInterface_t wisp_data;

//RFID Read flag
static bool read_flag = false;
//RFID BlockWrite flag
static bool blockwrite_flag = false;

//ERT runtime context
static ucontext_t runtime_ctx;
//ERT user context
static ucontext_t user_ctx;

//User context status variable
static ert_status_t* user_status_var;
//User context result variable
static void* user_result_var;
//User context result size
static uint16_t user_result_size;

//EPC update counter
static uint8_t epc_update_counter = 0;

/**
 * WISP RFID Read callback.
 */
static void ert_read_callback(void) {
    read_flag = true;
}

/**
 * WISP RFID Blockwrite callback.
 */
static void ert_blockwrite_callback(void) {
    blockwrite_flag = true;
}

/**
 * Function that starts ERT user main routine
 */
static WIO_CALLBACK(ert_enter_main) {
    void** rpc_results = (void**)result;
    //Remote constants
    urpc_vary_t* remote_consts = (urpc_vary_t*)(rpc_results[1]);
    //Initialize ERT constants with remote constants
    memcpy(&ert_consts_store, remote_consts->data, remote_consts->size);

    //No user main routine found
    if (!ert_main)
        return WIO_OK;

    //Allocate memory for user stack
    uint32_t* user_stack = malloc(ERT_USER_STACK_SIZE);
    if (!user_stack)
        return WIO_ERR_NO_MEMORY;
    //Set up user context
    user_ctx.uc_stack.ss_sp = user_stack;
    user_ctx.uc_stack.ss_size = ERT_USER_STACK_SIZE;
    makecontext(&user_ctx, ert_main, 0);

    //Jump to user context
    swapcontext(&runtime_ctx, &user_ctx);

    return WIO_OK;
}

/**
 * ERT WTP data received callback.
 */
static WIO_CALLBACK(ert_on_recv) {
    //Keep receiving data from WTP endpoint
    WIO_TRY(wtp_recv(ert_wtp_ep, data, ert_on_recv))
    //Call u-RPC data received callback
    WIO_TRY(urpc_on_recv(data, status, result))

    return WIO_OK;
}

/**
 * ERT WTP connected callback.
 */
static WIO_CALLBACK(ert_on_connect) {
    //Keep receiving data from WTP endpoint
    WIO_TRY(wtp_recv(ert_wtp_ep, ert_rpc_ep, ert_on_recv))
    //Load ERT constants
    WIO_TRY(urpc_call(
        ert_rpc_ep,
        ert_func_srv_consts,
        URPC_SIG(1, URPC_TYPE_VARY),
        URPC_ARG(URPC_VARY_CONST("fs", 2)),
        NULL,
        ert_enter_main
    ))

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
void ert_user_suspend(
    ert_status_t* _status_var,
    uint16_t result_size,
    void* _result_var
) {
    //Set status and result variable
    user_status_var = _status_var;
    user_result_var = _result_var;
    //Set result size
    user_result_size = result_size;

    //Jump to runtime context
    swapcontext(&user_ctx, &runtime_ctx);
}

/**
 * {@inheritDoc}
 */
WIO_CALLBACK(ert_user_resume) {
    //Set status
    *user_status_var = status;
    //Copy result
    memcpy(user_result_var, result, user_result_size);

    //Jump to user context
    swapcontext(&runtime_ctx, &user_ctx);

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
void main(void) {
    //Call pre-init hook
    if (ert_pre_init)
        ert_pre_init();

    //Initialize WISP firmware
    WISP_init();

    //Register RFID callback functions
    WISP_registerCallback_READ(ert_read_callback);
    WISP_registerCallback_BLOCKWRITE(ert_blockwrite_callback);

    //Blockwrite data buffer
    RWData.bwrBufPtr = (uint16_t*)blockwrite_buffer;

    //Get access to WISP data
    WISP_getDataBuffers(&wisp_data);

    //Set up operating parameters for WISP comm routines
    WISP_setMode(MODE_READ|MODE_WRITE|MODE_USES_SEL);
    WISP_setAbortConditions(CMD_ID_READ|CMD_ID_WRITE|CMD_ID_BLOCKWRITE|CMD_ID_ACK);

    //Set up EPC
    memset(wisp_data.epcBuf, 0, ERT_EPC_SIZE);
    //WISP ID
    memcpy(wisp_data.epcBuf, &ert_wisp_id, 2);

    //Initialize u-RPC endpoint
    urpc_init(
        //u-RPC endpoint
        ert_rpc_ep,
        //Function table size
        16,
        //Send buffer size
        96,
        //Temporary buffer size
        32,
        //Send function closure data
        ert_wtp_ep,
        //Send function
        (urpc_send_func_t)wtp_send,
        //Capacity of callback table
        8
    );

    //Initialize WTP endpoint
    wtp_init(
        //WTP endpoint
        ert_wtp_ep,
        //EPC memory
        //(The first two bytes are for WISP ID)
        wisp_data.epcBuf+2,
        //EPC memory size
        ERT_EPC_SIZE-2,
        //RFID Read memory
        (uint8_t*)wisp_data.readBufPtr,
        //RFID Write memory
        blockwrite_buffer,
        //Sliding window size
        64,
        //Timeout
        10,
        //Buffer size for transmit control
        200,
        //Buffer size for receving control
        200,
        //Capacity of sending messages
        5,
        //Capacity of receiving messages
        5
    );

    //WTP connected event handler
    wtp_on_event(ert_wtp_ep, WTP_EVENT_OPEN, NULL, ert_on_connect);
    //Connect to WTP server
    wtp_connect(ert_wtp_ep);

    //RFID loop
    while (true) {
        //TODO: Figure out why moving following EPC update code to a function doesn't work
        //TODO: Encapsulate following code in function "wtp_before_do_rfid()"

        //Update counter
        epc_update_counter++;

        //Triggered every 16 RFID operations
        if (epc_update_counter%16==1) {
            //Send packet buffer
            wio_buf_t* pkt_buf = &ert_wtp_ep->_tx_ctrl._pkt_buf;

            //Write packets to EPC memory
            if (pkt_buf->pos_a!=pkt_buf->pos_b) {
                //EPC buffer
                wio_buf_t* epc_buf = &ert_wtp_ep->_epc_buf;
                //Packet size
                uint8_t pkt_size;

                //Reset EPC buffer
                epc_buf->pos_a = epc_buf->pos_b = 0;

                //Fill EPC buffer with data packets
                while (pkt_buf->pos_a<pkt_buf->pos_b) {
                    //Packet size
                    pkt_size = *(pkt_buf->buffer+pkt_buf->pos_a);

                    //Exceeds EPC capacity
                    if (epc_buf->pos_a+pkt_size>epc_buf->size)
                        break;
                    //Skip packet size
                    pkt_buf->pos_a++;
                    //Copy packet data to EPC memory
                    wio_copy(pkt_buf, epc_buf, pkt_size);
                }
                //Write WTP_PKT_END (Ignore failure)
                wio_write(epc_buf, &WTP_PKT_END, 1);
                //Reset packet buffer if it's empty
                if (pkt_buf->pos_a==pkt_buf->pos_b)
                    pkt_buf->pos_a = pkt_buf->pos_b = 0;
            }
        }

        //Do RFID
        WISP_doRFID();

        //Called after a Read operation
        if (read_flag) {
            wtp_load_read_mem(ert_wtp_ep);
            read_flag = false;
        }
        //Called after a BlockWrite operation
        if (blockwrite_flag) {
            wtp_handle_blockwrite(ert_wtp_ep);
            blockwrite_flag = false;
        }
    }
}
