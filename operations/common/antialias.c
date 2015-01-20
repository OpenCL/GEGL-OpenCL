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
 * This operation is a port of the GIMP antialias plugin:
 *
 *     Auntie Alias 0.92 --- image filter plug-in
 *     Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *     Copyright (C) 2005 Adam D. Moss (adam@gimp.org)
 *
 * The porting to GEGL was done by Barak Itkin
 *
 *     Copyright 2013 Barak Itkin <lightningismyname@gmail.com>
 */

/* This plugin performs a pseudo-antialiasing effect on hard-edged source
 * material.  It does this by performing a 'clever' edge extrapolation for
 * each pixel which is then resampled back to a single pixel for output.
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

/* This operation has no properties */

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_SOURCE antialias.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *input_format = gegl_operation_get_source_format (operation, "input");
  const Babl *format;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  if (input_format == NULL || babl_format_has_alpha (input_format))
    format = babl_format ("R'G'B'A float");
  else
    format = babl_format ("R'G'B' float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);

  op_area->left   =
  op_area->right  =
  op_area->top    =
  op_area->bottom = 1;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle *region;

  region = gegl_operation_source_get_bounding_box (operation, "input");

  if (region != NULL)
    return *region;
  else
    return *GEGL_RECTANGLE (0, 0, 0, 0);
}

static int
extrapolate9 (const int components,
              gfloat *E0, gfloat *E1, gfloat *E2,
              gfloat *E3, gfloat *E4, gfloat *E5,
              gfloat *E6, gfloat *E7, gfloat *E8,
              gfloat *A,  gfloat *B,  gfloat *C,
              gfloat *D,  gfloat *E,  gfloat *F,
              gfloat *G,  gfloat *H,  gfloat *I)
{
#define PEQ(X,Y)      (0 == memcmp ((X), (Y), components * sizeof(gfloat)))
#define PCPY(DST,SRC) memcpy ((DST), (SRC), components * sizeof(gfloat))

  /* an implementation of the Scale3X edge-extrapolation algorithm */
  if ( (!PEQ(B,H)) && (!PEQ(D,F)) )
    {
      if (PEQ(D,B)) PCPY(E0,D); else PCPY(E0,E);
      if ((PEQ(D,B) && !PEQ(E,C)) || (PEQ(B,F) && !PEQ(E,A)))
        PCPY(E1,B); else PCPY(E1,E);
      if (PEQ(B,F)) PCPY(E2,F); else PCPY(E2,E);
      if ((PEQ(D,B) && !PEQ(E,G)) || (PEQ(D,H) && !PEQ(E,A)))
        PCPY(E3,D); else PCPY(E3,E);
      PCPY(E4,E);
      if ((PEQ(B,F) && !PEQ(E,I)) || (PEQ(H,F) && !PEQ(E,C)))
        PCPY(E5,F); else PCPY(E5,E);
      if (PEQ(D,H)) PCPY(E6,D); else PCPY(E6,E);
      if ((PEQ(D,H) && !PEQ(E,I)) || (PEQ(H,F) && !PEQ(E,G)))
        PCPY(E7,H); else PCPY(E7,E);
      if (PEQ(H,F)) PCPY(E8,F); else PCPY(E8,E);
      return 1;
    }
  else
    {
      return 0;
    }

#undef PEQ
#undef PCPY
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  gint          components;
  gint          col, c;
  gfloat       *rowbefore;
  gfloat       *rowthis;
  gfloat       *rowafter;
  gfloat       *dest;
  gfloat       *ninepix;
  gboolean      has_alpha;
  guint         alpha;
  const Babl   *format = gegl_operation_get_format (operation, "input");

  /* The rectangle of the current row we are working on */
  GeglRectangle rowrect;

  /* The rectangle of the sample we are going to take for the
   * next line (this does include the interpolation distance!) */
  GeglRectangle rownext_bufrect;

  components = babl_format_get_n_components (format);
  has_alpha  = babl_format_has_alpha (format);
  alpha      = components - 1;

  /* The original algorithm that appeared in GIMP did a manual clamping
   * of samples outside the input rectangle, by always allocating a
   * buffer which is 1-pixel wider than necessary, and then filling the
   * edges of the buffer with repetitions of the edges.
   * We don't need this complexity here thanks to the CLAMP abyss policy
   * which is implemented by GEGL. We still allocate buffers which are
   * larger, but we let GEGL fill the edges with the clamped values.
   */

  rowbefore  = g_new (gfloat, (roi->width + 2) * components);
  rowthis    = g_new (gfloat, (roi->width + 2) * components);
  rowafter   = g_new (gfloat, (roi->width + 2) * components);
  dest       = g_new (gfloat, roi->width * components);
  ninepix    = g_new (gfloat, 9 * components);

  gegl_rectangle_set (&rowrect, roi->x, roi->y, roi->width, 1);
  gegl_rectangle_set (&rownext_bufrect, roi->x - 1, roi->y - 1, roi->width + 2, 1);

  gegl_buffer_get (input, &rownext_bufrect, 1.0, format,
                   rowbefore, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
  ++rownext_bufrect.y;

  gegl_buffer_get (input, &rownext_bufrect, 1.0, format,
                   rowthis, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
  ++rownext_bufrect.y;

  gegl_buffer_get (input, &rownext_bufrect, 1.0, format,
                   rowafter, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
  ++rownext_bufrect.y;

  for (rowrect.y = roi->y; rowrect.y < (roi->y + roi->height); ++rowrect.y)
    {
      gfloat *tmp;

      /* this macro returns the current pixel if it has some opacity. Otherwise
       * it returns the center pixel of the current 3x3 area. */
#define USE_IF_ALPHA(p) (((!has_alpha) || *((p)+alpha)) ? (p) : &rowthis[(col+1) * components])

      for (col = 0; col < roi->width; ++col)
        {
          /* do 9x extrapolation pass */
          if (((!has_alpha) || (rowthis[(col + 1) * components + alpha] > 0)) &&
              extrapolate9 (components,
                            &ninepix[0 * components],
                            &ninepix[1 * components],
                            &ninepix[2 * components],
                            &ninepix[3 * components],
                            &ninepix[4 * components],
                            &ninepix[5 * components],
                            &ninepix[6 * components],
                            &ninepix[7 * components],
                            &ninepix[8 * components],
                            USE_IF_ALPHA (&rowbefore[(col + 0) * components]),
                            USE_IF_ALPHA (&rowbefore[(col + 1) * components]),
                            USE_IF_ALPHA (&rowbefore[(col + 2) * components]),
                            USE_IF_ALPHA (&rowthis  [(col + 0) * components]),
                                          &rowthis  [(col + 1) * components],
                            USE_IF_ALPHA (&rowthis  [(col + 2) * components]),
                            USE_IF_ALPHA (&rowafter [(col + 0) * components]),
                            USE_IF_ALPHA (&rowafter [(col + 1) * components]),
                            USE_IF_ALPHA (&rowafter [(col + 2) * components])
                            ))
            {
              /* subsample results and put into dest */
              for (c = 0; c < components; ++c)
                {
#define NINEPIX(index, c) ninepix[(index) * components + (c)]
                  dest[(col * components) + c] =
                    (3 * NINEPIX(0, c) + 5 * NINEPIX(1, c) + 3 * NINEPIX(2, c) +
                     5 * NINEPIX(3, c) + 6 * NINEPIX(4, c) + 5 * NINEPIX(5, c) +
                     3 * NINEPIX(6, c) + 5 * NINEPIX(7, c) + 3 * NINEPIX(8, c)
                    /* The GIMP implementation added 19 (out of 255) here before
                     * normalizing (dividing by 38), which is equivalent to a
                     * call to "ceil". We don't need this since we are working
                     * with floating point numbers... */
                    ) / 38;
#undef NINEPIX
                }
            }
          else
            {
              memcpy (&dest[col * components], &rowthis[(col + 1) * components], components * sizeof(gfloat));
            }
        }

#undef USE_IF_ALPHA

      /* write result row to dest */
      gegl_buffer_set (output, &rowrect, 0, format, &dest[0], GEGL_AUTO_ROWSTRIDE);

      /* rotate pointers */
      tmp       = rowbefore;
      rowbefore = rowthis;
      rowthis   = rowafter;
      rowafter  = tmp;

      /* populate new after-row */
      gegl_buffer_get (input, &rownext_bufrect, 1.0, format,
                       rowafter, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
      ++rownext_bufrect.y;
    }

  g_free (rowbefore);
  g_free (rowthis);
  g_free (rowafter);
  g_free (dest);
  g_free (ninepix);

  return  TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class          = GEGL_OPERATION_CLASS (klass);
  filter_class             = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  filter_class->process             = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:antialias",
    "title",       _("Scale3X Antialiasing"),
    "categories",  "enhance",
    "license",     "GPL3+",
    "description", _("Antialias using the Scale3X edge-extrapolation algorithm"),
    NULL);
}

#endif

