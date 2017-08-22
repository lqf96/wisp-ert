#include <stdbool.h>
#include <string.h>
#include <wisp-base.h>
#include <wtp.h>
#include <ert/urpc.h>
#include <ert/rpc.h>
#include <ert/runtime.h>

//WISP ID
uint16_t ert_wisp_id = 1;

//WTP endpoint
wtp_t* ert_wtp_ep = WIO_INST_PTR(wtp_t);
//u-RPC endpoint
urpc_t* ert_rpc_ep = WIO_INST_PTR(urpc_t);

//Blockwrite data buffer
static uint8_t blockwrite_buffer[_ERT_BW_SIZE] = {0};
//WISP data
static WISP_dataStructInterface_t wisp_data;

//Read action flag
static bool read_flag = false;
//Blockwrite action flag
static bool blockwrite_flag = false;

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

static WIO_CALLBACK(ert_on_connect) {
    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
void ert_async_suspend(
    ucontext_t* async_ctx,
    ert_status_t* _status,
    void** _result
) {

}

/**
 * {@inheritDoc}
 */
WIO_CALLBACK(ert_async_resume) {
    //TODO: Async resume
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
    WISP_setAbortConditions(CMD_ID_READ|CMD_ID_WRITE|CMD_ID_ACK);

    //Set up EPC
    memset(wisp_data.epcBuf, 0, ERT_EPC_SIZE);
    //WISP class
    wisp_data.epcBuf[0] = ERT_WISP_CLASS;
    //WISP ID
    memcpy(wisp_data.epcBuf+1, &ert_wisp_id, 2);

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
        //(The first three bytes are for WISP class and WISP ID)
        wisp_data.epcBuf+3,
        //EPC memory size
        ERT_EPC_SIZE-3,
        //RFID Read memory
        (uint8_t*)wisp_data.readBufPtr,
        //RFID Write memory
        //(The first byte is for BlockWrite data length)
        blockwrite_buffer+1,
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
        //WTP before RFID hook
        wtp_before_do_rfid(ert_wtp_ep);

        //Do RFID
        WISP_doRFID();

        //Called after a Read operation
        if (read_flag) {
            wtp_after_read(ert_wtp_ep);
            read_flag = false;
        }
        //Called after a BlockWrite operation
        if (blockwrite_flag) {
            wtp_handle_blockwrite(ert_wtp_ep, blockwrite_buffer[0]);
            blockwrite_flag = false;
        }
    }
}
