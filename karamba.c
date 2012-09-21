#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <curl/curl.h>
#include "queue.h"

const char karamba_usage_string[] =
  "karamba [-h] [-d dictonary filename] [http[s]://]hostname[:port]/";

// struct with config
struct Config {
  char *target_url;
  char *dictonary;
  long int position;
};

#define RIO_BUFSIZE 8192
typedef struct {
   int rio_fd;
   int rio_cnt;
   char *rio_bufptr;
   char rio_buf[RIO_BUFSIZE];
} rio_t;

static int handle_options(const char ***argv, int *argc, struct Config *k)
{  
  while (*argc > 0) {
    const char *cmd = (*argv)[0];


    if(!strcmp(cmd, "--help") || !strcmp(cmd, "-h") || !strcmp(cmd, "-?"))
    {
      printf("Karamba version 0.0\n");
      printf("usage: %s\n", karamba_usage_string);
      break;
    }

    if(!strcmp(cmd, "-d"))
    {
      // const char *dictonary = (*argv)[1];
      k->dictonary = (char *) (*argv)[1];
      (*argv)++;
      (*argc)--;
    }

    if(!strncmp(cmd, "http://", 7))
    {
      const char *target = cmd;
      k->target_url = (char *) cmd+7;
    }

    (*argc)--;
    (*argv)++;
  }
}

Queue queue_add(struct Config *k)
{
    // QUEUE
  Queue Q;
  int i = 0; 
  char *dir_ptr;
  char *buffer = (char*) malloc(4096 * sizeof(char));

  printf("\n");
  printf("Create Queue(4096)...\n\n");
  Q = CreateQueue(500);

  if(k->dictonary) {

    printf("[DICT] %s\n", k->dictonary);
    
    FILE *fp;
    
    fp = fopen(k->dictonary, "r");
    
    if(!fp) {
      printf("ERROR: cannot open dictionary, quitting..\n");
      exit(1);
    }
    fseek(fp, k->position, SEEK_SET);
    // while (!feof(fp)) {
    while(i++ < 500) {
      if (fgets(buffer, 4096*sizeof(char),fp) == NULL)
      {
        if(feof(fp))
        {
          printf("The end of the file!\n");
          free(buffer);
          printf("\n");
          k->position = -1;
          return Q;        
        }
      }  
      printf("%s", buffer);
      dir_ptr = malloc((int) strlen(buffer) + 1);
      strncpy(dir_ptr, buffer, strlen(buffer) - 1);
      Enqueue(dir_ptr, Q);
    }
    printf("Current position in the file : %ld\n", ftell(fp));
    k->position = ftell(fp);
  }

  free(buffer);
  printf("\n");
  return Q;
}


void scan(Queue Q, struct Config *k)
{
  char *dir_ptr;

  while (!IsEmpty(Q)) {
    dir_ptr = Front(Q);

    if(dir_ptr != NULL) {
      // ADD: Threads
      int header = get_header(k->target_url, dir_ptr);

    }

    Dequeue(Q);
    free(dir_ptr);
  }
}

void save_url(char *hostname, char *folder)
{
  FILE *fd;
  fd = fopen("/tmp/scan.txt", "a+");
  fprintf (fd, "%s/%s\n", hostname, folder);
  fclose(fd);
}

int get_header(char *hostname, char *url)
{
  int clientfd, port, i;
  char buf[4096];
  rio_t rio;
  // TODO: as a parameter
  port = 80;

  clientfd = open_clientfd(hostname, port);
  rio_readinitb(&rio, clientfd);
  printf("Connect to %s/%s\n", hostname,url);
  
  char *cmd;
  cmd = malloc(28 + strlen(url) + strlen(hostname));
  sprintf(cmd, "HEAD /%s/ HTTP/1.1\nHost: %s\n\n", url, hostname);
  write(clientfd, cmd, strlen(cmd));
  free(cmd);
  // FIX: bad style. infinity loop!
  while(1)
  {
    i = rio_readlineb(&rio, buf, 4096);
    if (i <= 2)
      break;
    if((strncmp(buf+9, "200", 3)) == 0 || (strncmp(buf+9, "403", 3)) == 0)
      save_url(hostname, url);
    printf("%s\n", buf);
  }
  close(clientfd);
  return 0;
}



int main(int argc, char const *argv[])
{
  struct Config karamba_conf;

  // if less 2 arg then exit
  if (argc < 2){
    printf("usage: %s\n", karamba_usage_string);
    exit(1);
  }

  handle_options(&argv, &argc, &karamba_conf);

  // set fseek at the begining of the world list
  karamba_conf.position = 0;

  if(karamba_conf.target_url == NULL || karamba_conf.dictonary == NULL) {
    printf("usage: %s\n", karamba_usage_string);
    return 1;
  }

  Queue Q;

  while(karamba_conf.position >= 0)
  {
    Q = queue_add(&karamba_conf);
    scan(Q, &karamba_conf);
  }

  printf("Position is : %ld\n", karamba_conf.position);
  printf("\n\n");
  printf("Dispose of the queue...\n");
  
  DisposeQueue(Q);

  return 0;
}