#ifndef __GEGL_ARRAY_VALUE_TYPES_H__
#define __GEGL_ARRAY_VALUE_TYPES_H__

#include <glib-object.h>

extern GType GEGL_TYPE_FLOAT_ARRAY;

#define G_VALUE_HOLDS_FLOAT_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_FLOAT_ARRAY))

void g_value_set_float_array (GValue *value, gint  length, const gfloat  *data);
const gfloat* g_value_get_float_array (const GValue *value, gint *length);

void gegl_array_value_types_init (void);

#endif /* __GEGL_ARRAY_VALUE_TYPES_H__ */
