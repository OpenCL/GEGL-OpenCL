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

#include "image/gegl-cache-store.h"
#include "image/gegl-entry-record.h"
#include "gegl-image-mock-cache-entry.h"
#include "ctest.h"
#include "csuite.h"

static GeglCacheStore * test_store;
static GeglEntryRecord * record1;
static GeglEntryRecord * record2;
static GeglEntryRecord * record3;
static GeglEntryRecord * record4;
static GeglMockCacheEntry * entry1;
static GeglMockCacheEntry * entry2;
static GeglMockCacheEntry * entry3;
static GeglMockCacheEntry * entry4;

void
setup_cache_store_tests ( GeglCacheStore * store)
{
  g_object_ref (store);
  test_store = store;
  entry1 = gegl_mock_cache_entry_new (1024);
  entry2 = gegl_mock_cache_entry_new (2048);
  entry3 = gegl_mock_cache_entry_new (4096);
  entry4 = gegl_mock_cache_entry_new (8192);

  record1 = gegl_entry_record_new (NULL, GEGL_CACHE_ENTRY(entry1));
  record2 = gegl_entry_record_new (NULL, GEGL_CACHE_ENTRY(entry2));
  record3 = gegl_entry_record_new (NULL, GEGL_CACHE_ENTRY(entry3));
  record4 = gegl_entry_record_new (NULL, GEGL_CACHE_ENTRY(entry4));
}

void
teardown_cache_store_tests ( void )
{

  if (record1 != NULL)
    gegl_entry_record_free (record1);
  if (record2 != NULL)
    gegl_entry_record_free (record2);
  if (record3 != NULL)
    gegl_entry_record_free (record3);
  if (record4 != NULL)
    gegl_entry_record_free (record4);

  g_object_unref (entry1);
  g_object_unref (entry2);
  g_object_unref (entry3);
  g_object_unref (entry4);

  g_object_unref (test_store);
}

void
test_cache_store_add_remove ( Test * test )
{
  gegl_cache_store_add (test_store, record1);
  gegl_cache_store_add (test_store, record2);
  gegl_cache_store_add (test_store, record3);
  gegl_cache_store_add (test_store, record4);

  ct_test(test, record1->store == test_store);
  ct_test(test, record2->store == test_store);
  ct_test(test, record3->store == test_store);
  ct_test(test, record4->store == test_store);

  gegl_cache_store_remove (test_store, record1);
  gegl_cache_store_remove (test_store, record2);
  gegl_cache_store_remove (test_store, record3);
  gegl_cache_store_remove (test_store, record4);

  ct_test(test, record1->store != test_store);
  ct_test(test, record2->store != test_store);
  ct_test(test, record3->store != test_store);
  ct_test(test, record4->store != test_store);
}

void
test_cache_store_zap ( Test * test)
{
  gegl_cache_store_add (test_store, record1);
  gegl_cache_store_add (test_store, record2);
  gegl_cache_store_add (test_store, record3);
  gegl_cache_store_add (test_store, record4);

  gegl_cache_store_zap (test_store, record1);
  gegl_cache_store_zap (test_store, record2);
  gegl_cache_store_zap (test_store, record3);
  gegl_cache_store_zap (test_store, record4);

  record1 = NULL;
  record2 = NULL;
  record3 = NULL;
  record4 = NULL;
}

void
test_cache_store_size ( Test * test)
{
  gint64 size = 0;
  gegl_cache_store_add (test_store, record1);
  size = gegl_cache_store_size (test_store);
  ct_test (test, size == (1024) * sizeof(gint));

  gegl_cache_store_add (test_store, record2);
  size = gegl_cache_store_size (test_store);
  ct_test (test, size == (1024+2048) * sizeof(gint));

  gegl_cache_store_add (test_store, record3);
  size = gegl_cache_store_size (test_store);
  ct_test (test, size == (1024+2048+4096) * sizeof(gint));

  gegl_cache_store_add (test_store, record4);
  size = gegl_cache_store_size (test_store);
  ct_test (test, size == (1024+2048+4096+8192) * sizeof(gint));

  record1 = NULL;
  record2 = NULL;
  record3 = NULL;
  record4 = NULL;
}

void
test_cache_store_pop (Test * test)
{
  GeglEntryRecord * rec;

  gegl_cache_store_add (test_store, record1);
  gegl_cache_store_add (test_store, record2);
  gegl_cache_store_add (test_store, record3);
  gegl_cache_store_add (test_store, record4);

  rec = gegl_cache_store_pop (test_store);
  ct_test (test, rec == record1);
  rec = gegl_cache_store_pop (test_store);
  ct_test (test, rec == record2);
  rec = gegl_cache_store_pop (test_store);
  ct_test (test, rec == record3);
  rec = gegl_cache_store_pop (test_store);
  ct_test (test, rec == record4);
  rec = gegl_cache_store_pop (test_store);
  ct_test (test, rec == NULL);
}

void
test_cache_store_peek (Test * test)
{
  GeglEntryRecord * rec;
  gegl_cache_store_add (test_store, record1);
  rec = gegl_cache_store_peek (test_store);
  ct_test (test, rec == record1);

  gegl_cache_store_add (test_store, record2);
  rec = gegl_cache_store_peek (test_store);
  ct_test (test, rec == record1);

  gegl_cache_store_add (test_store, record3);
  rec = gegl_cache_store_peek (test_store);
  ct_test (test, rec == record1);

  gegl_cache_store_add (test_store, record4);
  rec = gegl_cache_store_peek (test_store);
  ct_test (test, rec == record1);

  record1 = NULL;
  record2 = NULL;
  record3 = NULL;
  record4 = NULL;
}
