
#pragma once

#include "base_buf.h"
#include "base_inet_addr.h"
#include "object_pool_2.h"
#include "base_thread_mutex.h"

using namespace BASE_NAMEPSACE_DECL;


class CBaseRecvBuf
{
public:
	CBaseRecvBuf( CBufCallback* pCallback )
	{
		m_nPacketCount		=0;
		m_nRecvCount		=0;
		m_nReRecvCount		=0;
		m_nSendAckCount		=0;
		m_nSendRepairCount	=0;
		m_nRecvSynCount		=0;
		m_nSendAck2Count	=0;
		m_nReRecvSize		=0;
		m_nRecvSize			=0;
		m_pCallback			=pCallback;
	};
	virtual ~CBaseRecvBuf(){};

	//如果返回0，那么加入成功， 1：表示有数据可以返回； -1: 表示插入数据失败
	virtual int			addRecvData( uint32_t nSeqId, const uint8_t* pData, int nSize, bool packet_flag  )=0;
	virtual void		procTimerEvent(uint64_t time_now,  uint32_t nEventId )=0;
	virtual uint32_t	getAckId( )=0;
	virtual void		onDataSyn( uint32_t nSynId )=0;

	inline uint32_t		getReRecvCount(){return m_nReRecvCount;};
	inline uint32_t		getRecvCount(){return m_nRecvCount;};
	inline uint32_t		getPacketCount(){return m_nPacketCount;}
	inline uint64_t		getRecvSize(){return m_nRecvSize; };
	inline uint64_t		getReRecvSize( ){return m_nReRecvCount; };
	inline uint32_t		getSendAckCount(){return m_nSendAckCount; };
	inline uint32_t		getSendAck2Count(){return m_nSendAck2Count; };
	inline uint32_t		getSendRepairCount(){return m_nSendRepairCount; };
	inline uint32_t		getRecvSynCount(){return m_nRecvSynCount; }
protected:
	uint32_t				m_nReRecvCount;		//接收到的重发的数据个数
	uint32_t				m_nRecvCount;		//接收了的个数
	uint32_t				m_nPacketCount;		//接收了多少有用的数据
	uint64_t				m_nRecvSize;
	uint64_t				m_nReRecvSize;
	uint32_t				m_nRecvSynCount;	//接收到多少同步包	

	uint32_t				m_nSendAckCount;	//发送了多少ACK的包
	uint32_t				m_nSendAck2Count;	//发送了多少ACK的包
	uint32_t				m_nSendRepairCount;	//发送了多少个补包请求

	CBufCallback*			m_pCallback;		//回调信息
};



class CRecvDataNode
{
public:
	CRecvDataNode( const uint8_t* pData, uint16_t nDataSize, uint32_t nSeqId, bool packet_flag)
	{
		memcpy( m_szDataBuf, pData, nDataSize );
		m_nDataSize=	nDataSize;
		m_nSeqId=		nSeqId;
		packet_flag_=	packet_flag;
		m_pNext=		NULL;
	};
	~CRecvDataNode(){};

#ifdef _RECV_DATE_MEMPOOL_
	inline void* operator new(size_t nSize)
	{
		return m_RecvDataNodePool.pop_obj( );
	};
	inline void operator delete(void* p)
	{
		m_RecvDataNodePool.push_obj( (CRecvDataNode*)p );
	};

	static ObjectMutexPool_malloc<CRecvDataNode, BaseThreadMutex, 256>	m_RecvDataNodePool;
#endif


	uint8_t			m_szDataBuf[RUDP_BUF_SIZE];
	uint32_t		m_nDataSize;
	uint32_t		m_nSeqId;
	bool			packet_flag_;
	CRecvDataNode*	m_pNext;
};