#include "gegl-utils.h"
#include "gegl-types.h"

#include "gegl-color-model-gray-u8.h"
#include "gegl-color-model-gray-u16.h"
#include "gegl-color-model-gray-u16_4.h"
#include "gegl-color-model-gray-float.h"
#include "gegl-color-model-rgb-u8.h"
#include "gegl-color-model-rgb-u16.h"
#include "gegl-color-model-rgb-u16_4.h"
#include "gegl-color-model-rgb-float.h"
#include "gegl-tile-image-manager.h"
#include "gegl-image.h"

static gboolean gegl_initialized = FALSE;

/* Singleton image manager */
static GeglImageManager *gegl_image_mngr = NULL;

/* Singleton color models */
static GeglColorModel *gegl_gray_u8 = NULL;
static GeglColorModel *gegl_graya_u8 = NULL;
static GeglColorModel *gegl_rgb_u8 = NULL;
static GeglColorModel *gegl_rgba_u8 = NULL;
static GeglColorModel *gegl_gray_u16 = NULL;
static GeglColorModel *gegl_graya_u16 = NULL;
static GeglColorModel *gegl_rgb_u16 = NULL;
static GeglColorModel *gegl_rgba_u16 = NULL;
static GeglColorModel *gegl_gray_u16_4 = NULL;
static GeglColorModel *gegl_graya_u16_4 = NULL;
static GeglColorModel *gegl_rgb_u16_4 = NULL;
static GeglColorModel *gegl_rgba_u16_4 = NULL;
static GeglColorModel *gegl_gray_float = NULL;
static GeglColorModel *gegl_graya_float = NULL;
static GeglColorModel *gegl_rgb_float = NULL;
static GeglColorModel *gegl_rgba_float = NULL;

/* Inheritance trees for synthesized image graph attributes,
   like ColorAlphaSpace, ColorSpace, or ChannelDataType */

static
GNode *
gegl_utils_color_alpha_space_root()
{
  /*
     RGBA
     /   \
   RGB  GRAYA
     \
     GRAY

     Simple now, but will get more complicated.
  */

  GNode *rgba = g_node_new(GUINT_TO_POINTER(GEGL_COLOR_ALPHA_SPACE_RGBA));
  GNode *rgb = g_node_append_data(rgba, GUINT_TO_POINTER(GEGL_COLOR_ALPHA_SPACE_RGB));
  g_node_append_data(rgba, GUINT_TO_POINTER(GEGL_COLOR_ALPHA_SPACE_GRAYA));
  g_node_append_data(rgb, GUINT_TO_POINTER(GEGL_COLOR_ALPHA_SPACE_GRAY));

  return rgba;
}

static
GNode *
gegl_utils_color_space_root()
{
  GNode *rgb = g_node_new(GUINT_TO_POINTER(GEGL_COLOR_SPACE_RGB));
  g_node_append_data(rgb, GUINT_TO_POINTER(GEGL_COLOR_SPACE_GRAY));

  return rgb;
}

static
GNode *
gegl_utils_channel_data_type_root()
{
  /*
     FLOAT
      |  
     U16 
      |
     U16_4 
      |
     U8
  */

  GNode *float_node = g_node_new(GUINT_TO_POINTER(GEGL_FLOAT));
  GNode *u16_node = g_node_append_data(float_node, GUINT_TO_POINTER(GEGL_U16));
  GNode *u16_4_node = g_node_append_data(u16_node, GUINT_TO_POINTER(GEGL_U16_4));
  g_node_append_data(u16_4_node, GUINT_TO_POINTER(GEGL_U8));

  return float_node;
}

static
GNode *
gegl_utils_least_common_ancestor(GNode *root, 
                                 GList *list)
{
  GNode *common = root;
  GNode *child = g_node_first_child(root);
  GList *ll;
  gboolean is_ancestor;

  while(child)
    {
      ll = list;
      is_ancestor = TRUE;
      while(ll)
        {
          GNode *node = (GNode*)(ll->data);

          /* If node is not a descendent of child, next child */
          if (!g_node_is_ancestor(child,node) && node != child)
            {
              is_ancestor = FALSE;
              break;
            }
          ll = g_list_next(ll);
        }

      /* If this child isnt an ancestor, next child*/
      if(!is_ancestor)
        child = g_node_next_sibling(child);
      else  
        {
          /* Found a common ancestor, look at its children*/
          common = child; 
          child = g_node_first_child (child); 
        }
    }

  return common;
}

typedef gpointer (*GeglListToNodeDataFunc)(gpointer); 

static
GList *
gegl_utils_nodes_list(GList *inputs,
                      GeglListToNodeDataFunc func,
                      GNode *root)
{
  GList *ll = inputs;
  GList *nodes  = NULL;

  while (ll)
    {
      /* Extract from list data to data to look for in tree.*/
      gpointer data = (*func)(ll->data);

      /* Find the node in the tree with matching data */
      GNode * node = g_node_find(root,
                                 G_IN_ORDER, 
                                 G_TRAVERSE_ALL, 
                                 data);

      /* Put this node on the list of nodes to 
         use to determine least common ancestor */
      nodes = g_list_append(nodes, node);
      ll = g_list_next(ll);
    }

  /* Return the nodes list */ 
  return nodes;
}

static
gpointer
gegl_utils_image_to_color_alpha_space(gpointer data)
{
  /* Takes the image data and extracts the coloralpha space. */
  GeglImage *input = GEGL_IMAGE(data);
  GeglColorModel *cm = gegl_image_color_model(input);
  GeglColorAlphaSpace space = gegl_color_model_color_alpha_space(cm);
  return GUINT_TO_POINTER(space);
}

GeglColorAlphaSpace
gegl_utils_derived_color_alpha_space(GList *inputs)
{
  /* The ColorAlphaSpace inheritance tree.*/ 
  GNode *root = gegl_utils_color_alpha_space_root();

  /* A list of nodes of the ColorAlphaSpaces in inputs 
     eg RGB, GRAY, RGBA, */
  GList *nodes = gegl_utils_nodes_list(inputs,
                            gegl_utils_image_to_color_alpha_space,
                            root);

  /* Find the least common ancestor of nodes */
  GNode * least_common_ancestor =
    gegl_utils_least_common_ancestor(root, nodes); 

  /* Free the nodes list, and the inheritance tree */
  g_list_free(nodes);
  g_node_destroy(root);

  /* The node data is the derived color alpha space */
  return GPOINTER_TO_UINT(least_common_ancestor->data);
}

static
gpointer
gegl_utils_image_to_channel_data_type(gpointer data)
{
  /* Takes the image data and extracts the channel data space. */
  GeglImage *input = GEGL_IMAGE(data);
  GeglColorModel *cm = gegl_image_color_model(input);
  GeglChannelDataType type = gegl_color_model_data_type(cm);
  return GUINT_TO_POINTER(type);
}

GeglChannelDataType
gegl_utils_derived_channel_data_type(GList *inputs)
{
  /* The ChannelData inheritance tree.*/ 
  GNode *root = gegl_utils_channel_data_type_root();

  /* A list of nodes of the ChannelData type in inputs 
     eg. FLOAT, U8 */
  GList *nodes = gegl_utils_nodes_list(inputs,
                            gegl_utils_image_to_channel_data_type,
                            root);

  /* Find the least common ancestor of nodes */
  GNode * least_common_ancestor =
    gegl_utils_least_common_ancestor(root, nodes); 

  /* Free the nodes list, and the inheritance tree */
  g_list_free(nodes);
  g_node_destroy(root);

  /* The node data is the derived color alpha space */
  return GPOINTER_TO_UINT(least_common_ancestor->data);
}

static
gpointer
gegl_utils_image_to_color_space(gpointer data)
{
  /* Takes the image data and extracts the channel data space. */
  GeglImage *input = GEGL_IMAGE(data);
  GeglColorModel *cm = gegl_image_color_model(input);
  GeglColorSpace space = gegl_color_model_color_space(cm);
  return GUINT_TO_POINTER(space);
}

GeglColorSpace
gegl_utils_derived_color_space(GList *inputs)
{
  /* The ColorSpace inheritance tree.*/ 
  GNode *root = gegl_utils_color_space_root();

  GList *nodes = gegl_utils_nodes_list(inputs,
                            gegl_utils_image_to_color_space,
                            root);

  /* Find the least common ancestor of nodes */
  GNode * least_common_ancestor =
    gegl_utils_least_common_ancestor(root, nodes); 

  /* Free the nodes list, and the inheritance tree */
  g_list_free(nodes);
  g_node_destroy(root);

  /* The node data is the derived color space */
  return GPOINTER_TO_UINT(least_common_ancestor->data);
}

GeglImageManager *
gegl_image_manager_instance ()
{
  return gegl_image_mngr;
}

GeglColorModel *
gegl_color_model_instance(GeglColorModelType type)
{
  switch (type)
    {
    case GEGL_COLOR_MODEL_TYPE_GRAY_U8:
      return gegl_gray_u8;
    case GEGL_COLOR_MODEL_TYPE_GRAY_U16:
      return gegl_gray_u16;
    case GEGL_COLOR_MODEL_TYPE_GRAY_U16_4:
      return gegl_gray_u16_4;
    case GEGL_COLOR_MODEL_TYPE_GRAY_FLOAT:
      return gegl_gray_float;
    case GEGL_COLOR_MODEL_TYPE_RGB_U8:
      return gegl_rgb_u8;
    case GEGL_COLOR_MODEL_TYPE_RGB_U16:
      return gegl_rgb_u16;
    case GEGL_COLOR_MODEL_TYPE_RGB_U16_4:
      return gegl_rgb_u16_4;
    case GEGL_COLOR_MODEL_TYPE_RGB_FLOAT:
      return gegl_rgb_float;
    case GEGL_COLOR_MODEL_TYPE_GRAYA_U8:
      return gegl_graya_u8;
    case GEGL_COLOR_MODEL_TYPE_GRAYA_U16:
      return gegl_graya_u16;
    case GEGL_COLOR_MODEL_TYPE_GRAYA_U16_4:
      return gegl_graya_u16_4;
    case GEGL_COLOR_MODEL_TYPE_GRAYA_FLOAT:
      return gegl_graya_float;
    case GEGL_COLOR_MODEL_TYPE_RGBA_U8:
      return gegl_rgba_u8;
    case GEGL_COLOR_MODEL_TYPE_RGBA_U16:
      return gegl_rgba_u16;
    case GEGL_COLOR_MODEL_TYPE_RGBA_U16_4:
      return gegl_rgba_u16_4;
    case GEGL_COLOR_MODEL_TYPE_RGBA_FLOAT:
      return gegl_rgba_float;
    default:
      return NULL;
    }
}

GeglColorModel *
gegl_color_model_instance1(GeglColorAlphaSpace color_alpha_space,
                           GeglChannelDataType data_type)
{
  switch (color_alpha_space)
    {
    case GEGL_COLOR_ALPHA_SPACE_GRAY:
      switch (data_type)
        {
        case GEGL_U8:
          return gegl_gray_u8;
        case GEGL_U16:
          return gegl_gray_u16;
        case GEGL_U16_4:
          return gegl_gray_u16_4;
        case GEGL_FLOAT:
          return gegl_gray_float;
        default:
          return NULL;
        }
    case GEGL_COLOR_ALPHA_SPACE_GRAYA:
      switch (data_type)
        {
        case GEGL_U8:
          return gegl_graya_u8;
        case GEGL_U16:
          return gegl_graya_u16;
        case GEGL_U16_4:
          return gegl_graya_u16_4;
        case GEGL_FLOAT:
          return gegl_graya_float;
        default:
          return NULL;
        }
    case GEGL_COLOR_ALPHA_SPACE_RGB:
      switch (data_type)
        {
        case GEGL_U8:
          return gegl_rgb_u8;
        case GEGL_U16:
          return gegl_rgb_u16;
        case GEGL_U16_4:
          return gegl_rgb_u16_4;
        case GEGL_FLOAT:
          return gegl_rgb_float;
        default:
          return NULL;
        }
    case GEGL_COLOR_ALPHA_SPACE_RGBA:
      switch (data_type)
        {
        case GEGL_U8:
          return gegl_rgba_u8;
        case GEGL_U16:
          return gegl_rgba_u16;
        case GEGL_U16_4:
          return gegl_rgba_u16_4;
        case GEGL_FLOAT:
          return gegl_rgba_float;
        default:
          return NULL;
        }
    default:
      return NULL;
    } 
}

GeglColorModel *
gegl_color_model_instance2(GeglColorSpace color_space,
                           GeglChannelDataType data_type,
                           gboolean has_alpha)
{
  switch (color_space)
    {
    case GEGL_COLOR_SPACE_GRAY:
      switch (data_type)
        {
        case GEGL_U8:
          return has_alpha ? gegl_graya_u8: gegl_gray_u8;
        case GEGL_U16:
          return has_alpha ? gegl_graya_u16: gegl_gray_u16;
        case GEGL_U16_4:
          return has_alpha ? gegl_graya_u16_4: gegl_gray_u16_4;
        case GEGL_FLOAT:
          return has_alpha ? gegl_graya_float: gegl_gray_float;
        default:
          return NULL;
        }
    case GEGL_COLOR_SPACE_RGB:
      switch (data_type)
        {
        case GEGL_U8:
          return has_alpha ? gegl_rgba_u8: gegl_rgb_u8;
        case GEGL_U16:
          return has_alpha ? gegl_rgba_u16: gegl_rgb_u16;
        case GEGL_U16_4:
          return has_alpha ? gegl_rgba_u16_4: gegl_rgb_u16_4;
        case GEGL_FLOAT:
          return has_alpha ? gegl_rgba_float: gegl_rgb_float;
        default:
          return NULL;
        }
    default:
      return NULL;
    }
}

static
void
gegl_init_color_models(void)
{
  gegl_rgba_u8 = GEGL_COLOR_MODEL (gegl_color_model_rgb_u8_new(TRUE));
  gegl_rgba_u16 = GEGL_COLOR_MODEL (gegl_color_model_rgb_u16_new(TRUE));
  gegl_rgba_u16_4 = GEGL_COLOR_MODEL (gegl_color_model_rgb_u16_4_new(TRUE));
  gegl_rgba_float = GEGL_COLOR_MODEL (gegl_color_model_rgb_float_new(TRUE));
  gegl_graya_u8 = GEGL_COLOR_MODEL (gegl_color_model_gray_u8_new(TRUE));
  gegl_graya_u16 = GEGL_COLOR_MODEL (gegl_color_model_gray_u16_new(TRUE));
  gegl_graya_u16_4 = GEGL_COLOR_MODEL (gegl_color_model_gray_u16_4_new(TRUE));
  gegl_graya_float = GEGL_COLOR_MODEL (gegl_color_model_gray_float_new(TRUE));
  gegl_rgb_u8 = GEGL_COLOR_MODEL (gegl_color_model_rgb_u8_new(FALSE));
  gegl_rgb_u16 = GEGL_COLOR_MODEL (gegl_color_model_rgb_u16_new(FALSE));
  gegl_rgb_u16_4 = GEGL_COLOR_MODEL (gegl_color_model_rgb_u16_4_new(FALSE));
  gegl_rgb_float = GEGL_COLOR_MODEL (gegl_color_model_rgb_float_new(FALSE));
  gegl_gray_u8 = GEGL_COLOR_MODEL (gegl_color_model_gray_u8_new(FALSE));
  gegl_gray_u16 = GEGL_COLOR_MODEL (gegl_color_model_gray_u16_new(FALSE));
  gegl_gray_u16_4 = GEGL_COLOR_MODEL (gegl_color_model_gray_u16_4_new(FALSE));
  gegl_gray_float = GEGL_COLOR_MODEL (gegl_color_model_gray_float_new(FALSE));
}

static
void
gegl_free_color_models(void)
{
  gegl_object_unref (GEGL_OBJECT(gegl_rgba_u8)); 
  gegl_object_unref (GEGL_OBJECT(gegl_rgba_u16)); 
  gegl_object_unref (GEGL_OBJECT(gegl_rgba_u16_4)); 
  gegl_object_unref (GEGL_OBJECT(gegl_rgba_float)); 
  gegl_object_unref (GEGL_OBJECT(gegl_graya_u8)); 
  gegl_object_unref (GEGL_OBJECT(gegl_graya_u16)); 
  gegl_object_unref (GEGL_OBJECT(gegl_graya_u16_4)); 
  gegl_object_unref (GEGL_OBJECT(gegl_graya_float)); 
  gegl_object_unref (GEGL_OBJECT(gegl_rgb_u8)); 
  gegl_object_unref (GEGL_OBJECT(gegl_rgb_u16)); 
  gegl_object_unref (GEGL_OBJECT(gegl_rgb_u16_4)); 
  gegl_object_unref (GEGL_OBJECT(gegl_rgb_float)); 
  gegl_object_unref (GEGL_OBJECT(gegl_gray_u8)); 
  gegl_object_unref (GEGL_OBJECT(gegl_gray_u16)); 
  gegl_object_unref (GEGL_OBJECT(gegl_gray_u16_4)); 
  gegl_object_unref (GEGL_OBJECT(gegl_gray_float)); 
}

static
void
gegl_exit(void)
{
  gegl_free_color_models();
  gegl_object_unref(GEGL_OBJECT(gegl_image_mngr));
}

void
gegl_init (int *argc, 
           char ***argv)
{
  if (gegl_initialized)
    return;
  gegl_image_mngr = GEGL_IMAGE_MANAGER(gegl_tile_image_manager_new());
  gegl_init_color_models();
  g_atexit (gegl_exit);
  gegl_initialized = TRUE;
  printf("in gegl_init\n");
}

gint
gegl_channel_data_type_bytes (GeglChannelDataType data)
{
  switch (data)
   {
   case GEGL_U8:
     return sizeof(guint8);
   case GEGL_FLOAT:
     return sizeof(gfloat);
   case GEGL_U16:
     return sizeof(guint16);
   case GEGL_U16_4:
     return sizeof(guint16);
   default:
     return 0;
   } 
}

void 
gegl_rect_set (GeglRect *r,
               gint x,
               gint y,
               guint w,
               guint h)
{
  r->x = x;
  r->y = y;
  r->w = w;
  r->h = h;
}

void 
gegl_rect_union (GeglRect *dest,
                 GeglRect *src1,
                 GeglRect *src2)
{
  gint x1 = MIN(src1->x, src2->x); 
  gint x2 = MAX(src1->x + src1->w, src2->x + src2->w);  
  gint y1 = MIN(src1->y, src2->y); 
  gint y2 = MAX(src1->y + src1->h, src2->y + src2->h);  
    
  dest->x = x1;
  dest->y = y1; 
  dest->w = x2 - x1;
  dest->h = y2 - y1;
}

gboolean 
gegl_rect_intersect(GeglRect *dest,
                    GeglRect *src1,
                    GeglRect *src2)
{
  gint x1, x2, y1, y2; 
    
  x1 = MAX(src1->x, src2->x); 
  x2 = MIN(src1->x + src1->w, src2->x + src2->w);  

  if (x2 <= x1)
    {
      gegl_rect_set (dest,0,0,0,0);
      return FALSE;
    }

  y1 = MAX(src1->y, src2->y); 
  y2 = MIN(src1->y + src1->h, src2->y + src2->h);  

  if (y2 <= y1)
    {
      gegl_rect_set (dest,0,0,0,0);
      return FALSE;
    }

  dest->x = x1;
  dest->y = y1; 
  dest->w = x2 - x1;
  dest->h = y2 - y1;
  return TRUE;
}

void 
gegl_rect_copy (GeglRect *to,
                GeglRect *from)
{
  to->x = from->x;
  to->y = from->y;
  to->w = from->w;
  to->h = from->h;
}

gboolean 
gegl_rect_contains (GeglRect *r,
                    GeglRect *s)
{
  if (s->x >= r->x &&
      s->y >= r->y &&
      (s->x + s->w) <= (r->x + r->w) && 
      (s->y + s->h) <= (r->y + r->h) ) 
    return TRUE;
  else
    return FALSE;
}

gboolean
gegl_rect_equal (GeglRect *r,
                 GeglRect *s)
{
  if (r->x == s->x && 
      r->y == s->y &&
      r->w == s->w &&
      r->h == s->h)
    return TRUE;
  else
    return FALSE;
}

