#include "session_ctrl.h"
#include "udp_session.h"
#include "socket_thread.h"
#include "rudp_buf.h"
#include "rudp_log_macro.h"
rudp_config_t		g_RudpCfgInfo;


CSessionCtrl::CSessionCtrl( rudp_callback* pCallback)
{
	memset( m_pSocketThread, 0, sizeof(m_pSocketThread) );
	m_pCallback=	pCallback;
}

CSessionCtrl::~CSessionCtrl(void)
{
	releaseService( 1000 );
}

UDPSESSION CSessionCtrl::create_session( Inet_Addr* pAddrFrom )
{
	mutex_.acquire();
	int nThreadIndex= mallocThread( pAddrFrom );
	mutex_.release();
	if( nThreadIndex >= 0 )
	{
		return m_pSocketThread[nThreadIndex]->createUdpSession( );
	}
	return INVALID_UDPSESSION;
}

int	CSessionCtrl::send_udp( const uint8_t* pData, int nDataSize, const Inet_Addr* pAddrTo, const Inet_Addr* pAddrFrom )
{
	mutex_.acquire();
	int nIndex= mallocThread( pAddrFrom );
	mutex_.release();

	if( nIndex >= 0 )
	{
		m_pSocketThread[nIndex]->sendUdpData( pData, nDataSize, pAddrTo );
		return 0;
	}
	return -1;
}


int CSessionCtrl::connect( UDPSESSION Session, Inet_Addr& sockDest)
{
	uint32_t nIndex= THREAD_INDEX_FROM_UDPSESSION(Session);
	if( nIndex >= 0 )
	{
		return m_pSocketThread[nIndex]->connect( Session, sockDest );
	}
	return -1;
}

void	CSessionCtrl::close_session( UDPSESSION Session )
{
	uint32_t nIndex= THREAD_INDEX_FROM_UDPSESSION(Session);
	if( nIndex >= 0 )
	{
		return m_pSocketThread[nIndex]->closeUdpSession( Session );
	}
	return;
}

int	CSessionCtrl::get_stat( UDPSESSION Session )
{
	uint32_t nIndex= THREAD_INDEX_FROM_UDPSESSION(Session);
	if( nIndex >= 0 )
	{
		return m_pSocketThread[nIndex]->getSessionStat( Session );
	}
	return -1;
}

int		CSessionCtrl::set_callback_param( UDPSESSION Session, uint64_t wParam, uint64_t lParam )
{
	uint32_t nIndex= THREAD_INDEX_FROM_UDPSESSION(Session);
	if( nIndex >= 0 )
	{
		return m_pSocketThread[nIndex]->setCallbackParam( Session, wParam, lParam );
	}
	return -1;
}

int		CSessionCtrl::set_session_param( UDPSESSION Session, uint32_t nParamType, void* pData, int nSize)
{
	uint32_t nIndex= THREAD_INDEX_FROM_UDPSESSION(Session);
	if( nIndex >= 0 )
	{
		return m_pSocketThread[nIndex]->setSessionParam( Session, nParamType, pData, nSize );
	}
	return -1;
}

int		CSessionCtrl::get_session_param( UDPSESSION Session, uint32_t nParamType, void* pBuf, int& nBufSize)
{
	uint32_t nIndex= THREAD_INDEX_FROM_UDPSESSION(Session);
	if( nIndex >= 0 )
	{
		return m_pSocketThread[nIndex]->getSessionParam( Session, nParamType, pBuf, nBufSize );
	}
	return -1;
}


int CSessionCtrl::send_rudp( UDPSESSION Session, const uint8_t* pData, int nDataSize, uint32_t callback_id )
{
	uint32_t nIndex= THREAD_INDEX_FROM_UDPSESSION(Session);
	if( nIndex >= 0 )
	{
		if( g_RudpCfgInfo.nSendDirectlyFlag )
			return m_pSocketThread[nIndex]->sendDataDirect( Session, pData, nDataSize, callback_id );
		else
			return m_pSocketThread[nIndex]->sendData( Session, pData, nDataSize, callback_id );
	}
	return -1;
}

int	CSessionCtrl::get_local_addr( UDPSESSION Session, Inet_Addr& sockAddr )
{
	uint32_t nIndex= THREAD_INDEX_FROM_UDPSESSION(Session);
	if( nIndex >= 0 )
	{
		return m_pSocketThread[nIndex]->getLocalAddr( Session, sockAddr );
	}
	return -1;
}

int	CSessionCtrl::get_remote_addr( UDPSESSION Session, Inet_Addr& sockAddr )
{
	uint32_t nIndex= THREAD_INDEX_FROM_UDPSESSION(Session);
	if( nIndex >= 0 )
	{
		return m_pSocketThread[nIndex]->getRemoteAddr( Session, sockAddr );
	}
	return -1;
}

int CSessionCtrl::set_config_info( RUDP_CONFIG_PARAM ConfigType, void* pData, int nSize )
{
	switch(ConfigType )
	{
		case RUDP_CONFIG_LOG_LEVEL:
			{
				if( nSize == 4 )
				{
					memcpy( &g_RudpCfgInfo.nLogLevel, pData, 4); 
					SET_RUDP_LOG_LEVEL( g_RudpCfgInfo.nLogLevel );
					return 0;
				};
			}
			break;

		case RUDP_CONFIG_OUTPUT_LOG:
			{
				if( nSize == 4 )
				{
					memcpy( &g_RudpCfgInfo.nOutputLogFlag, pData, 4); 
					return 0;
				};
			}
			break;

		case RUDP_CONFIG_SEND_DIRECYLY:
			{
				if( nSize == 4 )
				{
					memcpy( &g_RudpCfgInfo.nSendDirectlyFlag, pData, 4); 
					return 0;
				};
			}
			break;

		case RUDP_CONFIG_PRINT_TIME:
			{
				if( nSize == 4 )
				{
					int nTime=0;
					memcpy( &nTime, pData, 4); 
					if( nTime > 100 )
					{
						g_RudpCfgInfo.nPrintLogTime= nTime;
					}
					return 0;
				};
			}
			break;

		case RUDP_CONFIG_CONNECT_TIMEOUT:
			{
				if( nSize == 4 )
				{
					int nTime=0;
					memcpy( &nTime, pData, 4); 
					if( nTime > 3000 )
					{
						g_RudpCfgInfo.nConnectionTimeout= nTime;
					}
					return 0;
				};
			}
			break;

		default:
			{

			}
			break;
	}
	return -1;
}


///////////////////////////////////////////////////////////////////////////


int CSessionCtrl::initService(Inet_Addr sockAddr[], uint16_t nAddrCount, uint32_t nMaxSessionCount )
{	
	WSADATA wsaData;
	if (  WSAStartup(0x0202, &wsaData) != 0)
	{
		return -1;
	}
	setLocalAddr( sockAddr, nAddrCount );

	if( createSocketThread( nMaxSessionCount ) != 0 )
	{
		return -1;
	}
	return 0;
}

void CSessionCtrl::releaseService( uint32_t nTimeout )
{
	mutex_.acquire();
	releaseSocketThread();
	mutex_.release();
}

void CSessionCtrl::setLocalAddr( Inet_Addr sockAddr[] , uint16_t nAddrCount )
{
	uint16_t i=0;
	for( i=0; i< nAddrCount && i< MAX_UDPPORT_COUNT; i++ )
	{
		memcpy( &m_LocalAddr[i], &sockAddr[i], sizeof(Inet_Addr) );
	}
	m_nAddrCount= i;
}

void CSessionCtrl::releaseSocketThread()
{
	//关闭收发线程
	for( int i=0; i< m_nAddrCount; i++ )
	{
		if( m_pSocketThread[i] )
		{
			m_pSocketThread[i]->releaseSocket();
			delete m_pSocketThread[i];
			m_pSocketThread[i] = NULL;
		}
	}
}

//这个地方需要把PORT 分配到所有的线程上面
int CSessionCtrl::createSocketThread( uint32_t nSessionCount )
{
	//启动收发线程
	for( uint16_t i=0; i< m_nAddrCount; i++ )
	{
		m_pSocketThread[i]= new CSocketThread( this, m_LocalAddr[i] );
		if( m_pSocketThread[i]->initSocket(  i, nSessionCount ) )
		{
			delete m_pSocketThread[i];
			m_pSocketThread[i]= NULL;
			return -1;
		}
	}
	
	return 0;

}

int	CSessionCtrl::mallocThread( const Inet_Addr* pAddr )
{
	if( pAddr != NULL )
	{
		for( int i=0; i< m_nAddrCount; i++ )
		{
			if( m_pSocketThread[i] != NULL && m_pSocketThread[i]->getIpAddr() == pAddr->get_ip() && m_pSocketThread[i]->getPort() == pAddr->get_port() )
			{
				return i;
			}
		}
		return -1;
	}
	else
	{
		static uint32_t nThreadIndex= 0;
		nThreadIndex++;
		return (nThreadIndex % m_nAddrCount );
	}
	return 0;
}
