#ifndef __AFFINE_MODULE_H__
#define __AFFINE_MODULE_H__

G_BEGIN_DECLS

#include <glib-object.h>
#include <gegl-module.h>

GTypeModule          * affine_module_get_module (void);
const GeglModuleInfo * gegl_module_query        (GTypeModule *module);
gboolean               gegl_module_register     (GTypeModule *module);

G_END_DECLS

#endif
