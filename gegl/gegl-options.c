/*#include <sys/types.h>
#include <sys/stat.h>
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gegl-options.h"

static GeglOptions *opts_new (void)
{
  GeglOptions *o = g_malloc0 (sizeof (GeglOptions));

  o->mode     = GEGL_RUN_MODE_HELP;
  o->xml      = NULL;
  o->output   = NULL;
  o->file     = NULL;
  o->delay    = 0.0;
  o->rest     = NULL;
  return o;
}

static void
usage (char *application_name)
{
    fprintf (stderr,
"usage: %s [options] <file | -- [op [op] ..]>\n"
"\n"
"  Options:\n"
"     --help                this help information\n"
"     -h\n"
"\n"
"     --file                read xml from named file\n"
"     -i\n"
"\n"
"     --xml                 use xml provided in next argument\n"
"     -x\n"
"\n"
"     --output              output generated image to named file\n"
"     -o                    (file is saved in PNG format)\n"
"\n"
"     -X                    output the XML that was read in\n"
"\n"
"     --verbose             print diagnostics while running\n"
"      -v\n"
"\n"
"     --ui                  use gtk+ ui (act like a viewer/editor)"
"     -u\n"
"\n"
"     --delay               wait for specified number of seconds before exit\n"
"     -d                    (only valid when --ui or -u option is also used)\n"
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
    fprintf (stderr,
"Parsed commandline:\n"
"\tmode:   %i\n"
"\tfile:   %s\n"
"\txml:    %s\n"
"\toutput: %s\n"
"\trest:   %s\n"
"\tdelay:  %f\n"
"\t\n",
    o->mode,
    o->file,
    o->xml,
    o->output,
    o->rest==NULL?"":"yes",
    o->delay
);
}

static void
fixup (GeglOptions *o)
{
    if (o->file && o->rest) {
        fprintf (stderr, "ERROR: both chain and file specified\n");
        exit (-1);
    }
 /*   fprintf (stderr, "op fixup not written yet\n");*/
}

GeglOptions *
gegl_options_parse (gint    argc,
                    gchar **argv)
{
    GeglOptions *o;

    o = parse_args (argc, argv);
    fixup (o);
    if (o->verbose)
        print_opts (o);
    return o;
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

        else if (match ("--delay") ||
                 match ("-d")) {
            get_float (o->delay);
        }

        else if (match ("--verbose") ||
                 match ("-v")) {
            o->verbose=1;
        }

        else if (match ("--file") ||
                 match ("-i")) {
            get_string (o->file);
        }

        else if (match ("--xml") ||
                 match ("-x")) {
            get_string (o->xml);
        }

        else if (match ("--output") ||
                 match ("-o")) {
            get_string_forced (o->output);
            o->mode = GEGL_RUN_MODE_PNG;
        }

        else if (match ("-X")) {
            o->mode = GEGL_RUN_MODE_XML;
        }

        else if (match ("--ui") ||
                 match ("-u")) {
            o->mode = GEGL_RUN_MODE_INTERACTIVE;
        }

        else if (match ("--")) {
            o->rest = curr+1;
            break;
        }

        else if (*curr[0]=='-') {
            fprintf (stderr, "\n\nunknown paramter '%s' giving you help instead\n\n\n", *curr);
            usage (argv[0]);
        }

        else
          {
            o->file = *curr;
          }
        curr++;
    }

    return o;
}
#undef match
#undef assert_argument
