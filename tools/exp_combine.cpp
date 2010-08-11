
#include <gegl.h> 

#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cmath>

#include <iostream>

#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>

using namespace std;

enum
{
  ARG_COMMAND,
  ARG_OUTPUT,
  ARG_PATH_0,

  NUM_ARGS
};


static const
gchar *COMBINER_INPUT_PREFIX = "exposure-";


static void
check_usage (gint argc, gchar **argv)
{
  if (argc == 1)
    {
      g_print ("This tool combines multiple exposures of one scene into a "
               "single buffer.\n");
      goto die;
    }

  if (argc < NUM_ARGS)
    {
      g_print ("Error: Insufficient arguments\n");
      goto die;
    }

  return;

die:
  g_print ("Usage: %s <output> <input> [<input> ...]\n", argv[0]);
  exit (EXIT_FAILURE);
}


static gfloat
expcombine_get_file_ev (const gchar *path)
{
  /* Open the file and read in the metadata */
  Exiv2::Image::AutoPtr image;
  try 
    {
      image = Exiv2::ImageFactory::open (path);
      image->readMetadata ();
    }
  catch (Exiv2::Error ex)
    {
      g_print ("Error: unable to read metadata from path: '%s'\n", path);
      exit (EXIT_FAILURE);
    }

  Exiv2::ExifData &exifData = image->exifData ();
  if (exifData.empty ())
      return NAN;

  /* Calculate the APEX brightness / EV */
  gfloat time, aperture, gain = 1.0f;

  time     = exifData["Exif.Photo.ExposureTime"].value().toFloat();
  aperture = exifData["Exif.Photo.FNumber"     ].value().toFloat();

  /* iso */
  try
    {
      gain = exifData["Exif.Photo.ISOSpeedRatings"].value().toLong() / 100.0f;
    }
  catch (Exiv2::Error ex)
    {
      // Assume ISO is set at 100. It's reasonably likely that the ISO is the
      // same across all images anyway, and for our purposes the relative
      // values can be sufficient.

      gain = 1.0f;
    }

  return log2f (aperture * aperture) + log2f (1 / time) + log2f (gain);
}


int
main (int    argc,
      char **argv)
{
  guint     cursor;
  GeglNode *gegl, *combiner, *sink;
  gchar    *all_evs = g_strdup ("");

  g_thread_init (NULL);
  gegl_init (&argc, &argv);
  check_usage (argc, argv);

  gegl     = gegl_node_new ();
  combiner = gegl_node_new_child (gegl,
                                  "operation", "gegl:exp-combine",
                                  NULL);

  for (cursor = ARG_PATH_0; cursor < argc; ++cursor)
    {
      const gchar *input_path;
      gchar        ev_string[G_ASCII_DTOSTR_BUF_SIZE + 1];
      gfloat       ev_val;

      gchar        combiner_pad[strlen (COMBINER_INPUT_PREFIX) +
                                G_ASCII_DTOSTR_BUF_SIZE + 1];
      gint         err;

      GeglNode    *img;
      
      input_path = argv[cursor];
      ev_val     = expcombine_get_file_ev (input_path);
      if (isnan (ev_val))
        {
          g_print ("Failed to calculate exposure value for '%s'\n",
                   input_path);
          exit (EXIT_FAILURE);
        }

      g_ascii_dtostr (ev_string, G_N_ELEMENTS (ev_string), ev_val);
      all_evs = g_strconcat (all_evs, " ", ev_string, NULL);

      /* Construct and link the input image into the combiner */
      img = gegl_node_new_child (gegl,
                                 "operation", "gegl:load",
                                 "path",      input_path,
                                  NULL);

      /* Create the exposure pad name */
      err = snprintf (combiner_pad,
                      G_N_ELEMENTS (combiner_pad),
                      "%s%u",
                      COMBINER_INPUT_PREFIX,
                      cursor - ARG_PATH_0);
      if (err < 1 || err >= G_N_ELEMENTS (combiner_pad))
        {
          g_warning ("Unable to construct input pad name for exposure %u\n",
                     cursor);
          return (EXIT_FAILURE);
        }

      gegl_node_connect_to (img, "output", combiner, combiner_pad);
    }

  g_return_val_if_fail (all_evs[0] == ' ', EXIT_FAILURE);
  gegl_node_set (combiner, "exposures", all_evs + 1, NULL);


  /* We should not have skipped past the last element of the arguments */
  g_return_val_if_fail (cursor == argc, EXIT_FAILURE);
  sink     = gegl_node_new_child (gegl,
                                  "operation", "gegl:save",
                                  "path", argv[ARG_OUTPUT],
                                   NULL);

  gegl_node_link_many (combiner, sink, NULL);
  gegl_node_process (sink);
  return (EXIT_SUCCESS);
}
