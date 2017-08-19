#pragma once

#include <stdint.h>

//=== ERT constants ===
//Blockwrite data buffer size
#define _ERT_BW_SIZE 0x20
static const uint8_t ERT_BW_SIZE = _ERT_BW_SIZE;
//EPC size
static const uint8_t ERT_EPC_SIZE = 12;
//WISP class
static const uint8_t ERT_WISP_CLASS = 0x10;

//=== ERT runtime APIs ===
/**
 * WISP program entry point.
 */
extern void main(void);
