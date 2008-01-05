#include "gegl-plugin.h"
#include "gegl-module.h"
#include "operation/gegl-operation-filter.h"

typedef struct Generated      GeglOperationRemap;
typedef struct GeneratedClass GeglOperationRemapClass;

struct Generated
{
  GeglOperationFilter parent_instance;
  GeglBuffer *low;
  GeglBuffer *high;
  gpointer   *priv;
};

struct GeneratedClass
{
  GeglOperationFilterClass parent_class;
};

static void gegl_operation_remap_init (GeglOperationRemap *self);
static void gegl_operation_remap_class_init (GeglOperationRemapClass *klass);
GType remap_get_type (GTypeModule *module);
const GeglModuleInfo * gegl_module_query (GTypeModule *module);
gboolean gegl_module_register (GTypeModule *module);
static gpointer gegl_operation_remap_parent_class = ((void *)0);

static void gegl_operation_remap_class_intern_init (gpointer klass) {
  gegl_operation_remap_parent_class = g_type_class_peek_parent (klass);
  gegl_operation_remap_class_init ((GeglOperationRemapClass*) klass);
}

GType remap_get_type (GTypeModule *module) {
  static GType g_define_type_id = 0;
  if (G_UNLIKELY (g_define_type_id == 0)) {
    static const GTypeInfo g_define_type_info = {
      sizeof (GeglOperationRemapClass), (GBaseInitFunc) ((void *)0), (GBaseFinalizeFunc) ((void *)0), (GClassInitFunc) gegl_operation_remap_class_intern_init, (GClassFinalizeFunc) ((void *)0), ((void *)0), sizeof (GeglOperationRemap), 0, (GInstanceInitFunc) gegl_operation_remap_init, ((void *)0) };
      g_define_type_id = gegl_module_register_type (module, GEGL_TYPE_OPERATION_FILTER, "GeglOpPlugIn-""remap", &g_define_type_info, (GTypeFlags) 0); }
      return g_define_type_id;
}

static const GeglModuleInfo modinfo = {
  GEGL_MODULE_ABI_VERSION, "remap", "v0.0", "(c) 2006, released under the LGPL", "June 2006"};
  const GeglModuleInfo *gegl_module_query (GTypeModule *module){ return &modinfo;}
  
  gboolean gegl_module_register (GTypeModule *module){
    remap_get_type (module);
    return TRUE;
}

enum
{
  PROP_0,
  PROP_low,
  PROP_high,
  PROP_LAST
};

static void
get_property (GObject *gobject,
              guint property_id,
              GValue *value,
              GParamSpec *pspec)
{
  GeglOperationRemap *self = ((GeglOperationRemap*)(gobject));

  switch (property_id)
  {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
  self = ((void *)0);
}

static void
set_property (GObject *gobject,
              guint property_id,
              const GValue *value,
              GParamSpec *pspec)
{
  GeglOperationRemap *self = ((GeglOperationRemap*)(gobject));

  switch (property_id)
  {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
  self = ((void *)0);
}
static gboolean process (GeglOperation *operation,
                         GeglNodeContext *context,
                         const GeglRectangle *result);

static void
gegl_operation_remap_init (GeglOperationRemap *self)
{
  self->priv = ((void *)0);
}

static void gegl_operation_remap_destroy_notify (gpointer data)
{
  GeglOperationRemap *self = ((GeglOperationRemap*)(data));
if (self->low) { g_object_unref (self->low); self->low = ((void *)0); }
if (self->high) { g_object_unref (self->high); self->high = ((void *)0); }
  self = ((void *)0);
}

static GObject *
gegl_operation_remap_constructor (GType type,
                        guint n_construct_properties,
                        GObjectConstructParam *construct_properties)
{
  GObject *obj;

  obj = G_OBJECT_CLASS (gegl_operation_remap_parent_class)->constructor (
            type, n_construct_properties, construct_properties);





  g_object_set_data_full (obj, "chant-data", obj, gegl_operation_remap_destroy_notify);

  return obj;
}

static gboolean
process (GeglOperation *operation,
         GeglNodeContext *context,
         const GeglRectangle *result)
{
  GeglOperationFilter *filter;
  GeglOperationRemap *self;
  GeglBuffer *input;
  GeglBuffer *low;
  GeglBuffer *high;
  GeglBuffer *output;

  filter = GEGL_OPERATION_FILTER (operation);
  self = ((GeglOperationRemap*)(operation));

  
  input = gegl_node_context_get_source (context, "input");
  low = gegl_node_context_get_source (context, "low");
  high = gegl_node_context_get_source (context, "high");

    {
      gfloat *buf;
      gfloat *min;
      gfloat *max;
      gint pixels = result->width  * result->height;
      gint i;

      buf = g_malloc (pixels * sizeof (gfloat) * 4);
      min = g_malloc (pixels * sizeof (gfloat) * 3);
      max = g_malloc (pixels * sizeof (gfloat) * 3);

      gegl_buffer_get (input, 1.0, result, babl_format ("RGBA float"), buf, GEGL_AUTO_ROWSTRIDE);
      gegl_buffer_get (low,   1.0, result, babl_format ("RGB float"), min, GEGL_AUTO_ROWSTRIDE);
      gegl_buffer_get (high,  1.0, result, babl_format ("RGB float"), max, GEGL_AUTO_ROWSTRIDE);

      output = gegl_node_context_get_target (context, "output");

      for (i=0;i<pixels;i++)
        {
          gint c;
          for (c=0;c<3;c++) 
            {
              gfloat delta = max[i*3+c]-min[i*3+c];
              if (delta > 0.0001 || delta < -0.0001)
                {
                  buf[i*4+c] = (buf[i*4+c]-min[i*3+c]) / delta;
                }
              /*else
                buf[i*4+c] = buf[i*4+c];*/
            } 
        }

      gegl_buffer_set (output, result, babl_format ("RGBA float"), buf,
                       GEGL_AUTO_ROWSTRIDE);

      g_free (buf);
      g_free (min);
      g_free (max);

    }
  return TRUE;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_defined_region (operation,
                                                                     "input");
  if (!in_rect)
    return result;

  result = *in_rect;

  return result;
}

static GeglRectangle
compute_input_request (GeglOperation       *operation,
                       const gchar         *input_pad,
                       const GeglRectangle *roi)
{
    return *roi;
}

static GeglRectangle
compute_affected_region (GeglOperation *self,
                         const gchar *input_pad,
                         GeglRectangle region)
{
  return region;
}


static void
attach (GeglOperation *self)
{
  GeglOperation  *operation    = GEGL_OPERATION (self);
  GObjectClass   *object_class = G_OBJECT_GET_CLASS (self);

  gegl_operation_create_pad (operation,
                               g_object_class_find_property (object_class,
                                                             "output"));
  gegl_operation_create_pad (operation,
                               g_object_class_find_property (object_class,
                                                             "input"));

  gegl_operation_create_pad (operation,
                               g_object_class_find_property (object_class,
                                                             "low"));
  gegl_operation_create_pad (operation,
                               g_object_class_find_property (object_class,
                                                             "high"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle
compute_input_request (GeglOperation       *operation,
                       const gchar         *input_pad,
                       const GeglRectangle *roi);

static void
gegl_operation_remap_class_init (GeglOperationRemapClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class;


  GeglOperationFilterClass *parent_class = GEGL_OPERATION_FILTER_CLASS (klass);
  parent_class->process = process;

  operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  object_class->constructor = gegl_operation_remap_constructor;

  operation_class->get_defined_region = get_defined_region;
  operation_class->compute_affected_region = compute_affected_region;
  operation_class->compute_input_request = compute_input_request;
  operation_class->attach = attach;

  gegl_operation_class_set_name (operation_class, "remap");;


  operation_class->description = "remaps the pixel value, so that min would become 0.0 and max become 1.0, (only affecting the input pixel).";

  operation_class->categories = "programming:hidden";

g_object_class_install_property (object_class, PROP_low, g_param_spec_object ("low", "low", "low buffer", G_TYPE_OBJECT, (GParamFlags) ( G_PARAM_READWRITE | GEGL_PAD_INPUT)));
g_object_class_install_property (object_class, PROP_high, g_param_spec_object ("high", "high", "high buffer", G_TYPE_OBJECT, (GParamFlags) ( G_PARAM_READWRITE | GEGL_PAD_INPUT)));


}

