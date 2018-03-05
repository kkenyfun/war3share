#pragma once


class CEasyBuffer
{
protected:
	uint8_t *		m_pBuffer;
	int			m_BufferSize;
	int			m_UsedSize;
	uint8_t		m_SelfDelete;
public:
	CEasyBuffer(uint8_t* pBuff,int Size)
	{
		m_pBuffer=pBuff;
		m_BufferSize=Size;
		m_UsedSize=0;
		m_SelfDelete= 0;
	}
	CEasyBuffer( int Size)
	{
		m_pBuffer=		new uint8_t[Size];
		m_BufferSize=	Size;
		m_SelfDelete=	1;
		m_UsedSize=		0;
	}
	~CEasyBuffer(void)
	{
		if( m_SelfDelete )
		{
			delete[] m_pBuffer;
			m_pBuffer=NULL;
		}
		return;
	}
	inline int GetBufferSize() const
	{
		return m_BufferSize;
	}
	inline int GetUsedSize() const
	{
		return m_UsedSize;
	}
	inline bool SetUsedSize(int Size)
	{
		if(Size>=0&&Size<=m_BufferSize)
		{
			m_UsedSize=Size;
			return true;
		}
		return false;
	}
	inline int GetFreeSize() const
	{
		return m_BufferSize-m_UsedSize;
	}
	inline uint8_t* GetBuffer() const
	{
		return m_pBuffer;
	}

	inline uint8_t* GetFreeBuffer() const
	{
		return m_pBuffer+m_UsedSize;
	}

	inline bool PushBack(const void* pData,int Size)
	{
		if(m_UsedSize+Size<=m_BufferSize)
		{		
			if(pData)
				memcpy(m_pBuffer+m_UsedSize,pData,Size);
			m_UsedSize+=Size;
			return true;
		}
		return false;
	}
	
	inline bool PopFront(void* pData,int Size)
	{
		if(Size<=m_UsedSize)
		{
			if(pData)
				memcpy(pData,m_pBuffer,Size);
			m_UsedSize-=Size;
			memmove(m_pBuffer,m_pBuffer+Size,m_UsedSize);		
			return true;
		}
		return false;
	}
};

