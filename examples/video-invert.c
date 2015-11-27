#include <gegl.h>
#include <stdio.h>

GeglNode *gegl_dec;

GeglNode   *decode, *store_buf;
GeglBuffer *buffer = NULL;

GeglNode *gegl_enc;
GeglNode *load_buf, *invert, *encode, *invert2, *invert3;

const char *input_path = "input.ogv";
const char *output_path = "output.ogv";

gdouble fps;

gint
main (gint argc,
      gchar **argv)
{
  
  if (argv[1])
  {
    input_path = argv[1];
    if (argv[2])
      output_path = argv[2];
  }


  gegl_init (&argc, &argv);

  gegl_dec  = gegl_node_new ();
  decode = gegl_node_new_child (gegl_dec, "operation", "gegl:ff-load", "path", input_path, NULL);
  store_buf = gegl_node_new_child (gegl_dec, "operation", "gegl:buffer-sink", "buffer", &buffer, NULL);
  gegl_node_link_many (decode, store_buf, NULL);
  
  gegl_enc = gegl_node_new ();
  load_buf = gegl_node_new_child (gegl_enc, "operation", "gegl:buffer-source", NULL);
  invert = gegl_node_new_child (gegl_enc,   "operation", "gegl:gray", NULL);
  encode = gegl_node_new_child (gegl_dec,   "operation", "gegl:ff-save", "path", output_path, NULL);

  gegl_node_link_many (load_buf, invert, /*invert2, invert3,*/ encode, NULL);

  {
    gint frame_no;
    gint frame_count = 100;
    gint audio_sample_rate = 44100;
    gint audio_channels = 0;

    for (frame_no = 0; frame_no < frame_count; frame_no++)
    {
      GeglAudioFragment *audio;
      fprintf (stderr, "\r%i/%i", frame_no, frame_count);
      gegl_node_set (decode, "frame", frame_no, NULL);
      if (buffer){
        g_object_unref (buffer);
        buffer = NULL;
      }
      gegl_node_process (store_buf);
      if (buffer)
      {
        if (frame_no == 0)
        {
          gegl_node_get (decode, "frame-rate", &fps, 
                                 "frames", &frame_count, 
                                 "audio-sample-rate", &audio_sample_rate, 
                                 "audio-channels", &audio_channels,
                                 NULL);
          fprintf (stderr, "frame-rate: %f\n", fps);
          gegl_node_set (encode, "frame-rate", fps, NULL);
        }
        gegl_node_get (decode, "audio", &audio, NULL);
        gegl_node_set (encode, "audio", audio, NULL);
        gegl_node_set (load_buf, "buffer", buffer, NULL);
        gegl_node_process (encode);
      }
    }
  }
  fprintf (stderr, "\n");
  g_object_unref (gegl_enc);
  g_object_unref (gegl_dec);
  g_object_unref (buffer);

  gegl_exit ();
  return 0;
}
