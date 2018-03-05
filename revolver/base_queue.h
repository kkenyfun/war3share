/*************************************************************************************
*filename:	base_queue.h
*
*to do:		定义队列类
*Create on: 2012-05
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __BASE_QUEUE_H
#define __BASE_QUEUE_H

#include "base_namespace.h"
#include "base_typedef.h"
#include "base_guard.h"

BASE_NAMESPACE_BEGIN_DECL

template<class T, class BASE_LOCK, int32_t CAPACITY>
class BaseQueue_T
{
public:
	BaseQueue_T()
	{
		data_ = new T[CAPACITY];
		rindex_ = 0;
		windex_ = 0;
	};

	~BaseQueue_T()
	{
		delete []data_;
	};

	bool put(const T& element)
	{

		BASE_GUARD_RETURN(BASE_LOCK, cf_mon, lock_, false);

		//队列满
		if(rindex_ == 0 && windex_ + 1 == CAPACITY)
		{
			return false;
		}

		//队列满
		if(windex_ + 1 == rindex_)
		{
			return false;
		}

		data_[windex_] = element;
		if(windex_ + 1 < CAPACITY)
		{
			windex_ ++;
		}
		else
		{
			windex_ = 0;
		}

		return true;
	};

	bool get(T& element)
	{
		BASE_GUARD_RETURN(BASE_LOCK, cf_mon, lock_, false);

		if(rindex_ == windex_)
			return false;

		element = data_[rindex_];
		if(rindex_ + 1 < CAPACITY)
		{
			rindex_ ++;
		}
		else
		{
			rindex_ = 0;
		}

		return true;
	};

	int32_t size()
	{	
		return (windex_ >= rindex_) ? (windex_ - rindex_) : (windex_ + CAPACITY - rindex_);
	};

private:
	T*					data_;
	BASE_LOCK	lock_;
	//直接消除锁的问题，提高消息队列性能
	int32_t	rindex_;
	int32_t	windex_;
};

BASE_NAMESPACE_END_DECL

#endif
/************************************************************************************/
