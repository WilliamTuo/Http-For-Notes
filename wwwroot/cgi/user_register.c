#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include "mysql/include/mysql.h"

#define MAX 10240  // 读取到服务器传过来的参数的缓冲区

int RunDB(const char* name, const char *password, char *user_id)
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
  sprintf(sql_query, "insert into user_table(user_name,password) values(\"%s\", \"%s\"))", name, password);
  
  mysql_query(mysql_fd, sql_query);

  // 从返回结果中过滤出我们要的结果
  MYSQL_RES *res = mysql_store_result(mysql_fd);
  int col = mysql_num_fields(res);
  MYSQL_ROW rowData = mysql_fetch_row(res);
  
  int j = 0;
  for (j = 0; j < col; ++j)
    printf("%s\n", rowData[j]);

  // 判断是否查找到用户
  if (strncasecmp((char*)rowData, "ERROR", 5) == 0)
  {
    // 没有找到，直接返回
    mysql_close(mysql_fd);
    return 0;
  }
  else
  {
    sprintf(sql_query, "select user_name, password from user_table\
        where user_name=%s and password=%s\n", name, password);
    mysql_query(mysql_fd, sql_query);

    MYSQL_RES *res = mysql_store_result(mysql_fd);
    MYSQL_ROW rowData = mysql_fetch_row(res);

    rowData = mysql_fetch_row(res);
    strcpy(user_id, (char *)rowData[0]);
  }

  mysql_close(mysql_fd);

  return 1;
}

int main()
{
  int len = atoi(getenv("CONTENT-LENGTH"));

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
  
  //printf("<html>%s<\/html>", name);
  //printf("<html>%s<\/html>", password);
  char *user_id = NULL;
  if (RunDB(name, password, user_id) == 1)
  {
    int success_fd = open("../users_home/register_success.html", O_RDWR);

    struct stat st;
    stat("../users_home/register_success.html", &st);

    int size = st.st_size;
    sendfile(1, success_fd, NULL, size);

    char *user_dir = "../users_notes";
    strstr(user_dir, user_id);
    open("../users_notes", O_CREAT); 
  }
  else
  {
    int failed_fd = open("../users_home/register_failed.html", O_RDWR);

    struct stat st;
    stat("../users_home/register_faild.html", &st);

    int size = st.st_size;
    sendfile(1, failed_fd, NULL, size);
  }

  return 0;
}

