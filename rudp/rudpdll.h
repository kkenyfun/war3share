
#pragma once
#include "base_typedef.h"
#include "base_inet_addr.h"


#if defined(RUDPDLL_EXPORTS)
#define RUDPDLL_API extern "C" __declspec(dllexport)
#elif defined(RUDPLIB_EXPORTS)
#define RUDPDLL_API
#else 
#define RUDPDLL_API extern "C" __declspec(dllimport)
#endif

using namespace BASE_NAMEPSACE_DECL;


static const  uint32_t RUDP_PACKET_PL_SIZE		=1024;
static const  uint32_t RUDP_BUF_SIZE			=1280;
static const  uint32_t RUDP_PLAYLOAD_SIZE		=20*1024;

#define UDPSESSION_VERSION		10
#define UDP_SESSION_COUNT		2048


//宏定义
#define UDPSESSION				uint64_t		//
#define INVALID_UDPSESSION		0x0		

//错误信息
#define	RUDP_SUCCEED			0			//成功
#define RUDP_ERROR_TIMEOUT		0x10		//超时
#define RUDP_ERROR_BINDSOCKET	0x11		//bind IP地址失败
#define RUDP_ERROR_BUFSIZE		0x12		//缓冲区已经满	
#define RUDP_ERROR_DATASIZE		0x13		//数据长度错误, SESSION_DATA_UDP / SESSION_DATA_RUDP 两种模式下.单次发送数据长度不能超过1024字节
#define RUDP_ERROR_MEMORY		0x14		//系统错误, 分配内存失败
#define RUDP_ERROR_CLOSED		0x15		//关闭

enum SESSION_STATUS
{
	SESSION_STATUS_NONE			=0,		//初始化状态
	SESSION_STATUS_CONNECTING	=1,		//正在连接状态
	SESSION_STATUS_NORMAL		=2,		//正常状态，可以收发数据了
	SESSION_STATUS_FIN			=3,		//本方关闭状态
	SESSION_STATUS_FIN2			=4,		//对方关闭状态
	SESSION_STATUS_CLOSE		=5,		//已经关闭状态
};

//Rudp的日志等级
enum RUDP_LOG_LEVEL
{
	RUDP_LOG_NONE =0,		//不打印
	RUDP_LOG_ERROR,			//打印错误信息
	RUDP_LOG_WARNING,		//打印警告信息		//一般这个等级
	RUDP_LOG_NORMAL,		//打印常规信息	主要包括连接状态	
	RUDP_LOG_ALL,			//打印所有信息，包括收发包，量比较大
};

//Session的参数类型
enum RUDP_SESSION_PARAM
{
	RUDP_SESSION_PARAM_KEEPALIVE_TIME	=11,		//设置心跳时间MS为单位  缺省 6000ms
	RUDP_SESSION_PARAM_SYN_TIME			=12,		//一般情况下设置为发包时间间隔 + 20 ms  缺省120ms
	RUDP_SESSION_PARAM_WINDOW_SIZE		=13,		//一般情况下设置为发包时间间隔 + 20 ms  缺省120ms
	RUDP_SESSION_PARAM_PING				=30,		//网络PING
	RUDP_SESSION_PARAM_SEND_BUF			=31,		//发送缓冲区大小
	RUDP_SESSION_PARAM_WAIT_BUF			=32,		//等待缓冲区大小
	RUDP_SESSION_URGENT_STATUS			=33,
};

enum RUDP_CONFIG_PARAM
{
	RUDP_CONFIG_LOG_LEVEL =	10,						//打印日志等级，缺省全部打印 详细 见 RUDP_LOG_LEVEL
	RUDP_CONFIG_OUTPUT_LOG=	11,						//是否内部打印日志：如果打印，日志在 EXE\LOG\***, 不打印的话，那么就会通过回调的方式出来
	RUDP_CONFIG_SEND_DIRECYLY=12,					//数据直接发送； *** 可以减少几个MS的延时，适用于客户端 和 压力较小的服务器端 缺省不直接发送
	RUDP_CONFIG_PRINT_TIME=	13,						//打印统计信息的时间，缺省 60000 ms= 60秒
	RUDP_CONFIG_CONNECT_TIMEOUT=14,					//连接超时时间 缺省 15000ms (ms为单位)；

};

//回调类
class rudp_callback
{
public:
	//有对方连接;被动
	virtual void on_accept( UDPSESSION Session, uint64_t& wParam, uint64_t& lParam )= 0;
	//成功连接对方;主动
	virtual void on_connection( UDPSESSION Session, uint64_t wParam, uint64_t lParam, int nErrorCode)= 0;
	//连接已经关闭
	virtual void on_disconnection( UDPSESSION Session, uint64_t wParam, uint64_t lParam )= 0;
	//收到数据
	virtual void on_rudp_recv( UDPSESSION Session, uint64_t wParam, uint64_t lParam, const uint8_t* pData, int nDataSize, Inet_Addr& sockAddr )= 0;
	virtual void on_udp_recv( const uint8_t* pData, int nDataSize, Inet_Addr& sockAddr, Inet_Addr* pAddrLocal=NULL)= 0;
	//已经发送数据，暂时没有意义
	virtual void on_rudp_send( UDPSESSION Session, uint64_t wParam, uint64_t lParam, uint32_t callback_id )= 0;
	//日志回调
	virtual void on_log_info( const char* pLogInfo, int nLine, int nDebugLev)=0;
};


//接口类
class rudp_module
{
public:
	//创建一个连接
	virtual UDPSESSION  create_session( Inet_Addr* pAddrFrom = NULL ) = 0;
	virtual int			send_udp( const uint8_t* pData, int nDataSize, const Inet_Addr* pAddrTo, const Inet_Addr* pAddrFrom=NULL )=0;

	//连接对方;
	virtual int			connect( UDPSESSION Session, Inet_Addr& sockDest)= 0;
	//关闭一个连接
	virtual void		close_session( UDPSESSION Session ) = 0;
	//发送数据, 但一定是连接存在
	virtual int			send_rudp( UDPSESSION Session, const uint8_t* pData, int nDataSize, uint32_t callback_id )= 0;

	//获取连接的状态
	virtual int			get_stat( UDPSESSION Session ) = 0;
	//设置回调参数
	virtual int			set_callback_param( UDPSESSION Session, uint64_t wParam, uint64_t lParam) = 0;

	//获取本地IP地址和对方的Ip地址
	virtual int			get_local_addr( UDPSESSION Session, Inet_Addr& sockAddr )= 0;
	virtual int			get_remote_addr( UDPSESSION Session, Inet_Addr& sockAddr )= 0;

	//设置关于连接的配置信息
	virtual int			set_session_param( UDPSESSION Session, uint32_t nParamType, void* pData, int nSize) = 0;
	virtual int			get_session_param( UDPSESSION Session, uint32_t nParamType, void* pBuf, int& nBufSize) = 0;
	virtual int			set_config_info( RUDP_CONFIG_PARAM ConfigType, void* pData, int nSize )=0;
};

//导出方法
RUDPDLL_API rudp_module*	create_rudp_module( rudp_callback* callback, Inet_Addr sock_addr[], uint16_t addr_count, uint32_t max_session_count );
RUDPDLL_API void			close_rudp_module( rudp_module* module );
