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
#include "ctest.h"

void setup_cache_store_tests ( GeglCacheStore * store);
void teardown_cache_store_tests ( void );

void test_cache_store_add_remove ( Test * test );
void test_cache_store_zap ( Test * test);
void test_cache_store_size ( Test * test);
void test_cache_store_pop (Test * test);
void test_cache_store_peek (Test * test);
