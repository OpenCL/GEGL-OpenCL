#include "gegl-dump-visitor.h"
#include "gegl-node.h"
#include "gegl-op.h"
#include "gegl-filter.h"
#include "gegl-graph.h"
#include "gegl-image-data.h"
#include "gegl-image.h"
#include "gegl-color-model.h"
#include "gegl-color-space.h"
#include "gegl-data-space.h"
#include <stdio.h> 

static void class_init (GeglDumpVisitorClass * klass);
static void init (GeglDumpVisitor * self, GeglDumpVisitorClass * klass);

static gchar * make_tabs(gint num); 
static GString* attributes_string(GeglOp *op);

static void visit_node (GeglVisitor *visitor, GeglNode * node);
static void visit_filter (GeglVisitor *visitor, GeglFilter * filter);
static void visit_graph (GeglVisitor *visitor, GeglGraph * graph);

static gpointer parent_class = NULL;

GType
gegl_dump_visitor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglDumpVisitorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglDumpVisitor),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_VISITOR, 
                                     "GeglDumpVisitor", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglDumpVisitorClass * klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  visitor_class->visit_node = visit_node;
  visitor_class->visit_filter = visit_filter;
  visitor_class->visit_graph = visit_graph;
}

static void 
init (GeglDumpVisitor * self, 
      GeglDumpVisitorClass * klass)
{
  self->tabs = 0;
}

static gchar *
make_tabs(gint num) 
{
  gint i;
  gchar *spaces = g_new(gchar, 2 * num + 1);
  gchar *ptr = spaces;

  for(i=0; i < 2*num; i++, ptr++)
    *ptr = ' ';

  *ptr = '\0';

  return spaces;
}

static GString* 
attributes_string(GeglOp *op)
{
  GeglRect rect = {0,0,0,0};
  char *color_model_name = "None";
  GObject* image_data_value = NULL;

  GeglAttributes * attributes = gegl_op_get_attributes(op); 

  /*
      attr addr [value (x,y,w,h) cm] 
  */

  GString *string = g_string_new("");

  if(attributes)
    {
      image_data_value = g_value_get_object(attributes->value);
      gegl_rect_copy(&rect, &attributes->rect);
      if(GEGL_IS_COLOR_MODEL(attributes->color_model))
        color_model_name = gegl_color_model_name(attributes->color_model);
    }

  g_string_printf(string, 
                  "attr %p [%p (%d,%d,%d,%d) %s]",
                  attributes,
                  image_data_value,
                  rect.x, rect.y, rect.w, rect.h,
                  color_model_name);

  return string;
} 

void
gegl_dump_visitor_traverse(GeglDumpVisitor * self, 
                           GeglNode * node)
{
  gint num_inputs;
  g_return_if_fail (self);
  g_return_if_fail (GEGL_IS_DUMP_VISITOR (self));
  g_return_if_fail (node);
  g_return_if_fail (GEGL_IS_NODE (node));

  gegl_node_accept(node, GEGL_VISITOR(self));

  num_inputs = gegl_node_get_num_inputs(node);
  if(num_inputs > 0)
    {
      gint i;
      self->tabs++;
      for(i=0; i < num_inputs; i++)
        {
          GeglNode *source = gegl_node_get_source(node, i);

          if(source) 
            gegl_dump_visitor_traverse(self, source);
          else
            {
              gchar * spaces = make_tabs(self->tabs);
              LOG_DIRECT("%s[NULL]", spaces);
              g_free(spaces);
            }
        }

      self->tabs--;
    }
}

static void      
visit_node(GeglVisitor * visitor,
           GeglNode *node)
{
  gchar * spaces;
  GeglDumpVisitor *self = GEGL_DUMP_VISITOR(visitor);
  GEGL_VISITOR_CLASS(parent_class)->visit_node(visitor, node);

  spaces = make_tabs(self->tabs);

  LOG_DIRECT("%s%s %s %p", 
             spaces,   
             G_OBJECT_TYPE_NAME(node), 
             gegl_object_get_name(GEGL_OBJECT(node)), 
             node);

  g_free(spaces);
}

static void      
visit_filter(GeglVisitor * visitor,
             GeglFilter *filter)
{
  gchar * spaces;
  GeglDumpVisitor * self = GEGL_DUMP_VISITOR(visitor);
  GString *attrs_string = attributes_string(GEGL_OP(filter));

  GEGL_VISITOR_CLASS(parent_class)->visit_filter(GEGL_VISITOR(self), filter);

  spaces = make_tabs(self->tabs);

  if(GEGL_IS_IMAGE(filter)) 
  {
    GeglImageData * image_data = gegl_image_get_image_data(GEGL_IMAGE(filter));
    GeglColorModel * image_data_colormodel = image_data ? gegl_image_data_get_color_model(image_data) : NULL;
    gchar * image_data_colormodel_name = image_data_colormodel ? 
       g_strdup(gegl_color_model_name(image_data_colormodel)):
       g_strdup("None");

    /* 
       name typename addr attr addr [value (x,y,w,h) cm] image_data addr cm 
    */

    LOG_DIRECT("%s%s %s %p image_data %p colormodel %s",
               spaces,   
               G_OBJECT_TYPE_NAME(filter), 
               gegl_object_get_name(GEGL_OBJECT(filter)), 
               filter,
               image_data, 
               image_data_colormodel_name);

    LOG_DIRECT("%s        (%s)",
               spaces,   
               attrs_string->str);

    g_free(image_data_colormodel_name);
  }
  else 
  {
    /* 
       name typename addr attr addr [value (x,y,w,h) cm]
    */
    LOG_DIRECT("%s%s %s %p",
               spaces,   
               G_OBJECT_TYPE_NAME(filter), 
               gegl_object_get_name(GEGL_OBJECT(filter)), 
               filter);

    LOG_DIRECT("%s    (%s)",
               spaces,   
               attrs_string->str);
  }

  g_string_free(attrs_string, TRUE);
  g_free(spaces);
}


static void      
visit_graph(GeglVisitor * visitor,
            GeglGraph *graph)
{
  GeglDumpVisitor * self = GEGL_DUMP_VISITOR(visitor);
  GeglGraph *prev_graph = visitor->graph;
  gchar * spaces;
  GString *attrs_string = attributes_string(GEGL_OP(graph));

  spaces = make_tabs(self->tabs);

  LOG_DIRECT("%s%s %s %p",
             spaces,   
             G_OBJECT_TYPE_NAME(graph), 
             gegl_object_get_name(GEGL_OBJECT(graph)), 
             graph);

  LOG_DIRECT("%s        (%s)",
             spaces,   
             attrs_string->str);

  g_string_free(attrs_string, TRUE);

  LOG_DIRECT("%s{", spaces);
  self->tabs++;
  visitor->graph = graph;
  gegl_dump_visitor_traverse(self, GEGL_NODE(graph->root));
  visitor->graph = prev_graph; 
  self->tabs--;
  LOG_DIRECT("%s}", spaces);

  g_free(spaces);
}
