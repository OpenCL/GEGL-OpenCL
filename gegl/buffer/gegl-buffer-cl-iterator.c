#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#include "gegl.h"

#include "gegl-buffer-types.h"
#include "gegl-buffer-cl-iterator.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"
#include "gegl-utils.h"

#define CL_ERROR {g_printf("[OpenCL] Error in %s:%d@%s - %s\n", __FILE__, __LINE__, __func__, gegl_cl_errstring(cl_err)); goto error;}

typedef struct GeglBufferClIterators
{
  /* current region of interest */
  gint          n;
  size_t        size [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX][2];  /* length of current data in pixels */
  cl_mem        tex  [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX];
  GeglRectangle roi  [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX];

  /* the following is private: */
  cl_mem        tex_buf [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX];
  cl_mem        tex_op  [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX];

  gint           iterators;
  gint           iteration_no;
  gboolean       is_finished;

  guint          flags          [GEGL_CL_BUFFER_MAX_ITERATORS];

  GeglRectangle  rect           [GEGL_CL_BUFFER_MAX_ITERATORS]; /* the region we iterate on. They can be
                                                                   different from each other, but width
                                                                   and height are the same */

  const Babl    *format         [GEGL_CL_BUFFER_MAX_ITERATORS]; /* The format required for the data */
  GeglBuffer    *buffer         [GEGL_CL_BUFFER_MAX_ITERATORS];

  /* buffer->format */
  cl_image_format buf_cl_format [GEGL_CL_BUFFER_MAX_ITERATORS];
  /* format */
  cl_image_format op_cl_format  [GEGL_CL_BUFFER_MAX_ITERATORS];

  gegl_cl_color_op conv         [GEGL_CL_BUFFER_MAX_ITERATORS];

  /* total iteration */
  gint           rois;
  gint           roi_no;
  GeglRectangle *roi_all;

} GeglBufferClIterators;

gint
gegl_buffer_cl_iterator_add (GeglBufferClIterator  *iterator,
                             GeglBuffer            *buffer,
                             const GeglRectangle   *result,
                             const Babl            *format,
                             guint                  flags)
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

  i->buffer[self]= g_object_ref (buffer);

  if (format)
    i->format[self]=format;
  else
    i->format[self]=buffer->format;
  i->flags[self]=flags;

  if (flags == GEGL_CL_BUFFER_WRITE)
    i->conv[self] = gegl_cl_color_supported (format, buffer->format);
  else
    i->conv[self] = gegl_cl_color_supported (buffer->format, format);

  gegl_cl_color_babl (buffer->format, &i->buf_cl_format[self], NULL);
  gegl_cl_color_babl (format,         &i->op_cl_format [self], NULL);

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
      for (y=result->y; y < result->y + result->height; y += cl_state.max_image_height)
        for (x=result->x; x < result->x + result->width;  x += cl_state.max_image_width)
          i->rois++;

      i->roi_no = 0;
      i->roi_all = g_new0 (GeglRectangle, i->rois);

      j = 0;
      for (y=0; y < result->height; y += cl_state.max_image_height)
        for (x=0; x < result->width;  x += cl_state.max_image_width)
          {
            GeglRectangle r = {x, y,
                               MIN(cl_state.max_image_width,  result->width  - x),
                               MIN(cl_state.max_image_height, result->height - y)};
            i->roi_all[j] = r;
            j++;
          }
    }

  return self;
}

gboolean
gegl_buffer_cl_iterator_next (GeglBufferClIterator *iterator, gboolean *err)
{
  GeglBufferClIterators *i = (gpointer)iterator;
  gboolean result = FALSE;
  gint no, j;
  cl_int cl_err = 0;

  const size_t origin_zero[3] = {0, 0, 0};

  if (i->is_finished)
    g_error ("%s called on finished buffer iterator", G_STRFUNC);
  if (i->iteration_no == 0)
    {
      for (no=0; no<i->iterators;no++)
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
                                                 i->format[no], i->buffer[no]->format);
                    if (cl_err == FALSE) CL_ERROR;
                  }

              /* Wait Processing */
              cl_err = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
              if (cl_err != CL_SUCCESS) CL_ERROR;

              /* GPU -> CPU */
              for (j=0; j < i->n; j++)
                {
                  gpointer data;
                  size_t pitch;
                  const size_t region[3] = {i->roi[no][j].width, i->roi[no][j].height, 1};

                  /* tile-ize */
                  if (i->conv[no] == GEGL_CL_COLOR_NOT_SUPPORTED)
                    {
                      data = gegl_clEnqueueMapImage(gegl_cl_get_command_queue(), i->tex_op[no][j], CL_TRUE,
                                                    CL_MAP_READ,
                                                    origin_zero, region, &pitch, NULL,
                                                    0, NULL, NULL, &cl_err);
                      if (cl_err != CL_SUCCESS) CL_ERROR;

                      /* color conversion using BABL */
                      gegl_buffer_set (i->buffer[no], &i->roi[no][j], i->format[no], data, pitch);

                      cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), i->tex_op[no][j], data,
                                                            0, NULL, NULL);
                      if (cl_err != CL_SUCCESS) CL_ERROR;
                    }
                  else
                    {
                      data = gegl_clEnqueueMapImage(gegl_cl_get_command_queue(), i->tex_buf[no][j], CL_TRUE,
                                                    CL_MAP_READ,
                                                    origin_zero, region, &pitch, NULL,
                                                    0, NULL, NULL, &cl_err);
                      if (cl_err != CL_SUCCESS) CL_ERROR;

                      /* color conversion has already been performed in the GPU */
                      gegl_buffer_set (i->buffer[no], &i->roi[no][j], i->buffer[no]->format, data, pitch);

                      cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), i->tex_buf[no][j], data,
                                                            0, NULL, NULL);
                      if (cl_err != CL_SUCCESS) CL_ERROR;
                    }
                }
            }
        }

      /* Run! */
      cl_err = gegl_clFinish(gegl_cl_get_command_queue());
      if (cl_err != CL_SUCCESS) CL_ERROR;

      for (no=0; no < i->iterators; no++)
        for (j=0; j < i->n; j++)
          {
            if (i->tex_buf[no][j]) gegl_clReleaseMemObject (i->tex_buf[no][j]);
            if (i->tex_op [no][j]) gegl_clReleaseMemObject (i->tex_op [no][j]);

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
          GeglRectangle r = {i->rect[no].x + i->roi_all[i->roi_no+j].x,
                             i->rect[no].y + i->roi_all[i->roi_no+j].y,
                             i->roi_all[i->roi_no+j].width,
                             i->roi_all[i->roi_no+j].height};
          i->roi [no][j] = r;

          i->size[no][j][0] = r.width;
          i->size[no][j][1] = r.height;
        }

      if (i->flags[no] == GEGL_CL_BUFFER_READ)
        {
          for (j=0; j < i->n; j++)
            {
              gpointer data;
              size_t pitch;
              const size_t region[3] = {i->roi[no][j].width, i->roi[no][j].height, 1};

              /* un-tile */
              switch (i->conv[no])
                {
                  case GEGL_CL_COLOR_NOT_SUPPORTED:

                    {
                    g_assert (i->tex_op[no][j] == NULL);
                    i->tex_op[no][j] = gegl_clCreateImage2D (gegl_cl_get_context (),
                                                          CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE,
                                                          &i->op_cl_format [no],
                                                          i->roi[no][j].width,
                                                          i->roi[no][j].height,
                                                          0, NULL, &cl_err);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    /* pre-pinned memory */
                    data = gegl_clEnqueueMapImage(gegl_cl_get_command_queue(), i->tex_op[no][j], CL_TRUE,
                                                  CL_MAP_WRITE,
                                                  origin_zero, region, &pitch, NULL,
                                                  0, NULL, NULL, &cl_err);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    /* color conversion using BABL */
                    gegl_buffer_get (i->buffer[no], 1.0, &i->roi[no][j], i->format[no], data, pitch);

                    i->tex[no][j] = i->tex_op[no][j];

                    break;
                    }

                  case GEGL_CL_COLOR_EQUAL:

                    {
                    g_assert (i->tex_buf[no][j] == NULL);
                    i->tex_buf[no][j] = gegl_clCreateImage2D (gegl_cl_get_context (),
                                                              CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE,
                                                              &i->buf_cl_format [no],
                                                              i->roi[no][j].width,
                                                              i->roi[no][j].height,
                                                              0, NULL, &cl_err);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    /* pre-pinned memory */
                    data = gegl_clEnqueueMapImage(gegl_cl_get_command_queue(), i->tex_buf[no][j], CL_TRUE,
                                                  CL_MAP_WRITE,
                                                  origin_zero, region, &pitch, NULL,
                                                  0, NULL, NULL, &cl_err);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    /* color conversion will be performed in the GPU later */
                    gegl_buffer_get (i->buffer[no], 1.0, &i->roi[no][j], i->buffer[no]->format, data, pitch);

                    cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), i->tex_buf[no][j], data,
                                                           0, NULL, NULL);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    i->tex[no][j] = i->tex_buf[no][j];

                    break;
                    }

                  case GEGL_CL_COLOR_CONVERT:

                    {
                    g_assert (i->tex_buf[no][j] == NULL);
                    i->tex_buf[no][j] = gegl_clCreateImage2D (gegl_cl_get_context (),
                                                              CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE,
                                                              &i->buf_cl_format [no],
                                                              i->roi[no][j].width,
                                                              i->roi[no][j].height,
                                                              0, NULL, &cl_err);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    g_assert (i->tex_op[no][j] == NULL);
                    i->tex_op[no][j] = gegl_clCreateImage2D (gegl_cl_get_context (),
                                                             CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE,
                                                             &i->op_cl_format [no],
                                                             i->roi[no][j].width,
                                                             i->roi[no][j].height,
                                                             0, NULL, &cl_err);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    /* pre-pinned memory */
                    data = gegl_clEnqueueMapImage(gegl_cl_get_command_queue(), i->tex_buf[no][j], CL_TRUE,
                                                  CL_MAP_WRITE,
                                                  origin_zero, region, &pitch, NULL,
                                                  0, NULL, NULL, &cl_err);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    /* color conversion will be performed in the GPU later */
                    gegl_buffer_get (i->buffer[no], 1.0, &i->roi[no][j], i->buffer[no]->format, data, pitch);

                    cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), i->tex_buf[no][j], data,
                                                           0, NULL, NULL);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    /* color conversion in the GPU (input) */
                    g_assert (i->tex_buf[no][j] && i->tex_op[no][j]);
                    cl_err = gegl_cl_color_conv (i->tex_buf[no][j], i->tex_op[no][j], i->size[no][j],
                                                 i->buffer[no]->format, i->format[no]);
                    if (cl_err == FALSE) CL_ERROR;

                    i->tex[no][j] = i->tex_buf[no][j];

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
                  i->tex_op[no][j] = gegl_clCreateImage2D (gegl_cl_get_context (),
                                                           CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE,
                                                           &i->op_cl_format [no],
                                                           i->roi[no][j].width,
                                                           i->roi[no][j].height,
                                                           0, NULL, &cl_err);
                  if (cl_err != CL_SUCCESS) CL_ERROR;

                  i->tex[no][j] = i->tex_op[no][j];

                  break;
                  }

                  case GEGL_CL_COLOR_EQUAL:

                  {
                  g_assert (i->tex_buf[no][j] == NULL);
                  i->tex_buf[no][j] = gegl_clCreateImage2D (gegl_cl_get_context (),
                                                            CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE,
                                                            &i->buf_cl_format [no],
                                                            i->roi[no][j].width,
                                                            i->roi[no][j].height,
                                                            0, NULL, &cl_err);
                  if (cl_err != CL_SUCCESS) CL_ERROR;

                  i->tex[no][j] = i->tex_buf[no][j];

                  break;
                  }

                  case GEGL_CL_COLOR_CONVERT:

                  {
                  g_assert (i->tex_buf[no][j] == NULL);
                  i->tex_buf[no][j] = gegl_clCreateImage2D (gegl_cl_get_context (),
                                                            CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE,
                                                            &i->buf_cl_format [no],
                                                            i->roi[no][j].width,
                                                            i->roi[no][j].height,
                                                            0, NULL, &cl_err);
                  if (cl_err != CL_SUCCESS) CL_ERROR;

                  g_assert (i->tex_op[no][j] == NULL);
                  i->tex_op[no][j] = gegl_clCreateImage2D (gegl_cl_get_context (),
                                                           CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE,
                                                           &i->op_cl_format [no],
                                                           i->roi[no][j].width,
                                                           i->roi[no][j].height,
                                                           0, NULL, &cl_err);
                  if (cl_err != CL_SUCCESS) CL_ERROR;

                  i->tex[no][j] = i->tex_op[no][j];

                  break;
                  }
               }
            }
        }
    }

  i->roi_no += i->n;

  i->iteration_no++;

  if (result == FALSE)
    {
      for (no=0; no<i->iterators;no++)
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
        }

      for (no=0; no<i->iterators;no++)
        {
          g_object_unref (i->buffer[no]);
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
                             guint                flags)
{
  GeglBufferClIterator *i = (gpointer)g_slice_new0 (GeglBufferClIterators);
  /* Because the iterator is nulled above, we can forgo explicitly setting
   * i->is_finished to FALSE. */
  gegl_buffer_cl_iterator_add (i, buffer, roi, format, flags);
  return i;
}

#undef CL_ERROR
