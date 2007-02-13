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
