#ifndef __KCP_SERVER_H__
#define __KCP_SERVER_H__
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
//#define SOCKET_RECV_NOBLOCK

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
	
extern int libkcp_server_init(int port, char *ip, int WndSize, IUINT32 UpdateTime);
extern int libkcp_server_send(unsigned char *pClientDataBuf);
extern int libkcp_server_recv(struct sockaddr_in *stTransAddr, unsigned char *pClientDataBuf);
extern void libkcp_server_free();

#endif

