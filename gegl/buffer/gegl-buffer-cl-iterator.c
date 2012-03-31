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
#include "gegl-utils.h"

#define CL_ERROR {GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error in %s:%d@%s - %s\n", __FILE__, __LINE__, __func__, gegl_cl_errstring(cl_err)); goto error;}

typedef struct GeglBufferClIterators
{
  /* current region of interest */
  gint          n;
  size_t        size [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX];  /* length of current data in pixels */
  cl_mem        tex  [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX];
  GeglRectangle roi  [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX];

  /* the following is private: */
  cl_mem        tex_buf [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX];
  cl_mem        tex_op  [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX];

  /* don't free textures loaded from cache */
  gboolean       tex_buf_from_cache [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX];

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

  /* buffer->soft_format */
  size_t buf_cl_format_size     [GEGL_CL_BUFFER_MAX_ITERATORS];
  /* format */
  size_t op_cl_format_size      [GEGL_CL_BUFFER_MAX_ITERATORS];

  gegl_cl_color_op conv         [GEGL_CL_BUFFER_MAX_ITERATORS];

  /* total iteration */
  gint           rois;
  gint           roi_no;
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

  if (flags == GEGL_CL_BUFFER_WRITE || flags == GEGL_CL_BUFFER_READ)
    {
      g_assert (buffer);

      i->buffer[self]= g_object_ref (buffer);

      if (format)
        i->format[self]=format;
      else
        i->format[self]=buffer->soft_format;

      if (flags == GEGL_CL_BUFFER_WRITE)
        i->conv[self] = gegl_cl_color_supported (format, buffer->soft_format);
      else
        i->conv[self] = gegl_cl_color_supported (buffer->soft_format, format);

      gegl_cl_color_babl (buffer->soft_format, &i->buf_cl_format_size[self]);
      gegl_cl_color_babl (format,              &i->op_cl_format_size [self]);
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

      i->roi_no = 0;
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
                             const GeglRectangle   *result,
                             const Babl            *format,
                             guint                  flags,
                             GeglAbyssPolicy        abyss_policy)
{
  return gegl_buffer_cl_iterator_add_2 (iterator, buffer, result, format, flags, 0,0,0,0, abyss_policy);
}

#define OPENCL_USE_CACHE 1

gboolean
gegl_buffer_cl_iterator_next (GeglBufferClIterator *iterator, gboolean *err)
{
  GeglBufferClIterators *i = (gpointer)iterator;
  gboolean result = FALSE;
  gint no, j;
  cl_int cl_err = 0;

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
              /* Wait Processing */
              cl_err = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
              if (cl_err != CL_SUCCESS) CL_ERROR;

              /* color conversion in the GPU (output) */
              if (i->conv[no] == GEGL_CL_COLOR_CONVERT)
                for (j=0; j < i->n; j++)
                  {
                    cl_err = gegl_cl_color_conv (i->tex_op[no][j], i->tex_buf[no][j], i->size[no][j],
                                                 i->format[no], i->buffer[no]->soft_format);
                    if (cl_err == FALSE) CL_ERROR;
                  }

              /* Wait Processing */
              cl_err = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
              if (cl_err != CL_SUCCESS) CL_ERROR;

              /* GPU -> CPU */
              for (j=0; j < i->n; j++)
                {
                  gpointer data;

                  /* tile-ize */
                  if (i->conv[no] == GEGL_CL_COLOR_NOT_SUPPORTED)
                    {
                      data = gegl_clEnqueueMapBuffer(gegl_cl_get_command_queue(), i->tex_op[no][j], CL_TRUE,
                                                     CL_MAP_READ,
                                                     0, i->size[no][j] * i->op_cl_format_size [no],
                                                     0, NULL, NULL, &cl_err);
                      if (cl_err != CL_SUCCESS) CL_ERROR;

                      /* color conversion using BABL */
                      gegl_buffer_set (i->buffer[no], &i->roi[no][j], 0, i->format[no], data, GEGL_AUTO_ROWSTRIDE);

                      cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), i->tex_op[no][j], data,
                                                             0, NULL, NULL);
                      if (cl_err != CL_SUCCESS) CL_ERROR;
                    }
                  else
#ifdef OPENCL_USE_CACHE
                    {
                      gegl_buffer_cl_cache_new (i->buffer[no], &i->roi[no][j], i->tex_buf[no][j]);
                      /* don't release this texture */
                      i->tex_buf[no][j] = NULL;
                    }
#else
                    {
                      data = gegl_clEnqueueMapBuffer(gegl_cl_get_command_queue(), i->tex_buf[no][j], CL_TRUE,
                                                     CL_MAP_READ,
                                                     0, i->size[no][j] * i->buf_cl_format_size [no],
                                                     0, NULL, NULL, &cl_err);
                      if (cl_err != CL_SUCCESS) CL_ERROR;

                      /* color conversion using BABL */
                      gegl_buffer_set (i->buffer[no], &i->roi[no][j], i->format[no], data, GEGL_AUTO_ROWSTRIDE);

                      cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), i->tex_buf[no][j], data,
                                                             0, NULL, NULL);
                      if (cl_err != CL_SUCCESS) CL_ERROR;
                    }
#endif
                }
            }
        }

      /* Run! */
      cl_err = gegl_clFinish(gegl_cl_get_command_queue());
      if (cl_err != CL_SUCCESS) CL_ERROR;

      for (no=0; no < i->iterators; no++)
        for (j=0; j < i->n; j++)
          {
            if (i->tex_buf_from_cache [no][j])
              {
                gboolean ok = gegl_buffer_cl_cache_release (i->tex_buf[no][j]);
                g_assert (ok);
              }

            if (i->tex_buf[no][j] && !i->tex_buf_from_cache [no][j])
              gegl_clReleaseMemObject (i->tex_buf[no][j]);

            if (i->tex_op [no][j])
              gegl_clReleaseMemObject (i->tex_op [no][j]);

            i->tex    [no][j] = NULL;
            i->tex_buf[no][j] = NULL;
            i->tex_op [no][j] = NULL;
          }
    }

  g_assert (i->iterators > 0);
  result = (i->roi_no >= i->rois)? FALSE : TRUE;

  i->n = MIN(GEGL_CL_NTEX, i->rois - i->roi_no);

  /* then we iterate all */
  for (no=0; no<i->iterators;no++)
    {
      for (j = 0; j < i->n; j++)
        {
          GeglRectangle r = {i->rect[no].x + i->roi_all[i->roi_no+j].x - i->area[no][0],
                             i->rect[no].y + i->roi_all[i->roi_no+j].y - i->area[no][2],
                             i->roi_all[i->roi_no+j].width             + i->area[no][0] + i->area[no][1],
                             i->roi_all[i->roi_no+j].height            + i->area[no][2] + i->area[no][3]};
          i->roi [no][j] = r;
          i->size[no][j] = r.width * r.height;
        }

      if (i->flags[no] == GEGL_CL_BUFFER_READ)
        {
          for (j=0; j < i->n; j++)
            {
              gpointer data;

              /* un-tile */
              switch (i->conv[no])
                {
                  case GEGL_CL_COLOR_NOT_SUPPORTED:

                    {
                    gegl_buffer_cl_cache_flush (i->buffer[no], &i->roi[no][j]);

                    g_assert (i->tex_op[no][j] == NULL);
                    i->tex_op[no][j] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                            CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY,
                                                            i->size[no][j] * i->op_cl_format_size [no],
                                                            NULL, &cl_err);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    /* pre-pinned memory */
                    data = gegl_clEnqueueMapBuffer(gegl_cl_get_command_queue(), i->tex_op[no][j], CL_TRUE,
                                                   CL_MAP_WRITE,
                                                   0, i->size[no][j] * i->op_cl_format_size [no],
                                                   0, NULL, NULL, &cl_err);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    /* color conversion using BABL */
                    gegl_buffer_get (i->buffer[no], &i->roi[no][j], 1.0, i->format[no], data, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

                    cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), i->tex_op[no][j], data,
                                                               0, NULL, NULL);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    i->tex[no][j] = i->tex_op[no][j];

                    break;
                    }

                  case GEGL_CL_COLOR_EQUAL:

                    {
                    i->tex_buf[no][j] = gegl_buffer_cl_cache_get (i->buffer[no], &i->roi[no][j]);

                    if (i->tex_buf[no][j])
                      i->tex_buf_from_cache [no][j] = TRUE; /* don't free texture from cache */
                    else
                      {
                        gegl_buffer_cl_cache_flush (i->buffer[no], &i->roi[no][j]);

                        g_assert (i->tex_buf[no][j] == NULL);
                        i->tex_buf[no][j] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                                 CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY,
                                                                 i->size[no][j] * i->buf_cl_format_size [no],
                                                                 NULL, &cl_err);
                        if (cl_err != CL_SUCCESS) CL_ERROR;

                        /* pre-pinned memory */
                        data = gegl_clEnqueueMapBuffer(gegl_cl_get_command_queue(), i->tex_buf[no][j], CL_TRUE,
                                                       CL_MAP_WRITE,
                                                       0, i->size[no][j] * i->buf_cl_format_size [no],
                                                       0, NULL, NULL, &cl_err);
                        if (cl_err != CL_SUCCESS) CL_ERROR;

                        /* color conversion will be performed in the GPU later */
                        gegl_buffer_get (i->buffer[no], &i->roi[no][j], 1.0, i->buffer[no]->soft_format, data, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

                        cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), i->tex_buf[no][j], data,
                                                               0, NULL, NULL);
                        if (cl_err != CL_SUCCESS) CL_ERROR;
                      }

                    i->tex[no][j] = i->tex_buf[no][j];

                    break;
                    }

                  case GEGL_CL_COLOR_CONVERT:

                    {
                    i->tex_buf[no][j] = gegl_buffer_cl_cache_get (i->buffer[no], &i->roi[no][j]);

                    if (i->tex_buf[no][j])
                      i->tex_buf_from_cache [no][j] = TRUE; /* don't free texture from cache */
                    else
                      {
                        gegl_buffer_cl_cache_flush (i->buffer[no], &i->roi[no][j]);

                        g_assert (i->tex_buf[no][j] == NULL);
                        i->tex_buf[no][j] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                                 CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY,
                                                                 i->size[no][j] * i->buf_cl_format_size [no],
                                                                 NULL, &cl_err);
                        if (cl_err != CL_SUCCESS) CL_ERROR;

                        /* pre-pinned memory */
                        data = gegl_clEnqueueMapBuffer(gegl_cl_get_command_queue(), i->tex_buf[no][j], CL_TRUE,
                                                       CL_MAP_WRITE,
                                                       0, i->size[no][j] * i->buf_cl_format_size [no],
                                                       0, NULL, NULL, &cl_err);
                        if (cl_err != CL_SUCCESS) CL_ERROR;

                        /* color conversion will be performed in the GPU later */
                        gegl_buffer_get (i->buffer[no], &i->roi[no][j], 1.0, i->buffer[no]->soft_format, data, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

                        cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), i->tex_buf[no][j], data,
                                                               0, NULL, NULL);
                        if (cl_err != CL_SUCCESS) CL_ERROR;
                      }

                    g_assert (i->tex_op[no][j] == NULL);
                    i->tex_op[no][j] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                            CL_MEM_READ_WRITE,
                                                            i->size[no][j] * i->op_cl_format_size [no],
                                                            NULL, &cl_err);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    /* color conversion in the GPU (input) */
                    g_assert (i->tex_buf[no][j] && i->tex_op[no][j]);
                    cl_err = gegl_cl_color_conv (i->tex_buf[no][j], i->tex_op[no][j], i->size[no][j],
                                                 i->buffer[no]->soft_format, i->format[no]);
                    if (cl_err == FALSE) CL_ERROR;

                    i->tex[no][j] = i->tex_op[no][j];

                    break;
                    }
                }
            }

          /* Wait Processing */
          cl_err = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
          if (cl_err != CL_SUCCESS) CL_ERROR;
        }
      else if (i->flags[no] == GEGL_CL_BUFFER_WRITE)
        {
          for (j=0; j < i->n; j++)
            {
              switch (i->conv[no])
                {
                  case GEGL_CL_COLOR_NOT_SUPPORTED:

                  {
                  g_assert (i->tex_op[no][j] == NULL);
                  i->tex_op[no][j] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                          CL_MEM_ALLOC_HOST_PTR | CL_MEM_WRITE_ONLY,
                                                          i->size[no][j] * i->op_cl_format_size [no],
                                                          NULL, &cl_err);
                  if (cl_err != CL_SUCCESS) CL_ERROR;

                  i->tex[no][j] = i->tex_op[no][j];

                  break;
                  }

                  case GEGL_CL_COLOR_EQUAL:

                  {
                  g_assert (i->tex_buf[no][j] == NULL);
                  i->tex_buf[no][j] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                           CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE, /* cache */
                                                           i->size[no][j] * i->buf_cl_format_size [no],
                                                           NULL, &cl_err);
                  if (cl_err != CL_SUCCESS) CL_ERROR;

                  i->tex[no][j] = i->tex_buf[no][j];

                  break;
                  }

                  case GEGL_CL_COLOR_CONVERT:

                  {
                  g_assert (i->tex_buf[no][j] == NULL);
                  i->tex_buf[no][j] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                           CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE, /* cache */
                                                           i->size[no][j] * i->buf_cl_format_size [no],
                                                           NULL, &cl_err);
                  if (cl_err != CL_SUCCESS) CL_ERROR;

                  g_assert (i->tex_op[no][j] == NULL);
                  i->tex_op[no][j] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                          CL_MEM_READ_WRITE,
                                                          i->size[no][j] * i->op_cl_format_size [no],
                                                          NULL, &cl_err);
                  if (cl_err != CL_SUCCESS) CL_ERROR;

                  i->tex[no][j] = i->tex_op[no][j];

                  break;
                  }
               }
            }
        }
      else if (i->flags[no] == GEGL_CL_BUFFER_AUX)
        {
          for (j=0; j < i->n; j++)
            {
              g_assert (i->tex_op[no][j] == NULL);
              i->tex_op[no][j] = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                      CL_MEM_READ_WRITE,
                                                      i->size[no][j] * i->op_cl_format_size [no],
                                                      NULL, &cl_err);
              if (cl_err != CL_SUCCESS) CL_ERROR;

              i->tex[no][j] = i->tex_op[no][j];
            }
        }
    }

  i->roi_no += i->n;

  i->iteration_no++;

  if (result == FALSE)
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
                gegl_buffer_unlock (i->buffer[no]);

              g_object_unref (i->buffer[no]);
            }
        }

      i->is_finished = TRUE;

      g_free (i->roi_all);
      g_slice_free (GeglBufferClIterators, i);
    }

  *err = FALSE;
  return result;

error:

  for (no=0; no<i->iterators;no++)
    for (j=0; j < i->n; j++)
      {
        if (i->tex_buf[no][j]) gegl_clReleaseMemObject (i->tex_buf[no][j]);
        if (i->tex_op [no][j]) gegl_clReleaseMemObject (i->tex_op [no][j]);

        i->tex    [no][j] = NULL;
        i->tex_buf[no][j] = NULL;
        i->tex_op [no][j] = NULL;
      }

  *err = TRUE;
  return FALSE;
}

GeglBufferClIterator *
gegl_buffer_cl_iterator_new (GeglBuffer          *buffer,
                             const GeglRectangle *roi,
                             const Babl          *format,
                             guint                flags,
                             GeglAbyssPolicy      abyss_policy)
{
  GeglBufferClIterator *i = (gpointer)g_slice_new0 (GeglBufferClIterators);
  /* Because the iterator is nulled above, we can forgo explicitly setting
   * i->is_finished to FALSE. */
  gegl_buffer_cl_iterator_add (i, buffer, roi, format, flags, abyss_policy);
  return i;
}

#undef CL_ERROR
