#include <stdio.h>   /* required for file operations */
#include <string.h>

FILE *fr;            /* declare the file pointer */

int main(int argc, char *argv[])

{
   char line[80];
   int n = 0;
   int numColon = 0;
   if (argc != 2)
     return -1;

   fr = fopen (argv[1], "rt");  /* open the file for reading */
   /* elapsed.dta is the name of the file */
   /* "rt" means open the file for reading text */

   while(fgets(line, 80, fr) != NULL)
   {
	 /* get a line, up to 80 chars from fr.  done if NULL */
	 //sscanf (line, "%ld", &elapsed_seconds);
	 /* convert the string to a long int */
	 printf ("line[%d]:<%s>\n", n, line);
     if (strstr(line, ":"))
       numColon++;
     n++;
   }
   fclose(fr);  /* close the file prior to exiting the routine */
   fprintf(stderr, "numColon: %d\n", numColon);
   return 0;
} /*of main*/

/*
When use path slicer, real prune, this command can finish in roughly 75 seconds, which shows
the effectiveness of path slicer. It can support 2000 bytes of symbolic files.
| ex2.c.bc | OpenClose | Real | *Yes* | 45.1049 | 1.82487 | 11.3785 | 1389 | 1407 | 3 | 419979 | *419979* | 345 | 41 | 42 | 2 | 2 | 0 | tbd | 

time klee --max-time 3600 --libc=uclibc --posix-runtime --init-env --randomize-fork \
--use-random-path --use-interleaved-covnew-NURS --use-batching-search \
--batch-instructions=10000 --only-output-states-covering-new --disable-inlining \
--use-one-checker=OpenClose --use-path-slicer=1 --mark-pruned-only=0 ex2.c.bc \
--sym-args 0 3 2 --sym-files 2 2000 --max-fail 1

If we use mark prune, it totally won't finish.

*/
