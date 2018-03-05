/*************************************************************************************
*filename:	base_sock_stream.h
*
*to do:		����TCP SOCKET��
*Create on: 2012-05
*Author:	zerok
*check list:
*************************************************************************************/

#ifndef __BASE_SOCK_STREAM_H
#define __BASE_SOCK_STREAM_H

#include "base_socket.h"

BASE_NAMESPACE_BEGIN_DECL
class CSockStream : public CBaseSocket
{
public:
	CSockStream();
	virtual ~CSockStream();

	int32_t			open(const Inet_Addr& local_addr, bool nonblocking = false, bool resue = true, bool client = false);
	//���Զ�˵Ķ�ӦIP��ַ,һ��TCP��Ч
	int32_t			get_remote_addr (Inet_Addr &remote_addr) const;
	//�󶨵�ַ
	int32_t			bind(Inet_Addr& local_addr);
private:
	Inet_Addr		local_addr_;
};
BASE_NAMESPACE_END_DECL
#endif
/************************************************************************************/


