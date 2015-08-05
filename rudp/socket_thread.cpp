#include "data_node.h"
#include "socket_thread.h"
#include "session_ctrl.h"
#include "rudp_log_macro.h"

CSocketThread::CSocketThread( CSessionCtrl * pService, Inet_Addr& sockAddr )
{
	m_nMaxIndex=			0;
	m_pSession=				NULL;
	m_nMaxSessionCount=		0;
	memcpy( &m_sockAddr,	&sockAddr, sizeof(Inet_Addr) );
	m_pSessionCtrl=			pService;

	m_nTimeLog=				MyGetTickCount();
	m_nTimeChk=				MyGetTickCount();
}

CSocketThread::~CSocketThread()
{
	releaseSocket();
}

void CSocketThread::releaseSocket()
{
	terminate( );
	releaseSession();
}


int CSocketThread::initSocket( uint16_t nIndex, uint32_t nMaxSessionCount )
{

	int32_t ret = m_sUdp.open(m_sockAddr, true, false);
	if(ret != 0)
	{
		return -1;
	}

	//设置缓冲区大小
	int32_t buf_size = 20 * 1024 * 1024; 
	m_sUdp.set_option(SOL_SOCKET, SO_RCVBUF, (void *)&buf_size, sizeof(int32_t));
	m_sUdp.set_option(SOL_SOCKET, SO_SNDBUF, (void *)&buf_size, sizeof(int32_t));

	m_nIndex= nIndex;
	createSessionBuf( nMaxSessionCount );
	return start( );
}

void CSocketThread::execute( )
{
	int		nErrorCode=0;
	CDataNode*	pRecvData= new CDataNode( m_nIndex );
	int			nCount =0;
	memset(	&m_stStaticInfo, 0, sizeof(m_stStaticInfo ) );


//	set_priority( HIGH_PRIORITY_CLASS );
	bool sleepflag= false;
	m_TimerManager.initRudpTimerManager();
	m_nTime_Now = MyGetTickCount();
	while(!get_terminated())
	{
		//处理所有的收数据的操作；
		for( int i=0; i< 2000; i++ )
		{
			pRecvData->m_nDataSize=	m_sUdp.recv( pRecvData->m_szDataBuf, RUDP_BUF_SIZE, pRecvData->m_sockAddr );
			if( pRecvData->m_nDataSize <= 0 )
			{
				break;
			}
			else
			{
				m_stStaticInfo.nRecvCount++;
				mutex_.acquire();
				procUdpMsg( pRecvData );
				mutex_.release();
			}
			nCount++;
		}
		//处理所有业务请求操作；

		sleepflag = Update( );

#ifdef WIN32
		timeBeginPeriod(1);
#endif
		if( true == sleepflag )
		{
			msSleep(1);
		}
		m_nTime_Now=	::MyGetTickCount();
#ifdef WIN32
		timeEndPeriod(1);
#endif
		nCount=0;
	}
	delete pRecvData;

	clear_thread();
	return;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
UDPSESSION CSocketThread::createUdpSession( )
{
	CUdpSession* pSession= new CUdpSession( m_pSessionCtrl, m_pSessionCtrl->getCallback(), this );
	if( pSession == NULL )
	{
		return INVALID_UDPSESSION;
	}	

	mutex_.acquire();
	UDPSESSION Session= INVALID_UDPSESSION;
	if( addUdpSession( pSession ) != 0 )
	{
		delete pSession;
	}
	else
	{
		Session= pSession->getSessionNode();
	}
	mutex_.release();
	return Session;
}


int	CSocketThread::connect(UDPSESSION Session, Inet_Addr& sockDest)
{
	int nRet= -1;
	mutex_.acquire();
	CUdpSession* pUdpSession= getUdpSesionfromSessionNode( Session );
	if( pUdpSession != NULL )
	{
		pUdpSession->Connect( sockDest );
		nRet = 0;
	}
	mutex_.release();
	return nRet;
}

void CSocketThread::closeUdpSession( UDPSESSION Session )
{
	mutex_.acquire();
	CUdpSession* pUdpSession= getUdpSesionfromSessionNode( Session );
	if( pUdpSession != NULL )
	{
		RUDP_INFO( "<CUdpSession::closeUdpSession  > L = " << pUdpSession->getLocalSessionId() << "D = " << pUdpSession->getDestSessionId()  );
		pUdpSession->Disconnect();
	}
	mutex_.release( );
	return;
}

void CSocketThread::sendUdpData( const uint8_t* pData, int nDataSize, const Inet_Addr* pAddrTo )
{
	mutex_.acquire();
	m_sUdp.send( pData, nDataSize, *pAddrTo );
	mutex_.release();
}

int	CSocketThread::getSessionStat( UDPSESSION Session )
{
	int nRet= 0;
	mutex_.acquire();
	CUdpSession* pUdpSession= getUdpSesionfromSessionNode( Session );
	if( pUdpSession != NULL )
	{
		nRet= pUdpSession->getStatus( );
	}
	mutex_.release();
	return nRet;
}

int	CSocketThread::setCallbackParam( UDPSESSION Session, uint64_t wParam, uint64_t lParam )
{
	int nRet= -1;
	mutex_.acquire();
	CUdpSession* pUdpSession= getUdpSesionfromSessionNode( Session );
	if( pUdpSession != NULL )
	{
		pUdpSession->setCallbackParam( wParam, lParam );
		nRet= 0;
	}
	mutex_.release();
	return nRet;
}

int	CSocketThread::setSessionParam( UDPSESSION Session, uint32_t nParamType, void* pData, int nSize)
{
	int nRet= -1;
	mutex_.acquire();
	CUdpSession* pSession= getUdpSesionfromSessionNode( Session );
	if( pSession )
	{
		nRet= pSession->setSessionParam( nParamType, pData, nSize );
	}
	mutex_.release();
	return nRet;
}

int	CSocketThread::getSessionParam( UDPSESSION Session, uint32_t nParamType, void* pBuf, int& nBufSize)
{
	int nRet= -1;
	mutex_.acquire();
	CUdpSession* pSession= getUdpSesionfromSessionNode( Session );
	if( pSession )
	{
		nRet= pSession->getSessionParam( nParamType, pBuf, nBufSize );
	}
	mutex_.release();
	return nRet;
}

int CSocketThread::sendData( UDPSESSION Session, const uint8_t* pData, int nDataSize, uint32_t callback_id )
{
	if( nDataSize > RUDP_PLAYLOAD_SIZE )
	{
		RUDP_DEBUG( "<CUdpSession::sendData  > size error Session = " << Session << "DataSize = " << nDataSize );
		return -1;
	}

	if( g_RudpCfgInfo.nLogLevel >= RUDP_LOG_ALL )
	{
		RUDP_DEBUG( "<CUdpSession::sendData  > Session = " << Session << "DataSize = " << nDataSize );
	}
	return sendDataDirect(Session, pData, nDataSize, callback_id );
}


int	CSocketThread::sendDataDirect( UDPSESSION Session, const uint8_t* pData, int nDataSize, uint32_t callback_id )
{
	int nRet= -1;
	mutex_.acquire( );
	CUdpSession* pSession= getUdpSesionfromSessionNode( Session );
	if( pSession != NULL )
	{
		m_stStaticInfo.nSendDataCount++;
		nRet= pSession->sendDataDirect( pData, nDataSize, callback_id );
	}
	mutex_.release();
	return nRet;
}


int	CSocketThread::getLocalAddr( UDPSESSION Session, Inet_Addr& sockAddr )
{
	int nRet= -1;
	mutex_.acquire();
	sockAddr = m_sockAddr;
	mutex_.release();
	return nRet;
}

int	CSocketThread::getRemoteAddr( UDPSESSION Session, Inet_Addr& sockAddr )
{
	int nRet= -1;
	mutex_.acquire();
	CUdpSession* pUdpSession= getUdpSesionfromSessionNode( Session );
	if( pUdpSession != NULL )
	{
		memcpy( &sockAddr, &pUdpSession->getRemoteAddr(), sizeof(Inet_Addr) );
		nRet= 0;
	}
	mutex_.release();
	return nRet;
}


void CSocketThread::Sendto( const uint8_t* pData, int nSize, Inet_Addr& sockAddr)
{
	m_sUdp.send( pData, nSize, sockAddr );
	m_stStaticInfo.nSendCount++;
}

void CSocketThread::msSleep( uint32_t nMs)
{
	//fd_set fdRead;
	//FD_ZERO( &fdRead );
	//timeval tv = {0};
	//tv.tv_usec= 1000*nMs;

	//FD_SET( m_sUdp.get_handler(), &fdRead );
	//::select( 0, &fdRead, NULL, NULL, &tv );

	usleep(nMs * 1000 );

	return;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CSocketThread::Update( )
{
	bool	bSleepFlag= true;
	CRudpTimer*	pTimer= NULL;

	uint32_t nCount=0;

	while( true )
	{
		pTimer= m_TimerManager.getCurrent();

		if( pTimer->getTimerTick() > m_nTime_Now )
		{
			break;
		}
		nCount= pTimer->getEventCount( );
		if( nCount > 0 )
		{
			for( uint32_t i=0; i< nCount; i++ )
			{
				TimerEvent&	Event= pTimer->getEvent( i );
				CUdpSession* pSession= m_pSession[ SESSION_INDEX_FROM_SESSSIONID( Event.nLocalSessionId) ];
				if( pSession && pSession->getLocalSessionId() == Event.nLocalSessionId && pSession->getStatus() == SESSION_STATUS_NORMAL  ) 
				{

					//RUDP_DEBUG( "procTimerEvent now =" << time_now << "time = " << pTimer->getTimerTick() << "session = " << Event.nLocalSessionId << "flag = " << Event.nFlag );
					mutex_.acquire();
					pSession->procTimerEvent( Event.nFlag );
					mutex_.release();
				};
			}
			bSleepFlag= false;
		}
		m_TimerManager.goNext();
	}

	if(  m_nTime_Now > m_nTimeChk + _rudp_config::SESSION_UPDATE_TIME )
	{
		mutex_.acquire();
		chkUpdate( );
		mutex_.release();
	
		m_nTimeChk= m_nTime_Now;
		bSleepFlag= false;
	}

	if(m_nIndex == 0 && m_nTime_Now > (uint64_t)(m_nTimeLog + (uint64_t)g_RudpCfgInfo.nPrintLogTime) )
	{
	//	printLogInfo();

		m_nTimeLog= m_nTime_Now;
		bSleepFlag= false;
	}
	return bSleepFlag;
}

//定时器，检查各个连接的状态
void CSocketThread::chkUpdate( )
{	
	uint32_t nMax= m_nMaxIndex;
	m_nMaxIndex= 0;

	m_stStaticInfo.nConnectionCount=0;
	for( uint32_t i=0; i< nMax; i++ )
	{
		//检查关闭
		if( m_pSession[i] == NULL )
		{
			continue;
		}
		if( m_pSession[i]->procTimer( ) != 0 )
		{
			delete m_pSession[i];
			m_pSession[i]= NULL;
		}
		else
		{
			m_stStaticInfo.nConnectionCount++;
			m_nMaxIndex= i;
			m_nMaxIndex++;
		}
	}
	return;
}

void CSocketThread::procUdpMsg( CDataNode* pData )
{
	session_head_t* pHead= (session_head_t*)pData->m_szDataBuf;
	if( pHead->nSize < sizeof(session_head_t ) || pHead->nSize > pData->m_nDataSize )
	{
		m_pSessionCtrl->getCallback()->on_udp_recv( pData->m_szDataBuf, pData->m_nDataSize, pData->m_sockAddr, &m_sockAddr );
		return;
	}
	if( pHead->nReserve != RUDP_PROTOCOL_RESERVE )
	{
		m_pSessionCtrl->getCallback()->on_udp_recv( pData->m_szDataBuf, pData->m_nDataSize, pData->m_sockAddr, &m_sockAddr );
		return;
	}

	switch( pHead->nCmd )
	{
			//处理客户端连接（服务方）
		case SESSION_CMD_SYN:
			{
				procSynMsg( pData->m_nIndex, (rudp_syn_t*) pHead, pData->m_szDataBuf + sizeof(rudp_syn_t), pData->m_nDataSize - sizeof(rudp_syn_t), pData->m_sockAddr );
			}
			break;

		default:
			{
				//表示请求的链接已经没有了
				CUdpSession* pSession= getUdpSessionfromSessionId( pHead->nDestSessionId );
				if( pSession != NULL )
				{
					pSession->procUdpMsg( pData ); 
				}
				else
				{
					m_pSessionCtrl->getCallback()->on_udp_recv( pData->m_szDataBuf, pData->m_nDataSize, pData->m_sockAddr, &m_sockAddr );
				}
			}
	}
	return ;
}

void CSocketThread::procSynMsg( uint32_t nThreadIndex, rudp_syn_t* pMsgHead, uint8_t* pData, uint32_t nSize, Inet_Addr& sockAddr )
{
	CUdpSession* pSession=NULL;

	//获取解析连接的时候的数据
	if( pMsgHead->nDestSessionId == 0 ){return;	}

	RUDP_INFO( "<CUdpSession::procSynMsg> syn L = " << pMsgHead->nDestSessionId << "D = " << pMsgHead->nLocalSessionId << "remote = " << sockAddr );

	//如果存在,表示这个连接已经存在,那个包已经收到过了;
	if( (pSession= chkSynSession( pMsgHead->nDestSessionId, pMsgHead->nLocalSessionId ) ) != NULL )
	{
		RUDP_WARNING( "<CUdpSession::procSynMsg> chkSynSession error L = " << pMsgHead->nDestSessionId << "D = " << pMsgHead->nLocalSessionId << "remote = " << sockAddr  );
		pSession->sendSyn2( 0 );
		return;
	}

	pSession= new CUdpSession( m_pSessionCtrl, m_pSessionCtrl->getCallback(), this );
	if( pSession == NULL )
	{
		RUDP_ERROR( "<CUdpSession::procSynMsg> create connection error L = " << pMsgHead->nDestSessionId << "D = " << pMsgHead->nLocalSessionId );
		return;
	}
	pSession->setDestSessionId( pMsgHead->nLocalSessionId );
	pSession->setRand( pMsgHead->nDestSessionId );
	if( addUdpSession( pSession ) != 0 )
	{
		RUDP_ERROR( "<CUdpSession::procSynMsg> add connection error L = " << pSession->getLocalSessionId() << "D = " << pSession->getDestSessionId()  );
	
		pSession->sendSyn2( ERROR_SYSTEM );
		delete pSession;
		return;
	}

	pSession->onAccept( pMsgHead, sockAddr );
	return;
}

int CSocketThread::addUdpSession( CUdpSession* pUdpSession )
{
	uint32_t nRand= (uint32_t)( rand() * rand() * m_nTime_Now );
	for( uint32_t i=0; i< m_nMaxIndex; i++ )
	{
		if( m_pSession[i] == NULL )
		{
			m_pSession[i]= pUdpSession;

			pUdpSession->setLocalSessionId( SESSIONID_FROM_THREAD_INDEX(m_nIndex, nRand, i) );
			return 0;
		}
	}
	if( m_nMaxIndex < m_nMaxSessionCount )
	{
		m_pSession[m_nMaxIndex]= pUdpSession;
		pUdpSession->setLocalSessionId( SESSIONID_FROM_THREAD_INDEX(m_nIndex, nRand, m_nMaxIndex) );
		m_nMaxIndex++;
		return 0;
	}
	return -1;
}

CUdpSession* CSocketThread::getUdpSessionfromSessionId( uint32_t nLocalSessionId, uint32_t nDestSessionId/* =0 */ )
{
	uint32_t nIndex= SESSION_INDEX_FROM_SESSSIONID( nLocalSessionId );
	if( nIndex >= m_nMaxSessionCount )
	{
		return NULL;
	}
	if( m_pSession[nIndex] == NULL )
	{
		return NULL;
	}
	if( m_pSession[nIndex]->getLocalSessionId() != nLocalSessionId  )
	{
		return NULL;
	}
	if( (nDestSessionId != 0) && m_pSession[nIndex]->getDestSessionId() != nDestSessionId )
	{
		return NULL;
	}
	return m_pSession[nIndex];
}
CUdpSession*	CSocketThread::getUdpSesionfromSessionNode( UDPSESSION Socket)
{
	uint32_t nIndex= SESSION_INDEX_FROM_UDPSESSION( Socket );
	if( nIndex >= m_nMaxSessionCount )
	{
		return NULL;
	}
	if( m_pSession[nIndex] == NULL )
	{
		return NULL;
	}
	if( m_pSession[nIndex]->getSessionNode() != Socket  )
	{
		return NULL;
	}
	return m_pSession[nIndex];
}

CUdpSession* CSocketThread::chkSynSession( uint32_t nRand, uint32_t nDestSessionId )
{
	for( uint32_t i=0; i< m_nMaxIndex; i++ )
	{
		if( m_pSession[i] != NULL )
		{
			if( m_pSession[i]->getRand() == nRand && m_pSession[i]->getDestSessionId() == nDestSessionId )
			{
				return m_pSession[i];
			}
		}
	}
	return NULL;
}


int CSocketThread::createSessionBuf( uint32_t nMaxSessionCount )
{
	m_nMaxSessionCount= nMaxSessionCount;
	m_pSession= new CUdpSession*[m_nMaxSessionCount];
	if( m_pSession == NULL )
	{
		return -1;
	}
	memset( m_pSession, 0, sizeof(CUdpSession*)*m_nMaxSessionCount );
	return 0;
}


void CSocketThread::releaseSession()
{
	if( m_pSession == NULL )
	{
		return;
	}
	//先关闭所有的链接
	for( uint32_t i= 0; i< m_nMaxIndex; i++ )
	{
		if( m_pSession[i] != NULL )
		{
			delete m_pSession[i];
			m_pSession[i]= NULL;
		}
	}
	delete[] m_pSession;
	m_pSession= NULL;
}

void CSocketThread::printLogInfo()
{
	if( m_nIndex == 0 )
	{
		RUDP_INFO( "CSocketThread::printLogInfo>> thead_id = " << m_nIndex << "connection_count = " << m_stStaticInfo.nConnectionCount << "AcceptCount" << m_stStaticInfo.nAcceptCount
				<< "DisconnectCount = " << m_stStaticInfo.nDisConnectionCount << "RecvCount = " << m_stStaticInfo.nRecvCount << "Send = " << m_stStaticInfo.nSendCount 
				<< "SendDataCount = " << m_stStaticInfo.nSendDataCount << "RecvDataCount = " << m_stStaticInfo.nRecvDataCount ); 
	}

#if 0
	if( m_nIndex == 0 )
	{
		sprintf(szLogInfo, 	"\n\tCUdpSession=		[%d]"
							"\n\tCSendDataNode=		[%d]"
							"\n\tCRecvDataNode=		[%d]"
							"\n\tCRudpPushRecvBuf=	[%d]"
							"\n\tCRudpPushSendBuf=	[%d]\n",

							CUdpSession::m_UdpSessionPool.get_pop_number(),
							CSendDataNode::m_SendDataNodePool.get_pop_number(),
							CRecvDataNode::m_RecvDataNodePool.get_pop_number(),
							CRudpPushRecvBuf::m_RudpRecvBufPool.get_pop_number(),
							CRudpPushSendBuf::m_RudpSendBufPool.get_pop_number() );
		m_pSessionCtrl->rudpLog( szLogInfo, __LINE__, RUDP_LOG_NONE);
	}
#endif
	//for( uint32_t i=0; i< m_nMaxIndex;i++ )
	//{
	//	if( m_pSession[i] != NULL )
	//	{
	//		m_pSession[i]->printLog();
	//	}
	//}
}
