#include "gegl-eval-mgr.h"
#include "gegl-node.h"
#include "gegl-op.h"
#include "gegl-image-data.h"
#include "gegl-param-specs.h"
#include "gegl-visitor.h"
#include "gegl-eval-bfs-visitor.h"
#include "gegl-eval-dfs-visitor.h"
#include "gegl-eval-visitor.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_ROOT, 
  PROP_ROI, 
  PROP_LAST 
};

static void class_init (GeglEvalMgrClass * klass);
static void init (GeglEvalMgr * self, GeglEvalMgrClass * klass);
static void finalize(GObject * gobject);

static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *param_spec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *param_spec);

static void evaluate (GeglEvalMgr * self);
static void eval_bfs_visitor(GeglEvalMgr *self); 
static void eval_dfs_visitor(GeglEvalMgr *self); 
static void eval_visitor(GeglEvalMgr *self); 

static gpointer parent_class = NULL;

GType
gegl_eval_mgr_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglEvalMgrClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglEvalMgr),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT , 
                                     "GeglEvalMgr", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglEvalMgrClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  klass->evaluate = evaluate;

  g_object_class_install_property (gobject_class, PROP_ROOT,
                                   g_param_spec_object ("root",
                                                        "root",
                                                        "Root node for this eval_mgr",
                                                         GEGL_TYPE_NODE,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_ROI,
                                   g_param_spec_pointer ("roi",
                                                        "roi",
                                                        "Region of interest for eval_mgr",
                                                         G_PARAM_READWRITE));

  return;
}

static void 
init (GeglEvalMgr * self, 
      GeglEvalMgrClass * klass)
{
  self->root = NULL;
  gegl_rect_set(&self->roi,0,0,GEGL_DEFAULT_WIDTH,GEGL_DEFAULT_HEIGHT);

  return;
}

static void
finalize(GObject *gobject)
{  
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglEvalMgr * eval_mgr = GEGL_EVAL_MGR(gobject);
  switch (prop_id)
  {
    case PROP_ROOT:
        gegl_eval_mgr_set_root(eval_mgr, (GeglNode*)g_value_get_object(value));  
      break;
    case PROP_ROI:
        gegl_eval_mgr_set_roi (eval_mgr, (GeglRect*)g_value_get_pointer (value));
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
  GeglEvalMgr * eval_mgr = GEGL_EVAL_MGR(gobject);
  switch (prop_id)
  {
    case PROP_ROOT:
      g_value_set_object(value, (GObject*)gegl_eval_mgr_get_root(eval_mgr));  
      break;
    case PROP_ROI:
      gegl_eval_mgr_get_roi (eval_mgr, (GeglRect*)g_value_get_pointer (value));
      break;
    default:
      break;

  }
}

/**
 * gegl_eval_mgr_get_root:
 * @self: a #GeglEvalMgr.
 *
 * Gets the root node of this eval_mgr.
 *
 * Returns: a #GeglNode, the root of this eval_mgr.
 **/
GeglNode* 
gegl_eval_mgr_get_root (GeglEvalMgr * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_EVAL_MGR (self), NULL);

  return self->root;
}

/**
 * gegl_eval_mgr_set_root:
 * @self: a #GeglEvalMgr.
 * @root: a #GeglNode.
 *
 * Sets the root node of this eval_mgr.
 *
 **/
void 
gegl_eval_mgr_set_root (GeglEvalMgr * self,
                      GeglNode *root)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_EVAL_MGR (self));
  self->root = root;
}

/**
 * gegl_eval_mgr_get_roi:
 * @self: a #GeglEvalMgr.
 * @roi: a #GeglRect.
 *
 * Gets the roi of this eval_mgr.
 *
 **/
void
gegl_eval_mgr_get_roi(GeglEvalMgr *self, 
                    GeglRect *roi)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_EVAL_MGR (self));

  gegl_rect_copy(roi, &self->roi);
}

/**
 * gegl_eval_mgr_set_roi:
 * @self: a #GeglEvalMgr.
 * @roi: a #GeglRect.
 *
 * Sets the roi of this eval_mgr.
 *
 **/
void
gegl_eval_mgr_set_roi(GeglEvalMgr *self, 
                    GeglRect *roi)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_EVAL_MGR (self));

  if(roi)
    gegl_rect_copy(&self->roi, roi);
  else
    gegl_rect_set(&self->roi,0,0,GEGL_DEFAULT_WIDTH,GEGL_DEFAULT_HEIGHT);
}

/**
 * gegl_eval_mgr_evaluate:
 * @self: a #GeglEvalMgr.
 *
 * Executes this eval .
 *
 **/
void 
gegl_eval_mgr_evaluate(GeglEvalMgr *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_EVAL_MGR (self));

  {
    GeglEvalMgrClass *klass = GEGL_EVAL_MGR_GET_CLASS(self);

    if(klass->evaluate)
        (*klass->evaluate)(self);
  }
}

static void      
evaluate (GeglEvalMgr * self) 
{
  /* Get the root and set roi on it */
  GeglData * data = gegl_op_get_data_output(GEGL_OP(self->root), 0);
  if(data && GEGL_IS_IMAGE_DATA(data))
    {
      GeglImageData *image_data = GEGL_IMAGE_DATA(data);
      gegl_image_data_set_rect(image_data, &self->roi);
      //gegl_rect_copy(&image_data->rect, &self->roi);
    }

  /* This part computes need rects, breadth first. */
  LOG_DEBUG("evaluate", 
            "begin bfs for %s %p", 
            G_OBJECT_TYPE_NAME(self->root), 
            self);
  eval_bfs_visitor(self);

  /* This part computes have rects, color models, depth first. */
  LOG_DEBUG("evaluate", 
            "begin dfs for %s %p", 
            G_OBJECT_TYPE_NAME(self->root), 
            self);
  eval_dfs_visitor(self);

  /* This part does the evaluation of the ops, depth first. */
  LOG_DEBUG("evaluate", 
            "begin evaluate dfs for %s %p", 
            G_OBJECT_TYPE_NAME(self->root), 
            self);
  eval_visitor(self);
}

static void 
eval_bfs_visitor(GeglEvalMgr *self) 
{
  GeglEvalBfsVisitor * eval_bfs_visitor = 
    g_object_new(GEGL_TYPE_EVAL_BFS_VISITOR, NULL); 
  gegl_bfs_visitor_traverse(GEGL_BFS_VISITOR(eval_bfs_visitor), 
                            GEGL_NODE(self->root));
  g_object_unref(eval_bfs_visitor);
}

static void 
eval_dfs_visitor(GeglEvalMgr *self) 
{
  GeglEvalDfsVisitor * eval_dfs_visitor = 
    g_object_new(GEGL_TYPE_EVAL_DFS_VISITOR, NULL); 
  gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(eval_dfs_visitor), 
                            GEGL_NODE(self->root));
  g_object_unref(eval_dfs_visitor);
}

static void 
eval_visitor(GeglEvalMgr *self) 
{
  GeglEvalVisitor * eval_visitor = 
    g_object_new(GEGL_TYPE_EVAL_VISITOR, NULL); 
  gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(eval_visitor), 
                            GEGL_NODE(self->root));
  g_object_unref(eval_visitor);
}
