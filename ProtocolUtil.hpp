#ifndef __PROTOCOLUTIL_H__
#define __PROTOCOLUTIL_H__

#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <algorithm>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>

#include "HttpServer.hpp"
#include "Config.h"

#define BACKLOG 5
#define BUF_NUM 1024

#define NORMAL 0
#define WARNING 1
#define ERROR  2


const char* ErrLevel[] = { "Normal", "Waring", "Error" };

void log(std::string msg, int level, std::string file, int line)
{
	std::cout << " [ "<< file << " : " << line << " ] " << msg << " ["<< ErrLevel[level] << "]" << std::endl;
}

#define LOG(msg, level) log(msg, level, __FILE__, __LINE__)

class Util
{
	public:
		// 函数功能： 将一个 aaa: bbb 类型的字符串分隔成 key, value 结构
		static void MakeKV(std::string s, std::string &k, std::string &v)
		{
			std::size_t pos = s.find(": ");
			k = s.substr(0, pos);
			v = s.substr(pos + 2);
		}

		// 将 int 转换成 string
		static std::string IntToString(int &i)
		{
			std::stringstream ss;
			ss << i;
			return ss.str();
		}

		// 将状态码转换成状态码描述信息
		static std::string CodeToDesc(int code)
		{
			switch(code)
			{
				case 200:
					return "Ok";
				case 404:
					return "File Not Found";
				default:
					break;
			}

			return "Unknow";
		}
};


class SockAPI
{
public:
	static int Socket()
	{
		int sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0)
		{
			LOG("socket error", ERROR);
			exit(2);
		}

		return sock;
	}

	static void Bind(int sock, int port)
	{
		struct sockaddr_in addr;
		bzero(&addr, sizeof(addr));

		int opt = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(sock, (sockaddr *)&addr, sizeof(addr)) < 0)
		{
			LOG("bind error", ERROR);
			exit(3);
		}
	}

	static void Listen(int sock)
	{
		if (listen(sock, BACKLOG) < 0)
		{
			LOG("listen error", ERROR);
			exit(4);
		}
	}

	static int Accept(int listen_sock, std::string &ip, int &port)
	{
		struct sockaddr_in peer;

		socklen_t len = sizeof(peer);

		int sock = accept(listen_sock, (struct sockaddr*)&peer, &len);
		if (sock < 0)
		{
			LOG("accept error", WARNING);

			return -1;
		}

		port = ntohs(peer.sin_port);
		ip = inet_ntoa(peer.sin_addr);

		return sock;
	}
};

class Http_Response
{
public:
	std::string status_line;								// 响应行
	std::vector<std::string> response_header;				// 响应报头
	std::string blank;
	std::string response_text;								// 响应正文

private:
	int code;
	std::string path;
	int resource_size;										// 保存目标资源的大小

public:
	Http_Response(): blank("\r\n"), code(200), resource_size(0)
	{}

	// 构造状态行
	void MakeStatusLine()
	{
		status_line = "HTTP/1.O";
		status_line += " ";

		status_line += Util::IntToString(code);
		status_line += " ";

		status_line += Util::CodeToDesc(code);
		status_line += "\r\n";

		LOG("Make Status Line Ok!", NORMAL);
	}

	// 构造响应报头
	void MakeResponseHeader()
	{
		std::string line;
		line  = "Content-Length: ";
		line += Util::IntToString(resource_size);
		line += "\r\n";
		response_header.push_back(line);

		response_header.push_back(blank);
		LOG("Make Response Header Ok!", NORMAL);
	}

	// 获取状态码
	int &Code()
	{
		return code;
	}

	// 设置资源路径
	void SetPath(std::string &path_)
	{
		path = path_;
	}

	std::string &GetPath()
	{
		return path;
	}

	// 设置目标资源的大小
	void SetResourceSize(int rs_)
	{
		resource_size = rs_;
	}

	// 获取目标资源的大小
	int ResourceSize()
	{
		return resource_size;
	}

	~Http_Response()
	{}

};

class Http_Request
{
// 协议字段
public:
	std::string request_line;								// 请求行
	std::vector<std::string> request_header;				// 请求报头
	std::string blank;
	std::string request_text;								// 请求正文

// 解析字段
private:
	std::string method;										// 保存请求行中的方法
	std::string uri;										// 保存请求行中的 uri
	std::string version;									// 保存请求行中的 HTTP 版本
	std::string path;										// 保存请求的资源路径
	int content_length;
	//int resource_size;									// 保存请求资源的大小
	std::string query_string;								// 保存请求参数
	std::unordered_map<std::string, std::string> header_kv; // 保存请求报头的 key, value
	bool cgi;

public:
	Http_Request(): blank("\r\n"), path(WEBROOT), cgi(false)//, resource_size(0)
	{}

	// 解析请求行
	void RequestLineParse()
	{
		std::stringstream ss(request_line);
		
		ss >> method >> uri >> version;

		transform(method.begin(), method.end(), method.begin(),::toupper);

		std::cout << method << std::endl;
	}

	// 判断请求行中的方法是否合法
	bool IsMethodLegal()
	{
		for (auto e: RequestMethod)
		{
			if (e == method)
				return true;
		}

		return false;
	}

	// 解析请求行中的 uri
	// 根据 GET 和 POST 方法传参方式不同进行不同的处理
	void UriParse()
	{
		if (method == "GET")
		{
			// GET
			// 判断有没有参数
			std::size_t pos = uri.find("?");
			if (pos != std::string::npos)
			{
				// uri 中有参数
				cgi = true;
				path += uri.substr(0, pos);
				query_string = uri.substr(pos + 1);
			}
			else
			{
				// 没有参数
				cgi = false;
				path += uri;
			}
		}
		else if (method == "POST")
		{
			// POST  
			path += uri;
		}

		if (path[path.size() - 1] == '/')
		{
			path += HOMEPAGE;
		}

		std::cout << path << std::endl;
	}

	// 判断请求行中路径是否合法
	bool IsPathLegal(Http_Response *rsp)
	{
		struct stat st;
		int rs = 0;

		if (stat(path.c_str(), &st) < 0)
		{
			LOG("file not exist!", WARNING);
			return 404;
		}
		else
		{
			rs = st.st_size;
			if (S_ISDIR(st.st_mode))
			{
				// 不在根目录，需要加入 "/"
				path += "/";
				path += "HOMEPAGE";

				// 这里的资源大小需要进行更新，uri 中为目录时默认为它加了一个 HOMEPATH 的资源路径
				stat(path.c_str(), &st);
				rs = st.st_size;
			}
			// 判断是否具有可执行权限，如果具有可执行权限，则以 cgi 的方式进行运行
			else if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
			{
				cgi = true;
			}
			else
			{
				// TODO
			}
		}

		// 文件存在并且符合条件
		rsp->SetPath(path);
		rsp->SetResourceSize(rs);

		LOG("PATH ok", NORMAL);
		 
		return 0;
	}

	// 解析报头
	void HeaderParse()
	{
		std::string k, v;
		for (auto e: request_header)
		{
			// 使用 工具类 中的 KV 生成函数：将一个字符串分割成 key, value 结构
			Util::MakeKV(e, k, v);
			header_kv.insert({k, v});
		}
	}

	// 判断是否需要继续读
	bool IsNeedRecv()
	{
		// TODO 只考虑了 POST 和 GET 两种情况
		return method == "POST" ? true : false;
	}

	bool IsCgi()
	{
		return cgi;
	}

	// 获取 content-length 字段
	int ContentLength()
	{
		std::string cl = header_kv["Content-Length"];
		std::stringstream ss(cl);
		ss >> content_length;
		
		return content_length;
	}

	~Http_Request()
	{}
};

class Connect
{
private:
	int sock;

public:
	Connect(int sock_):sock(sock_)
	{}

	// 读取行
	int RecvOneLine(std::string &line_)
	{
		char buf[BUF_NUM];
		char ch = '\0';
		int i = 0;
		
		while ( ch != '\n' && i < BUF_NUM - 1 )
		{	
			std::size_t s = recv(sock, &ch, 1, 0);
			if ( s > 0 )
			{
				if (ch == '\r')
				{
					recv(sock, &ch, 1, MSG_PEEK);
					if (ch == '\n')	
					{
						recv(sock, &ch, 1, 0);
					}
					else
					{
						ch = '\n';
					}
				}

				buf[i++] = ch;
			}
			else
			{
				break;
			}
		}

		buf[i] = 0;
		line_ = buf;

		return i;
	}

	// 读取报头
	void RecvRequestHeader(std::vector<std::string> &v)
	{
		std::string line = "a";

		// 判断返回值
		while (line != "\n")
		{
			// 返回值是读取到的一行的字节数，行中内容保存在 line 中
			RecvOneLine(line);
			if (line != "\n")
				v.push_back(line);
		}

		LOG("header is ok", NORMAL);
	}

	// 读取正文
	void RecvText(std::string &text, int content_length)
	{
		char c;
		for (auto i = 0; i < content_length; i++)
		{
			recv(sock, &c, 1, 0);
			text.push_back(c);
		}
	}

	// 发送状态行
	void SendStatusLine(Http_Response *rsp)
	{
		std::string &sl = rsp->status_line;
		send(sock, sl.c_str(), sl.size(), 0);
	}

	// 发送报头
	void SendHeader(Http_Response *rsp) 
	{
		std::vector<std::string> &v = rsp->response_header;

		for (auto it = v.begin(); it != v.end(); it++)
		{
			send(sock, it->c_str(), it->size(), 0);
		}
	}

	// 发送正文
	void SendText(Http_Response *rsp)
	{
		std::string &path = rsp->GetPath();
		int fd = open(path.c_str(), O_RDONLY);
		if (fd < 0)
		{
			LOG("open file error!", WARNING);
			return ;
		}
		
		sendfile(sock, fd, nullptr, rsp->ResourceSize());

		close(fd);
	}

	~Connect()
	{
		close(sock);
	}

};



class Entry
{
public:
	// 运行非 cgi 程序
	static void ProcessNonCgi(Connect* conn, Http_Request *rq, Http_Response *rsp)
	{
		rsp->MakeStatusLine();
		rsp->MakeResponseHeader();
		//rsp->MakeResponseText(rq);

		conn->SendStatusLine(rsp);
		conn->SendHeader(rsp); // 空行也发出去
		conn->SendText(rsp);

		LOG("Send Response Ok!", NORMAL);
	}

	// 运行响应程序
	static void ProcessResponse(Connect *conn, Http_Request *rq, Http_Response *rsp)
	{
		if (rq->IsCgi())
		{
			// ProcessCgi();
		}
		else
		{
			LOG("MakeResponse need nonecgi!", NORMAL);
			ProcessNonCgi(conn, rq, rsp);
		}
	}

	// 运行程序
	static void *HandlerRequest(void *arg)
	{
		pthread_detach(pthread_self());
		int *sock = (int *)arg;

#ifdef _DEBUG_
		char buf[10240];
		read(*sock, buf, sizeof(buf));

		std::cout << "##############################" << std::endl;
		std::cout << buf;
		std::cout << "##############################" << std::endl;

#else
		Connect *conn = new Connect(*sock);
		Http_Request *rq = new Http_Request();
		Http_Response *rsp = new Http_Response();
		conn->RecvOneLine(rq->request_line);

		int &code = rsp->Code();

		rq->RequestLineParse();
		if (!rq->IsMethodLegal())
		{
			LOG("request method illegal!", WARNING);
			goto end;
		}

		LOG("request_line parse ok", NORMAL);

		// 解析 url
		rq->UriParse();

		// 路径不合法
		if (rq->IsPathLegal(rsp))
		{
			code = 404;
			LOG("file is not exist!", WARNING);
			goto end;
		}

		// 将报头 和 空行获取出来
		conn->RecvRequestHeader(rq->request_header);

		rq->HeaderParse();

		LOG("Http request header read done!", NORMAL);

		// 根据 GET 和 POST 是否具有正文，进行分类
		if (rq->IsNeedRecv())
		{
			LOG("POST need continue read text", NORMAL);
			conn->RecvText(rq->request_text, rq->ContentLength());
		}
		
		// HTTP 请求读取完毕
		LOG("Http request read done!", NORMAL);

		// 写响应
		ProcessResponse(conn, rq, rsp);
end:
		delete conn;
		delete rq;
		delete rsp;
		delete sock;
#endif
		//close(sock); // ~connect close(sock) 
		return (void *)0;
	}

};


#endif //__PROTOCOLUTIL_H__
