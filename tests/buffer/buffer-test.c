#include "config.h"
#include <gegl.h>
#include <gegl-buffer.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>

/* This file consists of a testing suite for the GeglBuffer API. For every
 * function matching the regexp ^static.*(' in the file a test is performed and
 * the output is stored in in a file with the name of the function.
 *
 * The makefile contains shell scripting that provides knowledge of how much
 * passes the reference suite, testing should occur over a range of different
 * tile sizes to make sure the behavior is consistent.
 */

#include "../../gegl/buffer/gegl-buffer-iterator.h"



/* helper macros for the output, issue a test_start() after your defined
 * variables and the first logic, use print as your printf and print_buffer to
 * dump a GeglBuffer's contents to the log, issue test_end for the final
 * rendering.
 */

#define test_start()              GString *gstring=g_string_new("");\
                                  print (("Test: %s\n", G_STRINGIFY (TESTNAME)))
#define print(args)		  G_STMT_START {		\
	gchar *_fmt = g_strdup_printf args;			\
	g_string_append (gstring, _fmt);			\
	g_free (_fmt);						\
				  } G_STMT_END
#define print_buffer(buffer)      print_buffer_internal (gstring, buffer)
#define print_linear_buffer_u8(w,h,b)  print_linear_buffer_internal_u8 (gstring,w,h,b)
#define print_linear_buffer_float(w,h,b)  print_linear_buffer_internal_float (gstring,w,h,b)
#define test_end()                return g_string_free (gstring, FALSE)


static void print_buffer_internal (GString    *gstring,
                                   GeglBuffer *buffer);
static void
print_linear_buffer_internal_float (GString    *gstring,
                                    gint        width,
                                    gint        height,
                                    gfloat     *buf);
static void
print_linear_buffer_internal_u8    (GString    *gstring,
                                    gint        width,
                                    gint        height,
                                    guchar     *buf);

static void checkerboard          (GeglBuffer *buffer,
                                   gint        cellsize,
                                   gfloat      val1,
                                   gfloat      val2);

static void fill                  (GeglBuffer *buffer,
                                   gfloat      value);

static void vgrad                 (GeglBuffer *buffer);

static void rectangle             (GeglBuffer *buffer,
                                   gint        x,
                                   gint        y,
                                   gint        width,
                                   gint        height,
                                   gfloat      value);

static void fill_rect (GeglBuffer          *buffer,
                       const GeglRectangle *roi,
                       gfloat               value
                       );



/***********************************************************************/
/**************************************************************************/

static void
print_linear_buffer_internal_float (GString    *gstring,
                                    gint        width,
                                    gint        height,
                                    gfloat     *buf)
{
  gchar *scale[]={" ", "░", "▒", "▓", "█", "█"};
  gint x,y;
  print (("▛"));
  for (x=0;x<width;x++)
    print (("▀"));
  print (("▜\n"));
  for (y=0;y<height;y++)
    {
      print (("▌"));
      for (x=0;x<width;x++)
        {
          gint val = floor ( buf[y*width+x] * 4 + 0.5);
          if (val>4)
            val=4;
          if (val<0)
            val=0;
          print (("%s", scale[val]));
        }
      print (("▐\n"));
    }
  print (("▙"));
  for (x=0;x<width;x++)
    print (("▄"));
  print (("▟\n"));
}

static void
print_linear_buffer_internal_u8 (GString    *gstring,
                                 gint        width,
                                 gint        height,
                                 guchar     *buf)
{
  gchar *scale[]={" ", "░", "▒", "▓", "█"};
  gint x,y;
  print (("▛"));
  for (x=0;x<width;x++)
    print (("▀"));
  print (("▜\n"));
  for (y=0;y<height;y++)
    {
      print (("▌"));
      for (x=0;x<width;x++)
        print (("%s", scale[ (gint)floor ( buf[y*width+x]/256.0 * 4 + 0.5)]));
      print (("▐\n"));
    }
  print (("▙"));
  for (x=0;x<width;x++)
    print (("▄"));
  print (("▟\n"));
}


static void
print_buffer_internal (GString    *gstring,
                       GeglBuffer *buffer)
{
  gfloat *buf;
  gint width, height, x0, y0;
  g_object_get (buffer, "x", &x0,
                        "y", &y0,
                        "width", &width,
                        "height", &height,
                        NULL);
  buf = g_malloc (width*height*sizeof(gfloat));
  gegl_buffer_get (buffer, NULL, 1.0, babl_format ("Y float"), buf, 0,
                   GEGL_ABYSS_NONE);
  print_linear_buffer_internal_float (gstring, width, height, buf);
  g_free (buf);
}

static void
fill (GeglBuffer *buffer,
      gfloat      value)
{
  gfloat *buf;
  gint x,y;
  gint i;
  gint width, height, x0, y0;
  g_object_get (buffer, "x", &x0,
                        "y", &y0,
                        "width", &width,
                        "height", &height,
                        NULL);
  buf = g_malloc (width*height*sizeof(gfloat));
  gegl_buffer_get (buffer, NULL, 1.0, babl_format ("Y float"), buf, 0,
                   GEGL_ABYSS_NONE);

  i=0;
  for (y=0;y<height;y++)
    {
      for (x=0;x<width;x++)
        {
          buf[i++]=value;
        }
    }
  gegl_buffer_set (buffer, NULL, 0, babl_format ("Y float"), buf, GEGL_AUTO_ROWSTRIDE);
  g_free (buf);
}

static void checkerboard          (GeglBuffer *buffer,
                                   gint        cellsize,
                                   gfloat      val1,
                                   gfloat      val2)
{
  gfloat *buf;
  gint x,y;
  gint i;
  gint width, height, x0, y0;
  g_object_get (buffer, "x", &x0,
                        "y", &y0,
                        "width", &width,
                        "height", &height,
                        NULL);
  buf = g_malloc (width*height*sizeof(gfloat));
  gegl_buffer_get (buffer, NULL, 1.0, babl_format ("Y float"), buf, 0,
                   GEGL_ABYSS_NONE);

  i=0;
  for (y=0;y<height;y++)
    {
      for (x=0;x<width;x++)
        {
          gfloat val=val1;
          if ( (x/cellsize) % 2)
            {
              if ( (y/cellsize) % 2)
                {
                  val=val2;
                }
            }
          else
            {
              if ( (y/cellsize) % 2 == 0)
                {
                  val=val2;
                }
            }
          buf[i++]= val;
        }
    }

  gegl_buffer_set (buffer, NULL, 0, babl_format ("Y float"), buf, GEGL_AUTO_ROWSTRIDE);

  g_free (buf);
}

static void vgrad (GeglBuffer *buffer)
{
  gfloat *buf;
  gint x,y;
  gint i;
  gint width, height, x0, y0;
  g_object_get (buffer, "x", &x0,
                        "y", &y0,
                        "width", &width,
                        "height", &height,
                        NULL);
  buf = g_malloc (width*height*sizeof(gfloat));
  gegl_buffer_get (buffer, NULL, 1.0, babl_format ("Y float"), buf, 0,
                   GEGL_ABYSS_NONE);

  i=0;
  for (y=0;y<height;y++)
    {
      for (x=0;x<width;x++)
        {
          buf[i++]= (1.0*y)/height;
        }
    }
  gegl_buffer_set (buffer, NULL, 0, babl_format ("Y float"), buf, GEGL_AUTO_ROWSTRIDE);
  g_free (buf);
}

static void fill_rect (GeglBuffer          *buffer,
                       const GeglRectangle *roi,
                       gfloat               value
                       )
{
  GeglBufferIterator *gi;
  gi = gegl_buffer_iterator_new (buffer, roi, 0, NULL,
                                 GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
  while (gegl_buffer_iterator_next (gi))
    {
      gfloat *buf = gi->data[0];
      gint    i;
      for (i=0; i<gi->length; i++)
        {
          buf[i]=value;
        }
    }
}

void rectangle (GeglBuffer *buffer,
                gint        x,
                gint        y,
                gint        width,
                gint        height,
                gfloat      value)
{
  GeglBuffer *sub_buf;
  GeglRectangle rect={x,y,width,height};

  sub_buf = gegl_buffer_create_sub_buffer (buffer, &rect);
  fill (sub_buf, value);
  g_object_unref (sub_buf);
}

#include "buffer-tests.inc"

gint main (gint argc, gchar **argv)
{
  gint i;
  g_thread_init (NULL);
  gegl_init (&argc, &argv);

  for (i=0; i < G_N_ELEMENTS (tests); i++)
    {
      gchar *ret;

      if (argc > 1)
        {
          /* handle any extra commandline options as a list of tests to
           * run and output to standard output
           */
          gint j;
          for (j=1;j<argc;j++)
            {
              if (g_str_equal (argv[j], test_names[i]))
                {
                  ret=tests[i]();
                  printf ("%s", ret);
                  g_free (ret);
                }
            }
        }
      else
        {
          gchar output_file[1024];
          printf ("%s ", test_names[i]);
          ret=tests[i]();
          sprintf (output_file, "output/%s.buf", test_names[i]);
          g_file_set_contents (output_file, ret, -1, NULL);
          g_free (ret);
        }
    }

  gegl_exit ();
  return 0;
}
