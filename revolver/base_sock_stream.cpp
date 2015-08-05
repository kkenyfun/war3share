#include "base_sock_stream.h"

BASE_NAMESPACE_BEGIN_DECL
CSockStream::CSockStream()
{

}

CSockStream::~CSockStream()
{

}

int32_t CSockStream::open(const Inet_Addr& local_addr, bool nonblocking /* = false */, bool resue, bool client)
{
	handler_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(handler_ == INVALID_HANDLER)
	{
		return -1;
	}

	if(nonblocking) //�����첽SOCKET
	{
		set_socket_nonblocking(get_handler());
	}

	//�����շ�������
	int32_t buf_size = 64 * 1024;
	set_option(SOL_SOCKET, SO_RCVBUF, (void *)&buf_size, sizeof(int32_t));
	set_option(SOL_SOCKET, SO_SNDBUF, (void *)&buf_size, sizeof(int32_t));

	//����һ����������
	if(!local_addr.is_null())
	{
		//���ö˿ڸ���
		if(resue)
		{
			int32_t val = 1;
			set_option(SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int32_t));
		}

		local_addr_ = local_addr;

		if(client)
		{
			uint16_t port = local_addr_.get_port();
			uint32_t count = 0;
			while(this->bind(local_addr_) != 0) //һֱ�󶨣�ֱ���ɹ�
			{
				count ++;
				if (count > 4000)
				{
					CBaseSocket::close();
					return -1;
				}

				if (count > 2000 && local_addr_.get_ip() != 0)
				{
					local_addr_.set_ip(0);
				}

				port ++;
				local_addr_.set_port(port);
			}
		}
		else
		{
			if(this->bind(local_addr_) != 0)
			{
				CBaseSocket::close();
				return -1;
			}
		}
	}

	return 0;
}

int32_t CSockStream::get_remote_addr(Inet_Addr &remote_addr) const
{
	if(!isopen())
		return -1;
#ifdef WIN32
	int32_t len = sizeof(sockaddr_in);
#else
	uint32_t len = sizeof(sockaddr_in);
#endif

	sockaddr* addr = reinterpret_cast<sockaddr *>(remote_addr.get_addr());
	if(::getpeername(get_handler(), addr, &len) == -1)
	{
		return -1;
	}

	return 0; 
}

int32_t CSockStream::bind(Inet_Addr &local_addr)
{
	int32_t ret = ::bind(handler_, (struct sockaddr *)local_addr.get_addr(), sizeof(sockaddr_in));
	if(ret != 0)
	{
		return -1;
	}

	return 0;
}


BASE_NAMESPACE_END_DECL

