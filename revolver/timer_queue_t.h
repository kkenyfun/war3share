/*************************************************************************************
*filename:	timer_queue_t.h
*
*to do:		������ת��ʱ����ʵ��һ��4�̶ֿ�ת���Ķ�ʱ�������У���ȷ����С��λΪ����
*Create on: 2012-04
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __TIMER_QUEUE_T_H
#define __TIMER_QUEUE_T_H

#include "timer_ring.h"
#include "object_pool.h"
#include "timer_node_t.h"
#include "base_timer_value.h"
#include "base_event_handler.h"

#include <list>
#include <assert.h>

BASE_NAMESPACE_BEGIN_DECL
#define RINGS_SIZE	4

typedef std::list<uint32_t>	FreeTimerIDList;

template<class HANDLER, class FUNCTOR, class LOCK>
class CTimerQueue_T : public IRingEvent
{
public:
	typedef vector<BaseTimerNode_T<HANDLER>*>		TimerNodeArray;

	CTimerQueue_T(FUNCTOR* functor, size_t heap_size = TIMER_POOL_SIZE);
	~CTimerQueue_T();

	//����һ����ʱ��
	uint32_t schedule(HANDLER handler, const void* act, uint32_t delay, uint32_t interval = 0);
	//ȡ��һ����ʱ��
	void cancel_timer(uint32_t timer_id, const void **act);
	//����һ����ʱ��
	uint32_t reset_timer(uint32_t timer_id, uint32_t delay, uint32_t interval = 0);

	//ɨ�賬ʱ��������һ����ʱ��������СΪ10MS�����Ϊ50MS
	virtual uint32_t expire();

	//ԭ�Ӵ�������
	void ring_event(uint8_t ring_id, uint32_t timer_id);

	//�ж϶�ʱ�����Ƿ���
	bool full() const;

	FUNCTOR &upcall_functor (void);

#if _DEBUG
	//���ڴ����ٽ�㣬0,255,255,255
	void set_ring_id();
#endif

protected:
	void dispatch_info(BaseTimerDispathInfo_T<HANDLER>& info, uint32_t timer_id);
	//��ȡһ�����е�Timer node���
	uint32_t get_free_node();

	void clear();

	void alloc_heap(size_t size);

	void insert_node(BaseTimerNode_T<HANDLER>* node);
	void delete_node(BaseTimerNode_T<HANDLER>* node);

	uint32_t revolver(uint32_t scale); 
protected:
	ObjectPool<BaseTimerNode_T<HANDLER>, TIMER_POOL_SIZE>	node_pool_;
	
	//��ʱ����
	TimerNodeArray			heap_;
	size_t					heap_size_;
	//�Ѿ�ռ�õ�HEAP����
	uint32_t				used_num_;
	uint32_t				cur_heap_pos_;
	FreeTimerIDList			freeTimers_;

	//������
	CTimerRing				rings_[RINGS_SIZE];	//0��ʾ��λ�֣�3��ʾ��λ��,һ���ֱ�ʾ256���̶ȣ�4����������32λ����������

	//��ʼʱ��
	CBaseTimeValue			start_time_;	
	//��һ��expire�����ʱ��
	CBaseTimeValue			prev_time_;

	LOCK					mutex_;
	FUNCTOR*				functor_;
};

//����functor
class CTimerFunctor
{
public:

	CTimerFunctor()
	{

	}

	~CTimerFunctor()
	{

	}

	void registration(CEventHandler* handler, uint32_t id)
	{
		handler->add_timer_event(id);
	}

	void cancel_timer(CEventHandler *handler, uint32_t id)
	{
		handler->del_timer_event(id);
	}

	void timer_out(CEventHandler *event_handler,
		const void *act, 
		int32_t recurring_timer, 
		uint32_t timer_id)
	{
		event_handler->del_timer_event(timer_id);
		event_handler->handle_timeout(act, timer_id);
	}
};
BASE_NAMESPACE_END_DECL

#include "timer_queue_t.inl"

#endif
/*************************************************************************************/
