/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås
 */

#include "config.h"
#include <glib.h>
#include <string.h>
#include "gegl-instrument.h"

long babl_ticks (void);
long gegl_ticks (void)
{
  return babl_ticks ();
}

typedef struct _Timing Timing;

struct _Timing
{
  gchar  *name;
  long    usecs;
  Timing *parent;
  Timing *children;
  Timing *next;
};

Timing *root = NULL;

static Timing *iter_next (Timing *iter)
{
  if (iter->children)
    {
      iter = iter->children;
    }
  else if (iter->next)
    {
      iter = iter->next;
    }
  else
    {
      while (iter && !iter->next)
        iter = iter->parent;
      if (iter && iter->next)
        iter = iter->next;
      else
        return NULL;
    }
  return iter;
}

static gint timing_depth (Timing *timing)
{
  Timing *iter = timing;
  gint    ret  = 0;

  while (iter &&
         iter->parent)
    {
      ret++;
      iter = iter->parent;
    }

  return ret;
}

static Timing *timing_find (Timing      *root,
                            const gchar *name)
{
  Timing *iter = root;

  if (!iter)
    return NULL;

  while (iter)
    {
      if (!strcmp (iter->name, name))
        return iter;
      if (timing_depth (iter_next (iter)) <= timing_depth (root))
        return NULL;
      iter = iter_next (iter);
    }
  return NULL;
}

void
gegl_instrument (const gchar *parent_name,
                 const gchar *name,
                 long         usecs)
{
  Timing *iter;
  Timing *parent;

  if (root == NULL)
    {
      root       = g_slice_new0 (Timing);
      root->name = g_strdup (parent_name);
    }
  parent = timing_find (root, parent_name);
  if (!parent)
    {
      gegl_instrument (root->name, parent_name, 0);
      parent = timing_find (root, parent_name);
    }
  g_assert (parent);
  iter = timing_find (parent, name);
  if (!iter)
    {
      iter             = g_slice_new0 (Timing);
      iter->name       = g_strdup (name);
      iter->parent     = parent;
      iter->next       = parent->children;
      parent->children = iter;
    }
  iter->usecs += usecs;
}


static glong timing_child_sum (Timing *timing)
{
  Timing *iter = timing->children;
  glong   sum  = 0;

  while (iter)
    {
      sum += iter->usecs;
      iter = iter->next;
    }

  return sum;
}

static glong timing_other (Timing *timing)
{
  if (timing->children)
    return timing->usecs - timing_child_sum (timing);
  return 0;
}

static float seconds (glong usecs)
{
  return usecs / 1000000.0;
}

static float normalized (glong usecs)
{
  return 1.0 * usecs / root->usecs;
}

#include <string.h>

static GString *
tab_to (GString *string, gint position)
{
  gchar *p;
  gint   curcount = 0;
  gint   i;

  p = strrchr (string->str, '\n');
  if (!p)
    {
      p = string->str;
      curcount++;
    }
  while (p && *p != '\0')
    {
      curcount++;
      p++;
    }

  if (curcount > position && position != 0)
    {
      g_warning ("%s tab overflow %i>%i", G_STRLOC, curcount, position);
    }
  else
    {
      for (i = 0; i <= position - curcount; i++)
        string = g_string_append (string, " ");
    }
  return string;
}

static gchar *eight[] = {
  " ",
  "▏",
  "▍",
  "▌",
  "▋",
  "▊",
  "▉",
  "█"
};

static GString *
bar (GString *string, gint width, gfloat value)
{
  gboolean utf8 = TRUE;
  gint     i;

  if (value < 0)
    return string;
  if (utf8)
    {
      gint blocks = width * 8 * value;

      for (i = 0; i < blocks / 8; i++)
        {
          string = g_string_append (string, "█");
        }
      string = g_string_append (string, eight[blocks % 8]);
    }
  else
    {
      for (i = 0; i < width; i++)
        {
          if (width * value > i)
            string = g_string_append (string, "X");
        }
    }
  return string;
}

#define INDENT_SPACES    2
#define SECONDS_COL      29
#define BAR_COL          36
#define BAR_WIDTH        (78 - BAR_COL)

static void
sort_children (Timing *parent)
{
  Timing  *iter;
  Timing  *prev;
  gboolean changed = FALSE;

  do
    {
      iter    = parent->children;
      changed = FALSE;
      prev    = NULL;
      while (iter && iter->next)
        {
          Timing *next = iter->next;

          if (next->usecs > iter->usecs)
            {
              changed = TRUE;
              if (prev)
                {
                  prev->next = next;
                  iter->next = next->next;
                  next->next = iter;
                }
              else
                {
                  iter->next       = next->next;
                  next->next       = iter;
                  parent->children = next;
                }
            }
          prev = iter;
          iter = iter->next;
        }
    } while (changed);


  iter = parent->children;
  while (iter && iter->next)
    {
      sort_children (iter);
      iter = iter->next;
    }
}

gchar *
gegl_instrument_utf8 (void)
{
  GString *s = g_string_new ("");
  gchar   *ret;
  Timing  *iter = root;

  sort_children (root);

  while (iter)
    {
      gchar *buf;

      if (!strcmp (iter->name, root->name))
        {
          buf = g_strdup_printf ("Total time: %.3fs\n", seconds (iter->usecs));
          s   = g_string_append (s, buf);
          g_free (buf);
        }

      s = tab_to (s, timing_depth (iter) * INDENT_SPACES);
      s = g_string_append (s, iter->name);

      s   = tab_to (s, SECONDS_COL);
      buf = g_strdup_printf ("%5.1f%%", iter->parent ? 100.0 * iter->usecs / iter->parent->usecs : 100.0);
      s   = g_string_append (s, buf);
      g_free (buf);
      s = tab_to (s, BAR_COL);
      s = bar (s, BAR_WIDTH, normalized (iter->usecs));
      s = g_string_append (s, "\n");

      if (timing_depth (iter_next (iter)) < timing_depth (iter))
        {
          if (timing_other (iter->parent) > 0)
            {
              s   = tab_to (s, timing_depth (iter) * INDENT_SPACES);
              s   = g_string_append (s, "other");
              s   = tab_to (s, SECONDS_COL);
              buf = g_strdup_printf ("%5.1f%%", 100 * normalized (timing_other (iter->parent)));
              s   = g_string_append (s, buf);
              g_free (buf);
              s = tab_to (s, BAR_COL);
              s = bar (s, BAR_WIDTH, normalized (timing_other (iter->parent)));
              s = g_string_append (s, "\n");
            }
          s = g_string_append (s, "\n");
        }

      iter = iter_next (iter);
    }

  ret = g_strdup (s->str);
  g_string_free (s, TRUE);
  return ret;
}
