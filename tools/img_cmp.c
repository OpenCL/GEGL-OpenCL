#include <gegl.h> 
#include <math.h>
#include <string.h>

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *bufferA = NULL;
  GeglBuffer *bufferB = NULL;
  GeglBuffer *debug_buf = NULL;

  gegl_init (&argc, &argv);

  if (argc != 3)
    {
      g_print ("This is simple image difference detection tool for use in regression testing"
               "return message is non zero if images are different, if they are equal"
               "the output will contain the string identical.");
      g_print ("Usage: %s <imageA> <imageB>\n", argv[0]);
      return 1;
    }

  {
    GeglNode *graph, *sink;
    graph = gegl_graph (sink=gegl_node ("gegl:buffer-sink", "buffer", &bufferA, NULL,
                             gegl_node ("gegl:load", "path", argv[1], NULL)));
    gegl_node_process (sink);
    g_object_unref (graph);
    if (!bufferA)
      {
        g_print ("Failed top open %s\n", argv[1]);
        return 1;
      }

    graph = gegl_graph (sink=gegl_node ("gegl:buffer-sink", "buffer", &bufferB, NULL,
                             gegl_node ("gegl:load", "path", argv[2], NULL)));
    gegl_node_process (sink);
    g_object_unref (graph);
    if (!bufferB)
      {
        g_print ("Failed top open %s\n", argv[2]);
        return 1;
      }
  }

  if (gegl_buffer_get_width (bufferA) != gegl_buffer_get_width (bufferB) ||
      gegl_buffer_get_height (bufferA) != gegl_buffer_get_height (bufferB))
    {
      g_print ("%s and %s differ in size\n", argv[1], argv[2]);
      g_print ("  %ix%i vs %ix%i\n",
        gegl_buffer_get_width (bufferA), gegl_buffer_get_height (bufferA),
        gegl_buffer_get_width (bufferB), gegl_buffer_get_height (bufferB));
      return 1;
    }

  debug_buf = gegl_buffer_new (gegl_buffer_get_extent (bufferA), babl_format ("R'G'B' u8"));

   

  {
     gfloat *bufA, *bufB;
     gfloat *a, *b;
     guchar *debug, *d;
     gint   rowstrideA, rowstrideB, dRowstride;
     gint   pixels;
     gint   wrong_pixels=0;
     gint   i;
     gdouble diffsum = 0.0;
     gdouble max_diff = 0.0;

     pixels = gegl_buffer_get_pixel_count (bufferA);

     bufA = (void*)gegl_buffer_linear_open (bufferA, NULL, &rowstrideA,
                                            babl_format ("CIE Lab float"));
     bufB = (void*)gegl_buffer_linear_open (bufferB, NULL, &rowstrideB,
                                            babl_format ("CIE Lab float"));
     debug = (void*)gegl_buffer_linear_open (debug_buf, NULL, &dRowstride, babl_format ("R'G'B' u8"));

     a = bufA;
     b = bufB;
     d = debug;

     for (i=0;i<pixels;i++)
       {
#define P2(o) (((o))*((o)))
         gdouble diff = sqrt ( P2(a[0]-b[0])+
                               P2(a[1]-b[1])+
                               P2(a[2]-b[2])
                               /*+P2(a[3]-b[3])*/);
         if (diff>=0.01)
           {
             wrong_pixels++;
             diffsum += diff;
             if (diff > max_diff)
               max_diff = diff;
             d[0]=(diff/100.0 * 255);
             d[1]=0;
             d[2]=a[0]/100.0*255;
           }
         else
           {
             d[0]=a[0]/100.0*255;
             d[1]=a[0]/100.0*255;
             d[2]=a[0]/100.0*255;
           }
         a+=3;
         b+=3;
         d+=3;
       }

     a = bufA;
     b = bufB;
     d = debug;

     if (wrong_pixels)
       for (i=0;i<pixels;i++)
         {
           gdouble diff = sqrt ( P2(a[0]-b[0])+
                                 P2(a[1]-b[1])+
                                 P2(a[2]-b[2])
                                 /*+P2(a[3]-b[3])*/);
#undef P2
           if (diff>=0.01)
             {
               d[0]=(100-a[0])/100.0*64+32;
               d[1]=(diff/max_diff * 255);
               d[2]=0;
             }
           else
             {
               d[0]=a[0]/100.0*255;
               d[1]=a[0]/100.0*255;
               d[2]=a[0]/100.0*255;
             }
           a+=3;
           b+=3;
           d+=3;
         }

     gegl_buffer_linear_close (bufferA, bufA);
     gegl_buffer_linear_close (bufferB, bufB);
     gegl_buffer_linear_close (debug_buf, debug);

     if (max_diff >= 0.1)
       {
         g_print ("%s and %s differ\n"
                  "  wrong pixels   : %i/%i (%2.2f%%)\n"
                  "  max Δe         : %2.3f\n"
                  "  avg Δe (wrong) : %2.3f(wrong) %2.3f(total)\n",
                  argv[1], argv[2],
                  wrong_pixels, pixels, (wrong_pixels*100.0/pixels),
                  max_diff,
                  diffsum/wrong_pixels,
                  diffsum/pixels);
         if (max_diff > 1.5)
           {
             GeglNode *graph, *sink;
             gchar *debug_path = g_malloc (strlen (argv[2])+16);
             memcpy (debug_path, argv[2], strlen (argv[2])+1);
             memcpy (debug_path + strlen(argv[2])-4, "-diff.png", 11);
             graph = gegl_graph (sink=gegl_node ("gegl:png-save",
                                 "path", debug_path, NULL,
                                 gegl_node ("gegl:buffer-source", "buffer", debug_buf, NULL)));
             gegl_node_process (sink);
             return 1;
           }
         g_print ("we'll say ");
       }

  }

  g_print ("%s and %s are identical\n", argv[1], argv[2]);
  g_object_unref (debug_buf); 
  g_object_unref (bufferA); 
  g_object_unref (bufferB); 
  gegl_exit ();
  return 0;
}
