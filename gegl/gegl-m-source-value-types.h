#ifndef __GEGL_M_SOURCE_VALUE_H__
#define __GEGL_M_SOURCE_VALUE_H__

#include <glib-object.h>
#include "gegl-types.h"
#include "gegl-node.h"

/* --- derived m_source type --- */
extern GType GEGL_TYPE_M_SOURCE;

/* --- type macros --- */
#define G_VALUE_HOLDS_M_SOURCE(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_M_SOURCE))

/* --- prototypes --- */
void gegl_m_source_value_types_init (void);

GeglNode* g_value_get_m_source (const GValue *value,
                             gint *n);

#endif /* __GEGL_M_SOURCE_VALUE_H__ */
