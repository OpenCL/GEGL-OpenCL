#ifndef __GEGL_VALUE_TYPES_H__
#define __GEGL_VALUE_TYPES_H__

#include <glib-object.h>
#include "gegl-types.h"

#ifndef __TYPEDEF_GEGL_TILE__
#define __TYPEDEF_GEGL_TILE__
typedef struct _GeglTile  GeglTile;
#endif


/* --- type macros --- */
#define G_VALUE_HOLDS_IMAGE_DATA(value)        (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_IMAGE_DATA))


/* --- typedefs & structures --- */
typedef struct _GeglImageData          GeglImageData;

struct _GeglImageData 
{
  GeglTile * tile;
  GeglRect rect;
};

/* --- prototypes --- */
GeglTile*             g_value_get_image_data(const GValue *value, 
                                             GeglRect * rect);

void                  g_value_set_image_data(GValue *value, 
                                             GeglTile * tile,
                                             GeglRect * rect);

void                  g_value_get_image_data_rect(const GValue *value,
                                                  GeglRect *rect);
void                  g_value_set_image_data_rect(GValue *value, 
                                                  GeglRect * rect);

GeglTile *            g_value_get_image_data_tile(const GValue *value);
void                  g_value_set_image_data_tile(GValue *value, 
                                                  GeglTile * tile);

void                  g_value_print_image_data(GValue *value, char *msg);

#endif /* __GEGL_VALUE_TYPES_H__ */
