#pragma once

#include "data_node.h"
#include  "rudp_buf.h"
#include "base_inet_addr.h"

class CSessionCtrl;
class CSocketThread;
class CRateCtrl;
class CUdpSession : public CBufCallback
{
public:
	CUdpSession( CSessionCtrl* pSessionCtrl, rudp_callback* pCallback, CSocketThread* pThread, CBaseSendBuf* pSendBuf=NULL, CBaseRecvBuf* pRecvBuf=NULL );

	virtual ~CUdpSession(void);

	inline void* operator new(size_t nSize)
	{
		return m_UdpSessionPool.pop_obj( );
	};
	inline void operator delete(void* p)
	{
		m_UdpSessionPool.push_obj( (CUdpSession*) p );
	};

	static ObjectMutexPool_malloc<CUdpSession, BaseThreadMutex, 256>	m_UdpSessionPool;



	inline UDPSESSION	getSessionNode(){return m_SessionNode; };
	inline uint32_t		getLocalSessionId( ){ return m_nLocalSessionId; };
	inline void			setLocalSessionId( uint32_t nSessionId )
	{
		m_nLocalSessionId=	nSessionId; 
		m_SessionNode=		calcUdpSession(m_nLocalSessionId);
	};
	inline uint32_t			getDestSessionId(){return m_nDestSessionId;};
	inline void				setDestSessionId(uint32_t nSessionId){m_nDestSessionId= nSessionId; };
	inline uint32_t			getRand(){return m_nRand; };
	inline void				setRand(uint32_t nRand){m_nRand= nRand; }; 
	inline SESSION_STATUS	getStatus( ){return m_SessionStatus;};
	void					setStatus( SESSION_STATUS Status );
	void					procTimerEvent( uint32_t nFlag );

	//应用层接口
	inline Inet_Addr& getLocalGw(){return m_sockAddrGw; };
	inline Inet_Addr& getRemoteAddr(){return m_sockAddrRemote; };
	void	setCallbackParam(uint64_t wParam, uint64_t lParam ) ;
	int		setSessionParam( uint32_t nParamType, void* pData, int nSize);
	int		getSessionParam(uint32_t nParamType, void* pBuf, int& nBufSize);
	int		sendDataDirect( const uint8_t* pData, int nDataSize, uint32_t nTimeout=15 );

	void	onAccept( rudp_syn_t* pSyn, Inet_Addr&sockAddr );

	void	Connect(const Inet_Addr& Address);
	void	Disconnect();


	int		procTimer( );
	void	procUdpMsg( CDataNode* pData );
	void	printLog( );

	//发送数据包
	void	sendSyn( );
	void	sendSyn2( int nErrorCode );
	void	sendFin( );
	void	sendFin2( );
	void	sendKeepalive(  );
	void	sendKeepaliveAck(uint32_t nTickCount );
	
	//callback from buf
	virtual void	onAckData( uint32_t seq_id );
	virtual void	onAck2Data( uint32_t seq_id );
	virtual void	onSynData( uint32_t seq_id );
	virtual void	onRepairData( uint32_t seq_id );
	virtual void	onRudpData( uint32_t seq_id, uint8_t* pData, int nSize, bool packet_flag );		
	virtual void	onRudpDataList( CSendDataNode* node[], int count );

	virtual void	onRecvData( const uint8_t* pData, int nSize) ;
	virtual void	onSendData( uint32_t callback_id );
	virtual void	onRegisterTimer( TIMER_INT nTime, uint32_t nFlag );
	virtual TIMER_INT time_now();

private:
	void	procSyn2Msg( rudp_syn_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr );
	void	procFinMsg( session_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr );
	void	procFin2Msg( session_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr );
	void	procUdpDataMsg( rudp_data_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr );
	void	procKeepaliveMsg( rudp_keepalive_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr );
	void	procKeepaliveAckMsg( rudp_keepalive_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr );
	void	procRudpDataMsg( rudp_data_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr );
	void	procRudpAckMsg( rudp_ack_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr );
	void	procRudpAck2Msg( rudp_ack_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr );
	void	procRudpSynMsg( rudp_syn_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr );
	void	procRudpRepairMsg( rudp_repair_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr );


private:
	UDPSESSION calcUdpSession( uint32_t nLocalSessionId );

protected:
	UDPSESSION	m_SessionNode;		//SESSIONNODE
	uint32_t		m_nLocalSessionId;
	uint32_t		m_nDestSessionId;
	uint32_t		m_nRand;			//初始化建立连接时的随机数
	uint64_t		m_wParam;			//应用层ATTACH的参数
	uint64_t		m_lParam;			//应用层ATTACH的参数

	Inet_Addr	m_sockAddrGw;		//本地IP地址
	Inet_Addr	m_sockAddrRemote;		//对方Ip地址(GW).

	TIMER_INT	m_nCreateTime;			//建立连接的时间
	TIMER_INT	m_nStatusTime;			//状态开始时间
	TIMER_INT	m_nRetryTime;			//重发时间
	TIMER_INT	m_nRecvKeepaliveTime;	//收到心跳时间
	TIMER_INT	m_nSendKeepaliveTime;	//发送心跳时间

	//参数设置
	uint32_t			m_nCfgKeepaliveTime;

	SESSION_STATUS		m_SessionStatus;	//连接状态机


	//接口部分
	CSessionCtrl*		m_pSessionCtrl;	
	rudp_callback*		m_pCallback;
	CSocketThread*		m_pSocketThread;
	CBaseRecvBuf*		m_pRecvBuf;
	CBaseSendBuf*		m_pSendBuf;
	CRateCtrl			m_RateCtrl;

	uint8_t				m_nBufFlag;		//BUF是否是使用缺省的BUFF
};


