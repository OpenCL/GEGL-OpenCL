/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Foobar is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Foobar; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Daniel S. Rogers
 *
 */

#include "gegl-buffer.h"

enum
{
  PROP_0, 
  PROP_NUM_BANKS,
  PROP_ELEMENTS_PER_BANK,
  PROP_LAST 
};

static void class_init (GeglBufferClass * klass);
static void init (GeglBuffer * self, GeglBufferClass * klass);
static void finalize (GObject * gobject);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static void alloc_banks(GeglBuffer * self);
static void free_banks (GeglBuffer * self);

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
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT , 
                                     "GeglBuffer", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
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
  gobject_class->constructor = constructor;

  g_object_class_install_property (gobject_class, PROP_ELEMENTS_PER_BANK,
                                   g_param_spec_int ("elements_per_bank",
                                                      "Elements Per Bank",
                                                      "GeglBuffer elements in each bank",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_NUM_BANKS,
                                   g_param_spec_int ("num_banks",
                                                      "Number of Banks ",
                                                      "GeglBuffer number of banks",
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      G_PARAM_CONSTRUCT_ONLY | 
                                                      G_PARAM_READWRITE));

}

static void 
init (GeglBuffer * self, 
      GeglBufferClass * klass)
{
  self->banks = NULL;
  self->elements_per_bank = 0;
  self->num_banks = 0;
}

static void 
finalize (GObject * gobject)
{
  GeglBuffer *self = GEGL_BUFFER(gobject);

  free_banks(self);

  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglBuffer *self = GEGL_BUFFER(gobject);

  alloc_banks (self);

  return gobject;
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
    case PROP_ELEMENTS_PER_BANK:
      self->elements_per_bank = g_value_get_int (value);
      break;
    case PROP_NUM_BANKS:
      self->num_banks = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
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
    case PROP_ELEMENTS_PER_BANK:
      g_value_set_int (value, gegl_buffer_get_elements_per_bank(self));
      break;
    case PROP_NUM_BANKS:
      g_value_set_int (value, gegl_buffer_get_num_banks(self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
alloc_banks (GeglBuffer * self)
{
    gpointer * banks = NULL;
    gint i;
  
    g_return_if_fail (self != NULL);
    g_return_if_fail (GEGL_IS_BUFFER (self));
    g_return_if_fail(self->bytes_per_element != 0);
    
    banks = g_new(gpointer, self->num_banks);

    for(i=0 ; i < self->num_banks; i++)
    {
        banks[i] = g_malloc((self->elements_per_bank)*(self->bytes_per_element));
    }
    self->banks = banks;
}

static void 
free_banks (GeglBuffer * self)
{
  gint i;
  for(i = 0; i < self->num_banks; i++)
    g_free(self->banks[i]);

  g_free(self->banks);
  self->banks = NULL;
}


/**
 * gegl_buffer_elements_per_bank:
 * @self: a #GeglBuffer
 *
 * Gets the elements per bank.
 *
 * Returns: number of elements per bank. 
 **/
gint 
gegl_buffer_get_elements_per_bank (GeglBuffer * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_BUFFER (self), 0);

  return self->elements_per_bank;
}

/**
 * gegl_buffer_num_banks:
 * @self: a #GeglBuffer
 *
 * Gets the number of banks.
 *
 * Returns: number of banks. 
 **/
gint 
gegl_buffer_get_num_banks (GeglBuffer * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_BUFFER (self), 0);

  return self->num_banks;
}

/**
 * gegl_buffer_get_banks:
 * @self: a #GeglBuffer
 *
 * Gets the data pointers.
 *
 * Returns: pointers to the buffers. 
 **/
gpointer * 
gegl_buffer_get_banks(GeglBuffer * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (self), NULL);

  return self->banks;
}

gdouble
gegl_buffer_get_element_double(GeglBuffer* self,gint bank,gint index){
    g_return_val_if_fail(GEGL_IS_BUFFER(self),0);   
    GeglBufferClass* klass=GEGL_BUFFER_GET_CLASS(self);
    g_return_val_if_fail(klass->get_element_double != NULL, 0.0);
    
    return klass->get_element_double(self,bank,index);
}

void
gegl_buffer_set_element_double(GeglBuffer* self,
                               gint bank,
                               gint index,
                               gdouble elem) {
    g_return_if_fail(GEGL_IS_BUFFER(self));
    GeglBufferClass* klass=GEGL_BUFFER_GET_CLASS(self);
    g_return_if_fail(klass->set_element_double != NULL);
    
    return klass->set_element_double(self,bank,index,elem);
}

TransferType
gegl_buffer_get_transfer_type(GeglBuffer* self) {
    g_return_val_if_fail(GEGL_IS_BUFFER(self),TYPE_UNKNOWN);
    
    return self->transfer_type;
}
