/** @file
  RPMB definitions copied as is from optee_os/core/tee/tee_rpmb_fs.c

  Copyright (c) 2014, STMicroelectronics International N.V.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
**/
#ifndef __TEE_RPMB_FS_H__
#define __TEE_RPMB_FS_H__

/*
 * Lower interface to RPMB device
 */

#define RPMB_DATA_OFFSET            (RPMB_STUFF_DATA_SIZE + RPMB_KEY_MAC_SIZE)
#define RPMB_MAC_PROTECT_DATA_SIZE  (RPMB_DATA_FRAME_SIZE - RPMB_DATA_OFFSET)

#define RPMB_MSG_TYPE_REQ_AUTH_KEY_PROGRAM          0x0001
#define RPMB_MSG_TYPE_REQ_WRITE_COUNTER_VAL_READ    0x0002
#define RPMB_MSG_TYPE_REQ_AUTH_DATA_WRITE           0x0003
#define RPMB_MSG_TYPE_REQ_AUTH_DATA_READ            0x0004
#define RPMB_MSG_TYPE_REQ_RESULT_READ               0x0005
#define RPMB_MSG_TYPE_RESP_AUTH_KEY_PROGRAM         0x0100
#define RPMB_MSG_TYPE_RESP_WRITE_COUNTER_VAL_READ   0x0200
#define RPMB_MSG_TYPE_RESP_AUTH_DATA_WRITE          0x0300
#define RPMB_MSG_TYPE_RESP_AUTH_DATA_READ           0x0400

#define RPMB_STUFF_DATA_SIZE                        196
#define RPMB_KEY_MAC_SIZE                           32
#define RPMB_DATA_SIZE                              256
#define RPMB_NONCE_SIZE                             16
#define RPMB_DATA_FRAME_SIZE                        512

#define RPMB_RESULT_OK                              0x00
#define RPMB_RESULT_GENERAL_FAILURE                 0x01
#define RPMB_RESULT_AUTH_FAILURE                    0x02
#define RPMB_RESULT_COUNTER_FAILURE                 0x03
#define RPMB_RESULT_ADDRESS_FAILURE                 0x04
#define RPMB_RESULT_WRITE_FAILURE                   0x05
#define RPMB_RESULT_READ_FAILURE                    0x06
#define RPMB_RESULT_AUTH_KEY_NOT_PROGRAMMED         0x07
#define RPMB_RESULT_MASK                            0x3F
#define RPMB_RESULT_WR_CNT_EXPIRED                  0x80

 /* RPMB internal commands */
#define RPMB_CMD_DATA_REQ      0x00
#define RPMB_CMD_GET_DEV_INFO  0x01

/* Error codes for get_dev_info request/response. */
#define RPMB_CMD_GET_DEV_INFO_RET_OK     0x00
#define RPMB_CMD_GET_DEV_INFO_RET_ERROR  0x01

struct rpmb_data_frame {
  uint8_t stuff_bytes[RPMB_STUFF_DATA_SIZE];
  uint8_t key_mac[RPMB_KEY_MAC_SIZE];
  uint8_t data[RPMB_DATA_SIZE];
  uint8_t nonce[RPMB_NONCE_SIZE];
  uint8_t write_counter[4];
  uint8_t address[2];
  uint8_t block_count[2];
  uint8_t op_result[2];
  uint8_t msg_type[2];
};

struct rpmb_req {
  uint16_t cmd;
  uint16_t dev_id;
  uint16_t block_count;
  /* variable length of data */
  /* uint8_t data[]; REMOVED! */
};

#define TEE_RPMB_REQ_DATA(req) \
  ((void *)((struct rpmb_req *)(req) + 1))

#define RPMB_EMMC_CID_SIZE 16
struct rpmb_dev_info {
  uint8_t cid[RPMB_EMMC_CID_SIZE];
  /* EXT CSD-slice 168 "RPMB Size" */
  uint8_t rpmb_size_mult;
  /* EXT CSD-slice 222 "Reliable Write Sector Count" */
  uint8_t rel_wr_sec_c;
  /* Check the ret code and accept the data only if it is OK. */
  uint8_t ret_code;
};

#endif // __TEE_RPMB_FS_H__