#include    <gobject/gvaluecollector.h>

#include    "gegl-array-value-types.h"
#include    "gegl-utils.h"
#include    <string.h>

/* --- array types --- */
GType GEGL_TYPE_FLOAT_ARRAY = 0;

/* --- array value functions --- */
static void
value_init_float_array (GValue *value)
{
  value->data[0].v_int = 0;
  value->data[1].v_pointer = NULL; 
}

static void
value_free_float_array (GValue *value)
{
  g_free(value->data[1].v_pointer); 
  value->data[1].v_pointer = NULL;
}

static void
value_copy_float_array (const GValue *src_value,
                        GValue *dest_value)
{
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  memcpy(dest_value->data[1].v_pointer, 
         src_value->data[1].v_pointer, 
         src_value->data[0].v_int * sizeof(gfloat));
}

static gchar*
value_collect_float_array (GValue      *value,
                           guint        n_collect_values,
                           GTypeCValue *collect_values,
                           guint        collect_flags)
{
  value->data[0].v_int = collect_values[0].v_int;
  value->data[1].v_pointer = collect_values[1].v_pointer;

  return NULL;
}

static gchar*
value_lcopy_float_array (const GValue *value,
                         guint         n_collect_values,
                         GTypeCValue  *collect_values,
                         guint         collect_flags)
{
  gint *n_p = collect_values[0].v_pointer;
  gfloat**data_p = collect_values[1].v_pointer;

  if (!n_p || !data_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", 
                             G_VALUE_TYPE_NAME (value));
  
  *n_p = value->data[0].v_int;

  if(collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *data_p = value->data[1].v_pointer;
  else
    *data_p = g_memdup(value->data[1].v_pointer, 
                       value->data[0].v_int * sizeof(gfloat));
  
  return NULL;
}

void
gegl_array_value_types_init (void)
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
        value_init_float_array,        /* value_init */
        value_free_float_array,        /* value_free */
        value_copy_float_array,        /* value_copy */
        NULL,			               /* value_peek_pointer */
        "ip",                          /* collect_format */
        value_collect_float_array,     /* collect_value */
        "pp",                          /* lcopy_format */
        value_lcopy_float_array,       /* lcopy_value */
      };
    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_FLOAT_ARRAY == 0);
  GEGL_TYPE_FLOAT_ARRAY = g_type_fundamental_next();
  type = g_type_register_fundamental (GEGL_TYPE_FLOAT_ARRAY, 
                                      "GeglFloatArray", 
                                      &info, 
                                      &finfo, 
                                      0);

  g_assert (type == GEGL_TYPE_FLOAT_ARRAY);
}

void
g_value_set_float_array (GValue *value,
                         gint  length,
                         const gfloat  *data)
{
  g_return_if_fail (G_VALUE_HOLDS_FLOAT_ARRAY (value));

  value->data[0].v_int = length;
  value->data[1].v_pointer = g_memdup(data, sizeof(gfloat) * length);
}

const gfloat*
g_value_get_float_array (const GValue *value,
                         gint *length)
{
  g_return_val_if_fail (G_VALUE_HOLDS_FLOAT_ARRAY (value), NULL);

  *length = value->data[0].v_int; 
  return value->data[1].v_pointer;
}
