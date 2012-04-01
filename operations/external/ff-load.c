/* This file is an image processing operation for GEGL
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
 * Copyright 2003, 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_file_path (path, _("File"), "/home/pippin/input.avi", _("Path of file to load"))
gegl_chant_int (frame, _("Frame"), 0, 1000000, 0, _("Frame number"))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "ff-load.c"

#include "gegl-chant.h"
#include <errno.h>

#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
#include <libavformat/avformat.h>
#else
#include <avformat.h>
#endif

typedef struct
{
  gdouble          frames;
  gint             width;
  gint             height;
  gdouble          fps;
  gchar           *codec_name;
  gchar           *fourcc;

  int              video_stream;
  AVFormatContext *ic;
  AVStream        *video_st;
  AVCodecContext  *enc;
  AVCodec         *codec;
  AVPacket         pkt;
  AVFrame         *lavc_frame;

  glong            coded_bytes;
  guchar          *coded_buf;

  gchar           *loadedfilename; /* to remember which file is "cached"     */
  glong            prevframe;      /* previously decoded frame in loadedfile */
} Priv;


static void
print_error (const char *filename, int err)
{
  switch (err)
    {
#if LIBAVFORMAT_VERSION_MAJOR >= 53
    case AVERROR(EINVAL):
#else
    case AVERROR_NUMEXPECTED:
#endif
      g_warning ("%s: Incorrect image filename syntax.\n"
                 "Use '%%d' to specify the image number:\n"
                 "  for img1.jpg, img2.jpg, ..., use 'img%%d.jpg';\n"
                 "  for img001.jpg, img002.jpg, ..., use 'img%%03d.jpg'.\n",
                 filename);
      break;
    case AVERROR_INVALIDDATA:
#if LIBAVFORMAT_VERSION_MAJOR >= 53
      g_warning ("%s: Error while parsing header or unknown format\n", filename);
#else
      g_warning ("%s: Error while parsing header\n", filename);
      break;
    case AVERROR_NOFMT:
      g_warning ("%s: Unknown format\n", filename);
#endif
      break;
    default:
      g_warning ("%s: Error while opening file\n", filename);
      break;
    }
}

static void
init (GeglChantO *o)
{
  static gint inited = 0; /*< this is actually meant to be static, only to be done once */
  Priv       *p = (Priv*)o->chant_data;

  if (p==NULL)
    {
      p = g_new0 (Priv, 1);
      o->chant_data = (void*) p;
    }

  p->width = 320;
  p->height = 200;

  if (!inited)
    {
      av_register_all ();
      avcodec_register_all ();
      inited = 1;
    }
  p->loadedfilename = g_strdup ("");
  p->fourcc = g_strdup ("");
  p->codec_name = g_strdup ("");
}

/* FIXME: probably some more stuff to free here */
static void
ff_cleanup (GeglChantO *o)
{
  Priv *p = (Priv*)o->chant_data;
  if (p)
    {
      if (p->codec_name)
        g_free (p->codec_name);
      if (p->loadedfilename)
        g_free (p->loadedfilename);

      if (p->enc)
        avcodec_close (p->enc);
      if (p->ic)
        av_close_input_file (p->ic);
      if (p->lavc_frame)
        av_free (p->lavc_frame);

      p->enc = NULL;
      p->ic = NULL;
      p->lavc_frame = NULL;
      p->codec_name = NULL;
      p->loadedfilename = NULL;
    }
}

static glong
prev_keyframe (Priv *priv, glong frame)
{
  /* no way to detect previous keyframe at the moment for ffmpeg,
     so we'll just return 0, the first, and a forced reload happens
     if needed
   */
  return 0;
}

static int
decode_frame (GeglOperation *operation,
              glong          frame)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  Priv       *p = (Priv*)o->chant_data;
  glong       prevframe = p->prevframe;
  glong       decodeframe;        /*< frame to be requested decoded */

  if (frame >= p->frames)
    {
      frame = p->frames - 1;
    }

  if (frame < 0)
    {
      frame = 0;
    }

  if (frame == prevframe)
    {
      return 0;
    }

  /* figure out which frame we should start decoding at */

  if (frame == prevframe + 1)
    {
      decodeframe = prevframe + 1;
    }
  else
    {
      decodeframe = prev_keyframe (p, frame);
      if (prevframe > decodeframe && prevframe < frame)
        decodeframe = prevframe + 1;
    }

  if (decodeframe < prevframe)
    {
      /* seeking backwards, since it ffmpeg doesn't allow us,. we'll reload the file */
      g_free (p->loadedfilename);
      p->loadedfilename = NULL;
      init (o);
    }

  while (decodeframe <= frame)
    {
      int       got_picture = 0;

      do
        {
          int       decoded_bytes;

          if (p->coded_bytes <= 0)
            {
              do
                {
                  if (av_read_packet (p->ic, &p->pkt) < 0)
                    {
                      fprintf (stderr, "av_read_packet failed for %s\n",
                               o->path);
                      return -1;
                    }
                }
              while (p->pkt.stream_index != p->video_stream);

              p->coded_bytes = p->pkt.size;
              p->coded_buf = p->pkt.data;
            }
          decoded_bytes =
            avcodec_decode_video2 (p->video_st->codec, p->lavc_frame,
                                  &got_picture, &p->pkt);
          if (decoded_bytes < 0)
            {
              fprintf (stderr, "avcodec_decode_video failed for %s\n",
                       o->path);
              return -1;
            }

          p->coded_buf += decoded_bytes;
          p->coded_bytes -= decoded_bytes;
        }
      while (!got_picture);

      decodeframe++;
    }
  p->prevframe = frame;
  return 0;
}

static void
prepare (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  Priv       *p = (Priv*)o->chant_data;

  if (p == NULL)
    init (o);
  p = (Priv*)o->chant_data;

  g_assert (o->chant_data != NULL);

  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A u8"));


  if (!p->loadedfilename ||
      strcmp (p->loadedfilename, o->path))
    {
      gint i;
      gint err;

      ff_cleanup (o);
      err = av_open_input_file (&p->ic, o->path, NULL, 0, NULL);
      if (err < 0)
        {
          print_error (o->path, err);
        }
      err = av_find_stream_info (p->ic);
      if (err < 0)
        {
          g_warning ("ff-load: error finding stream info for %s", o->path);

          return;
        }
      for (i = 0; i< p->ic->nb_streams; i++)
        {
          AVCodecContext *c = p->ic->streams[i]->codec;
#if LIBAVFORMAT_VERSION_MAJOR >= 53
          if (c->codec_type == AVMEDIA_TYPE_VIDEO)
#else
          if (c->codec_type == CODEC_TYPE_VIDEO)
#endif
            {
              p->video_st = p->ic->streams[i];
              p->video_stream = i;
            }
        }

      p->enc = p->video_st->codec;
      p->codec = avcodec_find_decoder (p->enc->codec_id);

      /* p->enc->error_resilience = 2; */
      p->enc->error_concealment = 3;
      p->enc->workaround_bugs = FF_BUG_AUTODETECT;

      if (p->codec == NULL)
        {
          g_warning ("codec not found");
        }

      if (p->codec->capabilities & CODEC_CAP_TRUNCATED)
        p->enc->flags |= CODEC_FLAG_TRUNCATED;

      if (avcodec_open (p->enc, p->codec) < 0)
        {
          g_warning ("error opening codec %s", p->enc->codec->name);
          return;
        }

      p->width = p->enc->width;
      p->height = p->enc->height;
      p->frames = 10000000;
      p->lavc_frame = avcodec_alloc_frame ();

      if (p->fourcc)
        g_free (p->fourcc);
      p->fourcc = g_strdup ("none");
          p->fourcc[0] = (p->enc->codec_tag) & 0xff;
      p->fourcc[1] = (p->enc->codec_tag >> 8) & 0xff;
      p->fourcc[2] = (p->enc->codec_tag >> 16) & 0xff;
      p->fourcc[3] = (p->enc->codec_tag >> 24) & 0xff;

      if (p->codec_name)
        g_free (p->codec_name);
      if (p->codec->name)
        {
          p->codec_name = g_strdup (p->codec->name);
        }
      else
        {
          p->codec_name = g_strdup ("");
        }

      if (p->loadedfilename)
        g_free (p->loadedfilename);
      p->loadedfilename = g_strdup (o->path);
      p->prevframe = -1;
      p->coded_bytes = 0;
      p->coded_buf = NULL;
    }
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = {0,0,320,200};
  Priv *p = (Priv*)GEGL_CHANT_PROPERTIES (operation)->chant_data;
  result.width = p->width;
  result.height = p->height;
  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  Priv       *p = (Priv*)o->chant_data;

  {
    if (p->ic && !decode_frame (operation, o->frame))
      {
        guchar *buf;
        gint    pxsize;
        gint    x,y;


        g_object_get (output, "px-size", &pxsize, NULL);
        buf = g_new (guchar, p->width * p->height * pxsize);

        for (y=0; y < p->height; y++)
          {
            guchar       *dst  = buf + y * p->width * 4;
            const guchar *ysrc = p->lavc_frame->data[0] + y * p->lavc_frame->linesize[0];
            const guchar *usrc = p->lavc_frame->data[1] + y/2 * p->lavc_frame->linesize[1];
            const guchar *vsrc = p->lavc_frame->data[2] + y/2 * p->lavc_frame->linesize[2];

            for (x=0;x < p->width; x++)
              {
                gint R,G,B;
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
              } while(0)

              YUV82RGB8 (*ysrc, *usrc, *vsrc, R, G, B);

              *(unsigned int *) dst = R + G * 256 + B * 256 * 256 + 0xff000000;
              dst += 4;
              ysrc ++;
              if (x % 2)
                {
                  usrc++;
                  vsrc++;
                }
              }
          }
        gegl_buffer_set (output, NULL, 0, NULL, buf, GEGL_AUTO_ROWSTRIDE);
        g_free (buf);
      }
  }
  return  TRUE;
}

static void
finalize (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);

  if (o->chant_data)
    {
      Priv *p = (Priv*)o->chant_data;

      g_free (p->loadedfilename);
      g_free (p->fourcc);
      g_free (p->codec_name);

      g_free (o->chant_data);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
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

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:ff-load",
    "categories"  , "input:video",
    "description" , _("FFmpeg video frame importer"),
    NULL);
}

#endif
