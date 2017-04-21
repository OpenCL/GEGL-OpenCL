#include <gegl.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* running the gegl-imgcmp should build up the list of checksums..
 */

#define DEFAULT_ERROR_DIFFERENCE 1.5
#define SQR(x) ((x) * (x))

typedef enum {
    SUCCESS = 0,
    ERROR_WRONG_ARGUMENTS,
    ERROR_WRONG_SIZE,
    ERROR_PIXELS_DIFFERENT,
} ExitCode;

static gchar *compute_image_checksum (const char *path);
static gchar *
compute_image_checksum (const gchar *path)
{
  gchar *ret = NULL;
  GeglNode *gegl = gegl_node_new ();
  GeglRectangle comp_bounds;
  guchar *buf;
  GeglNode *img = gegl_node_new_child (gegl,
                                 "operation", "gegl:load",
                                 "path", path,
                                 NULL);
  comp_bounds = gegl_node_get_bounding_box (img);
  buf = g_malloc0 (comp_bounds.width * comp_bounds.height * 4);
  gegl_node_blit (img, 1.0, &comp_bounds, babl_format("R'G'B'A u8"), buf, GEGL_AUTO_ROWSTRIDE, GEGL_BLIT_DEFAULT);
  ret = g_compute_checksum_for_data (G_CHECKSUM_MD5, buf, comp_bounds.width * comp_bounds.height * 4);
  g_free (buf);
  g_object_unref (gegl);
  return ret;
}


const gchar *get_image_checksum (const char *path);
const gchar *get_image_checksum (const char *path)
{
  return compute_image_checksum (path);
}

gint
main (gint    argc,
      gchar **argv)
{
  GeglNode      *gegl, *imgA, *imgB, *comparison;
  GeglRectangle  boundsA, boundsB;
  gchar         *md5A, *md5B;
  gdouble        max_diff, avg_diff_wrong, avg_diff_total;
  gdouble        error_diff;
  gint           wrong_pixels, total_pixels;

  gegl_init (&argc, &argv);

  if ((argc != 3) && (argc != 4))
    {
      g_print ("This is simple image difference detection tool for use in regression testing.\n"
               "Exit code is non zero if images are different, if they are equal"
               "the output will contain the string identical.\n");
      g_print ("Usage: %s <imageA> <imageB> [<error-difference>]\n", argv[0]);
      return ERROR_WRONG_ARGUMENTS;
    }

  error_diff = DEFAULT_ERROR_DIFFERENCE;
  if (argc == 4)
    {
      gdouble t;
      char *end;

      end = NULL;
      errno = 0;
      t = g_ascii_strtod (argv[3], &end);
      if ((errno != ERANGE) && (end != argv[3]) && (end != NULL) && (*end == 0))
        error_diff = t;
    }

  if( access( argv[1], F_OK ) != 0 ) {
    g_print ("missing reference, assuming SUCCESS\n");
    return SUCCESS;
  }

  md5A = compute_image_checksum (argv[1]);
  md5B = compute_image_checksum (argv[2]);

  if (md5A && md5B && strcmp (md5A, md5B))
  {
    g_print ("raster md5s differ: %s vs %s\n", md5A, md5B);
  }

  gegl = gegl_node_new ();
  imgA = gegl_node_new_child (gegl,
                              "operation", "gegl:load",
                              "path", argv[1],
                              NULL);
  imgB = gegl_node_new_child (gegl,
                              "operation", "gegl:load",
                              "path", argv[2],
                              NULL);

  boundsA = gegl_node_get_bounding_box (imgA);
  boundsB = gegl_node_get_bounding_box (imgB);
  total_pixels = boundsA.width * boundsA.height;

  if (boundsA.width != boundsB.width || boundsA.height != boundsB.height)
    {
      g_print ("%s and %s differ in size\n", argv[1], argv[2]);
      g_print ("  %ix%i vs %ix%i\n",
                  boundsA.width, boundsA.height, boundsB.width, boundsB.height);
      return ERROR_WRONG_SIZE;
    }

  comparison = gegl_node_create_child (gegl, "gegl:image-compare");
  gegl_node_link (imgA, comparison);
  gegl_node_connect_to (imgB, "output", comparison, "aux");
  gegl_node_process (comparison);
  gegl_node_get (comparison,
                 "max diff", &max_diff,
                 "avg-diff-wrong", &avg_diff_wrong,
                 "avg-diff-total", &avg_diff_total,
                 "wrong-pixels", &wrong_pixels,
                 NULL);

  if (max_diff >= 0.1)
    {
      g_print ("%s and %s differ\n"
                  "  wrong pixels   : %i/%i (%2.2f%%)\n"
                  "  max Δe         : %2.3f\n"
                  "  avg Δe (wrong) : %2.3f(wrong) %2.3f(total)\n",
                  argv[1], argv[2],
                  wrong_pixels, total_pixels,
                  (wrong_pixels*100.0/total_pixels), max_diff,
                  avg_diff_wrong, avg_diff_total);

      if (!strstr (argv[2], "broken"))
        {
          GeglNode *save;
          gchar *debug_path = g_malloc (strlen (argv[2])+16);

          memcpy (debug_path, argv[2], strlen (argv[2])+1);
          memcpy (debug_path + strlen(argv[2])-4, "-diff.png", 10);

          save = gegl_node_new_child (gegl,
                                      "operation", "gegl:png-save",
                                      "path", debug_path,
                                      NULL);
          gegl_node_link (comparison, save);
          gegl_node_process (save);


          /*gegl_graph (sink=gegl_node ("gegl:png-save",
                                      "path", debug_path, NULL,
                                      gegl_node ("gegl:buffer-source", "buffer", debug_buf, NULL)));*/
          if (max_diff > error_diff)
            return ERROR_PIXELS_DIFFERENT;
        }
      if (strstr (argv[2], "broken"))
        g_print ("because the test is expected to fail ");
      else
        g_print ("because the error is smaller than %0.2f ", error_diff);
      g_print ("we'll say ");
    }

  g_print ("%s and %s are identical\n", argv[1], argv[2]);

  g_object_unref (gegl);

  gegl_exit ();
  return SUCCESS;
}
