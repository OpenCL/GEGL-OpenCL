#include    "gegl-array-param-specs.h"
#include    "gegl-array-value-types.h"
#include    "gegl-utils.h"

GType GEGL_TYPE_PARAM_FLOAT_ARRAY = 0;

static void 
param_spec_float_array_set_default (GParamSpec *pspec,
                                    GValue     *value)
{
  //GeglParamSpecFloatArray *spec = GEGL_PARAM_SPEC_FLOAT_ARRAY (pspec);

  value->data[0].v_int = 0;
  value->data[1].v_pointer = NULL;
}

static gboolean
param_spec_float_array_validate (GParamSpec *pspec,
                                 GValue     *value)
{
  //GeglParamSpecFloatArray *spec = GEGL_PARAM_SPEC_FLOAT_ARRAY (pspec);
  
  return FALSE; 
}

void
gegl_array_param_spec_types_init (void)
{
  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecFloatArray),      /* instance_size */
        16,                                    /* n_preallocs */
        NULL,                                  /* instance_init */
        0x0,                                   /* value_type */
        NULL,                                  /* finalize */
        param_spec_float_array_set_default,    /* value_set_default */
        param_spec_float_array_validate,       /* value_validate */
        NULL,                                  /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_FLOAT_ARRAY;
    GEGL_TYPE_PARAM_FLOAT_ARRAY = 
      g_param_type_register_static ("GeglParamFloatArray", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_FLOAT_ARRAY == 
              g_type_from_name("GeglParamFloatArray"));
  }
}

GParamSpec*
gegl_param_spec_float_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamFlags  flags)
{
  GeglParamSpecFloatArray *spec;

  spec = g_param_spec_internal (GEGL_TYPE_PARAM_FLOAT_ARRAY, 
                                 name, nick, blurb, flags);

  return G_PARAM_SPEC (spec);
}
