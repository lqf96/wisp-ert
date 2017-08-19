#include <stdbool.h>
#include <string.h>
#include <wisp-base.h>
#include <wtp.h>
#include <ert/runtime.h>

//WISP ID
uint16_t ert_wisp_id;

//Blockwrite data buffer
static uint8_t blockwrite_buffer[_ERT_BW_SIZE] = {0};
//WISP data
static WISP_dataStructInterface_t wisp_data;

//Read action flag
static bool read_flag = false;
//Blockwrite action flag
static bool blockwrite_flag = false;

//WTP endpoint
static wtp_t* wtp_ep = WIO_INST_PTR(wtp_t);

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
 * {@inheritDoc}
 */
void main(void) {
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

    /*/Initialize u-RPC endpoint
    urpc_init(
        ert_rpc_ep,
        16,
        192,
        64,
        ert_wtp_ep,
        (urpc_send_func_t)wtp_send,
        8
    );*/

    //Initialize WTP endpoint
    wtp_init(
        //WTP endpoint
        wtp_ep,
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
    //Connect to WTP server
    wtp_connect(wtp_ep);

    //RFID loop
    while (true) {
        //WTP before RFID hook
        wtp_before_do_rfid(wtp_ep);

        //Do RFID
        WISP_doRFID();

        //Called after a Read operation
        if (read_flag) {
            wtp_after_read(wtp_ep);
            read_flag = false;
        }
        //Called after a BlockWrite operation
        if (blockwrite_flag) {
            wtp_handle_blockwrite(wtp_ep, blockwrite_buffer[0]);
            blockwrite_flag = false;
        }
    }
}
