#include <string.h>
#include <stdbool.h>
#include <runtime.h>
#include "wisp-base.h"

//WISP ID
uint16_t ert_wisp_id = 0;

//Blockwrite data buffer
static uint8_t blockwrite_buffer[] = {0};
//WISP data
static WISP_dataStructInterface_t wisp_data;

//WTP endpoint
static wtp_t _wtp_ep;
wtp_t* ert_wtp_ep = &_wtp_ep;
//u-RPC endpoint
static urpc_t _rpc_ep;
urpc_t* ert_rpc_ep = &_rpc_ep;

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
    //TODO: Read size
    wtp_read_hook(
        ert_wtp_ep,
        wisp_data.readBufPtr,
        24
    );
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
    wtp_blockwrite_hook(
        ert_wtp_ep,
        (uint8_t*)(wisp_data.blockWriteBufPtr),
        *wisp_data.blockWriteSizePtr
    );
}

static WIO_CALLBACK(keep_recv) {
    wtp_recv(ert_wtp_ep, NULL, keep_recv);

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
void ert_main(void) {
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
    memset(wisp_data.epcBuf, 0, 12);
    memcpy(wisp_data.epcBuf, &ert_wisp_id, 2);

    //Initialize WTP endpoint
    wtp_init(
        ert_wtp_ep,
        wisp_data.epcBuf+2,
        10,
        blockwrite_buffer,
        sizeof(blockwrite_buffer),
        32,
        96,
        128,
        wtp_xor_checksum
    );
    //Initialize u-RPC endpoint
    urpc_init(
        ert_rpc_ep,
        16,
        192,
        64,
        ert_wtp_ep,
        (urpc_send_func_t)wtp_send,
        8
    );

    //RFID loop
    while (true)
        WISP_doRFID();
}
