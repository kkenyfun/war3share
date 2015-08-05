#pragma once
#include "base_typedef.h"
#include "base_inet_addr.h"
#include "rudpdll.h"

using namespace BASE_NAMEPSACE_DECL;

class CDataNode
{
public:
	CDataNode( const uint8_t* pData, uint16_t nDataSize, Inet_Addr& sockAddr, uint32_t nIndex )
	{
		memcpy( m_szDataBuf, pData, nDataSize );
		m_nDataSize= nDataSize;
		memcpy( &m_sockAddr, &sockAddr, sizeof(Inet_Addr) );
		m_nIndex= nIndex;
	}

	CDataNode( const uint8_t* pData, uint16_t nDataSize, Inet_Addr& sockAddr )
	{
		memcpy( m_szDataBuf, pData, nDataSize );
		m_nDataSize= nDataSize;
		memcpy( &m_sockAddr, &sockAddr, sizeof(Inet_Addr) );
		m_nIndex= 0;
	}
	CDataNode( uint32_t nIndex =0 )
	{
		m_nDataSize=0;
		m_nIndex=	nIndex;
	}
	~CDataNode()
	{
	};
public:
	uint8_t			m_szDataBuf[RUDP_BUF_SIZE];
	int				m_nDataSize;
	uint32_t		m_nIndex;
	Inet_Addr		m_sockAddr;
};
