#include "gegl.h"
#include "gegl-buffer-private.h"

#include <stdio.h>

#define SUCCESS  0
#define FAILURE -1

static gboolean
assert_is_empty (GeglBuffer *buf,
                 gint        x,
                 gint        y,
                 gpointer    shared_data)
{
  gboolean  result = TRUE;
  GeglTile *tile = gegl_tile_source_get_tile (GEGL_TILE_SOURCE (buf), x, y, 0);

  if (!tile->is_zero_tile)
    {
      g_warning ("Not a zero tile");
      result = FALSE;
    }
  if (gegl_tile_get_data(tile) != shared_data)
    {
      g_warning ("Not using shared data");
      result = FALSE;
    }

  gegl_tile_unref (tile);

  return result;
}

static gboolean
assert_is_unshared (GeglBuffer *buf,
                    gint        x,
                    gint        y,
                    gpointer    shared_data)
{
  gboolean  result = TRUE;
  GeglTile *tile = gegl_tile_source_get_tile (GEGL_TILE_SOURCE (buf), x, y, 0);

  if (!tile->is_zero_tile)
    {
      g_warning ("Not a zero tile");
      result = FALSE;
    }
  if (gegl_tile_get_data(tile) == shared_data)
    {
      g_warning ("Using shared data");
      result = FALSE;
    }

  gegl_tile_unref (tile);

  return result;
}

int main(int argc, char **argv)
{
  GeglTile *tile;
  GeglBuffer *buf_a, *buf_b, *buf_small_lin, *buf_big_lin;
  gpointer shared_data = NULL;
  gboolean result = TRUE;
  gpointer scratch_data;
  GeglRectangle buffer_rect = *GEGL_RECTANGLE(0, 0, 128, 128);

  gegl_init (&argc, &argv);

  buf_a = gegl_buffer_new (&buffer_rect, babl_format("RGBA u8"));
  buf_b = gegl_buffer_new (&buffer_rect, babl_format("RGBA u8"));
  buf_small_lin = gegl_buffer_linear_new (&buffer_rect, babl_format("RGBA float"));
  buf_big_lin = gegl_buffer_linear_new (GEGL_RECTANGLE(0, 0, 1024, 1024), babl_format("RGBA float"));

  tile = gegl_tile_source_get_tile (GEGL_TILE_SOURCE (buf_a), 0, 0, 0);
  shared_data = gegl_tile_get_data(tile);
  gegl_tile_unref (tile);

  if (!assert_is_empty (buf_a, 0, 0, shared_data))
    result = FALSE;
  if (!assert_is_empty (buf_b, 0, 1, shared_data))
    result = FALSE;

  if (!assert_is_empty (buf_a, 0, 0, shared_data))
    result = FALSE;
  if (!assert_is_empty (buf_b, 0, 1, shared_data))
    result = FALSE;

  if (!assert_is_empty (buf_small_lin, 0, 0, shared_data))
    result = FALSE;

  if (!assert_is_unshared (buf_big_lin, 0, 0, shared_data))
    result = FALSE;

  scratch_data = gegl_malloc(4 * buffer_rect.width * buffer_rect.height);
  gegl_buffer_get (buf_a, &buffer_rect, 1.0, babl_format("RGBA u8"), scratch_data, 0, GEGL_ABYSS_NONE);
  gegl_buffer_get (buf_b, &buffer_rect, 1.0, babl_format("RGBA u8"), scratch_data, 0, GEGL_ABYSS_NONE);
  gegl_buffer_get (buf_small_lin, &buffer_rect, 1.0, babl_format("RGBA u8"), scratch_data, 0, GEGL_ABYSS_NONE);
  gegl_buffer_get (buf_big_lin, &buffer_rect, 1.0, babl_format("RGBA u8"), scratch_data, 0, GEGL_ABYSS_NONE);
  gegl_free (scratch_data);

  g_object_unref(buf_a);
  g_object_unref(buf_b);
  g_object_unref(buf_small_lin);
  g_object_unref(buf_big_lin);

  gegl_exit();

  if (result)
    return SUCCESS;
  return FAILURE;
}