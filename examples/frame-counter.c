#include <gegl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char *output_path = "frame-counter.ogv";
const char *video_codec = NULL;

int video_bit_rate = 0;
int video_bit_rate_min = 0;
int video_bit_rate_max = 0;
int video_bufsize = 0;
float frame_rate = 0.0;

gint
main (gint    argc,
      gchar **argv)
{
  int c;
  for (c = 1; argv[c]; c++)
  {
    if (!strcmp (argv[c], "--video-bufsize"))
      video_bufsize = atoi(argv[++c]);
    else if (!strcmp (argv[c], "--video-bit-rate-max"))
      video_bit_rate_max = atoi(argv[++c]);
    else if (!strcmp (argv[c], "--video-bit-rate-min"))
      video_bit_rate_min = atoi(argv[++c]);
    else if (!strcmp (argv[c], "--video-bit-rate"))
      video_bit_rate = atoi(argv[++c]);
    else if (!strcmp (argv[c], "--fps"))
      frame_rate = g_strtod (argv[++c], NULL);
    else if (!strcmp (argv[c], "--video-codec"))
      video_codec = argv[++c];
    else
      output_path = argv[c];
  }

  gegl_init (&argc, &argv);  /* initialize the GEGL library */
  {
    GeglNode *gegl  = gegl_node_new ();
    GeglNode *store = gegl_node_new_child (gegl,
                              "operation", "gegl:ff-save",
                              "path", output_path,
                              NULL);
    GeglNode *crop    = gegl_node_new_child (gegl,
                              "operation", "gegl:crop",
                              "width", 512.0,
                              "height", 384.0,
                               NULL);
    GeglNode *over    = gegl_node_new_child (gegl,
                              "operation", "gegl:over",
                              NULL);
    GeglNode *text    = gegl_node_new_child (gegl,
                              "operation", "gegl:text",
                              "size", 120.0,
                              "color", gegl_color_new ("rgb(1.0,0.0,1.0)"),
                              NULL);
    GeglNode *bg      = gegl_node_new_child (gegl,
                             "operation", "gegl:color",
                              "value", gegl_color_new ("rgb(0.1,0.2,0.3)"),
                             NULL);

    if (frame_rate)
      gegl_node_set (store, "frame-rate", frame_rate, NULL);
    if (video_bufsize)
      gegl_node_set (store, "video-bufsize", video_bufsize, NULL);
    if (video_bit_rate)
      gegl_node_set (store, "video-bit-rate", video_bit_rate, NULL);
#if 0
    if (video_bit_rate_min)
      gegl_node_set (store, "video-bit-rate-min", video_bit_rate_min, NULL);
    if (video_bit_rate_max)
      gegl_node_set (store, "video-bit-rate-max", video_bit_rate_max, NULL);
#endif

    gegl_node_link_many (bg, over, crop, store, NULL);
    gegl_node_connect_to (text, "output",  over, "aux");

    {
      gint frame;
      gint frames = 200;

      for (frame=0; frame < frames; frame++)
        {
          gchar string[512];
          gdouble t = frame * 1.0/frames;
#define INTERPOLATE(min,max) ((max)*(t)+(min)*(1.0-t))

          sprintf (string, "#%i\n%1.2f%%", frame, t*100);
          gegl_node_set (text, "string", string, NULL);
          fprintf (stderr, "\r%i% 1.2f%% ", frame, t*100);
          gegl_node_process (store);
        }
    }
    g_object_unref (gegl);
  }
  gegl_exit ();
  return 0;
}
