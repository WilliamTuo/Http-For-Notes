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
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>

#include "HttpServer.hpp"
#include "Config.h"

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

		// 将状态码转换成状态码描述信息 TODO
		static std::string CodeToDesc(int code)
		{
			switch(code)
			{
				case 200:
					return "Ok";
				case 400:
					return "Bad Request";
				case 404:
					return "File Not Found";
				case 500:
					return "Internal Server Error";
				default:
					break;
			}

			return "Unknow";
		}

		static std::string SuffixToContent(std::string &suffix)
		{
			// TODO
			if (suffix == ".css")
			{
				return "text/css";
			}
			if (suffix == ".js")
			{
				return "application/javascript";
			}
			if (suffix == ".jpg")
			{
				return	"application/x-jpg";		
			}
			if (suffix == ".html")
			{
				return "text/html";
			}

			return "text/html";
		}


		// 将一个错误码转换到对应的错误处理页面
		static std::string CodeToExceptFile(int code)
		{
			switch(code)
			{
				case 404:
					return HTML_404;
				case 500:
					return HTML_404;
				case 503:
					return HTML_404;
				default:
					break;
			}
		}

		static int FileSize(std::string &except_path)
		{
      struct stat st;

      stat(except_path.c_str(), &st);
      return st.st_size;
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
	std::string version;
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
		status_line = version;
		status_line += " ";

		status_line += Util::IntToString(code);
		status_line += " ";

		status_line += Util::CodeToDesc(code);
		status_line += "\r\n";

		LOG("Make Status Line Ok!", NORMAL);
	}

	// 构造响应报头 TODO
	void MakeResponseHeader()
	{
		std::string line;
		std::string suffix;
		
		// Content-Type
		line = "Content-Type: ";
		std::size_t pos = path.rfind(".");
		if (std::string::npos != pos)
		{
			suffix = path.substr(pos);
			// 将文件名的后缀转成小写
			transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
		}
		line += Util::SuffixToContent(suffix);
		line += "\r\n";
		response_header.push_back(line);

		// Content-Length
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

  // 设置版本号
  void SetVersion(std::string version_)
  {
    version = version_;
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
	void RequestLineParse(Http_Response *req)
	{
		std::stringstream ss(request_line);
		
		ss >> method >> uri >> version;

		transform(method.begin(), method.end(), method.begin(),::toupper);
    
    req->SetVersion(version);
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
	int IsPathLegal(Http_Response *rsp)
	{
		struct stat st;
		int rs = 0;

		if (stat(path.c_str(), &st) < 0)
		{
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
        ;
			}
		}

		// 文件存在并且符合条件
		rsp->SetPath(path);
		rsp->SetResourceSize(rs);

		LOG("PATH ok", NORMAL);
		 
		return 200;
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
		// 只考虑了 POST 和 GET 两种情况
		if (method == "POST")
    {
      return true;
    }
    else 
    {
      return false;
    }
	}

	bool IsCgi()
	{
		return cgi;
	}

  // 获取 方法
  std::string GetMethod()
  {
    return method;
  }

	// 获取 content-length 字段
	int ContentLength()
	{
		std::string cl = header_kv["Content-Length"];
		std::stringstream ss(cl);
		ss >> content_length;
		
		return content_length;
	}
	
	// 获取请求中的参数
	std::string GetParam()
	{
		if (method == "GET")
		{
			return query_string;
		}
		else if (method == "POST")
		{
			return request_text;
		}
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
	void SendText(Http_Response *rsp, bool cgi)
	{
		if (!cgi)
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
		else
		{
			std::string &rsp_text = rsp->response_text;
			send(sock, rsp_text.c_str(), rsp_text.size(), 0);
		}
	}

	void ClearRequest(Http_Request *rq)
	{
		std::string line = "a";
    char ch = '\0';

		// 读到空行
		while (line != "\n")
		{
			RecvOneLine(line);
		}

		// 获取 content-length, 读取正文
    int length = rq->ContentLength(); 
    while (length--)
    {
      recv(sock, &ch, 1, 0);
    }
	}

	~Connect()
	{
		close(sock);
	}

};

class Entry
{
public:
	// 运行 cgi 程序
	static int ProcessCgi(Connect *conn, Http_Request *rq, Http_Response *rsp)
	{
		// 获取 cgi 可执行文件的路径
    std::string bin = rsp->GetPath();


		// 获取参数及参数大小
		std::string param = rq->GetParam();
		int size =  param.size();
		std::string param_size = "CONTENT-LENGTH=";
		param_size += Util::IntToString(size);
    

		std::string &response_text = rsp->response_text;  // 用于存储父进程接收到的子进程运行的结果


		// 相对子进程获取和子进程输出创建管道
		int input[2];	// 子进程进行参数获取的管道描述符
		int output[2];  // 子进程进行输出结果的管道描述符
		pipe(input);
		pipe(output);

		pid_t id = fork();
		if (id < 0)
		{
			LOG("fork error", WARNING);
			return 500;
		}
		else if (id == 0)
		{
			// 子进程
			close(input[1]);
			close(output[0]);

			// 将参数通过环境变量的方式传递给 cgi 程序
			putenv((char *)param_size.c_str());

			// 将管道描述符重定向到标准输入，标准输出，
			// 防止 exec 进行程序替换时将管道描述符作为数据进行转换
			dup2(input[0], 0);
			dup2(output[1], 1);
			
      // 调用 exec 函数进行程序替换
			execl(bin.c_str(), bin.c_str(), nullptr);

			exit(1);
		}
		else
		{
			// 父进程
			close(input[0]);
			close(output[1]);


			char c;
			for (auto i = 0; i < size; ++i)
			{
				c = param[i];
				write(input[1], &c, 1);
			}
			
			waitpid(id, nullptr, 0);

			while (read(output[0], &c, 1) > 0)
			{
				response_text.push_back(c);
			}	

			rsp->MakeStatusLine();
			rsp->SetResourceSize(response_text.size());
			rsp->MakeResponseHeader();

			conn->SendStatusLine(rsp);
			conn->SendHeader(rsp); 
			conn->SendText(rsp, true);

		}

		return 200;
	}

	// 运行非 cgi 程序
	static int ProcessNonCgi(Connect* conn , Http_Response *rsp)
	{
		rsp->MakeStatusLine();
		rsp->MakeResponseHeader();
		//rsp->MakeResponseText(rq);

		conn->SendStatusLine(rsp);
		conn->SendHeader(rsp); // 空行也发出去
		conn->SendText(rsp, false);

		LOG("Send Response Ok!", NORMAL);

		return 200;
	}

	// 运行响应程序
	static int ProcessResponse(Connect *conn, Http_Request *rq, Http_Response *rsp)
	{
		if (rq->IsCgi())
		{
			LOG("MakeResponse need cgi!", NORMAL);
			return ProcessCgi(conn, rq, rsp);
		}
		else
		{
			LOG("MakeResponse need nonecgi!", NORMAL);
			return ProcessNonCgi(conn, rsp);
		}
	}

	// 运行程序
	static void HandlerRequest(int sock/*void *arg*/)
	{
		pthread_detach(pthread_self());
		//int *sock = (int *)arg;

#ifdef _DEBUG_
		char buf[10240];
		read(sock, buf, sizeof(buf));

		std::cout << "##############################" << std::endl;
		std::cout << buf;
		std::cout << "##############################" << std::endl;

#else
		Connect *conn = new Connect(sock);
		Http_Request *rq = new Http_Request();
		Http_Response *rsp = new Http_Response();
		conn->RecvOneLine(rq->request_line);

		int &code = rsp->Code();

		rq->RequestLineParse(rsp);
		if (!rq->IsMethodLegal())
		{
			code = 400;
			conn->ClearRequest(rq);
			//ProcessResponse(conn, rq, rsp);
			LOG("request method illegal!", WARNING);
			goto end;
		}

		LOG("request_line parse ok", NORMAL);

		// 解析 url
		rq->UriParse();

		// 路径不合法
		if ((code = rq->IsPathLegal(rsp)) != 200)
		{
			conn->ClearRequest(rq);
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
		code = ProcessResponse(conn, rq, rsp);
end:
		// Request 已经读取完毕，错误处理
		if (code != 200)
		{
			std::string except_path = Util::CodeToExceptFile(code);
			int rs = Util::FileSize(except_path);
			rsp->SetPath(except_path);
			rsp->SetResourceSize(rs);
			ProcessNonCgi(conn, rsp);
		}
		delete conn;
		delete rq;
		delete rsp;
		//delete sock;
#endif
		//close(sock); // ~connect close(sock) 
		//return (void *)0;
	}

};

#endif //__PROTOCOLUTIL_H__
