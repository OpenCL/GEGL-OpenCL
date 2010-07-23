/*
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
 * Copyright (C) 2010 Martin Nordholts
 */

#include "config.h"

#include "gegl.h"
#include "gegl-plugin.h"

#define SUCCESS  0
#define FAILURE -1

static int
test_misc_case_insensitive_extension_handler (void)
{
  gint result = SUCCESS;
  const gchar *handler = "gegl:foo-handler";
  const gchar *lowercase = "fooext";
  const gchar *uppercase = "FOOEXT";
  const gchar *received_handler = NULL;

  gegl_extension_handler_register (lowercase, handler);

  /* Make sure comparisions are case insensitive */
  received_handler = gegl_extension_handler_get (uppercase);
  if (! strcmp (received_handler, handler) == 0)
    result = FAILURE;

  return result;
}

static int
test_misc_save_handler (void)
{
  gint result = SUCCESS;
  const gchar *handler = "gegl:bar-handler";
  const gchar *ext = "barext";
  const gchar *received_handler = NULL;

  gegl_extension_handler_register_saver (ext, handler);
  received_handler = gegl_extension_handler_get_saver (ext);
  if (! strcmp (received_handler, handler) == 0)
    result = FAILURE;
  
  return result;
}


int main(int argc, char *argv[])
{
  gint result = SUCCESS;

  if (result == SUCCESS)
    result = test_misc_case_insensitive_extension_handler ();

  if (result == SUCCESS)
    result = test_misc_save_handler ();

  return result;
}
