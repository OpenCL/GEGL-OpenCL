#include "gegl-cache.h"

static void class_init(gpointer g_class,
                       gpointer class_data);
static void instance_init(GTypeInstance *instance,
                          gpointer g_class);
static void get_property(GObject *object,
                         guint property_id,
                         GValue *value,
                         GParamSpec *pspec);
static void set_property(GObject *object,
                         guint property_id,
                         const GValue *value,
                         GParamSpec *pspec);
/* 
 * I am not acutally going to do anything with these properties until
 * I figure out how to handle gsize properties portably
 */
enum {
  PROP_0,
  PROP_SOFT_LIMIT,
  PROP_HARD_LIMIT,
  PROP_PERSISTENT,
  PROP_LAST,
};

GType
gegl_cache_get_type (void)
{
  static GType type=0;
  if (!type)
    {
      static const GTypeInfo typeInfo =
	{
	  /* interface types, classed types, instantiated types */
	  sizeof (GeglCacheClass),
	  NULL, /*base_init*/
	  NULL, /* base_finalize */
	  
	  /* classed types, instantiated types */
	  class_init, /* class_init */
	  NULL, /* class_finalize */
	  NULL, /* class_data */
	  
	  /* instantiated types */
	  sizeof(GeglCache),
	  0, /* n_preallocs */
	  instance_init, /* instance_init */
	  
	  /* value handling */
	  NULL /* value_table */
	};
      
      type = g_type_register_static (G_TYPE_OBJECT ,
				     "GeglCache",
				     &typeInfo,
				     G_TYPE_FLAG_ABSTRACT);
    }
  return type;
}

static void
class_init(gpointer g_class,
	   gpointer class_data)
{
  GeglCacheClass * class = GEGL_CACHE_CLASS(g_class);
  GObjectClass * g_object_class = G_OBJECT_CLASS (g_class);

  class->try_put=NULL;
  class->put = NULL;
  class->fetch = NULL;
  class->mark_as_dirty = NULL;
  class->flush = NULL;

  g_object_class->get_property=get_property;
  g_object_class->set_property=set_property;


  
}

static void
instance_init(GTypeInstance *instance,
	      gpointer g_class)
{
  GeglCache * self = GEGL_CACHE(instance);
  self->persistent=FALSE;
  self->soft_limit=0;
  self->hard_limit=0;
}

static void
get_property(GObject *object,
	     guint property_id,
	     GValue *value,
	     GParamSpec *pspec)
{
  /* FIXME: Figure out how to portably handle a gsize property */
}
static void
set_property(GObject *object,
	     guint property_id,
	     const GValue *value,
	     GParamSpec *pspec)
{
  /* FIXME: Figure out how to portably handle a gsize property */
}

gint
gegl_cache_try_put (GeglCache * cache,
		    GeglCacheEntry * entry,
		    gsize * entry_id)
{
  g_return_val_if_fail (GEGL_IS_CACHE(cache), GEGL_PUT_INVALID);
  g_return_val_if_fail (GEGL_IS_CACHE_ENTRY(entry), GEGL_PUT_INVALID);
  g_return_val_if_fail (entry_id != NULL, GEGL_PUT_INVALID);
  
  GeglCacheClass * class=GEGL_CACHE_GET_CLASS (cache);
  g_return_val_if_fail (class->try_put != NULL, GEGL_PUT_INVALID);
  return class->try_put (cache, entry, entry_id);
}
gint
gegl_cache_put (GeglCache * cache,
		GeglCacheEntry * entry,
		gsize * entry_id)
{
  g_return_val_if_fail (GEGL_IS_CACHE(cache), GEGL_PUT_INVALID);
  g_return_val_if_fail (GEGL_IS_CACHE_ENTRY(entry), GEGL_PUT_INVALID);
  g_return_val_if_fail (entry_id != NULL, GEGL_PUT_INVALID);
  
  GeglCacheClass * class=GEGL_CACHE_GET_CLASS (cache);
  g_return_val_if_fail (class->put != NULL, GEGL_PUT_INVALID);
  return class->put (cache, entry, entry_id);
}
gint
gegl_cache_fetch (GeglCache * cache,
		  gsize entry_id,
		  GeglCacheEntry ** entry)
{
  g_return_val_if_fail (GEGL_IS_CACHE(cache), GEGL_FETCH_INVALID);
  
  GeglCacheClass * class=GEGL_CACHE_GET_CLASS (cache);
  g_return_val_if_fail (class->fetch != NULL, GEGL_FETCH_INVALID);
  return class->fetch (cache, entry_id, entry);
}
void gegl_cache_mark_as_dirty (GeglCache * cache,
			       gsize entry_id)
{
  g_return_if_fail (GEGL_IS_CACHE(cache));
  GeglCacheClass * class=GEGL_CACHE_GET_CLASS (cache);
  g_return_if_fail (class->mark_as_dirty != NULL);
  class->mark_as_dirty(cache, entry_id);
}
void gegl_cache_flush (GeglCache * cache,
		       gsize entry_id)
{
  g_return_if_fail (GEGL_IS_CACHE(cache));
  GeglCacheClass * class=GEGL_CACHE_GET_CLASS (cache);
  g_return_if_fail (class->flush != NULL);
  class->flush (cache, entry_id);
}
