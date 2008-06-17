#include <gegl.h>
#include <gegl-buffer.h>
#include <math.h>
#include <stdio.h>

/* This file consists of a testing suite for the GeglBuffer API. For every
 * function matching the regexp ^static.*(' in the file a test is performed and
 * the output is stored in in a file with the name of the function.
 *
 * The makefile contains shell scripting that provides knowledge of how much
 * passes the reference suite, testing should occur over a range of different
 * tile sizes to make sure the behavior is consistent.
 */

/* helper macros for the output, issue a test_start() after your defined
 * variables and the first logic, use print as your printf and print_buffer to
 * dump a GeglBuffer's contents to the log, issue test_end for the final
 * rendering.
 */

#define test_start()              GString *gstring=g_string_new("");\
                                  print ("Test: %s\n", __FUNCTION__)
#define print(arg...)             g_string_append_printf (gstring, arg)
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


/***********************************************************************/

static gchar * vertical_gradient ()
{
  GeglBuffer    *buffer;
  GeglRectangle  rect = {0, 0, 20, 20};

  test_start();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));
  vgrad (buffer);
  print_buffer (buffer);
  gegl_buffer_destroy (buffer);
  test_end();
}

static gchar * test_checkerboard ()
{
  GeglBuffer    *buffer;
  GeglRectangle  rect = {0, 0, 20, 20};

  test_start ();

  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));
  checkerboard (buffer, 3, 0.0, 1.0);
  print_buffer (buffer);
  gegl_buffer_destroy (buffer);

  test_end ();
}

static gchar * test_get_buffer_scaled ()
{
  GeglBuffer    *buffer;
  GeglRectangle  rect = {0, 0, 50, 50};
  GeglRectangle  getrect = {0, 0, 12, 8};
  guchar        *buf;

  test_start ();

  buffer = gegl_buffer_new (&rect, babl_format ("Y u8"));
  checkerboard (buffer, 2, 0.0, 1.0);
  buf = g_malloc (getrect.width*getrect.height*sizeof(gfloat));

    {
      gint i;

      for (i=0; i<10; i++)
        {
          getrect.x=i;
          /*getrect.y=i;*/
          gegl_buffer_get (buffer, 1.2, &getrect, babl_format ("Y u8"), buf, 0);
          print_linear_buffer_u8 (getrect.width, getrect.height, buf);
        }
    }

  gegl_buffer_destroy (buffer);

  g_free (buf);
  test_end ();
}

static gchar * test_get_buffer_scaled2 ()
{
  GeglBuffer    *buffer;
  GeglRectangle  rect = {0, 0, 10, 10};
  GeglRectangle  getrect = {-2, -2, 10, 10};
  guchar        *buf;

  test_start ();

  buffer = gegl_buffer_new (&rect, babl_format ("Y u8"));
  checkerboard (buffer, 2, 0.0, 1.0);
  buf = g_malloc (getrect.width*getrect.height*sizeof(gfloat));

  gegl_buffer_get (buffer, 0.66, &getrect, babl_format ("Y u8"), buf, 0);


  print_linear_buffer_u8 (getrect.width, getrect.height, buf);
  gegl_buffer_destroy (buffer);

  g_free (buf);
  test_end ();
}





static gchar * test_blank ()
{
  GeglBuffer    *buffer;
  GeglRectangle  rect = {0, 0, 20, 20};

  test_start ();

  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));
  print_buffer (buffer);
  gegl_buffer_destroy (buffer);

  test_end ();
}

static gchar * test_gray ()
{
  GeglBuffer    *buffer;
  GeglRectangle  rect = {0, 0, 20, 20};

  test_start ();

  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));
  fill (buffer, 0.5);
  print_buffer (buffer);
  gegl_buffer_destroy (buffer);

  test_end ();
}

static gchar * test_rectangle ()
{
  GeglBuffer    *buffer;
  GeglRectangle  rect = {0, 0, 20, 20};

  test_start ();

  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));
  fill (buffer, 0.5);
  rectangle (buffer, 5,5, 10, 10, 0.0);
  print_buffer (buffer);
  gegl_buffer_destroy (buffer);

  test_end ();
}

static gchar * sub_rect_fills_and_gets ()
{
  GeglBuffer    *buffer, *sub1, *sub2, *sub3;
  GeglRectangle  subrect1 = {5, 5, 10, 10};
  GeglRectangle  subrect2 = {8, 8, 30, 30};
  GeglRectangle  subrect3 = {-2, -2, 24, 24};
  GeglRectangle  rect = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));
  sub1 = gegl_buffer_create_sub_buffer (buffer, &subrect1);
  sub2 = gegl_buffer_create_sub_buffer (buffer, &subrect2);
  sub3 = gegl_buffer_create_sub_buffer (buffer, &subrect3);

  fill (sub1, 0.5);
  print ("root with sub1 filled in:\n");
  print_buffer (buffer);

  print ("sub2 before fill:\n");
  print_buffer (sub2);
  fill (sub2, 1.0);
  print ("final root:\n");
  print_buffer (buffer);
  print ("final sub1:\n");
  print_buffer (sub1);
  print ("final sub3:\n");
  print_buffer (sub3);

  gegl_buffer_destroy (sub1);
  gegl_buffer_destroy (sub2);
  gegl_buffer_destroy (sub3);
  gegl_buffer_destroy (buffer);
  test_end ();
}

static gchar * sub_sub_fill2 ()
{
  GeglBuffer    *buffer, *sub, *subsub;
  GeglRectangle  subrect =    {5, 5, 10, 10};
  GeglRectangle  subsubrect = {3, 3, 4, 4};
  GeglRectangle  rect =       {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));
  fill (buffer, 0.8);
  sub = gegl_buffer_create_sub_buffer (buffer, &subrect);

  fill (sub, 0.5);

  subsub = gegl_buffer_create_sub_buffer (sub, &subsubrect);
  fill (subsub, 1.0);
  print_buffer (buffer);
  print_buffer (subsub);
  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (subsub);
  gegl_buffer_destroy (buffer);
  test_end ();
}

static gchar * sub_sub_fill ()
{
  GeglBuffer    *buffer, *sub, *subsub;
  GeglRectangle  subrect =    {5, 5, 10, 10};
  GeglRectangle  subsubrect = {3, 3, 4, 4};
  GeglRectangle  rect =       {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));
  sub = gegl_buffer_create_sub_buffer (buffer, &subrect);

  fill (sub, 0.5);

  subsub = gegl_buffer_create_sub_buffer (sub, &subsubrect);
  fill (subsub, 1.0);
  print_buffer (buffer);
  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (subsub);
  gegl_buffer_destroy (buffer);
  test_end ();
}

static gchar * test_sample ()
{
  GeglBuffer    *buffer, *sub, *subsub, *subsubsub;
  GeglRectangle  subrect       = {5, 5, 10, 10};
  GeglRectangle  subsubrect    = {3, 3, 14, 14};
  GeglRectangle  subsubsubrect = {5, 3, 2, 2};
  GeglRectangle  rect =       {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  vgrad (buffer);

  sub = gegl_buffer_create_sub_buffer (buffer, &subrect);

  fill (sub, 0.5);

  subsub = gegl_buffer_create_sub_buffer (sub, &subsubrect);
  fill (subsub, 1.0);
  subsubsub = gegl_buffer_create_sub_buffer (buffer, &subsubsubrect);
  fill (subsubsub, 1.0);

  print_buffer (buffer);
  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (subsub);
  gegl_buffer_destroy (subsubsub);
  gegl_buffer_destroy (buffer);
  test_end ();
}

/* this functionality is not accesible through the normal (non gobject
 * constructor properties based) API
 */
static gchar * disabled_abyss ()
{
  GeglBuffer    *buffer, *sub, *subsub;
  GeglRectangle  subrect =    {5, 5, 10, 10};
  GeglRectangle  subsubrect = {3, 3, 4, 4};
  GeglRectangle  rect =       {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  sub = g_object_new (GEGL_TYPE_BUFFER,
                         "source", buffer,
                         "x", subrect.x,
                         "y", subrect.y,
                         "width", subrect.width,
                         "height", subrect.height,
                         "abyss-width", -1,
                         "abyss-height", -1,
                         NULL);
  fill (sub, 0.5);
  subsub = g_object_new (GEGL_TYPE_BUFFER,
                         "source", sub,
                         "x", subsubrect.x,
                         "y", subsubrect.y,
                         "width", subsubrect.width,
                         "height", subsubrect.height,
                         "abyss-width", -1,
                         "abyss-height", -1,
                         NULL);

  fill (subsub, 1.0);
  print_buffer (buffer);
  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (subsub);
  gegl_buffer_destroy (buffer);
  test_end ();
}

static gchar * buffer_inherit_parent_extent ()
{
  GeglBuffer    *buffer, *sub, *subsub;
  GeglRectangle  subrect =    {5, 5, 10, 10};
  GeglRectangle  subsubrect = {0, 0, -1, -1};
  GeglRectangle  rect =       {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  sub = gegl_buffer_create_sub_buffer (buffer, &subrect);
  fill (sub, 0.5);
  subsub = gegl_buffer_create_sub_buffer (sub, &subsubrect);
  fill (subsub, 1.0);
  print_buffer (buffer);

  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (subsub);
  gegl_buffer_destroy (buffer);
  test_end ();
}

static gchar * buffer_shift_vertical ()
{
  GeglBuffer    *buffer, *sub, *subsub;
  GeglRectangle  subrect =    {5, 5, 10, 10};
  GeglRectangle  rect =       {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  sub = gegl_buffer_create_sub_buffer (buffer, &subrect);
  fill (sub, 0.5);
  subsub = g_object_new (GEGL_TYPE_BUFFER,
                         "source", sub,
                         "x", 5,
                         "y", 5,
                         "width", 4,
                         "height", 4,
                         "shift-y", 5,
                         "shift-x", 0,
                         NULL);

  fill (subsub, 1.0);
  print_buffer (buffer);
  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (subsub);
  gegl_buffer_destroy (buffer);
  test_end ();
}

static gchar * buffer_shift_horizontal ()
{
  GeglBuffer    *buffer, *sub, *subsub;
  GeglRectangle  subrect =    {5, 5, 10, 10};
  GeglRectangle  rect =       {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  sub = gegl_buffer_create_sub_buffer (buffer, &subrect);
  fill (sub, 0.5);
  subsub = g_object_new (GEGL_TYPE_BUFFER,
                         "source", sub,
                         "x", 5,
                         "y", 5,
                         "width", 6,
                         "height", 6,
                         "shift-x", 8,
                         NULL);

  fill (subsub, 1.0);
  print_buffer (buffer);
  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (subsub);
  gegl_buffer_destroy (buffer);
  test_end ();
}

static gchar * buffer_shift_diagonal ()
{
  GeglBuffer    *buffer, *sub, *subsub;
  GeglRectangle  subrect =    {5, 5, 10, 10};
  GeglRectangle  rect =       {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  sub = gegl_buffer_create_sub_buffer (buffer, &subrect);
  fill (sub, 0.5);
  subsub = g_object_new (GEGL_TYPE_BUFFER,
                         "source", sub,
                         "x", 3,
                         "y", 3,
                         "width", 4,
                         "height", 4,
                         "shift-x", 6,
                         "shift-y", 6,
                         NULL);

  fill (subsub, 0.2);
  print_buffer (buffer);
  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (subsub);
  gegl_buffer_destroy (buffer);
  test_end ();
}

static gchar * get_shifted ()
{
  GeglBuffer    *buffer, *sub, *subsub, *foo;
  GeglRectangle  subrect =    {5, 5, 10, 10};
  GeglRectangle  foor =  {0, 0, 10, 10};
  GeglRectangle  rect =  {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  sub = gegl_buffer_create_sub_buffer (buffer, &subrect);
  vgrad (buffer);
  vgrad (sub);
  subsub = g_object_new (GEGL_TYPE_BUFFER,
                         "source", sub,
                         "x", 0,
                         "y", 0,
                         "width", 40,
                         "height", 40,
                         "shift-x", 0,
                         "shift-y", 0,
                         NULL);
  foo = gegl_buffer_create_sub_buffer (subsub, &foor);

  /*fill (subsub, 0.2);*/
  print_buffer (buffer);
  print_buffer (foo);
  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (subsub);
  gegl_buffer_destroy (buffer);
  gegl_buffer_destroy (foo);
  test_end ();
}


static gchar * test_gegl_buffer_copy_self ()
{
  GeglBuffer    *buffer;
  GeglRectangle  source = {2, 2, 2, 2};
  GeglRectangle  dest  = {15, 15, 1, 1};
  GeglRectangle  rect = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  vgrad (buffer);
  gegl_buffer_copy (buffer, &source, buffer, &dest); /* copying to self */
  print_buffer (buffer);
  gegl_buffer_destroy (buffer);
  test_end ();
}


static gchar * test_gegl_buffer_copy ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  GeglRectangle  source = {2, 2, 5, 5};
  GeglRectangle  dest = {10, 10, 5, 5};
  test_start ();
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  buffer2 = gegl_buffer_new (&bound, babl_format ("Y float"));

  vgrad (buffer);
  gegl_buffer_copy (buffer, &source, buffer2, &dest); 
  print_buffer (buffer2);
  gegl_buffer_destroy (buffer);
  gegl_buffer_destroy (buffer2);
  test_end ();
}

static gchar * test_gegl_buffer_copy_upper_left ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  vgrad (buffer);
  {
    GeglRectangle rect = *gegl_buffer_get_extent(buffer);

    rect.width-=10;
    rect.height-=10;
  buffer2 = gegl_buffer_new (gegl_buffer_get_extent (buffer), gegl_buffer_get_format (buffer));
  gegl_buffer_copy (buffer, &rect, buffer2, &rect);
  }
  print_buffer (buffer2);
  gegl_buffer_destroy (buffer);
  gegl_buffer_destroy (buffer2);
  test_end ();
}


static gchar * test_gegl_buffer_copy_upper_right ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  vgrad (buffer);
  {
    GeglRectangle rect = *gegl_buffer_get_extent(buffer);

    rect.x=10;
    rect.width-=10;
    rect.height-=10;
  buffer2 = gegl_buffer_new (gegl_buffer_get_extent (buffer), gegl_buffer_get_format (buffer));
  gegl_buffer_copy (buffer, &rect, buffer2, &rect);
  }
  print_buffer (buffer2);
  gegl_buffer_destroy (buffer);
  gegl_buffer_destroy (buffer2);
  test_end ();
}

static gchar * test_gegl_buffer_copy_lower_right ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  vgrad (buffer);
  {
    GeglRectangle rect = *gegl_buffer_get_extent(buffer);

    rect.x=10;
    rect.y=10;
    rect.width-=10;
    rect.height-=10;
  buffer2 = gegl_buffer_new (gegl_buffer_get_extent (buffer), gegl_buffer_get_format (buffer));
  gegl_buffer_copy (buffer, &rect, buffer2, &rect);
  }
  print_buffer (buffer2);
  gegl_buffer_destroy (buffer);
  gegl_buffer_destroy (buffer2);
  test_end ();
}

static gchar * test_gegl_buffer_copy_lower_left ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  vgrad (buffer);
  {
    GeglRectangle rect = *gegl_buffer_get_extent(buffer);

    rect.x=0;
    rect.y=10;
    rect.width-=10;
    rect.height-=10;
  buffer2 = gegl_buffer_new (gegl_buffer_get_extent (buffer), gegl_buffer_get_format (buffer));
  gegl_buffer_copy (buffer, &rect, buffer2, &rect);
  }
  print_buffer (buffer2);
  gegl_buffer_destroy (buffer);
  gegl_buffer_destroy (buffer2);
  test_end ();
}


static gchar * test_gegl_buffer_duplicate ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  vgrad (buffer);
  buffer2 = gegl_buffer_dup (buffer);
  print_buffer (buffer2);
  gegl_buffer_destroy (buffer);
  gegl_buffer_destroy (buffer2);
  test_end ();
}

#include "../../gegl/buffer/gegl-buffer-iterator.h"

static void fill_rect (GeglBuffer          *buffer,
                       const GeglRectangle *roi,
                       gfloat               value
                       )
{
  GeglBufferIterator *gi;
  gi = gegl_buffer_iterator_new (buffer, roi, NULL, GEGL_BUFFER_WRITE);
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

static gchar * test_gegl_buffer_iterator1 ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0,20,20};
  GeglRectangle roi = {0,0,20,20};
  test_start();
  buffer = gegl_buffer_new (&extent, babl_format ("Y float"));
  fill_rect (buffer, &roi, 0.5);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (buffer);
}
static gchar * test_gegl_buffer_iterator2 ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0,20,20};
  GeglRectangle roi = {0,0,10,10};
  test_start();
  buffer = gegl_buffer_new (&extent, babl_format ("Y float"));
  fill_rect (buffer, &roi, 0.5);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (buffer);
}
static gchar * test_gegl_buffer_iterator3 ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0,20,20};
  GeglRectangle roi = {5,5,10,10};
  test_start();
  buffer = gegl_buffer_new (&extent, babl_format ("Y float"));
  fill_rect (buffer, &roi, 0.5);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (buffer);
}
static gchar * test_gegl_buffer_iterator4 ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0,20,20};
  GeglRectangle roi = {1,1,10,10};
  test_start();
  buffer = gegl_buffer_new (&extent, babl_format ("Y float"));
  fill_rect (buffer, &roi, 0.5);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (buffer);
}


static gchar * test_gegl_buffer_iterator1sub ()
{
  GeglBuffer   *buffer;
  GeglBuffer   *sub;
  GeglRectangle extent = {0,0,20,20};
  GeglRectangle sextent = {2,2,20,20};
  GeglRectangle roi = {0,0,20,20};
  test_start();
  buffer = gegl_buffer_new (&extent, babl_format ("Y float"));
  sub = gegl_buffer_create_sub_buffer (buffer, &sextent);

  fill_rect (sub, &roi, 0.5);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (buffer);
}
static gchar * test_gegl_buffer_iterator2sub ()
{
  GeglBuffer   *buffer;
  GeglBuffer   *sub;
  GeglRectangle extent = {0,0,20,20};
  GeglRectangle sextent = {2,2,20,20};
  GeglRectangle roi = {0,0,10,10};
  test_start();
  buffer = gegl_buffer_new (&extent, babl_format ("Y float"));
  sub = gegl_buffer_create_sub_buffer (buffer, &sextent);
  fill_rect (sub, &roi, 0.5);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (buffer);
}
static gchar * test_gegl_buffer_iterator3sub ()
{
  GeglBuffer   *buffer;
  GeglBuffer   *sub;
  GeglRectangle extent = {0,0,20,20};
  GeglRectangle sextent = {2,2,20,20};
  GeglRectangle roi = {5,5,10,10};
  test_start();
  buffer = gegl_buffer_new (&extent, babl_format ("Y float"));
  sub = gegl_buffer_create_sub_buffer (buffer, &sextent);
  fill_rect (sub, &roi, 0.5);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (buffer);
}
static gchar * test_gegl_buffer_iterator4sub ()
{
  GeglBuffer   *buffer;
  GeglBuffer   *sub;
  GeglRectangle extent = {0,0,20,20};
  GeglRectangle sextent = {2,2,20,20};
  GeglRectangle roi = {1,1,10,10};
  test_start();
  buffer = gegl_buffer_new (&extent, babl_format ("Y float"));
  sub = gegl_buffer_create_sub_buffer (buffer, &sextent);
  fill_rect (sub, &roi, 0.5);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (buffer);
}

#include "../../gegl/buffer/gegl-buffer-private.h"

static gchar * linear_new ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0,40,20};
  GeglRectangle roi = {1,1,30,10};
  test_start();
  g_print ("foo!\n");
  buffer = gegl_buffer_linear_new (&extent, babl_format ("Y float"));
  fill_rect (buffer, &roi, 0.5);
  roi.y+=3;
  roi.x+=20;
  fill_rect (buffer, &roi, 0.2);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (buffer);
}


static gchar * linear_modify ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0,40,20};
  GeglRectangle roi = {1,1,30,10};
  test_start();
  buffer = gegl_buffer_linear_new (&extent, babl_format ("Y float"));
  fill_rect (buffer, &roi, 0.5);
  roi.y+=3;
  roi.x+=20;

  {
    gint    rowstride;
    gfloat *buf;
    gint    x, y, i;

    buf = (gpointer)gegl_buffer_linear_open (buffer, &extent, &rowstride, NULL);
    g_assert (buf);

    i=0;
    for (y=0;y<extent.height;y++)
      for (x=0;x<extent.width;x++)
        {
          buf[i++]= ((x+y)*1.0) / extent.width;
        }
    gegl_buffer_linear_close (buffer, buf);
  }
  fill_rect (buffer, &roi, 0.2);

  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (buffer);
}

static gchar * linear_from_data ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0, 10, 10};
  gfloat       *buf;
  test_start();

  buf = g_malloc (sizeof (float) * 10 * 10);
  gint i;
  for (i=0;i<100;i++)
    buf[i]=i/100.0;

  buffer = gegl_buffer_linear_new_from_data (buf, babl_format ("Y float"),
                                             &extent,
                                             10 * 4,
                                             G_CALLBACK(g_free), /* destroy_notify */
                                             NULL   /* destroy_notify_data */);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (buffer);
}

static gchar * linear_from_data_rows ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0, 10, 10};
  gfloat       *buf;
  test_start();

  buf = g_malloc (sizeof (float) * 12 * 10);
  gint i;
  for (i=0;i<120;i++)
    buf[i]=i%12==5?0.5:0.0;

  buffer = gegl_buffer_linear_new_from_data (buf, babl_format ("Y float"),
                                             &extent,
                                             12 * 4,
                                             G_CALLBACK(g_free), /* destroy_notify */
                                             NULL   /* destroy_notify_data */);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (buffer);
}


static gchar * linear_proxy_modify ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0,40,20};
  GeglRectangle roi = {1,1,30,10};
  test_start();
  buffer = gegl_buffer_new (&extent, babl_format ("Y float"));
  fill_rect (buffer, &roi, 0.5);
  roi.y+=3;
  roi.x+=20;

  {
    gint    rowstride;
    gfloat *buf;
    gint    x, y, i;

    buf = (gpointer)gegl_buffer_linear_open (buffer, &extent, &rowstride, NULL);
    g_assert (buf);

    i=0;
    for (y=0;y<extent.height;y++)
      for (x=0;x<extent.width;x++)
        {
          buf[i++]= ((x+y)*1.0) / extent.width;
        }
    gegl_buffer_linear_close (buffer, buf);
  }
  fill_rect (buffer, &roi, 0.2);

  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (buffer);
}


/**************************************************************************/

static void
print_linear_buffer_internal_float (GString    *gstring,
                                    gint        width,
                                    gint        height,
                                    gfloat     *buf)
{
  gchar *scale[]={" ", "░", "▒", "▓", "█", "█", "!"};
  gint x,y;
  print ("▛");
  for (x=0;x<width;x++)
    print ("▀");
  print ("▜\n");
  for (y=0;y<height;y++)
    {
      print ("▌");
      for (x=0;x<width;x++)
        {
          gint val = floor ( buf[y*width+x] * 4 + 0.5);
          if (val>4)
            val=4;
          if (val<0)
            val=0;
          print ("%s", scale[val]);
        }
      print ("▐\n");
    }
  print ("▙");
  for (x=0;x<width;x++)
    print ("▄");
  print ("▟\n");
}

static void
print_linear_buffer_internal_u8 (GString    *gstring,
                                 gint        width,
                                 gint        height,
                                 guchar     *buf)
{
  gchar *scale[]={" ", "░", "▒", "▓", "█"};
  gint x,y;
  print ("▛");
  for (x=0;x<width;x++)
    print ("▀");
  print ("▜\n");
  for (y=0;y<height;y++)
    {
      print ("▌");
      for (x=0;x<width;x++)
        print ("%s", scale[ (gint)floor ( buf[y*width+x]/256.0 * 4 + 0.5)]);
      print ("▐\n");
    }
  print ("▙");
  for (x=0;x<width;x++)
    print ("▄");
  print ("▟\n");
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
  gegl_buffer_get (buffer, 1.0, NULL, babl_format ("Y float"), buf, 0);
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
  gegl_buffer_get (buffer, 1.0, NULL, babl_format ("Y float"), buf, 0);

  i=0;
  for (y=0;y<height;y++)
    {
      for (x=0;x<width;x++)
        {
          buf[i++]=value;
        }
    }
  gegl_buffer_set (buffer, NULL, babl_format ("Y float"), buf, GEGL_AUTO_ROWSTRIDE);
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
  gegl_buffer_get (buffer, 1.0, NULL, babl_format ("Y float"), buf, 0);

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

  gegl_buffer_set (buffer, NULL, babl_format ("Y float"), buf, GEGL_AUTO_ROWSTRIDE);

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
  gegl_buffer_get (buffer, 1.0, NULL, babl_format ("Y float"), buf, 0);

  i=0;
  for (y=0;y<height;y++)
    {
      for (x=0;x<width;x++)
        {
          buf[i++]= (1.0*y)/height;
        }
    }
  gegl_buffer_set (buffer, NULL, babl_format ("Y float"), buf, GEGL_AUTO_ROWSTRIDE);
  g_free (buf);
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
  gegl_buffer_destroy (sub_buf);
}

#include "buffer-tests.inc"

gint main (gint argc, gchar **argv)
{
  gint i;
  gegl_init (&argc, &argv);

  /* make tests dir */
  system ("mkdir output > /dev/null 2>&1");


  for (i=0; i<sizeof(tests)/sizeof(tests[0]);i++)
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
                  printf ("%s", tests[i]());
                }
            }
        }
      else
        {
          gchar output_file[1024];
          printf ("%s ", test_names[i]);
          ret=tests[i]();
          sprintf (output_file, "output/%s", test_names[i]);
          g_file_set_contents (output_file, ret, -1, NULL);
        }
    }

  gegl_exit ();
  return 0;
}
