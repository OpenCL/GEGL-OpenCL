#ifndef __TRANSFORM_MODULE_H__
#define __TRANSFORM_MODULE_H__

G_BEGIN_DECLS

#include <glib-object.h>
#include <gegl-plugin.h>

G_MODULE_EXPORT GTypeModule          * transform_module_get_module (void);
G_MODULE_EXPORT const GeglModuleInfo * gegl_module_query        (GTypeModule *module);
G_MODULE_EXPORT gboolean               gegl_module_register     (GTypeModule *module);

G_END_DECLS

#endif
