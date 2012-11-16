#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

/* network */
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#define DEBUG 1

#define F_CONNECTING          1
#define F_READING             2
#define F_DONE                4

#define     GET_CMD           "GET /%s HTTP/1.0\r\nHost: localhost\r\n\r\n"

struct file {
      int f_flags;
      int f_fd;
      int status;
      char *f_name;
};

int         nconn, nfiles, nlefttoconn, nlefttoread, maxfd;
fd_set      rset, wset;
char hostname[] = "localhost";

ssize_t
Read(int fd, void *ptr, size_t nbytes)
{
      ssize_t           n;

      if ( (n = read(fd, ptr, nbytes)) == -1)
            fprintf(stderr, "read error\n");
      return(n);
}

static inline void loadBar(int x, int n, int r, int w)
{
    // Only update r times.
    if ( x % (n/r) != 0 ) return;
 
    // Calculuate the ratio of complete-to-incomplete.
    float ratio = x/(float)n;
    int   c     = ratio * w;
 
    // Show the percentage complete.
    printf("%3d%% [", (int)(ratio*100) );
 
    // Show the load bar.
    for (x=0; x<c; x++)
       printf("=");
 
    for (x=c; x<w; x++)
       printf(" ");
 
    // ANSI Control codes to go back to the
    // previous line and clear it.
      printf("]\033[F\033[J");
}

void debug_printf(char *msg)
{
      if(!DEBUG)
            fprintf(stderr, "%s\n", msg);
}

int count_lines(FILE *f)
{
      int i, ch, lines = 0;

      for(;;) {
            ch = getc(f);
            if(ch == '\n')
                  lines++;
            if(ch == EOF) {
                  lines++;
                  break;  
            }         
      }
      return lines;
}

int read_line(FILE *f, char *buf){
      int ch, i = 0;
      for(;;) {
            ch = getc(f);
            if(ch =='\n' || ch == EOF) {
                  buf[i] = '\0';
                  return i;
            }
            else
                  buf[i++] = ch;
      }
}
struct file *init_pool()
{     
      FILE *fp;
      int chars, i = 0;
      char buf[4096], *bufptr;
      struct file *p;
      fp = fopen("small.txt", "r");
      if(!fp){
            fprintf(stderr, "Couldn\'t open the dictonary file\n");
            exit(0);
      }
      nfiles = count_lines(fp);

      if(!nfiles) {
            fprintf(stderr, "Dictonary is empty\n");
            exit(0);
      }

      p = malloc(sizeof(struct file) * nfiles);
      fseek(fp, 0L, SEEK_SET);

      while((chars = read_line(fp, buf)) != 0) {
            bufptr = malloc(sizeof(char) * chars);
            memcpy(bufptr, buf, chars);
            p[i].f_flags = 0;
            p[i].status = 0;

            p[i++].f_name = bufptr;
            // fprintf(stderr, "[%d] f_flags %d, f_name %s\n", i, p[i-1].f_flags, p[i-1].f_name);
      }
      FD_ZERO(&rset);
      FD_ZERO(&wset);
      return p;
}

void write_get_command(struct file *p)
{
      int n;
      char line[1024];
      n = snprintf(line, sizeof(line), GET_CMD, p->f_name);
   write(p->f_fd, line, n);
   debug_printf("write GET");
   p->f_flags = F_READING;
   FD_SET(p->f_fd, &rset);
   if(p->f_fd > maxfd)
      maxfd = p->f_fd;
}


void start_connect(struct file *fptr)
{
      int fd, flags, n;
      struct sockaddr_in sin;
      struct hostent *h;

      h = gethostbyname(hostname);
      if(!h) {
            fprintf(stderr, "Coulndt resolv %s\n", hostname);
            exit(0);
      }

      fd = socket(AF_INET, SOCK_STREAM, 0);
      if (fd < 0){
            perror("socket");
            exit(0);
      }
      sin.sin_family = AF_INET;
      sin.sin_port = htons(80);
      sin.sin_addr = *(struct in_addr *)h->h_addr;
      fptr->f_fd = fd;
      // fprintf(stderr, "start_connect for %s, fd %d\n", hostname, fd);

      /* set non-block */
      flags = fcntl(fd, F_GETFL, 0);
      fcntl(fd, F_SETFL, flags | O_NONBLOCK);

      if ( n = connect(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            if (errno != EINPROGRESS)
                  fprintf(stderr, "nonblocking connect error\n");
            fptr->f_flags = F_CONNECTING;
            FD_SET(fd, &rset);
            FD_SET(fd, &wset);
            if(fd > maxfd)
                  maxfd = fd;
      } else if (n >= 0) // connect if already done
            write_get_command(fptr); // write the GET command to http serv
}

int parse_answer(char *buf)
{

      // char str[] = "HTTP/1.", *ptr;
      // ptr = strstr((char *)b, (char *)str);
      // if(!ptr) {
      //       fprintf(stderr, "strange answer from the server\n");
      //       return -1;
      // }
      // printf("%s in %s\n", ptr, b);
      FILE *fptr;
      fptr = fopen("log.txt", "a+");
      fwrite(buf, sizeof(char), strlen(buf), fptr);
      fclose(fptr);

      if((strncmp(buf+9, "200", 3)) == 0) {
            return 200;
      } else if (strncmp(buf+9, "404", 3) == 0) {
            return 0;
      } else if (strncmp(buf+9, "403", 3) == 0) {
            return 403;
      } else if (strncmp(buf+9, "301", 3) == 0) { 
            printf("%s", buf+9);
            return 301;
      }
      return -1;
}

int main(int argc, char **argv)
{
      
      if(argc < 3) {
            fprintf(stderr, "usage: %s http://host.com <#conn>\n", argv[0]);
            exit(0);
      }

      int maxconn, i, fd, flags, error, n, status, total;
      fd_set rs, ws;
      char buf[1024];

      maxconn = atoi(argv[2]);
      struct file *files; /* create array of files to check */

      files = init_pool();

      maxfd = -1;
      nlefttoread = nlefttoconn = total = nfiles;
      nconn = 0;

      while(nlefttoread > 0) {
            while(nconn <maxconn && nlefttoconn > 0) {
                  for(i = 0; i < nfiles; i++) {
                        if(files[i].f_flags == 0) {
                              break;
                        }
                  }
                  if(i == nfiles)
                        fprintf(stderr, "nlefttoconn = %d, but nothing found\n", nlefttoconn);
                  start_connect(&files[i]);
                  nconn++;
                  nlefttoconn--;
            }

            rs = rset;
            ws = wset;

            n = select(maxfd + 1, &rs, &ws, NULL, NULL);
            for(i = 0; i < nfiles; i++) {
                  flags = files[i].f_flags;
                  if(flags == 0 || flags & F_DONE)
                        continue;
                  fd = files[i].f_fd;
                  if(flags & F_CONNECTING && (FD_ISSET(fd, &rs) || FD_ISSET(fd, &ws))) {
                        n = sizeof(error);
                        if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &n) < 0 || error != 0) {
                              fprintf(stderr, "nonblocking connect failed\n");
                        }
                        // fprintf(stderr, "connection established\n");
                        FD_CLR(fd, &wset);
                        write_get_command(&files[i]);
                  } else if (flags & F_READING && FD_ISSET(fd, &rs)) {
                        if((n = Read(fd, buf, sizeof(buf))) == 0) {
                              close(fd);
                              files[i].f_flags = F_DONE;
                              status = parse_answer(buf);
                              if(status != 0){
                                  fprintf(stderr, "\n[%d] Server response %d\n", i, status);
                                    files[i].status = status;
                              }
                              FD_CLR(fd, &rset);
                              nconn--;
                              nlefttoread--;
                        } else {
                               fprintf(stderr, "read %d bytes\n", n);
                        }
                  }
            }
            loadBar(total - nlefttoread, 1000, 100, 50);
            fflush(stdout);
            printf("\r");
      }

      for(i = 0; i < nfiles; i++) {
            // if (!(i % 100))
                  // printf("\n%d\n", i);
            if(files[i].status != 0) {
                  fprintf(stderr, "\n[%d][nfiles %d] Found %s with response code %d", 
                                    i, nfiles, files[i].f_name, files[i].status);
            }
      }
      // printf("\n[%d] found %s and status %d\n", 865, files[865].f_name, files[865].status);
      return 0;
}