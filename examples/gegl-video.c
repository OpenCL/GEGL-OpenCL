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
#include <SDL.h>

typedef enum NeglRunMode {
  NEGL_NO_UI = 0,
  NEGL_VIDEO,
  NEGL_TERRAIN,
  NEGL_UI,
} NeglRunmode;

int frame_start = 0;
int frame_end   = 0;

char *frame_extension = ".png";
char *frame_path = NULL;
char *video_path = NULL;
int output_frame_no = -1;
int run_mode = NEGL_NO_UI;
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
      printf ("usage: negl [options] <video> [thumb]\n"
" -p, --progress   - display info about current frame/progress in terminal\n"
"\n"
"\n"
"debug level control over run-mode options:\n"
" --video   - show video frames as they are decoded (for debug)\n"
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
    else if (g_str_equal (argv[i], "--ui"))
    {
      run_mode = NEGL_VIDEO;
    }
    else if (g_str_equal (argv[i], "--no-ui"))
    {
      run_mode = NEGL_NO_UI;
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
GeglNode   *gegl_display = NULL;
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

static int audio_len = 0;
static int audio_pos = 0;

int16_t audio_data[8192000];


static void fill_audio(void *udata, Uint8 *stream, int len)
{
  int audio_remaining = audio_len - audio_pos;
  if (audio_remaining < 0)
    return;

  if (audio_remaining < len) len = audio_remaining;

  //SDL_MixAudio(stream, (uint8_t*)&audio_data[audio_pos/2], len, SDL_MIX_MAXVOLUME);
  memcpy (stream, (uint8_t*)&audio_data[audio_pos/2], len);
  audio_pos += len;
}

static int audio_started = 0;

gint
main (gint    argc,
      gchar **argv)
{
  SDL_AudioSpec AudioSettings = {0};

  GeglNode *display = NULL;
  GeglNode *readbuf = NULL;

  SDL_Init(SDL_INIT_AUDIO);
  AudioSettings.freq = 48000;
  AudioSettings.format = AUDIO_S16SYS;
  AudioSettings.channels = 2;
  AudioSettings.samples = 1024;
  AudioSettings.callback = fill_audio;
  SDL_OpenAudio(&AudioSettings, 0);

  if (AudioSettings.format != AUDIO_S16SYS)
   {
      fprintf (stderr, "not getting format we wanted\n");
   }
  if (AudioSettings.freq != 48000)
   {
      fprintf (stderr, "not getting rate we wanted\n");
   }

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

  if (output_frame_no == -1)
    output_frame_no = frame_start;

  printf ("frames: %i - %i\n", frame_start, frame_end);
  printf ("video: %s\n", video_path);

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
        {
        GeglAudioFragment *audio;
        gdouble fps;
        int sample_count;
        int sample_rate;
        gegl_node_get (load, "audio", &audio,
                             "frame-rate", &fps, NULL);
        sample_count = gegl_audio_fragment_get_sample_count (audio);
        sample_rate = gegl_audio_fragment_get_sample_rate (audio);

        if (sample_count > 0)
        {
          int i;
          if (!audio_started)
           {
             SDL_PauseAudio(0);
             audio_started = 1;
           }
          for (i = 0; i < sample_count; i++)
          {
            audio_data[audio_len/2 + 0] = audio->data[0][i] * 32767.0;
            audio_data[audio_len/2 + 1] = audio->data[1][i] * 32767.0;
            audio_len += 4;
          }
        }

        switch(run_mode)
        {
          case NEGL_NO_UI:
            break;
          case NEGL_VIDEO:

	    gegl_node_set (readbuf, "buffer", video_frame, NULL);
	    gegl_node_process (display);
	    while ( (audio_pos / 4.0) / sample_rate < (frame / fps) - 0.05 )
            {
              g_usleep (500); /* sync audio */
            }
            break;
        }

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
  g_object_unref (gegl_display);

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
  gexiv2_metadata_free (e2m);

  SDL_CloseAudio();
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
  gexiv2_metadata_free (e2m);
}
