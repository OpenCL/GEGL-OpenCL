#include    <gobject/gvaluecollector.h>
#include    "gegl-m-source-value-types.h"
#include    "gegl-utils.h"

/* --- m_source types --- */
GType GEGL_TYPE_M_SOURCE = 0;

/* --- m_source value functions --- */
static void
value_init_m_source (GValue *value)
{
  value->data[0].v_pointer = NULL; 
  value->data[1].v_int = -1;
}

static void
value_free_m_source (GValue *value)
{
  value->data[1].v_int = -1;

  if(value->data[0].v_pointer)
    {
      GeglNode *node = GEGL_NODE(value->data[0].v_pointer); 
      g_object_unref(node);
      value->data[0].v_pointer = NULL;
    }
}

static void
value_copy_m_source (const GValue *src_value,
                  GValue       *dest_value)
{
  dest_value->data[1].v_int = src_value->data[1].v_int;

  if (src_value->data[0].v_pointer)
    dest_value->data[0].v_pointer = g_object_ref (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static gchar*
value_collect_m_source (GValue      *value,
                     guint        n_collect_values,
                     GTypeCValue *collect_values,
                     guint        collect_flags)
{
  g_assert(n_collect_values == 2);
  value->data[1].v_int = collect_values[1].v_int;
  if(collect_values[0].v_pointer)
    {
      GeglNode *node = GEGL_NODE(collect_values[0].v_pointer); 
      value->data[0].v_pointer = g_object_ref(node);
    }

  return NULL;
}

void
gegl_m_source_value_types_init (void)
{
  GTypeInfo info = 
  {
    0,				/* class_size */
    NULL,			/* base_init */
    NULL,			/* base_destroy */
    NULL,			/* class_init */
    NULL,			/* class_destroy */
    NULL,			/* class_data */
    0,				/* instance_size */
    0,				/* n_preallocs */
    NULL,			/* instance_init */
    NULL,			/* value_table */
  };
  const GTypeFundamentalInfo finfo = { G_TYPE_FLAG_DERIVABLE, };
  GType type;

  {
    static const GTypeValueTable value_table = 
      {
        value_init_m_source,          /* value_init */
        value_free_m_source,          /* value_free */
        value_copy_m_source,          /* value_copy */
        NULL,			             /* value_peek_pointer */
        "pi",                        /* collect_format */
        value_collect_m_source,       /* collect_value */
        NULL,                        /* lcopy_format */
        NULL,                        /* lcopy_value */
      };
    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_M_SOURCE == 0);
  GEGL_TYPE_M_SOURCE = G_TYPE_MAKE_FUNDAMENTAL(52);
  type = g_type_register_fundamental (GEGL_TYPE_M_SOURCE, 
                                      "GeglMSource", 
                                      &info, 
                                      &finfo, 0);

  g_assert (type == GEGL_TYPE_M_SOURCE);
}

/* --- MSource GValue functions --- */

GeglNode*
g_value_get_m_source (const GValue *value,
                   gint *n)
{
  g_return_val_if_fail (G_VALUE_HOLDS_M_SOURCE (value), 0);
  *n = value->data[1].v_int; 
  return (GeglNode*)value->data[0].v_pointer;
}
