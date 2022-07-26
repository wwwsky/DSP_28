#include "tftp.h"
#include "env.h"
 
 
 
#ifdef      OPTION_ENABLE
static TFTP_OPTION default_tftp_opt;//=
{
  .code = (uint8_t *)"timeout",
  .value = (uint8_t *)"3"
};
#endif
 
// return 0:IP匹配完成 ,1:IP 匹配失败
int ip_addr_comp(uint8_t *src ,uint8_t *comp)
{
uint8_t i=0;
    for(i=0;i<4;i++)
    {
        if(src[i]!=comp[i]) return 1;
    }
return 0;
}
 
int open_tftp_socket(uint8_t sock)
{
    uint8_t sd, sck_state;
    uint8_t retry =0;
    he: sd = socket(sock, Sn_MR_UDP, 5100,0);
    if(sd != 1)
    {
        return -1;
    }
    sck_state=getSn_SR(sock);
    if(sck_state != SOCK_UDP)
    {
        if(++retry>10)
        {
            return -1;
        }
        goto he;
    }
    return sock;
}
 
 
uint16_t recv_udp_packet(int socket, uint8_t *packet, uint32_t len, uint8_t *ip, uint16_t *port)
{
//  int ret;
    uint8_t sck_state;
    uint16_t recv_len;
    /* Receive Packet Process */
    sck_state=getSn_SR(socket);
    if(sck_state == SOCK_UDP)
    {
        if(getSn_IR(socket) & Sn_IR_RECV)
        {
            setSn_IR(socket, Sn_IR_RECV);// Sn_IR的RECV位置1
        }
        recv_len=getSn_RX_RSR(socket);
        if(recv_len)
        {
            recv_len = recvfrom(socket, packet, recv_len, (uint8_t *)ip, port);
            return recv_len;
        }
    }
    return 0;
}
 
 
 
static const char *error_string[8] =
{"Not defined error!", // 0
 
"File not found!", // 1
 
"Access denied!", // 2
 
"Disk full or allocation exceeded!", // 3
 
"Illegal TFTP operation!", // 4
 
"Unknown transfer ID!", // 5
 
"File already exists!", // 6
 
"No such user!" // 7
 
};
static uint8_t tmp_buffer[MAX_MTU_SIZE];
// tftp 获取文件
// return 0:fail ，1：ok
int tftp_get(struct tty *ptty ,char *filename,uint32_t *getbuf,uint32_t *getlen,uint32_t getmax)
{
    char *typestr;
    int Socket;
    uint8_t server_ip[4];
    uint8_t *pkt = tmp_buffer;
    uint32_t  len;
#ifdef      OPTION_ENABLE
    TFTP_OPTION *opt =&default_tftp_opt;//选项字节
#endif
    uint32_t    pos=0; //全局存储长度
    uint16_t flgst=0;
    uint16_t  from_port;
    uint8_t from_ip[4]; //
    uint16_t cur_tick,last_tick,det_tick;
    uint16_t opcode,current_pack,last_pack;
 
    if((Socket=open_tftp_socket(SOCK_TFTP))<0)
    {
        ptty->printf("[%s] socket error\r\n", __func__);
        goto err;
    }
    typestr = env_get("serverip");
    inet_addr_((uint8_t*)typestr,server_ip);
 
    pkt[0] =0;
    pkt[1] = TFTP_RRQ; //2019-9-20  add
    pkt += 2;
    strcpy((char *)pkt, (const char *)filename);
    pkt += strlen((char *)filename) + 1;
    strcpy((char *)pkt, (const char *)TRANS_BINARY);
    pkt += strlen((char *)TRANS_BINARY) + 1;
#ifdef      OPTION_ENABLE
    for(i = 0 ; i < 1 ; i++) {
        strcpy((char *)pkt, (const char *)opt[i].code);
        pkt += strlen((char *)opt[i].code) + 1;
        strcpy((char *)pkt, (const char *)opt[i].value);
        pkt += strlen((char *)opt[i].value) + 1;
    }
#endif
    len = pkt - tmp_buffer;
    sendto(Socket, tmp_buffer, len, server_ip, TFTP_SERVER_PORT);
 
    last_pack = 0;
    offset_len =0;
    flgst =0;
    while(1)
    {
        /* Receive Packet Process */
        len = recv_udp_packet(Socket, tmp_buffer, MAX_MTU_SIZE, from_ip, &from_port);
        if(len < 1)
        {
            if(flgst==0)
            {
                flgst=1;
                last_tick =mde_stc_GetTick();
            }
            cur_tick = mde_stc_GetTick();
            det_tick=cur_tick >= last_tick ? (cur_tick - last_tick): (Sysms_time_MAX + cur_tick - last_tick);
            if(det_tick>3000)
            {
               ptty->printf("recv_udp_packet timeout\r\n");
               goto err;
            }
        }
        else//Normal
        {
            last_tick =mde_stc_GetTick(); //time tick
            if(ip_addr_comp(server_ip,from_ip)) /* Verify Server IP */
            {
                ptty->printf("Server IP faults\r\n");
                goto err;
            }
            opcode =(uint16_t )tmp_buffer[0]<<8;
            opcode|=tmp_buffer[1];
            switch(opcode)
            {
                case TFTP_DATA :
                    current_pack =(uint16_t )tmp_buffer[2]<<8;
                    current_pack|=tmp_buffer[3];
                    if ((last_pack + 1) != current_pack)
                    {
                        ptty->printf("\ntftp: packet err!\n");
                        goto err;
                    }
                    last_pack = current_pack;
                    if (pos + len - 4 > getmax)
                    {
                        ptty->printf("\ntftp: mem is over\n");
                        goto err;
                    }
                    //TODO：  由于我的单板有SDRAM，这里开始将数据写入外部SDRAM中 。。。
                    //
                    //if(0==tftp_data_save(&tmp_buffer[4],len,getbuf,&pos))
                    //{
                    //  ptty->printf("offset_len>=4???\n");
                    // }
 
                    /* make ACK */
                    tmp_buffer[0] = 0;
                    tmp_buffer[1] = TFTP_ACK; /* opcode */
                    /* send ACK */
                     sendto(Socket, tmp_buffer, 4, from_ip, from_port);
//
                    if (pos % 50 == 0)  ptty->printf("*");
                    if (len != 516) goto ok;
                    break;
                case TFTP_ERROR:
                    current_pack =(uint16_t )tmp_buffer[2]<<8;
                    current_pack|=tmp_buffer[3];
                    if(current_pack>7) current_pack=0;
                    ptty->printf("err: %s\n", error_string[current_pack]);
                    goto err;
                default:
                    ptty->printf("unknow error! \n");
                    goto err;
            }
        }
    }
    ok: ptty->printf("\ndone \n");
    close(Socket);
    getlen[0] = pos;
    return 1;
    err:
    close(Socket);
    return 0;
}
 
// 发送文件
//return 0:fail ,1:ok
int tftp_send(struct tty *ptty , uint32_t *mem , char *filename , unsigned long setlen)
{
    char *typestr;
    int Socket;
    uint8_t server_ip[4];
    uint8_t *pkt = tmp_buffer;
    uint32_t i, len,remainLenth;
#ifdef      OPTION_ENABLE
    TFTP_OPTION *opt =&default_tftp_opt;//选项字节
#endif
    uint16_t flgst=0;
    uint32_t pos=0;
    uint16_t  from_port;
    uint8_t from_ip[4]; //
    uint16_t cnt = 0;
    uint16_t cur_tick,last_tick,det_tick;
    uint16_t opcode,current_pack,last_pack;
    uint16_t half_num =0;
    uint32_t data_temp =0;
    if((Socket=open_tftp_socket(SOCK_TFTP))<0)
    {
        ptty->printf("[%s] socket error\r\n", __func__);
        goto err;
    }
    typestr = env_get("serverip");
    inet_addr_((uint8_t*)typestr,server_ip);
 
    /*协议开始*/
    /*操作码*/
    pkt[0] =0;
    pkt[1] = TFTP_WRQ;
    pkt += 2;
    strcpy((char *)pkt, (const char *)filename);
    pkt += strlen((char *)filename) + 1;
    strcpy((char *)pkt, (const char *)TRANS_BINARY);
    pkt += strlen((char *)TRANS_BINARY) + 1;
#ifdef      OPTION_ENABLE
    for(i = 0 ; i < 1 ; i++) {
        strcpy((char *)pkt, (const char *)opt[i].code);
        pkt += strlen((char *)opt[i].code) + 1;
        strcpy((char *)pkt, (const char *)opt[i].value);
        pkt += strlen((char *)opt[i].value) + 1;
    }
#endif
    len = pkt - tmp_buffer;
    sendto(Socket, tmp_buffer, len, server_ip, TFTP_SERVER_PORT);
 
    last_pack = 0;
    offset_len =0;
    remainLenth = setlen;
    pos=0;
    flgst =0;
    while(1)
    {
       /* Receive Packet Process */
        cnt++;
       len = recv_udp_packet(Socket, tmp_buffer, MAX_MTU_SIZE, from_ip, &from_port);
       if(len < 1)
       {
           if(flgst==0)
           {
               flgst=1;
               last_tick =mde_stc_GetTick();
           }
           cur_tick = mde_stc_GetTick();
           det_tick=cur_tick >= last_tick ? (cur_tick - last_tick): (Sysms_time_MAX + cur_tick - last_tick);
           if(det_tick>3000)
           {
              ptty->printf("recv_udp_packet timeout\r\n");
              goto err;
           }
       }
       else//Normal
       {
           last_tick =mde_stc_GetTick(); //time tick
           if(ip_addr_comp(server_ip,from_ip)) /* Verify Server IP */
           {
               ptty->printf("Server IP faults\r\n");
               goto err;
           }
           opcode =(uint16_t )tmp_buffer[0]<<8;
           opcode|=tmp_buffer[1];
           switch(opcode)
           {
               case TFTP_ACK :
                   current_pack =(uint16_t )tmp_buffer[2]<<8;
                   current_pack|=tmp_buffer[3];
                   if (last_pack != current_pack)
                   {
                       ptty->printf("\ntftp: update packet err! current_ack_pack=%d   last_send_pack=%d remainLenth=%d \n",
                                    last_pack, last_pack, remainLenth);
                       goto err;
                   }
                   /* make ACK */
                   tmp_buffer[0] = 0;
                   tmp_buffer[1] = TFTP_DATA; /* opcode */
                   last_pack++;
                   tmp_buffer[2] =(uint16_t)last_pack>>8;
                   tmp_buffer[3] =last_pack&0xff;
                   len =0;
                   if (remainLenth == 0)
                   {
                       goto ok;
                   }
                   if (remainLenth > 512)
                   {
                       for(i=0;i<128;i++)//数据填充512/4=128
                       {
                           data_temp = mem[pos];
 
                           tmp_buffer[4+(i*4)] = ((uint32_t)data_temp>>24)&0xff;
                           tmp_buffer[5+(i*4)] = ((uint32_t)data_temp>>16)&0xff;
                           tmp_buffer[6+(i*4)] = ((uint32_t)data_temp>>8)&0xff;
                           tmp_buffer[7+(i*4)] = (uint32_t)data_temp&0xff;
                           pos+=2;
                       }
                       len = 512 + 4;
                   }
                   else
                   {
                       half_num =remainLenth/2;
                       for(i=0;i<half_num;i++)//数据填充
                       {
                           data_temp = mem[pos];
                           tmp_buffer[4+(i*4)] = ((uint32_t)data_temp>>24)&0xff;
                           tmp_buffer[5+(i*4)] = ((uint32_t)data_temp>>16)&0xff;
                           tmp_buffer[6+(i*4)] = ((uint32_t)data_temp>>8)&0xff;
                           tmp_buffer[7+(i*4)] = (uint32_t)data_temp&0xff;
                           pos+=2;
                       }
                       if((half_num*2)<remainLenth)
                       {
                           data_temp = mem[pos];
                           tmp_buffer[8+(i*4)] = ((uint32_t)data_temp>>24)&0xff;
                           tmp_buffer[9+(i*4)] = ((uint32_t)data_temp>>16)&0xff;
                           tmp_buffer[10+(i*4)] = ((uint32_t)data_temp>>8)&0xff;
                       }
                       len = remainLenth + 4;
                   }
                   sendto(Socket, tmp_buffer, len, from_ip, from_port);
                   if (cnt % 50 == 0)
                   {
                       ptty->printf("*");
                   }
                   remainLenth -= (remainLenth > 512 ? 512 : remainLenth);
                   if (remainLenth == 0)
                   {
                       goto ok;
                   }
                   break;
               case TFTP_ERROR:
                   current_pack =(uint16_t )tmp_buffer[2]<<8;
                   current_pack|=tmp_buffer[3];
                   if(current_pack>7) current_pack=0;
                   ptty->printf("err: %s\n", error_string[current_pack]);
                   goto err;
               default:
                   ptty->printf("unknow error! \n");
                   goto err;
           }
       }
    }
    ok: ptty->printf("\ndone \n");
    close(Socket);
    return 1;
    err:
    close(Socket);
    return 0;
}
 
