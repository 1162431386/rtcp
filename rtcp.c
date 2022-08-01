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
    pBuff = (char *)malloc(RTCP_SWAP_BUFF_MAX_SIZE);   /*这里需要优化内存申请释放的问题*/

    while(1)
    {
        /*这里直接转发，收到数据后，原封不动转发出去*/
        iRecvLen = sys_socket_read_wait(src_fd,pBuff,RTCP_SWAP_BUFF_MAX_SIZE,WAIT_FOREVER);
        iSendLen = sys_socket_writen(dist_fd,pBuff,iRecvLen);   
    }
    return 0;
}



static void *svr_process_thread_1(void *arg)
{
#if 1
    int another_sock_fd = -1;
    struct svr_process_t *psvr_t = (struct svr_process_t *)(arg);
    g_fd.sock_fd[0] = psvr_t->cli_sock_fd;                  /*将本端的描述符保存*/
    another_sock_fd = get_another_stream(1);                     /*获取对端fd*/
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
#ifdef DEBUG
        RTCP_PRINTF("sock_fd = %d ,another_sock_fd = %d\n",sock_fd,another_sock_fd);
#endif
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
#ifdef DEBUG
        RTCP_PRINTF("sock_fd = %d ,another_sock_fd = %d\n",sock_fd,another_sock_fd);
#endif
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



int rtcp_cli_main_start(char *ip,short svrport,short sshport)
{
    static struct rtcp_cli_t rtcp_cli_t1;
    static struct rtcp_cli_t rtcp_cli_t2;
    memset(&rtcp_cli_t1,0,sizeof(rtcp_cli_t1));
    memset(&rtcp_cli_t2,0,sizeof(rtcp_cli_t2));
    rtcp_cli_t1.cli_number = 1;
    memcpy(rtcp_cli_t1.ip,"127.0.0.1",sizeof(rtcp_cli_t1.ip));
    rtcp_cli_t1.port = sshport;

    rtcp_cli_t2.cli_number = 2;
    memcpy(rtcp_cli_t2.ip,ip,sizeof(rtcp_cli_t2.ip));
    rtcp_cli_t2.port = svrport;
    /*启动服务器监听相应端口*/

    rtcp_cli_thread_start(&rtcp_cli_t1);
    rtcp_cli_thread_start(&rtcp_cli_t2);
    return 0;
}



void rtcp_help(void)
{
    printf("-l：Server listening port，-l A:B，the server is listening on\n");
    printf("-c：The server ip port and client ssh port to which the client connects,\
    -c A:B:C，A is the server ip to connect to, B is the server port to connect to, and c is the local ssh port(22 by default)\n");
    printf("Example: Server: ./rctp -l 6001:6002\n"); 
    printf("clie: ./rtcp -c 192.168.1.1:6001\n");  
    printf("Press 'q' to exit\n");
    return;
}






int main(int argc, char *argv[])
{
    int Opt = 0;
    optind = 1;
    int ch = 0;
    init_keyboard();
    if(argc < 2)
    {
        printf("error !!! ./rtcp -v View version information  -h help\n");
        close_keyboard();
        return 0;
    }
    while (-1 != (Opt = getopt(argc, argv, "l:c:vhd"))) 
    {
        switch (Opt)
        {
            case 'l':
            {
                unsigned short  lPort1 = 0;
                unsigned short  lPort2 = 0;
                char *strsepstr = NULL;
                char *strPort1 = NULL;
                char *strPort2 = NULL;
                strsepstr = optarg;
                strPort1 = strsep(&strsepstr, ":");
                strPort2 = strsep(&strsepstr, ":");
                if(NULL == strPort1 || NULL == strPort2)
                {
                   printf("./rtcp -v View version information  -h help\n");
                   close_keyboard();
                   return 0; 
                }
                lPort1 = atoi(strPort1);
                lPort2 = atoi(strPort2);
                (void)rtcp_server_thread_start(lPort1,lPort2);
                break;
            }
            case 'c':
            {
                char *serverIp = NULL;
                unsigned short serverPort = 0;
                unsigned short localPort = 0;
                char *strsepstr = NULL;
                char *strserverPort = NULL;
                char *strlocalPort = NULL;
                strsepstr = optarg;
                serverIp = strsep(&strsepstr, ":");
                strserverPort = strsep(&strsepstr, ":");
                if(NULL == serverIp || NULL == strserverPort)
                {
                    printf("./rtcp -v View version information  -h help\n");
                    close_keyboard();
                    return 0; 
                }
                strlocalPort = strsep(&strsepstr, ":");
                if(NULL == strlocalPort)
                {
                    localPort = 22;
                }
                else
                {
                    localPort = atoi(strlocalPort);
                }
                serverPort = atoi(strserverPort);
                rtcp_cli_main_start(serverIp,serverPort,localPort);
                break; 
            }
            case 'v':
                printf("build  %s %s\n",__DATE__, __TIME__);
                close_keyboard();
                return -1;
            case 'h':
                rtcp_help();
                close_keyboard();
                return -1;
               
            default:
                printf("./rtcp -v View version information  -h help\n");
                close_keyboard();
                return -1;
        }


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

