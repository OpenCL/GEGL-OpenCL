#include "gegl-plugin.h"
#include "gegl-module.h"
#include "gegl-operation-filter.h"

typedef struct Generated GeglOperationRemap;
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
                         gpointer dynamic_id);




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





static GeglRectangle get_source_rect (GeglOperation *self,
                                      gpointer dynamic_id);


static gboolean
process (GeglOperation *operation,
         gpointer dynamic_id)
{
  GeglOperationFilter *filter;
  GeglOperationRemap *self;
  GeglBuffer *input;
  GeglBuffer *low;
  GeglBuffer *high;
  GeglBuffer *output;

  filter = GEGL_OPERATION_FILTER (operation);
  self = ((GeglOperationRemap*)(operation));

  
  input = GEGL_BUFFER (gegl_operation_get_data (operation, dynamic_id, "input"));
  low = GEGL_BUFFER (gegl_operation_get_data (operation, dynamic_id, "low"));
  high = GEGL_BUFFER (gegl_operation_get_data (operation, dynamic_id, "high"));

    {
      GeglRectangle *result = gegl_operation_result_rect (operation, dynamic_id);

      if (result->w==0 ||
          result->h==0)
        {
          output = g_object_ref (input);
        }
      else
        {
          gfloat *buf;
          gfloat *min;
          gfloat *max;
          gint pixels = result->w * result->h;
          gint i;

          buf = g_malloc (pixels * sizeof (gfloat) * 4);
          min = g_malloc (pixels * sizeof (gfloat));
          max = g_malloc (pixels * sizeof (gfloat));

          gegl_buffer_get (input, result, buf, babl_format ("RGBA float"), 1.0);
          gegl_buffer_get (low,   result, min, babl_format ("Y float"), 1.0);
          gegl_buffer_get (high,  result, max, babl_format ("Y float"), 1.0);

          output = g_object_new (GEGL_TYPE_BUFFER,
                                 "format", babl_format ("RGBA float"),
                                 "x", result->x,
                                 "y", result->y,
                                 "width", result->w,
                                 "height", result->h,
                                 NULL);

          for (i=0;i<pixels;i++)
            {
              gint c;
              gfloat delta = max[i]-min[i];
             
              for (c=0;c<3;c++)
                {
                  if (delta > 0.0001 || delta < -0.0001)
                    buf[i*4+c] = (buf[i*4+c]-min[i]) / delta;
                  else
                    buf[i*4+c] = 1.0-buf[i*4+c];
                } 
            }

          gegl_buffer_set (output, result, buf, babl_format ("RGBA float"));

          g_free (buf);
          g_free (min);
          g_free (max);
        }

      gegl_operation_set_data (operation, dynamic_id, "output", G_OBJECT (output));
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

static GeglRectangle get_source_rect (GeglOperation *self,
                                      gpointer dynamic_id)
{
  GeglRectangle rect;

  rect = *gegl_operation_get_requested_region (self, dynamic_id);

  return rect;
}

static gboolean
calc_source_regions (GeglOperation *self,
                     gpointer dynamic_id)
{
  GeglRectangle need = get_source_rect (self, dynamic_id);

  gegl_operation_set_source_region (self, dynamic_id, "input", &need);
  gegl_operation_set_source_region (self, dynamic_id, "low", &need);
  gegl_operation_set_source_region (self, dynamic_id, "high", &need);

  return TRUE;
}

static GeglRectangle
get_affected_region (GeglOperation *self,
                     const gchar *input_pad,
                     GeglRectangle region)
{
  return region;
}


static void
associate (GeglOperation *self)
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
}



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
  operation_class->get_affected_region = get_affected_region;
  operation_class->calc_source_regions = calc_source_regions;
  operation_class->associate = associate;

  gegl_operation_class_set_name (operation_class, "remap");;


  gegl_operation_class_set_description (operation_class, "remaps the pixel value, so that min would become 0.0 and max become 1.0, (only affecting the input pixel).");



  operation_class->categories = "misc";



g_object_class_install_property (object_class, PROP_low, g_param_spec_object ("low", "low", "low buffer", G_TYPE_OBJECT, (GParamFlags) ( G_PARAM_READWRITE | GEGL_PAD_INPUT)));
g_object_class_install_property (object_class, PROP_high, g_param_spec_object ("high", "high", "high buffer", G_TYPE_OBJECT, (GParamFlags) ( G_PARAM_READWRITE | GEGL_PAD_INPUT)));


}

