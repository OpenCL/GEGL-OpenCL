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
 *  Copyright 2003 Daniel S. Rogers
 *
 */

#include "image/gegl-buffer-double.h"
#include "image/gegl-memory-cache.h"

#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_buffer_caching(Test *test) {
  /*
    This will test, acquire, release, lock, unlock, unshare, is_finalized,
    attach, and detach.
  */

  GeglBuffer * buffer = gegl_buffer_create (TYPE_DOUBLE,
					    "num_banks", 4,
					    "elements_per_bank", 1024,
					    NULL);
  GeglBuffer * another_buffer = gegl_buffer_create (TYPE_DOUBLE,
						    "num_banks", 4,
						    "elements_per_bank", 1024,
						    NULL);
  GeglBuffer * buffer_copy;
  GeglCache * cache;

  gegl_buffer_acquire (buffer);
  buffer_copy = buffer;

  gegl_buffer_release (buffer);
  gegl_buffer_release (buffer_copy);

  buffer = gegl_buffer_create (TYPE_DOUBLE,
			       "num_banks", 4,
			       "elements_per_bank", 1024,
			       NULL);
  gegl_buffer_acquire (buffer);
  buffer_copy = buffer;

  gegl_buffer_lock (buffer_copy);
  buffer_copy = gegl_buffer_unshare (buffer_copy);
  gegl_buffer_unlock (buffer_copy, FALSE);

  /*
   * 32768 bytes (32kB) is the size of a single BufferDouble with 4,
   * 1024 banks.
   */
  cache = g_object_new (GEGL_TYPE_MEMORY_CACHE,
			"soft_limit", G_GINT64_CONSTANT(32768),
			"hard_limit", G_GINT64_CONSTANT(32768),
			"persistent", FALSE,
			NULL);
  gegl_buffer_attach (buffer, cache);
  gegl_buffer_detach (buffer);

  gegl_buffer_attach (buffer, cache);
  
  gegl_buffer_attach (another_buffer, cache);

  gegl_buffer_lock (buffer);
  ct_test (test, buffer->banks == NULL);
  gegl_buffer_unlock (buffer, FALSE);

  gegl_buffer_lock (another_buffer);
  ct_test (test, another_buffer->banks != NULL);
  gegl_buffer_unlock (another_buffer, FALSE);

  gegl_buffer_release (buffer);
  gegl_buffer_release (buffer_copy);
  gegl_buffer_release (another_buffer);
  g_object_unref (cache);

}


Test *
create_buffer_caching_test()
{
  Test* t = ct_create("GeglBufferCachingTest");
  /*
    g_assert(ct_addSetUp(t, color_test_setup));
    g_assert(ct_addTearDown(t, color_test_teardown));
  */
  g_assert(ct_addTestFun(t, test_buffer_caching));
  return t; 
}
