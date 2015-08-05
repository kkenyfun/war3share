/*************************************************************************************
*filename:	base_timer_value.h
*
*to do:		����UNIXʱ����
*Create on: 2012-05
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __BASE_TIMER_VALUE_H
#define __BASE_TIMER_VALUE_H

#include <time.h>
#include "base_namespace.h"
#include "base_typedef.h"
#include "base_os.h"
#include <assert.h>

#ifdef WIN32
#include <windows.h>
#endif

#include <iostream>

BASE_NAMESPACE_BEGIN_DECL

//100��
#define ONE_SECOND_IN_USECS 1000000

class CBaseTimeValue
{
public:
	CBaseTimeValue();
	CBaseTimeValue(time_t s, time_t us = 0);
	CBaseTimeValue(const struct timeval& tv);

	~CBaseTimeValue();

	static CBaseTimeValue get_time_value();
	
	void set(time_t s, time_t us = 0);
	void set(const struct timeval &tv);

	uint64_t	msec() const;
	uint64_t	sec() const;
	uint32_t	usec() const;
	uint64_t	to_usec()const;

	void		set_msec(uint64_t ms);
	void		set_sec(uint64_t s);

	void		get(struct timeval& tv) const;

	CBaseTimeValue& operator =(const CBaseTimeValue& time_value);
	CBaseTimeValue& operator =(const struct timeval& tv);

	CBaseTimeValue& operator +=(const CBaseTimeValue& time_value);
	CBaseTimeValue& operator -=(const CBaseTimeValue& time_value);

	CBaseTimeValue& operator ++(void);
	CBaseTimeValue& operator --(void);

	friend bool				operator ==(const CBaseTimeValue& time_value1, const CBaseTimeValue& time_value2);
	friend bool				operator !=(const CBaseTimeValue& time_value1, const CBaseTimeValue& time_value2);
	friend bool				operator <(const CBaseTimeValue& time_value1, const CBaseTimeValue& time_value2);
	friend bool				operator >=(const CBaseTimeValue& time_value1, const CBaseTimeValue& time_value2);
	friend bool				operator <=(const CBaseTimeValue& time_value1, const CBaseTimeValue& time_value2);
	friend bool				operator >(const CBaseTimeValue& time_value1, const CBaseTimeValue& time_value2);

	friend CBaseTimeValue	operator +(const CBaseTimeValue& t1, const CBaseTimeValue& t2)
	{
		uint64_t us = t1.to_usec() + t2.to_usec();
		CBaseTimeValue timer(us / ONE_SECOND_IN_USECS, us % ONE_SECOND_IN_USECS);
		return timer;
	}

	friend CBaseTimeValue	operator -(const CBaseTimeValue& t1, const CBaseTimeValue& t2)
	{
		{
			uint64_t us = 0;
			if(t1.to_usec() > t2.to_usec())
			{
				us = t1.to_usec() - t2.to_usec();
			}

			CBaseTimeValue timer(us / ONE_SECOND_IN_USECS, us % ONE_SECOND_IN_USECS);
			return timer;
		}
	}

	static uint64_t now_sec()
	{
		return now_msec() / 1000;
	}

	static uint64_t now_msec()
	{
#if defined WIN32
		return ::GetTickCount();
#else
		struct timespec tv;
		int rc = clock_gettime(CLOCK_MONOTONIC, &tv);
		if (rc != 0)
		{
			struct timeval tv;
			int rc = gettimeofday(&tv, NULL);
			assert(rc == 0); (unsigned)rc;
			return (tv.tv_sec * (uint64_t)1000 + tv.tv_usec / 1000);
		}
		return (tv.tv_sec * (uint64_t)1000 + tv.tv_nsec / 1000000);
#endif
	}

private:
	struct timeval	tv_;
};

BASE_NAMESPACE_END_DECL
#endif
/*************************************************************************************/

