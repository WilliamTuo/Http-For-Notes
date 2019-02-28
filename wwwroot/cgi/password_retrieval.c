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


void RunDB(const char* name, char **password)
{
  // 初始化数据库
  MYSQL *mysql_fd = mysql_init(NULL);

  if (mysql_real_connect(mysql_fd, "127.0.0.1",\
        "root", "tuoyinhang", "cloud_note_user", 3306, NULL, 0) == NULL)
  {
    printf("connect faild!\n");
    mysql_close(mysql_fd);
  }

  // 设置客户端字符编码为 utf8
  if (mysql_set_character_set(mysql_fd, "utf8") != 0)
  {
    fprintf(stderr, "user_register: set character error\n");
    mysql_close(mysql_fd);
  }

  char sql_query[1024]; // 保存 sql 语句
  sprintf(sql_query, "select password from user_table where user_name=\"%s\"", name);
  
  fprintf(stderr, "mysql_query: %s\n", sql_query);
  if (mysql_query(mysql_fd, sql_query) != 0)
  {
    fprintf(stderr, "mysql_query error:\n");
    mysql_close(mysql_fd);
  }
  else
  {
    fprintf(stderr, "after mysql_query\n"); 
    //fprintf(stderr, "select\n");
    // 从返回结果中过滤出我们要的结果
    // 该函数即使查询失败返回值也不为空
    MYSQL_RES *res = mysql_store_result(mysql_fd);
    fprintf(stderr, "after mysql_store_result\n"); 
    if (res)
    {
      if (!mysql_field_count(mysql_fd))
      {
        MYSQL_ROW rowData = mysql_fetch_row(res);
        fprintf(stderr, "row Data: %s\n", rowData[0]);

        strcpy(*password, rowData[0]);
        fprintf(stderr, "%s\n", *password);

        mysql_free_result(res);
      }
    }
    fprintf(stderr, "after select\n"); 
  }

  mysql_close(mysql_fd);
}

int main()
{
  int len = atoi(getenv("CONTENT-LENGTH"));
  if (len == 0)
  {
    fprintf(stderr, "password_retrieval.c: atoi failed\n");
    exit(1);
  }

  fprintf(stderr, "%d\n", len);
  char buf[MAX];
  int i = 0;
  char ch;

  for (; i < len; i++)
  {
    read(0, &ch, 1);
    buf[i] = ch;
  }

  char *name = NULL;

  strtok(buf, "=&");
  name = strtok(NULL, "=&");
  
  fprintf(stderr, "before\n");
  char *password = (char *)calloc(32,0);
  RunDB(name, &password);
  fprintf(stderr, "after\n");
  fprintf(stderr, "%s\n", password);
  
  if (password == 0)
  {
    printf("<html>");
    printf("<meta charset=\"utf-8\">"); 
    printf("<h2>您的密码找到了!</h2>"); 
    printf("<br>"); 
    printf("<span>");
    printf("密码是：%s", password);
    printf("</span>");
    printf("</html>");
  }
  else
  {
    printf("<html>");
    printf("<meta charset=\"utf-8\">"); 
    printf("<h2>查找失败，确认用户名输入是否有误!</h2>"); 
    printf("<br>"); 
    printf("<div><a href=\"http://60.205.188.244:8080/users_home/password_retrieval.html\">点击返回重新查找</a></div>");
    printf("</html>");
  }

  free(password);
  return 0;
}

