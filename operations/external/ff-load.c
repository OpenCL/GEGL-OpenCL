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

#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
   description (_("Path of video file to load"))

property_int (frame, _("Frame number"), 0)
   value_range (0, G_MAXINT)
   ui_range (0, 10000)

property_int (frames, _("frames"), 0)
   description (_("Number of frames in video, updates at least when first frame has been decoded."))
   value_range (0, G_MAXINT)
   ui_range (0, 10000)

property_double (frame_rate, _("frame-rate"), 0)
   description (_("Frames per second, permits computing time vs frame"))
   value_range (0, G_MAXINT)
   ui_range (0, 10000)

property_audio (audio, _("audio"), 0)

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_C_SOURCE ff-load.c

#include "gegl-op.h"
#include <errno.h>

#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
#include <libavformat/avformat.h>
#else
#include <avformat.h>
#endif

typedef struct
{
  gint             width;
  gint             height;
  gdouble          fps;
  gchar           *codec_name;
  gchar           *fourcc;
  GList           *audio_track; // XXX: must reset to NULL on reinit
  long             audio_pos;
  AVFormatContext *ic;
  int              video_stream;
  int              audio_stream;
  AVStream        *video_st;
  AVStream        *audio_st;
  AVCodecContext  *video_context;
  AVCodecContext  *audio_context;
  AVCodec         *video_codec;
  AVCodec         *audio_codec;
  AVPacket         pkt;
  AVFrame         *lavc_frame;
  glong            coded_bytes;
  guchar          *coded_buf;
  gchar           *loadedfilename; /* to remember which file is "cached"     */
  glong            prevframe;      /* previously decoded frame in loadedfile */
} Priv;

typedef struct AudioFrame {
  uint8_t          buf[16000];
  int              len;
  long             pos;
} AudioFrame;

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
init (GeglProperties *o)
{
  static gint inited = 0; /*< this is actually meant to be static, only to be done once */
  Priv       *p = (Priv*)o->user_data;

  if (p==NULL)
    {
      p = g_new0 (Priv, 1);
      o->user_data = (void*) p;
    }

  p->width = 320;
  p->height = 200;

  if (!inited)
    {
      av_register_all ();
      avcodec_register_all ();
      inited = 1;
    }
  while (p->audio_track)
  {
    g_free (p->audio_track->data);
    p->audio_track = g_list_remove (p->audio_track, p->audio_track->data);
  }
  p->loadedfilename = g_strdup ("");
  p->fourcc = g_strdup ("");
  p->codec_name = g_strdup ("");
}

/* FIXME: probably some more stuff to free here */
static void
ff_cleanup (GeglProperties *o)
{
  Priv *p = (Priv*)o->user_data;
  if (p)
    {
       while (p->audio_track)
        {
          g_free (p->audio_track->data);
          p->audio_track = g_list_remove (p->audio_track, p->audio_track->data);
	}
      if (p->codec_name)
        g_free (p->codec_name);
      if (p->loadedfilename)
        g_free (p->loadedfilename);
      if (p->video_context)
        avcodec_close (p->video_context);
      if (p->audio_context)
        avcodec_close (p->audio_context);
      if (p->ic)
        avformat_close_input(&p->ic);
      if (p->lavc_frame)
        av_free (p->lavc_frame);

      p->video_context = NULL;
      p->audio_context = NULL;
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
   */
  return 0;
}

static int
decode_frame (GeglOperation *operation,
              glong          frame)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv       *p = (Priv*)o->user_data;
  glong       prevframe = p->prevframe;
  glong       decodeframe;        /*< frame to be requested decoded */

  if (frame >= o->frames)
    {
      frame = o->frames - 1;
    }
  if (frame < 0)
    {
      frame = 0;
    }
  if (frame == prevframe)
    {
      /* we've already got the right frame ready for delivery */
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

  if (decodeframe < prevframe + 0.0) // XXX: shuts up gcc
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
                  if (av_read_frame (p->ic, &p->pkt) < 0)
                    {
                      fprintf (stderr, "av_read_frame failed for %s\n",
                               o->path);
                      return -1;
                    }
                 if (p->pkt.stream_index==p->audio_stream)
                   {
                     static AVFrame frame;
                     int got_frame;
                     //int len1 = 
                     avcodec_decode_audio4(p->audio_st->codec,
                                                &frame, &got_frame, &(p->pkt));
                     if (got_frame) {
                       AudioFrame *af = g_malloc0 (sizeof (AudioFrame));
                       int data_size = av_samples_get_buffer_size(NULL,
                           p->audio_context->channels,
                           frame.nb_samples,
                           p->audio_context->sample_fmt,
                           1);
                       af->pos = p->audio_pos;
                       
                       g_assert (data_size < 16000);
                       p->audio_track = g_list_append (p->audio_track, af);
                       memcpy (af->buf, frame.data[0], data_size);
                       af->len = data_size;

                       p->audio_pos += af->len;
                     }
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

          p->coded_buf   += decoded_bytes;
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
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv       *p = (Priv*)o->user_data;

  if (p == NULL)
    init (o);
  p = (Priv*)o->user_data;

  g_assert (o->user_data != NULL);

  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A u8"));


  if (!p->loadedfilename ||
      strcmp (p->loadedfilename, o->path))
    {
      gint i;
      gint err;

      ff_cleanup (o);
      err = avformat_open_input(&p->ic, o->path, NULL, 0);
      if (err < 0)
        {
          print_error (o->path, err);
        }
      err = avformat_find_stream_info (p->ic, NULL);
      if (err < 0)
        {
          g_warning ("ff-load: error finding stream info for %s", o->path);

          return;
        }
      for (i = 0; i< p->ic->nb_streams; i++)
        {
          AVCodecContext *c = p->ic->streams[i]->codec;
          if (c->codec_type == AVMEDIA_TYPE_VIDEO)
            {
              p->video_st = p->ic->streams[i];
              p->video_stream = i;
            }
          if (c->codec_type == AVMEDIA_TYPE_AUDIO)
            {
              p->audio_st = p->ic->streams[i];
              p->audio_stream = i;
            }
        }

      p->video_context = p->video_st->codec;
      p->video_codec = avcodec_find_decoder (p->video_context->codec_id);
      p->audio_context = p->audio_st->codec;
      p->audio_codec = avcodec_find_decoder (p->audio_context->codec_id);

      /* p->video_context->error_resilience = 2; */
      p->video_context->error_concealment = 3;
      p->video_context->workaround_bugs = FF_BUG_AUTODETECT;

      if (p->video_codec == NULL)
        {
          g_warning ("video codec not found");
        }
      if (p->audio_codec == NULL)
        {
          g_warning ("audio codec not found");
        }

      if (p->video_codec->capabilities & CODEC_CAP_TRUNCATED)
        p->video_context->flags |= CODEC_FLAG_TRUNCATED;

      if (avcodec_open2 (p->video_context, p->video_codec, NULL) < 0)
        {
          g_warning ("error opening codec %s", p->video_context->codec->name);
          return;
        }

      if (avcodec_open2 (p->audio_context, p->audio_codec, NULL) < 0)
        {
          g_warning ("error opening codec %s", p->audio_context->codec->name);
          return;
        }
 
      fprintf (stderr, "samplerate: %i channels: %i\n", p->audio_context->sample_rate,
  p->audio_context->channels);

      p->width = p->video_context->width;
      p->height = p->video_context->height;
      p->lavc_frame = av_frame_alloc ();

      if (p->fourcc)
        g_free (p->fourcc);
      p->fourcc = g_strdup ("none");
          p->fourcc[0] = (p->video_context->codec_tag) & 0xff;
      p->fourcc[1] = (p->video_context->codec_tag >> 8) & 0xff;
      p->fourcc[2] = (p->video_context->codec_tag >> 16) & 0xff;
      p->fourcc[3] = (p->video_context->codec_tag >> 24) & 0xff;

      if (p->codec_name)
        g_free (p->codec_name);
      if (p->video_codec->name)
        {
          p->codec_name = g_strdup (p->video_codec->name);
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

      o->frames = p->video_st->nb_frames;
      if (!o->frames)
      {
	/* With no declared frame count, compute number of frames based on
           duration and video codecs framerate
         */
        o->frames = p->ic->duration * p->video_st->time_base.den  / p->video_st->time_base.num / AV_TIME_BASE;
      }

      o->frame_rate = p->video_st->time_base.den  / p->video_st->time_base.num;
    }
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = {0,0,320,200};
  Priv *p = (Priv*)GEGL_PROPERTIES (operation)->user_data;
  result.width = p->width;
  result.height = p->height;
  return result;
}

static int
samples_per_frame (int    frame,
                   double frame_rate,
                   int    sample_rate,
                   long  *start)
/* XXX: this becomes a bottleneck for high frame numbers, but is good for now */
{
  double osamples;
  double samples = 0;
  int f = 0;
  for (f = 0; f < frame; f++)
  {
    samples += sample_rate / frame_rate;
  }
  osamples = samples;
  samples += sample_rate / frame_rate;
  (*start) = ceil(osamples);
  return ceil(samples)-ceil(osamples);
}

static void get_sample_data (Priv *p, long sample_no, float *left, float *right)
{
  GList *l;
  long no = 0;
/* XXX: todo optimize this so that we at least can reuse the
        found frame
 */
  for (l = p->audio_track; l; l = l->next)
  {
    AudioFrame *af = l->data;
    int16_t *data = (void*) af->buf;

    if (no + af->len/4 > sample_no)
      {
        int i = sample_no - no + af->len/4;
        *left  = data[i*2+0] / 32768.0;
        *right = data[i*2+0] / 32768.0;
        return;
      }

    no += af->len/4;
  }
  *left  = 0;
  *right = 0;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv       *p = (Priv*)o->user_data;

  {
    if (p->ic && !decode_frame (operation, o->frame))
      {
        guchar *buf;
        gint    pxsize;
        gint    x,y;

        long sample_start = 0;
        o->audio->samplerate = p->audio_context->sample_rate;
        o->audio->samples = samples_per_frame (o->frame, o->frame_rate, o->audio->samplerate, &sample_start);
        {
          int i;
          for (i = 0; i < o->audio->samples; i++)
          {
            get_sample_data (p, sample_start + i, &o->audio->left[i], &o->audio->right[i]);
          }
        }
	
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
          }\
        gegl_buffer_set (output, NULL, 0, NULL, buf, GEGL_AUTO_ROWSTRIDE);
        g_free (buf);
      }
  }
  return  TRUE;
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      Priv *p = (Priv*)o->user_data;

      g_free (p->loadedfilename);
      g_free (p->fourcc);
      g_free (p->codec_name);

      g_free (o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
gegl_op_class_init (GeglOpClass *klass)
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
    "name",         "gegl:ff-load",
    "title",        _("FFmpeg Frame Loader"),
    "categories"  , "input:video",
    "description" , _("FFmpeg video frame importer."),
    NULL);
}

#endif
