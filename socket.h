#ifndef __SOCKET_H_
#define __SOCKET_H_

#define SOCKET_BLOCK     0   
#define SOCKET_NOBLOCK   1

#define SOCK_LISTEN_NUM    (10)

#define NO_WAIT             0
#define WAIT_FOREVER        (-1)

#define true    1
#define false   0

#ifndef SAFE_CLOSE
#define SAFE_CLOSE(fd) do { \
	if (-1 != fd) { \
		close(fd); \
		fd = -1; \
	} \
} while (0)
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(ptr) do { \
	if (NULL == ptr) { \
		free(ptr); \
		ptr = NULL; \
	} \
} while (0)
#endif

#define RTCP_PRINTF(fmt,args...) do{printf("[%s %s] FILE:[%s]--[%s]LINE:[%d]:"fmt, __DATE__, __TIME__,__FILE__,__FUNCTION__,__LINE__,##args);}while(0);
typedef void* (*svr_process_thread)(void *);


/*
* @Function: svr_init
* @Description:服务端初始化函数
* @Input:arg  port 端口  func 数据处理自定义回调函数，可在该函数实现收发  uWaitMsec  等待链接超时  isBlock 是否阻塞 SOCKET_BLOCK 阻塞 SOCKET_NOBLOCK 非阻塞
* @Output:NULL
* @Return:成功 0  失败 -1
*/
int svr_init(unsigned short int port, svr_process_thread func, int uWaitMsec);

/*
* @Function: socket_connect_wait
* @Description:close socket
* @Input:sHostName  ip  port 端口  uWaitMsec  超时时间  isBlock 是否阻塞 SOCKET_BLOCK 阻塞 SOCKET_NOBLOCK 非阻塞
* @Output:NULL
* @Return: NULL
*/
int socket_client_tcp_create_ipv4(const char *sHostName, int uPort, int uWaitMsec, int isBlock);
#endif