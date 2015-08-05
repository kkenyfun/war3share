#pragma once


#include "base_inet_addr.h"
#include "base_buf.h"
#include "object_pool_2.h"
#include "base_thread_mutex.h"

using namespace BASE_NAMEPSACE_DECL;

#define SEND_BUF_OFFSET 	sizeof(rudp_data_head_t)


class CSendDataNode
{
	static const uint32_t DEFAULT_TIMEOUT=		3000;
public:
	CSendDataNode( uint64_t nSessionNode, const uint8_t* pData, uint16_t nDataSize, uint32_t nTimeout= DEFAULT_TIMEOUT )
	{
		memcpy( m_szBuf + SEND_BUF_OFFSET, pData, nDataSize );
		m_nSessionNode=		nSessionNode;
		m_nSeqId=			0;
		m_nDataSize=		nDataSize + SEND_BUF_OFFSET;
		m_nSendCount=		0;
		packet_flag_=		false;
		recv_flag_	=		false;
		m_nCallbackId=		0;
		last_send_time_=	0;
		m_pNext=			NULL;
	};
	~CSendDataNode(){};
#ifdef _SEND_DATA_MEMPOOL_
	inline void* operator new(size_t nSize)
	{
		return m_SendDataNodePool.pop_obj( );
	};
	inline void operator delete(void* p)
	{
		m_SendDataNodePool.push_obj( (CSendDataNode*)p );
	};
	static ObjectMutexPool_malloc<CSendDataNode, BaseThreadMutex, 256>	m_SendDataNodePool;
#endif

public:
	uint64_t			m_nSessionNode;
	uint32_t			m_nSeqId;
	uint8_t				m_szBuf[RUDP_BUF_SIZE];
	uint16_t			m_nDataSize;
	uint16_t			m_nSendCount;
	uint32_t			m_nCallbackId;
	uint64_t			last_send_time_;			//�����ʱ��
	bool				packet_flag_;				//�Ƿ���һ�������� 1:��ʾ����		
	bool				recv_flag_;					//��ʾ�Է��Ѿ��յ���Щ���� 1 �Ѿ��յ�
	CSendDataNode*		m_pNext;
};


class CBaseSendBuf
{
public:
	CBaseSendBuf( CBufCallback* pCallback )
	{
		m_nPacketCount		= 0;
		m_nSendCount		= 0;
		m_nReSendCount		= 0;
		m_nSendSize			= 0;
		m_nUrgentCount		= 0;
		m_nSendSynCount		= 0;

		m_nRecvAckCount		= 0;
		m_nRecvAck2Count	= 0;
		m_nRecvRepairCount	= 0;
		m_pCallback			= pCallback;
	};
	virtual ~CBaseSendBuf(){}
	
	//0:��ʾ�ɹ�������ʧ��

	virtual void	set_urgent_flag( bool flag )=0;
	virtual int		addSendNode( CSendDataNode* pNode )=0;
	virtual void	onDataAck( uint32_t nAckId )= 0;
	virtual void	onDataAck2(uint32_t nAckId )= 0;
	virtual void	onDataRepair( uint32_t nRepairId )=0;
	virtual uint32_t	getWaitSize()=0;
	virtual uint32_t	getBuffingSize()= 0; 

	virtual void	procTimerEvent(uint64_t time_now,  uint32_t nEvent )=0;
	
	inline uint32_t	getPacketCount( ){ return m_nPacketCount; };
	inline uint32_t	getReSendCount(){return m_nReSendCount;};
	inline uint32_t	getSendCount(){return m_nSendCount;};
	inline uint64_t	getSendSize(){return m_nSendSize; };
	inline uint32_t getUrgentCount(){return m_nUrgentCount; };


	inline uint32_t	getRecvAckCount(){return m_nRecvAckCount;};
	inline uint32_t	getRecvAck2Count(){return m_nRecvAck2Count;};

	inline uint32_t	getRecvRepairCount(){return m_nRecvRepairCount; };
	inline uint32_t	getSendSynCount(){return m_nSendSynCount; };

protected:
	uint32_t	m_nSendCount;		//���͵�������
	uint32_t	m_nReSendCount;		//�ط��ĸ���
	uint64_t	m_nSendSize;
	uint32_t	m_nUrgentCount;
	uint32_t	m_nPacketCount;		//ҵ��㷢�Ͱ��ĸ���
	uint32_t	m_nRecvAckCount;	//�յ�ACK�ĸ���
	uint32_t	m_nRecvAck2Count;	//�յ�ACK�ĸ���
	uint32_t	m_nRecvRepairCount;	//�յ������ĸ���
	uint32_t	m_nSendSynCount;	//����SYN���ĸ���


	CBufCallback*	m_pCallback;
};

