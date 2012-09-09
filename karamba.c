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
      k->target_url = (char *) cmd;
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
      // strcpy(dir_ptr, buffer);
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
  char *url;
  // long http_code;

  // CURL *curl;
  // CURLcode response;

  // init curl and set 301 follow
  // curl = curl_easy_init();
  // curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,1);


  printf("Print all 10 elements...\n");
  while (!IsEmpty(Q)) {
    dir_ptr = Front(Q);
    if(dir_ptr != NULL) {
      // url = malloc(strlen(k->target_url) + strlen(dir_ptr) + sizeof(char) * 2);
      // strcat(url, k->target_url);
      // strcat(url, "/");
      // strcat(url, dir_ptr);
      // strcat(url, "/");

      // printf("%s\n", url);
      // free(url);
      // url = "http://google.com/";
      // curl_easy_setopt(curl, CURLOPT_URL,url);
      // response = curl_easy_perform(curl);
      // curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    }
    Dequeue(Q);
    free(dir_ptr);
  }
}

int main(int argc, char const *argv[])
{
  struct Config *karamba_conf = malloc(sizeof(struct Config));
  assert(karamba_conf != NULL);
  handle_options(&argv, &argc, karamba_conf);

  // set fseek at the begining of the world list
  karamba_conf->position = 0;

  if(karamba_conf->target_url == NULL || karamba_conf->dictonary == NULL) {
    printf("usage: %s\n", karamba_usage_string);
    return 1;
  }
  
  Queue Q;

  while(karamba_conf->position >= 0)
  {
    Q = queue_add(karamba_conf);
    scan(Q, karamba_conf);
  }

  printf("Position is : %ld\n", karamba_conf->position);
  printf("\n\n");
  printf("Dispose of the queue...\n");
  DisposeQueue(Q);

  free(karamba_conf);
  return 0;
}