
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

//连接保持协议
#define SESSION_CMD_SYN				0x10		//主动发起连接
#define SESSION_CMD_SYN2			0x11		//发起连接返回包
#define SESSION_CMD_FIN				0x12		//主动发起关闭
#define SESSION_CMD_FIN2			0x13		//关闭返回包
#define SESSION_CMD_KEEPALIVE		0x14		//心跳包
#define SESSION_CMD_KEEPALIVE_ACK	0x15		//心跳返回包

//数据协议
#define SESSION_CMD_RUDP_DATA		0x20		//可靠数据
#define SESSION_CMD_RUDP_PACKET		0x21		//收到完整的数据包
#define SESSION_CMD_RUDP_ACK		0x23		//可靠数据确认
#define SESSION_CMD_RUDP_SYN		0x24		//发送缓冲区的确认
#define SESSION_CMD_RUDP_REPAIR		0x25		//补包请求
#define SESSION_CMD_RUDP_ACK_2		0x27		//确认数据，如果前面有数据包还没有到，则发送这种确认数据


////////////////////////////
//SESSION 定义:  高 8位 OxFF 下面的 INDEX;
//SYN:	带上自己的Session, Dest Session 为RAND数;
//SYN2: 带上自己的Session, Dest Session 为收到的SessionId;
//
typedef struct _session_head
{
	uint8_t	nReserve;			//自身的大小
	uint8_t	nCmd;				//命令
	uint16_t	nSize;				//大小

	uint32_t	nDestSessionId;		//连接编号			//目标端SESSIONID
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

