/* File: gegl-copy.c
 * Author: Daniel S. Rogers
 * Date: 21 February, 2003
 *Derived from gegl-fade.c
 */
#include "gegl-copy.h"
#include "gegl-scanline-processor.h"
#include "gegl-image-iterator.h"
#include "gegl-scalar-data.h"
#include "gegl-value-types.h"
#include "gegl-param-specs.h"
#include "gegl-utils.h"

/* for memcpy */
#include "string.h"

static void class_init (GeglCopyClass * klass);

static GeglScanlineFunc get_scanline_func(GeglUnary * unary, GeglColorSpaceType space, GeglChannelSpaceType type);

static void copy_general (GeglFilter * filter, GeglImageIterator ** iters, gint byte_width);
inline static void copy_float   (GeglFilter * filter, GeglImageIterator ** iters, gint width);
inline static void copy_uint8   (GeglFilter * filter, GeglImageIterator ** iters, gint width);
inline static void copy_uint16  (GeglFilter * filter, GeglImageIterator ** iters, gint width);

static gpointer parent_class = NULL;

GType
gegl_copy_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglCopyClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglCopy),
        0,
        (GInstanceInitFunc) NULL,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_UNARY, 
                                     "GeglCopy", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglCopyClass * klass)
{
  GeglUnaryClass *unary_class = GEGL_UNARY_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  unary_class->get_scanline_func = get_scanline_func;

}

/* scanline_funcs[data type] */
static GeglScanlineFunc scanline_funcs[] = 
{ 
  NULL, 
  copy_uint8, 
  copy_float, 
  copy_uint16 
};

static GeglScanlineFunc
get_scanline_func(GeglUnary * unary,
                  GeglColorSpaceType space,
                  GeglChannelSpaceType type)
{
  return scanline_funcs[type];
}

static void
copy_general (GeglFilter * filter,
              GeglImageIterator ** iters,
              gint byte_width)
{
  gpointer * d = gegl_image_iterator_color_channels(iters[0]);
  gpointer da = gegl_image_iterator_alpha_channel(iters[0]);
  gint d_color_chans = gegl_image_iterator_get_num_colors(iters[0]);

  gpointer * a = gegl_image_iterator_color_channels(iters[1]);
  gpointer aa = gegl_image_iterator_alpha_channel(iters[1]);
  gint a_color_chans = gegl_image_iterator_get_num_colors(iters[1]);

  gint alpha_mask = 0x0;

  if(aa)
    alpha_mask |= GEGL_A_ALPHA; 

  {
    gpointer d0 = (d_color_chans > 0) ? d[0]: NULL;   
    gpointer d1 = (d_color_chans > 1) ? d[1]: NULL;
    gpointer d2 = (d_color_chans > 2) ? d[2]: NULL;

    gpointer a0 = (a_color_chans > 0) ? a[0]: NULL;   
    gpointer a1 = (a_color_chans > 1) ? a[1]: NULL;
    gpointer a2 = (a_color_chans > 2) ? a[2]: NULL;
    

    /* I am not sure if it is save to assume here that op a has as
       many color channels as the destination.  I will check that
       before adding special cases.
    */
     
    switch(d_color_chans)
      {
          case 3: memcpy(d2,a2,byte_width);
          case 2: memcpy(d1,a1,byte_width);
          case 1: memcpy(d0,a0,byte_width);
      }

    if(alpha_mask == GEGL_A_ALPHA)
      {
        memcpy(da,aa,byte_width);
      }
  }

  g_free(d);
  g_free(a);
}

inline static void
copy_float (GeglFilter * filter,
            GeglImageIterator ** iters,
            gint width)
{
  copy_general(filter,iters,width * sizeof(gfloat));
}

inline static void
copy_uint8 (GeglFilter * filter,
            GeglImageIterator ** iters,
            gint width)
{
  copy_general(filter,iters,width * sizeof(guint8));
}

inline static void
copy_uint16 (GeglFilter * filter,
             GeglImageIterator ** iters,
             gint width)
{
  copy_general(filter,iters,width * sizeof(guint16));
}
