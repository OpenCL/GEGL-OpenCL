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

#include <libavformat/avformat.h>

typedef struct
{
  gint             width;
  gint             height;
  gdouble          fps;
  gchar           *codec_name;
  gchar           *fourcc;
  GList           *audio_track;
  GList           *audio_cursor;
  long             audio_cursor_pos;
  long             audio_pos;
  AVFormatContext *video_fcontext;
  AVFormatContext *audio_fcontext;
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
  gdouble          prevpts;
  gdouble          prevapts;
  glong            a_prevframe;   /* previously decoded a_frame in loadedfile */
} Priv;

#define MAX_AUDIO_CHANNELS  8
#define MAX_AUDIO_SAMPLES   8192

typedef struct AudioFrame {
  int64_t          pts;
  float            data[MAX_AUDIO_CHANNELS][MAX_AUDIO_SAMPLES];
  int              channels;
  int              sample_rate;
  int              len;
  long             pos;

} AudioFrame;

static void
print_error (const char *filename, int err)
{
  switch (err)
    {
    case AVERROR(EINVAL):
      g_warning ("%s: Incorrect image filename syntax.\n"
                 "Use '%%d' to specify the image number:\n"
                 "  for img1.jpg, img2.jpg, ..., use 'img%%d.jpg';\n"
                 "  for img001.jpg, img002.jpg, ..., use 'img%%03d.jpg'.\n",
                 filename);
      break;
    case AVERROR_INVALIDDATA:
      g_warning ("%s: Error while parsing header or unknown format\n", filename);
      break;
    default:
      g_warning ("%s: Error while opening file\n", filename);
      break;
    }
}

static void
init (GeglProperties *o)
{
  Priv       *p = (Priv*)o->user_data;
  static gint av_inited = 0;
  if (av_inited == 0)
    {
      av_register_all ();
      av_inited = 1;
    }

  if (p == NULL)
    {
      p = g_new0 (Priv, 1);
      o->user_data = (void*) p;
    }

  p->width = 320;
  p->height = 200;

  while (p->audio_track)
  {
    g_free (p->audio_track->data);
    p->audio_track = g_list_remove (p->audio_track, p->audio_track->data);
  }
  p->audio_cursor = NULL;
  p->loadedfilename = g_strdup ("");
  p->fourcc = g_strdup ("");
  p->codec_name = g_strdup ("");
}

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
      if (p->video_fcontext)
        avformat_close_input(&p->video_fcontext);
      if (p->audio_fcontext)
        avformat_close_input(&p->audio_fcontext);
      if (p->lavc_frame)
        av_free (p->lavc_frame);

      p->video_context = NULL;
      p->audio_context = NULL;
      p->video_fcontext = NULL;
      p->audio_fcontext = NULL;
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

/* maintain list of audio samples */
static int
decode_audio (GeglOperation *operation,
              gdouble        pts1,
              gdouble        pts2)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv       *p = (Priv*)o->user_data;

  /* figure out which frame we should start decoding at */
  //fprintf (stderr, "%f %f\n", p->prevapts, pts2);

  while (p->prevapts <= pts2)
    {
      int       decoded_bytes;

      if (av_read_frame (p->audio_fcontext, &p->pkt) < 0)
         {
           fprintf (stderr, "av_read_frame failed for %s\n",
                    o->path);
           return -1;
         }
      if (p->pkt.stream_index==p->audio_stream && p->audio_st)
        {
          static AVFrame frame;
          int got_frame;


          decoded_bytes = avcodec_decode_audio4(p->audio_st->codec,
                                     &frame, &got_frame, &(p->pkt));

          if (decoded_bytes < 0)
            {
              fprintf (stderr, "avcodec_decode_audio4 failed for %s\n",
                                o->path);
            }

          if (got_frame) {
            AudioFrame *af = g_malloc0 (sizeof (AudioFrame));
            g_assert (frame.nb_samples < MAX_AUDIO_SAMPLES);
          
            af->pts = p->pkt.pts;
            //fprintf (stderr, "audio-pts: %f\n", p->pkt.pts * av_q2d (p->audio_st->time_base));

            af->channels = p->audio_context->channels;
            switch (p->audio_context->sample_fmt)
            {
              case AV_SAMPLE_FMT_FLT:
                for (gint i = 0; i < frame.nb_samples; i++)
                  for (gint c = 0; c < af->channels; c++)
                    af->data[c][i] = ((int16_t *)frame.data[0])[i * af->channels + c];
                break;
              case AV_SAMPLE_FMT_FLTP:
                for (gint i = 0; i < frame.nb_samples; i++)
                  for (gint c = 0; c < af->channels; c++)
                    af->data[c][i] = ((float *)frame.data[c])[i];
                break;
              case AV_SAMPLE_FMT_S16:
                for (gint i = 0; i < frame.nb_samples; i++)
                  for (gint c = 0; c < af->channels; c++)
                    af->data[c][i] = ((int16_t *)frame.data[0])[i * af->channels + c] / 32768.0;
                break;
              case AV_SAMPLE_FMT_S16P:
                for (gint i = 0; i < frame.nb_samples; i++)
                  for (gint c = 0; c < af->channels; c++)
                    af->data[c][i] = ((int16_t *)frame.data[c])[i] / 32768.0;
                break;
              case AV_SAMPLE_FMT_S32:
                for (gint i = 0; i < frame.nb_samples; i++)
                  for (gint c = 0; c < af->channels; c++)
                    af->data[c][i] = ((int32_t *)frame.data[0])[i * af->channels + c] / 2147483648.0;
                break;
              case AV_SAMPLE_FMT_S32P:
                for (gint i = 0; i < frame.nb_samples; i++)
                  for (gint c = 0; c < af->channels; c++)
                    af->data[c][i] = ((int32_t *)frame.data[c])[i] / 2147483648.0;
                break;
              default:
                g_warning ("undealt with sample format\n");
            }
            af->len = frame.nb_samples;
            af->pos = p->audio_pos;
            p->audio_pos += af->len;
            p->audio_track = g_list_append (p->audio_track, af);
            p->prevapts = p->pkt.pts * av_q2d (p->audio_st->time_base);
          }
        }
    }
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
                  if (av_read_frame (p->video_fcontext, &p->pkt) < 0)
                    {
                      fprintf (stderr, "av_read_frame failed for %s\n",
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

          if(got_picture)
          {
            if (p->pkt.pts < 0)
              p->prevpts += av_q2d (p->video_st->time_base);
            else
              p->prevpts = p->pkt.pts * av_q2d (p->video_st->time_base);
            //fprintf (stderr, "video-pts: %li %f\n", p->pkt.pts, p->prevpts);
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
      err = avformat_open_input(&p->video_fcontext, o->path, NULL, 0);
      if (err < 0)
        {
          print_error (o->path, err);
        }
      err = avformat_find_stream_info (p->video_fcontext, NULL);
      if (err < 0)
        {
          g_warning ("ff-load: error finding stream info for %s", o->path);

          return;
        }
      err = avformat_open_input(&p->audio_fcontext, o->path, NULL, 0);
      if (err < 0)
        {
          print_error (o->path, err);
        }
      err = avformat_find_stream_info (p->audio_fcontext, NULL);
      if (err < 0)
        {
          g_warning ("ff-load: error finding stream info for %s", o->path);

          return;
        }

      for (i = 0; i< p->video_fcontext->nb_streams; i++)
        {
          AVCodecContext *c = p->video_fcontext->streams[i]->codec;
          if (c->codec_type == AVMEDIA_TYPE_VIDEO)
            {
              p->video_st = p->video_fcontext->streams[i];
              p->video_stream = i;
            }
          if (c->codec_type == AVMEDIA_TYPE_AUDIO)
            {
              p->audio_st = p->audio_fcontext->streams[i];
              p->audio_stream = i;
            }
        }

      p->video_context = p->video_st->codec;
      p->video_codec = avcodec_find_decoder (p->video_context->codec_id);

      if (p->audio_st)
        {
	  p->audio_context = p->audio_st->codec;
	  p->audio_codec = avcodec_find_decoder (p->audio_context->codec_id);
	  if (p->audio_codec == NULL)
            g_warning ("audio codec not found");
          else 
	    if (avcodec_open2 (p->audio_context, p->audio_codec, NULL) < 0)
              {
                 g_warning ("error opening codec %s", p->audio_context->codec->name);
              }
            else
              {
		 fprintf (stderr, "samplerate: %i channels: %i samplefmt: ", p->audio_context->sample_rate,
		p->audio_context->channels);
                 switch (p->audio_context->sample_fmt)
                 {
                   case AV_SAMPLE_FMT_U8: fprintf (stderr, "u8"); break;
                   case AV_SAMPLE_FMT_S16: fprintf (stderr, "s16"); break;
                   case AV_SAMPLE_FMT_S32: fprintf (stderr, "s32"); break;
                   case AV_SAMPLE_FMT_FLT: fprintf (stderr, "flt"); break;
                   case AV_SAMPLE_FMT_DBL: fprintf (stderr, "dbl"); break;
                   case AV_SAMPLE_FMT_U8P: fprintf (stderr, "u8-planar"); break;
                   case AV_SAMPLE_FMT_S16P: fprintf (stderr, "s16-planar"); break;
                   case AV_SAMPLE_FMT_S32P: fprintf (stderr, "s32-planar"); break;
                   case AV_SAMPLE_FMT_FLTP: fprintf (stderr, "flt-planar"); break;
                   case AV_SAMPLE_FMT_DBLP: fprintf (stderr, "dbl-planar"); break;
                   default: fprintf (stderr, "none"); break;
                 }
                 fprintf (stderr, "\n");
              }
        }

      p->video_context->err_recognition = AV_EF_IGNORE_ERR | AV_EF_BITSTREAM | AV_EF_BUFFER;
      p->video_context->workaround_bugs = FF_BUG_AUTODETECT;

#if 1
      p->video_context->error_concealment = 0;
#else
      p->video_context->error_concealment = FF_EC_DEBLOCK | FF_EC_GUESS_MVS | FF_EC_FAVOR_INTER;

#endif

      p->video_context->idct_algo = FF_IDCT_SIMPLEAUTO;

      p->video_context->thread_count = 0;
      p->video_context->thread_type = FF_THREAD_SLICE;
      /* XXX: permits slice parallell decode, at expense of h264 compliance of output */
      p->video_context->flags2 = AV_CODEC_FLAG2_FAST;

      if (p->video_codec == NULL)
          g_warning ("video codec not found");

      if (avcodec_open2 (p->video_context, p->video_codec, NULL) < 0)
        {
          g_warning ("error opening codec %s", p->video_context->codec->name);
          return;
        }

      p->width = p->video_context->width;
      p->height = p->video_context->height;
      p->lavc_frame = av_frame_alloc ();

      if (p->fourcc)
        g_free (p->fourcc);
      p->fourcc = g_strdup ("none");
      p->fourcc[0] = (p->video_context->codec_tag >> 0)  & 0xff;
      p->fourcc[1] = (p->video_context->codec_tag >> 8)  & 0xff;
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
      p->a_prevframe = -1;
      p->coded_bytes = 0;
      p->coded_buf = NULL;

      o->frames = p->video_st->nb_frames;
      o->frame_rate = av_q2d (av_guess_frame_rate (p->video_fcontext, p->video_st, NULL));
      if (!o->frames)
      {
        /* this is a guesstimate of frame-count */
	o->frames = p->video_fcontext->duration * o->frame_rate / AV_TIME_BASE;
        /* make second guess for things like luxo */
	if (o->frames < 1)
          o->frames = 1000;
      }
      fprintf (stdout, "fps: %f\n", o->frame_rate);
      fprintf (stdout, "frames: %i\n", o->frames);
      {
        int m ,h;
        int s = o->frames / o->frame_rate;
        m = s / 60;
        s -= m * 60;
        h = m / 60;
        m -= h * 60;
        fprintf (stdout, "duration: %02i:%02i:%02i\n", h, m, s);
      }
      fprintf (stdout, "fourcc: %s\n", p->fourcc);
      fprintf (stdout, "codec-name: %s\n", p->codec_name);
    }
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = {0, 0, 320, 200};
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
{
  double osamples;
  double samples = 0;
  int f = 0;

  if (fabs(fmod (sample_rate, frame_rate)) < 0.0001)
  {
    *start = (sample_rate / frame_rate) * frame;
    return sample_rate / frame_rate;
  }

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
  long no = 0;
  GList *l;
  l = p->audio_track;
  if (sample_no < 0)
    return;
  no = 0;
  if (p->audio_cursor && sample_no > p->audio_cursor_pos) {
    AudioFrame *af = p->audio_cursor->data;
    if (p->audio_cursor_pos + af->len > sample_no)
      {
        int i = sample_no - p->audio_cursor_pos;
        *left  = af->data[0][i];
        if (af->channels == 1)
          *right = af->data[0][i];
        else
          *right = af->data[1][i];
        return;
      }
    /* override start conditions of below loop */
    l = p->audio_cursor;
    no = p->audio_cursor_pos;
  }
  for (; l; l = l->next)
  {
    AudioFrame *af = l->data;
    if (no + af->len > sample_no)
      {
        int i = sample_no - no;
        *left  = af->data[0][i];
        if (af->channels == 1)
          *right = af->data[0][i];
        else
          *right = af->data[1][i];
        p->audio_cursor     = l;
        p->audio_cursor_pos = no;
        return;
      }
    no += af->len;
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
    if (p->video_fcontext && !decode_frame (operation, o->frame))
      {
        guchar *buf;
        gint    pxsize;
        gint    x,y;

        long sample_start = 0;

	if (p->audio_context)
        {
          o->audio->samplerate = p->audio_context->sample_rate;
          o->audio->samples = samples_per_frame (o->frame,
               o->frame_rate, o->audio->samplerate,
               &sample_start);
	  //decode_audio (operation, p->prevpts, p->prevpts + 1.0/o->frame_rate + 5.0);
	  decode_audio (operation, p->prevpts, p->prevpts + 5.0);//p->prevpts + 1.0/o->frame_rate + 5.0);

          {
            int i;
            for (i = 0; i < o->audio->samples; i++)
            {
              get_sample_data (p, sample_start + i, &o->audio->left[i],
                                  &o->audio->right[i]);
              if(0)fprintf (stderr, "%f %f %i\n", 
                 o->audio->left[i],
                 o->audio->right[i], o->audio->samples);
            }
          }
        }

	
        g_object_get (output, "px-size", &pxsize, NULL);
        buf = g_new (guchar, p->width * p->height * pxsize);

        for (y=0; y < p->height; y++)
          {
            guchar       *dst  = buf                    + y     * p->width * 4;
            const guchar *ysrc = p->lavc_frame->data[0] + y     * p->lavc_frame->linesize[0];
            const guchar *usrc = p->lavc_frame->data[1] + y / 2 * p->lavc_frame->linesize[1];
            const guchar *vsrc = p->lavc_frame->data[2] + y / 2 * p->lavc_frame->linesize[2];

            for (x=0;x < p->width; x++)
              {
                gint R,G,B;
#ifndef byteclamp
#define byteclamp(j) j=j<0?0:j>255?255:j
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
              dst  += 4;
              usrc += x%2;
              vsrc += x%2;
              ysrc ++;
              }
          }
        {
          GeglRectangle extent = {0,0,p->width,p->height};
          gegl_buffer_set (output, &extent, 0, NULL, buf, GEGL_AUTO_ROWSTRIDE);
        }
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
