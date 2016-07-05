#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv) {
   int f = open("/etc/passwd", O_RDONLY);
   char buf[10];
   read(f, buf, 10);
   buf[9] = 0;
   if (buf[0] == '#') {
       printf("is #\n");
   } else {
       printf("not #\n");
   }
}
