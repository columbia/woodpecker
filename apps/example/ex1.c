
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(int argc, char **argv) {
 FILE *input_desc;
 int argind = 1;
 const char *infile = "-";
 do { // iterate over input files and print one by one
   if(argind < argc)
     infile = argv[argind];
   if(strcmp(infile, "-")) // input is a file
     input_desc = fopen(infile, "r");
   else // input is @stdin@
     input_desc = stdin;
   assert(input_desc);
   int c;
   while((c = fgetc(input_desc)) != EOF) {
     if(c < 32 && c != '\n') { // non-printable char
       putchar('^');
       putchar(c + 64);
     } else // printable char
       putchar(c);
   }
   if(infile[0] != '-' || infile[1] != 0)
     fclose(input_desc); // input is a file
 } while (++argind < argc);
 return 0;
}
