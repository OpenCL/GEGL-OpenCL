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
 * Copyright 2003,2004,2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <stdlib.h>

#include <glib/gi18n-lib.h>

#include <libswscale/swscale.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string (path, _("File"), "/tmp/fnord.mp4", _("Target path and filename, use '-' for stdout."))
gegl_chant_double (bitrate, _("Bitrate"), 0.0, 100000000.0, 800000.0, _("target bitrate"))
gegl_chant_double (fps, _("FPS"), 0.0, 100.0, 25, _("frames per second"))

#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE       "ff-save.c"

#include "gegl-chant.h"

#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
#include <libavformat/avformat.h>
#else
#include <avformat.h>
#endif

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

  void     *oxide_audio_instance;
  /*< non NULL audio_query,. means audio present */

  int32_t (*oxide_audio_query) (void *audio_instance,
                                uint32_t * sample_rate,
                                uint32_t * bits,
                                uint32_t * channels,
                                uint32_t * fragment_samples,
                                uint32_t * fragment_size);

  /* get audio samples for the current video frame, this should provide all
   * audiosamples associated with the frame, frame centering on audio stream is
   * undefined (FIXME:<<)
   */

  int32_t (*oxide_audio_get_fragment) (void *audio_instance, uint8_t * buf);

  uint32_t  sample_rate;
  uint32_t  bits;
  uint32_t  channels;
  uint32_t  fragment_samples;
  uint32_t  fragment_size;

  uint8_t  *fragment;

  int       buffer_size;
  int       buffer_read_pos;
  int       buffer_write_pos;
  uint8_t  *buffer;   /* optimal buffer size should be calculated,. preferably
                         run time,. no magic numbers needed for the filewrite
                         case, perhaps a tiny margin for ntsc since it has a
                         strange framerate */

  int       audio_outbuf_size;
  int       audio_input_frame_size;
  int16_t  *samples;
  uint8_t  *audio_outbuf;
} Priv;

#define DISABLE_AUDIO

static void
init (GeglChantO *o)
{
  static gint inited = 0; /*< this is actually meant to be static, only to be done once */
  Priv       *p = (Priv*)o->chant_data;

  if (p == NULL)
    {
      p = g_new0 (Priv, 1);
      o->chant_data = (void*) p;
    }

  if (!inited)
    {
      av_register_all ();
      avcodec_register_all ();
      inited = 1;
    }

#ifndef DISABLE_AUDIO
  p->oxide_audio_instance = gggl_op_sym (op, "oxide_audio_instance");
  p->oxide_audio_query = gggl_op_sym (op, "oxide_audio_query()");
  p->oxide_audio_get_fragment =
    gggl_op_sym (op, "oxide_audio_get_fragment()");

  if (p->oxide_audio_instance && p->oxide_audio_query)
    {
      p->oxide_audio_query (p->oxide_audio_instance,
                            &p->sample_rate,
                            &p->bits,
                            &p->channels,
                            &p->fragment_samples, &p->fragment_size);

      /* FIXME: for now, the buffer is set to a size double that of a oxide
       * provided fragment,. should be enough no matter how things are handled,
       * but it should also be more than needed,. find out exact amount needed later
       */

      if (!p->buffer)
        {
          int size =
            (p->sample_rate / p->fps) * p->channels * (p->bits / 8) * 2;
          buffer_open (op, size);
        }

      if (!p->fragment)
        p->fragment = gggl_op_calloc (op, 1, p->fragment_size);
    }
#endif
}

static void close_video       (Priv            *p,
                               AVFormatContext *oc,
                               AVStream        *st);
void        close_audio       (Priv            *p,
                               AVFormatContext *oc,
                               AVStream        *st);
static int  tfile             (GeglChantO      *self);
static void write_video_frame (GeglChantO      *self,
                               AVFormatContext *oc,
                               AVStream        *st);
static void write_audio_frame (GeglChantO      *self,
                               AVFormatContext *oc,
                               AVStream        *st);




#define STREAM_FRAME_RATE 25    /* 25 images/s */

static int
buffer_used (Priv * p)
{
  int       ret;
  if (!p || !p->buffer_size || !p->buffer)
    return -1;

  if (p->buffer_write_pos == p->buffer_read_pos)
    return 0;
  if (p->buffer_write_pos > p->buffer_read_pos)
    {
      ret = p->buffer_write_pos - p->buffer_read_pos;
    }
  else
    {
      ret = p->buffer_size - (p->buffer_read_pos - p->buffer_write_pos);
    }
  return ret;
}

static int
buffer_unused (Priv * p)
{
  int       ret;
  ret = p->buffer_size - buffer_used (p) - 1;   /* 1 byte for indicating full */
  return ret;
}

#ifndef DISABLE_AUDIO
static void
buffer_flush (Priv * p)
{
  if (!p->buffer)
    return;
  p->buffer_write_pos = 0;
  p->buffer_read_pos = 0;
}

static void
buffer_open (GeglChantOperation *op, int size)
{
  Priv     *p = (Priv*)op->priv;

  if (p->buffer)
    {
      fprintf (stderr, "double init of buffer, eek!\n");
    }

  p->buffer_size = size;
  p->buffer = g_malloc (size);
  buffer_flush (p);
}

static void
buffer_close (GeglChantOperation *op)
{
  Priv     *p = (Priv*)op->priv;

  if (!p->buffer)
    return;
  g_free (p->buffer);
  p->buffer = NULL;
}

#endif

static int
buffer_write (Priv * p, uint8_t * source, int count)
{
  int       first_segment_size = 0;
  int       second_segment_size = 0;

  /* check if we have room for the data in the buffer */
  if (!p->buffer || buffer_unused (p) < count + 1)
    {
      fprintf (stderr, "ff_save audio buffer full!! shouldn't happen\n");
      return -1;
    }

  /* calculate size of segments to write */
  first_segment_size = p->buffer_size - p->buffer_write_pos;

  if (p->buffer_read_pos > p->buffer_write_pos)
    first_segment_size = p->buffer_read_pos - p->buffer_write_pos;

  if (first_segment_size >= count)
    {
      first_segment_size = count;
    }
  else
    {
      second_segment_size = count - first_segment_size;
    }

  memcpy (p->buffer + p->buffer_write_pos, source, first_segment_size);
  p->buffer_write_pos += first_segment_size;

  if (p->buffer_write_pos == p->buffer_size)
    p->buffer_write_pos = 0;

  if (second_segment_size)
    {
      memcpy (p->buffer + p->buffer_write_pos, source + first_segment_size,
              second_segment_size);
      p->buffer_write_pos = second_segment_size;
    }

  return first_segment_size + second_segment_size;
}

#if 0
. = unused byte
# = used byte
  all sizes are specified in contiguous bytes of memory
  __write_pos / __read_pos
  //
  |................................... | empty buffer
  \ _________________________________ /
  size_ / __read_pos __write_pos / /|........
#################..........|
  \ ______________ / used ()_ / __write_pos __read_pos / /|
############..................#####|
  \______________ / unused ()_ / ___write_pos / __read_pos / /|
#################.#################|    full buffer
#endif
     static int
     buffer_read (Priv * p, uint8_t * dest, int count)
{
  int       first_segment_size = 0;
  int       second_segment_size = 0;

  /* check if we have enough data to fulfil request */
  if (!p->buffer || buffer_used (p) < count)
    {
      fprintf (stderr, "ff_save audio buffer doesn't have enough data\n");
      return -1;
    }

  /* calculate size of segments to write */
  first_segment_size = p->buffer_size - p->buffer_read_pos;

  if (p->buffer_write_pos > p->buffer_read_pos)
    first_segment_size = p->buffer_write_pos - p->buffer_read_pos;

  if (first_segment_size >= count)
    {
      first_segment_size = count;
    }
  else
    {
      second_segment_size = count - first_segment_size;
    }

  memcpy (dest, p->buffer + p->buffer_read_pos, first_segment_size);
  p->buffer_read_pos += first_segment_size;

  if (p->buffer_read_pos == p->buffer_size)
    p->buffer_read_pos = 0;

  if (second_segment_size)
    {
      memcpy (dest + first_segment_size, p->buffer + p->buffer_read_pos,
              second_segment_size);
      p->buffer_read_pos = second_segment_size;
    }

  return first_segment_size + second_segment_size;
}

#ifndef DISABLE_AUDIO
/* add an audio output stream */
static AVStream *
add_audio_stream (GeglChantOperation *op, AVFormatContext * oc, int codec_id)
{
  Priv     *p = (Priv*)op->priv;
  AVCodecContext *c;
  AVStream *st;

  p = NULL;
  st = av_new_stream (oc, 1);
  if (!st)
    {
      fprintf (stderr, "Could not alloc stream\n");
      exit (1);
    }

  c = st->codec;
  c->codec_id = codec_id;
  c->codec_type = CODEC_TYPE_AUDIO;

  c->bit_rate = 64000;
  c->sample_rate = 44100;
  c->channels = 2;
  return st;
}
#endif

static void
open_audio (Priv * p, AVFormatContext * oc, AVStream * st)
{
  AVCodecContext *c;
  AVCodec  *codec;

  c = st->codec;

  /* find the audio encoder */
  codec = avcodec_find_encoder (c->codec_id);
  if (!codec)
    {
      fprintf (stderr, "codec not found\n");
      exit (1);
    }

  /* open it */
  if (avcodec_open (c, codec) < 0)
    {
      fprintf (stderr, "could not open codec\n");
      exit (1);
    }

  p->audio_outbuf_size = 10000;
  p->audio_outbuf = malloc (p->audio_outbuf_size);

  /* ugly hack for PCM codecs (will be removed ASAP with new PCM
     support to compute the input frame size in samples */
  if (c->frame_size <= 1)
    {
      fprintf (stderr, "eeko\n");
      p->audio_input_frame_size = p->audio_outbuf_size / c->channels;
      switch (st->codec->codec_id)
        {
        case CODEC_ID_PCM_S16LE:
        case CODEC_ID_PCM_S16BE:
        case CODEC_ID_PCM_U16LE:
        case CODEC_ID_PCM_U16BE:
          p->audio_input_frame_size >>= 1;
          break;
        default:
          break;
        }
    }
  else
    {
      p->audio_input_frame_size = c->frame_size;
    }
  /*audio_input_frame_size = 44100/25;*/
  p->samples = malloc (p->audio_input_frame_size * 2 * c->channels);
}

void
write_audio_frame (GeglChantO *op, AVFormatContext * oc, AVStream * st)
{
  Priv *p = (Priv*)op->chant_data;

  AVCodecContext *c;
  AVPacket  pkt;
  av_init_packet (&pkt);

  c = st->codec;

  /*fprintf (stderr, "going to grab %i\n", p->fragment_size);*/
  if (p->oxide_audio_get_fragment (p->oxide_audio_instance,
                                   p->fragment) == (signed) p->fragment_size)
    {
      buffer_write (p, p->fragment, p->fragment_size);
    }

  while (buffer_used (p) >= p->audio_input_frame_size * 2 * c->channels)
    {
      buffer_read (p, (uint8_t *) p->samples,
                   p->audio_input_frame_size * 2 * c->channels);

      pkt.size = avcodec_encode_audio (c, p->audio_outbuf,
                                       p->audio_outbuf_size, p->samples);

      pkt.pts = c->coded_frame->pts;
      pkt.flags |= PKT_FLAG_KEY;
      pkt.stream_index = st->index;
      pkt.data = p->audio_outbuf;

      if (av_write_frame (oc, &pkt) != 0)
        {
          fprintf (stderr, "Error while writing audio frame\n");
          exit (1);
        }
    }
}

/*p->audio_get_frame (samples, audio_input_frame_size, c->channels);*/

void
close_audio (Priv * p, AVFormatContext * oc, AVStream * st)
{
  avcodec_close (st->codec);

  av_free (p->samples);
  av_free (p->audio_outbuf);
}

/* add a video output stream */
static AVStream *
add_video_stream (GeglChantO *op, AVFormatContext * oc, int codec_id)
{
  Priv *p = (Priv*)op->chant_data;

  AVCodecContext *c;
  AVStream *st;

  st = av_new_stream (oc, 0);
  if (!st)
    {
      fprintf (stderr, "Could not alloc stream %p %p %i\n", op, oc, codec_id);
      exit (1);
    }

  c = st->codec;
  c->codec_id = codec_id;
  c->codec_type = CODEC_TYPE_VIDEO;

  /* put sample propeters */
  c->bit_rate = op->bitrate;
  /* resolution must be a multiple of two */
  c->width = p->width;
  c->height = p->height;
  /* frames per second */
  /*c->frame_rate = op->fps;
  c->frame_rate_base = 1;*/

#if LIBAVCODEC_BUILD >= 4754
  c->time_base=(AVRational){1, op->fps};
    #else
        c->frame_rate=op->fps;
        c->frame_rate_base=1;
    #endif
     c->pix_fmt = PIX_FMT_YUV420P;


  c->gop_size = 12;             /* emit one intra frame every twelve frames at most */
  if (c->codec_id == CODEC_ID_MPEG2VIDEO)
    {
      /* just for testing, we also add B frames */
      /*c->max_b_frames = 2;*/
    }
/*    if (!strcmp (oc->oformat->name, "mp4") ||
          !strcmp (oc->oformat->name, "3gp"))
    c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    */
  return st;
}


static AVFrame *
alloc_picture (int pix_fmt, int width, int height)
{
  AVFrame  *picture;
  uint8_t  *picture_buf;
  int       size;

  picture = avcodec_alloc_frame ();
  if (!picture)
    return NULL;
  size = avpicture_get_size (pix_fmt, width, height);
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
open_video (Priv * p, AVFormatContext * oc, AVStream * st)
{
  AVCodec  *codec;
  AVCodecContext *c;

  c = st->codec;

  /* find the video encoder */
  codec = avcodec_find_encoder (c->codec_id);
  if (!codec)
    {
      fprintf (stderr, "codec not found\n");
      exit (1);
    }

  /* open the codec */
  if (avcodec_open (c, codec) < 0)
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
  if (c->pix_fmt != PIX_FMT_RGB24)
    {
      p->tmp_picture = alloc_picture (PIX_FMT_RGB24, c->width, c->height);
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
fill_yuv_image (GeglChantO *op,
                AVFrame *pict, int frame_index, int width, int height)
{
  Priv     *p = (Priv*)op->chant_data;
  /*memcpy (pict->data[0],

   op->input_pad[0]->data,

          op->input_pad[0]->width * op->input_pad[0]->height * 3);*/
  GeglRectangle rect={0,0,width,height};
  gegl_buffer_get (p->input, 1.0, &rect, babl_format ("R'G'B' u8"), pict->data[0], GEGL_AUTO_ROWSTRIDE);
}

static void
write_video_frame (GeglChantO *op,
                   AVFormatContext *oc, AVStream *st)
{
  Priv     *p = (Priv*)op->chant_data;
  int       out_size, ret;
  AVCodecContext *c;
  AVFrame  *picture_ptr;

  c = st->codec;

  if (c->pix_fmt != PIX_FMT_RGB24)
    {
      struct SwsContext *img_convert_ctx;

      /* as we only generate a RGB24 picture, we must convert it
         to the codec pixel format if needed */
      fill_yuv_image (op, p->tmp_picture, p->frame_count, c->width,
                      c->height);

      img_convert_ctx = sws_getContext(c->width, c->height, c->pix_fmt,
                                       c->width, c->height, PIX_FMT_RGB24,
                                       SWS_BICUBIC, NULL, NULL, NULL);

      if (img_convert_ctx == NULL)
        {
          fprintf(stderr, "ff_save: Cannot initialize conversion context.");
        }
      else
        {
          sws_scale(img_convert_ctx,
                    p->tmp_picture->data,
                    p->tmp_picture->linesize,
                    0,
                    c->height,
                    p->picture->data,
                    p->picture->linesize);
        }
    }
  else
    {
      fill_yuv_image (op, p->picture, p->frame_count, c->width, c->height);
    }
  picture_ptr = p->picture;

  if (oc->oformat->flags & AVFMT_RAWPICTURE)
    {
      /* raw video case. The API will change slightly in the near
         future for that */
      AVPacket  pkt;
      av_init_packet (&pkt);

      pkt.flags |= PKT_FLAG_KEY;
      pkt.stream_index = st->index;
      pkt.data = (uint8_t *) picture_ptr;
      pkt.size = sizeof (AVPicture);

      ret = av_write_frame (oc, &pkt);
    }
  else
    {
      /* encode the image */
      out_size =
        avcodec_encode_video (c,
                              p->video_outbuf,
                              p->video_outbuf_size, picture_ptr);

      /* if zero size, it means the image was buffered */
      if (out_size != 0)
        {
          AVPacket  pkt;
          av_init_packet (&pkt);

          pkt.pts = c->coded_frame->pts;
          if (c->coded_frame->key_frame)
            pkt.flags |= PKT_FLAG_KEY;
          pkt.stream_index = st->index;
          pkt.data = p->video_outbuf;
          pkt.size = out_size;

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
tfile (GeglChantO *self)
{
  Priv *p = (Priv*)self->chant_data;

  p->fmt = guess_format (NULL, self->path, NULL);
  if (!p->fmt)
    {
      fprintf (stderr,
               "ff_save couldn't deduce outputformat from file extension: using MPEG.\n%s",
               "");
      p->fmt = guess_format ("mpeg", NULL, NULL);
    }
  p->oc = av_alloc_format_context ();/*g_malloc (sizeof (AVFormatContext));*/
  if (!p->oc)
    {
      fprintf (stderr, "memory error\n%s", "");
      return -1;
    }

  p->oc->oformat = p->fmt;

  snprintf (p->oc->filename, sizeof (p->oc->filename), "%s", self->path);

  p->video_st = NULL;
  p->audio_st = NULL;

  if (p->fmt->video_codec != CODEC_ID_NONE)
    {
      p->video_st = add_video_stream (self, p->oc, p->fmt->video_codec);
    }
  if (p->oxide_audio_query && p->fmt->audio_codec != CODEC_ID_NONE)
    {
     /*XXX: FOO p->audio_st = add_audio_stream (op, p->oc, p->fmt->audio_codec);*/
    }

  if (av_set_parameters (p->oc, NULL) < 0)
    {
      fprintf (stderr, "Invalid output format propeters\n%s", "");
      return -1;
    }

  dump_format (p->oc, 0, self->path, 1);

  if (p->video_st)
    open_video (p, p->oc, p->video_st);
  if (p->audio_st)
    open_audio (p, p->oc, p->audio_st);

  if (url_fopen (&p->oc->pb, self->path, URL_WRONLY) < 0)
    {
      fprintf (stderr, "couldn't open '%s'\n", self->path);
      return -1;
    }

  av_write_header (p->oc);

  return 0;
}

#if 0
static int
filechanged (GeglChantOperation *op, const char *att)
{
  init (op);
  return 0;
}
#endif

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result)
{
  static gint inited = 0;
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  Priv       *p = (Priv*)o->chant_data;

  g_assert (input);

  if (p == NULL)
    init (o);
  p = (Priv*)o->chant_data;

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

static void
finalize (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);
  if (o->chant_data)
    {
      Priv *p = (Priv*)o->chant_data;

    if (p->oc)
      {
        gint i;
        if (p->video_st)
          close_video (p, p->oc, p->video_st);
        if (p->audio_st)
          close_audio (p, p->oc, p->audio_st);

        av_write_trailer (p->oc);

        for (i = 0; i < p->oc->nb_streams; i++)
          {
            av_freep (&p->oc->streams[i]);
          }

        url_fclose (&p->oc->pb);
        free (p->oc);
      }
      g_free (o->chant_data);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->finalize (object);
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  G_OBJECT_CLASS (klass)->finalize = finalize;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process = process;
  sink_class->needs_full = TRUE;

  operation_class->name        = "gegl:ff-save";
  operation_class->categories  = "output:video";
  operation_class->description = _("FFmpeg video output sink");
}

#endif
