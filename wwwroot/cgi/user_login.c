#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include "mysql/include/mysql.h"

#define MAX 1024      // 读取到服务器传过来的参数的缓冲区
#define MAXPATH 1024  // 给定路径长度最大值

char err_desc[1024];  // 记录程序执行错误的信息

unsigned long long RunDB(const char* name, const char *password)
{
  // 初始化数据库
  MYSQL *mysql_fd = mysql_init(NULL);

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

  char sql_query[1024]; // 保存 sql 语句
  sprintf(sql_query, "select user_id from user_table where user_name=\"%s\" and password=\"%s\"", name, password);
  
  if (mysql_query(mysql_fd, sql_query) != 0)
  {
    fprintf(stderr, "mysql_query error:\n");
    mysql_close(mysql_fd);
    return 0;
  }
  else
  {
    //fprintf(stderr, "select\n");
    // 从返回结果中过滤出我们要的结果
    // 该函数即使查询失败返回值也不为空
    MYSQL_RES *res = mysql_store_result(mysql_fd);
    if (res)
    {
      MYSQL_ROW rowData = mysql_fetch_row(res);
      fprintf(stderr, "%s\n", rowData[0]);
      unsigned long long user_id;
      char *endptr = NULL;
      user_id = strtoull(rowData[0], &endptr, 10); 
      fprintf(stderr, "%lld\n", user_id);

      mysql_free_result(res);
      mysql_close(mysql_fd);

      return user_id;
    }
  }

  mysql_close(mysql_fd);
  return 0;
}

int main()
{
  int len = atoi(getenv("CONTENT-LENGTH"));
  if (len == 0)
  {
    fprintf(stderr, "user_login.c: atoi failed\n");
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

  //printf("<html>%s<\/html>", buf);
  
  char *name = NULL;
  char *password = NULL;

  strtok(buf, "=&");
  name = strtok(NULL, "=&");
  strtok(NULL, "=&");
  password = strtok(NULL, "=&");
  
  //printf("<html>%s</html>", name);
  //printf("<html>%s</html>", password);
  
  //fprintf(stderr, "%s\n", name);
  unsigned long long user_id = RunDB(name, password);
  fprintf(stderr, "after select: %lld\n", user_id);
  
  //char path[MAXPATH]; 
  //getcwd(path, MAXPATH);
  //printf("%s\n", path);
  //strcat(path, "../users_home/success.html");
  if (user_id)
  {
    char suffix[5];
    char user_index[MAXPATH];
    sprintf(suffix, "html");
    sprintf(user_index, "./wwwroot/users_notes/%s/%s.%s", name, name, suffix);
    struct stat st;
    if (stat(user_index, &st) < 0)
    {
      fprintf(stderr, "stat error in user_login.c\n");
    }

    // 打开文件并发给浏览器
    int user_fd = open(user_index, 'r');
    if (user_fd < 0)
    {
      fprintf(stderr, "open user_fd failed in user_index_init(have exist)\n");
    }

    sendfile(1, user_fd, NULL, st.st_size);

    close(user_fd);

    // 测试登录成功的页面
    //int success_fd = open("./wwwroot/users_home/success.html", O_RDONLY);
    //if (success_fd < 0)
    //{
    //  fprintf(stderr, "open success.html error\n");
    //  exit(2);
    //}

    //struct stat st;
    //if (fstat(success_fd, &st) < 0)
    //{
    //  fprintf(stderr, "stat success_fd error\n");
    //  exit(3);
    //}

    //int size = st.st_size;
    ////printf("%d\n", size);
    //sendfile(1, success_fd, NULL, size);
    // close(success_fd);
  }
  else
  {
    fprintf(stderr, "failed\n");
    int failed_fd = open("./wwwroot/users_home/failed.html", O_RDONLY);
    if (failed_fd < 0)
    {
      fprintf(stderr, "open failed_fd error\n");
      exit(4);
    }

    struct stat st;
    if (fstat(failed_fd, &st) < 0)
    {
      fprintf(stderr, "stat failed.html error\n");
      exit(5);
    }

    int size = st.st_size;
    sendfile(1, failed_fd, NULL, size);

    close(failed_fd);
  }

  return 0;
}

