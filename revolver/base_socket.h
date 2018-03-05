/*************************************************************************************
*filename:	base_socket.h
*
*to do:		����SOCKET��������
*Create on: 2012-05
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __BASE_SOCKET_H
#define __BASE_SOCKET_H

#include "base_namespace.h"
#include "base_inet_addr.h"

BASE_NAMESPACE_BEGIN_DECL

class CBaseSocket
{
public:
	CBaseSocket();
	virtual ~CBaseSocket();

	BASE_HANDLER	get_handler() const;
	void			set_handler(BASE_HANDLER handle);

	virtual int32_t	open(const Inet_Addr& local_addr, bool nonblocking = false , bool resue = true, bool client = false) = 0;
	int32_t			close();

	bool			isopen() const;

	//��������
	int32_t			set_option(int level, int option, void *optval, int optlen) const;
	int32_t			get_option(int level, int option, void *optval,int *optlen) const;

	//��ȡSOCKET�󶨵ĵ�ַ
	int32_t			get_local_addr (Inet_Addr &local_addr) const;

	int32_t			recv(void* buf, uint32_t buf_size, Inet_Addr& remote_addr);
	int32_t			recv(void* buf, uint32_t buf_size);
	int32_t			send(const void* buf, uint32_t buf_size, const Inet_Addr& remote_addr);
	int32_t			send(const void* buf, uint32_t buf_size);

protected:
	BASE_HANDLER	handler_;
};

BASE_NAMESPACE_END_DECL

#endif
/************************************************************************************/
