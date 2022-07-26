#include "stm32f4xx_hal.h"
#include "iinchip_conf.h"

#define TFTP_SOCKET_NUM                                (MAX_SOCK_NUM-1)//7
#define DEBUG_SOCKET_NUM                        (MAX_SOCK_NUM-2)//6
#define UDP_ECHO_SOCKET_NUM                        (MAX_SOCK_NUM-3)//5
#define TCP_ECHO_SOCKET_NUM                        (MAX_SOCK_NUM-4)//4

#define TFTP_WELLKNOWN_PORT                        69
#define TFTP_LOCAL_PORT                                5000//local port for tftp
#define DEBUG_LOCAL_PORT                        5100//local and remote port for debug_printf
#define UDP_ECHO_LOCAL_PORT                        5200//local port for udp echo
#define TCP_ECHO_LOCAL_PORT                        5300//local port for tcp echo


#define TFTP_OPCODE_LEN                                2
#define TFTP_BLKNUM_LEN                                2
#define TFTP_DATA_LEN_MAX                        512
#define TFTP_DATA_PKT_HDR_LEN                (TFTP_OPCODE_LEN + TFTP_BLKNUM_LEN)
#define TFTP_ERR_PKT_HDR_LEN                (TFTP_OPCODE_LEN + TFTP_ERRCODE_LEN)
#define TFTP_ACK_PKT_LEN                        (TFTP_OPCODE_LEN + TFTP_BLKNUM_LEN)
#define TFTP_DATA_PKT_LEN_MAX                (TFTP_DATA_PKT_HDR_LEN + TFTP_DATA_LEN_MAX)
#define TFTP_MAX_RETRIES                        3
#define TFTP_TIMEOUT_INTERVAL                5

/*
typedef struct
{
  int op;    //WRQ
  // last block read
  char data[TFTP_DATA_PKT_LEN_MAX];
  int  data_len;
  // destination ip:port
  unsigned int to_ip;
  int to_port;
  // next block number
  int block;
  // total number of bytes transferred
  int tot_bytes;
  // timer interrupt count when last packet was sent
  // this should be used to resend packets on timeout
  unsigned long long last_time;
}tftp_connection_args;
*/


/* TFTP opcodes as specified in RFC1350   */
typedef enum {
  TFTP_RRQ = 1,
  TFTP_WRQ = 2,
  TFTP_DATA = 3,
  TFTP_ACK = 4,
  TFTP_ERROR = 5
} tftp_opcode;


/* TFTP error codes as specified in RFC1350  */
typedef enum {
  TFTP_ERR_NOTDEFINED,
  TFTP_ERR_FILE_NOT_FOUND,
  TFTP_ERR_ACCESS_VIOLATION,
  TFTP_ERR_DISKFULL,
  TFTP_ERR_ILLEGALOP,
  TFTP_ERR_UKNOWN_TRANSFER_ID,
  TFTP_ERR_FILE_ALREADY_EXISTS,
  TFTP_ERR_NO_SUCH_USER,
} tftp_errorcode;


extern uint8_t ip[4];                                // for setting SIP register
extern uint8_t gw[4];                                // for setting GAR register
extern uint8_t sn[4];                                // for setting SUBR register
extern uint8_t serverip[4];                        // "TCP SERVER" IP address

extern uint8_t tftp_version_info[1024];
extern uint8_t tftp_frame_buffer[1024];

int tftpclient_version_check(void);
int tftpclient_fireware_exist_check(void);
int tftpclient_fireware_update(void);