#include <string.h>
#include <stdbool.h>
#include "wisp-base.h"

//Blockwrite data buffer
static uint8_t blockwrite_buffer[] = {0};
//WISP data
static WISP_dataStructInterface_t wisp_data;

//WISP ID
uint16_t ert_wisp_id = 0;

//WTP endpoint
static wtp_t _wtp_ep;
wtp_t* ert_wtp_ep = &_wtp_ep;
//u-RPC endpoint
static urpc_t _rpc_ep;
urpc_t* ert_rpc_ep = &_rpc_ep;

//WISP machine context
static ert_mcontext_t _ctx;
ert_mcontext_t* ctx = &_ctx;

/**
 * WISP RFID acknowledgement callback.
 */
static void ert_ack_callback(void) {
    asm(" NOP");
}

/**
 * WISP RFID Read callback.
 */
static void ert_read_callback(void) {

}

/**
 * WISP RFID Write callback.
 */
static void ert_write_callback(void) {
    asm(" NOP");
}

/**
 * WISP RFID Blockwrite callback.
 */
static void ert_blockwrite_callback(void) {
    
}

static WIO_CALLBACK(ert_resume_exec) {

}

void main(void) {
    //Initialize WISP firmware
    WISP_init();

    //Register RFID callback functions
    WISP_registerCallback_ACK(ert_ack_callback);
    WISP_registerCallback_READ(ert_read_callback);
    WISP_registerCallback_WRITE(ert_write_callback);
    WISP_registerCallback_BLOCKWRITE(ert_blockwrite_callback);

    //Blockwrite data buffer
    RWData.bwrBufPtr = (uint16_t*)blockwrite_buffer;

    //Get access to WISP data
    WISP_getDataBuffers(&wisp_data);

    //Set up operating parameters for WISP comm routines
    WISP_setMode(MODE_READ|MODE_WRITE|MODE_USES_SEL);
    WISP_setAbortConditions(CMD_ID_READ|CMD_ID_WRITE|CMD_ID_ACK);
    //Set up EPC
    memset(wisp_data.epcBuf, 0, RFID_EPC_LEN);
    memcpy(wisp_data.epcBuf, &ert_wisp_id, 2);


    //RFID loop
    while (FOREVER)
        WISP_doRFID();
}
