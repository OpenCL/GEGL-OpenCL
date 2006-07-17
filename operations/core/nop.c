#ifdef CHANT_SELF

#else

#define CHANT_NAME                 "nop"
#define CHANT_DESCRIPTION          "Passthrough operation"
#define CHANT_SELF                 "nop.c"
#define CHANT_SUPER_CLASS_FILTER
#define CHANT_CLASS_CONSTRUCT
#define CHANT_TypeName             OpNop
#define CHANT_type_name            op_nop
#include "gegl-chant.h"

static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  /* not used */
  return  TRUE;
}

static gboolean
op_evaluate (GeglOperation *operation,
             const gchar   *output_prop)
{
  GeglOperationFilter      *op_filter = GEGL_OPERATION_FILTER (operation);
  gboolean success = FALSE;

  if (op_filter->output != NULL)
    {
      g_object_unref (op_filter->output);
      op_filter->output = NULL;
    }

  if (op_filter->input != NULL)
    {
      op_filter->output=g_object_ref (op_filter->input);
      g_object_unref (op_filter->input);
      op_filter->input=NULL;
    }
  /* the NOP op does not complain about NULL inputs */
  return success;
}
static void class_init (GeglOperationClass *klass)
{
  klass->evaluate = op_evaluate;
}


#endif
