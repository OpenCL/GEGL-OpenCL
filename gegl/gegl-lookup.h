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

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * NOTE! tweaking these values will change ABI for operations using GeglLookup
 */
#define GEGL_LOOKUP_NEGATIVE     0 /* set to 1 to also cache range -1.0..-0.0 */
#define GEGL_LOOKUP_SHIFT_RIGHT  14 /* Number of bits to shift integer right
                                       before computing lookup bits

      value  tablesize  average error 
        16   2554       0.001298
        15   5108       0.000649
        14   10216      0.000324
 */

#if GEGL_LOOKUP_SHIFT_RIGHT == 16

#define GEGL_LOOKUP_START_POSITIVE   13702
#define GEGL_LOOKUP_END_POSITIVE     16256
#define GEGL_LOOKUP_START_NEGATIVE   46470
#define GEGL_LOOKUP_END_NEGATIVE     49024

#elif GEGL_LOOKUP_SHIFT_RIGHT == 15

#define GEGL_LOOKUP_START_POSITIVE   27404
#define GEGL_LOOKUP_END_POSITIVE     32512
#define GEGL_LOOKUP_START_NEGATIVE   92940
#define GEGL_LOOKUP_END_NEGATIVE     98048

#elif GEGL_LOOKUP_SHIFT_RIGHT == 14

#define GEGL_LOOKUP_START_POSITIVE   54808
#define GEGL_LOOKUP_END_POSITIVE     65024
#define GEGL_LOOKUP_START_NEGATIVE   185880
#define GEGL_LOOKUP_END_NEGATIVE     196096

#endif

#define GEGL_LOOKUP_SUM_POSITIVE    (GEGL_LOOKUP_END_POSITIVE-GEGL_LOOKUP_START_POSITIVE)
#define GEGL_LOOKUP_SUM_NEGATIVE    (GEGL_LOOKUP_END_NEGATIVE-GEGL_LOOKUP_START_NEGATIVE)

#if GEGL_LOOKUP_NEGATIVE
#define GEGL_LOOKUP_SUM             (GEGL_LOOKUP_SUM_POSITIVE+GEGL_LOOKUP_SUM_NEGATIVE)
#else
#define GEGL_LOOKUP_SUM             (GEGL_LOOKUP_SUM_NEGATIVE)
#endif

typedef     gfloat (GeglLookupFunction) (float, gpointer data);

typedef struct GeglLookup
{
  GeglLookupFunction *function; 
  gpointer            data;
  guint32             bitmask[(GEGL_LOOKUP_SUM+31)/32];
  gfloat              table[GEGL_LOOKUP_SUM];
} GeglLookup;

GeglLookup *gegl_lookup_new  (GeglLookupFunction   *function,
                              gpointer              data);
void        gegl_lookup_free (GeglLookup           *lookup);

static inline gfloat
gegl_lookup (GeglLookup *lookup,
             gfloat      number)
{
  union
  {
    float   f;
    guint32 i;
  } u;
  guint index;

  u.f = number;
  index = u.i >> GEGL_LOOKUP_SHIFT_RIGHT;
  if (index > GEGL_LOOKUP_START_POSITIVE && index < GEGL_LOOKUP_END_POSITIVE)
    index = index-GEGL_LOOKUP_START_POSITIVE;
#if GEGL_LOOKUP_NEGATIVE
  else if (index > GEGL_LOOKUP_START_NEGATIVE && index < GEGL_LOOKUP_END_NEGATIVE)
    index = index-GEGL_LOOKUP_START_NEGATIVE+GEGL_LOOKUP_SUM_POSITIVE;
#endif
  else
    return lookup->function (number, lookup->data);

  if (!(lookup->bitmask[index/32] & (1<<(index & 31))))
    {
      lookup->table[index]= lookup->function (number, lookup->data);
      lookup->bitmask[index/32] |= (1<<(index & 31));
    } 
  return lookup->table[index];
}

G_END_DECLS

#endif
