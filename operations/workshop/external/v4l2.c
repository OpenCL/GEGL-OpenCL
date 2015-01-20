/* Video4Linux2 frame source op for GEGL
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
 * Copyright 2004-2008, 2014 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, _("Path"), "/dev/video0")
     description (_("video device path"))
property_int  (width,  _("Width"),  320)
     description (_("Width for rendered image"))
property_int  (height, _("Height"), 240)
     description (_("Height for rendered image"))
property_int  (frame,  _("Frame"),  0)
     description (_("current frame number, can be changed to trigger a reload of the image."))
property_int  (fps, _("FPS"),  0)
     description (_("autotrigger reload this many times a second."))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_C_SOURCE v4l2.c

#include "gegl-op.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

struct buffer {
  void  *start;
  size_t length;
};

typedef struct
{
  gint           active;
  gint           w;
  gint           h;
  gint           w_stored;
  gint           h_stored;
  gint           frame;

  char          *dev_name;
  int            use_mmap;
  int            fd;
  int            io;
  struct buffer *buffers;
  unsigned int   n_buffers;
  int            out_buf;
  int            force_format;
  int            frame_count;
} Priv;

static void
init (GeglProperties *o)
{
  Priv *p = (Priv*)o->user_data;

  if (p==NULL)
    {
      p = g_new0 (Priv, 1);
      o->user_data = (void*) p;
    }

  p->w = 320;
  p->h = 240;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result ={0,0,640,480};
  GeglProperties *o = GEGL_PROPERTIES (operation);

  result.width = o->width;
  result.height = o->height;
  return result;
}

#define CLEAR(x) memset(&(x), 0, sizeof(x))

enum io_method {
	IO_METHOD_MMAP,
	IO_METHOD_READ,
	IO_METHOD_USERPTR,
};

static void errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

static int xioctl(int fh, unsigned long int request, void *arg)
{
	int r;

	do {
		r = ioctl(fh, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

static int read_frame(GeglOperation *operation, GeglBuffer *output)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p= (Priv*)o->user_data;

	struct v4l2_buffer buf;
	unsigned int i;

	switch (p->io) {
	case IO_METHOD_READ:
		if (-1 == read(p->fd, p->buffers[0].start, p->buffers[0].length)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("read");
			}
		}

		//process_image(priv, p->buffers[0].start, p->buffers[0].length);
    gegl_buffer_set (output, NULL, 0, NULL, p->buffers[0].start, GEGL_AUTO_ROWSTRIDE);
		break;

	case IO_METHOD_MMAP:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == xioctl(p->fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		assert(buf.index < p->n_buffers);

		//process_image(priv, p->buffers[buf.index].start, buf.bytesused);
    //
    //
    {
      guchar *capbuf = p->buffers[buf.index].start;
      guchar foobuf[o->width*o->height*4];
          /* XXX: foobuf is unneeded the conversions resets for every
           * scanline and could thus have been done in a line by line
           * manner an fed into the output buffer
           */
      gint y;
      for (y = 0; y < p->h; y++)
        {
          gint       x;

          guchar *dst = &foobuf[y*p->w*4];
          guchar *ysrc = (guchar *) (capbuf + (y) * (p->w) * 2);

          guchar *usrc = ysrc + 3;
          guchar *vsrc = ysrc + 1;



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
#if 0
              dst[0] = *ysrc;
              dst[1] = *ysrc;
              dst[2] = *ysrc;
#else
              dst[0] = B;
              dst[1] = G;
              dst[2] = R;
              dst[3] = 255;
#endif

              dst  += 4;
              ysrc += 2;

              if (x % 2)
                {
                  usrc += 4;
                  vsrc += 4;
                }
            }
        }
    
    gegl_buffer_set (output, NULL, 0, NULL, foobuf,
        GEGL_AUTO_ROWSTRIDE);
    }

		if (-1 == xioctl(p->fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
		break;

	case IO_METHOD_USERPTR:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl(p->fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		for (i = 0; i < p->n_buffers; ++i)
			if (buf.m.userptr == (unsigned long)p->buffers[i].start
			    && buf.length == p->buffers[i].length)
				break;

		assert(i < p->n_buffers);

		//process_image(priv, (void *)buf.m.userptr, buf.bytesused);
    gegl_buffer_set (output, NULL, 0, NULL, (void*)buf.m.userptr, GEGL_AUTO_ROWSTRIDE);

		if (-1 == xioctl(p->fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
		break;
	}

	return 1;
}

static void stop_capturing(Priv *priv)
{
	enum v4l2_buf_type type;

	switch (priv->io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(priv->fd, VIDIOC_STREAMOFF, &type))
			errno_exit("VIDIOC_STREAMOFF");
		break;
	}
}

static void start_capturing(Priv *priv)
{
	unsigned int i;
	enum v4l2_buf_type type;

	switch (priv->io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < priv->n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (-1 == xioctl(priv->fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(priv->fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < priv->n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long)priv->buffers[i].start;
			buf.length = priv->buffers[i].length;

			if (-1 == xioctl(priv->fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(priv->fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");
		break;
	}
}

static void uninit_device(Priv *priv)
{
	unsigned int i;

	switch (priv->io) {
	case IO_METHOD_READ:
		free(priv->buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < priv->n_buffers; ++i)
			if (-1 == munmap(priv->buffers[i].start, priv->buffers[i].length))
				errno_exit("munmap");
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < priv->n_buffers; ++i)
			free(priv->buffers[i].start);
		break;
	}

	free(priv->buffers);
}

static void init_read(Priv *priv, unsigned int buffer_size)
{
	priv->buffers = calloc(1, sizeof(*priv->buffers));

	if (!priv->buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	priv->buffers[0].length = buffer_size;
	priv->buffers[0].start = malloc(buffer_size);

	if (!priv->buffers[0].start) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
}

static void init_mmap(Priv *priv)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(priv->fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				 "memory mapping\n", priv->dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n",
			 priv->dev_name);
		exit(EXIT_FAILURE);
	}

	priv->buffers = calloc(req.count, sizeof(*priv->buffers));

	if (!priv->buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (priv->n_buffers = 0; priv->n_buffers < req.count; ++priv->n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = priv->n_buffers;

		if (-1 == xioctl(priv->fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		priv->buffers[priv->n_buffers].length = buf.length;
		priv->buffers[priv->n_buffers].start =
			mmap(NULL /* start anywhere */,
			      buf.length,
			      PROT_READ | PROT_WRITE /* required */,
			      MAP_SHARED /* recommended */,
			      priv->fd, buf.m.offset);

		if (MAP_FAILED == priv->buffers[priv->n_buffers].start)
			errno_exit("mmap");
	}
}

static void init_userp(Priv *priv, unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count  = 4;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(priv->fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				 "user pointer i/o\n", priv->dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	priv->buffers = calloc(4, sizeof(*priv->buffers));

	if (!priv->buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (priv->n_buffers = 0; priv->n_buffers < 4; ++priv->n_buffers) {
		priv->buffers[priv->n_buffers].length = buffer_size;
		priv->buffers[priv->n_buffers].start = malloc(buffer_size);

		if (!priv->buffers[priv->n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}

static void init_device(Priv *priv)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;

	if (-1 == xioctl(priv->fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n",
				 priv->dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n",
			 priv->dev_name);
		exit(EXIT_FAILURE);
	}

	switch (priv->io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s does not support read i/o\n",
				 priv->dev_name);
			exit(EXIT_FAILURE);
		}
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "%s does not support streaming i/o\n",
				 priv->dev_name);
			exit(EXIT_FAILURE);
		}
		break;
	}


	/* Select video input, video standard and tune here. */


	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(priv->fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl(priv->fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {
		/* Errors ignored. */
	}


	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (priv->force_format) {
		fmt.fmt.pix.width       = priv->w;
		fmt.fmt.pix.height      = priv->h;
		//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

		if (-1 == xioctl(priv->fd, VIDIOC_S_FMT, &fmt))
			errno_exit("VIDIOC_S_FMT");

		/* Note VIDIOC_S_FMT may change width and height. */
	} else {
		/* Preserve original settings as set by v4l2-ctl for example */
		if (-1 == xioctl(priv->fd, VIDIOC_G_FMT, &fmt))
			errno_exit("VIDIOC_G_FMT");
	}

	switch (priv->io) {
	case IO_METHOD_READ:
		init_read(priv, fmt.fmt.pix.sizeimage);
		break;

	case IO_METHOD_MMAP:
		init_mmap(priv);
		break;

	case IO_METHOD_USERPTR:
		init_userp(priv, fmt.fmt.pix.sizeimage);
		break;
	}
}

static void close_device(Priv *priv)
{
	if (-1 == close(priv->fd))
		errno_exit("close");

	priv->fd = -1;
}

static void open_device(Priv *priv)
{
	struct stat st;

	if (-1 == stat(priv->dev_name, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n",
			 priv->dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", priv->dev_name);
		exit(EXIT_FAILURE);
	}

	priv->fd = open(priv->dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

	if (-1 == priv->fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n",
			 priv->dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p= (Priv*)o->user_data;

  if (p == NULL)
    init (o);
  p = (Priv*)o->user_data;

  gegl_operation_set_format (operation, "output",
                            babl_format ("R'G'B'A u8"));


  if (!p->fd)
    {
      p->force_format = 1;
      p->dev_name = o->path;
      p->io = IO_METHOD_MMAP;
      p->w = o->width;
      p->h = o->height;

      open_device (p);
      init_device (p);

      start_capturing (p);
    }
}

static void
finalize (GObject *object)
{
 GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      Priv *p = (Priv*)o->user_data;
      if (p->fd)
      {
	      stop_capturing(p);
        uninit_device (p);
        close_device (p);
        p->fd = 0;
      }
      g_free (o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
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
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv       *p = (Priv*)o->user_data;

  static gboolean inited = FALSE;

    if (!inited && o->fps != 0)
      {
        inited= TRUE;
        g_timeout_add (1000/o->fps, update, operation);
      }


  {
    fd_set fds;
    struct timeval tv;
    FD_ZERO (&fds);
    FD_SET (p->fd, &fds);

    tv.tv_sec = 2;
    tv.tv_usec = 0;

    switch (select (p->fd + 1, &fds, NULL, NULL, &tv))
    {
      case -1:
        if (errno == EINTR)
        {
          g_warning ("select");
          return FALSE;
        }
        break;
      case 0:
        g_warning ("select timeout");
        return FALSE;
        break;
      default:
        read_frame (operation, output);
    }
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
gegl_op_class_init (GeglOpClass *klass)
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
    "name",         "gegl:v4l2",
    "title",        _("Video4Linux2 Frame Source"),
    "categories",   "input:video",
    "description",  _("Video4Linux2 input, webcams framegrabbers and similar devices."),
    NULL);
}

#endif
