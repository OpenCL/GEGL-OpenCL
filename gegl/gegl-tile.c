#include  <gobject/gvaluecollector.h>
#include "gegl-tile.h"
#include "gegl-object.h"
#include "gegl-color-model.h"
#include "gegl-tile-mgr.h"
#include "gegl-buffer.h"
#include "gegl-utils.h"



enum
{
  PROP_0, 
  PROP_AREA,
  PROP_COLOR_MODEL,
  PROP_LAST 
};

static void class_init (GeglTileClass * klass);
static void init (GeglTile *self, GeglTileClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static void finalize(GObject * gobject);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void unalloc (GeglTile * self);

static void value_init_tile (GValue *value);
static void value_free_tile (GValue *value);
static void value_copy_tile(const GValue *src_value, GValue *dest_value);
static void value_transform_tile (const GValue *src_value, GValue *dest_value);
static gpointer value_peek_pointer_tile (const GValue *value); 
static gchar *value_collect_value_tile (GValue*value, guint n_collect_values, GTypeCValue *collect_values, guint collect_flags);
static gchar *value_lcopy_value_tile (const GValue*value, guint n_collect_values, GTypeCValue *collect_values, guint collect_flags);

static gpointer parent_class = NULL;

GType
gegl_tile_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeValueTable value_table = 
      {
        value_init_tile,          /* value_init */
        value_free_tile,          /* value_free */
        value_copy_tile,          /* value_copy */
        value_peek_pointer_tile,  /* value_peek_pointer */
        "p",                      /* collect_format */
        value_collect_value_tile, /* collect_value */
        "p",                      /* lcopy_format */
        value_lcopy_value_tile,   /* lcopy_value */
      };

      static const GTypeInfo typeInfo =
      {
        sizeof (GeglTileClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        sizeof (GeglTile),
        0,
        (GInstanceInitFunc) init,
        &value_table,             /* value_table */
        //NULL,             /* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, "GeglTile", &typeInfo, 0);
      g_value_register_transform_func (GEGL_TYPE_TILE, GEGL_TYPE_TILE, 
                                       value_transform_tile);
    }

    return type;
}

static void 
class_init (GeglTileClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->constructor = constructor;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_AREA,
                                   g_param_spec_pointer ("area",
                                                        "Area",
                                                        "The GeglTile's area",
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_COLOR_MODEL,
                                   g_param_spec_object ("colormodel",
                                                        "ColorModel",
                                                        "The GeglTile's colormodel",
                                                         GEGL_TYPE_COLOR_MODEL,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));

  return;
}

static void 
init (GeglTile * self, 
      GeglTileClass * klass)
{
  return;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglTile *self = GEGL_TILE(gobject);

  if(!gegl_tile_alloc(self, &self->area, self->color_model))
    return FALSE;

  return gobject;
}

static void
finalize(GObject *gobject)
{
  GeglTile *self = GEGL_TILE (gobject);
  unalloc(self);

  if(self->color_model) 
    g_object_ref(self->color_model);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglTile *self = GEGL_TILE (gobject);

  switch (prop_id)
  {
    case PROP_AREA:
      if(!GEGL_OBJECT(gobject)->constructed)
        {
          gegl_tile_set_area (self, (GeglRect*)g_value_get_pointer (value));
        }
      else
        g_message("Cant set area after construction\n");
      break;
    case PROP_COLOR_MODEL:
      if(!GEGL_OBJECT(gobject)->constructed)
        {
          GeglColorModel *color_model = GEGL_COLOR_MODEL(g_value_get_object(value)); 
          gegl_tile_set_color_model (self,color_model);
        }
      else
        g_message("Cant set colormodel after construction\n");
      break;
    default:
      break;
  }
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglTile *self = GEGL_TILE (gobject);

  switch (prop_id)
  {
    case PROP_AREA:
      gegl_tile_get_area (self, (GeglRect*)g_value_get_pointer (value));
      break;
    case PROP_COLOR_MODEL:
      g_value_set_object (value, G_OBJECT(gegl_tile_get_color_model(self)));
      break;
    default:
      break;
  }
}

static void
value_init_tile (GValue *value)
{
  value->data[0].v_pointer = NULL;
}

static void
value_free_tile (GValue *value)
{
  if (value->data[0].v_pointer)
    g_object_unref (value->data[0].v_pointer);
}

static void
value_copy_tile (const GValue *src_value,
                 GValue *dest_value)
{
  if (src_value->data[0].v_pointer)
    dest_value->data[0].v_pointer = g_object_ref (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static void
value_transform_tile (const GValue *src_value,
                      GValue       *dest_value)
{
  if (src_value->data[0].v_pointer && 
      g_type_is_a (G_OBJECT_TYPE (src_value->data[0].v_pointer), G_VALUE_TYPE (dest_value)))
    dest_value->data[0].v_pointer = g_object_ref (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static gpointer
value_peek_pointer_tile (const GValue *value)
{
  return value->data[0].v_pointer;
}

static gchar*
value_collect_value_tile (GValue          *value,
                          guint        n_collect_values,
                          GTypeCValue *collect_values,
                          guint        collect_flags)
{
  if (collect_values[0].v_pointer)
    {
      GObject *object = collect_values[0].v_pointer;
      
      if (object->g_type_instance.g_class == NULL)
        return g_strconcat ("invalid unclassed object pointer for value type `",
                            G_VALUE_TYPE_NAME (value),
                            "'",
                            NULL);
      else if (!g_value_type_compatible (G_OBJECT_TYPE(object), G_VALUE_TYPE (value)))
        return g_strconcat ("invalid object type `",
                            G_OBJECT_TYPE_NAME (object),
                            "' for value type `",
                            G_VALUE_TYPE_NAME (value),
                            "'",
                            NULL);
      /* never honour G_VALUE_NOCOPY_CONTENTS for ref-counted types */
      value->data[0].v_pointer = g_object_ref (object);
    }
  else
    value->data[0].v_pointer = NULL;
  
  return NULL;
}

static gchar*
value_lcopy_value_tile (const GValue *value,
                        guint        n_collect_values,
                        GTypeCValue *collect_values,
                        guint        collect_flags)
{
  GeglTile **tile_p = collect_values[0].v_pointer;
  
  if (!tile_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", G_VALUE_TYPE_NAME (value));

  if (!value->data[0].v_pointer)
    *tile_p = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *tile_p = value->data[0].v_pointer;
  else
    *tile_p = g_object_ref (value->data[0].v_pointer);
  
  return NULL;
}

void
g_value_set_tile (GValue   *value,
                  gpointer  v_object)
{
  g_return_if_fail (G_VALUE_HOLDS_TILE (value));
  
  if (value->data[0].v_pointer)
    {
      g_object_unref (value->data[0].v_pointer);
      value->data[0].v_pointer = NULL;
    }

  if (v_object)
    {
      g_return_if_fail (GEGL_IS_TILE (v_object));
      g_return_if_fail (g_value_type_compatible (G_OBJECT_TYPE (v_object), G_VALUE_TYPE (value)));

      value->data[0].v_pointer = v_object;
      g_object_ref (value->data[0].v_pointer);
    }
}

void
g_value_set_tile_take_ownership (GValue  *value,
                                 gpointer v_object)
{
  g_return_if_fail (G_VALUE_HOLDS_TILE (value));

  if (value->data[0].v_pointer)
    {
      g_object_unref (value->data[0].v_pointer);
      value->data[0].v_pointer = NULL;
    }

  if (v_object)
    {
      g_return_if_fail (GEGL_IS_TILE (v_object));
      g_return_if_fail (g_value_type_compatible (G_OBJECT_TYPE (v_object), G_VALUE_TYPE (value)));

      value->data[0].v_pointer = v_object; /* we take over the reference count */
    }
}

gpointer
g_value_get_tile (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_TILE (value), NULL);
  
  return value->data[0].v_pointer;
}

GeglTile*
g_value_dup_tile (const GValue *value)
{
  g_return_val_if_fail (G_VALUE_HOLDS_TILE (value), NULL);
  
  return value->data[0].v_pointer ? g_object_ref (value->data[0].v_pointer) : NULL;
}


/**
 * gegl_tile_get_buffer:
 * @self: a #GeglTile
 *
 * Gets a pointer to the buffer.  
 *
 * Returns: the #GeglBuffer for the tile. 
 **/
GeglBuffer * 
gegl_tile_get_buffer (GeglTile * self)
{
  g_return_val_if_fail (self != NULL, (GeglBuffer * )0);
  g_return_val_if_fail (GEGL_IS_TILE (self), (GeglBuffer * )0);
   
  return self->buffer;
}

/**
 * gegl_tile_get_area:
 * @self: a #GeglTile
 * @area: rect to pass back.
 *
 * Gets the area of image space this tile represents.
 *
 **/
void 
gegl_tile_get_area (GeglTile * self, GeglRect * area)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE (self));

  gegl_rect_copy(area,&self->area);
}

void 
gegl_tile_set_area (GeglTile * self, GeglRect * area)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE (self));
 
  gegl_rect_copy(&self->area, area);
}

/**
 * gegl_tile_get_color_model:
 * @self: a #GeglTile
 *
 * Gets the color model of the tile.
 *
 * Returns: the #GeglColorModel of the tile. 
 **/
GeglColorModel * 
gegl_tile_get_color_model (GeglTile * self)
{
  g_return_val_if_fail (self != NULL, (GeglColorModel * )0);
  g_return_val_if_fail (GEGL_IS_TILE (self), (GeglColorModel * )0);

  return self->color_model;
}

void
gegl_tile_set_color_model (GeglTile * self, 
                           GeglColorModel * color_model)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE (self));
  g_return_if_fail (color_model != NULL);
  g_return_if_fail (GEGL_IS_COLOR_MODEL (color_model));

  self->color_model = color_model;

  g_object_ref(self->color_model);
}

/**
 * gegl_tile_get_data:
 * @self: a #GeglTile
 * @data: data pointers to fill in.
 *
 * Makes the passed data pointers point to data in the tile's
 * #GeglDataBuffer.  These will point to data that represents the upper left
 * corner of the tile. ie (area.x,area.y).  
 *
 **/
void 
gegl_tile_get_data (GeglTile * self, 
                    gpointer * data)
{
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_TILE (self));
   {
     gint i;
     gpointer * data_pointers = gegl_buffer_get_data_pointers(self->buffer);
     gint num_channels = gegl_color_model_num_channels(self->color_model);

      /* Copy the data pointers into the passed array */
      for(i=0; i < num_channels; i++)
        data[i] = data_pointers[i];
   }
}

/**
 * gegl_tile_get_data_at:
 * @self: a #GeglTile
 * @data: data pointers to fill in.
 * @x: x in [area.x, area.x + area.w - 1]
 * @y: y in [area.y, area.y + area.h - 1]
 *
 * Makes the passed data pointers point to data in the tile's #GeglDataBuffer
 * at (x,y). This is offset by  x - area.x and y - area.y from upper left
 * corner of the tile. 
 *
 **/
void 
gegl_tile_get_data_at (GeglTile * self, 
                       gpointer * data, 
                       gint x, 
                       gint y)
{
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_TILE (self));
   {
      gint i;
      gint x0 = x - self->area.x;
      gint y0 = y - self->area.y;

      gint num_channels = gegl_color_model_num_channels(self->color_model);
      gint bytes_per_channel = gegl_color_model_bytes_per_channel(self->color_model);
      gint channel_row_bytes = self->area.w * bytes_per_channel;

      /*g_print("get_data_at: %d %d\n", x0,y0); */
      /* Get data pointers for beginning of tile. */
      gegl_tile_get_data(self,data);

      /* Update the data pointers to point to (x,y) position */
      for(i=0; i < num_channels; i++)
        {
          data[i] = (gpointer)((guchar*)data[i] + 
                                y0 * channel_row_bytes + 
                                x0 * bytes_per_channel);
        }
   }
}

static void 
unalloc (GeglTile * self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE (self));

  if (self->color_model)
    {
      g_object_unref(self->color_model);
      self->color_model = NULL;
    }

  if (self->buffer)
    {
      g_object_unref(self->buffer);
      self->buffer = NULL;
    }
}

gboolean 
gegl_tile_alloc (GeglTile * self, 
                 GeglRect * area, 
                 GeglColorModel * color_model)
{
  g_return_val_if_fail (self != NULL, (gboolean )0);
  g_return_val_if_fail (GEGL_IS_TILE (self), (gboolean )0);

  {
    GeglTileMgr * tile_mgr = gegl_tile_mgr_instance();

    g_assert(tile_mgr != NULL);

    /* Get rid of any old data and color model. */
    unalloc(self);

    self->color_model = color_model;
    g_object_ref(color_model);
    gegl_rect_copy(&self->area, area); 

    self->buffer = gegl_tile_mgr_create_buffer(tile_mgr, self); 
                                                     
    g_object_unref(tile_mgr);

    if(!self->buffer)
      return FALSE;

    return TRUE;
  }
}

void 
gegl_tile_validate_data (GeglTile * self)
{
  gpointer * data_pointers = NULL;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_TILE (self));

  data_pointers = gegl_buffer_get_data_pointers(self->buffer);

  if(!data_pointers)
    gegl_buffer_alloc_data(self->buffer);
}
