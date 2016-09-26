#!/bin/sh

echo '#include "config.h"'
echo '#include <gegl-plugin.h>'

for a in `grep GEGL_OP_NAME "$@" | sed 's/.*NAME//'`;do echo "void gegl_op_"$a"_register_type(GTypeModule *module);" ;done

echo 'static const GeglModuleInfo modinfo = {
GEGL_MODULE_ABI_VERSION
};

const GeglModuleInfo * gegl_module_query (GTypeModule *module);
gboolean gegl_module_register (GTypeModule *module);

G_MODULE_EXPORT const GeglModuleInfo *
gegl_module_query (GTypeModule *module)
{
  return &modinfo;
}

G_MODULE_EXPORT gboolean
gegl_module_register (GTypeModule *module)
{'


for a in `grep GEGL_OP_NAME "$@" | sed 's/.*NAME//'`;do echo "  gegl_op_"$a"_register_type(module);" ;done


 echo ' return TRUE;
}'

