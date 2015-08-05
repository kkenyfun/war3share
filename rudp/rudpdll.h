
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


//�궨��
#define UDPSESSION				uint64_t		//
#define INVALID_UDPSESSION		0x0		

//������Ϣ
#define	RUDP_SUCCEED			0			//�ɹ�
#define RUDP_ERROR_TIMEOUT		0x10		//��ʱ
#define RUDP_ERROR_BINDSOCKET	0x11		//bind IP��ַʧ��
#define RUDP_ERROR_BUFSIZE		0x12		//�������Ѿ���	
#define RUDP_ERROR_DATASIZE		0x13		//���ݳ��ȴ���, SESSION_DATA_UDP / SESSION_DATA_RUDP ����ģʽ��.���η������ݳ��Ȳ��ܳ���1024�ֽ�
#define RUDP_ERROR_MEMORY		0x14		//ϵͳ����, �����ڴ�ʧ��
#define RUDP_ERROR_CLOSED		0x15		//�ر�

enum SESSION_STATUS
{
	SESSION_STATUS_NONE			=0,		//��ʼ��״̬
	SESSION_STATUS_CONNECTING	=1,		//��������״̬
	SESSION_STATUS_NORMAL		=2,		//����״̬�������շ�������
	SESSION_STATUS_FIN			=3,		//�����ر�״̬
	SESSION_STATUS_FIN2			=4,		//�Է��ر�״̬
	SESSION_STATUS_CLOSE		=5,		//�Ѿ��ر�״̬
};

//Rudp����־�ȼ�
enum RUDP_LOG_LEVEL
{
	RUDP_LOG_NONE =0,		//����ӡ
	RUDP_LOG_ERROR,			//��ӡ������Ϣ
	RUDP_LOG_WARNING,		//��ӡ������Ϣ		//һ������ȼ�
	RUDP_LOG_NORMAL,		//��ӡ������Ϣ	��Ҫ��������״̬	
	RUDP_LOG_ALL,			//��ӡ������Ϣ�������շ��������Ƚϴ�
};

//Session�Ĳ�������
enum RUDP_SESSION_PARAM
{
	RUDP_SESSION_PARAM_KEEPALIVE_TIME	=11,		//��������ʱ��MSΪ��λ  ȱʡ 6000ms
	RUDP_SESSION_PARAM_SYN_TIME			=12,		//һ�����������Ϊ����ʱ���� + 20 ms  ȱʡ120ms
	RUDP_SESSION_PARAM_WINDOW_SIZE		=13,		//һ�����������Ϊ����ʱ���� + 20 ms  ȱʡ120ms
	RUDP_SESSION_PARAM_PING				=30,		//����PING
	RUDP_SESSION_PARAM_SEND_BUF			=31,		//���ͻ�������С
	RUDP_SESSION_PARAM_WAIT_BUF			=32,		//�ȴ���������С
	RUDP_SESSION_URGENT_STATUS			=33,
};

enum RUDP_CONFIG_PARAM
{
	RUDP_CONFIG_LOG_LEVEL =	10,						//��ӡ��־�ȼ���ȱʡȫ����ӡ ��ϸ �� RUDP_LOG_LEVEL
	RUDP_CONFIG_OUTPUT_LOG=	11,						//�Ƿ��ڲ���ӡ��־�������ӡ����־�� EXE\LOG\***, ����ӡ�Ļ�����ô�ͻ�ͨ���ص��ķ�ʽ����
	RUDP_CONFIG_SEND_DIRECYLY=12,					//����ֱ�ӷ��ͣ� *** ���Լ��ټ���MS����ʱ�������ڿͻ��� �� ѹ����С�ķ������� ȱʡ��ֱ�ӷ���
	RUDP_CONFIG_PRINT_TIME=	13,						//��ӡͳ����Ϣ��ʱ�䣬ȱʡ 60000 ms= 60��
	RUDP_CONFIG_CONNECT_TIMEOUT=14,					//���ӳ�ʱʱ�� ȱʡ 15000ms (msΪ��λ)��

};

//�ص���
class rudp_callback
{
public:
	//�жԷ�����;����
	virtual void on_accept( UDPSESSION Session, uint64_t& wParam, uint64_t& lParam )= 0;
	//�ɹ����ӶԷ�;����
	virtual void on_connection( UDPSESSION Session, uint64_t wParam, uint64_t lParam, int nErrorCode)= 0;
	//�����Ѿ��ر�
	virtual void on_disconnection( UDPSESSION Session, uint64_t wParam, uint64_t lParam )= 0;
	//�յ�����
	virtual void on_rudp_recv( UDPSESSION Session, uint64_t wParam, uint64_t lParam, const uint8_t* pData, int nDataSize, Inet_Addr& sockAddr )= 0;
	virtual void on_udp_recv( const uint8_t* pData, int nDataSize, Inet_Addr& sockAddr, Inet_Addr* pAddrLocal=NULL)= 0;
	//�Ѿ��������ݣ���ʱû������
	virtual void on_rudp_send( UDPSESSION Session, uint64_t wParam, uint64_t lParam, uint32_t callback_id )= 0;
	//��־�ص�
	virtual void on_log_info( const char* pLogInfo, int nLine, int nDebugLev)=0;
};


//�ӿ���
class rudp_module
{
public:
	//����һ������
	virtual UDPSESSION  create_session( Inet_Addr* pAddrFrom = NULL ) = 0;
	virtual int			send_udp( const uint8_t* pData, int nDataSize, const Inet_Addr* pAddrTo, const Inet_Addr* pAddrFrom=NULL )=0;

	//���ӶԷ�;
	virtual int			connect( UDPSESSION Session, Inet_Addr& sockDest)= 0;
	//�ر�һ������
	virtual void		close_session( UDPSESSION Session ) = 0;
	//��������, ��һ�������Ӵ���
	virtual int			send_rudp( UDPSESSION Session, const uint8_t* pData, int nDataSize, uint32_t callback_id )= 0;

	//��ȡ���ӵ�״̬
	virtual int			get_stat( UDPSESSION Session ) = 0;
	//���ûص�����
	virtual int			set_callback_param( UDPSESSION Session, uint64_t wParam, uint64_t lParam) = 0;

	//��ȡ����IP��ַ�ͶԷ���Ip��ַ
	virtual int			get_local_addr( UDPSESSION Session, Inet_Addr& sockAddr )= 0;
	virtual int			get_remote_addr( UDPSESSION Session, Inet_Addr& sockAddr )= 0;

	//���ù������ӵ�������Ϣ
	virtual int			set_session_param( UDPSESSION Session, uint32_t nParamType, void* pData, int nSize) = 0;
	virtual int			get_session_param( UDPSESSION Session, uint32_t nParamType, void* pBuf, int& nBufSize) = 0;
	virtual int			set_config_info( RUDP_CONFIG_PARAM ConfigType, void* pData, int nSize )=0;
};

//��������
RUDPDLL_API rudp_module*	create_rudp_module( rudp_callback* callback, Inet_Addr sock_addr[], uint16_t addr_count, uint32_t max_session_count );
RUDPDLL_API void			close_rudp_module( rudp_module* module );
