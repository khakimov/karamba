#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>

#define RIO_BUFSIZE 8192
typedef struct {
   int rio_fd;
   int rio_cnt;
   char *rio_bufptr;
   char rio_buf[RIO_BUFSIZE];
} rio_t;

int main(int argc, char **argv)
{
  int clientfd, port, i, url;
  char *host, buf[4096];
  rio_t rio;
  if(argc < 2)
  {
    fprintf(stderr, "usage: ./eclient <host> <port>");
    exit(0);
  }
  host = argv[1];
  port = atoi(argv[2]);

  for(url = 0; url < 10; url++)
  {
    clientfd = open_clientfd(host, port);
    rio_readinitb(&rio, clientfd);

    char cmd[50];
    sprintf(cmd, "HEAD /%d.html HTTP/1.1\nHost: trainme.ru\n\n", url);
    write(clientfd, cmd, strlen(cmd));

    while(1)
    {
      i = rio_readlineb(&rio, buf, 4096);
      if (i <= 2)
        break;
      if((strncmp(buf+9, "200 OK", 6)) == 0)
        printf("YAU! Got it!");
      printf("%s\n", buf);
    }
  }
  return 0;
}