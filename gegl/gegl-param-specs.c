#include    "gegl-param-specs.h"

/* --- param spec functions --- */
static void
gegl_param_spec_int_init (GeglParamSpecInt *ispec)
{
  ispec->increment = 1;
}

/* --- type initialization --- */
GType *gegl_param_spec_types = NULL;

void
gegl_param_spec_types_init (void)
{
  const guint n_types = 1;
  GType type, *spec_types, *spec_types_bound;

  gegl_param_spec_types = g_new0 (GType, n_types);
  spec_types = gegl_param_spec_types;
  spec_types_bound = gegl_param_spec_types + n_types;

  { 
    static const GTypeInfo info = 
    {
      sizeof (GParamSpecClass),   /* class_size */
      NULL,                       /* base_init */
      NULL,                       /* base_destroy */
      NULL,                       /* class_init */
      NULL,                       /* class_destroy */
      NULL,                       /* class_data */
      sizeof (GeglParamSpecInt),  /* instance_size */
      8,                          /* n_preallocs */
      (GInstanceInitFunc)gegl_param_spec_int_init,   /* instance_init */
    };

    type = g_type_register_static (G_TYPE_PARAM_INT, "GeglParamInt", &info, 0);
    *spec_types++ = type;
    g_assert (type == g_type_from_name ("GeglParamInt"));
  }

  g_assert (spec_types == spec_types_bound);
}

/* --- GParamSpec initialization --- */
GParamSpec*
gegl_param_spec_int (const gchar *name,
                     const gchar *nick,
                     const gchar *blurb,
                     gint         minimum,
                     gint         maximum,
                     gint         default_value,
                     gint         increment,
                     GParamFlags  flags)
{
  GeglParamSpecInt *ispec;

  g_return_val_if_fail (default_value >= minimum && default_value <= maximum, NULL);

  ispec = g_param_spec_internal (GEGL_TYPE_PARAM_INT, name, nick, blurb, flags);
  
  G_PARAM_SPEC_INT(ispec)->minimum = minimum;
  G_PARAM_SPEC_INT(ispec)->maximum = maximum;
  G_PARAM_SPEC_INT(ispec)->default_value = minimum;
  ispec->increment = increment;
  
  return G_PARAM_SPEC (ispec);
}
