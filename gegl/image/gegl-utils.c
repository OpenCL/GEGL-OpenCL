#include "gegl-utils.h"
#include "gegl-types.h"

void gegl_log (GLogLevelFlags level, gchar * file, gint line,
	       gchar * function, gchar * format, ...);
void gegl_logv (GLogLevelFlags level, gchar * file, gint line,
		gchar * function, gchar * format, va_list args);
void gegl_direct_log (GLogLevelFlags level, gchar * format, ...);
void gegl_direct_logv (GLogLevelFlags level, gchar * format, va_list args);

inline gint
_gegl_float_epsilon_zero (float value)
{
  return value > -GEGL_FLOAT_EPSILON && value < GEGL_FLOAT_EPSILON;
}

inline gint
_gegl_float_epsilon_equal (float v1, float v2)
{
  register float diff = v1 - v2;
  return diff > -GEGL_FLOAT_EPSILON && diff < GEGL_FLOAT_EPSILON;
}

void
gegl_rect_set (GeglRect * r, gint x, gint y, guint w, guint h)
{
  r->x = x;
  r->y = y;
  r->w = w;
  r->h = h;
}

void
gegl_rect_bounding_box (GeglRect * dest, GeglRect * src1, GeglRect * src2)
{
  gboolean s1_has_area = src1->w && src1->h;
  gboolean s2_has_area = src2->w && src2->h;

  if (!s1_has_area && !s2_has_area)
    gegl_rect_set (dest, 0, 0, 0, 0);
  else if (!s1_has_area)
    gegl_rect_copy (dest, src2);
  else if (!s2_has_area)
    gegl_rect_copy (dest, src1);

  {
    gint x1 = MIN (src1->x, src2->x);
    gint x2 = MAX (src1->x + src1->w, src2->x + src2->w);
    gint y1 = MIN (src1->y, src2->y);
    gint y2 = MAX (src1->y + src1->h, src2->y + src2->h);
    dest->x = x1;
    dest->y = y1;
    dest->w = x2 - x1;
    dest->h = y2 - y1;
  }
}

gboolean
gegl_rect_intersect (GeglRect * dest, GeglRect * src1, GeglRect * src2)
{
  gint x1, x2, y1, y2;

  x1 = MAX (src1->x, src2->x);
  x2 = MIN (src1->x + src1->w, src2->x + src2->w);

  if (x2 <= x1)
    {
      gegl_rect_set (dest, 0, 0, 0, 0);
      return FALSE;
    }

  y1 = MAX (src1->y, src2->y);
  y2 = MIN (src1->y + src1->h, src2->y + src2->h);

  if (y2 <= y1)
    {
      gegl_rect_set (dest, 0, 0, 0, 0);
      return FALSE;
    }

  dest->x = x1;
  dest->y = y1;
  dest->w = x2 - x1;
  dest->h = y2 - y1;
  return TRUE;
}

void
gegl_rect_copy (GeglRect * to, GeglRect * from)
{
  to->x = from->x;
  to->y = from->y;
  to->w = from->w;
  to->h = from->h;
}

gboolean
gegl_rect_contains (GeglRect * r, GeglRect * s)
{
  if (s->x >= r->x &&
      s->y >= r->y &&
      (s->x + s->w) <= (r->x + r->w) && (s->y + s->h) <= (r->y + r->h))
    return TRUE;
  else
    return FALSE;
}

gboolean
gegl_rect_equal (GeglRect * r, GeglRect * s)
{
  if (r->x == s->x && r->y == s->y && r->w == s->w && r->h == s->h)
    return TRUE;
  else
    return FALSE;
}

gboolean
gegl_rect_equal_coords (GeglRect * r, gint x, gint y, gint w, gint h)
{
  if (r->x == x && r->y == y && r->w == w && r->h == h)
    return TRUE;
  else
    return FALSE;
}

#define GEGL_LOG_DOMAIN "Gegl"

void
gegl_log_debug (gchar * file,
		gint line, gchar * function, gchar * format, ...)
{
  va_list args;
  va_start (args, format);
  gegl_logv (G_LOG_LEVEL_DEBUG, file, line, function, format, args);
  va_end (args);
}

void
gegl_log_info (gchar * file, gint line, gchar * function, gchar * format, ...)
{
  va_list args;
  va_start (args, format);
  gegl_logv (G_LOG_LEVEL_INFO, file, line, function, format, args);
  va_end (args);
}

void
gegl_log_message (gchar * file,
		  gint line, gchar * function, gchar * format, ...)
{
  va_list args;
  va_start (args, format);
  gegl_logv (G_LOG_LEVEL_MESSAGE, file, line, function, format, args);
  va_end (args);
}

void
gegl_log_direct (gchar * format, ...)
{
  va_list args;
  va_start (args, format);
  gegl_direct_logv (G_LOG_LEVEL_DEBUG, format, args);
  va_end (args);
}

void
gegl_log (GLogLevelFlags level,
	  gchar * file, gint line, gchar * function, gchar * format, ...)
{
  va_list args;
  va_start (args, format);
  gegl_logv (level, file, line, function, format, args);
  va_end (args);
}

void
gegl_logv (GLogLevelFlags level,
	   gchar * file,
	   gint line, gchar * function, gchar * format, va_list args)
{
  if (g_getenv ("GEGL_LOG_ON"))
    {
      gchar *tabbed = NULL;

      /* log the file and line */
      g_log (GEGL_LOG_DOMAIN, level, "%s:  %s:%d:", function, file, line);

      /* move the regular output over a bit. */
      tabbed = g_strconcat ("   ", format, NULL);
      g_logv (GEGL_LOG_DOMAIN, level, tabbed, args);
      g_log (GEGL_LOG_DOMAIN, level, "        ");
      g_free (tabbed);
    }
}

void
gegl_direct_log (GLogLevelFlags level, gchar * format, ...)
{
  va_list args;
  va_start (args, format);
  gegl_direct_logv (level, format, args);
  va_end (args);
}

void
gegl_direct_logv (GLogLevelFlags level, gchar * format, va_list args)
{
  if (g_getenv ("GEGL_LOG_ON"))
    {
      gchar *tabbed = NULL;
      tabbed = g_strconcat ("   ", format, NULL);
      g_logv (GEGL_LOG_DOMAIN, level, tabbed, args);
      g_free (tabbed);
    }
}
