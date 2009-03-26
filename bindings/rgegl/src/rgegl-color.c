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

#include "config.h"
#include "rgegl.h"

#define _SELF(self) GEGL_COLOR(RVAL2GOBJ(self))

static VALUE
ccolor_initialize(self, str)
    VALUE self, str;
{
    GeglColor *color;
    color = gegl_color_new (RVAL2CSTR (str));
    G_INITIALIZE(self, color);
    return Qnil;
}

static VALUE
ccolor_get_rgba(self)
    VALUE self;
{
    float r, g, b, a;
    gegl_color_get_rgba (_SELF(self),
                         &r,
                         &g,
                         &b,
                         &a);
    return rb_ary_new3(4, rb_float_new(r), rb_float_new(g),
                       rb_float_new(b), rb_float_new(a));
}

static VALUE
ccolor_set_rgba(self, red, green, blue, alpha)
    VALUE self, red, green, blue, alpha;
{
    gdouble r, g, b, a;
    r = NUM2DBL(red);
    g = NUM2DBL(green);
    b = NUM2DBL(blue);
    a = NUM2DBL(alpha);
    gegl_color_set_rgba (_SELF(self),
                         r,
                         g,
                         b,
                         a);
    return self;
}


void
Init_gegl_color(mGnome)
    VALUE mGnome;
{
    VALUE gnoGeglColor = G_DEF_CLASS(GEGL_TYPE_COLOR, "Color", mGnome);

    rb_define_method(gnoGeglColor, "initialize", ccolor_initialize, 1);
    rb_define_method(gnoGeglColor, "get_rgba", ccolor_get_rgba, 0);
    rb_define_method(gnoGeglColor, "set_rgba", ccolor_set_rgba, 4);
}
