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

#include <glib.h>
#include <gegl.h>

/* a set of reasonably large primes to choose from for array sizes
 */
static long primes[]={
  99989, 99991,100003,100019,100043,100049,100057,100069,100103,100109,
 100129,100151,100153,100169,100183,100189,100193,100207,100213,100237,
 100267,100271,100279,100291,100297,100313,100333,100343,100357,100361,
 100363,100379,100391,100393,100403,100411,100417,100447,100459,100469,
 100483,100493,100501,100511,100517,100519,100523,100537,100547,100549,
 100559,100591,100609,100613,100621,100649,100669,100673,100693,100699,
 100703,100733,100741,100747,100769,100787,100799,100801,100811,100823,
 100829,100847,100853,100907,100913,100927,100931,100937,100943,100957,
 100981,100987,100999,101009,101021,101027,101051,101063,101081,101089,
 101107,101113,101117,101119,101141,101149,101159,101161,101173,101183,
 101197,101203,101207,101209,101221,101267,101273,101279,101281,101287,
 101293,101323,101333,101341,101347,101363,101377,101383,101399,101411,
 101419,101429,101449,101467,101477,101483,101489,101501,101503,101513,
 101527,101531,101533,101537,101561,101573,101581,101599,101603,101611,
 101627,101641,101653,101663,101681,101693,101701,101719,101723,101737,
 101741,101747,101749,101771,101789,101797,101807,101833,101837,101839,
 101863,101869,101873,101879,101891,101917,101921,101929,101939,101957,
 101963,101977,101987,101999,102001,102013,102019,102023
};

/* these primes should not exist in the above set */
#define XPRIME     103423
#define ZPRIME     101359
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
                           (y) * ZPRIME * XPRIME + \
                           (n) * NPRIME * ZPRIME * XPRIME)

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

      /* it might be possible to share a set of random data between sets
       * and rejuggle the prime sizes chosen and keep an additional offset
       * for feeding randomness.
       *
       */
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
