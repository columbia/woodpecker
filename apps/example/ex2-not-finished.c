#include <stdio.h>

int streq(const char *s1, const char *s2) {
 while(*s1 == *s2 && *s1 != 0) {
   ++s1; ++s2;
 }
 return (*s1 - *s2) == 0;
}

int main(int argc, char **argv) {
 const char *infile = "-";
 FILE *input_desc;
 int argind = 1;
 do { // iterate over input files and cat them one by one
   if(argind < argc)
     infile = argv[argind];
   if(streq(infile, "-"))  // cat from @stdin@
     input_desc = stdin;
   else // cat from a file
     input_desc = fopen(infile, "r");
   int c;
   while((c = fgetc(input_desc)) != EOF) {
     if(c < 32 && c != '\n') { // non-printable char
       putchar('^');
       putchar(c+64);
     } else // printable char
       putchar(c);
   }
   if(!streq(infile, "-"))
     fclose(input_desc);
 } while (++argind < argc);
 return 0;
}
