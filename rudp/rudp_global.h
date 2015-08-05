#pragma once

#include "base_namespace.h"
#include "base_typedef.h"
#include "base_inet_addr.h"
#include "base_timer_value.h"
#include "rudpdll.h"

using namespace BASE_NAMEPSACE_DECL;

class rudp_static_info_t
{
public:
	rudp_static_info_t()
	{
		memset(this, 0, sizeof(rudp_static_info_t) );
	}
	int nConnectionCount;
	int nAcceptCount;
	int	nDisConnectionCount;
	uint32_t nRecvCount;
	uint32_t nSendCount;
	uint32_t nSendDataCount;
	uint32_t nRecvDataCount;

};


typedef struct _rudp_config
{
	static const uint32_t SESSION_UPDATE_TIME=		1000;

	const static uint32_t SESSION_CLOSE_TIME			=3000;
	const static uint32_t	SESSION_CONNECT_TIME		=15000;
	const static uint32_t SESSION_KEEPALIVE_TIME		=6000;
	const static uint32_t SESSION_RESEND_TIME			=2200;
	const static uint32_t SESSION_TIMEOUT_TIME		=120000;
	const static uint32_t SESSION_NONE_TIME			=120000;


	_rudp_config()
	{
		nLogLevel=			RUDP_LOG_ALL;
		nOutputLogFlag=		0;
		nPrintLogTime=		60000;

		nSendDirectlyFlag=	0;
		nConnectionTimeout=	SESSION_CONNECT_TIME;
	}
	~_rudp_config(){}

	uint32_t	nLogLevel;					//日志等级
	uint32_t	nOutputLogFlag;				//打印日志的方式
	uint32_t	nPrintLogTime;				//打印统计数据日志时间
	uint32_t	nSendDirectlyFlag;			//是否直接发送
	uint32_t	nConnectionTimeout;			//连接超时时间
}rudp_config_t;

extern rudp_config_t	g_RudpCfgInfo;
extern rudp_callback*	g_pCallback;


#define OUT_PUT_LOG( log_level, arg )\
{\
	if( log_level <= g_RudpCfgInfo.nLogLevel )\
	{\
		ostringstream log;\
		log  << arg << std::endl;\
		if( g_pCallback ) g_pCallback->on_log_info( log.str().c_str(), __LINE__, log_level);\
	}\
};

#define SAFE_DELETE(p)	{if(p){delete p; p=NULL;}}

#define	TIMER_INT	uint64_t

static TIMER_INT MyGetTickCount( )
{
	return CBaseTimeValue::get_time_value().msec();
};
//static TIMER_INT	g_nTickNow=0;


/////////////////////协议错误代码/////////////////////////
#define ERROR_SESSIONID			10
#define ERROR_PACKET_SIZE		11
#define ERROR_SYN_RAND			12
#define ERROR_SYN_SIZE			13
#define ERROR_SYSTEM			14
