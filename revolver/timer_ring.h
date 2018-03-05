/*************************************************************************************
*filename:	timer_ring.h
*
*to do:		���嶨ʱ����ת�࣬ʵ�̶ֿ��ƽ������л�
*Create on: 2012-04
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __TIMER_RING_H
#define __TIMER_RING_H

#include "base_namespace.h"
#include "base_typedef.h"

#include <set>
#include <vector>

BASE_NAMESPACE_BEGIN_DECL

using namespace std;

typedef std::set<uint32_t>		ElementSet;
typedef std::vector<ElementSet>	RingVector;

class IRingEvent
{
public:
	//�ֵ�ԭ�Ӵ����¼�
	virtual void ring_event(uint8_t ring_id, uint32_t timer_id) = 0;
};

//ʱ�����������Ĭ����256���ֵ�Ԫ��
class CTimerRing
{
public:
	CTimerRing(uint8_t ring_id = 0);
	~CTimerRing();

	void reset();

	bool add_element(uint8_t pos, uint32_t timer_id);
	void delete_element(uint8_t pos, uint32_t timer_id);

	//scale����ת�Ŀ̶ȣ�ring_handler��ԭ�Ӵ�����,�������ֵΪTRUE��˵�����ֵ�����ĩ�ˣ���Ҫ�л���
	bool cycle(uint32_t& scale, IRingEvent* ring_handler);

	uint32_t get_pos() const {return pos_;};
	void set_pos(uint32_t pos) {pos_ = pos;};
	void set_ring_id(uint8_t id) {ring_id_ = id;};
	uint8_t get_ring_id() const {return ring_id_;};
private:
	void		clear();

private:
	uint8_t		ring_id_;		//����ID
	uint32_t	pos_;			//����ָ��λ��

	RingVector	ring_;			//����
};

BASE_NAMESPACE_END_DECL
#endif
/*************************************************************************************/

