#include    "gegl-m-source-param-specs.h"
#include    "gegl-m-source-value-types.h"
#include    "gegl-node.h"
#include    "gegl-utils.h"

GType GEGL_TYPE_PARAM_M_SOURCE = 0;

/* --- m_source param spec functions --- */

static void 
param_spec_m_source_set_default (GParamSpec *pspec,
                              GValue     *value)
{
  value->data[1].v_int = -1;
  value->data[0].v_pointer = NULL;
}

static gboolean
param_spec_m_source_validate (GParamSpec *pspec,
                           GValue     *value)
{
  GeglNode *node = (GeglNode*)value->data[0].v_pointer;
  guint changed = 0;

  if (node && !GEGL_IS_NODE(node))
  {
    g_object_unref (node);
    value->data[0].v_pointer = NULL;
    changed++;
  }

  return changed;
}

void
gegl_m_source_param_spec_types_init (void)
{
  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecMSource),          /* instance_size */
        16,                                   /* n_preallocs */
        NULL,                                 /* instance_init */
        0x0,                                  /* value_type */
        NULL,                                 /* finalize */
        param_spec_m_source_set_default,         /* value_set_default */
        param_spec_m_source_validate,            /* value_validate */
        NULL,                                 /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_M_SOURCE;
    GEGL_TYPE_PARAM_M_SOURCE = 
      g_param_type_register_static ("GeglParamMSource", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_M_SOURCE == 
              g_type_from_name("GeglParamMSource"));
  }
}

GParamSpec*
gegl_param_spec_m_source (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       GParamFlags  flags)
{
  GeglParamSpecMSource *spec;
  spec = g_param_spec_internal (GEGL_TYPE_PARAM_M_SOURCE, 
                                 name, nick, blurb, flags);
  return G_PARAM_SPEC (spec);
}
