#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>

int open_listenfd(int port);
ssize_t rio_readn(int fd, void *usrbuf, size_t n);

#define RIO_BUFSIZE 8192
typedef struct {
   int rio_fd;
   int rio_cnt;
   char *rio_bufptr;
   char rio_buf[RIO_BUFSIZE];
} rio_t;

void echo(int connfd)
{
  size_t n;
  char buf[4096];
  rio_t rio;

  rio_readinitb(&rio, connfd);
  while((n = rio_readlineb(&rio, buf, 4096)) != 0)
  {
    printf("server received %d bytes\n", (int)n);
    printf("data: %s\n", buf);
  }
}

int main()
{
  int listenfd, connfd, port, clientlen;
  struct sockaddr_in clientaddr;
  struct hostent *hp;
  char *haddrp;

  port = 1110;
  listenfd = open_listenfd(port);
  while(1)
  {
    clientlen = sizeof(clientaddr);
    connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);

    hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    haddrp = inet_ntoa(clientaddr.sin_addr);
    printf("server connected to %s (%s)\n", hp->h_name, haddrp);
    char buf[10];

    echo(connfd);
    close(connfd);
  }

  return 0;
}