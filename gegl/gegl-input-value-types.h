#ifndef __GEGL_INPUT_VALUE_H__
#define __GEGL_INPUT_VALUE_H__

#include <glib-object.h>
#include "gegl-types.h"
#include "gegl-node.h"

#ifndef __TYPEDEF_GEGL_NODE__
#define __TYPEDEF_GEGL_NODE__
typedef struct _GeglNode GeglNode;
#endif

/* --- derived input type --- */
extern GType GEGL_TYPE_INPUT;

/* --- type macros --- */
#define G_VALUE_HOLDS_INPUT(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_INPUT))

/* --- prototypes --- */
void gegl_input_value_types_init (void);

GeglNode* g_value_get_input (const GValue *value,
                             gint *n);

#endif /* __GEGL_INPUT_VALUE_H__ */
