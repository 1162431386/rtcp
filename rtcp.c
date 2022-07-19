#include "rtcp.h"
#include "socket.h"
#include "kbhit.h"

#define  RECONNETED_CNT    30    /*尝试重连次数*/

#define  RTCP_PORT1        6001
#define  RTCP_PORT2        6002
#define  RTCP_SWAP_BUFF_MAX_SIZE    (1024 * 2)

struct rtcp_cli_t
{
    char ip[32];
    unsigned short int port;
    int cli_number;
};

struct rtcp_fd_t
{
    int sock_fd[2];
};

struct rtcp_fd_t g_fd = {{-1,-1}};


int get_another_stream(int num)

{
    /*获取另一端fd，如果当前为空，则等待*/
    while(1)
        {
            if(-1 != g_fd.sock_fd[num])
            {
                return g_fd.sock_fd[num];
            }
            sleep(1);
        }
    return 0;
}

/*这里缓冲区必须是全局的，不然会释放掉之前发过来的数据*/
static int rtcp_swap_stream(int src_fd,int dist_fd)
{
    char *pBuff = NULL;
    int iRecvLen = 0;
    int iSendLen = 0;
    pBuff = (char *)malloc(RTCP_SWAP_BUFF_MAX_SIZE);
    while(1)
    {
        if(-1 == src_fd || -1 == dist_fd )
        {
            usleep(100);  
            continue;
        }

        iRecvLen = sys_socket_read_wait(src_fd,pBuff,RTCP_SWAP_BUFF_MAX_SIZE,WAIT_FOREVER);
        RTCP_PRINTF("iRecvLen = %d ,pBuff = %s\n",iRecvLen,pBuff);
        iSendLen = sys_socket_writen(dist_fd,pBuff,iRecvLen);
#if 0       
        if(iRecvLen > 0)
        {
           

            if(iSendLen != iRecvLen)
            {
                RTCP_PRINTF("send ERROR!\n");
                goto EXIT;
            }

        }

        else
        {
          RTCP_PRINTF("recv error!%d\n",iRecvLen);
          goto EXIT;
        }
 #endif    
    }
    return 0;

EXIT:
    RTCP_PRINTF("swap error!,close fd!\n");
    SAFE_CLOSE(src_fd);
    SAFE_CLOSE(dist_fd);
    SAFE_FREE(pBuff);
    return -1;
}



static void *svr_process_thread_1(void *arg)
{
#if 1
    int another_sock_fd = -1;
    struct svr_process_t *psvr_t = (struct svr_process_t *)(arg);
    g_fd.sock_fd[0] = psvr_t->cli_sock_fd;                  /*将本端的描述符保存*/
    another_sock_fd = get_another_stream(1);                     /*获取对端fd*/
    RTCP_PRINTF("psvr_t->cli_sock_fd = %d ,another_sock_fd = %d\n",psvr_t->cli_sock_fd,another_sock_fd);
    rtcp_swap_stream(psvr_t->cli_sock_fd,another_sock_fd);     /*获取对端fd*/
#endif 
    return NULL;
}

static void *svr_process_thread_2(void *arg)
{ 
 #if 1   
    int another_sock_fd = -1;
    struct svr_process_t *psvr_t = (struct svr_process_t *)(arg);
    g_fd.sock_fd[1] = psvr_t->cli_sock_fd;                  /*将本端的描述符保存*/
    another_sock_fd = get_another_stream(0);                     /*获取对端fd*/
    RTCP_PRINTF("psvr_t->cli_sock_fd = %d ,another_sock_fd = %d\n",psvr_t->cli_sock_fd,another_sock_fd);
    rtcp_swap_stream(psvr_t->cli_sock_fd,another_sock_fd);     /*获取对端fd*/
#endif 
    return NULL;
}


static int rtcp_server_thread_start(unsigned short int port_1, unsigned short int port_2)
{
    if(-1 == svr_init(port_1, svr_process_thread_1, WAIT_FOREVER))
    {
        RTCP_PRINTF("svr_1 crate faild!\n");
        return -1;
    }
#if 1
    if(-1 == svr_init(port_2, svr_process_thread_2, WAIT_FOREVER))
    {
        RTCP_PRINTF("svr_2 crate faild!\n");
        return -1;
    }
#endif
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
        RTCP_PRINTF("connected to  %s:%d\n",prtcp_cli_t->ip,prtcp_cli_t->port);
    }
   
    if(1 == prtcp_cli_t->cli_number)
    {
        
        g_fd.sock_fd[0] = sock_fd;                     /*将本端的描述符保存*/
        another_sock_fd = g_fd.sock_fd[1];             /*获取对端描述符*/
        RTCP_PRINTF("sock_fd = %d ,another_sock_fd = %d\n",sock_fd,another_sock_fd);
         while(1)
        {
            if(-1 != g_fd.sock_fd[1])
            {
                rtcp_swap_stream(sock_fd,g_fd.sock_fd[1]);        /*做数据交换*/
            }
            usleep(100);
        } 
        
    }
    else if(2 == prtcp_cli_t->cli_number)
    {
        g_fd.sock_fd[1] = sock_fd;                     /*将本端的描述符保存*/
        another_sock_fd = g_fd.sock_fd[0];             /*获取对端描述符*/
        RTCP_PRINTF("sock_fd = %d ,another_sock_fd = %d\n",sock_fd,another_sock_fd);
        rtcp_swap_stream(sock_fd,another_sock_fd);        /*做数据交换*/                                                 /*做数据交换*/
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
    rtcp_cli_t2.port = 6001;
    /*启动服务器监听相应端口*/
    rtcp_cli_thread_start(&rtcp_cli_t1);
    rtcp_cli_thread_start(&rtcp_cli_t2);
    return 0;
}




int main(int argc, void *argv[])
{
    int ch = 0;
    init_keyboard();
    if (argc < 2) {
        printf("FORMAT is [app svr] or  [app cli]\n");
        return -1;
    }

    if (0 == strcmp(argv[1], "svr")) {
        (void)rtcp_server_thread_start(RTCP_PORT1,RTCP_PORT2);
    }
    else if (0 == strcmp(argv[1], "cli")) {
        rtcp_mian();	
    }
    else {
        printf("INVALID arg, valid range {\"svr\", \"cli\"}\n");
        return -1;
    }
    while(ch != 'q')
    {
         if(kbhit()) 
        {
            ch = readch();
        }
        usleep(200);
    }
    close_keyboard();
    close(g_fd.sock_fd[0]);
    close(g_fd.sock_fd[1]);
    exit(0);
    return 0;
    
}