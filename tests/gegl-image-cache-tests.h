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



#include "gegl-image-mock-cache-entry.h"
#include "image/gegl-cache.h"
#include "ctest.h"

extern const gint num_entries;
extern const gint min_cache_size;

void gegl_cache_leak_test1 (Test * test);
void gegl_cache_leak_test2 (Test * test);
void gegl_cache_leak_test3 (Test * test);
void gegl_cache_leak_test4 (Test * test);
void gegl_cache_leak_test5 (Test * test);
void gegl_cache_leak_test6 (Test * test);

void setup_cache_leaks (GeglCache * cache);
void teardown_cache_leaks ();
void
gegl_cache_leaks_test (Test * test);
void setup_const_entries ();
void teardown_const_entries();
void setup_prime_entries();
void teardown_prime_entries();
