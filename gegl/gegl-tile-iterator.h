#ifndef __GEGL_TILE_ITERATOR_H__
#define __GEGL_TILE_ITERATOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "gegl-object.h"

#ifndef __TYPEDEF_GEGL_TILE__
#define __TYPEDEF_GEGL_TILE__
typedef struct _GeglTile GeglTile;
#endif

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__ 
typedef struct _GeglColorModel  GeglColorModel;
#endif

#define GEGL_TYPE_TILE_ITERATOR               (gegl_tile_iterator_get_type ())
#define GEGL_TILE_ITERATOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_ITERATOR, GeglTileIterator))
#define GEGL_TILE_ITERATOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_ITERATOR, GeglTileIteratorClass))
#define GEGL_IS_TILE_ITERATOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_ITERATOR))
#define GEGL_IS_TILE_ITERATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_ITERATOR))
#define GEGL_TILE_ITERATOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_ITERATOR, GeglTileIteratorClass))

#ifndef __TYPEDEF_GEGL_TILE_ITERATOR__
#define __TYPEDEF_GEGL_TILE_ITERATOR__
typedef struct _GeglTileIterator GeglTileIterator;
#endif
struct _GeglTileIterator 
{
   GeglObject object;

   /*< private >*/
   GeglTile * tile;
   GeglRect rect;
   gint current;
};

typedef struct _GeglTileIteratorClass GeglTileIteratorClass;
struct _GeglTileIteratorClass 
{
   GeglObjectClass object_class;
};

GType                 gegl_tile_iterator_get_type          (void);
GeglTile *            gegl_tile_iterator_get_tile          (GeglTileIterator * self);
void                  gegl_tile_iterator_get_rect          (GeglTileIterator * self, GeglRect * rect);
GeglColorModel *      gegl_tile_iterator_get_color_model   (GeglTileIterator * self);
void                  gegl_tile_iterator_first             (GeglTileIterator * self);
void                  gegl_tile_iterator_next              (GeglTileIterator * self);
gboolean              gegl_tile_iterator_is_done           (GeglTileIterator * self);
void                  gegl_tile_iterator_get_current       (GeglTileIterator * self,
                                                            gpointer * data_pointers);

/* protected */
void                  gegl_tile_iterator_set_rect (GeglTileIterator * self, GeglRect * rect);
void                  gegl_tile_iterator_set_tile (GeglTileIterator * self, GeglTile * tile);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
