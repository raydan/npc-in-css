
#ifndef COLLISIONUTILS_H
#define COLLISIONUTILS_H

#include "tier0/platform.h"

#ifdef _WIN32
#pragma once
#endif

#include "mathlib/ssemath.h"
#include "cutil.h"


struct Ray_t;
class Vector;
class Vector2D;
class Vector4D;
struct cplane_t;
class QAngle;
class CBaseTrace;
struct matrix3x4_t;



bool IsBoxIntersectingBox( const Vector& boxMin1, const Vector& boxMax1, 
						   const Vector& boxMin2, const Vector& boxMax2 );



bool FASTCALL IsBoxIntersectingRay( const Vector& boxMin, const Vector& boxMax, 
									const Vector& origin, const Vector& delta, float flTolerance = 0.0f );

bool FASTCALL IsBoxIntersectingRay( const Vector& boxMin, const Vector& boxMax, 
									const Ray_t& ray, float flTolerance = 0.0f );

bool FASTCALL IsBoxIntersectingRay( const Vector& boxMin, const Vector& boxMax, 
									const Vector& origin, const Vector& delta,
									const Vector& invDelta, float flTolerance = 0.0f );



bool IntersectRayWithOBB( const Vector &vecRayStart, const Vector &vecRayDelta, 
	const matrix3x4_t &matOBBToWorld, const Vector &vecOBBMins, const Vector &vecOBBMaxs, 
	float flTolerance, BoxTraceInfo_t *pTrace );


bool IsPointInBox( const Vector& pt, const Vector& boxMin, const Vector& boxMax );


// SIMD version
FORCEINLINE bool IsPointInBox( const fltx4& pt, const fltx4& boxMin, const fltx4& boxMax )
{
	fltx4 greater = CmpGtSIMD( pt,boxMax );
	fltx4 less = CmpLtSIMD( pt, boxMin );
	return (IsAllZeros(SetWToZeroSIMD(OrSIMD(greater,less))));
}



//-----------------------------------------------------------------------------
//
// IntersectRayWithRay
//
// Returns whether or not there was an intersection.  The "t" paramter is the
// distance along ray0 and the "s" parameter is the distance along ray1.  If 
// the two lines to not intersect the "t" and "s" represent the closest approach.
// "t" and "s" will not change if the rays are parallel.
//
//-----------------------------------------------------------------------------
bool IntersectRayWithRay( const Ray_t &ray0, const Ray_t &ray1, float &t, float &s );

// This version intersects a ray with an axis-aligned plane
float IntersectRayWithAAPlane( const Vector& vecStart, const Vector& vecEnd, int nAxis, float flSign, float flDist );

#endif
