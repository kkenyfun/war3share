
#pragma once
#include "rudp_timer.h"


using namespace BASE_NAMEPSACE_DECL;


#define	RUDP_PROTOCOL_VERSION	10
#define RUDP_PROTOCOL_RESERVE	0xFE

#define THREAD_INDEX_FROM_SESSIONID( nId )					( (nId >> 28) & 0xF)
#define SESSION_INDEX_FROM_SESSSIONID(nId)					( nId & 0xFFFF )
#define SESSIONID_FROM_THREAD_INDEX( nId, nRand, nIndex)	( (nId << 28) + (nRand & 0x0FFF0000) + nIndex )

#define THREAD_INDEX_FROM_UDPSESSION( Session )				( (Session >> 60) & 0xF)
#define SESSION_INDEX_FROM_UDPSESSION( Session )			( (Session >> 32) & 0xFFFF)

//���ӱ���Э��
#define SESSION_CMD_SYN				0x10		//������������
#define SESSION_CMD_SYN2			0x11		//�������ӷ��ذ�
#define SESSION_CMD_FIN				0x12		//��������ر�
#define SESSION_CMD_FIN2			0x13		//�رշ��ذ�
#define SESSION_CMD_KEEPALIVE		0x14		//������
#define SESSION_CMD_KEEPALIVE_ACK	0x15		//�������ذ�

//����Э��
#define SESSION_CMD_RUDP_DATA		0x20		//�ɿ�����
#define SESSION_CMD_RUDP_PACKET		0x21		//�յ����������ݰ�
#define SESSION_CMD_RUDP_ACK		0x23		//�ɿ�����ȷ��
#define SESSION_CMD_RUDP_SYN		0x24		//���ͻ�������ȷ��
#define SESSION_CMD_RUDP_REPAIR		0x25		//��������
#define SESSION_CMD_RUDP_ACK_2		0x27		//ȷ�����ݣ����ǰ�������ݰ���û�е�����������ȷ������


////////////////////////////
//SESSION ����:  �� 8λ OxFF ����� INDEX;
//SYN:	�����Լ���Session, Dest Session ΪRAND��;
//SYN2: �����Լ���Session, Dest Session Ϊ�յ���SessionId;
//
typedef struct _session_head
{
	uint8_t	nReserve;			//����Ĵ�С
	uint8_t	nCmd;				//����
	uint16_t	nSize;				//��С

	uint32_t	nDestSessionId;		//���ӱ��			//Ŀ���SESSIONID
}session_head_t;

typedef struct _rudp_syn : public _session_head
{
	uint8_t	nVersion;
	uint8_t	nErrorCode;
	uint16_t	nGwPort;
	uint32_t	nGwIp;
	uint32_t	nLocalSessionId;
}rudp_syn_t;

typedef struct _rudp_ack_head : public _session_head
{
	uint32_t		nAckSeqId;
}rudp_ack_head_t;

typedef struct _rudp_repair_head : public _session_head
{
	uint32_t		nRepairSeqId;
}rudp_repair_head_t;


typedef struct _rudp_syn_head : public _session_head
{
	uint32_t		nSynSeqId;
}rudp_syn_head_t;


typedef struct _rudp_data_head : public _rudp_ack_head
{
	uint32_t		nSeqId;
}rudp_data_head_t;

typedef struct _rudp_keepalive_head : public _session_head
{
	uint32_t		nTickCount;
}rudp_keepalive_head_t;

class CSendDataNode;


class CBufCallback
{
public:
	virtual void onAckData( uint32_t seq_id )=0;
	virtual void onAck2Data( uint32_t seq_id )=0;

	virtual void onRepairData( uint32_t seq_id )=0;
	virtual void onSynData( uint32_t seq_id )=0;
	virtual void onRudpDataList( CSendDataNode* node[], int count )=0;		
	virtual void onRudpData( uint32_t seq_id, uint8_t* pData, int nSize, bool packet_flag ) =0;

	virtual void onRecvData( const uint8_t* pData, int nSize )=0;
	virtual void onSendData( uint32_t callback_id )=0;
	virtual void onRegisterTimer( TIMER_INT nTime, uint32_t nFlag )=0;
	virtual TIMER_INT time_now()=0;
};

