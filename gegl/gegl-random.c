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
 * Copyright 2012 Øyvind Kolås
 */

/* This file provides random access - reproducable random numbers in three
 * dimensions. Well suited for predictable consistent output from image
 * processing operations; done in a way that should also work on GPUs/with
 * vector processing. The n dimension is to be used for successive random
 * numbers needed for each pixel, the z argument is currently unused but
 * it is there in the API to provide for mip-map behavior later.
 *
 * The way it works is by xoring three lookup tables that are iterated
 * cyclically, each LUT has a prime number as a size, thus the combination
 * of the three values xored will have a period of prime1 * prime2 * prime3,
 * with the primes used this yields roughly 3TB of random data from ~300kb
 * of lookup data. The LUTs are being initialized from a random seed.
 *
 * It might * be possible to change this so that the random source data is reused
 * across seeds - and only the sizes of the arrays are manipulated - thus
 * removing most of the initialization overhead when setting a new seed.
 */


#include <glib.h>
#include <gegl.h>

/* a set of reasonably large primes to choose from for array sizes
 */
static long primes[]={
14699,14713,14717,14723,14731,14737,14741,14747,14753,
14759,14767,14771,14779,14783,14797,14813,14821,14827,
14831,14843,14851,14867,14869,14879,14887,14891,14897,
14923,14929,14939,14947,14951,14957,14969,14983,15013,
15017,15031,15053,15061,15073,15077,15083,15091,15101
};

/* these primes should not exist in the above set */
#define XPRIME     103423
#define YPRIME     101359
#define NPRIME     101111
#define MAX_TABLES 3

typedef struct GeglRandomSet
{
  int     seed;
  int     tables;
  gint64 *table[MAX_TABLES];
  long    prime[MAX_TABLES];
} GeglRandomSet;

#define make_index(x,y,n) ((x) * XPRIME + \
                           (y) * YPRIME * XPRIME + \
                           (n) * NPRIME * YPRIME * XPRIME)

static GeglRandomSet *
gegl_random_set_new (int seed)
{
  GeglRandomSet *set = g_malloc0 (sizeof (GeglRandomSet));
  GRand *gr;
  int i;

  set->seed = seed;
  set->tables = MAX_TABLES;

  gr = g_rand_new_with_seed (set->seed);

  for (i = 0; i < set->tables; i++)
    {
      int j;
      int found = 0;
      do
        {
          found = 0;
          set->prime[i] = primes[g_rand_int_range (gr, 0, G_N_ELEMENTS (primes) - 2)];
          for (j = 0; j < i; j++)
            if (set->prime[j] == set->prime[i])
              found = 1;
        } while (found);

      set->table[i] = g_malloc0 (sizeof (gint64) * set->prime[i]);

      for (j = 0; j < set->prime[i]; j++)
        set->table[i][j] = (((gint64) g_rand_int (gr)) << 32) + g_rand_int (gr);
    }

  g_rand_free (gr);

  return set;
}

static void
gegl_random_set_free (GeglRandomSet *set)
{
  int i;

  for (i = 0; i < set->tables; i++)
    g_free (set->table[i]);

  g_free (set);
}

/* a better cache with more entries would be nice ;) */
static GeglRandomSet *cached = NULL;
static GList         *cache  = NULL;

static void
trim_cache_to_length (int length)
{
  while (g_list_length (cache) > length)
    {
      GeglRandomSet *last = g_list_last (cache)->data;

      cache = g_list_remove (cache, last);
      gegl_random_set_free (last);
    }
}

static inline GeglRandomSet *
gegl_random_get_set_for_seed (int seed)
{
  if (cached && cached->seed == seed)
    {
      return cached;
    }
  else
    {
      GList *l;

      if (cached)
        {
          cache = g_list_prepend (cache, cached);
          cached = NULL;
          trim_cache_to_length (10);
        }

      for (l = cache; l; l=l->next)
        {
          GeglRandomSet *s = l->data;

          if (s->seed == seed)
            {
              cached = s;
              cache = g_list_remove (cache, cached);

              return cached;
            }
        }

      cached = gegl_random_set_new (seed);
    }

  return cached;
}

gint64
gegl_random_int (int seed,
                 int x,
                 int y,
                 int z,
                 int n)
{
  GeglRandomSet *set = gegl_random_get_set_for_seed (seed);
  /* XXX: z is unhandled, it should average like a mipmap - or even
   * use mipmap versions of random set
   */
  unsigned long idx = make_index (x, y, n);
  gint64 ret = 0;
  int i;

  for (i = 0; i < set->tables; i++)
    ret ^= set->table[i][idx % (set->prime[i])];

  return ret;
}

gint64
gegl_random_int_range (int seed,
                       int x,
                       int y,
                       int z,
                       int n,
                       int min,
                       int max)
{
  gint64 ret = gegl_random_int (seed, x, y, z, n);

  return (ret % (max-min)) + min;
}

/* transform [0..2^32] -> [0..1] */
#define G_RAND_DOUBLE_TRANSFORM 2.3283064365386962890625e-10

double
gegl_random_double (int seed,
                    int x,
                    int y,
                    int z,
                    int n)
{
  return (gegl_random_int (seed, x, y, z, n) & 0xffffffff) * G_RAND_DOUBLE_TRANSFORM;
}

double
gegl_random_double_range (int seed,
                          int x,
                          int y,
                          int z,
                          int n,
                          double min,
                          double max)
{
  return gegl_random_double (seed, x, y, z, n) * (max - min) + min;
}
