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
 */

#ifndef __GEGL_IMAGE_TYPES_H__
#define __GEGL_IMAGE_TYPES_H__

G_BEGIN_DECLS


typedef struct _GeglBuffer               GeglBuffer;
typedef struct _GeglBufferDouble         GeglBufferDouble;
typedef struct _GeglCache                GeglCache;
typedef struct _GeglCacheStore           GeglCacheStore;
typedef struct _GeglCacheEntry           GeglCacheEntry;
typedef struct _GeglColorModel           GeglColorModel;
typedef struct _GeglComponentSampleModel GeglComponentSampleModel;
typedef struct _GeglEntryRecord          GeglEntryRecord;
typedef struct _GeglHeapCache            GeglHeapCache;
typedef struct _GeglHeapCacheStore       GeglHeapCacheStore;
typedef struct _GeglNullCacheStore       GeglNullCacheStore;
typedef struct _GeglSampleModel          GeglSampleModel;
typedef struct _GeglSwapCache            GeglSwapCache;
typedef struct _GeglSwapCacheStore       GeglSwapCacheStore;


typedef enum
{
  GEGL_STORED,
  GEGL_FETCHED,
  GEGL_DISCARDED,
  GEGL_UNDEFINED
} GeglCacheStatus;


/* I hate type enums.  This is what JAI does, and I just do what
 * they do for now, and try to figure out something better later.
 */
typedef enum
{
  TYPE_DOUBLE,
  TYPE_UNKNOWN
} TransferType;


G_END_DECLS

#endif /* __GEGL_IMAGE_TYPES_H__ */
