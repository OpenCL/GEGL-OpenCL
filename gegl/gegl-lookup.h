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
 * Copyright 2009 Øyvind Kolås
 */

#ifndef __GEGL_LOOKUP_H__
#define __GEGL_LOOKUP_H__

G_BEGIN_DECLS

typedef     gfloat (* GeglLookupFunction) (gfloat   value,
                                           gpointer data);

#define GEGL_LOOKUP_MAX_ENTRIES   (819200)

typedef struct GeglLookup
{
  GeglLookupFunction function;
  gpointer           data;
  gint               shift;
  guint32            positive_min, positive_max, negative_min, negative_max;
  guint32            bitmask[GEGL_LOOKUP_MAX_ENTRIES/32];
  gfloat             table[];
} GeglLookup;


GeglLookup *gegl_lookup_new_full  (GeglLookupFunction  function,
                                   gpointer            data,
                                   gfloat              start,
                                   gfloat              end,
                                   gfloat              precision);
GeglLookup *gegl_lookup_new       (GeglLookupFunction  function,
                                   gpointer            data);
void        gegl_lookup_free      (GeglLookup         *lookup);


static inline gfloat
gegl_lookup (GeglLookup *lookup,
             gfloat      number)
{
  union
  {
    float   f;
    guint32 i;
  } u;
  guint i;

  u.f = number;
  i = u.i >> lookup->shift;

  if (i > lookup->positive_min &&
      i < lookup->positive_max)
    i = i - lookup->positive_min;
  else if (i > lookup->negative_min &&
           i < lookup->negative_max)
    i = i - lookup->negative_min + (lookup->positive_max - lookup->positive_min);
  else
    return lookup->function (number, lookup->data);

  if (!(lookup->bitmask[i/32] & (1<<(i & 31))))
    {
      lookup->table[i]= lookup->function (number, lookup->data);
      lookup->bitmask[i/32] |= (1<<(i & 31));
    }

  return lookup->table[i];
}

G_END_DECLS

#endif
