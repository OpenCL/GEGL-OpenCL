#include "gegl-buffer.h"
#include <stdio.h>

enum
{
  PROP_0, 
  PROP_BYTES_PER_BUFFER,
  PROP_NUM_BUFFERS,
  PROP_LAST 
};

static void class_init (GeglBufferClass * klass);
static void init (GeglBuffer * self, GeglBufferClass * klass);
static void finalize (GObject * gobject);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_buffer_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglBufferClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglBuffer),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT , "GeglBuffer", &typeInfo, 0);
    }
    return type;
}

static void 
class_init (GeglBufferClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  klass->unalloc_data = NULL;
  klass->alloc_data = NULL;
  klass->ref = NULL;
  klass->unref = NULL;

  g_object_class_install_property (gobject_class, PROP_BYTES_PER_BUFFER,
                                   g_param_spec_int ("bytesperbuffer",
                                                      "Bytes Per Buffer",
                                                      "GeglBuffer bytes in each buffer",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_NUM_BUFFERS,
                                   g_param_spec_int ("numbuffers",
                                                      "Num Buffers",
                                                      "GeglBuffer number of buffers",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT | 
                                                      G_PARAM_READWRITE));

  return;
}

static void 
init (GeglBuffer * self, 
      GeglBufferClass * klass)
{
  self->data_pointers = NULL;
  self->ref_count = 0;   
  return;
}

static void 
finalize (GObject * gobject)
{
  GeglBuffer *self = GEGL_BUFFER(gobject);

  if (self->data_pointers)
    gegl_buffer_unalloc_data(self);

  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglBuffer *self = GEGL_BUFFER (gobject);

  switch (prop_id)
  {
    case PROP_BYTES_PER_BUFFER:
        if(!GEGL_OBJECT(gobject)->constructed)
          gegl_buffer_set_bytes_per_buffer(self,g_value_get_int (value));
        else
          g_message("Cant set bytes per buffer after construction\n");
      break;
    case PROP_NUM_BUFFERS:
        if(!GEGL_OBJECT(gobject)->constructed)
          gegl_buffer_set_num_buffers(self, g_value_get_int (value));
        else
          g_message("Cant set num buffers after construction\n");
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
  GeglBuffer *self = GEGL_BUFFER (gobject);

  switch (prop_id)
  {
    case PROP_BYTES_PER_BUFFER:
        g_value_set_int (value, gegl_buffer_get_bytes_per_buffer(self));
      break;
    case PROP_NUM_BUFFERS:
        g_value_set_int (value, gegl_buffer_get_num_buffers(self));
      break;
    default:
      break;
  }
}


/**
 * gegl_buffer_get_bytes_per_buffer:
 * @self: a #GeglBuffer
 *
 * Gets the bytes per buffer.
 *
 * Returns: number of bytes per buffer. 
 **/
gint 
gegl_buffer_get_bytes_per_buffer (GeglBuffer * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_BUFFER (self), (gint )0);

  return self->bytes_per_buffer;
}

void
gegl_buffer_set_bytes_per_buffer (GeglBuffer * self, 
                                  gint bytes_per_buffer)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_BUFFER (self));

  self->bytes_per_buffer = bytes_per_buffer;
}

/**
 * gegl_buffer_get_num_buffers:
 * @self: a #GeglBuffer
 *
 * Gets the number of buffer.
 *
 * Returns: number of buffers. 
 **/
gint 
gegl_buffer_get_num_buffers (GeglBuffer * self)
{
  g_return_val_if_fail (self != NULL, (gint )0);
  g_return_val_if_fail (GEGL_IS_BUFFER (self), (gint )0);

  return self->num_buffers;
}

void
gegl_buffer_set_num_buffers (GeglBuffer * self, 
                             gint num_buffers)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_BUFFER (self));

  self->num_buffers = num_buffers;
}

/**
 * gegl_buffer_get_data_pointers:
 * @self: a #GeglBuffer
 *
 * Gets the data pointers.
 *
 * Returns: pointers to the buffers. 
 **/
gpointer * 
gegl_buffer_get_data_pointers (GeglBuffer * self)
{
  g_return_val_if_fail (self != NULL, (gpointer * )0);
  g_return_val_if_fail (GEGL_IS_BUFFER (self), (gpointer * )0);

  return self->data_pointers;
}

gpointer * 
gegl_buffer_alloc_data_pointers (GeglBuffer * self)
{
  gpointer * data_pointers = NULL;
  gint i;

  g_return_val_if_fail (self != NULL, (gpointer * )0);
  g_return_val_if_fail (GEGL_IS_BUFFER (self), (gpointer * )0);

  data_pointers = g_new(gpointer, self->num_buffers);

  for(i=0 ; i < self->num_buffers; i++)
    data_pointers[i] = self->data_pointers[i];

  return data_pointers;
}

void 
gegl_buffer_unalloc_data (GeglBuffer * self)
{
  GeglBufferClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_BUFFER (self));
  klass = GEGL_BUFFER_GET_CLASS(self);

  if(klass->unalloc_data)
    (*klass->unalloc_data)(self);
}

void 
gegl_buffer_alloc_data (GeglBuffer * self)
{
  GeglBufferClass *klass;
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_BUFFER (self));
  klass = GEGL_BUFFER_GET_CLASS(self);

  if(klass->alloc_data)
    (*klass->alloc_data)(self);
}

void 
gegl_buffer_ref (GeglBuffer * self)
{
   GeglBufferClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_BUFFER (self));
   klass = GEGL_BUFFER_GET_CLASS(self);

   if(klass->ref)
      (*klass->ref)(self);
}

void 
gegl_buffer_unref (GeglBuffer * self)
{
   GeglBufferClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_BUFFER (self));
   klass = GEGL_BUFFER_GET_CLASS(self);

   if(klass->unref)
      (*klass->unref)(self);
}

gint 
gegl_buffer_get_ref_count (GeglBuffer * self)
{
  return self->ref_count;
}
