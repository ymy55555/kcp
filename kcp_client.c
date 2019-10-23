#include "kcp_client.h"

int g_sClientFd = -1;
struct sockaddr_in g_stTransAddr;
ikcpcb *g_client_kcp;

/* get system time */
static inline void itimeofday(long *sec, long *usec)
{
	struct timeval time;
	gettimeofday(&time, NULL);
	if (sec) *sec = time.tv_sec;
	if (usec) *usec = time.tv_usec;	
}

/* get clock in millisecond 64 */
static inline IINT64 iclock64(void)
{
	long s, u;
	IINT64 value;
	itimeofday(&s, &u);
	value = ((IINT64)s) * 1000 + (u / 1000);
	return value;
}

static inline IUINT32 iclock()
{
	return (IUINT32)(iclock64() & 0xfffffffful);
}



static int init_recv_handle(struct sockaddr_in *stTransAddr, unsigned char *pRecvBuf)
{
     int sRet = -1;
	 char  RecvBuf[MAX_SERVER_BUF_SIZE];
	 socklen_t addr_len = sizeof(struct sockaddr_in);
	 sRet = recvfrom(g_sClientFd, RecvBuf,MAX_SERVER_BUF_SIZE, 0,
	 	                       (struct sockaddr *)stTransAddr, &addr_len);
	 if(sRet < 0)
	 {
		PRINTF("recvfrom data failed.   errno %d\n", errno);
		return -1;
	 }
	 //kcp接收到下层协议UDP传进来的数据底层数据buffer转换成kcp的数据包格式
	 sRet = ikcp_input(g_client_kcp, RecvBuf, MAX_SERVER_BUF_SIZE);
	 //ikcp_update(kcp_arg.kcp, kcp_arg.iclock());	
     if (sRet < 0) 
	 {
        PRINTF("ikcp_input error, ret :%d\n", sRet);
		return -1;
     }
     sRet = ikcp_recv(g_client_kcp, RecvBuf, MAX_CLIENT_BUF_SIZE);
     if(sRet < 0)
	 {
        return -1;
     }
	 pRecvBuf = (unsigned char *)RecvBuf;
	 return 0;
}




//UDP发送的地址来自接收的客户端的地址
static int init_send_handle(void *sSendBuf, 
                       int sSendBufSize, struct sockaddr_in *stTransAddr)
{
	int sRet = -1;
	ikcp_update(g_client_kcp, iclock());    
	sRet = sendto(g_sClientFd, sSendBuf, sSendBufSize, 0,
		               (struct sockaddr *)stTransAddr, sizeof(struct sockaddr_in));
	ikcp_update(g_client_kcp, iclock());    
	if(0 == sRet)
	{
		PRINTF("send failed,Receiver is exit.\n");
		return -1;
	}else if(-1 == sRet)
	{
		PRINTF("send data failed.\n");
		return -1;
	}
	return 0;
}

int kcp_output(const char *buf, int len, ikcpcb *kcp, void *user)
{
   if(0 != init_send_handle((void *)buf, MAX_CLIENT_BUF_SIZE, &g_stTransAddr))
   {
	  PRINTF("Send failed.\n");    
   }
   return 0;
}

static int init_kcp(IUINT32 sConvNo, void *pUserData, int sWndSize, int sTransMode, IUINT32 sUpdateTime)
{
	// 创建两个端点的 kcp对象，第一个参数 conv是会话编号，同一个会话需要相同
	// 最后一个是 user参数，用来传递标识
	g_client_kcp = ikcp_create(sConvNo, pUserData);//多个客户端时kcp的情况待分析。。。

   //待考虑线程巡航检测更新。。。
    //IUINT32 current = iclock();
	//IUINT32 slap = current + sUpdateTime;
	// 设置kcp的下层输出，这里为 udp_output，模拟udp网络输出函数
	g_client_kcp->output = kcp_output;
	/*
	 配置窗口大小：平均延迟200ms，每20ms发送一个包，而考虑到丢包重发,
	 设置最大收发窗口为128KCP默认为32，即可以接收最大为32*MTU=43.75kB。
	 KCP采用update的方式，更新间隔为10ms，那么KCP限定了你最大传输速率为4375kB/s，
	 在高网速传输大内容的情况下需要调用ikcp_wndsize调整接收与发送窗口。
	 可以检测网络传输平均值，定期更新窗口大小
	*/
	ikcp_wndsize(g_client_kcp, sWndSize, sWndSize);
	switch(sTransMode)
	{
	   case 0:
		   // 默认模式
		   ikcp_nodelay(g_client_kcp, 0, 10, 0, 0);
	   break;

	   case 1:
		   // 普通模式，关闭流控等
		   ikcp_nodelay(g_client_kcp, 0, 10, 0, 1);
	   break;

	   case 2:
		   // 启动快速模式
		   // 第二个参数 nodelay-启用以后若干常规加速将启动
		   // 第三个参数 interval为内部处理时钟，默认设置为 10ms
		   // 第四个参数 resend为快速重传指标，设置为2
		   // 第五个参数 为是否禁用常规流控，这里禁止
		   ikcp_nodelay(g_client_kcp, 1, 10, 2, 1);
		   g_client_kcp->rx_minrto = 10;
		   g_client_kcp->fastresend = 1;
	   break;
	}
	return 0;

}

//WndSize 128 UpdateTime 20ms
int libkcp_client_init(int port, char *ip, int WndSize, IUINT32 UpdateTime)
{
	g_stTransAddr.sin_family = AF_INET;
    g_stTransAddr.sin_port = htons(port);
    g_stTransAddr.sin_addr.s_addr = inet_addr(ip);
    g_sClientFd = socket(AF_INET, SOCK_DGRAM, 0);
	
    if (-1 == g_sClientFd)
	{
	    PRINTF("Socket failed.\n");
		return -1;
	}
#if defined(SOCKET_RECV_NOBLOCK)//连接非阻塞
	int sNoBlock = 1;
    if(ioctl(g_sClientFd, FIONBIO, &sNoBlock) < 0)
	{		
	   PRINTF("ioctl FIONBIO failed.\n"); 
       return -1;   
	}
#endif
	init_kcp(AF_INET, NULL, WndSize, QUICK_MODE, UpdateTime);
    return 0;
   
}

void libkcp_client_free()
{
    close(g_sClientFd);
	ikcp_release(g_client_kcp);
}

int libkcp_client_send(unsigned char *pClientDataBuf)
{
	 int sRet = -1;
	 sRet = ikcp_send(g_client_kcp, (char *)pClientDataBuf, MAX_CLIENT_BUF_SIZE);
	 if(sRet < 0)
	 {
		   PRINTF("client send failed.\n");
		   return -1;
	 }
	 ikcp_update(g_client_kcp, iclock());
	 return 0;
}

int libkcp_client_recv(struct sockaddr_in *stTransAddr, unsigned char *pClientDataBuf)
{
	return init_recv_handle(stTransAddr, pClientDataBuf);	 
}

/*
int main()
{
	unsigned char buf[]={0x12, 0x13, 0x14};
    libkcp_client_init(6000, "192.168.5.84", 128, 20);
	while(1)
	{
      libkcp_client_send(buf);
#if 1//test
			char GetState;
			printf("continue please press ENTER.\n");
			GetState = getchar();
			if(GetState != '\n')break;
#endif
	}
	libkcp_client_free();
    return 0;
}
*/




