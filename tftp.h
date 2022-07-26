/**
 * @file tftp.h
 * @brief TFTP Header File.
 * @version 0.1.0
 * @author Sang-sik Kim
 */
#ifndef __TFTP_H__
#define __TFTP_H__
 
#include <stdint.h>
#include "utility.h"
#include "w5500.h"
#include "socket.h"
#include "w5500_conf.h"
#include <string.h>
#include "tty.h"
 
 
/* tftp message */
#define TFTP_RRQ		1
#define TFTP_WRQ		2
#define TFTP_DATA		3
#define TFTP_ACK		4
#define TFTP_ERROR		5
#define TFTP_OACK		6
 
/* tftp state */
#define STATE_NONE		0
#define STATE_RRQ		1
#define STATE_WRQ		2
#define STATE_DATA		3
#define STATE_ACK		4
#define STATE_OACK		5
 
/* tftp transfer mode */
#define TRANS_ASCII		"netascii"
#define TRANS_BINARY	"octet"
 
/* tftp progress state */
#define TFTP_PROGRESS	0
#define TFTP_FAIL		1
#define TFTP_SUCCESS	2
 
/* define */
#define TFTP_SERVER_PORT		69
#define TFTP_TEMP_PORT			51000
#define TFTP_BLK_SIZE			512
#define MAX_MTU_SIZE			1514
#define FILE_NAME_SIZE			20
 
//#define __TFTP_DEBUG__
 
/* typedef */ 
typedef struct tftp_data 
{
	uint8_t opcode[2];
	uint8_t block_num[2];
	uint8_t data[1024];
} TFTP_DATA_T;
 
typedef struct tftp_error 
{
	uint8_t opcode[2];
	uint8_t error_code[2];
	uint8_t error_msg[1024];
} TFTP_ERROR_T;
 
typedef struct tftp_option 
{
	uint8_t *code;
	uint8_t *value;
} TFTP_OPTION;
 
 
extern int tftp_get(struct tty *ptty ,char *filename,uint32_t *getbuf,uint32_t *getlen,uint32_t getmax);
extern int tftp_send(struct tty *ptty , uint32_t *mem , char *filename , unsigned long setlen);
#endif	/*__TFTP_H__ */