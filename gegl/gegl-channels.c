#include "gegl-channels.h"
#include "gegl-scanline-processor.h"
#include "gegl-image-data.h"
#include "gegl-image.h"

static void class_init (GeglChannelsClass * klass);
static void init (GeglChannels * self, GeglChannelsClass * klass);
static void finalize (GObject * gobject);

static void process (GeglFilter * filter);
static void prepare (GeglFilter * filter);

static void validate_inputs  (GeglFilter *filter, GArray *collected_data);
static void validate_outputs  (GeglFilter *filter);

static void validate_output_image(GeglChannels *self, gchar *name);
static void channels_rgb (GeglFilter * filter, GeglScanlineProcessor *processor, gint width);                       
static void compute_need_rects (GeglMultiImageOp *multi);
static void compute_have_rect (GeglMultiImageOp *multi);
static void compute_color_model (GeglMultiImageOp *multi);

static gpointer parent_class = NULL;

GType
gegl_channels_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglChannelsClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglChannels),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_MULTI_IMAGE_OP , 
                                     "GeglChannels", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglChannelsClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS(klass);
  GeglMultiImageOpClass *multi_image_op_class = GEGL_MULTI_IMAGE_OP_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);
  gobject_class->finalize = finalize;

  filter_class->process = process;
  filter_class->prepare = prepare;
  filter_class->validate_inputs = validate_inputs;
  filter_class->validate_outputs = validate_outputs;

  multi_image_op_class->compute_need_rects = compute_need_rects;
  multi_image_op_class->compute_have_rect = compute_have_rect;
  multi_image_op_class->compute_color_model = compute_color_model;
}

static void 
init (GeglChannels * self, 
      GeglChannelsClass * klass)
{
  self->scanline_processor = g_object_new(GEGL_TYPE_SCANLINE_PROCESSOR,NULL);
  self->scanline_processor->op = GEGL_FILTER(self);

  gegl_op_add_output_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "red");
  gegl_op_add_output_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "green");
  gegl_op_add_output_data(GEGL_OP(self), GEGL_TYPE_IMAGE_DATA, "blue");
}

static void 
finalize (GObject * gobject)
{
  GeglChannels * self = GEGL_CHANNELS(gobject);
  g_object_unref(self->scanline_processor);
  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void 
validate_inputs  (GeglFilter *filter, 
                  GArray *collected_data)
{
  GEGL_FILTER_CLASS(parent_class)->validate_inputs(filter, collected_data);
  {
    GeglData * data = gegl_op_get_input_data(GEGL_OP(filter), "source");
  }
}

static void 
validate_outputs (GeglFilter *filter)
{
  GeglChannels *self = GEGL_CHANNELS(filter);
  GEGL_FILTER_CLASS(parent_class)->validate_outputs(filter);

  validate_output_image(self, "red");
  validate_output_image(self, "green");
  validate_output_image(self, "blue");
}

static void
validate_output_image(GeglChannels *self,
                      gchar *name)
{
  GeglData *data = gegl_op_get_output_data(GEGL_OP(self), name);
  GValue *value = gegl_data_get_value(data);
  GeglImage *gray_image = (GeglImage*)g_value_get_object(value);

  if(!gray_image)
    {
      GeglColorModel *gray_color_model =  gegl_color_model_instance("gray-float");
      GeglImage * gray = g_object_new (GEGL_TYPE_IMAGE,
                                      "color_model", gray_color_model,
                                      NULL);
      GeglRect rect;
      gegl_image_data_get_rect(GEGL_IMAGE_DATA(data), &rect);

      gegl_log_debug(__FILE__, __LINE__,"validate_output_image", 
                     "have rect is x y w h is %d %d %d %d", 
                     rect.x, rect.y, rect.w, rect.h);

      gegl_image_create_tile(gray, gray_color_model, &rect); 
      g_value_set_object(value, gray);
      g_object_unref(gray);
    }
}

static void
compute_have_rect(GeglMultiImageOp *self)
{
  GeglData *red = gegl_op_get_output_data(GEGL_OP(self), "red");
  GeglData *green = gegl_op_get_output_data(GEGL_OP(self), "green");
  GeglData *blue = gegl_op_get_output_data(GEGL_OP(self), "blue");
  GeglData *source = gegl_op_get_input_data(GEGL_OP(self), "source");
  GeglRect s_rect;  /* source have rect */

  gegl_image_data_get_rect(GEGL_IMAGE_DATA(source), &s_rect);
  gegl_image_data_set_rect(GEGL_IMAGE_DATA(red), &s_rect);
  gegl_image_data_set_rect(GEGL_IMAGE_DATA(green), &s_rect);
  gegl_image_data_set_rect(GEGL_IMAGE_DATA(blue), &s_rect);
}

static void
compute_need_rects(GeglMultiImageOp *self)
{
  GeglData *red = gegl_op_get_output_data(GEGL_OP(self), "red");
  GeglData *green = gegl_op_get_output_data(GEGL_OP(self), "green");
  GeglData *blue = gegl_op_get_output_data(GEGL_OP(self), "blue");
  GeglData *source = gegl_op_get_input_data(GEGL_OP(self), "source");
  GeglRect r_rect;
  GeglRect g_rect;
  GeglRect b_rect;
  GeglRect bbox;
  GeglRect need_rect;  /* source need rect */

  gegl_image_data_get_rect(GEGL_IMAGE_DATA(red), &r_rect);
  gegl_image_data_get_rect(GEGL_IMAGE_DATA(green), &g_rect);
  gegl_image_data_get_rect(GEGL_IMAGE_DATA(blue), &b_rect);

  /* Find the bounding box of r, g, b need rects */
  gegl_rect_bounding_box (&bbox, &r_rect, &g_rect);
  gegl_rect_bounding_box (&need_rect, &b_rect, &bbox);

  gegl_image_data_set_rect(GEGL_IMAGE_DATA(source), &need_rect);
}

static void
compute_color_model(GeglMultiImageOp *self)
{
}

static void 
prepare (GeglFilter * filter) 
{
  GeglChannels *self =  GEGL_CHANNELS(filter);
  self->scanline_processor->func = channels_rgb;  
}

static void 
process (GeglFilter * filter) 
{
  GeglChannels *self =  GEGL_CHANNELS(filter);
  gegl_scanline_processor_process(self->scanline_processor);
}

static void                                                            
channels_rgb (GeglFilter * filter,              
              GeglScanlineProcessor *processor,
              gint width)                       
{
  GeglImageIterator *source = 
    gegl_scanline_processor_lookup_iterator(processor, "source");
  gfloat **s = (gfloat**)gegl_image_iterator_color_channels(source);
  gfloat *sa = (gfloat*)gegl_image_iterator_alpha_channel(source);
  gint s_color_chans = gegl_image_iterator_get_num_colors(source);

  GeglImageIterator *red = 
    gegl_scanline_processor_lookup_iterator(processor, "red");
  gfloat **r = (gfloat**)gegl_image_iterator_color_channels(red);

  GeglImageIterator *green = 
    gegl_scanline_processor_lookup_iterator(processor, "green");
  gfloat **g = (gfloat**)gegl_image_iterator_color_channels(green);

  GeglImageIterator *blue = 
    gegl_scanline_processor_lookup_iterator(processor, "blue");
  gfloat **b = (gfloat**)gegl_image_iterator_color_channels(blue);

  {
    gfloat *s0 = s[0];   
    gfloat *s1 = s[1];
    gfloat *s2 = s[2];

    gfloat *red = r[0];   
    gfloat *green = g[0];   
    gfloat *blue = b[0];   

    while(width--)                                                        
      {                                                                   
        *red++ = *s0++;
        *green++ = *s1++; 
        *blue++ = *s2++;
      }
  }

  g_free(s);
  g_free(r);
  g_free(g);
  g_free(b);
}
