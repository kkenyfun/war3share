#pragma once

#include "base_recvbuf.h"
#include "base_sendbuf.h"

#include "EasyBuffer.h"
#include "rudp_global.h"

using namespace BASE_NAMEPSACE_DECL;

enum
{
	RECV_BUF_FLAG=			0x80000000,

	SENDBUF_SYN_EVENT=		0x1,

	RECVBUF_REPAIRD_EVENT=	(RECV_BUF_FLAG + 1) ,
};

//#include "EasyBuffer.h"


static const uint16_t DEFAULT_WINDOW_SIZE	=100;
static const uint16_t DEFAULT_PING			=50;			//����
static const uint16_t DEFAULT_SYN_TIME		=120;			//MS
class CRateCtrl
{
public:
	CRateCtrl()
	{
		m_nPing				= DEFAULT_PING;
		m_nVariation		=0;
		m_nWindowSize		=DEFAULT_WINDOW_SIZE;
		m_nWindowAutoFlag	=1;
	}
	~CRateCtrl(){}

	inline uint16_t	getRtt()
	{
		return m_nPing + getRtc();
	};
	inline uint16_t	getRtc( )
	{
		return (uint16_t)( m_nVariation * 1.5 + 25 );
	}
	inline uint16_t	getVariation(){ return m_nVariation; };
	inline uint16_t	getSynTime( )
	{
		return m_nPing + (uint16_t)(m_nVariation * 1.5 + 155) ;
	};


	inline	void	setPing( uint16_t nPing )
	{ 
		//��һ��pingֵ
		if( nPing < 10 )
		{
			nPing= 10;
		}
		else if( nPing < 1000 )
		{
		}
		else if( nPing < 3000 )
		{
			nPing=  1000;
		}
		else
		{
			return;
		}

		m_nVariation = (uint16_t)(m_nVariation * 0.5 + abs(nPing - m_nPing ) * 0.5);

		m_nPing= (uint16_t)(m_nPing * 0.5 + nPing * 0.5);
		int nWindowSize= 200 * m_nPing / 1000;
		autoWindowSize( nWindowSize );
	}
	inline uint32_t	getPing(){return m_nPing; };

	inline uint32_t get_repair_count()
	{
		uint16_t nPing= getPing();
		/*if( nPing > 500 )
		{
			return	80;
		}
		else*/ if( nPing > 200 )
		{
			return 40;
		}
		else if( nPing > 100 )
		{
			return 20;
		}
		else
		{

		}
		return 10 ;
	};

	inline void		autoWindowSize( uint16_t nSize )
	{
		if( m_nWindowAutoFlag == 0 )
		{
			return;
		}
		if( nSize > 128 )
		{
			nSize= 128;
		}
		if( nSize < 16 )
		{
			nSize= 16;
		}
		m_nWindowSize= nSize;
		return;
	}
	inline uint16_t	getWindowSize(){return m_nWindowSize; };
	inline void		setWindowSize( uint32_t size )
	{
		if( size > 100 )
		{
			m_nWindowSize= size;
			m_nWindowAutoFlag = 0;
		}
	}

private:
	uint16_t		m_nPing;			//
	uint16_t		m_nVariation;		
	uint16_t		m_nWindowSize;		//���ڴ�С
	uint8_t			m_nWindowAutoFlag;
};

class CRudpPushRecvBuf : public CBaseRecvBuf
{
public:
	CRudpPushRecvBuf( CBufCallback* pCallback, CRateCtrl& RateCtrl ) 
		: CBaseRecvBuf( pCallback ),  m_RateCtrl(RateCtrl), recv_buffer(sz_temp_, sizeof( sz_temp_) )
	{
		m_nFirstSeqId=		0;
		m_nMaxSeqId=		0;
		dest_send_max_id_=	0;
		last_ack_seq_id_=	0;
		m_nReckNewTime=		m_pCallback->time_now();
		m_nSendRepairTime=	m_pCallback->time_now();
		m_pHead=			NULL;
	};

	~CRudpPushRecvBuf()
	{
		CRecvDataNode* pNode= m_pHead;
		while( pNode != NULL )
		{
			m_pHead= pNode->m_pNext;
			delete pNode;
			pNode= m_pHead;
		}
		return;
	};

	//�������0����ô����ɹ��� 1����ʾ�����ݿ��Է��أ� -1: ��ʾ��������ʧ��
	virtual int		addRecvData( uint32_t nSeqId, const uint8_t* pData, int nSize, bool packet_flag  );
	virtual void	procTimerEvent(uint64_t time_now,  uint32_t nFlag );
	virtual uint32_t	getAckId()
	{
		last_ack_seq_id_=	m_nFirstSeqId;
		return m_nFirstSeqId; 
	};
	virtual void	onDataSyn(uint32_t nSynId );

#ifdef _RECV_BUF_MEMPOOL_
	inline void* operator new(size_t nSize)
	{
		return m_RudpRecvBufPool.pop_obj( );
	};
	inline void operator delete(void* p)
	{
		m_RudpRecvBufPool.push_obj( (CRudpPushRecvBuf*)p );
	};
	static ObjectMutexPool_malloc<CRudpPushRecvBuf, BaseThreadMutex, 256>	m_RudpRecvBufPool;
#endif


private:
	void	sendAckInfo( );
	void	send_ack2_info( uint32_t ack_id );
	void	sendRepairInfo( uint32_t nSeqId );
	void	procRepairData( );
	int		addRecvNode(CRecvDataNode* pNode );			//�����ݰ���ӵ�������
	bool	chkReRecv(uint32_t nSeqId );					//�ж��Ƿ��յ������ظ�������
	void	callbackRecvData( );						//�ж��Ƿ���������Ҫ�ص���ȥ
	void	procSeqData( uint32_t nSeqId, const uint8_t* pData, int nSize, bool packet_flag );

private:
	uint32_t			m_nFirstSeqId;			//�Ѿ����յ�������������ID
	uint32_t			m_nMaxSeqId;			//�Ѿ����յ�������һ��ID
	uint32_t			dest_send_max_id_;		//�Է��Ѿ������˵�����SEQID
	uint32_t			last_ack_seq_id_;		//�ϴ�ȷ���˵�����SEQID
	TIMER_INT			m_nReckNewTime;			//���ȷ���˵�ʱ��
	TIMER_INT			m_nSendRepairTime;		//����Ͳ�����ʱ�䡣
	
	CEasyBuffer			recv_buffer;
	CRateCtrl&			m_RateCtrl;
	CRecvDataNode*		m_pHead;
	uint8_t				sz_temp_[RUDP_PLAYLOAD_SIZE];
};




class CRudpPushSendBuf : public CBaseSendBuf
{
public:
	CRudpPushSendBuf( CBufCallback* pCallback, CRateCtrl& RateCtrl )	
		: CBaseSendBuf( pCallback ), m_RateCtrl(RateCtrl)
	{
		m_nBufSeqId			= 1;
		m_pHead				= NULL;
		m_pTail				= NULL;
		last_send_data_time_= m_pCallback->time_now();
		timerflag_			= false;
		m_nMaxSendSeqId		= 0;

		m_nWaitSize			= 0;
		m_nSendingSize		= 0;
		m_nSendingCount		= 0;

		m_send_urgent_flag	= false;
	};
	~CRudpPushSendBuf()
	{
		CSendDataNode* p= NULL;
		while( m_pHead )
		{
			p= m_pHead->m_pNext;
			delete m_pHead;
			m_pHead= p;
		}
		return;
	};

#ifdef _SEND_BUF_MEM_POOL_
	inline void* operator new(size_t nSize)
	{
		return m_RudpSendBufPool.pop_obj( );
	};
	inline void operator delete(void* p)
	{
		m_RudpSendBufPool.push_obj((CRudpPushSendBuf*) p );
	};
	static ObjectMutexPool_malloc<CRudpPushSendBuf, BaseThreadMutex, 256>	m_RudpSendBufPool;
#endif


	virtual void set_urgent_flag( bool flag ){m_send_urgent_flag = flag; };
	virtual int		addSendNode( CSendDataNode* pNode );
	virtual void	onDataAck( uint32_t nAckId );
	virtual void	onDataAck2( uint32_t nAckId );
	virtual void	onDataRepair( uint32_t nRepairId );

	virtual uint32_t	getWaitSize();
	virtual uint32_t	getBuffingSize(); 
	

	virtual void	procTimerEvent(uint64_t time_now,  uint32_t nEventId );

private:
	void	procSynEvent( uint64_t time_now );
	void	sendNode( CSendDataNode* pNode );
	void	sendNodeAll( );
	void	reg_timer( uint64_t time_now );
	inline bool	canSend( ){ return m_nSendingCount < m_RateCtrl.getWindowSize(); };
	inline CSendDataNode* getNode(uint32_t seq_id)
	{
		CSendDataNode* p= m_pHead;
		while( p )
		{
			if( p->m_nSeqId == seq_id )
			{
				break;
			}
			p= p->m_pNext;
		}
		return p;
	};

private:
	uint32_t			m_nBufSeqId;		//���ݻ���������SEQID
	uint32_t			m_nMaxSendSeqId;	//�Ѿ������˵����İ������к�
	int32_t				m_nWaitSize;		//���ж������ݻ�û�з���
	int32_t				m_nSendingSize;		//���ж���������·��
	int32_t				m_nSendingCount;	//���ж��ٸ�����·��

	TIMER_INT			last_send_data_time_;		//���ҵ��㷢�����ݵ�ʱ��
	bool				timerflag_;
	CRateCtrl&			m_RateCtrl;
	CSendDataNode*		m_pHead;
	CSendDataNode*		m_pTail;
	bool				m_send_urgent_flag;			//�Ƿ�����UDP���ӷ����ֽ�������
};

