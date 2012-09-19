#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
   if (argc < 2)
   {
      printf("Usage: ./dd2hex 128.2.194.242\n");
      return 1;
   }
   struct in_addr in;
   inet_aton(argv[1], &in);
   printf("0x%x\n", htonl(in.s_addr));
   return 0;
}
