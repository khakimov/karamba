#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

int powerof(int x, int y)
{
  int i = 1, sum =1;
  for(; i<=y; i++)
    sum = sum * x;
  return sum;
}

int asciitohex(char *s)
{
  int n = strlen(s) - 2;
  int result = 0, h = 0, asciint;

  if (*s++ == '0')
    if(*s++ == 'x')
      while(*s != '\0'){
        // printf("n is %d and res %d\n",n, result);
        asciint = *s - '0';
        if ((*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F'))
          asciint = (*s > 'F') ? *s - 'a' + 10: *s - 'A' + 10;
        if (n == 1)
          result += asciint;
        else
          result += asciint * powerof(16, --n);
        *s++;
      }
  return result;
}

int main(int argc, char *argv[])
{
   struct in_addr in;
   if (argc <= 1)
   {
      printf("Usage: ./hex2dd 0x8002c2f2\n");
      return(1);
   }
   int hex = asciitohex(argv[1]);

   if (hex == 0)
   {
    printf("Error: wasn't hex number\n");
    printf("Usage: ./hex2dd 0x8002c2f2\n");
    return 1;
   }
   printf("htonl %u\n", htonl(hex));
   printf("ip address as int %d\n", hex);
   in.s_addr = htonl(hex);
   printf("%s\n", inet_ntoa(in));
   return 0;
}
