#include "gegl-visitor.h"
#include "gegl-node.h"
#include "gegl-op.h"
#include "gegl-filter.h"

static void class_init (GeglVisitorClass * klass);
static void init (GeglVisitor * self, GeglVisitorClass * klass);
static void finalize(GObject *gobject);

static void visit_node(GeglVisitor * self, GeglNode *node);

static gpointer parent_class = NULL;

GType
gegl_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglVisitor", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglVisitorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;

  klass->visit_node = visit_node; 
  klass->visit_op = NULL; 
  klass->visit_filter = NULL; 
}

static void 
init (GeglVisitor * self, 
      GeglVisitorClass * klass)
{
  self->visits_list = NULL;
}

static void
finalize(GObject *gobject)
{  
  GeglVisitor * self = GEGL_VISITOR(gobject);
  gint length = g_list_length(self->visits_list);

  if(length > 0)
    {
      gint i;
      for(i = 0; i < length; i++)
        g_free((gchar*)g_list_nth_data(self->visits_list, i));

      g_list_free(self->visits_list);
    }
     
  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

GList *
gegl_visitor_get_visits_list(GeglVisitor *self)
{
  return self->visits_list;
}

void      
gegl_visitor_visit_node(GeglVisitor * self,
                        GeglNode *node)
{
  GeglVisitorClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_VISITOR (self));
  g_return_if_fail (node != NULL);
  g_return_if_fail (GEGL_IS_NODE(node));

  klass = GEGL_VISITOR_GET_CLASS(self);

  if(klass->visit_node)
    (*klass->visit_node)(self, node);
}

static void      
visit_node(GeglVisitor * self,
           GeglNode *node)
{
 gchar* name = (gchar*)gegl_object_get_name(GEGL_OBJECT(node));
 self->visits_list = g_list_append(self->visits_list, name);
}

void      
gegl_visitor_visit_op(GeglVisitor * self,
                      GeglOp *op)
{
  GeglVisitorClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_VISITOR (self));
  g_return_if_fail (op != NULL);
  g_return_if_fail (GEGL_IS_OP(op));

  klass = GEGL_VISITOR_GET_CLASS(self);

  if(klass->visit_op)
    (*klass->visit_op)(self, op);
}

void      
gegl_visitor_visit_filter(GeglVisitor * self,
                          GeglFilter *filter)
{
  GeglVisitorClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_VISITOR (self));
  g_return_if_fail (filter != NULL);
  g_return_if_fail (GEGL_IS_FILTER(filter));

  klass = GEGL_VISITOR_GET_CLASS(self);

  if(klass->visit_filter)
    (*klass->visit_filter)(self, filter);
}
