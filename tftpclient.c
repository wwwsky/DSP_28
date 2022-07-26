*
*********************************************************************************************************
*   /,   ..  , \    | File     : tftpclient.c
*  /// .::::. \\\   | Descript : basic tftp client implementation for IAP
* ///\ :::::: /\\\  | Version  : V0.10
*********************************************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "bsp_common.h"
#include "socket.h"
#include "tftpclient.h"
#include "flash_if.h"
#include <string.h>
#include <stdio.h>

/* Private variables ---------------------------------------------------------*/
/* RNG handler declaration */
RNG_HandleTypeDef RngHandle;
/* W5300 configue info */
uint8_t tx_mem_conf[8] = {8,8,8,8,8,8,8,8};        // for setting TMSR regsiter
uint8_t rx_mem_conf[8] = {8,8,8,8,8,8,8,8};        // for setting RMSR regsiter
uint8_t ip[4] = {192,168,1,200};                                // for setting SIP register
uint8_t gw[4] = {192,168,1,1};                                // for setting GAR register
uint8_t sn[4] = {255,255,0,0};                                // for setting SUBR register
uint8_t serverip[4] = {192,168,1,2};                        // "TCP SERVER" IP address
uint8_t mac[6] = {0x55,0x66,0x88,0x00,0x00,0x00};// for setting SHAR register

/* Private variables ---------------------------------------------------------*/
uint32_t  wait_timeout;
uint32_t  Flash_Write_Address;
uint32_t  tftp_prev_block, tftp_cur_block;
uint16_t  tftp_remote_port;
uint8_t   tftp_version_info[1024];
uint8_t   tftp_frame_buffer[1024];

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/


/**
* nicholasldf: stole from lwip - Convert an uint16_t from network to host byte order.
* @param  n u16_t in network byte order
* @return n in host byte order
*/
static uint16_t ByteOrder(uint16_t n)
{
        return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

/**
  * @brief Sets the TFTP opcode
  * @param  buffer: pointer on the TFTP packet
  * @param  opcode: TFTP opcode
  * @retval none
  */
static void tftp_set_opcode(uint8_t *buffer, tftp_opcode opcode)
{
        buffer[0] = 0;
        buffer[1] = (uint8_t)opcode;
}

/**
  * @brief Sets the TFTP block number
  * @param packet: pointer on the TFTP packet
  * @param  block: block number
  * @retval none
  */
static void tftp_set_block(uint8_t *buffer, uint16_t block)
{
        uint16_t *p = (uint16_t *)buffer;
        p[1] = ByteOrder(block);
}

/**
  * @brief Sends TFTP ACK packet  
  * @param to: pointer on the receive IP address structure
  * @param to_port: receive port number
  * @param block: block number
  * @retval: err_t: error code
  */
static void tftp_send_ack_packet(int block)
{
        /* define the first two bytes of the packet */
        tftp_set_opcode(tftp_frame_buffer, TFTP_ACK);
       
        /* Specify the block number being ACK'd.
        * If we are ACK'ing a DATA pkt then the block number echoes that of the DATA pkt being ACK'd (duh)
        * If we are ACK'ing a WRQ pkt then the block number is always 0
        * RRQ packets are never sent ACK pkts by the server, instead the server sends DATA pkts to the
        * host which are, obviously, used as the "acknowledgement".  This saves from having to sEndTransferboth
        * an ACK packet and a DATA packet for RRQs - see RFC1350 for more info.  */
        tftp_set_block(tftp_frame_buffer, block);
       
        /* Sending packet by UDP protocol */
        sendto(TFTP_SOCKET_NUM, tftp_frame_buffer, 4, serverip, tftp_remote_port);
}

/**
  * @brief Sends TFTP error packet  
  * @param errorcode: error code indicating the nature of the error
  * @param errorStr: human consumption infomation
  */
static void tftp_send_error_packet(unsigned short errorcode, char *errorStr)
{
        int len;
       
        /* define the first two bytes of the packet */
        tftp_set_opcode(tftp_frame_buffer, TFTP_ERROR);
       
        //set errorcode
        tftp_frame_buffer[2] = 0;
        tftp_frame_buffer[3] = errorcode;
        //set errorStr
        strcpy((char *)(tftp_frame_buffer+4), errorStr);
       
        len = strlen(errorStr) + 5;
        /* Sending packet by UDP protocol */
        sendto(TFTP_SOCKET_NUM, tftp_frame_buffer, len, serverip, tftp_remote_port);
}

/**
  * @brief Sends TFTP read file request packet  
  * @param tftp_filename: file name of the file which tftpclient want to read from tftpserver
  * @retval: err_t: error code
  */
void tftp_send_readfile_request(char *tftp_filename)
{
        unsigned char *pkt;
        unsigned short *s;
        int len = 0;
       
        //set opcode
        s = (unsigned short *)tftp_frame_buffer;
        *s++ = ByteOrder(TFTP_RRQ);
       
        //set filename
        pkt = (unsigned char *)s;
        strcpy((char *)pkt, tftp_filename);
        pkt += strlen(tftp_filename) + 1;
       
        //set mode
        strcpy((char *)pkt, "octet");
        pkt += 5 /*strlen("octet")*/ + 1;
       
        //set timeout
        strcpy((char *)pkt, "timeout");
        pkt += 7 /*strlen("timeout")*/ + 1;
        sprintf((char *)pkt, "%u", 10000 / 1000);//timeout_ms
       
        //set blksize
        pkt += strlen((char *)pkt) + 1;
        pkt += sprintf((char *)pkt, "blksize%c%d%c", 0, 512, 0);
        len = pkt - tftp_frame_buffer;
       
        Debug_Printf("tftp_send_readfile_request : %s\r\n", tftp_frame_buffer+2);
       
        //send packet
        //net_send_udp_packet(net_server_ethaddr, tftp_remote_ip, tftp_remote_port, tftp_our_port, len);
        sendto(TFTP_SOCKET_NUM, tftp_frame_buffer, len, serverip, tftp_remote_port);
}

/**
  * @brief  try to read version file form TFTP server and check whether needed to do fireware update
  * @param  none
  * @retval error code
  */
int tftpclient_version_check(void)
{
        unsigned char  destip[4];
        unsigned short destport, changflag;
        unsigned int len;
        unsigned short proto;
        char LocalVersion[32];
       
        tftp_remote_port = TFTP_WELLKNOWN_PORT;
        changflag = 0;
       
        Debug_Printf("tftpclient_version_check : entry\r\n");
       
        // close the SOCKET
        close(TFTP_SOCKET_NUM);
        // open the SOCKET with UDP mode
        socket(TFTP_SOCKET_NUM, Sn_MR_UDP, TFTP_LOCAL_PORT, 0);
       
        //requet read fireware file from server
        tftp_send_readfile_request("version.txt");
       
        while(1){
                //wait for server's respond
                wait_timeout = 0;
                while(1) {
                        //check the size of received data
                        len = getSn_RX_RSR(TFTP_SOCKET_NUM);
                        if(len > 0){
                                break;
                        }else if(wait_timeout > 3*100000){//3S
                                Debug_Printf("tftpclient_version_check : timeout, no respond from server\r\n");
                                return -1;//timeout: no respond from server
                        }else{
                                delay_us(10); wait_timeout++;
                        }
                }
               
                //receive data
                len = recvfrom(TFTP_SOCKET_NUM, tftp_version_info, 512, destip, &destport);
                //Debug_Printf((const char *)tftp_version_info);//for debug
               
                //check ip address
                if( (destip[0]!=serverip[0]) || (destip[1]!=serverip[1]) ||\
                        (destip[2]!=serverip[2]) || (destip[3]!=serverip[3]) ) {
                        Debug_Printf("tftpclient_version_check : destip error, %d.%d.%d.%d\r\n", destip[0],destip[1],destip[2],destip[3]);
                        continue;
                }
               
                //the port may change, due to server may reserve wellkonw UDP port for other client
                /* A requesting host chooses its source TID as described above, and sends
                its initial request to the known TID 69 decimal (105 octal) on the
                serving host.  The response to the request, under normal operation,
                uses a TID chosen by the server as its source TID and the TID chosen
                for the previous message by the requestor as its destination TID.
                The two chosen TID's are then used for the remainder of the transfer.
                */
                if((0 == changflag) && (destport != tftp_remote_port)) {
                        Debug_Printf("tftpclient_version_check : remote server port change from %d to %d\r\n", tftp_remote_port, destport);
                        tftp_remote_port = destport;
                        changflag = 1;
                }else if((1 == changflag) && (destport != tftp_remote_port)){
                        //error
                        Debug_Printf("tftpclient_version_check : Error port (data from invalid tid)\n");
                        tftp_send_error_packet(TFTP_ERR_UKNOWN_TRANSFER_ID, "Unknown TID");
                        continue;
                }
               
                //check packet len
                if (len < 2) return -1;
                len -= 2;
               
                /* get tftp cmd code */
                proto = *( (unsigned short *)tftp_version_info );
                proto = ByteOrder(proto);
                switch (proto) {
                        //-----------------------------------data packet-----------------------------------
                        case TFTP_DATA:{
                                //set len equal to effective data counts
                                if (len < 2) return -1;
                                len -= 2;
                               
                                /*
                                 * RFC1350 specifies that the first data packet will
                                 * have sequence number 1. If we receive a sequence
                                 * number of 0 this means that there was a wrap
                                 * around of the (16 bit) counter.
                                 */
                                tftp_cur_block = ByteOrder( *( (unsigned short *)(tftp_version_info+2) ) );
                                if (tftp_cur_block != 1) {
                                        Debug_Printf("tftpclient_version_check : error, tftp_cur_block(%d) != 1\r\n", tftp_cur_block);
                                        tftp_send_error_packet(TFTP_ERR_DISKFULL, "block number error");
                                        return -1;//version file should contain very few info
                                }
                               
                                /* check packet length */
                                if ( (0==len) && (len>512) ) {
                                        Debug_Printf("tftpclient_version_check : len(%d) error\r\n", len);
                                        tftp_send_error_packet(TFTP_ERR_NOTDEFINED, "data len error");
                                        return -1;//error
                                }
                               
                                /*
                                 *        Acknowledge the block just received, which will prompt
                                 *        the remote for the next one.
                                 */
                                tftp_send_ack_packet(tftp_cur_block);
                               
                                /* compare received version info with version info stored in the Flash */
                                memcpy(LocalVersion, (char *)APPLICATION_VERSION_ADDRESS, 26);
                                LocalVersion[26] = 0;
                                Debug_Printf("tftpclient_version_check : local  version - %s\r\n", LocalVersion);
                                Debug_Printf("tftpclient_version_check : server version - %s\r\n", tftp_version_info+4);
                                //version info format - stm32f4xx_earthquake_vx.xx
                                if(0 == strncmp(LocalVersion, (char *)(tftp_version_info+4), 26)) {
                                        Debug_Printf("tftpclient_version_check : skip to update fireware\r\n");
                                        return 1;//no need to update fireware
                                } else {
                                        Debug_Printf("tftpclient_version_check : need to update fireware\r\n");
                                        return 2;//need to update fireware
                                }
                                //break;
                        }
                        //-----------------------------------error packet-----------------------------------
                        case TFTP_ERROR:{
                                Debug_Printf("tftpclient_version_check : TFTP error, '%s' (%d)\n", \
                                        (tftp_version_info+4), ByteOrder(*(unsigned short *)(tftp_version_info+2)));
                                /*
                                switch (ByteOrder(*(unsigned short *)pkt)) {
                                case TFTP_ERR_FILE_NOT_FOUND:
                                case TFTP_ERR_ACCESS_DENIED:
                                        puts("Not retrying...\n");
                                        eth_halt();
                                        net_set_state(NETLOOP_FAIL);
                                        break;
                                case TFTP_ERR_UNDEFINED:
                                case TFTP_ERR_DISK_FULL:
                                case TFTP_ERR_UNEXPECTED_OPCODE:
                                case TFTP_ERR_UNKNOWN_TRANSFER_ID:
                                case TFTP_ERR_FILE_ALREADY_EXISTS:
                                default:
                                        puts("Starting again\n\n");
                                        net_start_again();
                                        break;
                                }*/
                                return -1;
                                //break;
                        }
                        default:{
                                Debug_Printf("tftpclient_version_check : Illegal operation code(0x%x)\n", proto);
                                tftp_send_error_packet(TFTP_ERR_ILLEGALOP, "Illegal operation");
                                break;
                        }
                }//end switch loop
        }//end while loop
}


/**
  * @brief  try to read fireware file form TFTP server and write to stm32 flash
  * @param  none
  * @retval error code
  */
int tftpclient_fireware_update(void)
{
        unsigned char  destip[4];
        unsigned short destport, changflag;
        unsigned int len;
        unsigned short proto;
       
        Flash_Write_Address = APPLICATION_START_ADDRESS;
        tftp_prev_block = 0;
        tftp_cur_block = 0;
        tftp_remote_port = TFTP_WELLKNOWN_PORT;
        changflag = 0;
       
        Debug_Printf("tftpclient_fireware_update : entry\r\n");
       
        // close the SOCKET
        close(TFTP_SOCKET_NUM);
        // open the SOCKET with UDP mode
        socket(TFTP_SOCKET_NUM, Sn_MR_UDP, TFTP_LOCAL_PORT, 0);
       
        //requet read fireware file from server
        tftp_send_readfile_request("app.bin");
       
        while(1){
                //wait for server's respond
                wait_timeout = 0;
                while(1) {
                        //check the size of received data
                        len = getSn_RX_RSR(TFTP_SOCKET_NUM);
                        if(len > 0){
                                break;
                        }else if(wait_timeout > 3*100000){//3S
                                Debug_Printf("tftpclient_fireware_update : timeout, no respond from server\r\n");
                                return -1;//timeout: no respond from server
                        }else{
                                delay_us(10);  wait_timeout++;
                        }
                }
               
                //receive data
                len = recvfrom(TFTP_SOCKET_NUM, tftp_frame_buffer, 1024, destip, &destport);
               
                //check ip address
                if( (destip[0]!=serverip[0]) || (destip[1]!=serverip[1]) ||\
                        (destip[2]!=serverip[2]) || (destip[3]!=serverip[3]) ) {
                        Debug_Printf("tftpclient_fireware_update : destip error, %d.%d.%d.%d, continue\r\n", \
                                destip[0],destip[1],destip[2],destip[3]);
                        continue;
                }
               
                //the port may change, due to server may reserve wellkonw UDP port for other client
                /* A requesting host chooses its source TID as described above, and sends
                its initial request to the known TID 69 decimal (105 octal) on the
                serving host.  The response to the request, under normal operation,
                uses a TID chosen by the server as its source TID and the TID chosen
                for the previous message by the requestor as its destination TID.
                The two chosen TID's are then used for the remainder of the transfer.
                */
                if((0 == changflag) && (destport != tftp_remote_port)) {
                        Debug_Printf("tftpclient_fireware_update : remote server port change from %d to %d\r\n", tftp_remote_port, destport);
                        tftp_remote_port = destport;
                        changflag = 1;
                }else if((1 == changflag) && (destport != tftp_remote_port)){
                        //error
                        Debug_Printf("tftpclient_fireware_update : Error port (data from invalid tid)\n");
                        tftp_send_error_packet(TFTP_ERR_UKNOWN_TRANSFER_ID, "Unknown TID");
                        continue;
                }
               
                //check packet len
                if (len < 2) return -1;
                len -= 2;
               
                /* get tftp cmd code */
                proto = *( (unsigned short *)tftp_frame_buffer );
                proto = ByteOrder(proto);
                switch (proto) {
                        //-----------------------------------data packet-----------------------------------
                        case TFTP_DATA:{
                                //set len equal to effective data counts
                                if (len < 2) return -1;
                                len -= 2;
                               
                                /*
                                 * RFC1350 specifies that the first data packet will
                                 * have sequence number 1. If we receive a sequence
                                 * number of 0 this means that there was a wrap
                                 * around of the (16 bit) counter.
                                 */
                                tftp_cur_block = ByteOrder( *( (unsigned short *)(tftp_frame_buffer+2) ) );
                                if ( (tftp_cur_block == 0) || (tftp_cur_block > (tftp_prev_block + 1)) ) {
                                        Debug_Printf("tftpclient_fireware_update : block error, tftp_prev_block(%d), tftp_cur_block(%d)\r\n", \
                                                tftp_prev_block, tftp_cur_block);
                                        return -1;
                                }
                               
                                Debug_Printf("block(%d)size(%d)  ", tftp_cur_block, len);
                               
                                /* previous old block or Same block again; ignore it. */
                                if (tftp_cur_block <= tftp_prev_block) {
                                        //Acknowledge the block just received, which will prompt the remote for the next one.
                                        tftp_send_ack_packet(tftp_cur_block);
                                        break;
                                }
                               
                                tftp_prev_block = tftp_cur_block;
                                /* Does this packet have any valid data to write? */
                                if ( (0<len) && (len<=512) ) {
                                        /* Write received data in Flash */
                                        FLASH_If_Write(Flash_Write_Address, (uint32_t*)(tftp_frame_buffer+4), len/4);
                                        Flash_Write_Address += len;
                                }
                               
                                /*
                                 *        Acknowledge the block just received, which will prompt
                                 *        the remote for the next one.
                                 */
                                tftp_send_ack_packet(tftp_cur_block);
                               
                                /* If the last write returned less than the maximum TFTP data pkt length,
                                 * then we've received the whole file and so we can quit (this is how TFTP
                                 * signals the EndTransferof a transfer!)
                                */
                                if (len < 512)  return 1;//tftp_complete
                               
                                break;
                        }
                        //-----------------------------------error packet-----------------------------------
                        case TFTP_ERROR: {
                                Debug_Printf("tftpclient_fireware_update : TFTP error, '%s' (%d)\n", \
                                        (tftp_frame_buffer+4), ByteOrder(*(unsigned short *)(tftp_frame_buffer+2)));
                                /*switch (ByteOrder(*(unsigned short *)pkt)) {
                                case TFTP_ERR_FILE_NOT_FOUND:
                                case TFTP_ERR_ACCESS_DENIED:
                                        puts("Not retrying...\n");
                                        eth_halt();
                                        net_set_state(NETLOOP_FAIL);
                                        break;
                                case TFTP_ERR_UNDEFINED:
                                case TFTP_ERR_DISK_FULL:
                                case TFTP_ERR_UNEXPECTED_OPCODE:
                                case TFTP_ERR_UNKNOWN_TRANSFER_ID:
                                case TFTP_ERR_FILE_ALREADY_EXISTS:
                                default:
                                        puts("Starting again\n\n");
                                        net_start_again();
                                        break;
                                }*/
                                return -1;
                                //break;
                        }
                        default: {
                                Debug_Printf("tftpclient_fireware_update : Illegal operation code(%d)\n", proto);
                                tftp_send_error_packet(TFTP_ERR_ILLEGALOP, "Illegal operation");
                                break;
                        }
                }//end switch loop
        }//end while loop
}
