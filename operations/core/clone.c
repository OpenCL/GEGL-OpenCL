#ifdef CHANT_SELF

chant_string (ref, "", "The reference ID used as input.")

#else

#define CHANT_NAME                 "clone"
#define CHANT_DESCRIPTION          "A routing op used by the XML data model to implement clones."

#define CHANT_SELF                 "clone.c"
#define CHANT_SUPER_CLASS_FILTER
#define CHANT_TypeName             OpClone
#define CHANT_type_name            op_clone
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
