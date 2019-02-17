#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__

#include <iostream>
#include "ProtocolUtil.hpp"


class HttpServer
{
private:
	int listen_sock;
	int port;

public:
	HttpServer(int port_)
		: port(port_)
		, listen_sock(-1)
	{}

	void InitServer()
	{
		listen_sock = SockAPI::Socket();
		SockAPI::Bind(listen_sock, port);
		SockAPI::Listen(listen_sock);
	}

	void Start()
	{
		while (1)
		{
			std::string peer_ip;
			int peer_port;
			int *sockp = new int;
			*sockp = SockAPI::Accept(listen_sock, peer_ip, peer_port);
			if (*sockp >= 0)
			{
				std::cout << peer_ip << " : " << peer_port << std::endl;
				pthread_t tid;
				pthread_create(&tid, nullptr, Entry::HandlerRequest, (void *)sockp);
			}
		}
	}

	~HttpServer()
	{
		if (listen_sock > 0)
		{
			close(listen_sock);
		}
	}
};

#endif // __HTTPSERVER_H__
