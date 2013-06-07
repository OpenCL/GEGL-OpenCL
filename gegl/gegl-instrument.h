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
 * Copyright 2006 Øyvind Kolås
 */
#ifndef GEGL_INSTRUMENT_H
#define GEGL_INSTRUMENT_H

extern gboolean gegl_instrument_enabled;

/* return number of usecs since gegl was initialized */
long gegl_ticks               (void);

/* start tracking times with gegl_instrument */
void gegl_instrument_enable   (void);

#define GEGL_INSTRUMENT_START() \
  { long _gegl_instrument_ticks = 0; \
    if (gegl_instrument_enabled) { _gegl_instrument_ticks = gegl_ticks (); }

#define GEGL_INSTRUMENT_END(parent, scale) \
    if (gegl_instrument_enabled) { \
      real_gegl_instrument (parent, scale, gegl_ticks () - _gegl_instrument_ticks); \
                                 } \
  }

/* store a timing instrumentation (parent is expected to exist,
 * and to keep it's own record of the time-slice reported) */
#define gegl_instrument(parent, scale, usecs) \
  { if (gegl_instrument_enabled) { \
real_gegl_instrument (parent, scale, usecs); \
                                 } }

void real_gegl_instrument     (const gchar *parent,
                               const gchar *scale,
                               long         usecs);

/* create a utf8 string with bar charts for where time disappears
 * during a gegl-run
 */
gchar * gegl_instrument_utf8 (void);

#endif
