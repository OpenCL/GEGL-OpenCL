#include "gegl-mem-buffer.h"
#include "gegl-buffer.h"

static void class_init (GeglMemBufferClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static void alloc_data (GeglBuffer * self_buffer);
static void unalloc_data (GeglBuffer * self_buffer);
static void ref (GeglBuffer * self_buffer);
static void unref (GeglBuffer * self_buffer);

static gpointer parent_class = NULL;

GType
gegl_mem_buffer_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMemBufferClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMemBuffer),
        0,
        (GInstanceInitFunc) NULL,
      };

      type = g_type_register_static (GEGL_TYPE_BUFFER, "GeglMemBuffer", &typeInfo, 0);
    }
    return type;
}

static void 
class_init (GeglMemBufferClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglBufferClass *buffer_class = GEGL_BUFFER_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->constructor = constructor;

  buffer_class->alloc_data = alloc_data;
  buffer_class->unalloc_data = unalloc_data;
  buffer_class->ref = ref;
  buffer_class->unref = unref;

  return;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglMemBuffer *self = GEGL_MEM_BUFFER(gobject);

  gegl_buffer_alloc_data(GEGL_BUFFER(self));

  return gobject;
}

static void 
alloc_data (GeglBuffer * self_buffer)
{
  gint i;

  if(self_buffer->data_pointers)
    {
      g_warning("GeglMemBuffer: data already allocated");
      return;
    }

  /* Allocate buffers for data. */
  self_buffer->data_pointers = g_new(gpointer,self_buffer->num_buffers);

  /* Each buffer has size bytes_per_buffer. */
  for(i = 0; i < self_buffer->num_buffers; i++)
    self_buffer->data_pointers[i] = g_new(guchar,self_buffer->bytes_per_buffer);
}

static void 
unalloc_data (GeglBuffer * self_buffer)
{
  gint i;

  /* Free the data */
  for(i = 0; i < self_buffer->num_buffers; i++)
    g_free(self_buffer->data_pointers[i]);

  /* Free the array of data pointers */
  g_free(self_buffer->data_pointers);
  self_buffer->data_pointers = NULL;
}

static void 
ref (GeglBuffer * self_buffer)
{
  self_buffer->ref_count++;
}

static void 
unref (GeglBuffer * self_buffer)
{
  if (self_buffer->ref_count == 0)
    return;
  self_buffer->ref_count--;
}
