/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003-2004 Daniel S. Rogers
 *
 */

#include <string.h>

#include <glib-object.h>

#include "image/gegl-image-types.h"

#include "image/gegl-entry-record.h"
#include "gegl-image-mock-cache-entry.h"

#include "ctest.h"
#include "csuite.h"

static void
test_gegl_entry_record_new (Test * test)
{
  GeglMockCacheEntry * entry;
  GeglEntryRecord * record;

  entry = gegl_mock_cache_entry_new (1024);
  record = gegl_entry_record_new (NULL, GEGL_CACHE_ENTRY(entry));
  ct_test(test, record != NULL);
  gegl_entry_record_free (record);
  g_object_unref (entry);
}


Test *
create_entry_record_test()
{
  Test* t = ct_create("GeglEntryRecordTest");

  /*
    g_assert(ct_addSetUp(t, setup));
    g_assert(ct_addTearDown(t, teardown));
  */
  g_assert(ct_addTestFun(t, test_gegl_entry_record_new));
  return t;
}
