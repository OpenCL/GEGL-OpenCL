/* This file is part of GEGL editor -- an mrg frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2015 Øyvind Kolås pippin@gimp.org
 */

/* The code in this file is an image viewer/editor written using microraptor
 * gui and GEGL. It renders the UI directly from GEGLs data structures. 
 */

#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include "config.h"

#if HAVE_MRG

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <mrg.h>
#include <gegl.h>
#include <gexiv2/gexiv2.h>
#include <gegl-paramspecs.h>
#include <SDL.h>
#include <gegl-audio-fragment.h>

/* comment this out, and things render more correctly but much slower
 * for images larger than your screen/window resolution
 */
#define USE_MIPMAPS    1

/* set this to 1 to print the active gegl chain
 */
#define DEBUG_OP_LIST  0


static int audio_len   = 0;
static int audio_pos   = 0;
static int audio_post   = 0;
//static int audio_start = 0; /* which sample no is at the start of our circular buffer */

#define AUDIO_BUF_LEN 819200000

int16_t audio_data[AUDIO_BUF_LEN];

static void sdl_audio_cb(void *udata, Uint8 *stream, int len)
{
  int audio_remaining = audio_len - audio_pos;
  if (audio_remaining < 0)
    return;

  if (audio_remaining < len) len = audio_remaining;

  //SDL_MixAudio(stream, (uint8_t*)&audio_data[audio_pos/2], len, SDL_MIX_MAXVOLUME);
  memcpy (stream, (uint8_t*)&audio_data[audio_pos/2], len);
  audio_pos += len;
  audio_post += len;
  if (audio_pos >= AUDIO_BUF_LEN)
  {
    audio_pos = 0;
  }
}

static void sdl_add_audio_sample (int sample_pos, float left, float right)
{
   audio_data[audio_len/2 + 0] = left * 32767.0 * 0.46;
   audio_data[audio_len/2 + 1] = right * 32767.0 * 0.46;
   audio_len += 4;

   if (audio_len >= AUDIO_BUF_LEN)
   {
     audio_len = 0;
   }
}

static int audio_started = 0;


/*  this structure contains the full application state, and is what
 *  re-renderings of the UI is directly based on.
 */
typedef struct _State State;
struct _State {
  void      (*ui) (Mrg *mrg, void *state);
  Mrg        *mrg;

  char       *path;
  char       *save_path;

  GList      *paths;

  GeglBuffer *buffer;
  GeglNode   *gegl;
  GeglNode   *sink;
  GeglNode   *source;
  GeglNode   *load;
  GeglNode   *save;
  GeglNode   *active;

  GeglNode   *rotate;
  int         rev;
  float       u, v;
  float       scale;
  int         show_actions;
  int         show_controls;

  float       render_quality;
  float       preview_quality;

  int         controls_timeout;

  char      **ops; // the operations part of the commandline, if any

  float       slide_pause;
  int         slide_enabled;
  int         slide_timeout;

  GeglNode   *gegl_decode;
  GeglNode   *decode_load;
  GeglNode   *decode_store;

  int         is_video;
  int         frame_no;

  int         prev_frame_played;
};


typedef struct ActionData {
  const char *label;
  float priority; /* XXX: not yey used, should force scale/crop to happen early */
  const char *op_name;
} ActionData;

/* white-list of operations useful for improving/altering photos
 */
ActionData actions[]={
  {"rotate",            10,  "gegl:rotate"},
  {"crop",              20,  "gegl:crop"},
  {"color temperature", 50,  "gegl:color-temperature"},
  {"exposure",          60,  "gegl:exposure"},
  //{"levels",            60,  "gegl:levels"},
  //{"threshold",         70,  "gegl:threshold"},
  {NULL, 0, NULL}, /* sentinel */
};

static char *suffix = "-gegl";


void   gegl_meta_set (const char *path, const char *meta_data);
char * gegl_meta_get (const char *path);
GExiv2Orientation path_get_orientation (const char *path);

static char *suffix_path (const char *path);
static char *unsuffix_path (const char *path);
static int is_gegl_path (const char *path);

static void contrasty_stroke (cairo_t *cr);

static void mrg_gegl_blit (Mrg *mrg,
			   float x0, float y0,
			   float width, float height,
			   GeglNode *node,
			   float u, float v,
			   float scale,
                           float preview_multiplier);

gchar *get_thumb_path (const char *path);
gchar *get_thumb_path (const char *path)
{
  gchar *ret;
  gchar *uri = g_strdup_printf ("file://%s", path);
  gchar *hex = g_compute_checksum_for_string (G_CHECKSUM_MD5, uri, -1);
  int i;  
  for (i = 0; hex[i]; i++)
    hex[i] = tolower (hex[i]);
  ret = g_strdup_printf ("%s/.cache/thumbnails/large/%s.png", g_get_home_dir(), hex);
  if (access (ret, F_OK) == -1)
  {
    g_free (ret);
    ret = g_strdup_printf ("%s/.cache/thumbnails/normal/%s.png", g_get_home_dir(), hex);
  }
  g_free (uri);
  g_free (hex);
  return ret;
}


static void load_path (State *o);

static void go_next (State *o);
static void go_prev (State *o);
static void go_parent (State *o);

static void get_coords (State *o, float screen_x, float screen_y, float *gegl_x, float *gegl_y);
static void leave_editor (State *o);
static void drag_preview (MrgEvent *e);
static void load_into_buffer (State *o, const char *path);
static GeglNode *locate_node (State *o, const char *op_name);
static void zoom_to_fit (State *o);
static void center (State *o);
static void zoom_to_fit_buffer (State *o);

static void go_next_cb   (MrgEvent *event, void *data1, void *data2);
static void go_prev_cb   (MrgEvent *event, void *data1, void *data2);
static void go_parent_cb (MrgEvent *event, void *data1, void *data2);
static void zoom_fit_cb  (MrgEvent *e, void *data1, void *data2);
static void pan_left_cb  (MrgEvent *event, void *data1, void *data2);
static void pan_right_cb (MrgEvent *event, void *data1, void *data2);
static void pan_down_cb  (MrgEvent *event, void *data1, void *data2);
static void pan_up_cb    (MrgEvent *event, void *data1, void *data2);
static void preview_more_cb (MrgEvent *event, void *data1, void *data2);
static void preview_less_cb (MrgEvent *event, void *data1, void *data2);
static void zoom_1_cb               (MrgEvent *event, void *data1, void *data2);
static void zoom_in_cb              (MrgEvent *event, void *data1, void *data2);
static void zoom_out_cb             (MrgEvent *event, void *data1, void *data2);
static void toggle_actions_cb       (MrgEvent *event, void *data1, void *data2);
static void toggle_fullscreen_cb    (MrgEvent *event, void *data1, void *data2);
static void activate_op_cb          (MrgEvent *event, void *data1, void *data2);
static void disable_filter_cb       (MrgEvent *event, void *data1, void *data2);
static void apply_filter_cb         (MrgEvent *event, void *data1, void *data2);
static void discard_cb              (MrgEvent *event, void *data1, void *data2);
static void save_cb                 (MrgEvent *event, void *data1, void *data2);
static void toggle_show_controls_cb (MrgEvent *event, void *data1, void *data2);

static void gegl_ui       (Mrg *mrg, void *data);
int         mrg_ui_main   (int argc, char **argv, char **ops);
void        gegl_meta_set (const char *path, const char *meta_data);
char *      gegl_meta_get (const char *path); 


static void on_viewer_motion (MrgEvent *e, void *data1, void *data2);

static int str_has_image_suffix (char *path)
{
  return g_str_has_suffix (path, ".jpg") ||
         g_str_has_suffix (path, ".png") ||
         g_str_has_suffix (path, ".JPG") ||
         g_str_has_suffix (path, ".PNG") ||
         g_str_has_suffix (path, ".jpeg") ||
         g_str_has_suffix (path, ".JPEG") ||
         g_str_has_suffix (path, ".CR2") ||
         g_str_has_suffix (path, ".exr");
}

static int str_has_video_suffix (char *path)
{
  return g_str_has_suffix (path, ".avi") ||
         g_str_has_suffix (path, ".AVI") ||
         g_str_has_suffix (path, ".mp4") ||
         g_str_has_suffix (path, ".mp3") ||
         g_str_has_suffix (path, ".mpg") ||
         g_str_has_suffix (path, ".ogv") ||
         g_str_has_suffix (path, ".MPG") ||
         g_str_has_suffix (path, ".webm") ||
         g_str_has_suffix (path, ".MP4") ||
         g_str_has_suffix (path, ".mkv") ||
         g_str_has_suffix (path, ".MKV") ||
         g_str_has_suffix (path, ".mov") ||
         g_str_has_suffix (path, ".ogg");
}

static int str_has_visual_suffix (char *path)
{
  return str_has_image_suffix (path) || str_has_video_suffix (path);
}

static void populate_path_list (State *o)
{
  struct dirent **namelist;
  int i;
  struct stat stat_buf;
  char *path = strdup (o->path);
  int n;
  while (o->paths)
    {
      char *freed = o->paths->data;
      o->paths = g_list_remove (o->paths, freed);
      g_free (freed);
    }

  lstat (o->path, &stat_buf);
  if (S_ISREG (stat_buf.st_mode))
  {
    char *lastslash = strrchr (path, '/');
    if (lastslash)
      {
	if (lastslash == path)
	  lastslash[1] = '\0';
	else
	  lastslash[0] = '\0';
      }
  }
  n = scandir (path, &namelist, NULL, alphasort);
  
  for (i = 0; i < n; i++)
  {
    if (namelist[i]->d_name[0] != '.' &&
        str_has_visual_suffix (namelist[i]->d_name))
    {
      gchar *fpath = g_strdup_printf ("%s/%s", path, namelist[i]->d_name);

      lstat (fpath, &stat_buf);
      if (S_ISREG (stat_buf.st_mode))
      {
        if (is_gegl_path (fpath))
        {
          char *tmp = unsuffix_path (fpath);
          g_free (fpath);
          fpath = g_strdup (tmp);
          free (tmp);
        }
        if (!g_list_find_custom (o->paths, fpath, (void*)g_strcmp0))
        {
          o->paths = g_list_append (o->paths, fpath);
        }
      }
    }
  }
  free (namelist);
}

static State *hack_state = NULL;  // XXX: this shoudl be factored away

char **ops = NULL;


static void open_audio (int frequency)
{
  SDL_AudioSpec spec = {0};
  SDL_Init(SDL_INIT_AUDIO);
  spec.freq = frequency;
  spec.format = AUDIO_S16SYS;
  spec.channels = 2;
  spec.samples = 1024;
  spec.callback = sdl_audio_cb;
  SDL_OpenAudio(&spec, 0);

  if (spec.format != AUDIO_S16SYS)
   {
      fprintf (stderr, "not getting format we wanted\n");
   }
  if (spec.freq != frequency)
   {
      fprintf (stderr, "not getting desires samplerate(%i) we wanted got %i instead\n", frequency, spec.freq);
   }
}

static void end_audio (void)
{
  
}

int mrg_ui_main (int argc, char **argv, char **ops)
{
  Mrg *mrg = mrg_new (1024, 768, NULL);
  State o = {NULL,};

#ifdef USE_MIPMAPS
  /* to use this UI comfortably, mipmap rendering needs to be enabled, there are
   * still a few glitches, but for basic full frame un-panned, un-cropped use it
   * already works well.
   */
  g_setenv ("GEGL_MIPMAP_RENDERING", "1", TRUE);
#endif

/* we want to see the speed gotten if the fastest babl conversions we have were more accurate */
  g_setenv ("BABL_TOLERANCE", "0.1", TRUE);
  
  if(ops)
    o.ops = ops;
  else
  {
    int i;
    for (i = 0; argv[i]; i++)
    {
      if (!strcmp (argv[i], "--"))
      {
        o.ops = &argv[i];
        break;
      }
    }
  }

  gegl_init (&argc, &argv);
  o.gegl            = gegl_node_new (); // so that we have an object to unref
  o.mrg             = mrg;
  o.scale           = 1.0;
  o.render_quality  = 1.0;
  o.preview_quality = 2.0;
  o.slide_pause     = 5.0;
  o.slide_enabled   = 0;

  if (access (argv[1], F_OK) != -1)
    o.path = realpath (argv[1], NULL);
  else
    {
      printf ("usage: %s <full-path-to-image>\n", argv[0]);
      return -1;
    }


  load_path (&o);
  mrg_set_ui (mrg, gegl_ui, &o);
  hack_state = &o;  
  on_viewer_motion (NULL, &o, NULL);
  mrg_main (mrg);

  g_object_unref (o.gegl);
  if (o.buffer)
  {
    g_object_unref (o.buffer);
    o.buffer = NULL;
  }
  gegl_exit ();

  end_audio ();
  return 0;
}

static int hide_controls_cb (Mrg *mrg, void *data)
{
  State *o = data;
  o->controls_timeout = 0;
  o->show_controls = 0;
  mrg_queue_draw (o->mrg, NULL);
  return 0;
}

static void on_viewer_motion (MrgEvent *e, void *data1, void *data2)
{
  State *o = data1;
  {
    if (!o->show_controls)
    {
      o->show_controls = 1;
      mrg_queue_draw (o->mrg, NULL);
    }
    if (o->controls_timeout)
    {
      mrg_remove_idle (o->mrg, o->controls_timeout);
      o->controls_timeout = 0;
    }
    o->controls_timeout = mrg_add_timeout (o->mrg, 1000, hide_controls_cb, o);
  }
}

static void on_pan_drag (MrgEvent *e, void *data1, void *data2)
{
  State *o = data1;
  if (e->type == MRG_DRAG_MOTION)
  {
    o->u -= (e->delta_x );
    o->v -= (e->delta_y );
    mrg_queue_draw (e->mrg, NULL);
  }
  drag_preview (e);
}

static void prop_double_drag_cb (MrgEvent *e, void *data1, void *data2)
{
  GeglNode *node = data1;
  GParamSpec *pspec = data2;
  GeglParamSpecDouble *gspec = data2;
  gdouble value = 0.0;
  float range = gspec->ui_maximum - gspec->ui_minimum;

  value = e->x / mrg_width (e->mrg);
  value = value * range + gspec->ui_minimum;
  gegl_node_set (node, pspec->name, value, NULL);
   
  drag_preview (e);

  mrg_queue_draw (e->mrg, NULL);
}

static void draw_gegl_generic (State *state, Mrg *mrg, cairo_t *cr, GeglNode *node)
{
  const gchar* op_name = gegl_node_get_operation (node);
  mrg_set_edge_left (mrg, mrg_width (mrg) * 0.1);
  mrg_set_edge_top  (mrg, mrg_height (mrg) * 0.1);
  mrg_set_font_size (mrg, mrg_height (mrg) * 0.07);
  mrg_set_style (mrg, "color:white; background-color: transparent");

  cairo_save (cr);
  mrg_printf (mrg, "%s\n", op_name);
  {
    guint n_props;
    GParamSpec **pspecs = gegl_operation_list_properties (op_name, &n_props);

    if (pspecs)
    {
      int tot_pos = 0;
      int pos_no = 0;

      for (gint i = 0; i < n_props; i++)
      {
        if (g_type_is_a (pspecs[i]->value_type, G_TYPE_DOUBLE) ||
            g_type_is_a (pspecs[i]->value_type, G_TYPE_INT) ||
            g_type_is_a (pspecs[i]->value_type, G_TYPE_STRING) ||
            g_type_is_a (pspecs[i]->value_type, G_TYPE_BOOLEAN))
          tot_pos ++;
      }

      for (gint i = 0; i < n_props; i++)
      {
        mrg_set_xy (mrg, mrg_em(mrg), mrg_height (mrg) - mrg_em (mrg) * ((tot_pos-pos_no)));

        if (g_type_is_a (pspecs[i]->value_type, G_TYPE_DOUBLE))
        {
          float xpos;
          GeglParamSpecDouble *geglspec = (void*)pspecs[i];
          gdouble value;
          gegl_node_get (node, pspecs[i]->name, &value, NULL);

          cairo_rectangle (cr, 0,
             mrg_height (mrg) - mrg_em (mrg) * ((tot_pos-pos_no+1)),
             mrg_width (mrg), mrg_em(mrg));
          cairo_set_source_rgba (cr, 0,0,0, 0.5);

          mrg_listen (mrg, MRG_DRAG, prop_double_drag_cb, node,(void*)pspecs[i]);

          cairo_fill (cr);
          xpos = (value - geglspec->ui_minimum) / (geglspec->ui_maximum - geglspec->ui_minimum);
          cairo_rectangle (cr, xpos * mrg_width(mrg) - mrg_em(mrg)/4,
              mrg_height (mrg) - mrg_em (mrg) * ((tot_pos-pos_no+1)),
              mrg_em(mrg)/2, mrg_em(mrg));
          cairo_set_source_rgba (cr, 1,1,1, 0.5);
          cairo_fill (cr);

          mrg_printf (mrg, "%s:%f\n", pspecs[i]->name, value);
          pos_no ++;
        }
        else if (g_type_is_a (pspecs[i]->value_type, G_TYPE_INT))
        {
          gint value;
          gegl_node_get (node, pspecs[i]->name, &value, NULL);
          mrg_printf (mrg, "%s:%i\n", pspecs[i]->name, value);
          pos_no ++;
        }
        else if (g_type_is_a (pspecs[i]->value_type, G_TYPE_STRING))
        {
          char *value = NULL;
          gegl_node_get (node, pspecs[i]->name, &value, NULL);
          pos_no ++;
          mrg_printf (mrg, "%s:%s\n", pspecs[i]->name, value);
          if (value) g_free (value);
        }
        else if (g_type_is_a (pspecs[i]->value_type, G_TYPE_BOOLEAN))
        {
          gboolean value = FALSE;
          gegl_node_get (node, pspecs[i]->name, &value, NULL);
          pos_no ++;
          mrg_printf (mrg, "%s:%i\n", pspecs[i]->name, value);
        }
        
      }
      g_free (pspecs);
    }
  }

  cairo_restore (cr);
  mrg_set_style (mrg, "color:yellow; background-color: transparent");
}

static void crop_drag_ul (MrgEvent *e, void *data1, void *data2)
{
  GeglNode *node = data1;
  double x,y,width,height;
  double x0, y0, x1, y1;
  gegl_node_get (node, "x", &x, "y", &y, "width", &width, "height", &height, NULL);
  x0 = x; y0 = y; x1 = x0 + width; y1 = y + height;

  if (e->type == MRG_DRAG_MOTION)
  {
    x0 += e->delta_x;
    y0 += e->delta_y;

    x=x0;
    y=y0;
    width = x1 - x0;
    height = y1 - y0;
    gegl_node_set (node, "x", x, "y", y, "width", width, "height", height, NULL);

    mrg_queue_draw (e->mrg, NULL);
  }

  drag_preview (e);
}

static void crop_drag_lr (MrgEvent *e, void *data1, void *data2)
{
  GeglNode *node = data1;
  double x,y,width,height;
  double x0, y0, x1, y1;
  gegl_node_get (node, "x", &x, "y", &y, "width", &width, "height", &height, NULL);
  x0 = x; y0 = y; x1 = x0 + width; y1 = y + height;

  if (e->type == MRG_DRAG_MOTION)
  {
    x1 += e->delta_x;
    y1 += e->delta_y;

    x=x0;
    y=y0;
    width = x1 - x0;
    height = y1 - y0;
    gegl_node_set (node, "x", x, "y", y, "width", width, "height", height, NULL);

    mrg_queue_draw (e->mrg, NULL);
  }

  drag_preview (e);
}

static void crop_drag_rotate (MrgEvent *e, void *data1, void *data2)
{
  State *o = hack_state;
  double degrees;
  gegl_node_get (o->rotate, "degrees", &degrees, NULL);

  if (e->type == MRG_DRAG_MOTION)
  {
    degrees += e->delta_x / 100.0;

    gegl_node_set (o->rotate, "degrees", degrees, NULL);

    mrg_queue_draw (e->mrg, NULL);
  }

  drag_preview (e);
}

static void draw_gegl_crop (State *o, Mrg *mrg, cairo_t *cr, GeglNode *node)
{
  const gchar* op_name = gegl_node_get_operation (node);
  float dim = mrg_height (mrg) * 0.1 / o->scale;
  double x,y,width,height;
  double x0, y0, x1, y1;

  mrg_set_edge_left (mrg, mrg_width (mrg) * 0.1);
  mrg_set_edge_top  (mrg, mrg_height (mrg) * 0.1);
  mrg_set_font_size (mrg, mrg_height (mrg) * 0.07);
  mrg_set_style (mrg, "color:white; background-color: transparent");

  cairo_save (cr);
  mrg_printf (mrg, "%s\n", op_name);

  cairo_translate (cr, -o->u, -o->v);
  cairo_scale (cr, o->scale, o->scale);

  gegl_node_get (node, "x", &x, "y", &y, "width", &width, "height", &height, NULL);
  x0 = x; y0 = y; x1 = x0 + width; y1 = y + height;

  cairo_rectangle (cr, x0, y0, dim, dim);
  mrg_listen (mrg, MRG_DRAG, crop_drag_ul, node, NULL);
  contrasty_stroke (cr);

  cairo_rectangle (cr, x1-dim, y1-dim, dim, dim);
  mrg_listen (mrg, MRG_DRAG, crop_drag_lr, node, NULL);
  contrasty_stroke (cr);

  cairo_rectangle (cr, x0+dim, y0+dim, width-dim-dim, height-dim-dim);
  mrg_listen (mrg, MRG_DRAG, crop_drag_rotate, node, NULL);
  cairo_new_path (cr);

  cairo_restore (cr);
  mrg_set_style (mrg, "color:yellow; background-color: transparent");
}

static void ui_op_draw_apply_disable (State *o)
{
  Mrg *mrg = o->mrg;
  mrg_set_font_size (mrg, mrg_height (mrg) * 0.1);
  mrg_set_xy (mrg, 0, mrg_em(mrg));
  mrg_text_listen (mrg, MRG_PRESS, disable_filter_cb, o, NULL);
  mrg_printf (mrg, "X");
  mrg_text_listen_done (mrg);
  mrg_set_xy (mrg, mrg_width(mrg) - mrg_em (mrg), mrg_em(mrg));
  mrg_text_listen (mrg, MRG_PRESS, apply_filter_cb, o, NULL);
  mrg_printf (mrg, "O");
  mrg_text_listen_done (mrg);
}

static void ui_active_op (State *o)
{
  Mrg *mrg = o->mrg;
  cairo_t *cr = mrg_cr (mrg);
  char *opname = NULL;
  g_object_get (o->active, "operation", &opname, NULL);

  if (!strcmp (opname, "gegl:crop")) {
    zoom_to_fit_buffer (o);
    draw_gegl_crop (o, mrg, cr, o->active);
    ui_op_draw_apply_disable (o);
  }
  else
  {
    draw_gegl_generic (o, mrg, cr, o->active);
    ui_op_draw_apply_disable (o);
  }
}

#if DEBUG_OP_LIST
static void ui_debug_op_chain (State *o)
{
  Mrg *mrg = o->mrg;
  GeglNode *iter;
  mrg_set_edge_top  (mrg, mrg_height (mrg) * 0.2);
  iter = o->sink;
  while (iter)
   {
     char *opname = NULL;
     g_object_get (iter, "operation", &opname, NULL);
     if (iter == o->active)
       mrg_printf (mrg, "[%s]", opname);
     else
       mrg_printf (mrg, "%s", opname);
     mrg_printf (mrg, "\n");

     g_free (opname);
     iter = gegl_node_get_producer (iter, "input", NULL);
   }
}
#endif

static void ui_actions (State *o)
{
  Mrg *mrg = o->mrg;
  int i;
  cairo_t *cr = mrg_cr (mrg);
  mrg_set_edge_left (mrg, mrg_width (mrg) * 0.25);
  mrg_set_edge_top  (mrg, mrg_height (mrg) * 0.1);
  mrg_set_font_size (mrg, mrg_height (mrg) * 0.08);

  for (i = 0; actions[i].label; i ++)
    {
      mrg_text_listen (mrg, MRG_PRESS, activate_op_cb, o, &actions[i]);
      if (locate_node (o, actions[i].op_name))
        mrg_printf (mrg, "-");
      mrg_printf (mrg, "%s\n", actions[i].label);
      mrg_text_listen_done (mrg);
    }

  mrg_print (mrg, "\n");
  mrg_printf (mrg, "existing\n");
  mrg_text_listen (mrg, MRG_PRESS, discard_cb, o, NULL);
  mrg_printf (mrg, "discard\n");
  mrg_text_listen_done (mrg);
  cairo_scale (cr, mrg_width(mrg), mrg_height(mrg));
  cairo_new_path (cr);
  cairo_arc (cr, 0.9, 0.1, 0.1, 0.0, G_PI * 2);
  contrasty_stroke (cr);
  cairo_rectangle (cr, 0.8, 0.0, 0.2, 0.2); 
  mrg_listen (mrg, MRG_PRESS, toggle_actions_cb, o, NULL);
  cairo_new_path (cr);
}

static void dir_pgdn_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  o->u -= mrg_width (o->mrg) * 0.6; 
  mrg_queue_draw (o->mrg, NULL);
  mrg_event_stop_propagate (event);
}

static void dir_pgup_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  o->u += mrg_width (o->mrg) * 0.6; 
  mrg_queue_draw (o->mrg, NULL);
  mrg_event_stop_propagate (event);
}

static void entry_pressed (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  free (o->path);
  o->path = strdup (data2);
  load_path (o);
  mrg_queue_draw (event->mrg, NULL);
}

static void ui_dir_viewer (State *o)
{
  Mrg *mrg = o->mrg;
  cairo_t *cr = mrg_cr (mrg);
  GList *iter;
  float x = 0;
  float y = 0;
  float dim = mrg_height (mrg) * 0.25;

  cairo_rectangle (cr, 0,0, mrg_width(mrg), mrg_height(mrg));
  mrg_listen (mrg, MRG_MOTION, on_viewer_motion, o, NULL);
  cairo_new_path (cr);

  mrg_set_edge_right (mrg, 4095);
  cairo_save (cr);
  cairo_translate (cr, o->u, 0);//o->v);

  for (iter = o->paths; iter; iter=iter->next)
  {
      int w, h;
      gchar *path = iter->data;
      char *lastslash = strrchr (path, '/');

      gchar *p2 = suffix_path (path);

      gchar *thumbpath = get_thumb_path (p2);
      free (p2);
      if (access (thumbpath, F_OK) == -1)
      {
        g_free (thumbpath);
        thumbpath = get_thumb_path (path);
      }
  
      if (
         access (thumbpath, F_OK) != -1 && //XXX: query image should suffice
         mrg_query_image (mrg, thumbpath, &w, &h))
      {
        float wdim = dim;
        float hdim = dim;
        if (w > h)
          hdim = dim / (1.0 * w / h);
        else
          wdim = dim * (1.0 * w / h);

        mrg_image (mrg, x + (dim-wdim)/2, y + (dim-hdim)/2, wdim, hdim, thumbpath);
      }
      g_free (thumbpath);

      mrg_set_xy (mrg, x, y + dim - mrg_em(mrg));
      mrg_printf (mrg, "%s\n", lastslash+1);
      cairo_new_path (mrg_cr(mrg));
      cairo_rectangle (mrg_cr(mrg), x, y, dim, dim);
      mrg_listen_full (mrg, MRG_CLICK, entry_pressed, o, path, NULL, NULL);
      cairo_new_path (mrg_cr(mrg));

      y += dim;
      if (y+dim > mrg_height (mrg))
      {
        y = 0;
        x += dim;
      }
  }
  cairo_restore (cr);

  cairo_scale (cr, mrg_width(mrg), mrg_height(mrg));
  cairo_new_path (cr);
  cairo_move_to (cr, 0.2, 0.8);
  cairo_line_to (cr, 0.2, 1.0);
  cairo_line_to (cr, 0.0, 0.9);
  cairo_close_path (cr);
  if (o->show_controls)
    contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, 0.0, 0.8, 0.2, 0.2); 
  mrg_listen (mrg, MRG_PRESS, dir_pgup_cb, o, NULL);

  cairo_new_path (cr);

  cairo_move_to (cr, 0.8, 0.8);
  cairo_line_to (cr, 0.8, 1.0);
  cairo_line_to (cr, 1.0, 0.9);
  cairo_close_path (cr);

  if (o->show_controls)
    contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, 0.8, 0.8, 0.2, 0.2); 
  mrg_listen (mrg, MRG_PRESS, dir_pgdn_cb, o, NULL);
  cairo_new_path (cr);

  mrg_add_binding (mrg, "left", NULL, NULL, dir_pgup_cb, o);
  mrg_add_binding (mrg, "right", NULL, NULL, dir_pgdn_cb, o);
}

static int slide_cb (Mrg *mrg, void *data)
{
  State *o = data;
  o->slide_timeout = 0;
  go_next (data);
  return 0;
}

static void ui_viewer (State *o)
{
  Mrg *mrg = o->mrg;
  cairo_t *cr = mrg_cr (mrg);
  cairo_rectangle (cr, 0,0, mrg_width(mrg), mrg_height(mrg));
  mrg_listen (mrg, MRG_DRAG, on_pan_drag, o, NULL);
  mrg_listen (mrg, MRG_MOTION, on_viewer_motion, o, NULL);
  cairo_scale (cr, mrg_width(mrg), mrg_height(mrg));
  cairo_new_path (cr);
  cairo_rectangle (cr, 0.05, 0.05, 0.05, 0.05);
  cairo_rectangle (cr, 0.15, 0.05, 0.05, 0.05);
  cairo_rectangle (cr, 0.05, 0.15, 0.05, 0.05);
  cairo_rectangle (cr, 0.15, 0.15, 0.05, 0.05);
  if (o->show_controls)
    contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, 0.0, 0.0, 0.2, 0.2); 
  mrg_listen (mrg, MRG_PRESS, go_parent_cb, o, NULL);


  cairo_new_path (cr);
  cairo_move_to (cr, 0.2, 0.8);
  cairo_line_to (cr, 0.2, 1.0);
  cairo_line_to (cr, 0.0, 0.9);
  cairo_close_path (cr);
  if (o->show_controls)
    contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, 0.0, 0.8, 0.2, 0.2); 
  mrg_listen (mrg, MRG_PRESS, go_prev_cb, o, NULL);
  cairo_new_path (cr);

  cairo_move_to (cr, 0.8, 0.8);
  cairo_line_to (cr, 0.8, 1.0);
  cairo_line_to (cr, 1.0, 0.9);
  cairo_close_path (cr);

  if (o->show_controls)
    contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, 0.8, 0.8, 0.2, 0.2); 
  mrg_listen (mrg, MRG_PRESS, go_next_cb, o, NULL);
  cairo_new_path (cr);

  cairo_arc (cr, 0.9, 0.1, 0.1, 0.0, G_PI * 2);

  if (o->show_controls)
    contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, 0.8, 0.0, 0.2, 0.2); 
  mrg_listen (mrg, MRG_PRESS, toggle_actions_cb, o, NULL);
  cairo_new_path (cr);

  mrg_add_binding (mrg, "left", NULL, NULL,  pan_left_cb, o);
  mrg_add_binding (mrg, "right", NULL, NULL, pan_right_cb, o);
  mrg_add_binding (mrg, "up", NULL, NULL,    pan_up_cb, o);
  mrg_add_binding (mrg, "down", NULL, NULL,  pan_down_cb, o);
  mrg_add_binding (mrg, "+", NULL, NULL,     zoom_in_cb, o);
  mrg_add_binding (mrg, "=", NULL, NULL,     zoom_in_cb, o);
  mrg_add_binding (mrg, "-", NULL, NULL,     zoom_out_cb, o);
  mrg_add_binding (mrg, "1", NULL, NULL,     zoom_1_cb, o);
  mrg_add_binding (mrg, "m", NULL, NULL,     zoom_fit_cb, o);
  mrg_add_binding (mrg, "m", NULL, NULL,     zoom_fit_cb, o);
  mrg_add_binding (mrg, "x", NULL, NULL,     discard_cb, o);

  mrg_add_binding (mrg, "space", NULL, NULL,     go_next_cb , o);
  mrg_add_binding (mrg, "n", NULL, NULL,         go_next_cb, o);
  mrg_add_binding (mrg, "p", NULL, NULL,         go_prev_cb, o);
  mrg_add_binding (mrg, "backspace", NULL, NULL, go_prev_cb, o);

  if (o->slide_enabled && o->slide_timeout == 0)
  {
    o->slide_timeout = 
       mrg_add_timeout (o->mrg, o->slide_pause * 1000, slide_cb, o);
  }
}

static void toggle_show_controls_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  o->show_controls = !o->show_controls;
  mrg_queue_draw (o->mrg, NULL);
}

static void toggle_slideshow_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  o->slide_enabled = !o->slide_enabled;
  if (o->slide_timeout)
      mrg_remove_idle (o->mrg, o->slide_timeout);
  o->slide_timeout = 0;
  mrg_queue_draw (o->mrg, NULL);
}

static void gegl_ui (Mrg *mrg, void *data)
{
  State *o = data;

  if (o->is_video)
   {
     o->frame_no++;
     if (g_getenv ("GEGL_UI_DEBUG_SEEK"))
     {
       if ((o->frame_no / 200) % 2 == 1)
         o->frame_no+=600;
     }
     fprintf (stderr, "\r%i", o->frame_no);
     gegl_node_set (o->load, "frame", o->frame_no, NULL);
     mrg_queue_draw (o->mrg, NULL);
   }

  mrg_gegl_blit (mrg,
		 0, 0,
                 mrg_width (mrg), mrg_height (mrg),
		 o->sink,
		 o->u, o->v,
		 o->scale,
                 o->render_quality);

  if (o->is_video)
  {
    GeglAudioFragment *audio = NULL;
    gdouble fps;
    gegl_node_get (o->load, "audio", &audio, "frame-rate", &fps, NULL);
    if (audio)
    {
       int sample_count = gegl_audio_fragment_get_sample_count (audio);
       if (sample_count > 0)
       {
         int i;
         if (!audio_started)
         {
  	   open_audio (gegl_audio_fragment_get_sample_rate (audio));
           SDL_PauseAudio(0);
           audio_started = 1;
         }
         for (i = 0; i < sample_count; i++)
         {
           sdl_add_audio_sample (0, audio->data[0][i], audio->data[1][i]);
         }

         while (audio_len > audio_pos + 5000)
           g_usleep (50);
         o->prev_frame_played = o->frame_no;
       }
       g_object_unref (audio);
    }
  }
  
  if (o->show_controls)
  {
    mrg_printf (mrg, "%s\n", o->path);
#if DEBUG_OP_LIST
    ui_debug_op_chain (o);
#endif
  }

  if (o->show_actions)
  {
    ui_actions (o);
    mrg_add_binding (mrg, "return", NULL, NULL, toggle_actions_cb, o);
  }
  else if (o->active)
  {
    ui_active_op (o);
    mrg_add_binding (mrg, "return", NULL, NULL, apply_filter_cb, o);
    mrg_add_binding (mrg, "escape", NULL, NULL,     disable_filter_cb, o);
  }
  else
  {
    struct stat stat_buf;
    lstat (o->path, &stat_buf);
    if (S_ISREG (stat_buf.st_mode))
    {
      ui_viewer (o);

    }
    else if (S_ISDIR (stat_buf.st_mode))
    {
      ui_dir_viewer (o);
    }
    
    mrg_add_binding (mrg, "escape", NULL, NULL, go_parent_cb, o);
    mrg_add_binding (mrg, "return", NULL, NULL, toggle_actions_cb, o);
  }

  mrg_add_binding (mrg, "control-q", NULL, NULL, mrg_quit_cb, o);
  mrg_add_binding (mrg, "q", NULL, NULL,         mrg_quit_cb, o);
  mrg_add_binding (mrg, "f", NULL, NULL,         toggle_fullscreen_cb, o);
  mrg_add_binding (mrg, "F11", NULL, NULL,       toggle_fullscreen_cb, o);
  mrg_add_binding (mrg, "tab", NULL, NULL,       toggle_show_controls_cb, o);
  mrg_add_binding (mrg, "s", NULL, NULL,       toggle_slideshow_cb, o);

  mrg_add_binding (mrg, ",", NULL, NULL,         preview_less_cb, o);
  mrg_add_binding (mrg, ".", NULL, NULL,         preview_more_cb, o);
}

/***********************************************/

static char *get_path_parent (const char *path)
{
  char *ret = strdup (path);
  char *lastslash = strrchr (ret, '/');
  if (lastslash)
  {
    if (lastslash == ret)
      lastslash[1] = '\0';
    else
      lastslash[0] = '\0';
  }
  return ret;
}

static char *suffix_path (const char *path)
{
  char *ret, *last_dot;

  if (!path)
    return NULL;
  ret  = malloc (strlen (path) + strlen (suffix) + 3);
  strcpy (ret, path);
  last_dot = strrchr (ret, '.');
  if (last_dot)
  {
    char *extension = strdup (last_dot + 1);
    sprintf (last_dot, "%s.%s", suffix, extension);
   free (extension);
  }
  else
  {
    sprintf (ret, "%s%s", path, suffix);
  }
  return ret;
}

static char *unsuffix_path (const char *path)
{
  char *ret = NULL, *last_dot, *extension;
  char *suf;

  if (!path)
    return NULL;
  ret = malloc (strlen (path) + 4);
  strcpy (ret, path);
  last_dot = strrchr (ret, '.');
  extension = strdup (last_dot + 1);

  suf = strstr(ret, suffix);
  sprintf (suf, ".%s", extension);
  free (extension);
  return ret;
}

static int is_gegl_path (const char *path)
{
  if (strstr (path, suffix)) return 1;
  return 0;
}

static void contrasty_stroke (cairo_t *cr)
{
  double x0 = 6.0, y0 = 6.0;
  double x1 = 4.0, y1 = 4.0;

  cairo_device_to_user_distance (cr, &x0, &y0);
  cairo_device_to_user_distance (cr, &x1, &y1);
  cairo_set_source_rgba (cr, 0,0,0,0.5);
  cairo_set_line_width (cr, y0);
  cairo_stroke_preserve (cr);
  cairo_set_source_rgba (cr, 1,1,1,0.5);
  cairo_set_line_width (cr, y1);
  cairo_stroke (cr);
}

static unsigned char *copy_buf = NULL;
static int copy_buf_len = 0;

static void mrg_gegl_blit (Mrg *mrg,
			   float x0, float y0,
			   float width, float height,
			   GeglNode *node,
			   float u, float v,
			   float scale,
                           float preview_multiplier)
{
  float fake_factor = preview_multiplier;
  GeglRectangle bounds;

  cairo_t *cr = mrg_cr (mrg);
  cairo_surface_t *surface = NULL;

  if (!node)
    return;

  bounds = gegl_node_get_bounding_box (node);

  if (width == -1 && height == -1)
  {
    width  = bounds.width;
    height = bounds.height;
  }

  if (width == -1)
    width = bounds.width * height / bounds.height;
  if (height == -1)
    height = bounds.height * width / bounds.width;

  width /= fake_factor;
  height /= fake_factor;
  u /= fake_factor;
  v /= fake_factor;
 
  if (copy_buf_len < width * height * 4)
  {
    if (copy_buf)
      free (copy_buf);
    copy_buf_len = width * height * 4;
    copy_buf = malloc (copy_buf_len);
  }
  {
    static int foo = 0;
    unsigned char *buf = copy_buf;
    GeglRectangle roi = {u, v, width, height};
    static const Babl *fmt = NULL;

foo++;
    if (!fmt) fmt = babl_format ("cairo-RGB24");
    gegl_node_blit (node, scale / fake_factor, &roi, fmt, buf, width * 4, 
         GEGL_BLIT_DEFAULT);
  surface = cairo_image_surface_create_for_data (buf, CAIRO_FORMAT_RGB24, width, height, width * 4);
  }

  cairo_save (cr);
  cairo_surface_set_device_scale (surface, 1.0/fake_factor, 1.0/fake_factor);
  
  width *= fake_factor;
  height *= fake_factor;
  u *= fake_factor;
  v *= fake_factor;

  cairo_rectangle (cr, x0, y0, width, height);

  cairo_clip (cr);
  cairo_translate (cr, x0 * fake_factor, y0 * fake_factor);
  cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
  cairo_set_source_surface (cr, surface, 0, 0);
   
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  cairo_surface_destroy (surface);
  cairo_restore (cr);
}

static void load_path (State *o)
{
  char *path;
  char *meta;
  populate_path_list (o);
  if (is_gegl_path (o->path))
  {
    if (o->save_path)
      free (o->save_path);
    o->save_path = o->path;
    o->path = unsuffix_path (o->save_path);
  }
  else
  {
    if (o->save_path)
      free (o->save_path);
    o->save_path = suffix_path (o->path);
  }
  path  = o->path;

  if (access (o->save_path, F_OK) != -1)
    path = o->save_path;

  g_object_unref (o->gegl);
  o->gegl = NULL;
  o->sink = NULL;
  o->source = NULL;
  o->rev = 0;
  o->u = 0;
  o->v = 0;
  o->is_video = 0;
  o->frame_no = 0;
  o->prev_frame_played = 0;

  if (str_has_video_suffix (path))
  {
    o->is_video = 1;
    o->gegl = gegl_node_new ();
    o->sink = gegl_node_new_child (o->gegl,
                       "operation", "gegl:nop", NULL);
    o->source = gegl_node_new_child (o->gegl,
                       "operation", "gegl:nop", NULL);
    o->load = gegl_node_new_child (o->gegl,
  			      "operation", "gegl:ff-load", "path", path, "frame", o->frame_no,
			       NULL);
    gegl_node_link_many (o->load, o->source, o->sink, NULL);
  }
  else
  {
    meta = gegl_meta_get (path);
    if (meta)
    {
      GSList *nodes, *n;
      char *containing_path = get_path_parent (o->path);
      o->gegl = gegl_node_new_from_xml (meta, containing_path);
      free (containing_path);
      o->sink = gegl_node_new_child (o->gegl,
                       "operation", "gegl:nop", NULL);
      o->source = NULL;
      gegl_node_link_many (
        gegl_node_get_producer (o->gegl, "input", NULL), o->sink, NULL);
      nodes = gegl_node_get_children (o->gegl);
      for (n = nodes; n; n=n->next)
      {
        const char *op_name = gegl_node_get_operation (n->data);
        if (!strcmp (op_name, "gegl:load"))
        {
          GeglNode *load;
          gchar *path;
          gegl_node_get (n->data, "path", &path, NULL);
          load_into_buffer (o, path);
          gegl_node_set (n->data, "operation", "gegl:nop", NULL);
          o->source = n->data;
          load = gegl_node_new_child (o->gegl, "operation", "gegl:buffer-source",
                                               "buffer", o->buffer, NULL);
          gegl_node_link_many (load, o->source, NULL);
          g_free (path);
          break;
        }
      }
      o->save = gegl_node_new_child (o->gegl, "operation", "gegl:save",
                                              "path", path,
                                              NULL);
    }
    else
    {
      o->gegl = gegl_node_new ();
      o->sink = gegl_node_new_child (o->gegl,
                         "operation", "gegl:nop", NULL);
      o->source = gegl_node_new_child (o->gegl,
                         "operation", "gegl:nop", NULL);
      load_into_buffer (o, path);
      o->load = gegl_node_new_child (o->gegl,
                                     "operation", "gegl:buffer-source",
			             NULL);
      o->save = gegl_node_new_child (o->gegl,
                                     "operation", "gegl:save",
  		     "path", o->save_path,
  		     NULL);
    gegl_node_link_many (o->load, o->source, o->sink, NULL);
    gegl_node_set (o->load, "buffer", o->buffer, NULL);
  }
  }
  {
    struct stat stat_buf;
    lstat (o->path, &stat_buf);
    if (S_ISREG (stat_buf.st_mode))
    {
      if (o->is_video)
        center (o);
      else
        zoom_to_fit (o);
    }
  }

  if (o->ops)
  {
    int i;
    for (i = 0; o->ops[i]; i++)
    {
      o->active = gegl_node_new_child (o->gegl,
	"operation", o->ops[i], NULL);

      gegl_node_link_many (gegl_node_get_producer (o->sink, "input", NULL),
				  o->active,
				  o->sink,
				  NULL);
    }
  }

  mrg_queue_draw (o->mrg, NULL);
}

static void go_parent (State *o)
{
  char *lastslash = strrchr (o->path, '/');
  if (lastslash)
  {
    if (lastslash == o->path)
      lastslash[1] = '\0';
    else
      lastslash[0] = '\0';
    load_path (o);
    mrg_queue_draw (o->mrg, NULL);
  }
}

static void go_next (State *o)
{
  GList *curr = g_list_find_custom (o->paths, o->path, (void*)g_strcmp0);

  if (curr && curr->next)
  {
    free (o->path);
    o->path = strdup (curr->next->data);
    load_path (o);
    mrg_queue_draw (o->mrg, NULL);
  }
}

static void go_prev (State *o)
{
  GList *curr = g_list_find_custom (o->paths, o->path, (void*)g_strcmp0);

  if (curr && curr->prev)
  {
    free (o->path);
    o->path = strdup (curr->prev->data);
    load_path (o);
    mrg_queue_draw (o->mrg, NULL);
  }
}

static void go_next_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  if (o->rev)
    save_cb (event, data1, data2);
  go_next (data1);
  o->active = NULL;
  mrg_event_stop_propagate (event);
}

static void go_parent_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  if (o->rev)
    save_cb (event, data1, data2);
  go_parent (data1);
  o->active = NULL;
  mrg_event_stop_propagate (event);
}

static void go_prev_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  if (o->rev)
    save_cb (event, data1, data2);
  go_prev (data1);
  o->active = NULL;
  mrg_event_stop_propagate (event);
}

static void leave_editor (State *o)
{
  char *opname = NULL;
  g_object_get (o->active, "operation", &opname, NULL);
  if (!strcmp (opname, "gegl:crop"))
   {
     zoom_to_fit (o);
   }
}

static void drag_preview (MrgEvent *e)
{
  State *o = hack_state;
  static float old_factor = 1;
  switch (e->type)
  {
    case MRG_DRAG_PRESS:
      old_factor = o->render_quality;
      if (o->render_quality < o->preview_quality)
        o->render_quality = o->preview_quality;
      break;
    case MRG_DRAG_RELEASE:
      o->render_quality = old_factor;
      mrg_queue_draw (e->mrg, NULL);
      break;
    default:
    break;
  }
}

static void load_into_buffer (State *o, const char *path)
{
  GeglNode *gegl, *load, *sink;
  struct stat stat_buf;

  if (o->buffer)
  {
    g_object_unref (o->buffer);
    o->buffer = NULL;
  }

  lstat (path, &stat_buf);
  if (S_ISREG (stat_buf.st_mode))
  {


  gegl = gegl_node_new ();
  load = gegl_node_new_child (gegl, "operation", "gegl:load",
                                    "path", path,
                                    NULL);
  sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink",
                                    "buffer", &(o->buffer),
                                    NULL);
  gegl_node_link_many (load, sink, NULL);
  gegl_node_process (sink);
  g_object_unref (gegl);

  {
    GExiv2Orientation orientation = path_get_orientation (path);
    gboolean hflip = FALSE;
    gboolean vflip = FALSE;
    double degrees = 0.0;
    switch (orientation)
    {
      case GEXIV2_ORIENTATION_UNSPECIFIED:
      case GEXIV2_ORIENTATION_NORMAL:
        break;
      case GEXIV2_ORIENTATION_HFLIP: hflip=TRUE; break;
      case GEXIV2_ORIENTATION_VFLIP: vflip=TRUE; break;
      case GEXIV2_ORIENTATION_ROT_90: degrees = 90.0; break;
      case GEXIV2_ORIENTATION_ROT_90_HFLIP: degrees = 90.0; hflip=TRUE; break;
      case GEXIV2_ORIENTATION_ROT_90_VFLIP: degrees = 90.0; vflip=TRUE; break;
      case GEXIV2_ORIENTATION_ROT_180: degrees = 180.0; break;
      case GEXIV2_ORIENTATION_ROT_270: degrees = 270.0; break;
    }

    if (degrees != 0.0 || vflip || hflip)
     {
       /* XXX: deal with vflip/hflip */
       GeglBuffer *new_buffer = NULL;
       GeglNode *rotate;
       gegl = gegl_node_new ();
       load = gegl_node_new_child (gegl, "operation", "gegl:buffer-source",
                                   "buffer", o->buffer,
                                   NULL);
       sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink",
                                   "buffer", &(new_buffer),
                                   NULL);
       rotate = gegl_node_new_child (gegl, "operation", "gegl:rotate",
                                   "degrees", -degrees,
                                   "sampler", GEGL_SAMPLER_NEAREST,
                                   NULL);
       gegl_node_link_many (load, rotate, sink, NULL);
       gegl_node_process (sink);
       g_object_unref (gegl);
       g_object_unref (o->buffer);
       o->buffer = new_buffer;
     }
  }

#if 0 /* hack to see if having the data in some formats already is faster */
  {
  GeglBuffer *tempbuf;
  tempbuf = gegl_buffer_new (gegl_buffer_get_extent (o->buffer),
                                         babl_format ("RGBA float"));

  gegl_buffer_copy (o->buffer, NULL, GEGL_ABYSS_NONE, tempbuf, NULL);
  g_object_unref (o->buffer);
  o->buffer = tempbuf;
  }
#endif
    }
  else
    {
      GeglRectangle extent = {0,0,1,1}; /* segfaults with NULL / 0,0,0,0*/
      o->buffer = gegl_buffer_new (&extent, babl_format("R'G'B' u8"));
    }
}

static GeglNode *locate_node (State *o, const char *op_name)
{
  GeglNode *iter = o->sink;
  while (iter)
   {
     char *opname = NULL;
     g_object_get (iter, "operation", &opname, NULL);
     if (!strcmp (opname, op_name))
       return iter;
     g_free (opname);
     iter = gegl_node_get_producer (iter, "input", NULL);
   }
  return NULL;
}
static void zoom_to_fit (State *o)
{
  Mrg *mrg = o->mrg;
  GeglRectangle rect = gegl_node_get_bounding_box (o->sink);
  float scale, scale2;

  scale = 1.0 * mrg_width (mrg) / rect.width;
  scale2 = 1.0 * mrg_height (mrg) / rect.height;

  if (scale2 < scale) scale = scale2;

  o->scale = scale;

  o->u = -(mrg_width (mrg) - rect.width * o->scale) / 2;
  o->v = -(mrg_height (mrg) - rect.height * o->scale) / 2;
  o->u += rect.x * o->scale;
  o->v += rect.y * o->scale;

  mrg_queue_draw (mrg, NULL);
}
static void center (State *o)
{
  Mrg *mrg = o->mrg;
  GeglRectangle rect = gegl_node_get_bounding_box (o->sink);
  o->scale = 1.0;

  o->u = -(mrg_width (mrg) - rect.width * o->scale) / 2;
  o->v = -(mrg_height (mrg) - rect.height * o->scale) / 2;
  o->u += rect.x * o->scale;
  o->v += rect.y * o->scale;

  mrg_queue_draw (mrg, NULL);
}

static void zoom_to_fit_buffer (State *o)
{
  Mrg *mrg = o->mrg;
  GeglRectangle rect = *gegl_buffer_get_extent (o->buffer);
  float scale, scale2;

  scale = 1.0 * mrg_width (mrg) / rect.width;
  scale2 = 1.0 * mrg_height (mrg) / rect.height;

  if (scale2 < scale) scale = scale2;

  o->scale = scale;
  o->u = -(mrg_width (mrg) - rect.width * o->scale) / 2;
  o->v = -(mrg_height (mrg) - rect.height * o->scale) / 2;
  o->u += rect.x * o->scale;
  o->v += rect.y * o->scale;


  mrg_queue_draw (mrg, NULL);
}

static void zoom_fit_cb (MrgEvent *e, void *data1, void *data2)
{
  zoom_to_fit (data1);
}

static int deferred_zoom_to_fit (Mrg *mrg, void *data)
{
  zoom_to_fit (data);
  return 0;
}

static void pan_left_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  float amount = mrg_width (event->mrg) * 0.1;
  o->u = o->u - amount;
  mrg_queue_draw (o->mrg, NULL);
}  

static void pan_right_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  float amount = mrg_width (event->mrg) * 0.1;
  o->u = o->u + amount;
  mrg_queue_draw (o->mrg, NULL);
}  

static void pan_down_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  float amount = mrg_width (event->mrg) * 0.1;
  o->v = o->v + amount;
  mrg_queue_draw (o->mrg, NULL);
}  

static void pan_up_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  float amount = mrg_width (event->mrg) * 0.1;
  o->v = o->v - amount;
  mrg_queue_draw (o->mrg, NULL);
}  

static void get_coords (State *o, float screen_x, float screen_y, float *gegl_x, float *gegl_y)
{
  float scale = o->scale;
  *gegl_x = (o->u + screen_x) / scale;
  *gegl_y = (o->v + screen_y) / scale;
}

static void preview_more_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1; 
  o->render_quality *= 2;
  mrg_queue_draw (o->mrg, NULL);
}

static void preview_less_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1; 
  o->render_quality /= 2;
  if (o->render_quality <= 1.0)
    o->render_quality = 1.0;
  mrg_queue_draw (o->mrg, NULL);
}

static void zoom_1_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1; 
  float x, y;
  get_coords (o, mrg_width(o->mrg)/2, mrg_height(o->mrg)/2, &x, &y);
  o->scale = 1.0;
  o->u = x * o->scale - mrg_width(o->mrg)/2;
  o->v = y * o->scale - mrg_height(o->mrg)/2;
  mrg_queue_draw (o->mrg, NULL);
}

static void zoom_in_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1; 
  float x, y;
  get_coords (o, mrg_width(o->mrg)/2, mrg_height(o->mrg)/2, &x, &y);
  o->scale = o->scale * 1.1;  
  o->u = x * o->scale - mrg_width(o->mrg)/2;
  o->v = y * o->scale - mrg_height(o->mrg)/2;
  mrg_queue_draw (o->mrg, NULL);
}

static void zoom_out_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  float x, y;
  get_coords (o, mrg_width(o->mrg)/2, mrg_height(o->mrg)/2, &x, &y);
  o->scale = o->scale / 1.1;  
  o->u = x * o->scale - mrg_width(o->mrg)/2;
  o->v = y * o->scale - mrg_height(o->mrg)/2;
  mrg_queue_draw (o->mrg, NULL);
}

static void toggle_actions_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  o->show_actions = !o->show_actions;
  mrg_queue_draw (o->mrg, NULL);
}

static void toggle_fullscreen_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  mrg_set_fullscreen (event->mrg, !mrg_is_fullscreen (event->mrg));
  mrg_event_stop_propagate (event);
  mrg_add_timeout (event->mrg, 250, deferred_zoom_to_fit, o);
}

static void activate_op_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  GeglNode *found;
  ActionData *ad = data2;
  o->show_actions = 0;
  o->rev ++;
  found = locate_node (o, ad->op_name);
  if (found)
    {
       o->active = found;
       if (!strcmp (ad->op_name, "gegl:crop"))
         o->rotate = locate_node (o, "gegl:rotate");
    }
  else
    {
      if (!strcmp (ad->op_name, "gegl:rotate"))
      {
        const GeglRectangle *extent = gegl_buffer_get_extent (o->buffer);
        o->active = gegl_node_new_child (o->gegl,
          "operation", ad->op_name, NULL);
           gegl_node_set (o->active, "origin-x", extent->width * 0.5,
                                     "origin-y", extent->height * 0.5,
                                     "degrees", 0.0,
                                     NULL);
      } else if (!strcmp (ad->op_name, "gegl:crop"))
      {
         const GeglRectangle *extent = gegl_buffer_get_extent (o->buffer);
         o->active = gegl_node_new_child (o->gegl,
            "operation", ad->op_name, NULL);
            gegl_node_set (o->active, "x", 0.0,
                                      "y", 0.0,
                                      "width", extent->width * 1.0,
                                      "height", extent->height * 1.0,
			              NULL);
        o->rotate = gegl_node_new_child (o->gegl,
          "operation", "gegl:rotate", NULL);
           gegl_node_set (o->rotate, "origin-x", extent->width * 0.5,
                                     "origin-y", extent->height * 0.5,
                                     "degrees", 0.0,
                                     NULL);

       gegl_node_link_many (gegl_node_get_producer (o->sink, "input", NULL),
                              o->rotate,
                              o->sink,
                              NULL);

       } else
       {
          o->active = gegl_node_new_child (o->gegl, "operation", ad->op_name, NULL);
       }

       gegl_node_link_many (gegl_node_get_producer (o->sink, "input", NULL),
                              o->active,
                              o->sink,
                              NULL);
  }
  mrg_queue_draw (o->mrg, NULL);
}

static void disable_filter_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  GeglNode *iter = o->sink;
  GeglNode *prev = NULL;
  GeglNode *next = NULL;
  while (iter)
   {
     prev = iter;
     iter = gegl_node_get_producer (iter, "input", NULL);
     if (o->active == iter)
     {
       next = gegl_node_get_producer (iter, "input", NULL);
       break;
     }
   }
  gegl_node_link_many (next, prev, NULL);
  leave_editor (o);
  gegl_node_remove_child (o->gegl, o->active);
  o->active = NULL;
  mrg_queue_draw (o->mrg, NULL);
}

static void  apply_filter_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  leave_editor (o);
  o->active = NULL;
  mrg_queue_draw (o->mrg, NULL);
}

static void discard_cb (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  char *old_path = strdup (o->path);
  char *tmp;
  char *lastslash;
  go_next_cb (event, data1, data2); 
  if (!strcmp (old_path, o->path))
   {
     go_prev_cb (event, data1, data2);
   }
  tmp = strdup (old_path);
  lastslash  = strrchr (tmp, '/');
  if (lastslash)
  {
    char command[2048];
    char *suffixed = suffix_path (old_path);
    if (lastslash == tmp)
      lastslash[1] = '\0';
    else
      lastslash[0] = '\0';

    sprintf (command, "mkdir %s/.discard > /dev/null 2>&1", tmp);
    system (command);
    sprintf (command, "mv %s %s/.discard", old_path, tmp);
    sprintf (command, "mv %s %s/.discard", suffixed, tmp);
    system (command);
    free (suffixed);
  }
  free (tmp);
  free (old_path);
}

static void save_cb (MrgEvent *event, void *data1, void *data2)
{
  GeglNode *load;
  State *o = data1;
  gchar *path;
  char *xml;

  gegl_node_link_many (o->sink, o->save, NULL);
  gegl_node_process (o->save);
  gegl_node_get (o->save, "path", &path, NULL);
  fprintf (stderr, "saved to %s\n", path);

  load = gegl_node_new_child (o->gegl, "operation", "gegl:load",
                                    "path", o->path,
                                    NULL);
  gegl_node_link_many (load, o->source, NULL);
  {
    char *containing_path = get_path_parent (o->path);
    xml = gegl_node_to_xml (o->sink, containing_path);
    free (containing_path);
  }
  gegl_node_remove_child (o->gegl, load);
  gegl_node_link_many (o->load, o->source, NULL);
  gegl_meta_set (path, xml);
  g_free (xml);
  o->rev = 0;
}

#if 0
void gegl_node_defaults (GeglNode *node)
{
  const gchar* op_name = gegl_node_get_operation (node);
  {
    guint n_props;
    GParamSpec **pspecs = gegl_operation_list_properties (op_name, &n_props);
    if (pspecs)
    {
      for (gint i = 0; i < n_props; i++)
      {
        if (g_type_is_a (pspecs[i]->value_type, G_TYPE_DOUBLE))
        {
          GParamSpecDouble    *pspec = (void*)pspecs[i];
          gegl_node_set (node, pspecs[i]->name, pspec->default_value, NULL);
        }
        else if (g_type_is_a (pspecs[i]->value_type, G_TYPE_INT))
        {
          GParamSpecInt *pspec = (void*)pspecs[i];
          gegl_node_set (node, pspecs[i]->name, pspec->default_value, NULL);
        }
        else if (g_type_is_a (pspecs[i]->value_type, G_TYPE_STRING))
        {
          GParamSpecString *pspec = (void*)pspecs[i];
          gegl_node_set (node, pspecs[i]->name, pspec->default_value, NULL);
        }
      }
      g_free (pspecs);
    }
  }
}
#endif

/* loads the source image corresponding to o->path into o->buffer and
 * creates live gegl pipeline, or nops.. rigs up o->save_path to be
 * the location where default saves ends up.
 */
void
gegl_meta_set (const char *path,
               const char *meta_data)
{
  GError *error = NULL;
  GExiv2Metadata *e2m = gexiv2_metadata_new ();
  gexiv2_metadata_open_path (e2m, path, &error);
  if (error)
  {
    g_warning ("%s", error->message);
  }
  else
  {
    if (gexiv2_metadata_has_tag (e2m, "Xmp.xmp.GEGL"))
      gexiv2_metadata_clear_tag (e2m, "Xmp.xmp.GEGL");

    gexiv2_metadata_set_tag_string (e2m, "Xmp.xmp.GEGL", meta_data);
    gexiv2_metadata_save_file (e2m, path, &error);
    if (error)
      g_warning ("%s", error->message);
  }
  gexiv2_metadata_free (e2m);
}

char *
gegl_meta_get (const char *path)
{
  gchar  *ret   = NULL;
  GError *error = NULL;
  GExiv2Metadata *e2m = gexiv2_metadata_new ();
  gexiv2_metadata_open_path (e2m, path, &error);
  if (!error)
    ret = gexiv2_metadata_get_tag_string (e2m, "Xmp.xmp.GEGL");
  /*else
    g_warning ("%s", error->message);*/
  gexiv2_metadata_free (e2m);
  return ret;
}

GExiv2Orientation path_get_orientation (const char *path)
{
  GExiv2Orientation ret = 0;
  GError *error = NULL;
  GExiv2Metadata *e2m = gexiv2_metadata_new ();
  gexiv2_metadata_open_path (e2m, path, &error);
  if (!error)
    ret = gexiv2_metadata_get_orientation (e2m);
  /*else
    g_warning ("%s", error->message);*/
  gexiv2_metadata_free (e2m);
  return ret;
}


#endif
