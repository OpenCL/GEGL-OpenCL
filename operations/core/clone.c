#ifdef CHANT_SELF

chant_string (ref, "")

#else

#define CHANT_NAME                 "clone"
#define CHANT_SELF                 "clone.c"
#define CHANT_SUPER_CLASS_FILTER
#define CHANT_STATIC
#define CHANT_TypeName             OpClone
#define CHANT_type_name            op_clone
#define CHANT_type_name_init       op_clone_init
#define CHANT_type_name_class_init op_clone_class_init
#include "gegl-chant.h"


/* Actual image processing code
 ************************************************************************/
static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglOperationFilter   *filter = GEGL_OPERATION_FILTER(operation);
  GeglBuffer *input  = filter->input;

  if (filter->output)
    g_object_unref (filter->output);

  filter->output = g_object_ref (input);
  return  TRUE;
}

#endif
