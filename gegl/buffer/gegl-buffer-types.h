/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#ifndef _GEGL_BUFFER_TYPES_H
#define _GEGL_BUFFER_TYPES_H
 
typedef struct _GeglTile                 GeglTile;
typedef struct _GeglTileClass            GeglTileClass;

typedef struct _GeglTileStore            GeglTileStore;
typedef struct _GeglTileStoreClass       GeglTileStoreClass;

typedef struct _GeglTileBackend          GeglTileBackend;
typedef struct _GeglTileBackendClass     GeglTileBackendClass;

typedef struct _GeglTileTrait            GeglTileTrait;
typedef struct _GeglTileTraitClass       GeglTileTraitClass;

typedef struct _GeglTileTraits           GeglTileTraits;
typedef struct _GeglTileTraitsClass      GeglTileTraitsClass;

typedef struct _GeglStorage              GeglStorage;
typedef struct _GeglStorageClass         GeglStorageClass;

typedef struct _GeglBuffer               GeglBuffer;
typedef struct _GeglBufferClass          GeglBufferClass;

typedef struct _GeglBufferAllocator      GeglBufferAllocator;
typedef struct _GeglBufferAllocatorClass GeglBufferAllocatorClass;

#endif
