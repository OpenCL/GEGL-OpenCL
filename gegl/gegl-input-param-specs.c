#include    "gegl-input-param-specs.h"
#include    "gegl-input-value-types.h"
#include    "gegl-node.h"
#include    "gegl-utils.h"

GType GEGL_TYPE_PARAM_INPUT = 0;

/* --- input param spec functions --- */

static void 
param_spec_input_set_default (GParamSpec *pspec,
                              GValue     *value)
{
  value->data[0].v_int = -1;
  value->data[1].v_pointer = NULL;
}

static gboolean
param_spec_input_validate (GParamSpec *pspec,
                           GValue     *value)
{
  GeglNode *node = (GeglNode*)value->data[1].v_pointer;
  guint changed = 0;

  if (node && !GEGL_IS_NODE(node))
  {
    g_object_unref (node);
    value->data[1].v_pointer = NULL;
    changed++;
  }

  return changed;
}

void
gegl_input_param_spec_types_init (void)
{
  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecInput),          /* instance_size */
        16,                                   /* n_preallocs */
        NULL,                                 /* instance_init */
        0x0,                                  /* value_type */
        NULL,                                 /* finalize */
        param_spec_input_set_default,         /* value_set_default */
        param_spec_input_validate,            /* value_validate */
        NULL,                                 /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_INPUT;
    GEGL_TYPE_PARAM_INPUT = 
      g_param_type_register_static ("GeglParamInput", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_INPUT == 
              g_type_from_name("GeglParamInput"));
  }
}

GParamSpec*
gegl_param_spec_input (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       GParamFlags  flags)
{
  GeglParamSpecInput *spec;
  spec = g_param_spec_internal (GEGL_TYPE_PARAM_INPUT, 
                                 name, nick, blurb, flags);
  return G_PARAM_SPEC (spec);
}
