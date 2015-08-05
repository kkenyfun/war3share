/*************************************************************************************
*filename:	base_log_thread.h
*
*to do:		����LOG �߳���
*Create on: 2012-05
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __BASE_LOG_THREAD_H
#define __BASE_LOG_THREAD_H

#include "base_thread.h"
#include "base_queue.h"
#include "base_thread_mutex.h"
#include "object_pool.h"

using namespace BASE_NAMEPSACE_DECL;

typedef struct LogInfoData
{
	int32_t index;
	int32_t level;
	string	str_log;

	LogInfoData()
	{
		reset();
	};

	void reset()
	{
		level = 0;
		index = -1;
		str_log = "";
	};

}LogInfoData;

class BaseLogThread : public CThread
{
public:
	BaseLogThread();
	~BaseLogThread();

public:
	void put_log(LogInfoData* data);
	void execute();

	void clear();
	
	//���ڷ������˳�������
	void stop();

private:
#ifndef WIN32
	BaseQueue_T<LogInfoData*, BaseThreadMutex, 5120> queue_;
#else
	BaseQueue_T<LogInfoData*, BaseThreadMutex, 128> queue_;
#endif
};

#define LOG_THREAD_CREATE		CSingleton<BaseLogThread>::instance
#define LOG_THREAD_INSTANCE		CSingleton<BaseLogThread>::instance
#define LOG_THREAD_DESTROY		CSingleton<BaseLogThread>::destroy

//������Ϣ��
extern ObjectMutexPool<LogInfoData, BaseThreadMutex, LOG_POOL_SIZE>	LOGPOOL;

#endif

/************************************************************************************/


