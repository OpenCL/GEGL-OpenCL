#include "gegl-utils.h"
#include "gegl-types.h"

#include "gegl-color-model-rgb-float.h"
#include "gegl-color-model-gray-float.h"
#include "gegl-color-model-rgb-u8.h"
#include "gegl-color-model-gray-u8.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-tile-mgr.h"
#include "gegl-graph.h"
#include "gegl-dump-visitor.h"

static gboolean gegl_initialized = FALSE;

extern void gegl_param_spec_types_init(void);
extern void gegl_value_types_init(void);

static
void
gegl_init_color_models(void)
{
  GeglColorModel * rgb_float = g_object_new(GEGL_TYPE_COLOR_MODEL_RGB_FLOAT, 
                                            "hasalpha", 
                                            FALSE, 
                                            NULL);

  GeglColorModel * rgba_float = g_object_new(GEGL_TYPE_COLOR_MODEL_RGB_FLOAT, 
                                            "hasalpha", 
                                            TRUE, 
                                            NULL);

  GeglColorModel * gray_float = g_object_new(GEGL_TYPE_COLOR_MODEL_GRAY_FLOAT, 
                                            "hasalpha", 
                                            FALSE, 
                                            NULL);

  GeglColorModel * graya_float = g_object_new(GEGL_TYPE_COLOR_MODEL_GRAY_FLOAT, 
                                            "hasalpha", 
                                            TRUE, 
                                            NULL);

  GeglColorModel * rgb_u8 = g_object_new(GEGL_TYPE_COLOR_MODEL_RGB_U8, 
                                         "hasalpha", 
                                          FALSE, 
                                          NULL);

  GeglColorModel * rgba_u8 = g_object_new(GEGL_TYPE_COLOR_MODEL_RGB_U8, 
                                          "hasalpha", 
                                          TRUE, 
                                          NULL);

  GeglColorModel * gray_u8 = g_object_new(GEGL_TYPE_COLOR_MODEL_GRAY_U8, 
                                         "hasalpha", 
                                          FALSE, 
                                          NULL);

  GeglColorModel * graya_u8 = g_object_new(GEGL_TYPE_COLOR_MODEL_GRAY_U8, 
                                          "hasalpha", 
                                          TRUE, 
                                          NULL);

  gegl_color_model_register("RgbFloat", rgb_float);
  gegl_color_model_register("RgbFloatAlpha", rgba_float);
  gegl_color_model_register("GrayFloat", gray_float);
  gegl_color_model_register("GrayFloatAlpha", graya_float);
  gegl_color_model_register("RgbU8", rgb_u8);
  gegl_color_model_register("RgbU8Alpha", rgba_u8);
  gegl_color_model_register("GrayU8", gray_u8);
  gegl_color_model_register("GrayU8Alpha", graya_u8);
}

static
void
gegl_free_color_models(void)
{
  GeglColorModel * rgb_float = gegl_color_model_instance("RgbFloat");
  GeglColorModel * rgba_float = gegl_color_model_instance("RgbFloatAlpha");
  GeglColorModel * gray_float = gegl_color_model_instance("GrayFloat");
  GeglColorModel * graya_float = gegl_color_model_instance("GrayFloatAlpha");
  GeglColorModel * rgb_u8 = gegl_color_model_instance("RgbU8");
  GeglColorModel * rgba_u8 = gegl_color_model_instance("RgbU8Alpha");
  GeglColorModel * gray_u8 = gegl_color_model_instance("GrayU8");
  GeglColorModel * graya_u8 = gegl_color_model_instance("GrayU8Alpha");

  g_object_unref (rgb_float); 
  g_object_unref (rgb_float); 
  g_object_unref (rgba_float); 
  g_object_unref (rgba_float); 
  g_object_unref (gray_float); 
  g_object_unref (gray_float); 
  g_object_unref (graya_float); 
  g_object_unref (graya_float); 
  g_object_unref (rgb_u8); 
  g_object_unref (rgb_u8); 
  g_object_unref (rgba_u8); 
  g_object_unref (rgba_u8); 
  g_object_unref (gray_u8); 
  g_object_unref (gray_u8); 
  g_object_unref (graya_u8); 
  g_object_unref (graya_u8); 
}

static
void
gegl_exit(void)
{
  GeglTileMgr *mgr = gegl_tile_mgr_instance();
  g_object_unref(mgr);
  g_object_unref(mgr);

  gegl_free_color_models();
}

void
gegl_init (int *argc, 
           char ***argv)
{
  if (gegl_initialized)
    return;

  gegl_param_spec_types_init ();
  gegl_init_color_models();

  {
    GeglTileMgr *mgr = g_object_new(GEGL_TYPE_TILE_MGR, NULL);
    gegl_tile_mgr_install(mgr);
  }

  g_atexit(gegl_exit);
  gegl_initialized = TRUE;
  
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
gegl_rect_bounding_box (GeglRect *dest,
                        GeglRect *src1,
                        GeglRect *src2)
{
  gboolean s1_has_area = src1->w && src1->h;
  gboolean s2_has_area = src2->w && src2->h;

  if( !s1_has_area &&  !s2_has_area)
      gegl_rect_set(dest,0,0,0,0);
  else if(!s1_has_area)
      gegl_rect_copy(dest,src2);
  else if(!s2_has_area)
      gegl_rect_copy(dest,src1);

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

gboolean
gegl_rect_equal_coords (GeglRect *r,
                        gint x,
                        gint y,
                        gint w,
                        gint h)
{
  if (r->x == x && 
      r->y == y &&
      r->w == w &&
      r->h == h)
    return TRUE;
  else
    return FALSE;
}

#define GEGL_LOG_DOMAIN "Gegl"


void
gegl_log(GLogLevelFlags level,
         gchar *file,
         gint line,
         gchar *function,
         gchar *format,
         ...)
{
    va_list args;
    va_start(args,format);
    gegl_logv(level,file,line,function,format,args);
    va_end(args);
}

void
gegl_logv(GLogLevelFlags level,
         gchar *file,
         gint line,
         gchar *function,
         gchar *format,
         va_list args)
{
    if (g_getenv("GEGL_LOG_ON"))
      {
        gchar *tabbed = NULL;

        /* log the file and line */
        g_log(GEGL_LOG_DOMAIN,level, "%s:  %s:%d:", function, file, line);

        /* move the regular output over a bit. */
        tabbed = g_strconcat("   ", format, NULL);
        g_logv(GEGL_LOG_DOMAIN,level, tabbed, args);
        g_log(GEGL_LOG_DOMAIN,level, "        ");
        g_free(tabbed);
      }
}

void
gegl_direct_log(GLogLevelFlags level,
         gchar *format,
         ...)
{
    va_list args;
    va_start(args,format);
    gegl_direct_logv(level,format,args);
    va_end(args);
}

void
gegl_direct_logv(GLogLevelFlags level,
         gchar *format,
         va_list args)
{
    if (g_getenv("GEGL_LOG_ON"))
      {
        gchar *tabbed = NULL;
        tabbed = g_strconcat("   ", format, NULL);
        g_logv(GEGL_LOG_DOMAIN, level, tabbed, args);
        g_free(tabbed);
      }
}

void
gegl_dump_graph_msg(gchar * msg, 
                    GeglNode * root) 
{
   LOG_DIRECT(msg);
   gegl_dump_graph(root);
}

void
gegl_dump_graph(GeglNode * root) 
{
   GeglDumpVisitor * dump = g_object_new(GEGL_TYPE_DUMP_VISITOR, NULL); 
   gegl_dump_visitor_traverse(dump, root); 
   g_object_unref(dump);
}
