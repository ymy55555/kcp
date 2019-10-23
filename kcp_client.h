#ifndef _KCP_CLIENT_H_
#define _KCP_CLIENT_H_
#include <sys/types.h>       
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>
#include <uuid/uuid.h>
#include <sys/ioctl.h>
#include "ikcp.h"

#define CLIENT_BIND_SERVER
#define MAX_CLIENT_BUF_SIZE 1460
#define MAX_SERVER_BUF_SIZE (MAX_CLIENT_BUF_SIZE + 24)
//#define SOCKET_RECV_NOBLOCK  //接受=收数据非阻塞

#define PRINTF(fmt...)   \
    do {\
        printf("[%s]:%d:  ", __FUNCTION__, __LINE__);\
        printf(fmt);\
    }while(0)
	
typedef enum{
	DEFAULT_MODE = 0,
	NORMAL_MODE,
	QUICK_MODE,
}TRANS_MODE;

//WndSize 128 UpdateTime 20ms
extern int libkcp_client_init(int port, char *ip, int WndSize, IUINT32 UpdateTime);
extern void libkcp_client_free();
extern int libkcp_client_send(unsigned char *pClientDataBuf);
extern int libkcp_client_recv(struct sockaddr_in *stTransAddr, unsigned char *pClientDataBuf);

#endif



