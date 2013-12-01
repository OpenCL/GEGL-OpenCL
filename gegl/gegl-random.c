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
 * Copyright 2012, 2013 Øyvind Kolås
 * Copyright 2013       Téo Mazars <teomazars@gmail.com>
 */

/* This file provides random access - reproducable random numbers in three
 * dimensions. Well suited for predictable consistent output from image
 * processing operations; done in a way that should also work on GPUs/with
 * vector processing. The n dimension is to be used for successive random
 * numbers needed for each pixel, the z argument is currently unused but
 * it is there in the API to provide for mip-map behavior later.
 *
 * The way it works is by xoring three lookup tables that are iterated
 * cyclically, each LUT has a different prime number as a size, thus
 * the combination of the three values xored will have a period of
 * prime1 * prime2 * prime3, with the primes used this yields more than
 * 1TB of random, non-repeating data from ~180kb of lookup data.
 *
 * The data for the LUTs is shared between different random seeds, it
 * is just the sizes of the LUTs that change, currently with 533 primes
 * there is about 533 * 532 * 531 different seeds achivable.
 */

#include <glib.h>
#include <gegl.h>
#include "gegl-random.h"
#include "gegl-random-private.h"

/* a set of reasonably large primes to choose from for array sizes
 */
guint16 gegl_random_primes[533]={
10007,10009,10037,10039,10061,10067,10069,10079,10091,10093,10099,10103,10111,
10133,10139,10141,10151,10159,10163,10169,10177,10181,10193,10211,10223,10243,
10247,10253,10259,10267,10271,10273,10289,10301,10303,10313,10321,10331,10333,
10337,10343,10357,10369,10391,10399,10427,10429,10433,10453,10457,10459,10463,
10477,10487,10499,10501,10513,10529,10531,10559,10567,10589,10597,10601,10607,
10613,10627,10631,10639,10651,10657,10663,10667,10687,10691,10709,10711,10723,
10729,10733,10739,10753,10771,10781,10789,10799,10831,10837,10847,10853,10859,
10861,10867,10883,10889,10891,10903,10909,10937,10939,10949,10957,10973,10979,
10987,10993,11003,11027,11047,11057,11059,11069,11071,11083,11087,11093,11113,
11117,11119,11131,11149,11159,11161,11171,11173,11177,11197,11213,11239,11243,
11251,11257,11261,11273,11279,11287,11299,11311,11317,11321,11329,11351,11353,
11369,11383,11393,11399,11411,11423,11437,11443,11447,11467,11471,11483,11489,
11491,11497,11503,11519,11527,11549,11551,11579,11587,11593,11597,11617,11621,
11633,11657,11677,11681,11689,11699,11701,11717,11719,11731,11743,11777,11779,
11783,11789,11801,11807,11813,11821,11827,11831,11833,11839,11863,11867,11887,
11897,11903,11909,11923,11927,11933,11939,11941,11953,11959,11969,11971,11981,
11987,12007,12011,12037,12041,12043,12049,12071,12073,12097,12101,12107,12109,
12113,12119,12143,12149,12157,12161,12163,12197,12203,12211,12227,12239,12241,
12251,12253,12263,12269,12277,12281,12289,12301,12323,12329,12343,12347,12373,
12377,12379,12391,12401,12409,12413,12421,12433,12437,12451,12457,12473,12479,
12487,12491,12497,12503,12511,12517,12527,12539,12541,12547,12553,12569,12577,
12583,12589,12601,12611,12613,12619,12637,12641,12647,12653,12659,12671,12689,
12697,12703,12713,12721,12739,12743,12757,12763,12781,12791,12799,12809,12821,
12823,12829,12841,12853,12889,12893,12899,12907,12911,12917,12919,12923,12941,
12953,12959,12967,12973,12979,12983,13001,13003,13007,13009,13033,13037,13043,
13049,13063,13093,13099,13103,13109,13121,13127,13147,13151,13159,13163,13171,
13177,13183,13187,13217,13219,13229,13241,13249,13259,13267,13291,13297,13309,
13313,13327,13331,13337,13339,13367,13381,13397,13399,13411,13417,13421,13441,
13451,13457,13463,13469,13477,13487,13499,13513,13523,13537,13553,13567,13577,
13591,13597,13613,13619,13627,13633,13649,13669,13679,13681,13687,13691,13693,
13697,13709,13711,13721,13723,13729,13751,13757,13759,13763,13781,13789,13799,
13807,13829,13831,13841,13859,13873,13877,13879,13883,13901,13903,13907,13913,
13921,13931,13933,13963,13967,13997,13999,14009,14011,14029,14033,14051,14057,
14071,14081,14083,14087,14107,14143,14149,14153,14159,14173,14177,14197,14207,
14221,14243,14249,14251,14281,14293,14303,14321,14323,14327,14341,14347,14369,
14387,14389,14401,14407,14411,14419,14423,14431,14437,14447,14449,14461,14479,
14489,14503,14519,14533,14537,14543,14549,14551,14557,14561,14563,14591,14593,
14621,14627,14629,14633,14639,14653,14657,14699,14713,14717,14723,14731,14737,
14741,14747,14753,14759,14767,14771,14779,14783,14797,14813,14821,14827,14831,
14843,14851,14867,14869,14879,14887,14891,14897,14923,14929,14939,14947,14951,
14957,14969,14983,15013,15017,15031,15053,15061,15073,15077,15083,15091,15101
};

/* these primes should not exist in the above set */
#define XPRIME 103423LL
#define YPRIME 101359LL
#define NPRIME 101111LL

#define SQR(x) ((x)*(x))

static guint32  *gegl_random_data;
static gboolean  random_data_inited = FALSE;

static void
gegl_random_init (void)
{
  if (random_data_inited)
    return;
  else
    {
      GRand *gr = g_rand_new_with_seed (42);
      gint   i;

      /* Ensure alignment, needed with the use of CL_MEM_USE_HOST_PTR */
      gegl_random_data = gegl_malloc (RANDOM_DATA_SIZE * sizeof (guint32));

      for (i = 0; i < RANDOM_DATA_SIZE; i++)
        gegl_random_data[i] = g_rand_int (gr);

      g_rand_free (gr);
      random_data_inited = TRUE;
    }
}

void
gegl_random_cleanup (void)
{
  if (random_data_inited)
    {
      gegl_free (gegl_random_data);
      gegl_random_data = NULL;
      random_data_inited = FALSE;
    }
}

guint32*
gegl_random_get_data (void)
{
  gegl_random_init ();
  return gegl_random_data;
}

GeglRandom*
gegl_random_new (void)
{
  guint seed = (guint) g_random_int();

  return gegl_random_new_with_seed (seed);
}

GeglRandom*
gegl_random_new_with_seed (guint32 seed)
{
  GeglRandom *rand = g_new (GeglRandom, 1);

  gegl_random_set_seed (rand, seed);

  return rand;
}

void
gegl_random_free (GeglRandom *rand)
{
  g_free (rand);
}

void
gegl_random_set_seed (GeglRandom *rand,
                      guint32     seed)
{
  guint number_elem = G_N_ELEMENTS (gegl_random_primes);
  guint id0, id1, id2;

  /* XXX: Probably needs to be done at gegl-init */
  gegl_random_init ();

  /* gives roughly number_elem^3 different sets */
  id0 =  seed % number_elem;
  id1 = (seed / number_elem) % number_elem;
  id2 = (seed / SQR (number_elem)) % number_elem;

  /* Shuffle a bit to avoid to hit edge cases near 0 */
  id0  = (id0 + 42)  % number_elem;
  id1  = (id1 + 212) % number_elem;
  id2  = (id2 + 17)  % number_elem;

  /* all primes numbers must be distincts */
  while (id0 == id1 || id0 == id2)
    id0 = (id0 + 1) % number_elem;

  while (id1 == id0 || id1 == id2)
    id1 = (id1 + 1) % number_elem;

  rand->prime0 = gegl_random_primes[id0];
  rand->prime1 = gegl_random_primes[id1];
  rand->prime2 = gegl_random_primes[id2];
}

GeglRandom*
gegl_random_duplicate (GeglRandom *gegl_random)
{
  GeglRandom *result = g_new (GeglRandom, 1);

  *result = *gegl_random;

  return result;
}

GType
gegl_random_get_type (void)
{
  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static (g_intern_static_string ("GeglRandom"),
                                             (GBoxedCopyFunc) gegl_random_duplicate,
                                             (GBoxedFreeFunc) g_free);
  return our_type;
}

static inline guint32
_gegl_random_int (const GeglRandom *rand,
                  gint              x,
                  gint              y,
                  gint              z,
                  gint              n)
{
  guint64 idx = x * XPRIME +
                y * YPRIME * XPRIME +
                n * NPRIME * YPRIME * XPRIME;
  return
    gegl_random_data[idx % rand->prime0] ^
    gegl_random_data[rand->prime0 + (idx % (rand->prime1))] ^
    gegl_random_data[rand->prime0 + rand->prime1 + (idx % (rand->prime2))];
}

guint32
gegl_random_int (const GeglRandom *rand,
                 gint              x,
                 gint              y,
                 gint              z,
                 gint              n)
{
  return _gegl_random_int (rand, x, y, z, n);
}

gint32
gegl_random_int_range (const GeglRandom *rand,
                       gint              x,
                       gint              y,
                       gint              z,
                       gint              n,
                       gint              min,
                       gint              max)
{
  guint32 ret = _gegl_random_int (rand, x, y, z, n);
  return (ret % (max - min)) + min;
}

#define G_RAND_FLOAT_TRANSFORM  0.00001525902189669642175

gfloat
gegl_random_float (const GeglRandom *rand,
                   gint              x,
                   gint              y,
                   gint              z,
                   gint              n)
{
  return (_gegl_random_int (rand, x, y, z, n) & 0xffff) * G_RAND_FLOAT_TRANSFORM;
}

gfloat
gegl_random_float_range (const GeglRandom *rand,
                         gint              x,
                         gint              y,
                         gint              z,
                         gint              n,
                         gfloat            min,
                         gfloat            max)
{
  return gegl_random_float (rand, x, y, z, n) * (max - min) + min;
}
