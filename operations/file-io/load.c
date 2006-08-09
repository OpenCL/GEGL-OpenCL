/* TODO: maybe this operation should be folded into gegl-core, and
 * allow for other load ops to register as the handlers */

#ifdef CHANT_SELF
chant_string (path, "/tmp/gegl-logo.svg", "Path to image file to load.")
chant_pointer (cached, "private")
#else

#define CHANT_SOURCE
#define CHANT_NAME            load
#define CHANT_DESCRIPTION     "Multipurpose file loader, that uses other " \
                              "native handlers, and fallback conversion " \
                              "using image magick's convert."

#define CHANT_SELF            "load.c"
#define CHANT_CLASS_CONSTRUCT
#include "gegl-chant.h"
#include <stdio.h>

typedef struct LoaderMapping
{
  gchar *extension;
  gchar *handler;
} LoaderMapping;

static LoaderMapping mappings[]=
{
  {".jpg",  "jpg-load"},
  {".jpeg", "jpg-load"},
  {".JPG",  "jpg-load"},
  {".JPEG", "jpg-load"},
  {".png",  "png-load"},
  {".PNG",  "png-load"},
  {".raw",  "raw-load"},
  {".RAW",  "raw-load"},
  {".raf",  "raw-load"},
  {NULL,    "magick-load"}
};
  
static void
load_cache (ChantInstance *op_load)
{
  if (!op_load->cached)
    {
        GeglNode *temp_gegl;
        gchar xml[1024]="";
        LoaderMapping *map = &mappings[0];

        while (map->extension)
          {
            if (strstr (op_load->path, map->extension))
              {
                sprintf (xml, "<gegl><tree><node class='%s' path='%s'></node></tree></gegl>", map->handler, op_load->path);
                break;
              }
            map++;
          }
        if (map->extension==NULL) /* no extension matched, using fallback */
          {
            sprintf (xml, "<gegl><tree><node class='%s' path='%s'></node></tree></gegl>", map->handler, op_load->path);
          }

    temp_gegl = gegl_xml_parse (xml);
    gegl_node_apply (temp_gegl, "output");

    /*FIXME: this should be unneccesary, using the graph
     * directly as a node is more elegant.
     */
    gegl_node_get (temp_gegl, "output", &(op_load->cached), NULL);
    g_object_unref (temp_gegl);
  }
}


static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglOperationSource *op_source = GEGL_OPERATION_SOURCE(operation);
  ChantInstance *self = CHANT_INSTANCE (operation);

  if(strcmp("output", output_prop))
    return FALSE;

  if (op_source->output)
    g_object_unref (op_source->output);
  op_source->output=NULL;

  g_assert (self->cached);
  {
    GeglRect *result = gegl_operation_result_rect (operation);

    op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                        "source", self->cached,
                        "x",      result->x,
                        "y",      result->y,
                        "width",  result->w,
                        "height", result->h,
                        NULL);
  }
  return  TRUE;
}

static gboolean
calc_have_rect (GeglOperation *operation)
{
  ChantInstance *self = CHANT_INSTANCE (operation);
  gint width, height;

  load_cache (self);

  width  = GEGL_BUFFER (self->cached)->width;
  height = GEGL_BUFFER (self->cached)->height;

  gegl_operation_set_have_rect (operation, 0, 0, width, height);

  return TRUE;
}

static void dispose (GObject *gobject)
{
  ChantInstance *self = CHANT_INSTANCE (gobject);
  if (self->cached)
    {
      g_object_unref (self->cached);
      self->cached = NULL;
    }
  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (gobject)))->dispose (gobject);
}

static void class_init (GeglOperationClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = dispose;
}

#endif
