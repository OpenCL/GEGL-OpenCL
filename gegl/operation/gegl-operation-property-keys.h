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
 * Copyright 2014 Øyvind Kolås
 */

#ifndef __GEGL_OPERATION_PROPERTY_KEYS_H__
#define __GEGL_OPERATION_PROPERTY_KEYS_H__

#include <glib-object.h>

void gegl_operation_class_set_property_key (GeglOperationClass *klass,
                                            const gchar        *property_name,
                                            const gchar        *key_name,
                                            const gchar        *value);

const gchar *
gegl_operation_class_get_property_key (GeglOperationClass *operation_class,
                                       const gchar        *property_name,
                                       const gchar        *key_name);

#endif
