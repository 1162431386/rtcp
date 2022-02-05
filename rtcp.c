#include "rtcp.h"

#include "socket.h"

#define  RECONNETED_CNT    30    /*尝试重连次数*/

#define  RTCP_PORT1        9000
#define  RTCP_PORT2        9001


struct rtcp_cli_t
{
    char ip[32];
    unsigned short int port;
    int cli_number;
};

struct rtcp_fd_t
{
   int svr_fd_num[2];
   int cli_fd_num[2];
};

struct rtcp_fd_t g_fd = {{-1},{-1}};

static void *svr_process_thread_1(void *arg)
{
    struct svr_process_t *psvr_t = (struct svr_process_t *)(arg);
    g_fd.svr_fd_num[0] = psvr_t->cli_sock_fd;
    

    return NULL;
}

static void *svr_process_thread_2(void *arg)
{
    struct svr_process_t *psvr_t = (struct svr_process_t *)(arg);
    g_fd.svr_fd_num[1] = psvr_t->cli_sock_fd;


    return NULL;
}


static int rtcp_server_thread_start(unsigned short int port_1, unsigned short int port_2)
{
    if(-1 == svr_init(port_1, svr_process_thread_1, WAIT_FOREVER))
    {
        RTCP_PRINTF("svr_1 crate faild!\n");
        return -1;
    }

    if(-1 == svr_init(port_2, svr_process_thread_2, WAIT_FOREVER))
    {
        RTCP_PRINTF("svr_2 crate faild!\n");
        return -1;
    }
    return 0;
}


static void  *rtcp_client_thread(void *arg)
{
    int sock_fd = -1;
    int count = 0;
    int another_sock_fd = -1;
    struct rtcp_cli_t *prtcp_cli_t = (struct rtcp_cli_t *)arg;
    do
    {
        sock_fd = socket_client_tcp_create_ipv4(prtcp_cli_t->ip, prtcp_cli_t->port, 3000, SOCKET_NOBLOCK);
        if(count > RECONNETED_CNT)
        {
            RTCP_PRINTF("connect faild! port = %d\n",prtcp_cli_t->port);
            break;
        }
        count ++;
    } while (-1 == sock_fd);
    if(-1 != sock_fd)
    {
        RTCP_PRINTF("connect success! port = %d\n",prtcp_cli_t->port);
    }
    if(1 == prtcp_cli_t->cli_number)
    {
        g_fd.cli_fd_num[0] = sock_fd;                     /*将本端的描述符保存*/
        another_sock_fd = g_fd.svr_fd_num[0];             /*获取对端描述符*/
                                                          /*做数据交换*/
    }
    else if(2 == prtcp_cli_t->cli_number)
    {
        g_fd.cli_fd_num[1] = sock_fd;                     /*将本端的描述符保存*/
        another_sock_fd = g_fd.svr_fd_num[1];             /*获取对端描述符*/
                                                          /*做数据交换*/
    }
    else
    {
        RTCP_PRINTF("ERROR prtcp_cli_t->cli_number = %d\n",prtcp_cli_t->cli_number);
        return NULL;
    }
    return 0;
}

int rtcp_cli_thread_start(struct rtcp_cli_t *prtcp_cli_t)
{
	int ret = 0;
    pthread_attr_t attr;
	pthread_t tid = (pthread_t)-1;

    setPthreadAttr(&attr, 50, 64 * 1024, 1);
	ret = pthread_create(&tid, NULL, rtcp_client_thread, prtcp_cli_t);
    pthread_attr_destroy(&attr);
	if (0 != ret) {
		RTCP_PRINTF("FAIL to create rtcp_cli_thread, port = %u, %s\n", (unsigned int)prtcp_cli_t->port, strerror(ret));
		return -1;
	}
	return 0;   
}


int rtcp_mian()
{
    static struct rtcp_cli_t rtcp_cli_t1;
    static struct rtcp_cli_t rtcp_cli_t2;
    memset(&rtcp_cli_t1,0,sizeof(rtcp_cli_t1));
    memset(&rtcp_cli_t2,0,sizeof(rtcp_cli_t2));
    rtcp_cli_t1.cli_number = 1;
    memcpy(rtcp_cli_t1.ip,"127.0.0.1",sizeof(rtcp_cli_t1.ip));
    rtcp_cli_t1.port = 22;

    rtcp_cli_t2.cli_number = 2;
    memcpy(rtcp_cli_t2.ip,"127.0.0.1",sizeof(rtcp_cli_t2.ip));
    rtcp_cli_t2.port = 9000;
    /*启动服务器监听相应端口*/
    (void)rtcp_server_thread_start(RTCP_PORT1,RTCP_PORT2);
    rtcp_cli_thread_start(&rtcp_cli_t1);
    rtcp_cli_thread_start(&rtcp_cli_t2);
    return 0;
}


int main()
{
  rtcp_mian();
  while(1)
  {
      pause();
  }
  return 0;
}
