/* rgegl - a ruby binding for GEGL
 *
 * rgegl is free software; you can redistribute it and/or
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
 * License along with rgegl; if not, see <http://www.gnu.org/licenses/>.
 *
 *
 *
 * 2007 © Øyvind Kolås.
 */

#ifndef _RBGEGL_H
#define _RBGEGL_H 1
#include "ruby.h"
#include <gegl.h>
#include "rbgtk.h"
#include "rbgobject.h"
/*#include "rbgeglversion.h"*/

extern void Init_gegl(void);
extern void Init_gegl_buffer(VALUE);
extern void Init_gegl_color(VALUE);
extern void Init_gegl_node(VALUE);
extern void Init_gegl_processor(VALUE);
extern void Init_gegl_rectangle(VALUE);

#endif /* _RBGEGL_H */
