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
 * Copyright (C) 2013 Daniel Sabo
 */

#include <string.h>
#include <stdio.h>

#include "gegl.h"
#include "gegl-plugin.h"

#define SUCCESS  0
#define FAILURE -1

/* GPL2 */

typedef struct
{
  GeglOperation  parent_instance;
} GeglTestOperationGPL2;

typedef struct
{
  GeglOperationClass  parent_class;
} GeglTestOperationGPL2Class;

GType   gegl_test_operation_gpl2_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (GeglTestOperationGPL2, gegl_test_operation_gpl2,
               GEGL_TYPE_OPERATION);

static void
gegl_test_operation_gpl2_init (GeglTestOperationGPL2 *self)
{
}

static void
gegl_test_operation_gpl2_class_init (GeglTestOperationGPL2Class *klass)
{
  gegl_operation_class_set_keys (GEGL_OPERATION_CLASS (klass),
                                 "name",          "gegl-test:license-gpl2",
                                 "description",   "",
                                 "license",       "GPL2",
                                 NULL);
}

/* GPL2+ */

typedef struct
{
  GeglOperation  parent_instance;
} GeglTestOperationGPL2Plus;

typedef struct
{
  GeglOperationClass  parent_class;
} GeglTestOperationGPL2PlusClass;

GType   gegl_test_operation_gpl2_plus_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (GeglTestOperationGPL2Plus, gegl_test_operation_gpl2_plus,
               GEGL_TYPE_OPERATION);

static void
gegl_test_operation_gpl2_plus_init (GeglTestOperationGPL2Plus *self)
{
}

static void
gegl_test_operation_gpl2_plus_class_init (GeglTestOperationGPL2PlusClass *klass)
{
  gegl_operation_class_set_keys (GEGL_OPERATION_CLASS (klass),
                                 "name",          "gegl-test:license-gpl2-plus",
                                 "description",   "",
                                 "license",       "GPL2+",
                                 NULL);
}

/* GPL3 */

typedef struct
{
  GeglOperation  parent_instance;
} GeglTestOperationGPL3;

typedef struct
{
  GeglOperationClass  parent_class;
} GeglTestOperationGPL3Class;

GType   gegl_test_operation_gpl3_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (GeglTestOperationGPL3, gegl_test_operation_gpl3,
               GEGL_TYPE_OPERATION);

static void
gegl_test_operation_gpl3_init (GeglTestOperationGPL3 *self)
{
}

static void
gegl_test_operation_gpl3_class_init (GeglTestOperationGPL3Class *klass)
{
  gegl_operation_class_set_keys (GEGL_OPERATION_CLASS (klass),
                                 "name",          "gegl-test:license-gpl3",
                                 "description",   "",
                                 "license",       "GPL3",
                                 NULL);
}

/* GPL3+ */

typedef struct
{
  GeglOperation  parent_instance;
} GeglTestOperationGPL3Plus;

typedef struct
{
  GeglOperationClass  parent_class;
} GeglTestOperationGPL3PlusClass;

GType   gegl_test_operation_gpl3_plus_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (GeglTestOperationGPL3Plus, gegl_test_operation_gpl3_plus,
               GEGL_TYPE_OPERATION);

static void
gegl_test_operation_gpl3_plus_init (GeglTestOperationGPL3Plus *self)
{
}

static void
gegl_test_operation_gpl3_plus_class_init (GeglTestOperationGPL3PlusClass *klass)
{
  gegl_operation_class_set_keys (GEGL_OPERATION_CLASS (klass),
                                 "name",          "gegl-test:license-gpl3-plus",
                                 "description",   "",
                                 "license",       "GPL3+",
                                 NULL);
}

/* Foo */

typedef struct
{
  GeglOperation  parent_instance;
} GeglTestOperationFoo;

typedef struct
{
  GeglOperationClass  parent_class;
} GeglTestOperationFooClass;

GType   gegl_test_operation_foo_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (GeglTestOperationFoo, gegl_test_operation_foo,
               GEGL_TYPE_OPERATION);

static void
gegl_test_operation_foo_init (GeglTestOperationFoo *self)
{
}

static void
gegl_test_operation_foo_class_init (GeglTestOperationFooClass *klass)
{
  gegl_operation_class_set_keys (GEGL_OPERATION_CLASS (klass),
                                 "name",          "gegl-test:license-foo",
                                 "description",   "",
                                 "license",       "Foo",
                                 NULL);
}

/* Bar */

typedef struct
{
  GeglOperation  parent_instance;
} GeglTestOperationBar;

typedef struct
{
  GeglOperationClass  parent_class;
} GeglTestOperationBarClass;

GType   gegl_test_operation_bar_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (GeglTestOperationBar, gegl_test_operation_bar,
               GEGL_TYPE_OPERATION);

static void
gegl_test_operation_bar_init (GeglTestOperationBar *self)
{
}

static void
gegl_test_operation_bar_class_init (GeglTestOperationBarClass *klass)
{
  gegl_operation_class_set_keys (GEGL_OPERATION_CLASS (klass),
                                 "name",          "gegl-test:license-bar",
                                 "description",   "",
                                 "license",       "Bar",
                                 NULL);
}

#define RUN_TEST(test_name) \
{ \
  if (test_name()) \
    { \
      printf ("" #test_name " ... PASS\n"); \
      tests_passed++; \
    } \
  else \
    { \
      printf ("" #test_name " ... FAIL\n"); \
      tests_failed++; \
    } \
  tests_run++; \
}

static gboolean
test_default (void)
{
  if (gegl_has_operation ("gegl-test:license-gpl2"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl2-plus"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl3"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl3-plus"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-foo"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-bar"))
    return FALSE;
  return TRUE;
}

static gboolean
test_gpl1 (void)
{
  g_object_set (gegl_config (),
              "application-license", "GPL1",
              NULL);

  if (gegl_has_operation ("gegl-test:license-gpl2"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl2-plus"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl3"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl3-plus"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-foo"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-bar"))
    return FALSE;
  return TRUE;
}

static gboolean
test_gpl2 (void)
{
  g_object_set (gegl_config (),
              "application-license", "GPL2",
              NULL);

  if (!gegl_has_operation ("gegl-test:license-gpl2"))
    return FALSE;
  if (!gegl_has_operation ("gegl-test:license-gpl2-plus"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl3"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl3-plus"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-foo"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-bar"))
    return FALSE;
  return TRUE;
}

static gboolean
test_gpl3 (void)
{
  g_object_set (gegl_config (),
              "application-license", "GPL3",
              NULL);

  if (gegl_has_operation ("gegl-test:license-gpl2"))
    return FALSE;
  if (!gegl_has_operation ("gegl-test:license-gpl2-plus"))
    return FALSE;
  if (!gegl_has_operation ("gegl-test:license-gpl3"))
    return FALSE;
  if (!gegl_has_operation ("gegl-test:license-gpl3-plus"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-foo"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-bar"))
    return FALSE;
  return TRUE;
}

static gboolean
test_foo (void)
{
  g_object_set (gegl_config (),
              "application-license", "foo",
              NULL);

  if (gegl_has_operation ("gegl-test:license-gpl2"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl2-plus"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl3"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl3-plus"))
    return FALSE;
  if (!gegl_has_operation ("gegl-test:license-foo"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-bar"))
    return FALSE;
  return TRUE;
}

static gboolean
test_foo_bar (void)
{
  g_object_set (gegl_config (),
              "application-license", "foo,bar",
              NULL);

  if (gegl_has_operation ("gegl-test:license-gpl2"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl2-plus"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl3"))
    return FALSE;
  if (gegl_has_operation ("gegl-test:license-gpl3-plus"))
    return FALSE;
  if (!gegl_has_operation ("gegl-test:license-foo"))
    return FALSE;
  if (!gegl_has_operation ("gegl-test:license-bar"))
    return FALSE;
  return TRUE;
}

int
main (int argc, char *argv[])
{
  gint tests_run    = 0;
  gint tests_passed = 0;
  gint tests_failed = 0;

  gegl_init (&argc, &argv);

  g_type_class_peek (gegl_test_operation_gpl2_get_type ());
  g_type_class_peek (gegl_test_operation_gpl2_plus_get_type ());
  g_type_class_peek (gegl_test_operation_gpl3_get_type ());
  g_type_class_peek (gegl_test_operation_gpl3_plus_get_type ());
  g_type_class_peek (gegl_test_operation_foo_get_type ());
  g_type_class_peek (gegl_test_operation_bar_get_type ());

  RUN_TEST (test_default)
  RUN_TEST (test_gpl1)
  RUN_TEST (test_gpl2)
  RUN_TEST (test_gpl3)
  RUN_TEST (test_foo)
  RUN_TEST (test_foo_bar)

  gegl_exit ();

  if (tests_passed == tests_run)
    return SUCCESS;
  return FAILURE;
}