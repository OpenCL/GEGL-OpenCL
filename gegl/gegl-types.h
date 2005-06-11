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
 */

#ifndef __GEGL_TYPES_H__
#define __GEGL_TYPES_H__

G_BEGIN_DECLS


typedef struct _GeglConnection  GeglConnection;
typedef struct _GeglEvalMgr     GeglEvalMgr;
typedef struct _GeglEvalVisitor GeglEvalVisitor;
typedef struct _GeglOperation   GeglOperation;
typedef struct _GeglGraph       GeglGraph;
typedef struct _GeglNode        GeglNode;
typedef struct _GeglProperty    GeglProperty;
typedef struct _GeglVisitable   GeglVisitable; /* dummy typedef */
typedef struct _GeglVisitor     GeglVisitor;


typedef struct _GeglRect      GeglRect;
typedef struct _GeglPoint     GeglPoint;
typedef struct _GeglDimension GeglDimension;

struct _GeglRect
{
  gint x;
  gint y;
  gint w;
  gint h;
};

struct _GeglPoint
{
  gint x;
  gint y;
};

struct _GeglDimension
{
  gint width;
  gint height;
};


G_END_DECLS

#endif /* __GEGL_TYPES_H__ */
