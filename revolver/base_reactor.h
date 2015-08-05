/*************************************************************************************
*filename:	base_rector.h
*
*to do:		���巴Ӧ���ӿ�
*Create on: 2012-05
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __BASE_REACTOR_H
#define __BASE_REACTOR_H

#include "base_namespace.h"
#include "base_typedef.h"
#include "timer_queue_t.h"
#include "object_pool.h"
#include "base_thread_mutex.h"

#include <map>
#include <vector>
using namespace std;

BASE_NAMESPACE_BEGIN_DECL

class CEventHandler;

typedef CTimerQueue_T<CEventHandler*, CTimerFunctor, BaseThreadMutex> TIMEQUEUE;

typedef struct ReactorEventHandlerInfo
{
	CEventHandler*	event_handler;
	int32_t			event_mask;
	bool			event_close_;

	ReactorEventHandlerInfo()
	{
		event_handler = NULL;
		event_mask = 0;
		event_close_ = true;
	};
}ReactorEventHandlerInfo;

typedef map<BASE_HANDLER, ReactorEventHandlerInfo*> ReactorEventHandlerMap;

typedef ObjectPool<ReactorEventHandlerInfo, HANDLER_POOL_SIZE> ReactorEventHandlerInfoPool;

class IMessageProcessor
{
public:
	//ɨ���ڲ�����
	virtual int32_t	processor() = 0;
};

class CReactor
{
public:
	CReactor(){msg_proc_ = NULL;};
	virtual ~CReactor(){};
	
	void	set_message_processor(IMessageProcessor* proc){msg_proc_ = proc;};

	virtual int32_t open_reactor(uint32_t number_of_handlers) = 0;
	virtual int32_t close_reactor() = 0;

	virtual int32_t event_loop() = 0;
#if defined(WIN32)
	virtual int32_t noblock_loop() = 0;
#endif
	virtual int32_t stop_event_loop() = 0;

	virtual	void print() {};

	//�¼�����
	//���һ���¼��ļ���
	virtual int32_t register_handler(CEventHandler *handler, uint32_t masks) = 0;
	//ɾ��һ���¼����ض�����
	virtual int32_t remove_handler(CEventHandler *handler, uint32_t masks) = 0;
	//ɾ��һ���¼�
	virtual int32_t delete_handler(CEventHandler *handler, bool del_event_obj = false) = 0;
	//��ʱ������ 
	//���һ����ʱ��
	virtual uint32_t set_timer(CEventHandler *event_handler, const void *act, uint32_t delay) = 0;
	//ɾ��һ����ʱ��
	virtual uint32_t cancel_timer(uint32_t timer_id, const void **act) = 0;

protected:
	IMessageProcessor* msg_proc_;
};

BASE_NAMESPACE_END_DECL
#endif

/*************************************************************************************/
