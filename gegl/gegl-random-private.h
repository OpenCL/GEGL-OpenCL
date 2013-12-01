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
 * Copyright 2012, 2013 Øyvind Kolås
 */

#ifndef __GEGL_RANDOM_PRIV_H__
#define __GEGL_RANDOM_PRIV_H__

#define RANDOM_DATA_SIZE (15101 * 3)

struct _GeglRandom
{
  guint16 prime0;
  guint16 prime1;
  guint16 prime2;
};

guint32*
gegl_random_get_data (void);

void
gegl_random_cleanup (void);

#endif /* __GEGL_RANDOM_PRIV_H__ */
