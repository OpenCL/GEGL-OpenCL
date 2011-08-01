/* 
 * File:   poly2tri-private.h
 * Author: user
 *
 * Created on 4 יוני 2011, 13:22
 */

#ifndef POLY2TRI_PRIVATE_H
#define	POLY2TRI_PRIVATE_H

#ifdef	__cplusplus
extern "C"
{
#endif

typedef struct _P2tNode P2tNode;
typedef struct AdvancingFront_ P2tAdvancingFront;
typedef struct CDT_ P2tCDT;
typedef struct _P2tEdge P2tEdge;
typedef struct _P2tPoint P2tPoint;
typedef struct _P2tTriangle P2tTriangle;
typedef struct SweepContext_ P2tSweepContext;
typedef struct Sweep_ P2tSweep;

typedef struct P2tSweepContextBasin_ P2tSweepContextBasin;
typedef struct P2tSweepContextEdgeEvent_ P2tSweepContextEdgeEvent;


#ifdef	__cplusplus
}
#endif

#endif	/* POLY2TRI_PRIVATE_H */

