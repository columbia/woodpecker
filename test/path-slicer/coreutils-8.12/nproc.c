/* nproc - print the number of processors.
   Copyright (C) 2009-2011 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Giuseppe Scrivano.  */

#include "config.h"
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>

//#include "system.h"
//#include "error.h"
#include "nproc.h"
//#include "xstrtol.h"
#include <limits.h>
#include <stdarg.h>
#define _(arg) arg
#define proper_name(x) (x)

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "nproc"
#define program_name PROGRAM_NAME

#define AUTHORS proper_name ("Giuseppe Scrivano")

#define case_GETOPT_VERSION_CHAR(Program_name, Authors)			\
  case GETOPT_VERSION_CHAR:						\
    version_etc3 (stdout, Program_name, PACKAGE_NAME, "Version", Authors,	\
                 (char *) NULL);					\
    exit (EXIT_SUCCESS);						\
    break;

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define case_GETOPT_HELP_CHAR			\
  case GETOPT_HELP_CHAR:			\
    usage (EXIT_SUCCESS);			\
    break;

enum
{
  GETOPT_HELP_CHAR = (CHAR_MIN - 2),
  GETOPT_VERSION_CHAR = (CHAR_MIN - 3)
};

#define GETOPT_HELP_OPTION_DECL \
  "help", no_argument, NULL, GETOPT_HELP_CHAR
#define GETOPT_VERSION_OPTION_DECL \
  "version", no_argument, NULL, GETOPT_VERSION_CHAR
#define GETOPT_SELINUX_CONTEXT_OPTION_DECL \
  "context", required_argument, NULL, 'Z'

//enum
//{
  //ALL_OPTION = CHAR_MAX + 1,
  //IGNORE_OPTION
//};

static struct option const longopts[] =
{
  //{"all", no_argument, NULL, ALL_OPTION},
  //{"ignore", required_argument, NULL, IGNORE_OPTION},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

extern const char version_etc_copyright[];
enum { COPYRIGHT_YEAR = 2011 };

extern void version_etc_arn (FILE *stream,
                             const char *command_name, const char *package,
                             const char *version,
                             const char * const * authors, size_t n_authors);

/* Names are passed in the NULL-terminated array AUTHORS.  */
extern void version_etc_ar (FILE *stream,
                            const char *command_name, const char *package,
                            const char *version, const char * const * authors);

/* Names are passed in the NULL-terminated va_list.  */
extern void version_etc_va (FILE *stream,
                            const char *command_name, const char *package,
                            const char *version, va_list authors);

/* Names are passed as separate arguments, with an additional
   NULL argument at the end.  */
extern void version_etc (FILE *stream,
                         const char *command_name, const char *package,
                         const char *version,
                         /* const char *author1, ..., NULL */ ...);

/* Display the --version information the standard way.

   Author names are given in the array AUTHORS. N_AUTHORS is the
   number of elements in the array. */
void
version_etc_arn (FILE *stream,
                 const char *command_name, const char *package,
                 const char *version,
                 const char * const * authors, size_t n_authors)
{

  fprintf(stream, "version_etc_arn haha\n");
  return;
  if (command_name)
    fprintf (stream, "%s (%s) %s\n", command_name, package, version);
  else
    fprintf (stream, "%s %s\n", package, version);

#ifdef PACKAGE_PACKAGER
# ifdef PACKAGE_PACKAGER_VERSION
  fprintf (stream, _("Packaged by %s (%s)\n"), PACKAGE_PACKAGER,
           PACKAGE_PACKAGER_VERSION);
# else
  fprintf (stream, _("Packaged by %s\n"), PACKAGE_PACKAGER);
# endif
#endif

  /* TRANSLATORS: Translate "(C)" to the copyright symbol
     (C-in-a-circle), if this symbol is available in the user's
     locale.  Otherwise, do not translate "(C)"; leave it as-is.  */
  fprintf (stream, version_etc_copyright, _("(C)"), COPYRIGHT_YEAR);

  fputs (_("\
\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
"),
         stream);

  switch (n_authors)
    {
    case 0:
      /* The caller must provide at least one author name.  */
      abort ();
    case 1:
      /* TRANSLATORS: %s denotes an author name.  */
      fprintf (stream, _("Written by %s.\n"), authors[0]);
      break;
    case 2:
      /* TRANSLATORS: Each %s denotes an author name.  */
      fprintf (stream, _("Written by %s and %s.\n"), authors[0], authors[1]);
      break;
    case 3:
      /* TRANSLATORS: Each %s denotes an author name.  */
      fprintf (stream, _("Written by %s, %s, and %s.\n"),
               authors[0], authors[1], authors[2]);
      break;
    case 4:
      /* TRANSLATORS: Each %s denotes an author name.
         You can use line breaks, estimating that each author name occupies
         ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      fprintf (stream, _("Written by %s, %s, %s,\nand %s.\n"),
               authors[0], authors[1], authors[2], authors[3]);
      break;
    case 5:
      /* TRANSLATORS: Each %s denotes an author name.
         You can use line breaks, estimating that each author name occupies
         ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      fprintf (stream, _("Written by %s, %s, %s,\n%s, and %s.\n"),
               authors[0], authors[1], authors[2], authors[3], authors[4]);
      break;
    case 6:
      /* TRANSLATORS: Each %s denotes an author name.
         You can use line breaks, estimating that each author name occupies
         ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      fprintf (stream, _("Written by %s, %s, %s,\n%s, %s, and %s.\n"),
               authors[0], authors[1], authors[2], authors[3], authors[4],
               authors[5]);
      break;
    case 7:
      /* TRANSLATORS: Each %s denotes an author name.
         You can use line breaks, estimating that each author name occupies
         ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      fprintf (stream, _("Written by %s, %s, %s,\n%s, %s, %s, and %s.\n"),
               authors[0], authors[1], authors[2], authors[3], authors[4],
               authors[5], authors[6]);
      break;
    case 8:
      /* TRANSLATORS: Each %s denotes an author name.
         You can use line breaks, estimating that each author name occupies
         ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      fprintf (stream, _("\
Written by %s, %s, %s,\n%s, %s, %s, %s,\nand %s.\n"),
                authors[0], authors[1], authors[2], authors[3], authors[4],
                authors[5], authors[6], authors[7]);
      break;
    case 9:
      /* TRANSLATORS: Each %s denotes an author name.
         You can use line breaks, estimating that each author name occupies
         ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      fprintf (stream, _("\
Written by %s, %s, %s,\n%s, %s, %s, %s,\n%s, and %s.\n"),
               authors[0], authors[1], authors[2], authors[3], authors[4],
               authors[5], authors[6], authors[7], authors[8]);
      break;
    default:
      /* 10 or more authors.  Use an abbreviation, since the human reader
         will probably not want to read the entire list anyway.  */
      /* TRANSLATORS: Each %s denotes an author name.
         You can use line breaks, estimating that each author name occupies
         ca. 16 screen columns and that a screen line has ca. 80 columns.  */
      fprintf (stream, _("\
Written by %s, %s, %s,\n%s, %s, %s, %s,\n%s, %s, and others.\n"),
                authors[0], authors[1], authors[2], authors[3], authors[4],
                authors[5], authors[6], authors[7], authors[8]);
      break;
    }
}

/* Display the --version information the standard way.  See the initial
   comment to this module, for more information.

   Author names are given in the NULL-terminated array AUTHORS. */
void
version_etc_ar (FILE *stream,
                const char *command_name, const char *package,
                const char *version, const char * const * authors)
{
  size_t n_authors;

  for (n_authors = 0; authors[n_authors]; n_authors++)
    ;
  version_etc_arn (stream, command_name, package, version, authors, n_authors);
}

/* Display the --version information the standard way.  See the initial
   comment to this module, for more information.

   Author names are given in the NULL-terminated va_list AUTHORS. */
void
version_etc_va (FILE *stream,
                const char *command_name, const char *package,
                const char *version, va_list authors)
{
  size_t n_authors;
  const char *authtab[10];

  /*for (n_authors = 0;
       n_authors < 10
         && (authtab[n_authors] = va_arg (authors, char *)) != NULL;
       n_authors++)
    ;*/
  version_etc_arn (stream, command_name, package, version,
                   authtab, n_authors);
}


/* Display the --version information the standard way.

   If COMMAND_NAME is NULL, the PACKAGE is assumed to be the name of
   the program.  The formats are therefore:

   PACKAGE VERSION

   or

   COMMAND_NAME (PACKAGE) VERSION.

   The authors names are passed as separate arguments, with an additional
   NULL argument at the end.  */
void
version_etc3 (FILE *stream,
             const char *command_name, const char *package,
             const char *version, /* const char *author1, ...*/ ...)
{
  va_list authors;

  va_start (authors, version);
  version_etc_va (stream, command_name, package, version, authors);
  va_end (authors);
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
             program_name);
  else
    {
      printf (_("Usage: %s [OPTION]...\n"), program_name);
      fputs (_("\
Print the number of processing units available to the current process,\n\
which may be less than the number of online processors\n\
\n\
"), stdout);
      fputs (_("\
     --all       print the number of installed processors\n\
     --ignore=N  if possible, exclude N processing units\n\
"), stdout);

      /*fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_ancillary_info ();*/
    }
  exit (status);
} 

int
main (int argc, char **argv)
{
  unsigned long nproc, ignore = 0;
  /*initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);*/

  //atexit (close_stdout);

  enum nproc_query mode = NPROC_CURRENT_OVERRIDABLE;

  while (1)
    {
      int c = getopt_long (argc, argv, "", longopts, NULL);
      if (c == -1)
        break;
      switch (c)
        {
        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
/*
        case ALL_OPTION:
          mode = NPROC_ALL;
          break;

        case IGNORE_OPTION:
          if (xstrtoul (optarg, NULL, 10, &ignore, "") != LONGINT_OK)
            {
              error (0, 0, _("%s: invalid number to ignore"), optarg);
              usage (EXIT_FAILURE);
            }
          break;
*/
        default:
          usage (EXIT_FAILURE);
        }
    }

  //nproc = num_processors (mode);
/*
  if (ignore < nproc)
    nproc -= ignore;
  else
    nproc = 1;

  printf ("%lu\n", nproc);
*/
  exit (EXIT_SUCCESS);
}

// Tesing purpose: PHI_DEF_BETWEEN, from those switch statements.

// Testing commands:
// RUN: %srcroot/common-scripts/build-bc.sh %s
// RUN: %srcroot/common-scripts/link-libs.sh %s.bc
// RUN: %srcroot/common-scripts/klee-opt.sh %s.bc

// RUN: %kleebindir/klee --max-time 3600 --libc=uclibc --posix-runtime --init-env --use-interleaved-covnew-NURS \
// RUN: --use-batching-search --batch-instructions=10000  --use-random-path \
// RUN: --only-output-states-covering-new --disable-inlining --use-one-checker=File --use-path-slicer=1 --mark-pruned-only=0 \
// RUN: %s.bc --sym-files 2 8  --max-fail 1  --sym-args 0 3 3              2> %s.output

// RUN: cat %s.output | FileCheck %s
   
 
// Expected results: 

// Note: This testcase makes sure this fwrite() event in version_etc3() function is reached.

// CHECK: Stat::printEventCalls exed [No-Loc]: F: version_etc3: BB: return:   %1 = call i64 bitcast (i64 (i8*, i64, i64, %6*)* @fwrite_unlocked to i64 (i8*, i64, i64, i8*)*)(i8* getelementptr inbounds ([22 x i8]* @.str2, i64 0, i64 0), i64 1, i64 21, i8* %0) nounwind ; <i64> [#uses=0]


