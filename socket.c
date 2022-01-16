#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "socket.h"


/*
* @Function: socket_create
* @Description:crate socket
* @Input:used udp?
* @Output:1:succe -1 fail
* @Return: 1:succe -1 fail
*/
int socket_create(int isUdp)
{
	int iSockFd = -1;

  iSockFd = socket(AF_INET, isUdp ? SOCK_DGRAM : SOCK_STREAM, 0);

	if(iSockFd < 0)
    {
    	RTCP_PRINTF("socket_create failed: errno %d\n", errno);
    }
	return iSockFd;
}


/*
* @Function: socket_close
* @Description:close socket
* @Input:fd
* @Output:NULL
* @Return: NULL
*/
void socket_close(int iSockFd)
{
    close(iSockFd);
}


/*
* @Function: socket_connect_wait
* @Description:close socket
* @Input:fd
* @Output:NULL
* @Return: NULL
*/
int socket_connect_wait(int sockfd, struct sockaddr_in *address, int msecond)
{
    int err = 0; 
    int len = sizeof(int);
    int block_or_not = 0; // 将socket设置成阻塞或非阻塞
    int ret_val = -1;     // 接收函数返回
    fd_set set;
    struct timeval mytm;

    if ((NULL == address) || (sockfd < 0))
    {
        RTCP_PRINTF("connect_with_timeout para error\n");
        return -1;
    }

    memset(&mytm, 0, sizeof(struct timeval));

    if (-1 == msecond)  /*非阻塞connet*/
    {
        ret_val = connect(sockfd, (struct sockaddr *)address, sizeof(struct sockaddr_in));
        return ret_val;
    }

    block_or_not = 1; // 设置非阻塞
    if (0 != ioctl(sockfd, FIONBIO, &block_or_not))
    {
        RTCP_PRINTF("ioctl socket failed\n");
    }
    
  
    ret_val = connect(sockfd, (struct sockaddr *)address, (socklen_t)sizeof(struct sockaddr_in));

    if (-1 == ret_val)
    {
        if (EINPROGRESS == errno)
        {
            FD_ZERO(&set);
            FD_SET(sockfd, &set);
            mytm.tv_sec = msecond / 1000;
            mytm.tv_usec = (msecond % 1000) * 1000;

            if (select(sockfd + 1, NULL, &set, NULL, &mytm) > 0)
            {
                // 清除错误
                (void)getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, (socklen_t *)&len);
                if (0 == err)
                {
                    ret_val = 0;
                }
                else
                {
                    ret_val = -1;
                }
            }
            else
            {
                ret_val = -1;
            }
        }
    }

    block_or_not = 0; // 设置阻塞
    if (0 != ioctl(sockfd, FIONBIO, &block_or_not))
    {
        RTCP_PRINTF("ioctl socket failed\n");
    }

    return ret_val;
}

/*
* @Function: socket_connect_wait
* @Description:close socket
* @Input:fd
* @Output:NULL
* @Return: NULL
*/
int socket_client_tcp_create_ipv4(const char *sHostName, int uPort, int uWaitMsec)
{
    int iSock = -1;
    struct sockaddr_in address;

    iSock = socket_create(0); /*建立TCP连接*/
    if (iSock < 0)
    {
        RTCP_PRINTF("sys_socket_create：  failed!\n");
        return -1;
    }
    memset(&address,0,sizeof(address));

    address.sin_family = AF_INET;  /*ipv4协议簇*/
    address.sin_addr.s_addr = inet_addr(sHostName);
    address.sin_port = htons(uPort);

    if (0 != socket_connect_wait(iSock, &address, uWaitMsec))
    {
        socket_close(iSock);
        RTCP_PRINTF("connect to host %s:%d in %dms failed!\n", sHostName, uPort, uWaitMsec);
        return -1;
    }

    return iSock;
}




int main()
{
  char *ww = "🐏青青";
  RTCP_PRINTF("wdwdw = %s\n",ww);
  socket_client_tcp_create_ipv4("127.0.0.1", 9000, 3000);
  return 0;
}