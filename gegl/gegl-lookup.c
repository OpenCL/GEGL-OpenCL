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
 * Copyright 2009 Øyvind Kolås.
 */

#include "config.h"

#include <glib-object.h>

#include "gegl.h"
#include "gegl-lookup.h"

GeglLookup *
gegl_lookup_new_full (GeglLookupFunction function,
                      gpointer           data,
                      gfloat             start,
                      gfloat             end,
                      gfloat             precision)
{
  GeglLookup *lookup;
  union
  {
    float   f;
    guint32 i;
  } u;
  gint positive_min, positive_max, negative_min, negative_max;
  gint shift;

  /* normalize input parameters */
  if (start > end)
    { /* swap */
      u.f = start;
      start = end;
      end = u.f;
    }

       if (precision <= 0.000005) shift =  0; /* checked for later */
  else if (precision <= 0.000010) shift =  8;
  else if (precision <= 0.000020) shift =  9;
  else if (precision <= 0.000040) shift = 10;
  else if (precision <= 0.000081) shift = 11;
  else if (precision <= 0.000161) shift = 12;
  else if (precision <= 0.000324) shift = 14;
  else if (precision <= 0.000649) shift = 15;
  else shift = 16; /* a bit better than 8bit sRGB quality */

  /* Adjust slightly away from 0.0, saving many entries close to 0, this
   * causes lookups very close to zero to be passed directly to the
   * function instead.
   */
  if (start == 0.0)
    start = precision;
  if (end == 0.0)
    end = -precision;

  /* Compute start and */

  if (start < 0.0 || end < 0.0)
    {
      if (end < 0.0)
        {
          u.f = start;
          positive_max = u.i >> shift;
          u.f = end;
          positive_min = u.i >> shift;
          negative_min = positive_max;
          negative_max = positive_max;
        }
      else
        {
          u.f = 0 - precision;
          positive_min = u.i >> shift;
          u.f = start;
          positive_max = u.i >> shift;

          u.f = 0 + precision;
          negative_min = u.i >> shift;
          u.f = end;
          negative_max = u.i >> shift;
        }
    }
  else
    {
      u.f = start;
      positive_min = u.i >> shift;
      u.f = end;
      positive_max = u.i >> shift;
      negative_min = positive_max;
      negative_max = positive_max;
    }

  if (shift == 0) /* short circuit, do not use ranges */
    {
      positive_min = positive_max = negative_min = negative_max = 0;
    }

  if ((positive_max-positive_min) + (negative_max-negative_min) > GEGL_LOOKUP_MAX_ENTRIES)
    {
      /* Reduce the size of the cache tables to fit within the bittable
       * budget (the maximum allocation is around 2.18mb of memory
       */

      gint diff = (positive_max-positive_min) + (negative_max-negative_min) - GEGL_LOOKUP_MAX_ENTRIES;

      if (negative_max - negative_min > 0)
        {
          if (negative_max - negative_min >= diff)
            {
              negative_max -= diff;
              diff = 0;
            }
          else
            {
              diff -= negative_max - negative_min;
              negative_max = negative_min;
            }
        }
      if (diff)
        positive_max-=diff;
    }

  lookup = g_malloc0 (sizeof (GeglLookup) + sizeof (gfloat) *
                                                  ((positive_max-positive_min)+
                                                   (negative_max-negative_min)));

  lookup->positive_min = positive_min;
  lookup->positive_max = positive_max;
  lookup->negative_min = negative_min;
  lookup->negative_max = negative_max;
  lookup->shift = shift;
  lookup->function = function;
  lookup->data = data;

  return lookup;
}

GeglLookup *
gegl_lookup_new (GeglLookupFunction function,
                 gpointer           data)
{
  return gegl_lookup_new_full (function, data, 0, 1.0, 0.000010);
}

void
gegl_lookup_free (GeglLookup *lookup)
{
  g_free (lookup);
}
