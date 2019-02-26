#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "mysql/include/mysql.h"

#define MAX 1024  // 接收环境变量的缓冲区大小

int main()
{
  char buf[MAX];
  int i = 0;

  int len = atoi(getenv("CONTENT-LENGTH"));
  char c;

  for (; i < len; ++i)
  {
    read(0, &c, 1);
    buf[i] = c;
  }
  buf[i] = '\0';
    
  printf("%s\n", buf);

  return 0;
}
