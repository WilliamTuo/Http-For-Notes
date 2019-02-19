#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__

#include <iostream>
#include <signal.h>

#include "ProtocolUtil.hpp"
#include "ThreadPool.hpp"

class HttpServer
{
private:
	int listen_sock;
	int port;
	//ThreadPool pool;

public:
	HttpServer(int port_)
		: port(port_)
		, listen_sock(-1)
		//, pool(POOL_NUM)  // 5
	{}

	void InitServer()
	{
		listen_sock = SockAPI::Socket();
		SockAPI::Bind(listen_sock, port);
		SockAPI::Listen(listen_sock);
		//pool.InitThreadPool();
	}

	void Start()
	{
		// 防止浏览器关闭链接后，服务器收到 SIG_PIPE 信号挂掉 
		signal(SIGPIPE, SIG_IGN);

		while (1)
		{
			std::string peer_ip;
			int peer_port;
			int sock = SockAPI::Accept(listen_sock, peer_ip, peer_port);
			if (sock >= 0)
			{
				std::cout << peer_ip << " : " << peer_port << std::endl;
				
				Task t(sock, Entry::HandlerRequest);
				Singleton::GetInstance()->PushTask(t);
				//pool.PushTask(t);
				//pthread_t tid;
				//pthread_create(&tid, nullptr, Entry::HandlerRequest, (void *)sockp);
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
