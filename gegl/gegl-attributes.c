#include "gegl-attributes.h"
#include "gegl-color-model.h"
#include "gegl-utils.h"

GeglAttributes* 
gegl_attributes_new() 
{
  GeglAttributes * attrs = g_new0(GeglAttributes, 1);
  attrs->color_model = NULL;
  return attrs;
}

void
gegl_attributes_set_rect(GeglAttributes * attributes,
                         GeglRect *rect)
{
  gegl_rect_copy(&attributes->rect,rect);
}

void
gegl_attributes_get_rect(GeglAttributes * attributes,
                         GeglRect *rect)
{
  gegl_rect_copy(rect, &attributes->rect);
}

GeglRect *
gegl_attributes_rect(GeglAttributes * attributes,
                     GeglRect *rect)
{
  return &attributes->rect;
}

void
gegl_attributes_set_color_model(GeglAttributes * attributes,
                                GeglColorModel * color_model)
{
  attributes->color_model = color_model;
}

GeglColorModel*
gegl_attributes_get_color_model(GeglAttributes * attributes)
{
  return attributes->color_model;
}
