#include "udp_session.h"
#include "session_ctrl.h"
#include "socket_thread.h"
#include "rudp_log_macro.h"

#include "rudp_buf.h"


ObjectMutexPool_malloc<CUdpSession, BaseThreadMutex, 256>	CUdpSession::m_UdpSessionPool;

CUdpSession::CUdpSession( CSessionCtrl* pSessionCtrl, rudp_callback* pCallback, CSocketThread* pThread,  CBaseSendBuf* pSendBuf, CBaseRecvBuf* pRecvBuf )
	: m_pSessionCtrl(pSessionCtrl), m_pCallback(pCallback), m_pSocketThread(pThread)
{
	if( pSendBuf == NULL )
	{
		m_pSendBuf= new CRudpPushSendBuf( this, m_RateCtrl );
		m_pRecvBuf= new CRudpPushRecvBuf( this, m_RateCtrl );
		m_nBufFlag= 1;
	}
	else
	{
		m_pSendBuf= pSendBuf;
		m_pRecvBuf= pRecvBuf;
		m_nBufFlag= 0;
	}
	srand( (uint32_t)m_pSocketThread->get_now() );


	m_nLocalSessionId=	0;
	m_nDestSessionId=	0;
	m_nRand=			(uint32_t)(rand() * rand()) ;
	m_SessionNode=		INVALID_UDPSESSION;
	m_lParam=			0;
	m_wParam=			0;

	m_nCreateTime=			m_pSocketThread->get_now();
	m_nRetryTime=			m_pSocketThread->get_now();
	m_nRecvKeepaliveTime=	m_pSocketThread->get_now();
	m_nSendKeepaliveTime=	m_pSocketThread->get_now();
	m_SessionStatus=		SESSION_STATUS_NONE;

	//连接的缺省参数
	m_nCfgKeepaliveTime=	_rudp_config::SESSION_KEEPALIVE_TIME;
}


CUdpSession::~CUdpSession(void)
{
	if( SESSION_STATUS_CONNECTING == m_SessionStatus || SESSION_STATUS_NORMAL != m_SessionStatus )
	{
		RUDP_INFO( "<CUdpSession::~CUdpSession> L = " << m_nLocalSessionId << "D = " << m_nDestSessionId );
		Disconnect();
	}

	if( m_nBufFlag )
	{
		SAFE_DELETE(m_pSendBuf);
		SAFE_DELETE(m_pRecvBuf);
	}
	return;
}

UDPSESSION CUdpSession::calcUdpSession( uint32_t nLocalSessionId )
{
	UDPSESSION Id= nLocalSessionId;
	Id = (Id<< 32);
	Id+= (uint32_t)m_pSocketThread->get_now();
	return Id;
};


void CUdpSession::onAckData( uint32_t seq_id)
{
	RUDP_DEBUG( "<CUdpSession::onAckData> L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "seq_id = " << seq_id);

	rudp_ack_head_t Head;
	Head.nCmd=				SESSION_CMD_RUDP_ACK;
	Head.nSize=				(uint16_t)sizeof(rudp_ack_head_t);
	Head.nReserve=			RUDP_PROTOCOL_RESERVE;
	Head.nDestSessionId=	m_nDestSessionId;
	Head.nAckSeqId=			seq_id;

	m_pSocketThread->Sendto( (const uint8_t*)&Head, Head.nSize, m_sockAddrRemote );
}

void CUdpSession::onAck2Data( uint32_t seq_id )
{
	RUDP_DEBUG( "<CUdpSession::onAck2Data> L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "seq_id = " << seq_id);

	rudp_ack_head_t Head;
	Head.nCmd=				SESSION_CMD_RUDP_ACK_2;
	Head.nSize=				(uint16_t)sizeof(rudp_ack_head_t);
	Head.nReserve=			RUDP_PROTOCOL_RESERVE;
	Head.nDestSessionId=	m_nDestSessionId;
	Head.nAckSeqId=			seq_id;

	m_pSocketThread->Sendto( (const uint8_t*)&Head, Head.nSize, m_sockAddrRemote );
}


void CUdpSession::onRepairData( uint32_t seq_id )
{
	RUDP_DEBUG( "<CUdpSession::onRepairData> L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "seq_id = " << seq_id);
	rudp_repair_head_t Head;
	Head.nCmd=				SESSION_CMD_RUDP_REPAIR;
	Head.nSize=				(uint16_t)sizeof(rudp_ack_head_t);
	Head.nReserve=			RUDP_PROTOCOL_RESERVE;
	Head.nDestSessionId=	m_nDestSessionId;
	Head.nRepairSeqId=		seq_id;

	m_pSocketThread->Sendto( (const uint8_t*)&Head, Head.nSize, m_sockAddrRemote );
}


void CUdpSession::onSynData( uint32_t seq_id )
{
	RUDP_DEBUG( "<CUdpSession::onSynData> L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "seq_id = " << seq_id);

	rudp_syn_head_t Head;
	Head.nCmd=				SESSION_CMD_RUDP_SYN;
	Head.nSize=				(uint16_t)sizeof(rudp_ack_head_t);
	Head.nReserve=			RUDP_PROTOCOL_RESERVE;
	Head.nDestSessionId=	m_nDestSessionId;
	Head.nSynSeqId=			seq_id;

	m_pSocketThread->Sendto( (const uint8_t*)&Head, Head.nSize, m_sockAddrRemote );
}

void CUdpSession::onRudpData( uint32_t seq_id, uint8_t* pData, int nSize, bool packet_flag )
{
	rudp_data_head_t* pHead=	(rudp_data_head_t*)pData;
	if( packet_flag )
	{
		pHead->nCmd=			SESSION_CMD_RUDP_PACKET;
	}
	else
	{
		pHead->nCmd=			SESSION_CMD_RUDP_DATA;
	}
	pHead->nSize=			nSize;
	pHead->nReserve=		RUDP_PROTOCOL_RESERVE;
	pHead->nDestSessionId=	m_nDestSessionId;
	pHead->nSeqId=			seq_id;
	pHead->nAckSeqId=		m_pRecvBuf->getAckId();

	RUDP_DEBUG( "<CUdpSession::onRudpData> L = " << m_nLocalSessionId << "D = " << m_nDestSessionId  << "seq_id = " << seq_id << "ack_seqid = " << pHead->nAckSeqId << "size = " << nSize);

	m_pSocketThread->Sendto( pData, pHead->nSize, m_sockAddrRemote );
}


void CUdpSession::onRudpDataList( CSendDataNode* node[], int count )
{
	uint8_t buf[RUDP_BUF_SIZE]={0};
	int total_size=0;
	for( int i=0; i< count; i++ )
	{
		if( total_size + node[i]->m_nDataSize > RUDP_BUF_SIZE )
		{
			RUDP_ERROR( __FUNCTION__ << " size error total_size = " << total_size << " next size = " << node[i]->m_nDataSize );
			break;
		}
		rudp_data_head_t* pHead=	(rudp_data_head_t*)(node[i]->m_szBuf);
		if( node[i]->packet_flag_ )
		{
			pHead->nCmd=			SESSION_CMD_RUDP_PACKET;
		}
		else
		{
			pHead->nCmd=			SESSION_CMD_RUDP_DATA;
		}
		pHead->nSize=			node[i]->m_nDataSize;
		pHead->nReserve=		RUDP_PROTOCOL_RESERVE;
		pHead->nDestSessionId=	m_nDestSessionId;
		pHead->nSeqId=			node[i]->m_nSeqId;
		pHead->nAckSeqId=		m_pRecvBuf->getAckId();

		memcpy( buf + total_size, node[i]->m_szBuf, node[i]->m_nDataSize );
		total_size+= node[i]->m_nDataSize;
	}
	m_pSocketThread->Sendto( buf, total_size, m_sockAddrRemote );
}


void CUdpSession::onRecvData( const uint8_t* pData, int nSize )
{
	RUDP_DEBUG( "<CUdpSession::onRecvData> L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "size = " << nSize);
	m_pSocketThread->m_stStaticInfo.nRecvCount++;
	m_pCallback->on_rudp_recv( m_SessionNode, m_wParam, m_lParam, pData, nSize, m_sockAddrRemote );
}

void CUdpSession::onSendData( uint32_t callback_id )
{
	m_pCallback->on_rudp_send( m_SessionNode, m_wParam, m_lParam, callback_id );
}

void CUdpSession::onRegisterTimer( TIMER_INT nTime, uint32_t nFlag )
{
	m_pSocketThread->m_TimerManager.registerTimer( nTime, m_nLocalSessionId, nFlag );
}
TIMER_INT CUdpSession::time_now()
{
	return m_pSocketThread->get_now();
}


void CUdpSession::Connect( const Inet_Addr& sockDest )
{
	RUDP_INFO( "<CUdpSession::Connect> L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "ip = " << sockDest );

	memcpy( &m_sockAddrRemote, &sockDest, sizeof(Inet_Addr) );
	setStatus( SESSION_STATUS_CONNECTING );

	sendSyn( );

	return;
}

void CUdpSession::Disconnect()
{
	if( SESSION_STATUS_CONNECTING == getStatus() || getStatus() == SESSION_STATUS_NORMAL )
	{
		RUDP_INFO( "<CUdpSession::Disconnect> L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "ip = " << m_sockAddrRemote );
		sendFin( );
		m_pSocketThread->m_stStaticInfo.nDisConnectionCount++;
		m_pSessionCtrl->getCallback()->on_disconnection( m_SessionNode, m_wParam, m_lParam );
		setStatus( SESSION_STATUS_FIN );
	}
}

void CUdpSession::setCallbackParam(uint64_t wParam, uint64_t lParam ) 
{
	m_wParam= wParam;
	m_lParam= lParam;
}

int	CUdpSession::setSessionParam( uint32_t nParamType, void* pData, int nSize)
{
	switch( nParamType )
	{
		case RUDP_SESSION_PARAM_SYN_TIME:
			{
				if( nSize == sizeof(int ))
				{
					int nTime=0;
					memcpy( &nTime, pData, sizeof(int ));
					if( nTime > 20 )
					{
						return 0;
					}
				}
			}
			break;
		case RUDP_SESSION_PARAM_KEEPALIVE_TIME:
			{
				if( nSize == sizeof(int ))
				{
					memcpy( &m_nCfgKeepaliveTime, pData, sizeof(int ));
					return 0;
				}
			}
			break;

		case RUDP_SESSION_PARAM_WINDOW_SIZE:
			{
				if( nSize == sizeof(int) )
				{
					int nSize=0;
					memcpy( &nSize, pData, sizeof(int ));
					m_RateCtrl.setWindowSize( nSize );
					RUDP_INFO( "<CUdpSession::setSessionParam > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "WINDOW_SIZE = " << nSize );
					return 0;
				}
			}
			break;
		case RUDP_SESSION_URGENT_STATUS:
			{
				if( nSize == sizeof(int) )
				{
					int flag=0;
					memcpy( &flag, pData, sizeof(int ));
					m_pSendBuf->set_urgent_flag( flag != 0  );
					RUDP_INFO( "<CUdpSession::setSessionParam > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "URGENT = " << flag );
					return 0;
				}
			}
			break;
		default :
			{

			}
			break;
	}
	return -1;
};

int	CUdpSession::getSessionParam(uint32_t nParamType, void* pBuf, int& nBufSize)
{
	int nRet= -1;
	switch( nParamType )
	{
		case RUDP_SESSION_PARAM_SYN_TIME:
			{
				if( nBufSize >= sizeof(int ))
				{
					uint32_t nSize= m_RateCtrl.getSynTime();
					memcpy( pBuf, &nSize, sizeof(uint32_t ) );
					nBufSize = sizeof(uint32_t);
					return 0;
				}
			}
			break;

		case RUDP_SESSION_PARAM_KEEPALIVE_TIME:
			{
				if( nBufSize >= sizeof(int ))
				{
					memcpy( pBuf, &m_nCfgKeepaliveTime, sizeof(uint32_t ) );
					nBufSize = sizeof(uint32_t);
					return 0;
				}
			}
			break;
		case RUDP_SESSION_PARAM_WINDOW_SIZE:
			{
				if( nBufSize >= sizeof(int) )
				{
					uint32_t size= m_RateCtrl.getWindowSize();
					memcpy( pBuf, &size, sizeof(uint32_t ) );
					nBufSize = sizeof(uint32_t);
					return 0;
				}
			}
			break;

		case RUDP_SESSION_PARAM_PING:
			{
				if( nBufSize >= sizeof(int) )
				{
					uint32_t ping= m_RateCtrl.getPing();
					memcpy( pBuf, &ping, sizeof(uint32_t ) );
					nBufSize = sizeof(uint32_t);
					return 0;
				}
			}
			break;

		case RUDP_SESSION_PARAM_SEND_BUF:
			{
				if( nBufSize >= sizeof(int) )
				{
					uint32_t ping= m_pSendBuf->getBuffingSize();
					memcpy( pBuf, &ping, sizeof(uint32_t ) );
					nBufSize = sizeof(uint32_t);
					return 0;
				}
			}
			break;

		case RUDP_SESSION_PARAM_WAIT_BUF:
			{
				if( nBufSize >= sizeof(int) )
				{
					uint32_t ping= m_pSendBuf->getWaitSize( );
					memcpy( pBuf, &ping, sizeof(uint32_t ) );
					nBufSize = sizeof(uint32_t);
					return 0;
				}
			}
			break;

		default :
			{

			}
			break;
	}
	return -1;		
}


int	CUdpSession::sendDataDirect( const uint8_t* pData, int nSize, uint32_t callback_id )
{
	if( getStatus() != SESSION_STATUS_NORMAL )
	{
		RUDP_DEBUG( "<CUdpSession::sendDataDirect status error > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "size = " << nSize );
		return -1;
	}

	if( nSize > RUDP_PLAYLOAD_SIZE )
	{
		RUDP_DEBUG( "<CUdpSession::sendDataDirect size error > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "size = " << nSize );
		return -1;
	}

	if( g_RudpCfgInfo.nLogLevel >= RUDP_LOG_ALL )
	{
		RUDP_DEBUG( "<CUdpSession::sendDataDirect > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "size = " << nSize );
	}

	int nRet= 0;
	int	 nReadSize=0;
	uint32_t nPacketSize=0;
	CSendDataNode* pNode= NULL;
	while( nReadSize < nSize )
	{
		nPacketSize= ( (nSize - nReadSize) > RUDP_PACKET_PL_SIZE ? RUDP_PACKET_PL_SIZE :(nSize - nReadSize) );
		CSendDataNode*  pNode =new CSendDataNode( m_SessionNode, pData + nReadSize, nPacketSize );
		nReadSize+= nPacketSize;

		if( pNode == NULL )
		{
			nRet= -1;
			break;
		}

		if( nReadSize >= nSize )
		{
			pNode->packet_flag_ = true;
			pNode->m_nCallbackId= callback_id;
		}

		m_pSendBuf->addSendNode( pNode );
	}
	return 0;
}


void CUdpSession::onAccept( rudp_syn_t* pSyn, Inet_Addr&sockAddr )
{
	RUDP_INFO( "<CUdpSession::onAccept  > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "SESSION = " << m_SessionNode << "remote_addr = " << sockAddr  );

	m_sockAddrRemote = sockAddr;
	sendSyn2( 0 );
	m_pSocketThread->m_stStaticInfo.nAcceptCount++;
	m_pCallback->on_accept( m_SessionNode, m_wParam, m_lParam );
	setStatus( SESSION_STATUS_NORMAL );
}

void CUdpSession::procUdpMsg( CDataNode* pData )
{
	session_head_t* pHead= (session_head_t*)pData->m_szDataBuf;

	if( m_sockAddrRemote != pData->m_sockAddr )
	{
		RUDP_INFO( "<CUdpSession::procUdpMsg address change > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "old = " << m_sockAddrRemote << "new = " << pData->m_sockAddr  );
		//保存IP地址
		memcpy( &m_sockAddrRemote, &pData->m_sockAddr, sizeof(Inet_Addr) );
	}

	switch( pHead->nCmd )
	{
			//处理连接请求的返回（请求方）
		case SESSION_CMD_SYN2:
			{
				procSyn2Msg(  (rudp_syn_t*)pHead, pData->m_szDataBuf + sizeof(rudp_syn_t), pData->m_nDataSize - sizeof(rudp_syn_t), pData->m_sockAddr );
			}
			break;
			//处理结束连接（服务方）
		case SESSION_CMD_FIN:
			{
				procFinMsg(  pHead, pData->m_szDataBuf + sizeof(session_head_t), pData->m_nDataSize - sizeof(session_head_t), pData->m_sockAddr );
			}
			break;
			//处理连接结束（请求方）
		case SESSION_CMD_FIN2:
			{
				procFin2Msg( pHead, pData->m_szDataBuf + sizeof(session_head_t), pData->m_nDataSize - sizeof(session_head_t), pData->m_sockAddr );
			}
			break;

			//心跳
		case SESSION_CMD_KEEPALIVE:
			{
				procKeepaliveMsg( (rudp_keepalive_head_t*)pHead, pData->m_szDataBuf + sizeof(rudp_keepalive_head_t), pData->m_nDataSize- sizeof(rudp_keepalive_head_t), pData->m_sockAddr );
			}
			break;
			//心跳返回的消息
		case SESSION_CMD_KEEPALIVE_ACK:
			{
				procKeepaliveAckMsg( (rudp_keepalive_head_t*)pHead, pData->m_szDataBuf + sizeof(rudp_keepalive_head_t), pData->m_nDataSize- sizeof(rudp_keepalive_head_t), pData->m_sockAddr );
			}
			break;

		//处理RUDP数据
		case SESSION_CMD_RUDP_PACKET:
		case SESSION_CMD_RUDP_DATA:
			{
				int pos = 0;
				while( pos < pData->m_nDataSize )
				{
					rudp_data_head_t* head = (rudp_data_head_t*)(pData->m_szDataBuf + pos);
					if( head->nSize < sizeof(rudp_data_head_t ) )
					{
						break;
					}
					if( head->nSize + pos <= pData->m_nDataSize )
					{
						procRudpDataMsg( head, pData->m_szDataBuf + pos + sizeof(rudp_data_head_t), head->nSize - sizeof(rudp_data_head_t), pData->m_sockAddr );
						pos+= head->nSize;
					}
					else
					{
						break;
					}
				}
			}
			break;

			//处理RUDP确认
		case SESSION_CMD_RUDP_ACK:
			{
				procRudpAckMsg( (rudp_ack_head_t*)pHead, pData->m_szDataBuf + sizeof(rudp_ack_head_t), pData->m_nDataSize- sizeof(rudp_ack_head_t), pData->m_sockAddr );
			}
			break;
		case SESSION_CMD_RUDP_ACK_2:
			{
				procRudpAck2Msg( (rudp_ack_head_t*)pHead, pData->m_szDataBuf + sizeof(rudp_ack_head_t), pData->m_nDataSize- sizeof(rudp_ack_head_t), pData->m_sockAddr );
			}
			break;
			//处理RUDP确认
		case SESSION_CMD_RUDP_REPAIR:
			{
				procRudpRepairMsg( (rudp_repair_head_t*)pHead, pData->m_szDataBuf + sizeof(rudp_repair_head_t), pData->m_nDataSize- sizeof(rudp_repair_head_t), pData->m_sockAddr );
			}
			break;
			//处理RUDP确认
		case SESSION_CMD_RUDP_SYN:
			{
				procRudpSynMsg( (rudp_syn_head_t*)pHead, pData->m_szDataBuf + sizeof(rudp_syn_head_t), pData->m_nDataSize- sizeof(rudp_syn_head_t), pData->m_sockAddr );
			}
			break;

		default:
			{
			}
			break;
	}
	return;
}

void CUdpSession::procSyn2Msg( rudp_syn_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr )
{
	//如果连接状态不对,那么直接返回
	if( getStatus() != SESSION_STATUS_CONNECTING )
	{ 
		return;
	}
	RUDP_INFO( "<CUdpSession::procSyn2Msg  > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "remote_addr = " << m_sockAddrRemote  );
	///////////////////////////////////////////////////
	m_nDestSessionId= pMsgHead->nLocalSessionId;

	//连接出现错误
	if( pMsgHead->nErrorCode != 0 )
	{
		RUDP_WARNING( "<CUdpSession::procSyn2Msg  > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "remote_addr = " << m_sockAddrRemote << "error_code = " <<  pMsgHead->nErrorCode );

		m_pCallback->on_connection( m_SessionNode, m_wParam, m_lParam, pMsgHead->nErrorCode );
		setStatus( SESSION_STATUS_CLOSE );
		return;
	}

	memcpy(&m_sockAddrRemote, &sockAddr, sizeof(sockAddr) );
	m_sockAddrGw.set_ip(	pMsgHead->nGwIp);
	m_sockAddrGw.set_port( pMsgHead->nGwPort);

	sendKeepalive( );
	setStatus( SESSION_STATUS_NORMAL );

	m_pCallback->on_connection( m_SessionNode, m_wParam, m_lParam, 0 );
	return;
}

void CUdpSession::procFinMsg( session_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr )
{
	RUDP_INFO( "<CUdpSession::procFinMsg  > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "remote_addr = " << sockAddr  );

	sendFin2( );

	if( getStatus() == SESSION_STATUS_NORMAL )
	{
		m_pSocketThread->m_stStaticInfo.nDisConnectionCount++;
		m_pSessionCtrl->getCallback()->on_disconnection( m_SessionNode, m_wParam, m_lParam );
	}
	setStatus( SESSION_STATUS_FIN2 );
}

void CUdpSession::procFin2Msg( session_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr )
{
	RUDP_INFO( "<CUdpSession::procFin2Msg  > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "remote_addr = " << sockAddr  );


	if( getStatus() == SESSION_STATUS_NORMAL )
	{
		RUDP_WARNING( "<CUdpSession::procFin2Msg > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "Status = " << getStatus() );
		return;
	}
	
	if( SESSION_STATUS_CLOSE != getStatus() )
	{
		sendFin2();
	}
	setStatus( SESSION_STATUS_CLOSE );
}

//计算用户的ping值
void CUdpSession::procKeepaliveMsg( rudp_keepalive_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr )
{
	RUDP_DEBUG( "<CUdpSession::procKeepaliveMsg > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId );


	sendKeepaliveAck( pMsgHead->nTickCount );
	return;
}

//计算用户的ping值
void CUdpSession::procKeepaliveAckMsg( rudp_keepalive_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr )
{
	if( m_pSocketThread->get_now() >= pMsgHead->nTickCount )
	{
		uint16_t ping = (uint16_t)(m_pSocketThread->get_now() - pMsgHead->nTickCount);
		m_RateCtrl.setPing( ping );

		RUDP_DEBUG( "<CUdpSession::procKeepaliveAckMsg > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "ping = " << ping );
	}
	m_nRecvKeepaliveTime= m_pSocketThread->get_now();

	return;
}

void CUdpSession::procRudpDataMsg( rudp_data_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr )
{
	if( getStatus() != SESSION_STATUS_NORMAL )
	{
		return;
	}
	if( nSize < 0 )
	{
		return;
	}
	RUDP_DEBUG( "<CUdpSession::procRudpDataMsg > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "seq_id = " << pMsgHead->nSeqId << "ack_seq_id = " << pMsgHead->nAckSeqId << "size = " << nSize);
	m_pSendBuf->onDataAck( pMsgHead->nAckSeqId );


	int nErrorCode= m_pRecvBuf->addRecvData( pMsgHead->nSeqId, pData, nSize, (pMsgHead->nCmd == SESSION_CMD_RUDP_PACKET) );
	return;
}

void CUdpSession::procRudpAckMsg( rudp_ack_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr )
{
	RUDP_DEBUG( "<CUdpSession::procRudpAckMsg > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "seq_id = " << pMsgHead->nAckSeqId );
	m_pSendBuf->onDataAck( pMsgHead->nAckSeqId );
}


void CUdpSession::procRudpAck2Msg( rudp_ack_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr )
{
	RUDP_DEBUG( "<CUdpSession::procRudpAck2Msg > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "seq_id = " << pMsgHead->nAckSeqId );
	m_pSendBuf->onDataAck2( pMsgHead->nAckSeqId );
}

void CUdpSession::procRudpSynMsg( rudp_syn_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr )
{
	RUDP_DEBUG( "<CUdpSession::procRudpSynMsg > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "seq_id = " << pMsgHead->nSynSeqId );
	m_pRecvBuf->onDataSyn( pMsgHead->nSynSeqId );
}

void CUdpSession::procRudpRepairMsg( rudp_repair_head_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr )
{
	RUDP_DEBUG( "<CUdpSession::rudp_repair > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "seq_id = " << pMsgHead->nRepairSeqId );
	m_pSendBuf->onDataRepair( pMsgHead->nRepairSeqId );
}

void CUdpSession::sendSyn( )
{
	RUDP_INFO( "<CUdpSession::sendSyn > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId  );


	rudp_syn_t Head ;
	memset( &Head, 0, sizeof(rudp_syn_t) );
		
	Head.nCmd=				SESSION_CMD_SYN;
	Head.nSize=				sizeof(rudp_syn_t);
	Head.nReserve=			RUDP_PROTOCOL_RESERVE;
	Head.nDestSessionId=	m_nRand;
	Head.nLocalSessionId=	m_nLocalSessionId;
	Head.nVersion=			RUDP_PROTOCOL_VERSION;


	m_pSocketThread->Sendto( (const uint8_t*)&Head, Head.nSize, m_sockAddrRemote );
}

void CUdpSession::sendSyn2( int nErrorCode )
{
	RUDP_INFO( "<CUdpSession::sendSyn2 > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId  );

	rudp_syn_t Head;
	memset( &Head, 0, sizeof(rudp_syn_t) );
	Head.nCmd=				SESSION_CMD_SYN2;
	Head.nSize=				sizeof(rudp_syn_t);
	Head.nReserve=			RUDP_PROTOCOL_RESERVE;
	Head.nDestSessionId=	m_nDestSessionId;
	Head.nLocalSessionId=	m_nLocalSessionId;
	Head.nVersion=			RUDP_PROTOCOL_VERSION;
	Head.nGwIp=				m_sockAddrRemote.get_ip();
	Head.nGwPort=			m_sockAddrRemote.get_port();
	Head.nErrorCode=		nErrorCode;

	m_pSocketThread->Sendto( (const uint8_t*)&Head, Head.nSize, m_sockAddrRemote );
	return;
}

void CUdpSession::sendFin( )
{
	RUDP_INFO( "<CUdpSession::sendFin > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId  );

	session_head_t Head;
	Head.nCmd=			SESSION_CMD_FIN;
	Head.nSize=			sizeof(session_head_t);
	Head.nReserve=		RUDP_PROTOCOL_RESERVE;
	Head.nDestSessionId=	m_nDestSessionId;

	m_pSocketThread->Sendto( (const uint8_t*)&Head, Head.nSize, m_sockAddrRemote );

	return;
}

void CUdpSession::sendFin2( )
{
	RUDP_INFO( "<CUdpSession::sendFin2 > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId  );

	session_head_t Head;
	Head.nCmd=			SESSION_CMD_FIN2;
	Head.nSize=			sizeof(session_head_t);
	Head.nReserve=		RUDP_PROTOCOL_RESERVE;
	Head.nDestSessionId=	m_nDestSessionId;

	m_pSocketThread->Sendto( (const uint8_t*)&Head, Head.nSize, m_sockAddrRemote );
	return;
}

void CUdpSession::sendKeepalive( )
{
	RUDP_DEBUG( "<CUdpSession::sendKeepalive > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId );

	CDataNode DataNode( THREAD_INDEX_FROM_SESSIONID( m_nLocalSessionId ) );

	//报文头的数据
	rudp_keepalive_head_t Head;
	Head.nCmd=				SESSION_CMD_KEEPALIVE;
	Head.nSize=				(uint16_t)sizeof(rudp_keepalive_head_t);
	Head.nReserve=			RUDP_PROTOCOL_RESERVE;
	Head.nDestSessionId=	m_nDestSessionId;
	Head.nTickCount=		(uint32_t)m_pSocketThread->get_now();

	m_pSocketThread->Sendto( (const uint8_t*)&Head, Head.nSize, m_sockAddrRemote );

	m_nSendKeepaliveTime= m_pSocketThread->get_now();
}

void CUdpSession::sendKeepaliveAck(uint32_t nTickCount )
{
	RUDP_DEBUG( "<CUdpSession::sendKeepaliveAck > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId );
	//报文头的数据
	rudp_keepalive_head_t Head;

	Head.nCmd=				SESSION_CMD_KEEPALIVE_ACK;
	Head.nSize=				(uint16_t)sizeof(rudp_keepalive_head_t);		//带上发送的时间
	Head.nReserve=			RUDP_PROTOCOL_RESERVE;
	Head.nDestSessionId=	m_nDestSessionId;
	Head.nTickCount=		nTickCount;
	

	//发送数据
	m_pSocketThread->Sendto( (const uint8_t*)&Head, Head.nSize, m_sockAddrRemote );
}

void CUdpSession::setStatus(SESSION_STATUS Status )
{
	if( m_SessionStatus == Status )
	{
		return;
	}
	if( SESSION_STATUS_CLOSE == m_SessionStatus )
	{
		return;
	}

	RUDP_INFO( "<CUdpSession::setStatus > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "status = " << Status );

	switch( Status )
	{
		case SESSION_STATUS_FIN:
			{
			}
			break;

		case SESSION_STATUS_CONNECTING:
			{
			}
			break;

		case SESSION_STATUS_FIN2:
			{
			}
			break;

		case SESSION_STATUS_CLOSE:
			{
				printLog();
			}
			break;

		case SESSION_STATUS_NONE:
			{

			}
			break;
	}

	m_nRetryTime=		m_pSocketThread->get_now();
	m_nStatusTime=		m_nRetryTime;
	m_SessionStatus=	Status ;

}


void CUdpSession::procTimerEvent( uint32_t nFlag )
{
	if( (nFlag & RECV_BUF_FLAG) )
	{
		m_pRecvBuf->procTimerEvent(m_pSocketThread->get_now(), nFlag );
	}
	else
	{
		m_pSendBuf->procTimerEvent(m_pSocketThread->get_now(), nFlag );
	}
}


int CUdpSession::procTimer( )
{
	switch( getStatus() )
	{
		case SESSION_STATUS_NORMAL:
			{
				if( m_pSocketThread->get_now() > _rudp_config::SESSION_TIMEOUT_TIME + m_nRecvKeepaliveTime )
				{
					RUDP_WARNING( "<CUdpSession::Timer > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "SESSION_STATUS_NORMAL Timeout" );
					Disconnect();
				}
				else 
				{
					if( m_pSocketThread->get_now() > m_nSendKeepaliveTime + m_nCfgKeepaliveTime )
					{
						sendKeepalive( );
					}
				}
			}
			break;
		case SESSION_STATUS_CONNECTING:
			{
				//检查是否超时
				if( m_pSocketThread->get_now() > g_RudpCfgInfo.nConnectionTimeout + m_nStatusTime )
				{
					RUDP_WARNING( "<CUdpSession::Timer > L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "SESSION_STATUS_CONNECTING Timeout" );
					Disconnect();
					m_pSessionCtrl->getCallback()->on_connection( m_nLocalSessionId, m_wParam, m_lParam, RUDP_ERROR_TIMEOUT );
				}
				else if( m_pSocketThread->get_now() > _rudp_config::SESSION_RESEND_TIME + m_nRetryTime )
				{
					sendSyn( );
					m_nRetryTime = m_pSocketThread->get_now();
				}
			}
			break;

		case SESSION_STATUS_FIN:
			{
				if( m_pSocketThread->get_now() > _rudp_config::SESSION_CLOSE_TIME + m_nStatusTime)
				{
					setStatus( SESSION_STATUS_CLOSE );
				}
				else if( m_pSocketThread->get_now() > _rudp_config::SESSION_RESEND_TIME + m_nRetryTime )
				{
					sendFin( );
					m_nRetryTime= m_pSocketThread->get_now();
				}
			}
			break;
		case SESSION_STATUS_FIN2:
			{
				if( m_pSocketThread->get_now() > _rudp_config::SESSION_CLOSE_TIME + m_nStatusTime)
				{
					setStatus( SESSION_STATUS_CLOSE );
				}
				else if( m_pSocketThread->get_now() > _rudp_config::SESSION_RESEND_TIME + m_nRetryTime )
				{
					sendFin2( );
					m_nRetryTime= m_pSocketThread->get_now();
				}
			}
			break;

		case  SESSION_STATUS_NONE:
			{
				if( m_pSocketThread->get_now() > _rudp_config::SESSION_NONE_TIME + m_nCreateTime )
				{
					setStatus( SESSION_STATUS_CLOSE );
				}
			}
			break;

		case SESSION_STATUS_CLOSE:
			{ 
				return -1;
			}
			break;
	}
	return 0;
}

void CUdpSession::printLog()
{
	if( m_pSendBuf->getSendCount() > 1000 )
	{
		RUDP_INFO( "<CUdpSession::printLog> L = " << m_nLocalSessionId << "D = " << m_nDestSessionId << "dest_ip = " << m_sockAddrRemote );
		RUDP_INFO( "<CUdpSession::printLog SendBuf> L = " << m_nLocalSessionId << "D = " << m_nDestSessionId 
						<< " packet = " <<  m_pSendBuf->getPacketCount() 
						<< " send = " << m_pSendBuf->getSendCount() 
						<< " re_send = " << m_pSendBuf->getReSendCount() 
						<< " urgent_send = " << m_pSendBuf->getUrgentCount() 
						<< " recv_ack_count = " << m_pSendBuf->getRecvAckCount() 
						<< " recv_ack2_count = " << m_pSendBuf->getRecvAck2Count() 
						<< " recv_repair_count = " << m_pSendBuf->getRecvRepairCount() 
						<< " send_syn_count = " << m_pSendBuf->getSendSynCount() 
						<< " WindowSize = " << m_RateCtrl.getWindowSize() 
						<< " Ping = " <<  m_RateCtrl.getPing() )


		RUDP_INFO( "<CUdpSession::printLog RecvBuf> L = " << m_nLocalSessionId << "D = " << m_nDestSessionId 
						<< " packet = " << m_pRecvBuf->getPacketCount() 
						<< " recv = " << m_pRecvBuf->getRecvCount() 
						<< " re_recv = " << m_pRecvBuf->getReRecvCount() 
						<< " send_ack_count = " << m_pRecvBuf->getSendAckCount() 
						<< " send_ack2_count = " << m_pRecvBuf->getSendAck2Count() 
						<< " send_repair_count = " << m_pRecvBuf->getSendRepairCount() 
						<< " recv_syn_count = " << m_pRecvBuf->getRecvSynCount()
						<< " Variation = " << m_RateCtrl.getVariation() 
						<< " Rtc = " << m_RateCtrl.getRtc() 
						<< " Rtt = " << m_RateCtrl.getRtt()  )
	}
}
