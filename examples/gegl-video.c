/*
 *   gegl-video <video-file|frame-folder|frames|edl>
 *
 *      --output-frames path/image- --frame-extension png
 *      --output-video  path/video.avi
 *      --play (default)
 *      -- gegl:threshold value=0.3
 *
 * the edl,. should be a list of frames.. and video with frame start->end bits
 * 0001.png
 * 0002.png
 * 0003.png
 * 0004.png
 * foo.avi 3-210
 * clip2.avi
 *
 *   image folders are folder containing image files to be played in sequence
 *   each image file contains its own embedded PCM data, facilitating easy re-assembly.
 *
 */

#include <gegl.h>
#include <gegl-audio-fragment.h>
#include <glib/gprintf.h>
#include <gexiv2/gexiv2.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int frame_start = 0;
int frame_end   = 0;

char *frame_extension = ".png";
char *frame_path = NULL;
char *video_path = NULL;
int output_frame_no = -1;
int show_progress = 0;

void
gegl_meta_set_audio (const char         *path,
                     GeglAudioFragment  *audio);
void
gegl_meta_get_audio (const char         *path,
                     GeglAudioFragment  *audio);

void usage(void);
void usage(void)
{
      printf ("usage: negl [options] <video> [thumb]\n\n"
" -p, --progress   - display info about current frame/progress in terminal\n"
" -s <frame>, --start-frame <frame>\n"
"           - first frame to extract analysis from (default 0)\n"
" -e <frame>, --end-frame <frame>\n"
"           - last frame to extract analysis from (default is 0 which means auto end)\n"
" -of <output_path_prefix>, --output-path <path_prefix>\n"
" -fe <.ext> --frame-extension <.ext>\n"
"           - .png or .jpg are supported file formats, .jpg default\n"
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
    if (g_str_equal (argv[i], "-p") ||
        g_str_equal (argv[i], "--progress"))
    {
      show_progress = 1;
    }
    else if (g_str_equal (argv[i], "-s") ||
             g_str_equal (argv[i], "--start-frame"))
    {
      frame_start = g_strtod (argv[i+1], NULL);
      i++;
    } 
    else if (g_str_equal (argv[i], "-c") ||
             g_str_equal (argv[i], "--frame-count"))
    {
      frame_end = frame_start + g_strtod (argv[i+1], NULL) - 1;
      i++;
    } 
    else if (g_str_equal (argv[i], "--output-frames") ||
             g_str_equal (argv[i], "-of"))
    {
      frame_path = g_strdup (argv[i+1]);
      i++;
    } 
    else if (g_str_equal (argv[i], "--output-frame-start") ||
             g_str_equal (argv[i], "-ofs`"))
    {
      output_frame_no = g_strtod (argv[i+1], NULL);
      i++;
    } 
    else if (g_str_equal (argv[i], "--frame-extension") ||
             g_str_equal (argv[i], "-fe"))
    {
      frame_extension = g_strdup (argv[i+1]);
      i++;
    } 
    else if (g_str_equal (argv[i], "-e") ||
             g_str_equal (argv[i], "--end-frame"))
    {
      frame_end = g_strtod (argv[i+1], NULL);
      i++;
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
    } 
  }
}

GeglNode   *gegl_decode  = NULL;
GeglNode   *display      = NULL;

GeglBuffer *previous_video_frame  = NULL;
GeglBuffer *video_frame  = NULL;

GeglNode *store, *load;

static void decode_frame_no (int frame)
{
  if (video_frame)
  {
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
  if (argc < 2)
    usage();

  gegl_init (&argc, &argv);
  parse_args (argc, argv);

  gegl_decode = gegl_node_new ();

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

  if (output_frame_no == -1)
    output_frame_no = frame_start;

  printf ("frames: %i - %i\n", frame_start, frame_end);
  printf ("video: %s\n", video_path);

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
        {
        GeglAudioFragment *audio;
        gdouble fps;
        gegl_node_get (load, "audio", &audio,
                             "frame-rate", &fps, NULL);

	if (frame_path)
	{
          char *path = g_strdup_printf ("%s%07i%s", frame_path, output_frame_no++, frame_extension);
	  GeglNode *save_graph = gegl_node_new ();
	  GeglNode *readbuf, *save;
     
	  readbuf = gegl_node_new_child (save_graph, "operation", "gegl:buffer-source", "buffer", video_frame, NULL);
	  save = gegl_node_new_child (save_graph, "operation", "gegl:save",
	    "path", path, NULL);
	    gegl_node_link_many (readbuf, save, NULL);
	  gegl_node_process (save);
	  g_object_unref (save_graph);
          gegl_meta_set_audio (path, audio);
          g_free (path);
	}
        g_object_unref (audio);
        }

      }
      if (show_progress)
        fprintf (stdout, "\n");
  }

  if (video_frame)
    g_object_unref (video_frame);
  video_frame = NULL;
  g_object_unref (gegl_decode);

  gegl_exit ();
  return 0;
}

void
gegl_meta_set_audio (const char        *path,
                     GeglAudioFragment *audio)
{
  GError *error = NULL; 
  GExiv2Metadata *e2m = gexiv2_metadata_new ();
  gexiv2_metadata_open_path (e2m, path, &error);
  if (error)
  { 
    g_warning ("%s", error->message);
  }
  else
  { 
    int i, c;
    GString *str = g_string_new ("");
    int sample_count = gegl_audio_fragment_get_sample_count (audio);
    int channels = gegl_audio_fragment_get_channels (audio);
    if (gexiv2_metadata_has_tag (e2m, "Xmp.xmp.GEGL"))
      gexiv2_metadata_clear_tag (e2m, "Xmp.xmp.GEGL");

    g_string_append_printf (str, "%i %i %i %i", 
                              gegl_audio_fragment_get_sample_rate (audio),
                              gegl_audio_fragment_get_channels (audio),
                              gegl_audio_fragment_get_channel_layout (audio),
                              gegl_audio_fragment_get_sample_count (audio));

    for (i = 0; i < sample_count; i++)
      for (c = 0; c < channels; c++)
        g_string_append_printf (str, " %0.5f", audio->data[c][i]);

    gexiv2_metadata_set_tag_string (e2m, "Xmp.xmp.GeglAudio", str->str);
    gexiv2_metadata_save_file (e2m, path, &error);
    if (error)
      g_warning ("%s", error->message);
    g_string_free (str, TRUE);
  }
  g_object_unref (e2m);
}

void
gegl_meta_get_audio (const char        *path,
                     GeglAudioFragment *audio)
{
  //gchar  *ret   = NULL;
  GError *error = NULL;
  GExiv2Metadata *e2m = gexiv2_metadata_new ();
  gexiv2_metadata_open_path (e2m, path, &error);
  if (!error)
  {
    gchar *ret = gexiv2_metadata_get_tag_string (e2m, "Xmp.xmp.GeglAudio");
    fprintf (stderr, "should parse audio\n");
    g_free (ret);
  }
  else
    g_warning ("%s", error->message);
  g_object_unref (e2m);
}
