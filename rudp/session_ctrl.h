#pragma once
#include "base_thread_mutex.h"
#include "udp_session.h"

class CUdpSession;
class CSocketThread;
class CDataNode;

class CSessionCtrl : public rudp_module
{
	static const int MAX_UDPPORT_COUNT=	8;
public:
	CSessionCtrl( rudp_callback* pCallback);
	virtual ~CSessionCtrl(void);
	int		initService(  Inet_Addr sockAddr[], uint16_t nAddrCount, uint32_t nMaxSessionCount );
	void	releaseService( uint32_t nTimeout );

	inline rudp_callback*	getCallback( ){return m_pCallback;};

public:
	//Override from IUdpService///////////////////////////////////////
	virtual UDPSESSION  create_session( Inet_Addr* pAddrFrom= NULL );

	virtual int			send_udp( const uint8_t* pData, int nDataSize, const Inet_Addr* pAddrTo, const Inet_Addr* pAddrFrom=NULL );
	virtual int			connect( UDPSESSION Session, Inet_Addr& sockDest);
	virtual void		close_session( UDPSESSION Session );
	virtual int			send_rudp( UDPSESSION Session, const uint8_t* pData, int nDataSize, uint32_t callback_id );
	virtual int			get_stat( UDPSESSION Session );
	virtual int			set_callback_param( UDPSESSION Session, uint64_t wParam, uint64_t lParam);
	virtual int			set_session_param( UDPSESSION Session, uint32_t nParamType, void* pData, int nSize);
	virtual int			get_session_param( UDPSESSION Session, uint32_t nParamType, void* pBuf, int& nBufSize);

	virtual int			get_local_addr( UDPSESSION Session, Inet_Addr& sockAddr );
	virtual int			get_remote_addr( UDPSESSION Session, Inet_Addr& sockAddr );
	//新增加的接口，后续实现
	virtual int			set_config_info( RUDP_CONFIG_PARAM ConfigType, void* pData, int nSize );
	
private:
	void			setLocalAddr( Inet_Addr sockAddr[] , uint16_t nAddrCount );
	int				createSocketThread( uint32_t nSessionCount );
	void			releaseSocketThread();

	int				mallocThread( const Inet_Addr* pAddr = NULL );

private:
	BaseThreadMutex			mutex_;

	rudp_callback*		m_pCallback;
	
	Inet_Addr				m_LocalAddr[MAX_UDPPORT_COUNT ];
	uint16_t				m_nAddrCount;
	CSocketThread*			m_pSocketThread[MAX_UDPPORT_COUNT];
};

