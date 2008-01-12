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
#if GEGL_CHANT_PROPERTIES

gegl_chant_path (path, "/home/pippin/ed.avi", "Path of file to load.")
gegl_chant_int (frame, 0, 1000000, 0, "frame number")

#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME           ff_load
#define GEGL_CHANT_DESCRIPTION    "FFmpeg video frame importer."

#define GEGL_CHANT_SELF           "ff-load.c"
#define GEGL_CHANT_CATEGORIES     "input:video"
#define GEGL_CHANT_INIT
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"

#include <errno.h>
#include <ffmpeg/avformat.h>

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
    case AVERROR_NUMEXPECTED:
      g_warning ("%s: Incorrect image filename syntax.\n"
                 "Use '%%d' to specify the image number:\n"
                 "  for img1.jpg, img2.jpg, ..., use 'img%%d.jpg';\n"
                 "  for img001.jpg, img002.jpg, ..., use 'img%%03d.jpg'.\n",
                 filename);
      break;
    case AVERROR_INVALIDDATA:
      g_warning ("%s: Error while parsing header\n", filename);
      break;
    case AVERROR_NOFMT:
      g_warning ("%s: Unknown format\n", filename);
      break;
    default:
      g_warning ("%s: Error while opening file\n", filename);
      break;
    }
}

static void
init (GeglChantOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  Priv               *p = (Priv*)self->priv;
  static gint         inited = 0; /*< this is actually meant to be static, only to be done once */
  if (p==NULL)
    {
      p = g_malloc0 (sizeof (Priv));
      self->priv = (void*) p;
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
ff_cleanup (GeglChantOperation *operation)
{
  Priv *p = (Priv*)operation->priv;
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


static void
finalize (GObject *object)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (object);
  if (self->priv)
    {
      ff_cleanup (self);
      g_free (self->priv);
      self->priv = NULL;
    }

  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->finalize (object);
}

static void
prepare (GeglOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  Priv *p= (Priv*)self->priv;

  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A u8"));

  if (!p->loadedfilename ||
      strcmp (p->loadedfilename, self->path))
    {
      gint i;
      gint err;

      ff_cleanup (self);
      err = av_open_input_file (&p->ic, self->path, NULL, 0, NULL);
      if (err < 0)
        {
          print_error (self->path, err);
        }
      err = av_find_stream_info (p->ic);
      if (err < 0)
        {
          g_warning ("ff-load: error finding stream info for %s", self->path);

          return;
        }
      for (i = 0; i< p->ic->nb_streams; i++)
        {
          AVCodecContext *c = p->ic->streams[i]->codec;
          if (c->codec_type == CODEC_TYPE_VIDEO)
            {
              p->video_st = p->ic->streams[i];
              p->video_stream = i;
            }
        }

      p->enc = p->video_st->codec;
      p->codec = avcodec_find_decoder (p->enc->codec_id);

      p->enc->error_resilience = 2;
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
      p->loadedfilename = g_strdup (self->path);
      p->prevframe = -1;
      p->coded_bytes = 0;
      p->coded_buf = NULL;
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
decode_frame (GeglChantOperation *op,
              glong               frame)
{
  Priv     *p = (Priv*)op->priv;
  glong     prevframe = p->prevframe;
  glong     decodeframe;        /*< frame to be requested decoded */

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
      init (op);
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
                               op->path);
                      return -1;
                    }
                }
              while (p->pkt.stream_index != p->video_stream);

              p->coded_bytes = p->pkt.size;
              p->coded_buf = p->pkt.data;
            }
          decoded_bytes =
            avcodec_decode_video (p->video_st->codec, p->lavc_frame,
                                  &got_picture, p->coded_buf, p->coded_bytes);
          if (decoded_bytes < 0)
            {
              fprintf (stderr, "avcodec_decode_video failed for %s\n",
                       op->path);
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

static gboolean
process (GeglOperation       *operation,
         GeglNodeContext     *context,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantOperation  *self = GEGL_CHANT_OPERATION (operation);
  Priv                *p = (Priv*)self->priv;

  {
    if (p->ic && !decode_frame (self, self->frame))
      {
        gint pxsize;
        gchar *buf;
        gint x,y;
        g_object_get (output, "px-size", &pxsize, NULL);
        buf = g_malloc (p->width * p->height * pxsize);
        for (y=0; y < p->height; y++)
          {
            guchar *dst = (guchar*)buf + y * p->width * 4;
            guchar *ysrc = (guchar*) (p->lavc_frame->data[0] + y * p->lavc_frame->linesize[0]);
            guchar *usrc = (guchar*) (p->lavc_frame->data[1] + y/2 * p->lavc_frame->linesize[1]);
            guchar *vsrc = (guchar*) (p->lavc_frame->data[2] + y/2 * p->lavc_frame->linesize[2]);

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
              }while(0)

              YUV82RGB8 (*ysrc, *usrc, *vsrc, R, G, B);

              *(unsigned int *) dst = R + G * 256 + B * 256 * 256 + 0xff000000;
              dst += 4;
              ysrc ++;
              if (x % 2)
                {
                  usrc ++;
                  vsrc ++;
                }
              }
          }
        gegl_buffer_set (output, NULL, NULL, buf, GEGL_AUTO_ROWSTRIDE);
        g_free (buf);
      }
  }
  return  TRUE;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {0,0,320,200};
  Priv *p = (Priv*)GEGL_CHANT_OPERATION (operation)->priv;
  result.width = p->width;
  result.height = p->height;
  return result;
}

static void class_init (GeglOperationClass *operation_class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (operation_class);

  gobject_class->finalize = finalize;
  operation_class->prepare = prepare;
}

#endif
