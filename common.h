#include<stdio.h>

//printf("[%s %s] %s: %s: %d\n", __DATE__, __TIME__, __FILE__, __func__, __LINE__);
//#define RTCP_PRINTF(fmt, ...) printf("[%s %s] %s: %s: %d:"fmt, __DATE__, __TIME__,__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define RTCP_PRINTF(fmt,args...) do{printf("[%s %s] FILE:[%s]--[%s]LINE:[%d]:"fmt, __DATE__, __TIME__,__FILE__,__FUNCTION__,__LINE__,##args);}while(0);
