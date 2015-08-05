#pragma once
#include "udp_session.h"
#include "session_ctrl.h"
#include "rudp_timer.h"
#include "base_thread_mutex.h"
#include "base_namespace.h"
#include "base_typedef.h"
#include "base_thread.h"
#include "base_sock_dgram.h"

class CSessionCtrl;
class CDataNode;


using namespace BASE_NAMEPSACE_DECL;



class CSocketThread : public CThread
{

public:
	CSocketThread( CSessionCtrl * pService, Inet_Addr& sockAddr );
	virtual ~CSocketThread(void);

	int				initSocket( uint16_t nIndex, uint32_t nMaxSessionCount );
	void			releaseSocket( );

	uint64_t		createUdpSession();
	void			sendUdpData( const uint8_t* pData, int nDataSize, const Inet_Addr* pAddrTo );

	int				connect( UDPSESSION Session, Inet_Addr& sockDest);
	void			closeUdpSession( UDPSESSION Session );
	int				sendData( UDPSESSION Session, const uint8_t* pData, int nDataSize, uint32_t nTimeout=15 );
	int				sendDataDirect( UDPSESSION Session, const uint8_t* pData, int nDataSize, uint32_t nTimeout=15 );

	int				getSessionStat( UDPSESSION Session );
	int				setCallbackParam( UDPSESSION Session, uint64_t wParam, uint64_t lParam);
	int				setSessionParam( UDPSESSION Session, uint32_t nParamType, void* pData, int nSize);
	int				getSessionParam( UDPSESSION Session, uint32_t nParamType, void* pBuf, int& nBufSize);
	int				getLocalAddr( UDPSESSION Session, Inet_Addr& sockAddr );
	int				getRemoteAddr( UDPSESSION Session, Inet_Addr& sockAddr );



	inline uint16_t getPort(){ return m_sockAddr.get_port(); };
	inline uint32_t getIpAddr(){ return m_sockAddr.get_ip(); };
	inline TIMER_INT	get_now(){return m_nTime_Now; };

	void Sendto( const uint8_t* pData, int nSize, Inet_Addr& sockAddr );

protected:	
	virtual void execute();

protected:
	CSockDgram		m_sUdp;
	Inet_Addr		m_sockAddr;
	 uint16_t		m_nIndex;

	CSessionCtrl*	m_pSessionCtrl;
private:
	void			msSleep( uint32_t nMs );
	bool			Update( );
	void			chkUpdate( );
	void			procUdpMsg( CDataNode* pData );
	void			procSynMsg( uint32_t nThreadIndex, rudp_syn_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr );
	void			procSendNode( CSendDataNode* pNode );

	void			printLogInfo();
	CUdpSession*	getUdpSessionfromSessionId( uint32_t nLocalSessionId, uint32_t nDestSessionId=0 );
	CUdpSession*	getUdpSesionfromSessionNode( UDPSESSION Socket);
	CUdpSession*	chkSynSession( uint32_t nRand, uint32_t nDestSessionId );
	int				addUdpSession( CUdpSession* pConnection );

	int				createSessionBuf( uint32_t nSessionCount );
	void			releaseSession( );


	TIMER_INT		m_nTimeLog;
	TIMER_INT		m_nTimeChk;
	TIMER_INT		m_nTime_Now;

	CUdpSession**			m_pSession;
	uint32_t				m_nMaxIndex;
	uint32_t				m_nMaxSessionCount;
	BaseThreadMutex			mutex_;

public:
	CRudpTimerManager		m_TimerManager;
	rudp_static_info_t		m_stStaticInfo;
};

