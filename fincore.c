/* fincore.c - report which blocks of a file are in the page buffer cache
   Copyright (C) 1991-1997, 1999-2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Martin A. Brown <mabrown@renesys.com> */

#include <config.h>

#include <stdio.h>     /* perror, fprintf, stderr, printf */
#include <errno.h>
#include <limits.h>
#include <getopt.h>
#include <fcntl.h>     /* fcntl, open */
#include <stdlib.h>    /* exit, calloc, free */
#include <string.h>    /* strerror */
#include <unistd.h>    /* sysconf, close */
#include <sys/stat.h>  /* stat, fstat */
#include <sys/types.h> /* size_t */
#include <sys/mman.h>  /* mincore */

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "fincore"
#define AUTHORS "Martin A. Brown"

#define FINC_NO_HEADER        ( 1 << 0 )
#define FINC_QUELL            ( 1 << 1 )
#define FINC_SUMMARY          ( 1 << 2 )
#define FINC_LIST_PAGES       ( 1 << 3 )
#define FINC_SUMMARY_ONLY     ( FINC_NO_HEADER | FINC_QUELL | FINC_SUMMARY )

struct a_options {
  char                   *files_from;
  int                     reporting_mode;
  unsigned long           total_pages;
  unsigned long           total_incore;
  unsigned long           total_files;
};

struct a_options o = {
     .files_from      = NULL,
     .reporting_mode  = 0,
     .total_pages     = 0,
     .total_incore    = 0
};

/* Usage Macros */
#define MSG( D, F, ...) fprintf( D, F , ##__VA_ARGS__ )
#define FATAL( F, ...) { MSG( stderr, F, ##__VA_ARGS__ ); exit( EXIT_FAILURE ); }
#define USAGE_FATAL( F, ... ) { short_usage( EXIT_FAILURE ); FATAL( F, ##__VA_ARGS__ ); }


/* The name this program was run with. */
char *program_name;
char *program_btime     = __DATE__" "__TIME__;
char *program_copyright = "copyright Renesys"  ;
const float program_version = 0.10f;


void
short_usage(int ret)
{
  fprintf( ( ret ? stderr : stdout ),
    "usage: %s [ OPTIONS ] [ <file> ... ]\nTry \"%s --help\" for more.\n",
    program_name, program_name );
}

void
long_usage(int ret)
{
  short_usage( ret );
  fprintf( ( ret ? stderr : stdout ),
    "\n"
    "Options (defaults in parentheses):\n"
    "  -h, --help, --usage       display this help and exit.\n"
    "  -V, --version             display version information and exit.\n"
    "  -v, --verbose             produce listing of blocks/pages in core\n"
    "  -f, --files-from <file>   read list of files from <file>\n"
    "  -N, --no-header           suppress initial header line\n"
    "  -s, --summary             include a summary at the bottom\n"
    "  -S, --summary-only        produce only totals\n"
    "\n"
         );
  exit( ret );
}

void
print_version(int ret)
{
  fprintf( ( ret ? stderr : stdout ), "%s\n%s-%.2f (compiled %s)\n",
           program_copyright, program_name, program_version, program_btime );
  exit( ret );
}

int
parse_options(int ac, char **av)
{
  extern char *optarg;
  extern int optind;
  int c;

  static struct option const longopts[] =
  {
    {"no-header", no_argument, NULL, 'N'},
    {"files-from", required_argument, NULL, 'f'},
    {"summary", no_argument, NULL, 's'},
    {"summary-only", no_argument, NULL, 'S'},
    {"verbose", no_argument, NULL, 'v'},
    {"version", no_argument, NULL, 'V'},
    {"help", no_argument, NULL, 'h'},
    {"usage", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

  opterr = 0; /* Keep getopt quiet. */

  while ( -1 != (c = getopt_long (ac, av, "+hVvNSsf:", longopts, NULL)) ) 
  {
    switch (c)
    {
      case '?': USAGE_FATAL( "Unrecognized option: \"%s\"\n", 
                             av[optind - 1] );                       break;
      case 'f': o.files_from      = optarg;                          break;
      case 'S': o.reporting_mode  = FINC_SUMMARY_ONLY;               break;
      case 'N': o.reporting_mode |= FINC_NO_HEADER;                  break;
      case 's': o.reporting_mode |= FINC_SUMMARY;                    break;
      case 'v': o.reporting_mode |= FINC_LIST_PAGES;                 break;
      case 'h': long_usage( EXIT_SUCCESS );                          break;
      case 'V': print_version( EXIT_SUCCESS );                       break;
      default: USAGE_FATAL("%s", "Should not have arrived here.");   break;
    }
  }
  return optind;
}

/* fincore -
 */
void
fincore(char *filename)
{
   int fd;
   float percentage;
   unsigned long inCore;
   unsigned long pageCount;
   struct stat st;
   void *pa = (char *)0;
   unsigned char *vec = (unsigned char *)0;
   //register size_t n = 0;
   size_t pageSize = getpagesize();
   register size_t pageIndex;

   if (0 != stat(filename, &st)) {
      fprintf(stderr, "skipping, fstat(%s): %s\n", filename, strerror(errno));
      return;
   }

   if (!S_ISREG(st.st_mode)) {
      fprintf(stderr, "skipping, non-regular file %s\n", filename);
      return;
   }

   fd = open(filename, 0);
   if (0 > fd) {
      fprintf(stderr, "skipping, open(%s): %s\n", filename, strerror(errno));
      return;
   }

   pa = mmap((void *)0, st.st_size, PROT_NONE, MAP_SHARED, fd, 0);
   if (MAP_FAILED == pa) {
      fprintf(stderr, "skipping, mmap(0, %lu, PROT_NONE, MAP_SHARED, %d, 0): %s\n",
              (unsigned long)st.st_size, fd, strerror(errno));
      close(fd);
      return;
   }

   pageCount = (st.st_size+pageSize-1)/pageSize;
   vec = calloc(1, pageCount);
   if ((void *)0 == vec) {
      fprintf(stderr, "skipping, calloc(1, %lu): %s\n",
              pageCount, strerror(errno));
      close(fd);
      return;
   }

   if (0 != mincore(pa, st.st_size, vec)) {
      /* perror("mincore"); */
      fprintf(stderr, "skipping, mincore(%p, %lu, %p): %s\n",
              pa, (unsigned long)st.st_size, vec, strerror(errno));
      free(vec);
      close(fd);
      return;
   }

   /* prepare a few simple calculations */
   inCore = 0;
   for (pageIndex = 0; pageIndex < pageCount; pageIndex++) {
      if (vec[pageIndex]&1) inCore++;
   }
   o.total_files++;
   o.total_pages += pageCount;
   o.total_incore += inCore;

   /* produce some output */
   if (! ( o.reporting_mode & FINC_QUELL ) )
   {
       percentage = inCore / pageCount;
       printf("%s %lu %lu %.2f", 
         filename, pageCount, inCore, percentage );

       if (o.reporting_mode & FINC_LIST_PAGES)
       {
         for (pageIndex = 0; pageIndex < pageCount; pageIndex++) {
            if (vec[pageIndex]&1) {
               printf(" %lu", (unsigned long)pageIndex);
            }
         }
       }
       printf("\n");
   }

   free(vec);
   vec = (unsigned char *)0;

   munmap(pa, st.st_size);
   close(fd);
   return;
}

int
main (int argc, char **argv)
{
  int argn;

  program_name = PROGRAM_NAME; /* argv[0]; */
  argn = parse_options( argc, argv );

  if (! ( o.reporting_mode & FINC_NO_HEADER ) )
  {
    printf("File  Size  Pages  Percent%s\n", 
      o.reporting_mode & FINC_LIST_PAGES ? " Details" : "");
  }

  while ( argn < argc )
  {
    fincore(argv[argn]);
    argn++;
  }

  if ( o.reporting_mode & FINC_SUMMARY )
  {
    printf("Total files: %lu\nTotal pages: %lu\nTotal in core:%lu\n", 
      o.total_files, o.total_pages, o.total_incore);
  }
  exit( EXIT_SUCCESS );
}

/* end of file */
