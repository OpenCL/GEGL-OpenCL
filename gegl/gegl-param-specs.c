#include    "gegl-param-specs.h"
#include    "gegl-utils.h"
#include    "gegl-color-model.h"
#include    "gegl-tile.h"

GType *gegl_param_spec_types = NULL;

/* --- param spec functions --- */

static void
param_tile_init (GParamSpec *spec)
{
  GeglParamSpecTile *tspec = GEGL_PARAM_SPEC_TILE(spec);
  gegl_rect_set(&tspec->rect,0,0,0,0);
  tspec->color_model = NULL;
}

static void
param_tile_set_default (GParamSpec *pspec,
			            GValue     *value)
{
  value->data[0].v_pointer = NULL;
}

static gboolean
param_tile_validate (GParamSpec *pspec,
		       GValue     *value)
{
  GeglParamSpecTile *tspec = GEGL_PARAM_SPEC_TILE (pspec);
  GeglTile *tile = value->data[0].v_pointer;
  guint changed = 0;
  
  if (tile && !g_value_type_compatible (G_OBJECT_TYPE (tile), G_PARAM_SPEC_VALUE_TYPE (tspec)))
    {
      g_object_unref (tile);
      value->data[0].v_pointer = NULL;
      changed++;
    }
  
  return changed;
}

static gint
param_tile_values_cmp (GParamSpec   *pspec,
                         const GValue *value1,
                         const GValue *value2)
{
  guint8 *p1 = value1->data[0].v_pointer;
  guint8 *p2 = value2->data[0].v_pointer;

  return p1 < p2 ? -1 : p1 > p2;
}


void
gegl_param_spec_types_init (void)
{
  const guint n_types = 1;
  GType type, *spec_types, *spec_types_bound;

  gegl_param_spec_types = g_new0 (GType, n_types);
  spec_types = gegl_param_spec_types;
  spec_types_bound = gegl_param_spec_types + n_types;

  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecTile),/* instance_size */
        16,                        /* n_preallocs */
        param_tile_init,           /* instance_init */
        0x0,                       /* value_type */
        NULL,                      /* finalize */
        param_tile_set_default,    /* value_set_default */
        param_tile_validate,       /* value_validate */
        param_tile_values_cmp,     /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_TILE;
    type = g_param_type_register_static ("GeglParamTile", &pspec_info);
    *spec_types++ = type;
    g_assert (type == GEGL_TYPE_PARAM_TILE);
  }

  g_assert (spec_types == spec_types_bound);
}

GParamSpec*
gegl_param_spec_tile (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      GeglRect *rect,
                      GeglColorModel *color_model,
                      GParamFlags  flags)
{
  GeglParamSpecTile *tspec;

  tspec = g_param_spec_internal (GEGL_TYPE_PARAM_TILE, name, nick, blurb, flags);
  
  gegl_rect_copy(&tspec->rect, rect);
  tspec->color_model = color_model;
  
  return G_PARAM_SPEC (tspec);
}
