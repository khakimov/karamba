#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
   char **pp;
   struct in_addr addr;
   struct hostent *hostp;

   if (argc != 2)
   {
      fprintf(stderr, "usage: %s <domain name of dotted-decimal>\n", argv[0]);
      exit(0);
   }

   if(inet_aton(argv[1], &addr) != 0)
      hostp = gethostbyaddr((const char *)&addr, sizeof(addr), AF_INET);
   else
      hostp = gethostbyname(argv[1]);

   printf("official hostname: %s\n", hostp->h_name);

   for(pp = hostp->h_aliases; *pp != NULL; pp++)
      printf("alias: %s\n", *pp);

   for(pp = hostp->h_addr_list; *pp != NULL; pp++)
   {
      addr.s_addr = *((unsigned int*) *pp);
      printf("address: %s\n", inet_ntoa(addr));
   }
   exit(0);
}
