/* bounded.c                                                              */
/* Code for Producer/Consumer problem using mutex and condition variables */
/* To compile me for Unix, type:  gcc -o filename filename.c -lpthread */

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <curl/curl.h>

#define BSIZE 200


typedef struct {
  int *buf[BSIZE];
  int occupied;
  int nextin, nextout;
  int total;
  size_t done;
  pthread_mutex_t mutex;
  pthread_cond_t more;
  pthread_cond_t less;
} buffer_t;

buffer_t buffer;

#define RIO_BUFSIZE 8192
typedef struct {
   int rio_fd;
   int rio_cnt;
   char *rio_bufptr;
   char rio_buf[RIO_BUFSIZE];
} rio_t;

void * producer(void *);
void * consumer(void *);


char *hostname;
int debug = 0;

void usage(char *pname)
{
      fprintf(stderr, "\nUsage: %s http://target.com <num threads>\n", pname);
      exit(0);      
}

main( int argc, char *argv[] ) 
{
      int i, workers;

      if (argc < 3) {
            usage(argv[0]);
      }
      
      if(!strncmp(argv[1], "http://", 7)) {
            hostname = (char *) (argv[1]) + 7;
            if(debug) printf("Target is %s\n", hostname);
      }
      else
            usage(argv[0]);

      workers = atoi(argv[2]);
      
      if (!workers) usage(argv[0]);


      if(debug) printf("Total threads: %d\n", workers);
      pthread_t tid[workers];      /* array of thread IDs */

      pthread_cond_init(&(buffer.more), NULL);
      pthread_cond_init(&(buffer.less), NULL);


      for(i = 1; i < workers + 1; i++) {
            pthread_create(&tid[i], NULL, consumer, NULL);
      }
      pthread_create(&tid[0], NULL, producer, NULL);


      for ( i = 0; i < workers; i++)
            pthread_join(tid[i], NULL);

      printf("\nmain() reporting that all %d threads have terminated\n", i);

}  /* main */

  

void * producer(void * parm)
{

  if(debug) printf("producer started.\n");

      FILE *fp;
      int *p[1000], i = 0;

      buffer.done = 0;
      buffer.total = 0;
      
      char *buf = malloc(sizeof(char) * 4096);
      fp = fopen("small.txt", "r");

      while((fgets(buf, 4096, fp)) != NULL)
      {
            pthread_mutex_lock(&(buffer.mutex));

            if (buffer.occupied >= BSIZE) 
                  if(debug) printf("producer waiting.\n");

            while (buffer.occupied >= BSIZE)
                  pthread_cond_wait(&(buffer.less), &(buffer.mutex) );

            if(debug) printf("producer executing.\n");
            buf[strlen(buf) - 1] = '\0';
            buffer.buf[buffer.nextin++] = (int *)buf;
            buf = malloc(sizeof(char) * 4096);
            buffer.nextin %= BSIZE;
            buffer.occupied++;
            pthread_cond_signal(&(buffer.more));
            pthread_mutex_unlock(&(buffer.mutex));
      }


  //     /* now: either buffer.occupied < BSIZE and buffer.nextin is the index
  //  of the next empty slot in the buffer, or
  //  buffer.occupied == BSIZE and buffer.nextin is the index of the
  //  next (occupied) slot that will be emptied by a consumer
  //  (such as buffer.nextin == buffer.nextout) */
  if(debug) printf("producer exiting.\n");
  buffer.done = 1;
  pthread_exit(0);
}

void *get_header(char *url)
{
      int clientfd, port, i;
      char buf[4096];
      rio_t rio;
      // TODO: as a parameter
      port = 80;

      clientfd = open_clientfd(hostname, port);
      rio_readinitb(&rio, clientfd);
      if(debug) printf("Connect to %s/%s\n", hostname,url);

      char *cmd;
      cmd = malloc(28 + strlen(url) + strlen(hostname));
      sprintf(cmd, "HEAD /%s/ HTTP/1.1\nHost: %s\n\n", url, hostname);
      write(clientfd, cmd, strlen(cmd));
      free(cmd);

      while(1)
      {
            i = rio_readlineb(&rio, buf, 4096);

            if (i <= 2)
                  break;
            if((strncmp(buf+9, "200", 3)) == 0 || (strncmp(buf+9, "403", 3)) == 0)
                  fprintf(stderr, "Found http://%s/%s\n", hostname, url);
            // save_url(hostname, url);
            // printf("%s\n", buf);
      }
      close(clientfd);
}

void * consumer(void * parm)
{
      char *item;

      if(debug) printf("consumer started.\n");

      while(1)
      {
            pthread_mutex_lock(&(buffer.mutex) );
            if (buffer.done == 1 && buffer.occupied <= 0) break;
            if (buffer.occupied <= 0) 
                  if(debug) printf("consumer waiting. waiting [%d]\n", buffer.occupied);

            while(buffer.occupied <= 0)
                  pthread_cond_wait(&(buffer.more), &(buffer.mutex) );

            if(debug) printf("consumer executing. occupied [%d]\n", buffer.occupied);

            item = (char *) buffer.buf[buffer.nextout++];
            if(debug) printf("[%ld] %s", (long)pthread_self(), item);
            buffer.nextout %= BSIZE;
            buffer.occupied--;
            buffer.total++;
            /* now: either buffer.occupied > 0 and buffer.nextout is the index
            of the next occupied slot in the buffer, or
            buffer.occupied == 0 and buffer.nextout is the index of the next
            (empty) slot that will be filled by a producer (such as
            buffer.nextout == buffer.nextin) */

            pthread_cond_signal(&(buffer.less));
            pthread_mutex_unlock(&(buffer.mutex));
            if(item) get_header(item);
            fprintf(stderr,"%d\r", buffer.total);
            fflush(stdout);

      }
      if(debug) printf("consumer exiting.\n");
      pthread_exit(0);
}


