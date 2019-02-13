#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX 1024
#define HOME "index.html"
#define ERR_HOME "./wanroot/err.html"

// 绑定 socket 
static int sockup(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
	{
        perror("socket");
        exit(2);
    }
    
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
        perror("bind");
        exit(3);
    }

    if(listen(sock, 5) < 0)
	{
        perror("listen");
        exit(4);
    }

    return sock;
}

// 解析首行
void get_line(int client_fd, char line[], int size)
{
    char a = 'a';
    int i = 0;
    ssize_t s = 0;

    while(i < size - 1 && a != '\n')
	{
        s = recv(client_fd, (void*)&a, 1, 0);
        
		if(s > 0)
		{
            if(a == '\r')
			{
                recv(client_fd, &a, 1, MSG_PEEK);
                
				if(a == '\n')
				{
                    recv(client_fd, &a, 1, 0);
                }
				else
				{
                    a = '\n';
                }
            }

            line[i] = a;
            i++;
        }
		else
		{
            break;
        } // end of if (s > 0)
    } // end of while(i < size-1 && a != '\n')

    line[i] = '\0';
}

void clean_headers(int sock)
{
    char line[MAX] = { 0 };

    while(strcmp(line, "\n") != 0)
	{
        get_line(sock, line, sizeof(line));
    }
}

static int cgi_exe(int sock, char* method, char path[], char* argv_string)
{
	// cgi 程序的逻辑
    char line[MAX] = { 0 };
    int content_length = -1;
    char method_env[MAX / 32];// 设置 method 的环境变量
    char argv_string_env[MAX];// 设置 GET 方法的参数的环境变量
    char content_length_env[MAX / 16];// 设置 POST 方式的数据长度的环境变量

    if(strcasecmp(method,"GET") == 0)
	{
		// GET
        clean_headers(sock);
    }
	else
	{
		// POST
        while(strcmp(line, "\n") != 0)
		{
			// 判断是否读到空行
            get_line(sock, line, sizeof(line));
            
			if(strncmp(line, "Content-Length: ", 16) == 0)
			{
				//读取实体长度
                content_length = atoi(line + 16);
            }
        }

        if(content_length == -1)
		{
            return 404;
        }
    }
    
    printf("content_length:%d\n", content_length);

    sprintf(line, "HTTP/1.0 200 OK\r\n");// 响应报头
    send(sock, line, strlen(line), 0);
    sprintf(line, "Content-Type:text/html\r\n");// 响应报头
    send(sock, line, strlen(line), 0);
    sprintf(line, "\r\n");//空行
    send(sock, line, strlen(line), 0);

    int input[2];
    int output[2];

    pipe(input);//创建管道
    pipe(output);

    pid_t id = fork();//因为处理请求是多线程的版本，所以没有办法直接进行线程替换，只能先让线程创建子进程来处理 cgi 程序
    if (id < 0)
	{
        return 404;
    }
	else if (id == 0)
	{
		//子进程，用来进行cgi程序
        close(input[1]);
        close(output[0]);

        dup2(input[0], 0);//重定向输入
        dup2(output[1], 1);//重定向输出

        sprintf(method_env, "METHOD_ENV=%s", method);
        putenv(method_env);//添加环境变量，进程替换不会影响环境变量

        if (strcasecmp(method, "GET") == 0)
		{
			//GET方法的话需要添加参数的环境变量
            sprintf(argv_string_env, "ARGV_STRING_ENV=%s", argv_string);
            putenv(argv_string_env);
        }
		else
		{
			//POST方法的话需要添加参数长度的环境变量
            sprintf(content_length_env, "CONTENT_LENGTH_ENV=%d", content_length);
            putenv(content_length_env);
        }
        
        execl(path, path, NULL);
        exit(1);
    }
	else
	{
		//父进程，进行等待子进程
        close(input[0]);
        close(output[1]);

        char a = 'a';
        if (strcasecmp(method, "POST") == 0)
		{
			//如果是POST方法的话，我们得把数据参数写入到管道中，以便子进程读取
            int i = 0;
            for (; i<content_length; i++)
			{
                read(sock, &a, 1);
                write(input[1], &a, 1);
            }
        }

        
        while(read(output[0], &a, 1) > 0)
		{
            send(sock, &a, 1, 0);
        }

        waitpid(id, NULL, 0);
        close(input[1]);
        close(output[0]);
    }

    return 200;
}

void echo_www(int sock, char* path, int size, int* err)
{
    // 写回首页时侯，我们得把 sock 中的请求报头信息全部读完.
    clean_headers(sock);

    int fd = open(path, O_RDONLY);// 以只读的信息打开 path 文件
    //int fd = open("./wanroot/cgi2/data.txt",O_RDONLY);

    if(fd < 0)
	{
        *err = 404;
        return;
    }
    
	char line[MAX] = { 0 };

    // 写回响应报头
    printf("fd:%d size:%d\n", fd, size);
    sprintf(line, "HTTP/1.0 200 OK\r\n");// 响应报头
    send(sock, line, strlen(line), 0);
    sprintf(line, "Content_Type: text/html\r\n");//响应报头
    send(sock, line, strlen(line), 0);
    sprintf(line, "\r\n");//空行
    send(sock, line, strlen(line), 0);
    sendfile(sock, fd, NULL, size);//发送文件
    close(fd);
}

void send_err_home(int sock)
{
    clean_headers(sock);
    
	struct stat st;
    stat(ERR_HOME, &st);
    int fd = open(ERR_HOME,O_RDONLY);
    char line[MAX] = { 0 };

    sprintf(line, "HTTP/1.0 404 NotFound\r\n");
    send(sock, line, strlen(line), 0);
    sprintf(line, "Content_Type: text/html\r\n");
    send(sock, line, strlen(line), 0);
    sprintf(line, "\r\n");
    send(sock, line, strlen(line), 0);
    sendfile(sock, fd, NULL, st.st_size);
    close(fd);
}

void echo_err(int errCode, int sock)
{
    switch(errCode)
	{
		case 404:
		    send_err_home(sock);
			break;
		default:
			break;
    }
}

void* root(void* client_fd)
{
    int sock = *(int *)client_fd;
    char line[MAX] = { 0 };
    char path[MAX];
    char* method = NULL;
    char* url = NULL;
    char* argv_string = NULL;
    char* pro_Ver = NULL;
    int errCode = 200;
    int cgi_flag = 0;
//#define aa1
#ifdef aa1
    do
	{
        get_line(sock, line, sizeof(line));
        printf("%s", line);
    } while(strcmp(line, "\n") != 0);
#else
    get_line(sock, line, sizeof(line));
    printf("%s", line);
//    if(line[0] == 0)
//    {
//        errCode = 404;
//        goto end;
//    }
    size_t i = 0;
    method = line;
    
	//get method
    while (i < sizeof(line) && !isspace(line[i]))
	{
        i++;
    }

    line[i] = '\0';
    if (strcasecmp(method, "GET") == 0)
	{
        ;
    }
	else if (strcasecmp(method, "POST") == 0)
	{
        cgi_flag = 1;
    }
	else
	{
        errCode = 404;
        goto end;
    }
    
    //get url
    url = &line[i + 1];
    while (i < sizeof(line) && !isspace(line[i]))
	{
        if (line[i] == '?')
		{
			//get argv_string
            line[i] = '\0';
            cgi_flag = 1;
            argv_string = &line[i+1];
        }

        i++;
    }

    line[i] = '\0';

    //get pro_Ver
    pro_Ver = &line[i + 1];
    while (i < sizeof(line) && !isspace(line[i]))
	{
        i++;
    }
    // printf("method:%s\n",method);
    // printf("url:%s\n",url);
    // printf("argv_string:%s\n",argv_string);
    // printf("pro_Ver:%s",pro_Ver);

    sprintf(path, "wanroot%s", url);
    printf("path:%s\n", path);

    if (path[strlen(path) - 1] == '/')
	{
		// 判断路径是否以'/'结尾，用来判断路径是否是目录
        strcat(path, HOME);// 如果路径是以目录结尾，则拼接路径后面的目录首页
    }
    
	struct stat st;
    int ret = stat(path, &st);// stat 函数用来查询路径所在文件是否存在，存在则 st 结构体返回文件信息，否则返回 -1.
    if (ret < 0)
	{
        errCode = 404;
        goto end;
    }
	else
	{
        if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP)\
                ||(st.st_mode & S_IXOTH))
		{
			//直接判断路径是否具有执行的权限.如果是则进入 cgi 程序,否则写回首页.
            cgi_flag = 1;
        }

        printf("method:%s,path:%s\n", method, path);
        if(cgi_flag == 1)
		{
			// 判断cgi标志位是否置位
            errCode = cgi_exe(sock, method, path, argv_string);// 走cgi程序
        }
		else
		{
            echo_www(sock, path, st.st_size, &errCode);//发送首页
        }
    }

#endif
end:
    if(errCode != 200)
	{
        //printf("method:%s,path:%s\n", method, path);
        echo_err(errCode, sock);
    }
    close(sock);

    return NULL;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
	{
        printf("please enter: %s [port]\n", argv[0]);
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);
    int listen_sock = sockup(atoi(argv[1]));//创建监听套接字

    for (;;)
	{
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(listen_sock, (struct sockaddr*)&client_addr, &len);//进程accept函数

        if (client_fd < 0)
		{
            perror("accept");
            continue;
        }

        pthread_t ret;
        pthread_create(&ret, NULL, root, &client_fd);//创建线程
        pthread_detach(ret);
    }

    return 0;
}
