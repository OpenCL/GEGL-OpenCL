/* Video4Linux operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2004-2008 Øyvind Kolås <pippin@gimp.org>
 *                     originally written for gggl
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_file_path (path,   _("Path"), "/dev/video0", _("Path to v4l device"))
gegl_chant_int  (width,  _("Width"),  0, G_MAXINT, 320, _("Width for rendered image"))
gegl_chant_int  (height, _("Height"), 0, G_MAXINT, 240, _("Height for rendered image"))
gegl_chant_int  (frame,  _("Frame"),  0, G_MAXINT, 0,
        _("current frame number, can be changed to trigger a reload of the image."))
gegl_chant_int  (fps,    _("FPS"),  0, G_MAXINT, 0,
        _("autotrigger reload this many times a second."))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "v4l.c"

#include "gegl-chant.h"

#include "v4lutils/v4lutils.h"
#include "v4lutils/v4lutils.c"

typedef struct
{
  gint active;
  gint w;
  gint h;
  gint w_stored;
  gint h_stored;
  gint frame;
  gint decode;
  v4ldevice *vd;
} Priv;

static void
init (GeglChantO *o)
{
  Priv *p = (Priv*)o->chant_data;

  if (p==NULL)
    {
      p = g_new0 (Priv, 1);
      o->chant_data = (void*) p;
    }

  p->w = 320;
  p->h = 240;
}


static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result ={0,0,320,200};
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  result.width = o->width;
  result.height = o->height;
  return result;
}

static void
prepare (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  Priv *p= (Priv*)o->chant_data;

  if (p == NULL)
    init (o);
  p = (Priv*)o->chant_data;

  gegl_operation_set_format (operation, "output",
                            babl_format_new (
                                  babl_model ("R'G'B'"),
                                  babl_type ("u8"),
                                  babl_component ("B'"),
                                  babl_component ("G'"),
                                  babl_component ("R'"),
                                  NULL));

  p->w = o->width;
  p->h = o->height;

  if (!p->vd)
    {
      p->vd = g_malloc0 (sizeof (v4ldevice));

      if (v4lopen (o->path, p->vd))
        return;

      p->active = 1;

      if (v4lmmap (p->vd))
        return;

      v4lsetdefaultnorm (p->vd, VIDEO_MODE_PAL);
      v4lgetcapability (p->vd);

      if (!(p->vd->capability.type & VID_TYPE_CAPTURE)) {
          g_warning (
               "video_init: This device seems not to support video capturing.\n");
         return;
      }
    }

  if (p->w != p->w_stored || p->h != p->h_stored)
    {

      if (p->w > p->vd->capability.maxwidth
          || p->h > p->vd->capability.maxheight)
        {
          p->w = p->vd->capability.maxwidth;
          p->h = p->vd->capability.maxheight;
          o->width = p->w;
          o->height = p->h;
          g_warning ( "capturing size is set to %dx%d.\n", p->w, p->h);
        }
      else if (p->w < p->vd->capability.minwidth
               || p->h < p->vd->capability.minheight)
        {
          p->w = p->vd->capability.minwidth;
          p->h = p->vd->capability.minheight;
          o->width = p->w;
          o->height = p->h;
          g_warning ( "capturing size is set to %dx%d.\n", p->w, p->h);
        }

      p->w_stored = p->w;
      p->h_stored = p->h;

      /* FIXME: try other palettes as well, do some profiling on the spca
       * based cameras to see what is the ideal format wrt performance
       */

      if (!v4lsetpalette (p->vd, VIDEO_PALETTE_RGB24))
        {
           p->decode=0;
        }
      else if (!v4lsetpalette (p->vd, VIDEO_PALETTE_YUV420P))
        {
           p->decode=1;
        }
      else
        {
          g_warning ( "oops,. no usable v4l format found\n");
          return;
        }
      v4lgrabinit (p->vd, p->w, p->h);
      v4lgrabf (p->vd);
    }
}

static void
finalize (GObject *object)
{
 GeglChantO *o = GEGL_CHANT_PROPERTIES (object);

  if (o->chant_data)
    {
      Priv *p = (Priv*)o->chant_data;

      if (p->vd)
        {
          v4lmunmap (p->vd);
          v4lclose (p->vd);
          g_free (p->vd);
        }
      g_free (o->chant_data);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
}

static gboolean update (gpointer operation)
{
  GeglRectangle bounds = gegl_operation_get_bounding_box (operation);
  gegl_operation_invalidate (operation, &bounds, FALSE);
  return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  Priv       *p = (Priv*)o->chant_data;
  guchar     *capbuf;
  static gboolean inited = FALSE;

    if (!inited && o->fps != 0)
      {
        inited= TRUE;
        g_timeout_add (1000/o->fps, update, operation);
      }

  if (!p->active)
    return FALSE;

  v4lgrabf (p->vd);
  capbuf = v4lgetaddress (p->vd);
  v4lsyncf (p->vd);

  if (!capbuf)
    {
      g_warning ("no capbuf found");
      return FALSE;
    }

  if (p->decode)
    {
      guchar foobuf[o->width*o->height*3];
          /* XXX: foobuf is unneeded the conversions resets for every
           * scanline and could thus have been done in a line by line
           * manner an fed into the output buffer
           */
      gint y;
      for (y = 0; y < p->h; y++)
        {
          gint       x;

          guchar *dst = &foobuf[y*p->w*3];
          guchar *ysrc = (guchar *) (capbuf + (y) * (p->w) * 1);
          guchar *usrc = (guchar *) (capbuf + (p->w) * (p->h) + (y) / 2 * (p->w) / 2);
          guchar *vsrc = (guchar *) (capbuf + (p->w) * (p->h) + ((p->w) * (p->h)) / 4 + (y) / 2 * (p->w) / 2);

          for (x = 0; x < p->w; x++)
            {

              gint       R, G, B;

#ifndef byteclamp
#define byteclamp(j) do{if(j<0)j=0; else if(j>255)j=255;}while(0)
#endif

#define YUV82RGB8(Y,U,V,R,G,B)do{\
                  R= ((Y<<15)                 + 37355*(V-128))>>15;\
                  G= ((Y<<15) -12911* (U-128) - 19038*(V-128))>>15;\
                  B= ((Y<<15) +66454* (U-128)                )>>15;\
                  byteclamp(R);\
                  byteclamp(G);\
                  byteclamp(B);\
              }while(0)

      /* the format support for this code is not very good, but it
       * works for the devices I have tested with, conversion even
       * for chroma subsampled images is something we should let
       * babl handle.
       */
              YUV82RGB8 (*ysrc, *usrc, *vsrc, R, G, B);
              dst[0] = B;
              dst[1] = G;
              dst[2] = R;

              dst += 3;
              ysrc++;
              if (x % 2)
                {
                  usrc++;
                  vsrc++;
                }
            }
        }
      gegl_buffer_set (output, NULL, 0, NULL, foobuf, GEGL_AUTO_ROWSTRIDE);
    }
  else
    {
      gegl_buffer_set (output, NULL, 0, NULL, capbuf, GEGL_AUTO_ROWSTRIDE);
    }
  return  TRUE;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  G_OBJECT_CLASS (klass)->finalize = finalize;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process             = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;
  operation_class->prepare          = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:v4l",
    "categories"  , "input:video",
    "description" , _("Video4Linux input, webcams framegrabbers and similar devices."),
    NULL);
}


#endif
