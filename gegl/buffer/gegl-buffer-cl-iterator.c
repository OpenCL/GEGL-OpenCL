/* This file is part of GEGL
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
 * Copyright 2012 Victor Oliveira (victormatheus@gmail.com)
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#include "gegl.h"
#include "gegl/gegl-debug.h"

#include "gegl-buffer-types.h"
#include "gegl-buffer-cl-iterator.h"
#include "gegl-buffer-cl-cache.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"

#include "opencl/gegl-cl.h"

typedef struct GeglBufferClIterators
{
  /* current region of interest */
  size_t        size [GEGL_CL_BUFFER_MAX_ITERATORS];  /* length of current data in pixels */
  cl_mem        tex  [GEGL_CL_BUFFER_MAX_ITERATORS];
  GeglRectangle roi  [GEGL_CL_BUFFER_MAX_ITERATORS];

  /* the following is private: */
  cl_mem        tex_buf [GEGL_CL_BUFFER_MAX_ITERATORS];
  cl_mem        tex_op  [GEGL_CL_BUFFER_MAX_ITERATORS];

  /* don't free textures loaded from cache */
  gboolean       tex_buf_from_cache [GEGL_CL_BUFFER_MAX_ITERATORS];

  gint           iterators;
  gint           iteration_no;
  gboolean       is_finished;

  guint          flags          [GEGL_CL_BUFFER_MAX_ITERATORS];
  gint           area           [GEGL_CL_BUFFER_MAX_ITERATORS][4];

  GeglRectangle  rect           [GEGL_CL_BUFFER_MAX_ITERATORS]; /* the region we iterate on. They can be
                                                                   different from each other, but width
                                                                   and height are the same */

  const Babl    *format         [GEGL_CL_BUFFER_MAX_ITERATORS]; /* The format required for the data */
  GeglBuffer    *buffer         [GEGL_CL_BUFFER_MAX_ITERATORS];

  /* GeglBuffer's buffer */
  size_t buf_cl_format_size     [GEGL_CL_BUFFER_MAX_ITERATORS];
  /* Iterator's format */
  size_t op_cl_format_size      [GEGL_CL_BUFFER_MAX_ITERATORS];

  GeglClColorOp conv            [GEGL_CL_BUFFER_MAX_ITERATORS];

  GeglAbyssPolicy abyss_policy  [GEGL_CL_BUFFER_MAX_ITERATORS];

  /* total iteration */
  gint           rois;
  GeglRectangle *roi_all;

} GeglBufferClIterators;

gint
gegl_buffer_cl_iterator_add_2 (GeglBufferClIterator  *iterator,
                               GeglBuffer            *buffer,
                               const GeglRectangle   *result,
                               const Babl            *format,
                               guint                  flags,
                               gint                   left,
                               gint                   right,
                               gint                   top,
                               gint                   bottom,
                               GeglAbyssPolicy        abyss_policy)
{
  GeglBufferClIterators *i = (gpointer)iterator;
  gint self = 0;
  if (i->iterators+1 > GEGL_CL_BUFFER_MAX_ITERATORS)
    {
      g_error ("too many iterators (%i)", i->iterators+1);
    }

  if (i->iterators == 0) /* for sanity, we zero at init */
    {
      memset (i, 0, sizeof (GeglBufferClIterators));
    }

  self = i->iterators++;

  if (!result)
    result = self==0?&(buffer->extent):&(i->rect[0]);
  i->rect[self]=*result;

  i->flags[self]=flags;

  i->abyss_policy[self]=abyss_policy;
  if(flags != GEGL_CL_BUFFER_READ && abyss_policy != GEGL_ABYSS_NONE)
    g_error ("invalid abyss");

  if (flags == GEGL_CL_BUFFER_WRITE || flags == GEGL_CL_BUFFER_READ)
    {
      const Babl *buffer_format = gegl_buffer_get_format (buffer);

      g_assert (buffer);

      i->buffer[self]= g_object_ref (buffer);

      if (format)
        i->format[self] = format;
      else
        i->format[self] = buffer_format;

      if (flags == GEGL_CL_BUFFER_WRITE)
        i->conv[self] = gegl_cl_color_supported (format, buffer_format);
      else
        i->conv[self] = gegl_cl_color_supported (buffer_format, format);

      gegl_cl_color_babl (buffer_format, &i->buf_cl_format_size[self]);
      gegl_cl_color_babl (format,        &i->op_cl_format_size [self]);

      /* alpha, non-alpha & GEGL_ABYSS_NONE */
      if (babl_format_has_alpha(buffer_format) != babl_format_has_alpha(format) &&
          abyss_policy == GEGL_ABYSS_NONE &&
          !gegl_rectangle_contains (gegl_buffer_get_extent (buffer), result))
        {
          i->conv[self] = GEGL_CL_COLOR_NOT_SUPPORTED;
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Performance warning: not using color conversion "
                                        "with OpenCL because abyss depends on alpha");
        }
    }
  else /* GEGL_CL_BUFFER_AUX */
    {
      g_assert (buffer == NULL);

      i->buffer[self] = NULL;
      i->format[self] = NULL;
      i->conv[self]   = -1;
      i->buf_cl_format_size[self] = SIZE_MAX;

      gegl_cl_color_babl (format, &i->op_cl_format_size [self]);
    }

  i->area[self][0] = left;
  i->area[self][1] = right;
  i->area[self][2] = top;
  i->area[self][3] = bottom;

  if (flags == GEGL_CL_BUFFER_WRITE
      && (left > 0 || right > 0 || top > 0 || bottom > 0))
	g_assert(FALSE);

  if (self!=0)
    {
      /* we make all subsequently added iterators share the width and height of the first one */
      i->rect[self].width  = i->rect[0].width;
      i->rect[self].height = i->rect[0].height;
    }
  else
    {
      gint x, y, j;

      i->rois = 0;
      for (y=result->y; y < result->y + result->height; y += gegl_cl_get_iter_height ())
        for (x=result->x; x < result->x + result->width;  x += gegl_cl_get_iter_width ())
          i->rois++;

      i->iteration_no = 0;

      i->roi_all = g_new0 (GeglRectangle, i->rois);

      j = 0;
      for (y=0; y < result->height; y += gegl_cl_get_iter_height ())
        for (x=0; x < result->width;  x += gegl_cl_get_iter_width ())
          {
            GeglRectangle r = {x, y,
                               MIN(gegl_cl_get_iter_width (),  result->width  - x),
                               MIN(gegl_cl_get_iter_height (), result->height - y)};
            i->roi_all[j] = r;
            j++;
          }
    }

  return self;
}

gint
gegl_buffer_cl_iterator_add (GeglBufferClIterator  *iterator,
                             GeglBuffer            *buffer,
                             const GeglRectangle   *roi,
                             const Babl            *format,
                             guint                  flags,
                             GeglAbyssPolicy        abyss_policy)
{
  return gegl_buffer_cl_iterator_add_2 (iterator,
                                        buffer, roi,
                                        format, flags,
                                        0, 0, 0, 0,
                                        abyss_policy);
}

gint
gegl_buffer_cl_iterator_add_aux  (GeglBufferClIterator  *iterator,
                                  const GeglRectangle   *roi,
                                  const Babl            *format,
                                  gint                   left,
                                  gint                   right,
                                  gint                   top,
                                  gint                   bottom)
{
  return gegl_buffer_cl_iterator_add_2 (iterator,
                                        NULL, roi,
                                        format, GEGL_CL_BUFFER_AUX,
                                        left, right,
                                        top, bottom,
                                        GEGL_ABYSS_NONE);
}

static void
dealloc_iterator(GeglBufferClIterators *i)
{
  int no;

  for (no=0; no<i->iterators;no++)
    {
      if (i->buffer[no])
        {
          gint j;
          gboolean found = FALSE;
          for (j=0; j<no; j++)
            if (i->buffer[no]==i->buffer[j])
              {
                found = TRUE;
                break;
              }
          if (!found)
            gegl_buffer_unlock (i->buffer[no]);

          g_object_unref (i->buffer[no]);
        }
    }

  g_free (i->roi_all);
  g_slice_free (GeglBufferClIterators, i);
}

#define OPENCL_USE_CACHE 1

gboolean
gegl_buffer_cl_iterator_next (GeglBufferClIterator *iterator, gboolean *err)
{
  GeglBufferClIterators *i = (gpointer)iterator;
  gint no;
  cl_int cl_err = 0;
  int color_err = 0;
  gboolean is_finished;

  if (i->is_finished)
    g_error ("%s called on finished buffer iterator", G_STRFUNC);

  if (i->iteration_no == 0)
    {
      for (no=0; no<i->iterators;no++)
        {
          if (i->buffer[no])
            {
              gint j;
              gboolean found = FALSE;
              for (j=0; j<no; j++)
                if (i->buffer[no]==i->buffer[j])
                  {
                    found = TRUE;
                    break;
                  }
              if (!found)
                gegl_buffer_lock (i->buffer[no]);

              if (i->flags[no] == GEGL_CL_BUFFER_WRITE
                  || (i->flags[no] == GEGL_CL_BUFFER_READ
                      && (i->area[no][0] > 0 || i->area[no][1] > 0 || i->area[no][2] > 0 || i->area[no][3] > 0)))
                {
                  gegl_buffer_cl_cache_flush (i->buffer[no], &i->rect[no]);
                }
            }
        }
    }
  else
    {
      /* complete pending write work */
      for (no=0; no<i->iterators;no++)
        {
          if (i->flags[no] == GEGL_CL_BUFFER_WRITE)
            {
              /* color conversion in the GPU (output) */
              if (i->conv[no] == GEGL_CL_COLOR_CONVERT)
                  {
                    color_err = gegl_cl_color_conv (i->tex_op[no], i->tex_buf[no], i->size[no],
                                                    i->format[no], gegl_buffer_get_format (i->buffer[no]));
                    if (color_err) goto error;
                  }

              /* GPU -> CPU */
                {
                  gpointer data;

                  /* tile-ize */
                  if (i->conv[no] == GEGL_CL_COLOR_NOT_SUPPORTED)
                    {
                      data = g_malloc(i->size[no] * i->op_cl_format_size [no]);

                      cl_err = gegl_clEnqueueReadBuffer(gegl_cl_get_command_queue(),
                                                        i->tex_op[no], CL_TRUE,
                                                        0, i->size[no] * i->op_cl_format_size[no], data,
                                                        0, NULL, NULL);
                      CL_CHECK;

                      /* color conversion using BABL */
                      gegl_buffer_set (i->buffer[no], &i->roi[no], 0, i->format[no], data, GEGL_AUTO_ROWSTRIDE);

                      g_free(data);
                    }
                  else
#ifdef OPENCL_USE_CACHE
                    {
                      gegl_buffer_cl_cache_new (i->buffer[no], &i->roi[no], i->tex_buf[no]);
                      /* don't release this texture */
                      i->tex_buf[no] = NULL;
                    }
#else
                    {
                      data = gegl_clEnqueueMapBuffer(gegl_cl_get_command_queue(), i->tex_buf[no], CL_TRUE,
                                                     CL_MAP_READ,
                                                     0, i->size[no] * i->buf_cl_format_size [no],
                                                     0, NULL, NULL, &cl_err);
                      CL_CHECK;

                      /* color conversion using BABL */
                      gegl_buffer_set (i->buffer[no], &i->roi[no], i->format[no], data, GEGL_AUTO_ROWSTRIDE);

                      cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), i->tex_buf[no], data,
                                                             0, NULL, NULL);
                      CL_CHECK;
                    }
#endif
                }
            }
        }

      /* Run! */
      cl_err = gegl_clFinish(gegl_cl_get_command_queue());
      CL_CHECK;

      for (no=0; no < i->iterators; no++)
          {
            if (i->tex_buf_from_cache [no])
              {
                gboolean ok = gegl_buffer_cl_cache_release (i->tex_buf[no]);
                g_assert (ok);
              }

            if (i->tex_buf[no] && !i->tex_buf_from_cache [no])
              gegl_clReleaseMemObject (i->tex_buf[no]);

            if (i->tex_op [no])
              gegl_clReleaseMemObject (i->tex_op [no]);

            i->tex    [no] = NULL;
            i->tex_buf[no] = NULL;
            i->tex_op [no] = NULL;
          }
    }

  g_assert (i->iterators > 0);
  is_finished = i->is_finished = (i->iteration_no >= i->rois);

  /* then we iterate all */
  if (!i->is_finished)
    {
      for (no=0; no<i->iterators;no++)
        {
            {
              GeglRectangle r = {i->rect[no].x + i->roi_all[i->iteration_no].x - i->area[no][0],
                                 i->rect[no].y + i->roi_all[i->iteration_no].y - i->area[no][2],
                                 i->roi_all[i->iteration_no].width             + i->area[no][0] + i->area[no][1],
                                 i->roi_all[i->iteration_no].height            + i->area[no][2] + i->area[no][3]};
              i->roi [no] = r;
              i->size[no] = r.width * r.height;

              g_assert(i->size[no] > 0);
            }

          if (i->flags[no] == GEGL_CL_BUFFER_READ)
            {
                {
                  gpointer data;

                  /* un-tile */
                  switch (i->conv[no])
                    {
                      case GEGL_CL_COLOR_NOT_SUPPORTED:

                        {
                        gegl_buffer_cl_cache_flush (i->buffer[no], &i->roi[no]);

                        g_assert (i->tex_op[no] == NULL);
                        i->tex_op[no] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                                CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY,
                                                                i->size[no] * i->op_cl_format_size [no],
                                                                NULL, &cl_err);
                        CL_CHECK;

                        /* pre-pinned memory */
                        data = gegl_clEnqueueMapBuffer(gegl_cl_get_command_queue(), i->tex_op[no], CL_TRUE,
                                                       CL_MAP_WRITE,
                                                       0, i->size[no] * i->op_cl_format_size [no],
                                                       0, NULL, NULL, &cl_err);
                        CL_CHECK;

                        /* color conversion using BABL */
                        gegl_buffer_get (i->buffer[no], &i->roi[no], 1.0, i->format[no], data, GEGL_AUTO_ROWSTRIDE, i->abyss_policy[no]);

                        cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), i->tex_op[no], data,
                                                                   0, NULL, NULL);
                        CL_CHECK;

                        i->tex[no] = i->tex_op[no];

                        break;
                        }

                      case GEGL_CL_COLOR_EQUAL:

                        {
                        i->tex_buf[no] = gegl_buffer_cl_cache_get (i->buffer[no], &i->roi[no]);

                        if (i->tex_buf[no])
                          i->tex_buf_from_cache [no] = TRUE; /* don't free texture from cache */
                        else
                          {
                            gegl_buffer_cl_cache_flush (i->buffer[no], &i->roi[no]);

                            g_assert (i->tex_buf[no] == NULL);
                            i->tex_buf[no] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                                     CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY,
                                                                     i->size[no] * i->buf_cl_format_size [no],
                                                                     NULL, &cl_err);
                            CL_CHECK;

                            /* pre-pinned memory */
                            data = gegl_clEnqueueMapBuffer(gegl_cl_get_command_queue(), i->tex_buf[no], CL_TRUE,
                                                           CL_MAP_WRITE,
                                                           0, i->size[no] * i->buf_cl_format_size [no],
                                                           0, NULL, NULL, &cl_err);
                            CL_CHECK;

                            /* color conversion will be performed in the GPU later */
                            gegl_buffer_get (i->buffer[no], &i->roi[no], 1.0, NULL, data, GEGL_AUTO_ROWSTRIDE, i->abyss_policy[no]);

                            cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), i->tex_buf[no], data,
                                                                   0, NULL, NULL);
                            CL_CHECK;
                          }

                        i->tex[no] = i->tex_buf[no];

                        break;
                        }

                      case GEGL_CL_COLOR_CONVERT:

                        {
                        i->tex_buf[no] = gegl_buffer_cl_cache_get (i->buffer[no], &i->roi[no]);

                        if (i->tex_buf[no])
                          i->tex_buf_from_cache [no] = TRUE; /* don't free texture from cache */
                        else
                          {
                            gegl_buffer_cl_cache_flush (i->buffer[no], &i->roi[no]);

                            g_assert (i->tex_buf[no] == NULL);
                            i->tex_buf[no] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                                     CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY,
                                                                     i->size[no] * i->buf_cl_format_size [no],
                                                                     NULL, &cl_err);
                            CL_CHECK;

                            /* pre-pinned memory */
                            data = gegl_clEnqueueMapBuffer(gegl_cl_get_command_queue(), i->tex_buf[no], CL_TRUE,
                                                           CL_MAP_WRITE,
                                                           0, i->size[no] * i->buf_cl_format_size [no],
                                                           0, NULL, NULL, &cl_err);
                            CL_CHECK;

                            /* color conversion will be performed in the GPU later */
                            gegl_buffer_get (i->buffer[no], &i->roi[no], 1.0, NULL, data, GEGL_AUTO_ROWSTRIDE, i->abyss_policy[no]);

                            cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), i->tex_buf[no], data,
                                                                   0, NULL, NULL);
                            CL_CHECK;
                          }

                        g_assert (i->tex_op[no] == NULL);
                        i->tex_op[no] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                                CL_MEM_READ_WRITE,
                                                                i->size[no] * i->op_cl_format_size [no],
                                                                NULL, &cl_err);
                        CL_CHECK;

                        /* color conversion in the GPU (input) */
                        g_assert (i->tex_buf[no] && i->tex_op[no]);
                        color_err = gegl_cl_color_conv (i->tex_buf[no], i->tex_op[no], i->size[no],
                                                        gegl_buffer_get_format (i->buffer[no]), i->format[no]);
                        if (color_err) goto error;

                        i->tex[no] = i->tex_op[no];

                        break;
                        }
                    }
                }
            }
          else if (i->flags[no] == GEGL_CL_BUFFER_WRITE)
            {
                {
                  switch (i->conv[no])
                    {
                      case GEGL_CL_COLOR_NOT_SUPPORTED:

                      {
                      g_assert (i->tex_op[no] == NULL);
                      i->tex_op[no] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                              CL_MEM_WRITE_ONLY,
                                                              i->size[no] * i->op_cl_format_size [no],
                                                              NULL, &cl_err);
                      CL_CHECK;

                      i->tex[no] = i->tex_op[no];

                      break;
                      }

                      case GEGL_CL_COLOR_EQUAL:

                      {
                      g_assert (i->tex_buf[no] == NULL);
                      i->tex_buf[no] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                               CL_MEM_READ_WRITE, /* cache */
                                                               i->size[no] * i->buf_cl_format_size [no],
                                                               NULL, &cl_err);
                      CL_CHECK;

                      i->tex[no] = i->tex_buf[no];

                      break;
                      }

                      case GEGL_CL_COLOR_CONVERT:

                      {
                      g_assert (i->tex_buf[no] == NULL);
                      i->tex_buf[no] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                               CL_MEM_READ_WRITE, /* cache */
                                                               i->size[no] * i->buf_cl_format_size [no],
                                                               NULL, &cl_err);
                      CL_CHECK;

                      g_assert (i->tex_op[no] == NULL);
                      i->tex_op[no] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                              CL_MEM_READ_WRITE,
                                                              i->size[no] * i->op_cl_format_size [no],
                                                              NULL, &cl_err);
                      CL_CHECK;

                      i->tex[no] = i->tex_op[no];

                      break;
                      }
                   }
                }
            }
          else if (i->flags[no] == GEGL_CL_BUFFER_AUX)
            {
                {
                  g_assert (i->tex_op[no] == NULL);
                  i->tex_op[no] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                          CL_MEM_READ_WRITE,
                                                          i->size[no] * i->op_cl_format_size [no],
                                                          NULL, &cl_err);
                  CL_CHECK;

                  i->tex[no] = i->tex_op[no];
                }
            }
        }

      i->iteration_no++;
    }
  else /* i->is_finished == TRUE */
    {
      dealloc_iterator(i);
    }

  if (err)
    *err = FALSE;
  return !is_finished;

error:
  gegl_buffer_cl_iterator_stop ((GeglBufferClIterator *)i);

  if (err)
    *err = TRUE;
  return FALSE;
}

void
gegl_buffer_cl_iterator_stop (GeglBufferClIterator *iterator)
{
  GeglBufferClIterators *i = (GeglBufferClIterators *)iterator;
  int no;

  for (no = 0; no < i->iterators; no++)
    {
      if (i->tex_buf[no]) gegl_clReleaseMemObject (i->tex_buf[no]);
      if (i->tex_op [no]) gegl_clReleaseMemObject (i->tex_op [no]);

      i->tex    [no] = NULL;
      i->tex_buf[no] = NULL;
      i->tex_op [no] = NULL;
    }

  dealloc_iterator (i);
}

GeglBufferClIterator *
gegl_buffer_cl_iterator_new (GeglBuffer          *buffer,
                             const GeglRectangle *roi,
                             const Babl          *format,
                             guint                flags)
{
  GeglBufferClIterator *i = (gpointer)g_slice_new0 (GeglBufferClIterators);
  /* Because the iterator is nulled above, we can forgo explicitly setting
   * i->is_finished to FALSE. */
  gegl_buffer_cl_iterator_add (i, buffer, roi, format, flags, GEGL_ABYSS_NONE);
  return i;
}
