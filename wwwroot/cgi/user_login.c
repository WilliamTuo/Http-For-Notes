#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include "mysql/include/mysql.h"

#define MAX 1024  // 读取到服务器传过来的参数的缓冲区

char* RunDB(const char* name, const char *password)
{
  // 初始化数据库
  MYSQL *mysql_fd = mysql_init(NULL);

  if (mysql_real_connect(mysql_fd, "127.0.0.1",\
        "root", "tuoyinhang", "cloud_note_user", 3306, NULL, 0) == NULL)
  {
    printf("connect faild!\n");
    mysql_close(mysql_fd);
    exit(1);
  }

  char sql_query[1024]; // 保存 sql 语句
  sprintf(sql_query, "select * from user_table where user_name=\"%s\" and password=\"%s\";", name, password);
  
  mysql_query(mysql_fd, sql_query);

  // 从返回结果中过滤出我们要的结果
  MYSQL_RES *res = mysql_store_result(mysql_fd);

  if (res)
  {
      char *user_id = (char *)malloc(sizeof(int) + 1); 

      MYSQL_ROW rowData = mysql_fetch_row(res);
  
      // 如果查找到用户，则第 0 行第 0 列就是用户 id
      strcpy(user_id, (char *)rowData[0]);

      mysql_close(mysql_fd);
      return user_id;
  }

  mysql_close(mysql_fd);
  return NULL;
}

int main()
{
  int log_fd = open("./log.txt", O_RDWR);

  int len = atoi(getenv("CONTENT-LENGTH"));
  if (len == 0)
  {
    write(log_fd, "atoi error");
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
  
  //printf("<html>%s<\/html>", name);
  //printf("<html>%s<\/html>", password);
  
  char *user_id = RunDB(name, password);
  if (user_id != NULL)
  {
    int success_fd = open("../users_home/success.html", O_RDONLY);

    struct stat st;
    if (stat("../users_home/success.html", &st) < 0)
    {
      sprintf(log_fd, "stat error\n");
    }

    int size = st.st_size;
    printf("%d\n", size);
    sendfile(1, success_fd, NULL, size);
  }
  else
  {
    int failed_fd = open("../users_home/failed.html", O_RDONLY);

    struct stat st;
    stat("../users_home/faild.html", &st);

    int size = st.st_size;
    sendfile(1, failed_fd, NULL, size);
  }


  return 0;
}

