#include    <gobject/gvaluecollector.h>

#include    "gegl-channel-value-types.h"
#include    "gegl-utils.h"
#include    "gegl-color-space.h"
#include    "gegl-color-model.h"
#include    <string.h>

/* --- channel types --- */
GType GEGL_TYPE_CHANNEL_FLOAT = 0;
GType GEGL_TYPE_CHANNEL_UINT8 = 0;

/* --- channel value functions --- */
static void
value_init_channel_uint8 (GValue *value)
{
  value->data[0].v_uint = 0; 
}

static void
value_free_channel_uint8 (GValue *value)
{
}

static void
value_copy_channel_uint8 (const GValue *src_value,
                          GValue       *dest_value)
{
  memcpy(dest_value, src_value, sizeof(GValue));
}

static gchar*
value_collect_channel_uint8 (GValue      *value,
                             guint        n_collect_values,
                             GTypeCValue *collect_values,
                             guint        collect_flags)
{
  value->data[0].v_uint = collect_values[0].v_int;
  return NULL;
}

static gchar*
value_lcopy_channel_uint8 (const GValue *value,
                           guint         n_collect_values,
                           GTypeCValue  *collect_values,
                           guint         collect_flags)
{
  guint8 *uint8_p = collect_values[0].v_pointer;
  
  if (!uint8_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", 
                             G_VALUE_TYPE_NAME (value));
  
  *uint8_p = value->data[0].v_uint;
  
  return NULL;
}

static void
value_init_channel_float (GValue *value)
{
  value->data[0].v_float = 0.0; 
}

static void
value_free_channel_float (GValue *value)
{
}

static void
value_copy_channel_float (const GValue *src_value,
                          GValue       *dest_value)
{
  memcpy(dest_value, src_value, sizeof(GValue));
}

static gchar*
value_collect_channel_float (GValue      *value,
                             guint        n_collect_values,
                             GTypeCValue *collect_values,
                             guint        collect_flags)
{
  value->data[0].v_float = collect_values[0].v_double;
  return NULL;
}

static gchar*
value_lcopy_channel_float (const GValue *value,
                           guint         n_collect_values,
                           GTypeCValue  *collect_values,
                           guint         collect_flags)
{
  gfloat *float_p = collect_values[0].v_pointer;
  
  if (!float_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", 
                            G_VALUE_TYPE_NAME (value));
  
  *float_p = value->data[0].v_float;
  
  return NULL;
}

void
gegl_channel_value_types_init (void)
{
  static ChannelValueInfo channel_value_info_float = {"float", 32};
  static ChannelValueInfo channel_value_info_uint8 = {"uint8", 8};

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

  {
    static const GTypeValueTable value_table = 
      {
        value_init_channel_uint8,    /* value_init */
        value_free_channel_uint8,    /* value_free */
        value_copy_channel_uint8,    /* value_copy */
        NULL,			             /* value_peek_pointer */
        "i",                         /* collect_format */
        value_collect_channel_uint8, /* collect_value */
        "p",                         /* lcopy_format */
        value_lcopy_channel_uint8,   /* lcopy_value */
      };
    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_CHANNEL_UINT8 == 0);
  GEGL_TYPE_CHANNEL_UINT8 = g_type_register_static (GEGL_TYPE_CHANNEL, 
                                            "GeglChannelUInt8", 
                                            &info, 
                                            0);
  g_assert (GEGL_TYPE_CHANNEL_UINT8 == g_type_from_name("GeglChannelUInt8"));

  g_type_set_qdata(GEGL_TYPE_CHANNEL_UINT8, 
                   g_quark_from_string("channel_value_info"), 
                   &channel_value_info_uint8);

  {
    static const GTypeValueTable value_table = 
      {
        value_init_channel_float,    /* value_init */
        value_free_channel_float,    /* value_free */
        value_copy_channel_float,    /* value_copy */
        NULL,			             /* value_peek_pointer */
        "d",	                     /* collect_format */
        value_collect_channel_float, /* collect_value */
        "p",  			             /* lcopy_format */
        value_lcopy_channel_float,   /* lcopy_value */
      };

    info.value_table = &value_table;
  }

  g_assert (GEGL_TYPE_CHANNEL_FLOAT == 0);
  GEGL_TYPE_CHANNEL_FLOAT = g_type_register_static (GEGL_TYPE_CHANNEL, 
                                            "GeglChannelFloat", 
                                            &info, 
                                            0);
  g_assert (GEGL_TYPE_CHANNEL_FLOAT == g_type_from_name("GeglChannelFloat"));

  g_type_set_qdata(GEGL_TYPE_CHANNEL_FLOAT, 
                   g_quark_from_string("channel_value_info"), 
                   &channel_value_info_float);

}

static void 
transform_uint8_float(const GValue *src_value,
                      GValue *dest_value)
{
  /* Dest value was unset, so we have to call value_init? */
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  dest_value->data[0].v_float = src_value->data[0].v_uint/255.0;
}

static void 
transform_float_uint8(const GValue *src_value,
                      GValue *dest_value)
{
  /* Dest value was unset, so we have to call value_init? */
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  {
   gint channel = ROUND(src_value->data[0].v_float * 255.0);
   dest_value->data[0].v_uint = CLAMP(channel, 0, 255);
  }

}

void
gegl_channel_value_transform_init(void) 
{
  g_value_register_transform_func(GEGL_TYPE_CHANNEL_UINT8, GEGL_TYPE_CHANNEL_FLOAT, transform_uint8_float);
  g_value_register_transform_func(GEGL_TYPE_CHANNEL_FLOAT, GEGL_TYPE_CHANNEL_UINT8, transform_float_uint8);
}

/* --- Channel GValue functions --- */

void
g_value_set_channel_uint8 (GValue *value,
                        guint8    v_uint8)
{
  g_return_if_fail (G_VALUE_HOLDS_CHANNEL_UINT8 (value));

  value->data[0].v_uint = v_uint8;
}

guint8
g_value_get_channel_uint8 (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_CHANNEL_UINT8 (value), 0);

  return (guint8)value->data[0].v_uint;
}

void
g_value_set_channel_float (GValue *value,
                        gfloat  v_float)
{
  g_return_if_fail (G_VALUE_HOLDS_CHANNEL_FLOAT (value));

  value->data[0].v_float = v_float;
}

gfloat
g_value_get_channel_float (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_CHANNEL_FLOAT (value), 0);

  return value->data[0].v_float;
}

void
g_value_channel_print(GValue * value)
{
  g_return_if_fail (G_VALUE_HOLDS_CHANNEL_UINT8(value) ||
                    G_VALUE_HOLDS_CHANNEL_FLOAT(value));

  if (G_VALUE_HOLDS_CHANNEL_UINT8 (value))
    {
      gegl_log_direct("channel value type is uint8");
      gegl_log_direct("value is %d", value->data[0].v_uint);
    }
  else if (G_VALUE_HOLDS_CHANNEL_FLOAT (value))
    {
      gegl_log_direct("channel value type is float");
      gegl_log_direct("value is %f", value->data[0].v_float);
    }
}
