/*************************************************************************************
*filename:	base_cache_buffer.h
*
*to do:		ʵ��һ�����嵥Ԫ�Ļ�������Ϊ�˿��Խ��ж�Ӧ�Ĳ��Һ͸��¡��滻��������Ҫ�û�
			��ý�屨�ĵĻ�����
*Create on: 2013-03
			2013-03 ʵ�ֻ����Ļ���������
*Author:	zerok
*check list:
*************************************************************************************/

#ifndef __BASE_CACHE_BUFFER_H_
#define __BASE_CACHE_BUFFER_H_

#include "base_typedef.h"
#include "base_namespace.h"

BASE_NAMESPACE_BEGIN_DECL

//T����һ��Ҫ��ָ������!!!!
template<typename T, int32_t CAPACITY>
class CacheBuffer_T
{
public:
	CacheBuffer_T()
	{
		min_index_ = 0;
		max_index_ = 0;

		min_key_ = 0;
		max_key_ = 0;

		buffer_ = new T[CAPACITY];
	};

	virtual ~CacheBuffer_T()
	{
		if(buffer_ != NULL)
		{
			delete []buffer_;
		}

		min_index_ = 0;
		max_index_ = 0;

		min_key_ = 0;
		max_key_ = 0;
	};

	T find(uint32_t key)
	{
		T ret = NULL;
		if(key >= min_key_ && key <= max_key_)
		{
			uint32_t index = (key - min_key_ + min_index_) % CAPACITY;
			ret = buffer_[index];
		}

		return ret;
	}

	//����һ����Ԫ
	bool insert(uint32_t key, const T& data)
	{
		bool ret = false;

		if(key > min_key_ && key < max_key_)
		{
			uint32_t index = (key - min_key_ + min_index_) % CAPACITY;
			if(buffer_[index] == NULL)
			{
				buffer_[index] = data;
				ret = true;
			}
		}
		else if(key > max_key_)
		{ 
			uint32_t key_space = key - max_key_;
			for(uint32_t i = 0; i < key_space; ++i)
			{
				max_index_ ++;
				if(max_index_ == min_index_) //��ǰ�ƶ�
				{
					min_index_ ++;
					min_index_ = min_index_ % CAPACITY;
				}

				max_index_ = max_index_ % CAPACITY;
				buffer_[max_index_] = NULL;
			}

			buffer_[max_index_] = data;

			max_key_ = key;
			if(min_key_ == 0)
			{
				min_key_ = key;
			}

			ret = true;
		}

		return ret;
	}

	//ɾ����ǰ��ĵ�Ԫ
	T erase()
	{
		T ret = NULL;
		if(emtpy())
			return ret;

		if(max_index_ == min_index_)
		{
			ret = buffer_[min_index_];
			buffer_[min_index_] = NULL;

			min_index_ = 0;
			max_index_ = 0;	
		}
		else
		{
			ret = buffer_[min_index_];
			buffer_[min_index_] = NULL;

			min_index_ ++;
			min_index_ = min_index_ % CAPACITY;

			min_key_ ++;
		}

		return ret;
	}

	bool emtpy() const
	{
		return ((max_index_ == 0 && min_index_ == 0 && buffer_[min_index_] == NULL) ? true : false);
	};


private:
	T*			buffer_;

	uint32_t	min_index_;		//��С��ŵ�λ��
	uint32_t	max_index_;		//�����ŵ�λ��

	uint32_t	min_key_;		//��С���
	uint32_t	max_key_;		//������
};

BASE_NAMESPACE_END_DECL

#endif


/************************************************************************************/


