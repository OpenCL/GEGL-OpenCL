#include "gegl-image-mgr.h"
#include "gegl-object.h"
#include "gegl-node.h"
#include "gegl-op.h"
#include "gegl-buffer.h"
#include "gegl-sampled-image.h"
#include "gegl-tile-mgr.h"

static void class_init (GeglImageMgrClass * klass);
static void finalize(GObject *gobject);

static gpointer parent_class = NULL;
static GeglImageMgr *image_mgr_singleton = NULL;

void 
gegl_image_mgr_install(GeglImageMgr *image_mgr)
{
  g_return_if_fail (image_mgr != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_MGR (image_mgr));

  /* Install this image manager */
  image_mgr_singleton = image_mgr;
} 

GeglImageMgr *
gegl_image_mgr_instance()
{
  if(!image_mgr_singleton)
    {
      g_warning("No ImageMgr installed.");
      return NULL;
    }

  g_object_ref(image_mgr_singleton);
  return image_mgr_singleton;
} 

GType
gegl_image_mgr_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglImageMgrClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglImageMgr),
        0,
        (GInstanceInitFunc) NULL,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglImageMgr", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglImageMgrClass * klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  g_object_class->finalize = finalize;

  klass->apply = NULL;
  klass->delete_image = NULL;
  klass->add_image = NULL;

  return;
}

static void
finalize(GObject *gobject)
{
  GeglImageMgr *self = GEGL_IMAGE_MGR(gobject);

  g_object_unref(self->tile_mgr);
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

/**
 * gegl_image_mgr_apply:
 * @self: a #GeglImageMgr
 * @root: root node of graph.
 * @dest: a destination #GeglSampledImage for result.
 * @roi: a region of interest.
 *
 * This evaluates an operator graph, when @root is a #GeglImage. Subclass
 * image managers should implement this to traverse the graph and call each
 * #GeglOp's prepare, process, and finish routines.
 *
 **/
void 
gegl_image_mgr_apply (GeglImageMgr * self, 
                          GeglOp * root, 
                          GeglSampledImage * dest, 
                          GeglRect * roi)
{
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_IMAGE_MGR (self));
   {
     GeglImageMgrClass *klass =  GEGL_IMAGE_MGR_GET_CLASS(self);
     (*klass->apply)(self,root,dest,roi);
   }
}

/**
 * gegl_image_mgr_add_image:
 * @self: a #GeglImageMgr
 * @image: a #GeglImage to add.
 *
 *
 * Inform the ImageMgr that an image has been added. 
 * Called from #GeglImage's constructor.
 *
 **/
void 
gegl_image_mgr_add_image (GeglImageMgr * self, 
                              GeglImageImpl * image)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_MGR (self));
  {
    GeglImageMgrClass *klass = GEGL_IMAGE_MGR_GET_CLASS(self);
    (*klass->add_image)(self,image);
  }
}


/**
 * gegl_image_mgr_delete_image:
 * @self: a #GeglImageMgr
 * @image: a #GeglImage to delete from memory.
 *
 * Inform the ImageMgr that an image has been deleted. 
 * Called from #GeglImage's destroy.
 *
 **/
void 
gegl_image_mgr_delete_image (GeglImageMgr * self, 
                                 GeglImageImpl * image)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_MGR (self));
  {
    GeglImageMgrClass *klass = GEGL_IMAGE_MGR_GET_CLASS(self);
    (*klass->delete_image)(self,image);
  }
}

GeglTileMgr *
gegl_image_mgr_get_tile_mgr(GeglImageMgr *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_MGR (self), NULL);

  return self->tile_mgr;
}
