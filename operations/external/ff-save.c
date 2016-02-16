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
 * Copyright 2003,2004,2007, 2015 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <stdlib.h>

#include <glib/gi18n-lib.h>

//#define USE_FINE_GRAINDED_FFMPEG 1

#ifdef GEGL_PROPERTIES

property_string (path, _("File"), "/tmp/fnord.ogv")
    description (_("Target path and filename, use '-' for stdout."))

property_audio_fragment (audio, _("audio"), 0)
property_string (audio_codec, _("Audio codec"), "auto")
   description (_("Audio codec to use, or auto to use a good default based on container format."))
property_int (audio_sample_rate, _("audio sample rate"), -1)
    description (_("-1 means autodetect on first audio fragment"))

property_int (audio_bit_rate, _("audio bitrate in kb/s"), 64)
    description (_("Target encoded video bitrate in kb/s"))

property_double (frame_rate, _("Frames/second"), 25.0)
    value_range (0.0, 100.0)

property_string (video_codec, _("Video codec"), "auto")
   description (_("Video codec to use, or auto to use a good default based on container format."))
property_int (video_bit_rate, _("video bitrate in kb/s"), 128)
    description (_("Target encoded video bitrate in kb/s"))
property_int (video_bufsize, _("Video bufsize"), 0)

property_string (container_format, _("Container format"), "auto")
   description (_("Container format to use, or auto to autodetect based on file extension."))

#ifdef USE_FINE_GRAINED_FFMPEG
property_int (global_quality, _("global quality"), 0)
property_int (noise_reduction, _("noise reduction"), 0)
property_int (scenechange_threshold, _("scenechange threshold"), 0)
property_int (video_bit_rate_min, _("video bitrate min"), 0)
property_int (video_bit_rate_max, _("video bitrate max"), 0)
property_int (video_bit_rate_tolerance, _("video bitrate tolerance"), -1)

property_int (keyint_min, _("keyint-min"), 0)
property_int (trellis, _("trellis"), 0)
property_int (qmin, _("qmin"), 0)
property_int (qmax, _("qmax"), 0)
property_int (max_qdiff, _("max_qdiff"), 0)
property_int (me_range, _("me_range"), 0)
property_int (max_b_frames, _("max_b_frames"), 0)
property_int (gop_size, _("gop-size"), 0)
property_double (qcompress, _("qcompress"), 0.0)
property_double (qblur, _("qblur"), 0.0)
property_double (i_quant_factor, _("i-quant-factor"), 0.0)
property_double (i_quant_offset, _("i-quant-offset"), 0.0)
property_int (me_subpel_quality, _("me-subpel-quality"), 0)
#endif


#else

#define GEGL_OP_SINK
#define GEGL_OP_C_SOURCE ff-save.c

#include "gegl-op.h"

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

typedef struct
{
  gdouble    frame;
  gdouble    frames;
  gdouble    width;
  gdouble    height;
  GeglBuffer *input;

  AVOutputFormat *fmt;
  AVFormatContext *oc;
  AVStream *video_st;

  AVFrame  *picture, *tmp_picture;
  uint8_t  *video_outbuf;
  int       frame_count, video_outbuf_size;

    /** the rest is for audio handling within oxide, note that the interface
     * used passes all used functions in the oxide api through the reg_sym api
     * of gggl, this means that the ops should be usable by other applications
     * using gggl directly,. without needing to link with the oxide library
     */
  AVStream *audio_st;

  uint32_t  sample_rate;
  uint32_t  bits;
  uint32_t  channels;
  uint32_t  fragment_samples;
  uint32_t  fragment_size;

  int       bufsize;
  int       buffer_read_pos;
  int       buffer_write_pos;
  uint8_t  *buffer; 
                   
  int       audio_outbuf_size;
  int16_t  *samples;

  GList    *audio_track;
  long      audio_pos;
  long      audio_read_pos;
  
  int       next_apts;
} Priv;

static void
clear_audio_track (GeglProperties *o)
{
  Priv *p = (Priv*)o->user_data;
  while (p->audio_track)
    {
      g_object_unref (p->audio_track->data);
      p->audio_track = g_list_remove (p->audio_track, p->audio_track->data);
    }
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
    if (start)
      *start = (sample_rate / frame_rate) * frame;
    return sample_rate / frame_rate;
  }

  for (f = 0; f < frame; f++) 
  {
    samples += sample_rate / frame_rate;
  }
  osamples = samples;
  samples += sample_rate / frame_rate;
  if (start)
     (*start) = ceil(osamples);
  return ceil(samples)-ceil(osamples);
}

static void get_sample_data (Priv *p, long sample_no, float *left, float *right)
{
  int to_remove = 0;
  GList *l;
  if (sample_no < 0)
    return;
  for (l = p->audio_track; l; l = l->next)
  {
    GeglAudioFragment *af = l->data;
    int channels = gegl_audio_fragment_get_channels (af);
    int pos = gegl_audio_fragment_get_pos (af);
    int sample_count = gegl_audio_fragment_get_sample_count (af);
    if (sample_no > pos + sample_count)
    {
      to_remove ++;
    }

    if (pos <= sample_no &&
        sample_no < pos + sample_count)
      {
        int i = sample_no - pos;
        *left  = af->data[0][i];
        if (channels == 1)
          *right = af->data[0][i];
        else
          *right = af->data[1][i];

	if (to_remove)  /* consuming audiotrack */
        {
          again:
          for (l = p->audio_track; l; l = l->next)
          {
            GeglAudioFragment *af = l->data;
            int pos = gegl_audio_fragment_get_pos (af);
            int sample_count = gegl_audio_fragment_get_sample_count (af);
            if (sample_no > pos + sample_count)
            {
              p->audio_track = g_list_remove (p->audio_track, af);
              g_object_unref (af);
              goto again;
            }
          }
        }
        return;
      }
  }
  *left  = 0;
  *right = 0;
}

static void
init (GeglProperties *o)
{
  static gint inited = 0; /*< this is actually meant to be static, only to be done once */
  Priv       *p = (Priv*)o->user_data;

  if (p == NULL)
    {
      p = g_new0 (Priv, 1);
      o->user_data = (void*) p;
    }

  if (!inited)
    {
      av_register_all ();
      avcodec_register_all ();
      inited = 1;
    }

  clear_audio_track (o);
  p->audio_pos = 0;
  p->audio_read_pos = 0;

  o->audio_sample_rate = -1; /* only do this if it hasn't been manually set? */

  av_log_set_level (AV_LOG_WARNING);
}

static void close_video       (Priv            *p,
                               AVFormatContext *oc,
                               AVStream        *st);
void        close_audio       (Priv            *p,
                               AVFormatContext *oc,
                               AVStream        *st);
static int  tfile             (GeglProperties  *o);
static void write_video_frame (GeglProperties  *o,
                               AVFormatContext *oc,
                               AVStream        *st);
static void write_audio_frame (GeglProperties      *o,
                               AVFormatContext *oc,
                               AVStream        *st);

#define STREAM_FRAME_RATE 25    /* 25 images/s */

#ifndef DISABLE_AUDIO
/* add an audio output stream */
static AVStream *
add_audio_stream (GeglProperties *o, AVFormatContext * oc, int codec_id)
{
  AVCodecContext *c;
  AVStream *st;

  st = avformat_new_stream (oc, NULL);
  if (!st)
    {
      fprintf (stderr, "Could not alloc stream\n");
      exit (1);
    }

  c = st->codec;
  c->codec_id = codec_id;
  c->codec_type = AVMEDIA_TYPE_AUDIO;

  if (oc->oformat->flags & AVFMT_GLOBALHEADER)
    c->flags |= CODEC_FLAG_GLOBAL_HEADER;

  return st;
}
#endif

static void
open_audio (GeglProperties *o, AVFormatContext * oc, AVStream * st)
{
  AVCodecContext *c;
  AVCodec  *codec;
  int i;

  c = st->codec;

  /* find the audio encoder */
  codec = avcodec_find_encoder (c->codec_id);
  if (!codec)
    {
      fprintf (stderr, "codec not found\n");
      exit (1);
    }
  c->bit_rate = o->audio_bit_rate * 1000;
  c->sample_fmt = codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;

  if (o->audio_sample_rate == -1)
  {
    if (o->audio)
    {
      if (gegl_audio_fragment_get_sample_rate (o->audio) == 0)
      {
        gegl_audio_fragment_set_sample_rate (o->audio, 48000); // XXX: should skip adding audiostream instead
      }
      o->audio_sample_rate = gegl_audio_fragment_get_sample_rate (o->audio);
    }
  }
  c->sample_rate = o->audio_sample_rate;
  c->channel_layout = AV_CH_LAYOUT_STEREO;
  c->channels = 2;


  if (codec->supported_samplerates)
  {
    c->sample_rate = codec->supported_samplerates[0];
    for (i = 0; codec->supported_samplerates[i]; i++)
    {
      if (codec->supported_samplerates[i] == o->audio_sample_rate)
         c->sample_rate = o->audio_sample_rate;
    }
  }
  //st->time_base = (AVRational){1, c->sample_rate};
  st->time_base = (AVRational){1, o->audio_sample_rate};

  c->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL; // ffmpeg AAC is not quite stable yet

  /* open it */
  if (avcodec_open2 (c, codec, NULL) < 0)
    {
      fprintf (stderr, "could not open codec\n");
      exit (1);
    }
}

static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples)
{
  AVFrame *frame = av_frame_alloc();
  int ret;

  if (!frame) {
      fprintf(stderr, "Error allocating an audio frame\n");
      exit(1);
  }

  frame->format         = sample_fmt;
  frame->channel_layout = channel_layout;
  frame->sample_rate    = sample_rate;
  frame->nb_samples     = nb_samples;

  if (nb_samples) {
      ret = av_frame_get_buffer(frame, 0);
      if (ret < 0) {
          fprintf(stderr, "Error allocating an audio buffer\n");
          exit(1);
      }
  }
  return frame;
}

void
write_audio_frame (GeglProperties *o, AVFormatContext * oc, AVStream * st)
{
  Priv *p = (Priv*)o->user_data;
  AVCodecContext *c = st->codec;
  int sample_count = 100000;
  static AVPacket  pkt = { 0 };

  if (pkt.size == 0)
  {
    av_init_packet (&pkt);
  }

  /* first we add incoming frames audio samples */
  if (o->audio)
  {
    int i;
    GeglAudioFragment *af;
    sample_count = gegl_audio_fragment_get_sample_count (o->audio);
    af = gegl_audio_fragment_new (gegl_audio_fragment_get_sample_rate (o->audio),
                                                     gegl_audio_fragment_get_channels (o->audio),
                                                     gegl_audio_fragment_get_channel_layout (o->audio),
                                                     sample_count);
    gegl_audio_fragment_set_sample_count (af, sample_count);
    for (i = 0; i < sample_count; i++)
      {
        af->data[0][i] = o->audio->data[0][i];
        af->data[1][i] = o->audio->data[1][i];
      }
    gegl_audio_fragment_set_pos (af, p->audio_pos);
    p->audio_pos += sample_count;
    p->audio_track = g_list_append (p->audio_track, af);
  }
  else
  {
    int i;
    GeglAudioFragment *af;
    sample_count = samples_per_frame (p->frame_count, o->frame_rate, o->audio_sample_rate, NULL);
    af = gegl_audio_fragment_new (sample_count,
                                                     2,
                                                     0,
                                                     sample_count);
    gegl_audio_fragment_set_sample_count (af, sample_count);
    gegl_audio_fragment_set_pos (af, p->audio_pos);
    for (i = 0; i < sample_count; i++)
      {
        af->data[0][i] = 0.0;
        af->data[1][i] = 0.0;
      }
    p->audio_pos += sample_count;
    p->audio_track = g_list_append (p->audio_track, af);
  }
  if (!(c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE))
    sample_count = c->frame_size;

  /* then we encode as much as we can in a loop using the codec frame size */
  
  while (p->audio_pos - p->audio_read_pos > sample_count)
  {
    long i;
    int ret;
    int got_packet = 0;
    AVFrame *frame = alloc_audio_frame (c->sample_fmt, c->channel_layout,
                                        c->sample_rate, sample_count);

    switch (c->sample_fmt) {
      case AV_SAMPLE_FMT_FLT:
        for (i = 0; i < sample_count; i++)
        {
          float left = 0, right = 0;
          get_sample_data (p, i + p->audio_read_pos, &left, &right);
          ((float*)frame->data[0])[c->channels*i+0] = left;
          ((float*)frame->data[0])[c->channels*i+1] = right;
        }
        break;
      case AV_SAMPLE_FMT_FLTP:
        for (i = 0; i < sample_count; i++)
        {
          float left = 0, right = 0;
          get_sample_data (p, i + p->audio_read_pos, &left, &right);
          ((float*)frame->data[0])[i] = left;
          ((float*)frame->data[1])[i] = right;
        }
        break;
      case AV_SAMPLE_FMT_S16:
        for (i = 0; i < sample_count; i++)
        {
          float left = 0, right = 0;
          get_sample_data (p, i + p->audio_read_pos, &left, &right);
          ((int16_t*)frame->data[0])[c->channels*i+0] = left * (1<<15);
          ((int16_t*)frame->data[0])[c->channels*i+1] = right * (1<<15);
        }
        break;
      case AV_SAMPLE_FMT_S32:
        for (i = 0; i < sample_count; i++)
        {
          float left = 0, right = 0;
          get_sample_data (p, i + p->audio_read_pos, &left, &right);
          ((int32_t*)frame->data[0])[c->channels*i+0] = left * (1<<31);
          ((int32_t*)frame->data[0])[c->channels*i+1] = right * (1<<31);
        }
        break;
      case AV_SAMPLE_FMT_S32P:
        for (i = 0; i < sample_count; i++)
        {
          float left = 0, right = 0;
          get_sample_data (p, i + p->audio_read_pos, &left, &right);
          ((int32_t*)frame->data[0])[i] = left * (1<<31);
          ((int32_t*)frame->data[1])[i] = right * (1<<31);
        }
        break;
      case AV_SAMPLE_FMT_S16P:
        for (i = 0; i < sample_count; i++)
        {
          float left = 0, right = 0;
          get_sample_data (p, i + p->audio_read_pos, &left, &right);
          ((int16_t*)frame->data[0])[i] = left * (1<<15);
          ((int16_t*)frame->data[1])[i] = right * (1<<15);
        }
        break;
      default:
        fprintf (stderr, "eeeek unhandled audio format\n");
        break;
    }
    frame->pts = p->next_apts;
    p->next_apts += sample_count;

    av_frame_make_writable (frame);
    ret = avcodec_encode_audio2 (c, &pkt, frame, &got_packet);

    av_packet_rescale_ts (&pkt, st->codec->time_base, st->time_base);
    if (ret < 0) {
      fprintf (stderr, "Error encoding audio frame: %s\n", av_err2str (ret));
    }

    if (got_packet)
    {
      pkt.stream_index = st->index;
      av_interleaved_write_frame (oc, &pkt);
      av_packet_unref (&pkt);
    }

    av_frame_free (&frame);
    p->audio_read_pos += sample_count;
  }
}

void
close_audio (Priv * p, AVFormatContext * oc, AVStream * st)
{
  avcodec_close (st->codec);

}

/* add a video output stream */
static AVStream *
add_video_stream (GeglProperties *o, AVFormatContext * oc, int codec_id)
{
  Priv *p = (Priv*)o->user_data;

  AVCodecContext *c;
  AVStream *st;

  st = avformat_new_stream (oc, NULL);
  if (!st)
    {
      fprintf (stderr, "Could not alloc stream %p %p %i\n", o, oc, codec_id);
      exit (1);
    }

  c = st->codec;
  c->codec_id = codec_id;
  c->codec_type = AVMEDIA_TYPE_VIDEO;
  /* put sample propeters */
  c->bit_rate = o->video_bit_rate * 1000;
#ifdef USE_FINE_GRAINED_FFMPEG
  c->rc_min_rate = o->video_bit_rate_min * 1000;
  c->rc_max_rate = o->video_bit_rate_max * 1000;
  if (o->video_bit_rate_tolerance >= 0)
    c->bit_rate_tolerance = o->video_bit_rate_tolerance * 1000;
#endif
  /* resolution must be a multiple of two */
  c->width = p->width;
  c->height = p->height;
  /* frames per second */
  st->time_base =(AVRational){1000, o->frame_rate * 1000};
  c->time_base = st->time_base;

  c->pix_fmt = AV_PIX_FMT_YUV420P;
  

  if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO)
    {
      c->max_b_frames = 2;
    }

  if (c->codec_id == AV_CODEC_ID_H264)
   {
     c->qcompress = 0.6;  // qcomp=0.6
     c->me_range = 16;    // me_range=16
     c->gop_size = 250;   // g=250
     c->max_b_frames = 3; // bf=3
   }

  if (o->video_bufsize)
    c->rc_buffer_size = o->video_bufsize * 1000;
#if USE_FINE_GRAINED_FFMPEG
  if (o->global_quality)
     c->global_quality = o->global_quality;
  if (o->qcompress != 0.0)
     c->qcompress = o->qcompress;
  if (o->qblur != 0.0)
     c->qblur = o->qblur;
  if (o->max_qdiff != 0)
     c->max_qdiff = o->max_qdiff;
  if (o->me_subpel_quality != 0)
     c->me_subpel_quality = o->me_subpel_quality;
  if (o->i_quant_factor != 0.0)
     c->i_quant_factor = o->i_quant_factor;
  if (o->i_quant_offset != 0.0)
     c->i_quant_offset = o->i_quant_offset;
  if (o->max_b_frames)
    c->max_b_frames = o->max_b_frames;
  if (o->me_range)
    c->me_range = o->me_range;
  if (o->noise_reduction)
    c->noise_reduction = o->noise_reduction;
  if (o->scenechange_threshold)
    c->scenechange_threshold = o->scenechange_threshold;
  if (o->trellis)
    c->trellis = o->trellis;
  if (o->qmin)
    c->qmin = o->qmin;
  if (o->qmax)
    c->qmax = o->qmax;
  if (o->gop_size)
    c->gop_size = o->gop_size;
  if (o->keyint_min)
    c->keyint_min = o->keyint_min;
#endif

   if (oc->oformat->flags & AVFMT_GLOBALHEADER)
     c->flags |= CODEC_FLAG_GLOBAL_HEADER;

  return st;
}


static AVFrame *
alloc_picture (int pix_fmt, int width, int height)
{
  AVFrame  *picture;
  uint8_t  *picture_buf;
  int       size;

  picture = av_frame_alloc ();
  if (!picture)
    return NULL;
  size = avpicture_get_size (pix_fmt, width + 1, height + 1);
  picture_buf = malloc (size);
  if (!picture_buf)
    {
      av_free (picture);
      return NULL;
    }
  avpicture_fill ((AVPicture *) picture, picture_buf, pix_fmt, width, height);
  return picture;
}

static void
open_video (GeglProperties *o, AVFormatContext * oc, AVStream * st)
{
  Priv           *p = (Priv*)o->user_data;
  AVCodec  *codec;
  AVCodecContext *c;
  AVDictionary *codec_options = {0};

  c = st->codec;

  /* find the video encoder */
  codec = avcodec_find_encoder (c->codec_id);
  if (!codec)
    {
      fprintf (stderr, "codec not found\n");
      exit (1);
    }

  if (codec->pix_fmts){
    int i = 0;
    c->pix_fmt = codec->pix_fmts[0];
    while (codec->pix_fmts[i] !=-1)
    {
      if (codec->pix_fmts[i] ==  AV_PIX_FMT_RGB24)
         c->pix_fmt = AV_PIX_FMT_RGB24;
      i++;
    }
  }
#if 0
  if (o->video_preset[0])
    av_dict_set (&codec_options, "preset", o->video_preset, 0);
#endif

  /* open the codec */
  if (avcodec_open2 (c, codec, &codec_options) < 0)
    {
      fprintf (stderr, "could not open codec\n");
      exit (1);
    }

  p->video_outbuf = NULL;
  if (!(oc->oformat->flags & AVFMT_RAWPICTURE))
    {
      /* allocate output buffer */
      /* XXX: API change will be done */
      p->video_outbuf_size = 200000;
      p->video_outbuf = malloc (p->video_outbuf_size);
    }

  /* allocate the encoded raw picture */
  p->picture = alloc_picture (c->pix_fmt, c->width, c->height);
  if (!p->picture)
    {
      fprintf (stderr, "Could not allocate picture\n");
      exit (1);
    }

  /* if the output format is not YUV420P, then a temporary YUV420P
     picture is needed too. It is then converted to the required
     output format */
  p->tmp_picture = NULL;
  if (c->pix_fmt != AV_PIX_FMT_RGB24)
    {
      p->tmp_picture = alloc_picture (AV_PIX_FMT_RGB24, c->width, c->height);
      if (!p->tmp_picture)
        {
          fprintf (stderr, "Could not allocate temporary picture\n");
          exit (1);
        }
    }
}

static void
close_video (Priv * p, AVFormatContext * oc, AVStream * st)
{
  avcodec_close (st->codec);
  av_free (p->picture->data[0]);
  av_free (p->picture);
  if (p->tmp_picture)
    {
      av_free (p->tmp_picture->data[0]);
      av_free (p->tmp_picture);
    }
  av_free (p->video_outbuf);
}

#include "string.h"

/* prepare a dummy image */
static void
fill_rgb_image (GeglProperties *o,
                AVFrame *pict, int frame_index, int width, int height)
{
  Priv     *p = (Priv*)o->user_data;
  GeglRectangle rect={0,0,width,height};
  gegl_buffer_get (p->input, &rect, 1.0, babl_format ("R'G'B' u8"), pict->data[0], GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
}

static void
write_video_frame (GeglProperties *o,
                   AVFormatContext *oc, AVStream *st)
{
  Priv           *p = (Priv*)o->user_data;
  int             out_size, ret;
  AVCodecContext *c;
  AVFrame        *picture_ptr;

  c = st->codec;

  if (c->pix_fmt != AV_PIX_FMT_RGB24)
    {
      struct SwsContext *img_convert_ctx;
      fill_rgb_image (o, p->tmp_picture, p->frame_count, c->width,
                      c->height);

      img_convert_ctx = sws_getContext(c->width, c->height, AV_PIX_FMT_RGB24,
                                       c->width, c->height, c->pix_fmt,
                                       SWS_BICUBIC, NULL, NULL, NULL);

      if (img_convert_ctx == NULL)
        {
          fprintf(stderr, "ff_save: Cannot initialize conversion context.");
        }
      else
        {
          sws_scale(img_convert_ctx,
                    (void*)p->tmp_picture->data,
                    p->tmp_picture->linesize,
                    0,
                    c->height,
                    p->picture->data,
                    p->picture->linesize);
         p->picture->format = c->pix_fmt;
         p->picture->width = c->width;
         p->picture->height = c->height;
         sws_freeContext (img_convert_ctx);
        }
    }
  else
    {
      fill_rgb_image (o, p->picture, p->frame_count, c->width, c->height);
    }

  picture_ptr      = p->picture;
  picture_ptr->pts = p->frame_count;

  if (oc->oformat->flags & AVFMT_RAWPICTURE)
    {
      /* raw video case. The API will change slightly in the near
         future for that */
      AVPacket  pkt;
      av_init_packet (&pkt);

      pkt.flags |= AV_PKT_FLAG_KEY;
      pkt.stream_index = st->index;
      pkt.data = (uint8_t *) picture_ptr;
      pkt.size = sizeof (AVPicture);
      pkt.pts = picture_ptr->pts;
      pkt.dts = picture_ptr->pts;
      av_packet_rescale_ts (&pkt, c->time_base, st->time_base);

      ret = av_write_frame (oc, &pkt);
    }
  else
    {
      /* encode the image */
      AVPacket pkt2;
      int got_packet = 0;
      av_init_packet(&pkt2);
      pkt2.data = p->video_outbuf;
      pkt2.size = p->video_outbuf_size;

      out_size = avcodec_encode_video2(c, &pkt2, picture_ptr, &got_packet);
    
      if (!out_size && got_packet && c->coded_frame)
        {
          c->coded_frame->pts       = pkt2.pts;
          c->coded_frame->key_frame = !!(pkt2.flags & AV_PKT_FLAG_KEY);
          if (c->codec->capabilities & AV_CODEC_CAP_INTRA_ONLY)
              c->coded_frame->pict_type = AV_PICTURE_TYPE_I;
        }

      if (pkt2.side_data_elems > 0)
        {
          int i;
          for (i = 0; i < pkt2.side_data_elems; i++)
            av_free(pkt2.side_data[i].data);
          av_freep(&pkt2.side_data);
          pkt2.side_data_elems = 0;
        }

      if (!out_size)
        out_size = pkt2.size;

      /* if zero size, it means the image was buffered */
      if (out_size != 0)
        {
          AVPacket  pkt;
          av_init_packet (&pkt);
          if (c->coded_frame->key_frame)
            pkt.flags |= AV_PKT_FLAG_KEY;
          pkt.stream_index = st->index;
          pkt.data = p->video_outbuf;
          pkt.size = out_size;
          pkt.pts = picture_ptr->pts;
          pkt.dts = picture_ptr->pts;
          av_packet_rescale_ts (&pkt, c->time_base, st->time_base);
          /* write the compressed frame in the media file */
          ret = av_write_frame (oc, &pkt);
        }
      else
        {
          ret = 0;
        }
    }
  if (ret != 0)
    {
      fprintf (stderr, "Error while writing video frame\n");
      exit (1);
    }
  p->frame_count++;
}

static int
tfile (GeglProperties *o)
{
  Priv *p = (Priv*)o->user_data;

  if (strcmp (o->container_format, "auto"))
    p->fmt = av_guess_format (o->container_format, o->path, NULL);
  else
    p->fmt = av_guess_format (NULL, o->path, NULL);

  if (!p->fmt)
    {
      fprintf (stderr,
               "ff_save couldn't deduce outputformat from file extension: using MPEG.\n%s",
               "");
      p->fmt = av_guess_format ("mpeg", NULL, NULL);
    }
  p->oc = avformat_alloc_context ();
  if (!p->oc)
    {
      fprintf (stderr, "memory error\n%s", "");
      return -1;
    }

  p->oc->oformat = p->fmt;

  snprintf (p->oc->filename, sizeof (p->oc->filename), "%s", o->path);

  p->video_st = NULL;
  p->audio_st = NULL;
  
  if (strcmp (o->video_codec, "auto"))
  {
    AVCodec *codec = avcodec_find_encoder_by_name (o->video_codec);
    p->fmt->video_codec = AV_CODEC_ID_NONE;
    if (codec)
      p->fmt->video_codec = codec->id;
    else
      {
        fprintf (stderr, "didn't find video encoder \"%s\"\navailable codecs: ", o->video_codec);
        while ((codec = av_codec_next (codec)))
          if (av_codec_is_encoder (codec) &&
              avcodec_get_type (codec->id) == AVMEDIA_TYPE_VIDEO)
          fprintf (stderr, "%s ", codec->name);
        fprintf (stderr, "\n");
      }
  }
  if (strcmp (o->audio_codec, "auto"))
  {
    AVCodec *codec = avcodec_find_encoder_by_name (o->audio_codec);
    p->fmt->audio_codec = AV_CODEC_ID_NONE;
    if (codec)
      p->fmt->audio_codec = codec->id;
    else
      {
        fprintf (stderr, "didn't find audio encoder \"%s\"\navailable codecs: ", o->audio_codec);
        while ((codec = av_codec_next (codec)))
          if (av_codec_is_encoder (codec) &&
              avcodec_get_type (codec->id) == AVMEDIA_TYPE_AUDIO)
                fprintf (stderr, "%s ", codec->name);
        fprintf (stderr, "\n");
      }
  }

  if (p->fmt->video_codec != AV_CODEC_ID_NONE)
    {
      p->video_st = add_video_stream (o, p->oc, p->fmt->video_codec);
    }
  if (p->fmt->audio_codec != AV_CODEC_ID_NONE)
    {
     p->audio_st = add_audio_stream (o, p->oc, p->fmt->audio_codec);
    }


  if (p->video_st)
    open_video (o, p->oc, p->video_st);

  if (p->audio_st)
    open_audio (o, p->oc, p->audio_st);

  av_dump_format (p->oc, 0, o->path, 1);

  if (avio_open (&p->oc->pb, o->path, AVIO_FLAG_WRITE) < 0)
    {
      fprintf (stderr, "couldn't open '%s'\n", o->path);
      return -1;
    }

  if (avformat_write_header (p->oc, NULL) < 0)
  {
    fprintf (stderr, "'%s' error writing header\n", o->path);
    return -1;
  }
  return 0;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p = (Priv*)o->user_data;
  static gint     inited = 0;

  g_assert (input);

  if (p == NULL)
    init (o);
  p = (Priv*)o->user_data;

  p->width = result->width;
  p->height = result->height;
  p->input = input;

  if (!inited)
    {
      tfile (o);
      inited = 1;
    }

  write_video_frame (o, p->oc, p->video_st);
  if (p->audio_st)
    write_audio_frame (o, p->oc, p->audio_st);

  return  TRUE;
}

static void flush_audio (GeglProperties *o)
{
  Priv *p = (Priv*)o->user_data;

  int got_packet;
  if (!p->audio_st)
    return;
  do
  {
    AVPacket  pkt = { 0 };
    int ret;
    got_packet = 0;
    av_init_packet (&pkt);
    ret = avcodec_encode_audio2 (p->audio_st->codec, &pkt, NULL, &got_packet);
    if (ret < 0)
      break;
    if (got_packet)
      {
        pkt.stream_index = p->audio_st->index;
        av_packet_rescale_ts (&pkt, p->audio_st->codec->time_base, p->audio_st->time_base);
        av_interleaved_write_frame (p->oc, &pkt);
        av_packet_unref (&pkt);
      }
  } while (got_packet);
}

static void flush_video (GeglProperties *o)
{
  Priv *p = (Priv*)o->user_data;
  int got_packet = 0;
  long ts = p->frame_count;
  do {
    AVPacket  pkt = { 0 };
    int ret;
    got_packet = 0;
    av_init_packet (&pkt);
    ret = avcodec_encode_video2 (p->video_st->codec, &pkt, NULL, &got_packet);
    if (ret < 0)
      return;
      
     if (got_packet)
     {
       pkt.stream_index = p->video_st->index;
       pkt.pts = ts;
       pkt.dts = ts++;
       av_packet_rescale_ts (&pkt, p->video_st->codec->time_base, p->video_st->time_base);
       av_interleaved_write_frame (p->oc, &pkt);
       av_packet_unref (&pkt);
     }
  } while (got_packet);
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);
  if (o->user_data)
    {
      Priv *p = (Priv*)o->user_data;
      flush_audio (o);
      flush_video (o);

      av_write_trailer (p->oc);

      if (p->video_st)
        close_video (p, p->oc, p->video_st);
      if (p->audio_st)
        close_audio (p, p->oc, p->audio_st);

      avio_closep (&p->oc->pb);
      avformat_free_context (p->oc);

      g_free (o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->finalize (object);
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  G_OBJECT_CLASS (klass)->finalize = finalize;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process = process;
  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:ff-save",
    "categories"  , "output:video",
    "description" , _("FFmpeg video output sink"),
    NULL);
}

#endif
