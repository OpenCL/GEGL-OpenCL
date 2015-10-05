/*
 *   gegl-video play   <video-file|image folder>
 *   gegl-video decode <video-file> image-folder/
 *   gegl-video encode image-foler/ video-file
 *
 *   image folders are folder containing image files to be played in sequence
 *   each image file contains its own embedded PCM data, facilitating easy re-assembly.
 *
 */

#include <gegl.h>
#include <gegl-audio.h>
#include <glib/gprintf.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

typedef enum NeglRunMode {
  NEGL_NO_UI = 0,
  NEGL_VIDEO,
  NEGL_TERRAIN,
  NEGL_UI,
} NeglRunmode;

int frame_start = 0;
int frame_end   = 0;

char *video_path = NULL;
char *thumb_path = NULL;
char *input_analysis_path = NULL;
char *output_analysis_path = NULL;
int run_mode = NEGL_NO_UI;
int show_progress = 0;
int sum_diff = 0;
int frame_thumb = 0;
int time_out = 0;

long babl_ticks (void);

void usage(void);
void usage(void)
{
      printf ("usage: negl [options] <video> [thumb]\n"
" -p, --progress   - display info about current frame/progress in terminal\n"
" -oa <analysis-path>, --output-analysis\n"
"                  - specify path for analysis dump to write (a PNG)\n"
" -ia <analysis-path>, --input-analysis\n"
"                  - specify path for analysis dump to read (a PNG)\n"
" -d, --sum-diff\n"
"           - sum up all pixel differences for neighbour frames (defaults to not do it)\n"
"\n"
"\n"
"debug level control over run-mode options:\n"
" -t, --timeout - stop doing frame info dump after this mant seconds have passed)\n"
" --video   - show video frames as they are decoded (for debug)\n"
" --terrain - show data realtime as it is being gathered\n"
" --no-ui   - do not show a window (this is the default)\n"
" -s <frame>, --start-frame <frame>\n"
"           - first frame to extract analysis from (default 0)\n"
" -e <frame>, --end-frame <frame>\n"
"           - last frame to extract analysis from (default is 0 which means auto end)\n"
"\n"
"Options can also follow then video (and thumb) arguments.\n"
"\n");
  exit (0);
}

static void parse_args (int argc, char **argv)
{
  int i;
  int stage = 0;
  for (i = 1; i < argc; i++)
  {
    if (g_str_equal (argv[i], "-oa") ||
        g_str_equal (argv[i], "--output-analysis"))
    {
      output_analysis_path = g_strdup (argv[i+1]);
      i++;
    } 
    else if (g_str_equal (argv[i], "-p") ||
        g_str_equal (argv[i], "--progress"))
    {
      show_progress = 1;
    }
    else if (g_str_equal (argv[i], "-d") ||
        g_str_equal (argv[i], "--sum-diff"))
    {
      sum_diff = 1;
    }
    else if (g_str_equal (argv[i], "-ia") ||
        g_str_equal (argv[i], "--input-analysis"))
    {
      input_analysis_path = g_strdup (argv[i+1]);
      i++;
    } 
    else if (g_str_equal (argv[i], "-s") ||
             g_str_equal (argv[i], "--start-frame"))
    {
      frame_start = g_strtod (argv[i+1], NULL);
      i++;
    } 
    else if (g_str_equal (argv[i], "-t") ||
             g_str_equal (argv[i], "--time-out"))
    {
      time_out = g_strtod (argv[i+1], NULL);
      i++;
    } 
    else if (g_str_equal (argv[i], "-e") ||
             g_str_equal (argv[i], "--end-frame"))
    {
      frame_end = g_strtod (argv[i+1], NULL);
      i++;
    } 
    else if (g_str_equal (argv[i], "--video"))
    {
      run_mode = NEGL_VIDEO;
    }
    else if (g_str_equal (argv[i], "--no-ui"))
    {
      run_mode = NEGL_NO_UI;
    }
    else if (g_str_equal (argv[i], "--ui"))
    {
      run_mode = NEGL_UI;
    }
    else if (g_str_equal (argv[i], "--terrain"))
    {
      run_mode = NEGL_TERRAIN;
    }
    else if (g_str_equal (argv[i], "-h") ||
             g_str_equal (argv[i], "--help"))
    {
      usage();
    }
    else if (stage == 0)
    {
      video_path = g_strdup (argv[i]);
      stage = 1;
    } else if (stage == 1)
    {
      thumb_path = g_strdup (argv[i]);
      stage = 2;
    }
  }

  printf ("frames: %i - %i\n", frame_start, frame_end);
  printf ("video: %s\n", video_path);
  printf ("thumb: %s\n", thumb_path);
  printf ("input analysis: %s\n", input_analysis_path);
  printf ("output analysis: %s\n", output_analysis_path);
}

#include <string.h>

GeglNode   *gegl_decode  = NULL;
GeglNode   *gegl_display = NULL;
GeglNode   *display      = NULL;

GeglBuffer *previous_video_frame  = NULL;
GeglBuffer *video_frame  = NULL;

GeglNode *store, *load;

static void decode_frame_no (int frame)
{
  if (video_frame)
  {
    if (sum_diff)
    {
       if (previous_video_frame)
         g_object_unref (previous_video_frame);
       previous_video_frame = gegl_buffer_dup (video_frame);
    }
    g_object_unref (video_frame);
  }
  video_frame = NULL;
  gegl_node_set (load, "frame", frame, NULL);
  gegl_node_process (store);
}

GeglNode *translate = NULL;

gint
main (gint    argc,
      gchar **argv)
{
  GeglNode *display = NULL;
  GeglNode *readbuf = NULL;

  if (argc < 2)
    usage();

  gegl_init (&argc, &argv);
  parse_args (argc, argv);

  gegl_decode = gegl_node_new ();
  gegl_display = gegl_node_new ();

  store = gegl_node_new_child (gegl_decode,
                               "operation", "gegl:buffer-sink",
                               "buffer", &video_frame, NULL);
  load = gegl_node_new_child (gegl_decode,
                              "operation", "gegl:ff-load",
                              "frame", 0,
                              "path", video_path,
                              NULL);
  gegl_node_link_many (load, store, NULL);

  decode_frame_no (0);

  {
    int frames = 0; 
    double frame_rate = 0;
    gegl_node_get (load, "frames", &frames, NULL);
    gegl_node_get (load, "frame-rate", &frame_rate, NULL);
    
    if (frame_end == 0)
      frame_end = frames;
  }

  switch(run_mode)
  {
    case NEGL_NO_UI:
      break;
    case NEGL_VIDEO:
      readbuf = gegl_node_new_child (gegl_display,
                                     "operation", "gegl:buffer-source",
                                     NULL);
      display = gegl_node_create_child (gegl_display, "gegl:display");
      gegl_node_link_many (readbuf, display, NULL);
      break;
  }

  {
    gint frame;
    for (frame = frame_start; frame <= frame_end; frame++)
      {
        if (show_progress)
        {
          fprintf (stdout, "\r%2.1f%% %i/%i (%i)", 
                   (frame-frame_start) * 100.0 / (frame_end-frame_start),
                   frame-frame_start,
                   frame_end-frame_start,
                   frame);
          fflush (stdout);
        }

	decode_frame_no (frame);

        switch(run_mode)
        {
          case NEGL_NO_UI:
            break;
          case NEGL_VIDEO:
	    gegl_node_set (readbuf, "buffer", video_frame, NULL);
	    gegl_node_process (display);
            break;
        }
      }
      if (show_progress)
      {
        fprintf (stdout, "\n");
        fflush (stdout);
      }
  }

  if (thumb_path)
  {
    GeglNode *save_graph = gegl_node_new ();
    GeglNode *readbuf, *save;
    decode_frame_no (frame_thumb);
    readbuf = gegl_node_new_child (save_graph, "operation", "gegl:buffer-source", "buffer", video_frame, NULL);
    save = gegl_node_new_child (save_graph, "operation", "gegl:png-save",
      "path", thumb_path, NULL);
      gegl_node_link_many (readbuf, save, NULL);
    gegl_node_process (save);
    g_object_unref (save_graph);
  }

  if (video_frame)
    g_object_unref (video_frame);
  video_frame = NULL;
  g_object_unref (gegl_decode);
  g_object_unref (gegl_display);

  gegl_exit ();
  return 0;
}
