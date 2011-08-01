/* 
 * File:   cutils.h
 * Author: user
 *
 * Created on 28 מאי 2011, 02:24
 */

#ifndef CUTILS_H
#define	CUTILS_H

#include <glib.h>
#include "poly2tri-private.h"

#ifdef	__cplusplus
extern "C"
{
#endif

#define DEBUG FALSE

  typedef GPtrArray* P2tEdgePtrArray;
#define edge_index(array,index_) ((P2tEdge*)g_ptr_array_index(array,index_))
  typedef GPtrArray* P2tPointPtrArray;
#define point_index(array,index_) ((P2tPoint*)g_ptr_array_index(array,index_))
  typedef GPtrArray* P2tTrianglePtrArray;
#define triangle_index(array,index_) ((P2tTriangle*)g_ptr_array_index(array,index_))
  typedef GPtrArray* P2tNodePtrArray;
#define node_index(array,index_) ((P2tNode*)g_ptr_array_index(array,index_))
  typedef GList* P2tTrianglePtrList;
#define triangle_val(list) ((P2tTriangle*)((list)->data))

#define g_ptr_array_index_cyclic(array,index_) g_ptr_array_index(array,(index_)%((array)->len))
  
#ifdef	__cplusplus
}
#endif

#endif	/* CUTILS_H */

