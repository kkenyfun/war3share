/*************************************************************************************
*filename:	base_block_buffer.h
*
*to do:		ʵ��SOCKET�Ľ��ܺͷ���BUFFER���ƣ���Ҫ���TCP������շ�������
*Create on: 2012-04
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __BASE_BLOCK_BUFFER_H
#define __BASE_BLOCK_BUFFER_H

#include "base_namespace.h"
#include "base_typedef.h"
#include <string>
#include <ostream>

BASE_NAMESPACE_BEGIN_DECL

#define MAX_BUFFER_SIZE (1024 * 1024 * 30)

template<uint32_t CAPACITY>
class CBlockBuffer_T
{
public:
	CBlockBuffer_T()
	{
		read_pos_ = 0;
		write_pos_ = 0;
		buffer_ = (uint8_t *)malloc(CAPACITY * sizeof(uint8_t));
		//buffer_ = new uint8_t[CAPACITY];
		buffer_size_ = CAPACITY;
	};

	virtual ~CBlockBuffer_T()
	{
		read_pos_ = 0;
		write_pos_ = 0;

		//delete []buffer_;
		free(buffer_);
		buffer_size_  = 0;
	};

	bool realloc_buffer(uint32_t size) //����һ��2����BUFFER
	{
		//ʣ��ռ乻��
		if(size < remaining_length() || buffer_size_ > MAX_BUFFER_SIZE)
			return false;

		uint32_t buffer_size = buffer_size_ * 2;
		while(buffer_size -  data_length() < size) //�����µ�ʣ��ռ��Ƿ���
		{
			buffer_size = buffer_size * 2;
		}

		buffer_ = (uint8_t *)realloc(buffer_, buffer_size);
		//uint8_t* buffer = new uint8_t[buffer_size];
		//memcpy(buffer, buffer_, buffer_size_);
		//delete []buffer_;
		//buffer_ = buffer;

		buffer_size_ = buffer_size;

		return true;
	}

	void reset()
	{
		if(buffer_size_ > CAPACITY)
		{
			//delete []buffer_;
			//buffer_ = new uint8_t[CAPACITY];
			free(buffer_);

			buffer_ = (uint8_t *)malloc(CAPACITY * sizeof(uint8_t));
			buffer_size_ = CAPACITY;
		}
		
		memset(buffer_, 0x00, buffer_size_);

		read_pos_ = 0;
		write_pos_ = 0;
	}

	//��ȡ��λ��
	uint8_t* get_rptr()
	{
		return buffer_ + read_pos_;
	};
	//�ƶ���λ��
	void	 move_rptr(uint32_t n)
	{
		read_pos_ += n;
	};
	//��ȡдλ��
	uint8_t* get_wptr()
	{
		return buffer_ + write_pos_;
	};
	//�ƶ�дλ��
	void	 move_wptr(uint32_t n)
	{
		write_pos_ += n;
	};

	//BUFFER��������С
	uint32_t length() const
	{
		return buffer_size_;
	};
	//���ݳ���
	uint32_t data_length()
	{
		if(write_pos_ > read_pos_)
			return write_pos_ - read_pos_;
		else
		{
			recycle();
			return 0;
		}
	};
	//ʣ�໺��������
	uint32_t remaining_length() const
	{
		return buffer_size_ - write_pos_;
	};

	bool	is_full() const
	{
		return (remaining_length() == 0);
	};

	void	 recycle()
	{
		if(write_pos_ <= read_pos_)
		{
			write_pos_ = 0;
			read_pos_ = 0;
		}
		else if(read_pos_ > 0)
		{
			memmove(buffer_, buffer_ + read_pos_, write_pos_ - read_pos_);
			write_pos_ = write_pos_ - read_pos_;
			read_pos_ = 0;
		}	
	};

protected:
	uint8_t*	buffer_;
	uint32_t	buffer_size_;

	uint32_t	read_pos_;
	uint32_t	write_pos_;
};

//SOCKET���ܶ���
template<class T, class R, uint32_t CAPACITY>
class CReciverBuffer_T : public CBlockBuffer_T<CAPACITY>
{
public:
	CReciverBuffer_T()
	{

	};

	virtual ~CReciverBuffer_T()
	{

	};

	//�ӽ��ܶ�������ȡ����
	int32_t recv(R& reciver)
	{
		if(this->remaining_length() <= 0) //�޿ռ䣬���Կ��ٸ���Ŀռ�
		{
			this->realloc_buffer(this->buffer_size_ / 8);
		}

		int32_t read_size = reciver.recv(this->get_wptr(), this->remaining_length());
		if(read_size > 0)
		{
			this->move_wptr(read_size);
			this->recycle();
		}

		return read_size;
	};

	//�ж�һ�������Ƿ���ȡ���
	int32_t split(T& packet)
	{
		if(this->data_length() < sizeof(uint32_t))
			return 1;

		uint32_t packet_size = 0;
		
		packet.rewind(true);
		packet.resize(sizeof(uint32_t));
		memcpy(packet.get_wptr(), this->get_rptr(), sizeof(uint32_t));
		packet.set_used_size(sizeof(uint32_t));
		try{
			packet >> packet_size;
		}
		catch (...){
			return -1;
		}

		if(packet_size > MAX_BUFFER_SIZE / 2)
			return -1;

		//��ȡ������
		if(this->data_length() >= packet_size + sizeof(uint32_t))
		{
			packet.rewind(true);
			packet.resize(packet_size);
			memcpy(packet.get_wptr(), this->get_rptr() + sizeof(uint32_t), packet_size);
			packet.set_used_size(packet_size);

			this->move_rptr(packet_size + sizeof(uint32_t));

			this->recycle();

			return 0;
		}
		else if(this->data_length() < packet_size  + sizeof(uint32_t) && this->remaining_length() <= 0) //������̫С��
		{
			uint32_t alloc_size = packet_size + sizeof(uint32_t) - this->data_length();
			if(alloc_size > MAX_BUFFER_SIZE)
				alloc_size = MAX_BUFFER_SIZE;

			this->realloc_buffer(alloc_size);
		}

		return 1;
	};
};

//SOCKET���Ͷ���
template<class T, class S, uint32_t CAPACITY>
class CSenderBuffer_T : public CBlockBuffer_T<CAPACITY>
{
public:
	CSenderBuffer_T()
	{
	};
	virtual ~CSenderBuffer_T()
	{
	};

	//�����ݵݽ������Ͷ�����з���
	int32_t send(S& sender)
	{
		int32_t sender_size = sender.send(this->get_rptr(), this->data_length());
		if(sender_size > 0)
		{
			this->move_rptr(sender_size);
			this->recycle();
		}

		return sender_size;
	};

	//���������ݵݽ������Ͷ�����
	bool put(T& packet)
	{
		uint32_t packet_size = packet.data_size();
		if(packet_size <= 0 || packet_size > MAX_BUFFER_SIZE / 2)
			return false;

		if(packet_size + sizeof(uint32_t) > this->remaining_length())
		{
			if(!this->realloc_buffer(packet_size + sizeof(uint32_t) - this->remaining_length()))
				return false;
		}

		head.rewind(true);
		head << packet_size;
		memcpy(this->get_wptr(), head.get_rptr(), head.data_size());
		this->move_wptr( head.data_size());
		this->recycle();

		memcpy(this->get_wptr(), packet.get_rptr(), packet_size);
		this->move_wptr(packet_size);

		this->recycle();

		return true;
	};

	bool put(const string& packet)
	{
		uint32_t packet_size = packet.size();
		if(packet_size <= 0 || packet_size > MAX_BUFFER_SIZE / 2)
			return false;

		if(packet_size + sizeof(uint32_t) > this->remaining_length())
		{
			if(!this->realloc_buffer(packet_size + sizeof(uint32_t) - this->remaining_length()))
				return false;
		}

		head.rewind(true);
		head << packet_size;
		memcpy(this->get_wptr(), head.get_rptr(), head.data_size());
		this->move_wptr( head.data_size());
		this->recycle();

		memcpy(this->get_wptr(), packet.data(), packet_size);
		this->move_wptr(packet_size);

		this->recycle();

		return true;
	}

public:
	T	head;
};

BASE_NAMESPACE_END_DECL
#endif

/*************************************************************************************/


