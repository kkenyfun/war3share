#pragma once
#include "base_timer_value.h"
#include "rudp_log_macro.h"
#include "rudp_global.h"
using namespace BASE_NAMEPSACE_DECL;

struct TimerEvent
{
	uint32_t nLocalSessionId;
	uint32_t nFlag;
};

class CRudpTimer
{
	static const uint32_t MAX_TIMER_EVENT_COUNT		=4096;
public:
	CRudpTimer(void)
	{
		reset();
		m_pNext= NULL;
		m_nTimerTick=0;
	}
	virtual ~CRudpTimer(void){};

	inline void			reset(  ){ m_nCount=0;};
	inline TIMER_INT	getTimerTick(){return m_nTimerTick; };
	inline uint32_t		getEventCount( ){ return m_nCount;	};
	inline TimerEvent&	getEvent( uint32_t nIndex){return m_Event[nIndex]; };

	inline void			registerTimer( uint32_t nLocalSessionId, uint32_t nFlag )
	{
		if( m_nCount <  MAX_TIMER_EVENT_COUNT )
		{
			m_Event[m_nCount].nLocalSessionId=	nLocalSessionId;
			m_Event[m_nCount].nFlag=			nFlag;
			m_nCount++;
		}
		return;
	}
	
private:
	CRudpTimer*	m_pNext;
	TIMER_INT	m_nTimerTick;
	TimerEvent	m_Event[MAX_TIMER_EVENT_COUNT];
	uint32_t		m_nCount;
	friend class CRudpTimerManager;
};


class CRudpTimerManager
{
	static const uint32_t MAX_TIMER_COUNT=	2048;
public:
	CRudpTimerManager()
	{
		m_pCurrent= NULL;
	}
	~CRudpTimerManager(){ }

	void	initRudpTimerManager()
	{
		m_nInitCount=  ::MyGetTickCount( );

		for( uint32_t i=0; i< MAX_TIMER_COUNT-1; i++ )
		{
			m_RudpTimer[i].m_nTimerTick=	m_nInitCount+i;
			m_RudpTimer[i].m_pNext=			&m_RudpTimer[i+1];
		}
		
		m_RudpTimer[MAX_TIMER_COUNT-1].m_nTimerTick=	m_nInitCount + MAX_TIMER_COUNT- 1;
		m_RudpTimer[MAX_TIMER_COUNT-1].m_pNext=			&m_RudpTimer[0];

		m_pCurrent= &m_RudpTimer[0];
	}


	void	registerTimer( TIMER_INT nTickCount, uint32_t nLocalSessionId, uint32_t nFlag )
	{
		uint32_t nIndex= (nTickCount - m_nInitCount) % MAX_TIMER_COUNT;
		m_RudpTimer[nIndex].registerTimer( nLocalSessionId, nFlag );
		return;
	}

	CRudpTimer*	getCurrent(){ return m_pCurrent; }
	void	goNext()
	{
		m_pCurrent->m_nTimerTick+= MAX_TIMER_COUNT;
		m_pCurrent->reset();
		m_pCurrent= m_pCurrent->m_pNext;
	}

private:
	CRudpTimer	m_RudpTimer[MAX_TIMER_COUNT];	
	CRudpTimer*	m_pCurrent;
	TIMER_INT	m_nInitCount;
};

