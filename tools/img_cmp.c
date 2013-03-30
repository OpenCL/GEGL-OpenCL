#include <gegl.h>
#include <math.h>
#include <string.h>

#define SQR(x) ((x) * (x))

gint
main (gint    argc,
      gchar **argv)
{
  GeglNode      *gegl, *imgA, *imgB, *comparison;
  GeglRectangle  boundsA, boundsB;
  gdouble        max_diff, avg_diff_wrong, avg_diff_total;
  gint           wrong_pixels, total_pixels;

  gegl_init (&argc, &argv);

  if (argc != 3)
    {
      g_print ("This is simple image difference detection tool for use in regression testing"
               "return message is non zero if images are different, if they are equal"
               "the output will contain the string identical.");
      g_print ("Usage: %s <imageA> <imageB>\n", argv[0]);
      return 1;
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
      g_printerr ("%s and %s differ in size\n", argv[1], argv[2]);
      g_printerr ("  %ix%i vs %ix%i\n",
                  boundsA.width, boundsA.height, boundsB.width, boundsB.height);
      return 1;
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
      g_printerr ("%s and %s differ\n"
                  "  wrong pixels   : %i/%i (%2.2f%%)\n"
                  "  max Δe         : %2.3f\n"
                  "  avg Δe (wrong) : %2.3f(wrong) %2.3f(total)\n",
                  argv[1], argv[2],
                  wrong_pixels, total_pixels,
                  (wrong_pixels*100.0/total_pixels), max_diff,
                  avg_diff_wrong, avg_diff_total);

      if (max_diff > 1.5 &&
          !strstr (argv[2], "broken"))
        {
          GeglNode *save;
          gchar *debug_path = g_malloc (strlen (argv[2])+16);

          memcpy (debug_path, argv[2], strlen (argv[2])+1);
          memcpy (debug_path + strlen(argv[2])-4, "-diff.png", 11);

          save = gegl_node_new_child (gegl,
                                      "operation", "gegl:png-save",
                                      "path", debug_path,
                                      NULL);
          gegl_node_link (comparison, save);
          gegl_node_process (save);


          /*gegl_graph (sink=gegl_node ("gegl:png-save",
                                      "path", debug_path, NULL,
                                      gegl_node ("gegl:buffer-source", "buffer", debug_buf, NULL)));*/

          return 1;
        }
      if (strstr (argv[2], "broken"))
        g_print ("because the test is expected to fail ");
      else
        g_print ("because the error is small ");
      g_print ("we'll say ");
    }

  g_print ("%s and %s are identical\n", argv[1], argv[2]);

  g_object_unref (gegl);

  gegl_exit ();
  return 0;
}
