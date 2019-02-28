#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/sendfile.h>

#include "mysql/include/mysql.h"

#define MAX 10240  // 读取到服务器传过来的参数的缓冲区

#define MAXPATH 1024      // 读取到服务器传过来的参数的缓冲区

unsigned long long RunDB(const char* name, const char *password)
{
  char sql_query[1024]; // 保存 sql 语句

  // 初始化数据库
  MYSQL *mysql_fd = mysql_init(NULL);
  if (mysql_fd == NULL)
  {
    fprintf(stderr, "user_register.c: mysql init error\n");
    mysql_close(mysql_fd);
    return 0;
  }

  if (mysql_real_connect(mysql_fd, "127.0.0.1",\
        "root", "tuoyinhang", "cloud_note_user", 3306, NULL, 0) == NULL)
  {
    printf("connect faild!\n");
    mysql_close(mysql_fd);
    return 0;
  }

  // 设置客户端字符编码为 utf8
  if (mysql_set_character_set(mysql_fd, "utf8") != 0)
  {
    fprintf(stderr, "user_register: set character error\n");
    mysql_close(mysql_fd);
    return 0;
  }


  // 1.查看用户名是否已经存在
  memset(sql_query, 0x00, sizeof(sql_query));
  sprintf(sql_query, "select user_name from user_table where user_name=\"%s\"", name);
  
  fprintf(stderr, "%s\n", sql_query);
  if (mysql_query(mysql_fd, sql_query) != 0)
  {
    // 执行成功，说明用户已经存在，返回失败结果
    fprintf(stderr, "user_register.c: mysql_query error(1)\n");
    mysql_close(mysql_fd);
    return 0;
  }
  
  MYSQL_RES *select_res_1 = mysql_store_result(mysql_fd);
  mysql_free_result(select_res_1);

  fprintf(stderr, "user_register: user_name is available!\n");
  
  // 2. 如果用户名不存在，可以将用户名和密码插入到用户表中
  // 程序到这里说明用户名未使用，可以注册成为新用户
  // 将用户名和密码插入到用户表中
  // fprintf(stderr, "user_register: before insert\n");
  memset(sql_query, 0x00, sizeof(sql_query));
  sprintf(sql_query, "insert into user_table(user_name,password) values(\"%s\", \"%s\")", name, password);
  
  fprintf(stderr, "user_register: mysql_query: %s\n", sql_query);
  mysql_query(mysql_fd, sql_query);
 
  MYSQL_RES *insert_res = mysql_store_result(mysql_fd);
  mysql_free_result(insert_res);

  fprintf(stderr, "user_register: mysql_query insert success\n");
      
  // 3.插入成功之后，获取 user_id 并返回给主调函数
  my_ulonglong user_id = mysql_insert_id(mysql_fd);

  mysql_close(mysql_fd);
  return user_id;
}

int main()
{
  int len = atoi(getenv("CONTENT-LENGTH"));
  if (len < 0)
  {
    fprintf(stderr, "user_register: atoi error\n");
    exit(1);
  }

  char buf[MAX];
  int i = 0;
  char ch;

  for (; i < len; i++)
  {
    read(0, &ch, 1);
    buf[i] = ch;
  }
  
  char *name = NULL;
  char *password = NULL;

  strtok(buf, "=&");
  name = strtok(NULL, "=&");
  strtok(NULL, "=&");
  password = strtok(NULL, "=&");
  
  //printf("<html>%s</html>", name);
  //printf("<html>%s</html>", password);
  
  fprintf(stderr, "user_register: before RunDB\n");
  unsigned long long user_id = RunDB(name, password);
  if (user_id)
  {
    char user_path[MAXPATH];
    fprintf(stderr, "user_register: register success\n");
    sprintf(user_path, "./wwwroot/users_notes/%s", name);
    
    // 使用 access 用来判断指定文件是否存在
    // access 返回值，如果存在为 0 ，否则返回 -1
    if (access(user_path, 0) == -1)
    {
      umask(0);
      if (mkdir(user_path, 0755) < 0)
      {
        fprintf(stderr, "mkdir error\n");
        exit(2);
      }
    }
    else
    {
      fprintf(stderr, "Directory is exist!\n");
      exit(3);
    }

    pid_t id = fork();
    if (id < 0)
    {
      fprintf(stderr, "fork error in user_login.c");
      exit(4);
    }
    else if (id == 0)
    {
      // 将用户名通过环境变量传给 exec 进程
      char userName[30];
      sprintf(userName, "USER-NAME=%s", name);
      putenv(userName);
      //char path[MAXPATH]; 
      //getcwd(path, MAXPATH);
      //fprintf(stderr, "%s\n", path);
      char user_index_init[MAXPATH];
      strcpy(user_index_init, "./wwwroot/cgi/user_index_init");
      execl(user_index_init, user_index_init, NULL);
    }
    else
    {
      waitpid(id, NULL, 0); 
    }
    
    // 用户注册成功
    int success_fd = open("./wwwroot/users_home/register_success.html", 'r');
    if (success_fd < 0)
    {
      fprintf(stderr, "open register_success.html failed\n");
      exit(5);
    }

    struct stat st;
    if (stat("./wwwroot/users_home/register_success.html", &st) < 0)
    {
      fprintf(stderr, "stat register_success.html failed\n");
      exit(6);
    }

    int size = st.st_size;
    sendfile(1, success_fd, NULL, size);
    
    fprintf(stderr, "sendfile ok!\n");

    close(success_fd);
  }
  else
  {
    fprintf(stderr, "user_register: register failed\n");
    int failed_fd = open("wwwroot/users_home/register_failed.html", 'r');
    if (failed_fd < 0)
    {
      fprintf(stderr, "user_resgister.c: open error\n");
    }

    struct stat st;
    if (stat("wwwroot/users_home/register_failed.html", &st) < 0)
    {
      fprintf(stderr, "user_resgister.c: stat failed.html error\n");
    }

    int size = st.st_size;
    sendfile(1, failed_fd, NULL, size);

    close(failed_fd);
  }

  return 0;
}

