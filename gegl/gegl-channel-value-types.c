#include    <gobject/gvaluecollector.h>

#include    "gegl-channel-value-types.h"
#include    "gegl-utils.h"
#include    "gegl-data-space.h"
#include    "gegl-color-space.h"
#include    "gegl-color-model.h"
#include    <string.h>

/* --- channel types --- */
GType GEGL_TYPE_CHANNEL_FLOAT = 0;
GType GEGL_TYPE_CHANNEL_UINT8 = 0;

#if 0
GType GEGL_TYPE_CHANNEL2_UINT8 = 0;
GType GEGL_TYPE_CHANNEL2_FLOAT = 0;

GType GEGL_TYPE_CHANNEL3_UINT8 = 0;
GType GEGL_TYPE_CHANNEL3_FLOAT = 0;

GType GEGL_TYPE_CHANNEL4_UINT8 = 0;
GType GEGL_TYPE_CHANNEL4_FLOAT = 0;
#endif

/* --- channel value functions --- */
static void
value_init_channel_uint8 (GValue *value)
{
  value->data[0].v_pointer = gegl_data_space_instance("uint8");
  value->data[1].v_uint = 0; 
}

static void
value_free_channel_uint8 (GValue *value)
{
  value->data[0].v_pointer = NULL;
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
  value->data[0].v_pointer = gegl_data_space_instance("uint8");
  value->data[1].v_uint = collect_values[0].v_int;
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
  
  *uint8_p = value->data[1].v_uint;
  
  return NULL;
}

static void
value_init_channel_float (GValue *value)
{
  value->data[0].v_pointer = gegl_data_space_instance("float");
  value->data[1].v_float = 0.0; 
}

static void
value_free_channel_float (GValue *value)
{
  value->data[0].v_pointer = NULL;
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
  value->data[0].v_pointer = gegl_data_space_instance("float");
  value->data[1].v_float = collect_values[0].v_double;
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
  
  *float_p = value->data[1].v_float;
  
  return NULL;
}

void
gegl_channel_value_types_init (void)
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
}

static void 
value_transform_channel(const GValue *src_value,
                        GValue *dest_value)
{
  /* Dest value was unset, so we have to call value_init? */
  GTypeValueTable * value_table = g_type_value_table_peek(G_VALUE_TYPE(dest_value));
  value_table->value_init(dest_value);

  {
    GeglDataSpace *src_data_space = src_value->data[0].v_pointer;
    GeglDataSpace *dest_data_space = dest_value->data[0].v_pointer;
    GValue  float_value = {0,}; 
    g_value_init(&float_value, GEGL_TYPE_CHANNEL_FLOAT);
    gegl_data_space_convert_value_to_float(src_data_space, 
                                           &float_value, 
                                           (GValue*)src_value); 

    gegl_data_space_convert_value_from_float(dest_data_space, 
                                             dest_value, 
                                             &float_value);
  }
}

void
gegl_channel_value_transform_init(void) 
{
  g_value_register_transform_func(GEGL_TYPE_CHANNEL_UINT8, GEGL_TYPE_CHANNEL_FLOAT, value_transform_channel);
  g_value_register_transform_func(GEGL_TYPE_CHANNEL_FLOAT, GEGL_TYPE_CHANNEL_UINT8, value_transform_channel);
}

/* --- Channel GValue functions --- */

void
g_value_set_channel_uint8 (GValue *value,
                        guint8    v_uint8)
{
  g_return_if_fail (G_VALUE_HOLDS_CHANNEL_UINT8 (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_data_space_instance("uint8"));

  value->data[1].v_uint = v_uint8;
}

guint8
g_value_get_channel_uint8 (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_CHANNEL_UINT8 (value), 0);
  g_return_val_if_fail (value->data[0].v_pointer == 
                        gegl_data_space_instance("uint8"), 0);

  return (guint8)value->data[1].v_uint;
}

void
g_value_set_channel_float (GValue *value,
                        gfloat  v_float)
{
  g_return_if_fail (G_VALUE_HOLDS_CHANNEL_FLOAT (value));
  g_return_if_fail (value->data[0].v_pointer == 
                    gegl_data_space_instance("float"));

  value->data[1].v_float = v_float;
}

gfloat
g_value_get_channel_float (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_CHANNEL_FLOAT (value), 0);
  g_return_val_if_fail (value->data[0].v_pointer == 
                        gegl_data_space_instance("float"), 0);

  return value->data[1].v_float;
}

void
g_value_channel_print(GValue * value)
{
  GeglDataSpace * data_space = value->data[0].v_pointer;
  gchar * name = "none"; 

  if(data_space)
    name = gegl_data_space_name(data_space);

  LOG_DIRECT("data space name is %s", name);

  if(data_space == gegl_data_space_instance("uint8"))
    LOG_DIRECT("value is %d", value->data[1].v_uint);
  else if(data_space == gegl_data_space_instance("float"))
    LOG_DIRECT("value is %f", value->data[1].v_float);
  else if(data_space == gegl_data_space_instance("u16"))
    LOG_DIRECT("value is %d", value->data[1].v_uint);
}
