#include <glib.h>
#include <stdio.h>
#include <stdarg.h>

#include "trace.h"

/* temporary logging facilities */


static char indent[] = "                                                 ";
static int level = 48;

void 
trace_enter  (
              char * func
              )
{
  printf ("%s%s () {\n", indent+level, func);
  level -= 2;
}


void 
trace_exit  (
             void
             )
{
  level += 2;
  printf ("%s}\n", indent+level);
}


#if 0
void 
trace_begin  (
              char * format,
              ...
              )
{
  va_list args;
  char *buf;
    
  va_start (args, format);
  buf = g_strdup_vprintf (format, args);
  va_end (args);

  fputs (indent+level, stdout);
  fputs (buf, stdout);
  fputs (": {\n", stdout);

  level -= 2;
}
#endif

void 
trace_begin  (
              char * format,
              ...
              )
{
  va_list args;
  char *buf;
    
  va_start (args, format);
  buf = g_strdup_vprintf (format, args);
  va_end (args);

  fputs (indent+level, stdout);
  fputs (buf, stdout);
  fputs (": \n", stdout);
  fputs (indent+level, stdout);
  fputs ("{\n", stdout);

  level -= 2;
}

void 
trace_end  (
            void
            )
{
  level += 2;
  printf ("%s}\n", indent+level);
}


void 
trace_printf  (
               char * format,
               ...
               )
{
  va_list args;
  char *buf;

  va_start (args, format);
  buf = g_strdup_vprintf (format, args);
  va_end (args);

  fputs (indent+level, stdout);
  fputs (buf, stdout);
}


  

