/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "gegl-options.h"

static GeglOptions *opts_new (void)
{
  GeglOptions *o = g_malloc0 (sizeof (GeglOptions));

  o->mode     = GEGL_RUN_MODE_DISPLAY;
  o->xml      = NULL;
  o->output   = NULL;
  o->files    = NULL;
  o->file     = NULL;
  o->rest     = NULL;
  return o;
}

static G_GNUC_NORETURN void
usage (char *application_name)
{
    fprintf (stderr,
"usage: %s [options] <file | -- [op [op] ..]>\n"
"\n"
"  Options:\n"
"     -h, --help      this help information\n"
"\n"
"     -i, --file      read xml from named file\n"
"\n"
"     -x, --xml       use xml provided in next argument\n"
"\n"
"     --dot           output a graphviz graph description\n"
"\n"
"     -o, --output    output generated image to named file, type based\n"
"                     on extension.\n"
"\n"
"     -p              increment frame counters of various elements when\n"
"                     processing is done.\n"
"\n"
"     -X              output the XML that was read in\n"
"\n"
"     -v, --verbose   print diagnostics while running\n"
"\n"
"All parameters following -- are considered ops to be chained together\n"
"into a small composition instead of using an xml file, this allows for\n"
"easy testing of filters. Be aware that the default value will be used\n"
"for all properties.\n"
, application_name);
    exit (0);
}

#define match(string) (!strcmp (*curr, (string)))
#define assert_argument() do {\
    if (!curr[1] || curr[1][0]=='-') {\
        fprintf (stderr, "ERROR: '%s' option expected argument\n", *curr);\
        exit(-1);\
    }\
}while(0)

#define get_float(var) do{\
    assert_argument();\
    curr++;\
    (var)=atof(*curr);\
}while(0)

#define get_int(var) do{\
    assert_argument();\
    curr++;\
    (var)=atoi(*curr);\
}while(0)

#define get_string(var) do{\
    assert_argument();\
    curr++;\
    (var)=*curr;\
}while(0)

#define get_string_forced(var) do{\
    curr++;\
    (var)=*curr;\
}while(0)

static GeglOptions *
parse_args (gint    argc,
            gchar **argv);

static void
print_opts (GeglOptions *o)
{
  char *mode_str;
  switch (o->mode)
    {
      case GEGL_RUN_MODE_DISPLAY:
        mode_str = "Display on screen"; break;
      case GEGL_RUN_MODE_XML:
        mode_str = "Print XML"; break;
      case GEGL_RUN_MODE_OUTPUT:
        mode_str = "Output in a file"; break;
      case GEGL_RUN_MODE_HELP:
        mode_str = "Display help information"; break;
      default:
        g_warning ("Unknown GeglOption mode: %d", o->mode);
        break;
    }

    fprintf (stderr,
"Parsed commandline:\n"
"\tmode:   %s\n"
"\tfile:   %s\n"
"\txml:    %s\n"
"\toutput: %s\n"
"\trest:   %s\n"
"\t\n",
    mode_str,
    o->file==NULL?"(null)":o->file,
    o->xml==NULL?"(null)":o->xml,
    o->output==NULL?"(null)":o->output,
    o->rest==NULL?"":"yes"
);
    {
      GList *files = o->files;
      while (files)
        {
          fprintf (stderr, "\t%s\n", (gchar*)files->data);
          files = g_list_next (files);
        }
    }
}

GeglOptions *
gegl_options_parse (gint    argc,
                    gchar **argv)
{
    GeglOptions *o;

    o = parse_args (argc, argv);
    if (o->verbose)
        print_opts (o);
    return o;
}

gboolean
gegl_options_next_file (GeglOptions *o)
{
  GList *current = g_list_find (o->files, o->file);
  current = g_list_next (current);
  if (current)
    {
      g_warning ("%s", o->file);
      o->file = current->data;
      g_warning ("%s", o->file);
      return TRUE;
    }
  return FALSE;
}

gboolean
gegl_options_previous_file (GeglOptions *o)
{
  GList *current = g_list_find (o->files, o->file);
  current = g_list_previous (current);
  if (current)
    {
      o->file = current->data;
      return TRUE;
    }
  return FALSE;
}


static GeglOptions *
parse_args (int    argc,
            char **argv)
{
    GeglOptions *o;
    char **curr;

    if (argc==1) {
        usage (argv[0]);
    }

    o = opts_new ();
    curr = argv+1;

    while (*curr && !o->rest) {
        if (match ("-h")    ||
            match ("--help")) {
            o->mode = GEGL_RUN_MODE_HELP;
            usage (argv[0]);
        }

        else if (match ("--verbose") ||
                 match ("-v")) {
            o->verbose=1;
        }

        else if (match ("--g-fatal-warnings") ||
                 match ("-v")) {
            o->fatal_warnings=1;
        }

        else if (match ("-p")){
            o->play=TRUE;
        }

        else if (match ("--file") ||
                 match ("-i")) {
            const gchar *file_path;
            get_string (file_path);
            o->files = g_list_append (o->files, g_strdup (file_path));
        }

        else if (match ("--xml") ||
                 match ("-x")) {
            get_string (o->xml);
        }

        else if (match ("--output") ||
                 match ("-o")) {
            get_string_forced (o->output);
            o->mode = GEGL_RUN_MODE_OUTPUT;
        }

        else if (match ("-X")) {
            o->mode = GEGL_RUN_MODE_XML;
        }

        else if (match ("--")) {
            o->rest = curr+1;
            break;
        }

        else if (*curr[0]=='-') {
            fprintf (stderr, "\n\nunknown parameter '%s' giving you help instead\n\n\n", *curr);
            usage (argv[0]);
        }

        else
          {
            o->files = g_list_append (o->files, g_strdup (*curr));
          }
        curr++;
    }

    if (o->files)
      o->file = o->files->data;
    return o;
}
#undef match
#undef assert_argument
