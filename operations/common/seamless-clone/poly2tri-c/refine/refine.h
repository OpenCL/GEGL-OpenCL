/* 
 * File:   refine.h
 * Author: Barak
 *
 * Created on 29 יולי 2011, 04:52
 */

#ifndef REFINE_H
#define	REFINE_H

#include <glib.h>
#include "triangulation.h"

#ifdef	__cplusplus
extern "C"
{
#endif

P2tRTriangulation*
p2tr_triangulate_and_refine (GPtrArray *pts, int max_refine_steps);



#ifdef	__cplusplus
}
#endif

#endif	/* REFINE_H */

