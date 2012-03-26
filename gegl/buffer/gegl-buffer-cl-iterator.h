#ifndef __GEGL_BUFFER_CL_ITERATOR_H__
#define __GEGL_BUFFER_CL_ITERATOR_H__

#include "gegl-buffer.h"
#include "opencl/gegl-cl.h"

#define GEGL_CL_NTEX 16
#define GEGL_CL_BUFFER_MAX_ITERATORS 6

enum
{
  GEGL_CL_BUFFER_READ   = 1,
  GEGL_CL_BUFFER_WRITE  = 2,
  GEGL_CL_BUFFER_AUX    = 3
};

typedef struct GeglBufferClIterator
{
  gint          n;
  size_t        size [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX];  /* length of current data in pixels */
  cl_mem        tex  [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX];
  GeglRectangle roi  [GEGL_CL_BUFFER_MAX_ITERATORS][GEGL_CL_NTEX];
} GeglBufferClIterator;

gint gegl_buffer_cl_iterator_add (GeglBufferClIterator  *iterator,
                                  GeglBuffer            *buffer,
                                  const GeglRectangle   *roi,
                                  const Babl            *format,
                                  guint                  flags,
                                  GeglAbyssPolicy        abyss_policy);

gint gegl_buffer_cl_iterator_add_2 (GeglBufferClIterator  *iterator,
                                    GeglBuffer            *buffer,
                                    const GeglRectangle   *roi,
                                    const Babl            *format,
                                    guint                  flags,
                                    gint                   left,
                                    gint                   right,
                                    gint                   top,
                                    gint                   bottom,
                                    GeglAbyssPolicy        abyss_policy);

gboolean gegl_buffer_cl_iterator_next (GeglBufferClIterator *iterator, gboolean *err);

GeglBufferClIterator *gegl_buffer_cl_iterator_new (GeglBuffer          *buffer,
                                                   const GeglRectangle *roi,
                                                   const Babl          *format,
                                                   guint                flags,
                                                   GeglAbyssPolicy      abyss_policy);
#endif
