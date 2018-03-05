/*************************************************************************************
*filename:	base_sock_connector.h
*
*to do:		����TCP SOCKET�����ӷ����࣬��Ҫ����TCP CLIENT
*Create on: 2012-05
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __BASE_SOCK_CONNNECTOR_H
#define __BASE_SOCK_CONNNECTOR_H

#include "base_sock_stream.h"

BASE_NAMESPACE_BEGIN_DECL

class CSockConnector
{
public:
	CSockConnector();
	~CSockConnector();

	int32_t connect(CSockStream& sock_stream, const Inet_Addr& remote_addr);

private:
	Inet_Addr remote_addr_;
};

BASE_NAMESPACE_END_DECL
#endif
/************************************************************************************/
