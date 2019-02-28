#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#define MAXPATH 1024

int main()
{
  int user_fd;    // 用来记录用户根目录的文件描述符
  char suffix[6];
  
  strcpy(suffix, "html");
  char *user_name = getenv("USER-NAME");
  char user_index[MAXPATH];
  sprintf(user_index, "wwwroot/users_notes/%s/%s.%s", user_name, user_name,suffix);

  
  fprintf(stderr, "user_index = %s\n", user_index);
  

  // 以用户名为文件名创建用户的根目录
  if (access(user_index, 0) == 0)
  {
    // 文件已经存在
    fprintf(stderr, "File already exist!\n");
  
    //struct stat st;
    //if (stat(user_index, &st) < 0)
    //{
    //  fprintf(stderr, "stat error in user_index_init\n");
    //}

    // 打开文件并发给浏览器
    //user_fd = open(user_index, 'r');
    //if (user_fd < 0)
    //{
    //  fprintf(stderr, "open user_fd failed in user_index_init(have exist)\n");
    //}

    //sendfile(1, user_fd, NULL, st.st_size);
  }
  else
  {
    umask(0);
    user_fd = creat(user_index , 0644);
    if (user_fd < 0)
    {
      fprintf(stderr, "create file error\n");
      //fprintf(stderr, "create file error: %s\n", strerror(errno));
    }

    // 文件已经创建好了，给用户指定一个默认的目录页面
    // 默认目录网页是 user_name.html 
    // 做法是：将 准备好的 index.html 拷贝给 user_name.html 文件
    FILE *pfd1 = fopen(user_index, "w");
    if (pfd1 == NULL)
    {
      fprintf(stderr, "fopen user_index error\n");
    }

    FILE *pfd2 = fopen("./wwwroot/users_notes/index.html", "r");
    if (pfd2 == NULL)
    {
      fprintf(stderr, "fopen user_index error\n");
    }

    char c;
    int size = 0;
    while ((c = fgetc(pfd2)) != EOF)
    {
      fputc(c, pfd1);  
      size++;
    }

    //sendfile(1, user_fd, NULL, size);

    fclose(pfd1);
    fclose(pfd2);
  }  // end of if(access(user_index, 0) == 0)

  return 0;
}
