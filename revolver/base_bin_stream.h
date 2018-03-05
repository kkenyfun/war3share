/*************************************************************************************
*filename:	base_bin_stream.h
*
*to do:		ʵ��Э�����Ķ����ƴ��������
*Create on: 2012-04
			2012-11 ���� =�����ء�bin_to_string������������Ҫʵ�ֶ�STRING���໥ת��
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __BASE_BIN_STREAM_H
#define __BASE_BIN_STREAM_H

#include "base_namespace.h"
#include "base_typedef.h"
#include "base_hex_string.h"
#include <assert.h>
#include <string.h>
#include <string>

#define MAX_STREAM_STIRNG_SIZE 10485760 //10M

using namespace std;

BASE_NAMESPACE_BEGIN_DECL

//1K��Ĭ�ϴ�С
#define DEFAULT_PACKET_SIZE	 1024

#define ENCODE(data, type) \
	int32_t type_size = sizeof(type);\
	resize(used_ + type_size);\
	uint8_t* pos = (uint8_t*)&data;\
	if(big_endian) \
	{\
		::memcpy(wptr_, pos, type_size);\
	}\
	else\
	{\
		pos = pos + type_size - 1;\
		for(int32_t i = 0; i < type_size; ++i)\
		{\
			wptr_[i] = *pos;\
			-- pos;\
		}\
	}\
	used_ += type_size;\
	wptr_ += type_size;\


#define DECODE(data, type)\
	int32_t type_size = sizeof(type);\
	if(used_ >= rsize_ + type_size)\
	{\
		uint8_t* pos = (uint8_t*)&data;\
		if(big_endian)\
		{\
			::memcpy(pos ,wptr_, type_size);\
		}\
		else\
		{\
			pos = pos + type_size - 1;\
			for(int32_t i = 0; i < type_size; ++i)\
			{\
				*pos = rptr_[i];\
				 -- pos; \
			}\
		}\
		rsize_ += type_size;\
		rptr_ += type_size;\
	}\
	else \
	{\
		memset(&data, 0x00, type_size);\
	}

template<class Elem, uint32_t CAPACITY>
class BinStream_T
{
public:
	typedef BinStream_T<Elem, CAPACITY> _MyBint;

	BinStream_T();
	virtual ~BinStream_T();

	//��λ
	void		rewind(bool reset = false);
	void		resize(uint32_t new_size);
	//����ֻ��������״̬�µ���
	void		reduce();
	void		set_used_size(uint32_t used)
	{
		used_ = used;
	};

	const Elem* get_data_ptr() const
	{
		return data_;
	};

	uint8_t*	get_wptr()
	{
		return wptr_;
	};

	const uint8_t* get_rptr() const
	{
		return rptr_;
	};

	//��ȡ�������Ĵ�С
	uint32_t	size() const
	{
		return size_;
	};
	//��ȡ����������Ĵ�С
	uint32_t	data_size() const
	{
		return used_;
	};
	uint32_t	read_size() const
	{
		return rsize_;
	};
	_MyBint& operator<<(bool val)
	{
		ENCODE(val, bool);
		return (*this);
	};

	_MyBint& operator>>(bool& val)
	{
		DECODE(val, bool);
		return (*this);
	};

	_MyBint& operator<<(int8_t val)
	{
		ENCODE(val, int8_t);
		return (*this);
	};

	_MyBint& operator>>(int8_t& val)
	{
		DECODE(val, int8_t);
		return (*this);
	};

	_MyBint& operator<<(int16_t val)
	{
		ENCODE(val, int16_t);
		return (*this);
	};

	_MyBint& operator>>(int16_t& val)
	{
		DECODE(val, int16_t);
		return (*this);
	};

	_MyBint& operator<<(int32_t val)
	{
		ENCODE(val, int32_t);
		return (*this);
	};
	_MyBint& operator>>(int32_t& val)
	{
		DECODE(val, int32_t);
		return (*this);
	};

	_MyBint& operator<<(int64_t val)
	{
		ENCODE(val, int64_t);
		return (*this);
	};

	_MyBint& operator>>(int64_t& val)
	{
		DECODE(val, int64_t);
		return (*this);
	};

	_MyBint& operator<<(uint8_t val)
	{
		ENCODE(val, uint8_t);
		return (*this);
	};

	_MyBint& operator>>(uint8_t& val)
	{
		DECODE(val, uint8_t);
		return (*this);
	};

	_MyBint& operator<<(uint16_t val)
	{
		ENCODE(val, uint16_t);
		return (*this);
	};

	_MyBint& operator>>(uint16_t& val)
	{
		DECODE(val, uint16_t);
		return (*this);
	};

	_MyBint& operator<<(uint32_t val)
	{
		ENCODE(val, uint32_t);
		return (*this);
	};

	_MyBint& operator>>(uint32_t& val)
	{
		DECODE(val, uint32_t);
		return (*this);
	};

	_MyBint& operator<<(uint64_t val)
	{
		ENCODE(val, uint64_t);
		return (*this);
	};

	_MyBint& operator>>(uint64_t& val)
	{
		DECODE(val, uint64_t);
		return (*this);
	};
	
	_MyBint& operator<<(const string& val)
	{
		uint32_t val_size = val.size();
		ENCODE(val_size, uint32_t);
	
		resize(used_ + val_size);
		::memcpy((void *)wptr_, (const void *)val.data(), (size_t)val_size);
		wptr_ += val_size;
		used_ += val_size;

		return (*this);
	};

	_MyBint& operator>>(string& val)
	{
		uint32_t val_size = 0;
		DECODE(val_size, uint32_t);

		if(val_size + rsize_ > used_) //��ֹԽ�����
		{			
			throw 0;
		}
		else if(val_size == 0)
		{
			val = "";
		}
		else 
		{
			val.assign((char *)rptr_, val_size);

			rptr_ += val_size;
			rsize_ += val_size;
		}

		return (*this);
	};

	_MyBint& operator=(const _MyBint& strm)
	{
		resize(strm.size_);
		::memcpy(data_, strm.data_, strm.size_);
		used_ = strm.used_;
		rsize_ = strm.rsize_;
		rptr_ = data_ + rsize_;
		wptr_ = data_ + used_;

		return *this;
	}

	_MyBint& operator=(const string& data)
	{
		rewind(true);
		resize(data.size());
		set_used_size(data.size());

		::memcpy(get_wptr(), data.data(), data.size());
		wptr_ = data_ + used_;

		return *this;
	}

	void push_data(const uint8_t *data, uint32_t data_len)
	{
		ENCODE(data_len, uint32_t);

		resize(used_ + data_len);
		::memcpy((void *)wptr_, (const void *)data, (size_t)data_len);
		wptr_ += data_len;
		used_ += data_len;
	}

	void bin_to_string(string& data)
	{
		data.clear();
		data.assign((char*)rptr_, data_size());
	}

	const string to_string()
	{
		string text;
		return bin2asc((uint8_t *)data_, used_);
	}

protected:
	Elem*		data_;	//���ݻ�����
	uint8_t*	rptr_;	//��ǰ��λ��ָ��
	uint8_t*	wptr_;	//��ǰдλ��ָ��

	size_t		size_;	//��󻺳����ߴ�
	size_t		used_;	//�Ѿ�ʹ�õĻ������ߴ�
	size_t		rsize_;	//��ȡ���ֽ���

	bool		big_endian;//ϵͳ�ֽ����־
};

typedef BinStream_T<uint8_t, DEFAULT_PACKET_SIZE> BinStream;

BASE_NAMESPACE_END_DECL

#include "base_bin_stream.inl"

#endif

/************************************************************************************/



