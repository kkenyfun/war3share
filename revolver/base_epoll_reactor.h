/*************************************************************************************
*filename:	base_epoll_reactor.h
*
*to do:		����LINUXϵͳ�µ�EPOLL��Ӧ��
*Create on: 2012-05
*Author:	zerok
*check list: EPOLL REACTOR����������������HANDLER�Ĳ����ɾ������ͨ���ڲ�����Ϣ����������
			 ���Խ�ֹ���̲߳���EPOLL ��Ӧ�������ɾ��������	
*************************************************************************************/
#ifndef __BASE_EPOLL_REACTOR_H
#define __BASE_EPOLL_REACTOR_H

#include "base_namespace.h"
#include "base_reactor.h"
#include "base_timer_value.h"

#include <set>

#ifndef WIN32
#include <sys/epoll.h>
#endif

BASE_NAMESPACE_BEGIN_DECL

#ifndef WIN32
#define EPOLL_MAX_HANDLER		32768
#define EPOLL_HEAP_SIZE			65536	//64K 

typedef set<uint32_t>			ReactorIndexSet;

class CEventHandler;
class CEpollReactor : public CReactor
{
public:
	CEpollReactor();
	virtual ~CEpollReactor();

	int32_t open_reactor(uint32_t number_of_handlers);
	int32_t close_reactor();

	int32_t event_loop();
	int32_t stop_event_loop();

	int32_t register_handler(CEventHandler *handler, uint32_t masks);
	int32_t remove_handler(CEventHandler *handler, uint32_t masks);
	int32_t delete_handler(CEventHandler *handler, bool del_event_obj = false);

	uint32_t set_timer(CEventHandler *event_handler, const void *act, uint32_t delay);
	uint32_t cancel_timer(uint32_t timer_id, const void **act);

	void print();

private:
	void	check_connection(int32_t rc, CEventHandler* handler);
	ReactorEventHandlerInfo* find_handler_info(CEventHandler* handler);

private:
	ReactorEventHandlerInfoPool handler_pool_;
	
	ReactorEventHandlerInfo*	handler_heap_[EPOLL_HEAP_SIZE];

	ReactorIndexSet				free_set_;

	TIMEQUEUE					timer_queue_;
	CTimerFunctor				functor_;

	uint32_t					epoll_delay_;
	uint32_t					max_handler_num_;

	BASE_HANDLER				epfd_;
	epoll_event*				events_;
	int32_t						nevent_;		//������EVENT����

	uint64_t					prev_ts_;
};

#endif

BASE_NAMESPACE_END_DECL
#endif

/************************************************************************************/
