/* This file is part of GEGL
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEGL_MODULE_TYPES_H__
#define __GEGL_MODULE_TYPES_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#ifndef GEGL_DISABLE_DEPRECATED
/*
 * GEGL_MODULE_PARAM_SERIALIZE is deprecated, use
 * GEGL_CONFIG_PARAM_SERIALIZE instead.
 */
#define GEGL_MODULE_PARAM_SERIALIZE (1 << (0 + G_PARAM_USER_SHIFT))
#endif


typedef struct _GeglModule     GeglModule;
typedef struct _GeglModuleInfo GeglModuleInfo;
typedef struct _GeglModuleDB   GeglModuleDB;


G_END_DECLS

#endif  /* __GEGL_MODULE_TYPES_H__ */
