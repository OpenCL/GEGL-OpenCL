#include    <gobject/gvaluecollector.h>

#include    "gegl-value-types.h"
#include    "gegl-param-specs.h"
#include    "gegl-types.h"
#include    "gegl-utils.h"


/* --- value functions --- */

static void
value_init_image_data(GValue *value) 
{
  value->data[0].v_pointer = NULL;
}

static void
value_free_image_data(GValue *value) 
{
  g_free(value->data[0].v_pointer);
}

static void
value_copy_image_data(const GValue *src_value,
                      GValue *dest_value) 
{
  dest_value->data[0].v_pointer = g_memdup(src_value->data[0].v_pointer, 
                                           sizeof(GeglImageData));
                                            
}

static gchar*
value_collect_image_data(GValue      *value,
                         guint        n_collect_values,
                         GTypeCValue *collect_values,
                         guint        collect_flags)
{
  if (!collect_values[0].v_pointer)
    return g_strdup_printf ("ImageData value passed as NULL");

  value->data[0].v_pointer = g_memdup (collect_values[0].v_pointer, 
                                       sizeof (GeglImageData));

  return NULL;
}

static gchar*
value_lcopy_image_data(const GValue *value,
                       guint         n_collect_values,
                       GTypeCValue  *collect_values,
                       guint         collect_flags)
{
  GeglImageData ** image_data_p = collect_values[0].v_pointer;

  if(!image_data_p)
    return g_strdup_printf ("ImageData location for %s passed as NULL",
                             G_VALUE_TYPE_NAME(value));

  if(collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *image_data_p = value->data[0].v_pointer; 
  else
    *image_data_p = g_memdup(value->data[0].v_pointer, 
                             sizeof(GeglImageData)); 
  
  return NULL;
}

/* --- type initialization --- */

void
gegl_value_types_init (void)
{
  GTypeInfo info = {
    0,               /* class_size */
    NULL,            /* base_init */
    NULL,            /* base_destroy */
    NULL,            /* class_init */
    NULL,            /* class_destroy */
    NULL,            /* class_data */
    0,               /* instance_size */
    0,               /* n_preallocs */
    NULL,            /* instance_init */
    NULL,            /* value_table */
  };
  const GTypeFundamentalInfo finfo = { G_TYPE_FLAG_DERIVABLE, };
  GType type;
  
  /* GEGL_TYPE_IMAGE_DATA
   */
  {
    static const GTypeValueTable value_table = {
      value_init_image_data,   /* value_init */
      value_free_image_data,   /* value_free */
      value_copy_image_data,   /* value_copy */
      NULL,                    /* value_peek_pointer */
      "p",                     /* collect_format */
      value_collect_image_data,/* collect_value */
      "p",                     /* lcopy_format */
      value_lcopy_image_data,  /* lcopy_value */
    };

    info.value_table = &value_table;
    type = g_type_register_fundamental (GEGL_TYPE_IMAGE_DATA, 
                                        "GeglImageData", 
                                        &info, 
                                        &finfo, 
                                        0);

    g_assert (type == GEGL_TYPE_IMAGE_DATA);
  }
}

/* --- GValue functions --- */

void
g_value_set_image_data(GValue *value,
                       GeglTile * tile,
                       GeglRect * rect)
{
   GeglImageData * old_image_data;
   GeglImageData * image_data;

   g_return_if_fail (G_VALUE_HOLDS_IMAGE_DATA (value));

   old_image_data = value->data[0].v_pointer;

   image_data = value->data[0].v_pointer = g_new (GeglImageData, 1);

   image_data->tile = tile;
   gegl_rect_copy(&image_data->rect, rect); 
   g_free(old_image_data);
}

GeglTile*
g_value_get_image_data(const GValue *value,
                       GeglRect *rect)
{
   GeglImageData * image_data;
   g_return_val_if_fail (G_VALUE_HOLDS_IMAGE_DATA (value), NULL);

   image_data = value->data[0].v_pointer;

   gegl_rect_copy(rect, &image_data->rect);

   return image_data->tile;
}

void
g_value_set_image_data_rect(GValue *value,
                            GeglRect * rect)
{
   GeglImageData * old_image_data;
   GeglImageData * image_data; 
   g_return_if_fail (G_VALUE_HOLDS_IMAGE_DATA (value));

   old_image_data = value->data[0].v_pointer;
   image_data = value->data[0].v_pointer = g_new(GeglImageData, 1);

   image_data->tile = old_image_data? old_image_data->tile: NULL;
   gegl_rect_copy(&image_data->rect, rect); 

   g_free(old_image_data);
}

void
g_value_get_image_data_rect(const GValue *value,
                            GeglRect * rect)
{
   GeglImageData * image_data; 
   g_return_if_fail (G_VALUE_HOLDS_IMAGE_DATA (value));
   image_data = value->data[0].v_pointer;
   g_assert(image_data);
   gegl_rect_copy(rect, &image_data->rect); 
}

void
g_value_set_image_data_tile(GValue *value,
                            GeglTile * tile)
{
   GeglImageData * old_image_data;
   GeglImageData * image_data; 

   g_return_if_fail (G_VALUE_HOLDS_IMAGE_DATA (value));

   old_image_data = value->data[0].v_pointer;
   image_data = value->data[0].v_pointer = g_new(GeglImageData, 1);

   image_data->tile = tile; 

   if(old_image_data)
     gegl_rect_copy(&image_data->rect, &old_image_data->rect); 
   else
     gegl_rect_set(&image_data->rect, 0,0,-1,-1); 

   g_free(old_image_data);
}

GeglTile *
g_value_get_image_data_tile(const GValue *value)
{
  GeglImageData * image_data; 
  g_return_val_if_fail (G_VALUE_HOLDS_IMAGE_DATA (value), NULL);
  image_data = value->data[0].v_pointer;
  return image_data->tile; 
}

void
g_value_print_image_data(GValue *value,
                         char * msg)
{
  GeglImageData * image_data; 
  g_return_if_fail (G_VALUE_HOLDS_IMAGE_DATA (value));
  image_data = value->data[0].v_pointer;
  LOG_DEBUG("g_value_print_image_data", "%s rect: %d %d %d %d tile: %x",
            msg,
            image_data->rect.x, 
            image_data->rect.y, 
            image_data->rect.w, 
            image_data->rect.h,
            (guint)image_data->tile); 
}
