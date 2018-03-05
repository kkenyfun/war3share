#include "rudp_buf.h"
#include "rudp_timer.h"
#include "rudp_log_macro.h"

#ifdef _SEND_DATA_MEMPOOL_
ObjectMutexPool_malloc<CSendDataNode, BaseThreadMutex, 256>	CSendDataNode::m_SendDataNodePool;
#endif

#ifdef _RECV_DATA_MEMPOOL_
ObjectMutexPool_malloc<CRecvDataNode, BaseThreadMutex, 256>	CRecvDataNode::m_RecvDataNodePool;
#endif
#ifdef _RECV_BUF_MEMPOOL_
ObjectMutexPool_malloc<CRudpPushRecvBuf, BaseThreadMutex, 256>	CRudpPushRecvBuf::m_RudpRecvBufPool;
#endif
#ifdef _SEND_BUF_MEMPOOL_
ObjectMutexPool_malloc<CRudpPushSendBuf, BaseThreadMutex, 256>	CRudpPushSendBuf::m_RudpSendBufPool;
#endif


//如果返回0，那么加入成功， 1：表示有数据可以返回； -1: 表示插入数据失败
int	CRudpPushRecvBuf::addRecvData( uint32_t nSeqId, const uint8_t* pData, int nSize, bool packet_flag )
{
	int nRet = 0;

	if( nSeqId > m_nFirstSeqId + 512 )	{ return -1; }
	//测试
	//if( m_pCallback->time_now() % 50 == 0 )
	//{
	//	return -1;
	//}

	m_nRecvCount++;
	m_nRecvSize+= nSize;

	if( nSeqId <= m_nFirstSeqId )
	{
		m_nReRecvSize+= nSize;
		m_nReRecvCount++;
		return 0;
	}

	if( nSeqId > m_nMaxSeqId )
	{	
		m_nMaxSeqId =		nSeqId;
		m_nReckNewTime=		m_pCallback->time_now();
	}

	if( nSeqId == m_nFirstSeqId + 1 )
	{
		procSeqData( nSeqId, pData, nSize, packet_flag );
		callbackRecvData( );
	}
	else
	{
		if( chkReRecv( nSeqId ) )
		{
			m_nReRecvCount++;
		}
		else
		{
			CRecvDataNode* p= new CRecvDataNode( pData, nSize, nSeqId, packet_flag );
			addRecvNode( p );

			procRepairData( );
			send_ack2_info(nSeqId);
		}
	}
	if( m_nFirstSeqId > last_ack_seq_id_ + 5 ){ sendAckInfo(); }
	return 0;
}


void CRudpPushRecvBuf::onDataSyn(uint32_t nSynId )
{
	dest_send_max_id_= nSynId;
	m_nRecvSynCount++;

	//还没有收到确认的报文，已经确认将对方所有的报文都收到了
	if( dest_send_max_id_ == m_nFirstSeqId )
	{
		sendAckInfo();
	}
	else
	{
		procRepairData( );
	}
};


void CRudpPushRecvBuf::procRepairData( )
{
	if( m_pCallback->time_now() < m_nSendRepairTime + m_RateCtrl.getRtt() ){ return; }

	uint32_t MAX_REPAIR_COUNT = m_RateCtrl.get_repair_count();
	uint32_t nRepairCount=0;

	//这种情况下，一般是后面有包没有收到
	if( m_pHead == NULL )
	{
		for( uint32_t i=1; i + m_nFirstSeqId <= dest_send_max_id_ && nRepairCount < MAX_REPAIR_COUNT; i++ )
		{
			sendRepairInfo(m_nFirstSeqId + i);
			nRepairCount++;
		}
	}
	else
	{
		uint32_t nSeq=		m_nFirstSeqId+ 1;
		//这种情况下，中间应该有漏了的包
		CRecvDataNode* p=	m_pHead;
		while( p && nRepairCount< MAX_REPAIR_COUNT)
		{
			while( nSeq < p->m_nSeqId && nRepairCount < MAX_REPAIR_COUNT )
			{
				sendRepairInfo( nSeq );
				nSeq++;
				nRepairCount++;
			}
			nSeq++;
			p= p->m_pNext;
		}
	}

	if( nRepairCount > 0 )
	{
		m_nSendRepairTime= m_pCallback->time_now();
	}
	return;
}


void CRudpPushRecvBuf::procTimerEvent( uint64_t time_now, uint32_t nFlag )
{
	return;
}

//发送ACK报文
void CRudpPushRecvBuf::sendAckInfo( )
{
	m_pCallback->onAckData( m_nFirstSeqId );
	m_nSendAckCount++;
	last_ack_seq_id_= m_nFirstSeqId;
	return;
}

void CRudpPushRecvBuf::send_ack2_info( uint32_t ack_id )
{
	m_pCallback->onAck2Data( ack_id );
	m_nSendAck2Count++;
}


//发送补包的报文
void CRudpPushRecvBuf::sendRepairInfo( uint32_t nSeqId )
{
	m_pCallback->onRepairData( nSeqId );
	m_nSendRepairCount++;
}

//将数据包添加到队列中
int CRudpPushRecvBuf::addRecvNode(CRecvDataNode* pNode )
{
	if( m_pHead== NULL  )
	{
		//队列中没有数据,那么放在头部
		m_pHead= pNode;
	}
	else if( m_pHead->m_nSeqId > pNode->m_nSeqId )
	{
		//队列中头部的ID 小于 刚才接受到的ID, 
		pNode->m_pNext= m_pHead;
		m_pHead= pNode;
	}
	else
	{
		CRecvDataNode* p= m_pHead;
		while( p != NULL )
		{
			if( p->m_pNext == NULL )
			{
				p->m_pNext= pNode;
				break;
			}
			else if( p->m_pNext->m_nSeqId < pNode->m_nSeqId )
			{
				p= p->m_pNext;
			}
			else
			{
				pNode->m_pNext= p->m_pNext;
				p->m_pNext= pNode;
				break;
			}
		}
	}
	return 0;
}

//判断是否收到的是重复的数据
bool CRudpPushRecvBuf::chkReRecv(uint32_t nSeqId )
{
	if( m_nFirstSeqId >= nSeqId )
	{
		return true;
	}

	CRecvDataNode* p= m_pHead;
	while( p != NULL && p->m_nSeqId <= nSeqId)
	{
		if( p->m_nSeqId == nSeqId )
		{
			return true;
		}
		p= p->m_pNext;
	}
	return false;
};

//回调其他的数据
void CRudpPushRecvBuf::callbackRecvData( )
{
	//第一次回调；
	CRecvDataNode* pNode= m_pHead;
	while( pNode != NULL &&  pNode->m_nSeqId == m_nFirstSeqId + 1)
	{
		procSeqData( pNode->m_nSeqId, pNode->m_szDataBuf, pNode->m_nDataSize, pNode->packet_flag_ );
		m_pHead= pNode->m_pNext;
		delete pNode;
		pNode= m_pHead;
	}
	return;
}

//检查是否有连续的数据包
void CRudpPushRecvBuf::procSeqData( uint32_t nSeqId, const uint8_t* pData, int nSize, bool packet_flag )
{
	m_nFirstSeqId = nSeqId;
	m_nPacketCount++;
	if( packet_flag )
	{
		m_nPacketCount++;
		if( recv_buffer.GetUsedSize() == 0 )
		{
			m_pCallback->onRecvData( pData,  nSize );
		}
		else
		{
			recv_buffer.PushBack( pData, nSize );
			m_pCallback->onRecvData( recv_buffer.GetBuffer(), recv_buffer.GetUsedSize() );
			recv_buffer.SetUsedSize( 0 );
		}
	}
	else
	{
		recv_buffer.PushBack( pData, nSize );
	}
	return;
}

/*******************************************************************************************************************************/
void CRudpPushSendBuf::onDataAck( uint32_t nSeqId )
{
	m_nRecvAckCount++;

	//鉴定数据合法性
	if( nSeqId > m_nMaxSendSeqId )
	{
		RUDP_ERROR( " onDataAck Error ACKID = " << nSeqId << " MaxSendId " << m_nMaxSendSeqId );
		return;
	}

	//删除前面的数据
	CSendDataNode* p= NULL;
	while( m_pHead != NULL && m_pHead->m_nSeqId <= nSeqId )
	{
		//如果一个完整的包发送完毕，那么回掉发送成功
		if( m_pHead->packet_flag_ == true )
		{
			m_pCallback->onSendData( m_pHead->m_nCallbackId );
		}
		
		//统计buf中的数据信息
		if( false == m_pHead->recv_flag_ )
		{
			m_nSendingSize-= m_pHead->m_nDataSize;
			m_nSendingCount--;
		}

		p= m_pHead->m_pNext;
		delete m_pHead;
		m_pHead= p;
	}

	//如果数据对方都已经收到了
	if( m_pHead == NULL )
	{	
		m_pTail= NULL;	
	}
	else
	{
		//看看是否还有没有发送的数据
		sendNodeAll();
	}
	return;
}

void CRudpPushSendBuf::onDataAck2( uint32_t nAckId )
{
	m_nRecvAck2Count++;

	//鉴定数据合法性
	if( nAckId > m_nMaxSendSeqId )
	{
		RUDP_ERROR( " onDataAck2 Error ACKID = " << nAckId << " MaxSendId " << m_nMaxSendSeqId );
		return;
	}

	CSendDataNode* p= getNode( nAckId );
	if( p != NULL )
	{
		if( false == p->recv_flag_ )
		{
			m_nSendingSize-= p->m_nDataSize;
			m_nSendingCount--;
			p->recv_flag_ = true;
		}
		//看看是否还有没有发送的数据
		sendNodeAll();
	}
	return;
}


uint32_t CRudpPushSendBuf::getWaitSize()
{
	return m_nWaitSize;
}

uint32_t CRudpPushSendBuf::getBuffingSize()
{
	return m_nWaitSize + m_nSendingSize;
}

void CRudpPushSendBuf::onDataRepair( uint32_t nSeqId )
{
	//统计数据
	m_nRecvRepairCount++;

	if( nSeqId >= m_nBufSeqId )
	{
		m_pCallback->onSynData( m_nBufSeqId - 1);
		return;
	}

	CSendDataNode* p= getNode(nSeqId);
	if( p )
	{
		sendNode( p );
	}
	else
	{
		RUDP_ERROR( "onDataRepair Error SeqId = " << nSeqId << "BufSeqId" << m_nBufSeqId << "HeadId" << (m_pHead ? m_pHead->m_nSeqId : 0) );
	}
	return;
}

//重发所有的
void CRudpPushSendBuf::procTimerEvent(uint64_t time_now,  uint32_t nEvent  )
{
	switch( nEvent )
	{
		case SENDBUF_SYN_EVENT:
			{
				procSynEvent( time_now );
			}
			break;
		default:
			{

			}
			break;
	}
	return;
}

void CRudpPushSendBuf::procSynEvent( uint64_t time_now )
{
	if( m_pHead == NULL )
	{
		timerflag_ = false;
	}
	else
	{
		reg_timer( time_now );
	}
	return;
}

void CRudpPushSendBuf::reg_timer( uint64_t time_now )
{
	//还没有到发送的实际，最后新的数据包的时间 + RTT 后发送SYN, 如果有新的数据包发送后，这个时间要单独调整
	if( last_send_data_time_ + m_RateCtrl.getSynTime() < time_now )
	{
		m_nSendSynCount++; 
		m_pCallback->onSynData( m_nMaxSendSeqId );
	}
	m_pCallback->onRegisterTimer( time_now + m_RateCtrl.getSynTime(), SENDBUF_SYN_EVENT );
	timerflag_ = true;
}


int CRudpPushSendBuf::addSendNode( CSendDataNode* pNode )
{
	pNode->m_nSeqId= m_nBufSeqId++;

	if( m_pHead == NULL )
	{
		m_pHead= pNode;
		m_pTail= pNode;
	}
	else
	{
		m_pTail->m_pNext= pNode;
		m_pTail= pNode;
	}

	m_nPacketCount++;

	m_nWaitSize += pNode->m_nDataSize;

	if( canSend() )
	{
		sendNode( pNode );
	}
	if( false == timerflag_ )
	{
		reg_timer( CBaseTimeValue::get_time_value().msec() );
	}
	return 0;
}

void CRudpPushSendBuf::sendNodeAll( )
{
	//如果数据都发送完了，就不用发送了
	if( m_nMaxSendSeqId == m_nBufSeqId ){return;}

	CSendDataNode* p= m_pHead;
	while( canSend() && p )
	{
		if( p->m_nSendCount == 0 )
		{
			sendNode( p );
		}
		p= p->m_pNext;
	}
	return;
}

void CRudpPushSendBuf::sendNode( CSendDataNode* pNode )
{
	uint64_t now= m_pCallback->time_now();

	if( m_send_urgent_flag  )
	{
		CSendDataNode* data_list[4]={NULL};
		int count = 0;

		data_list[count++]= pNode;
		int data_size= pNode->m_nDataSize;

		CSendDataNode* p= m_pHead;
		while(  p )
		{
			if( count >= 4 )
			{
				break;
			}
			if( p != pNode && p->last_send_time_ + m_RateCtrl.getRtt() < now  && data_size + p->m_nDataSize < RUDP_PACKET_PL_SIZE )
			{
				p->last_send_time_ = now;
				p->m_nSendCount++;
				m_nUrgentCount++;
				data_list[count++]= p;
				data_size+= p->m_nDataSize;
				if( pNode->m_nSeqId > m_nMaxSendSeqId ) { m_nMaxSendSeqId = pNode->m_nSeqId; }
			}
			p= p->m_pNext;
		}
		if( 1 == count )
		{
			m_pCallback->onRudpData( pNode->m_nSeqId, pNode->m_szBuf, pNode->m_nDataSize, pNode->packet_flag_ );
		}
		else
		{
			m_pCallback->onRudpDataList( data_list, count );
		}
	}
	else
	{
		m_pCallback->onRudpData( pNode->m_nSeqId, pNode->m_szBuf, pNode->m_nDataSize, pNode->packet_flag_ );
	}

	//统计数据
	if( pNode->m_nSendCount != 0 ) 
	{	
		m_nReSendCount++;
	}
	else
	{
		m_nWaitSize-= pNode->m_nDataSize;
		m_nSendingSize += pNode->m_nDataSize;
		m_nSendingCount++;

		last_send_data_time_=	m_pCallback->time_now();
	}

	if( pNode->m_nSeqId > m_nMaxSendSeqId )	{ m_nMaxSendSeqId = pNode->m_nSeqId; }

	pNode->m_nSendCount++;
	pNode->last_send_time_ = m_pCallback->time_now();
	m_nSendCount++;
	m_nSendSize+= pNode->m_nDataSize;
	return;
}
