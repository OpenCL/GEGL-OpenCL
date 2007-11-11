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
#include "rgegl.h"

void
Init_gegl(void)
{
    VALUE mGegl = rb_define_module("Gegl");

    Init_gegl_color(mGegl);
    Init_gegl_node(mGegl);
    Init_gegl_processor(mGegl);
    Init_gegl_rectangle(mGegl);
}
