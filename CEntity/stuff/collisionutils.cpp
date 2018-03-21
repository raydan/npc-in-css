
#include "collisionutils.h"
#include "cmodel.h"
#include "mathlib/mathlib.h"
#include "mathlib/vector.h"
#include "tier0/dbg.h"
#include <float.h>
#include "mathlib/vector4d.h"
#include "trace.h"
#include "cutil.h"




#define USE_SIMD_RAY_CHECKS 1

bool IsBoxIntersectingBox( const Vector& boxMin1, const Vector& boxMax1, 
						const Vector& boxMin2, const Vector& boxMax2 )
{
	Assert( boxMin1[0] <= boxMax1[0] );
	Assert( boxMin1[1] <= boxMax1[1] );
	Assert( boxMin1[2] <= boxMax1[2] );
	Assert( boxMin2[0] <= boxMax2[0] );
	Assert( boxMin2[1] <= boxMax2[1] );
	Assert( boxMin2[2] <= boxMax2[2] );

	if ( (boxMin1[0] > boxMax2[0]) || (boxMax1[0] < boxMin2[0]) )
		return false;
	if ( (boxMin1[1] > boxMax2[1]) || (boxMax1[1] < boxMin2[1]) )
		return false;
	if ( (boxMin1[2] > boxMax2[2]) || (boxMax1[2] < boxMin2[2]) )
		return false;
	return true;
}

bool FASTCALL IsBoxIntersectingRay( const Vector& vecBoxMin, const Vector& vecBoxMax, const Ray_t& ray, float flTolerance )
{
	if ( !ray.m_IsSwept )
	{
		Vector rayMins, rayMaxs;
		VectorSubtract( ray.m_Start, ray.m_Extents, rayMins );
		VectorAdd( ray.m_Start, ray.m_Extents, rayMaxs );
		if ( flTolerance != 0.0f )
		{
			rayMins.x -= flTolerance; rayMins.y -= flTolerance; rayMins.z -= flTolerance;
			rayMaxs.x += flTolerance; rayMaxs.y += flTolerance; rayMaxs.z += flTolerance;
		}
		return IsBoxIntersectingBox( vecBoxMin, vecBoxMax, rayMins, rayMaxs );
	}

	Vector vecExpandedBoxMin, vecExpandedBoxMax;
	VectorSubtract( vecBoxMin, ray.m_Extents, vecExpandedBoxMin );
	VectorAdd( vecBoxMax, ray.m_Extents, vecExpandedBoxMax );
	return IsBoxIntersectingRay( vecExpandedBoxMin, vecExpandedBoxMax, ray.m_Start, ray.m_Delta, flTolerance );
}


bool FASTCALL IsBoxIntersectingRay( const Vector& boxMin, const Vector& boxMax, 
									const Vector& origin, const Vector& vecDelta, float flTolerance )
{
	
#if USE_SIMD_RAY_CHECKS
	// Load the unaligned ray/box parameters into SIMD registers
	fltx4 start = LoadUnaligned3SIMD(origin.Base());
	fltx4 delta = LoadUnaligned3SIMD(vecDelta.Base());
	fltx4 boxMins = LoadUnaligned3SIMD( boxMin.Base() );
	fltx4 boxMaxs = LoadUnaligned3SIMD( boxMax.Base() );
	fltx4 epsilon = ReplicateX4(flTolerance);
	// compute the mins/maxs of the box expanded by the ray extents
	// relocate the problem so that the ray start is at the origin.
	fltx4 offsetMins = SubSIMD(boxMins, start);
	fltx4 offsetMaxs = SubSIMD(boxMaxs, start);
	fltx4 offsetMinsExpanded = SubSIMD(offsetMins, epsilon);
	fltx4 offsetMaxsExpanded = AddSIMD(offsetMaxs, epsilon);

	// Check to see if both the origin (start point) and the end point (delta) are on the front side
	// of any of the box sides - if so there can be no intersection
	fltx4 startOutMins = CmpLtSIMD(Four_Zeros, offsetMinsExpanded);
	fltx4 endOutMins = CmpLtSIMD(delta,offsetMinsExpanded);
	fltx4 minsMask = AndSIMD( startOutMins, endOutMins );
	fltx4 startOutMaxs = CmpGtSIMD(Four_Zeros, offsetMaxsExpanded);
	fltx4 endOutMaxs = CmpGtSIMD(delta,offsetMaxsExpanded);
	fltx4 maxsMask = AndSIMD( startOutMaxs, endOutMaxs );
	if ( IsAnyNegative(SetWToZeroSIMD(OrSIMD(minsMask,maxsMask))))
		return false;

	// now build the per-axis interval of t for intersections
	fltx4 invDelta = ReciprocalSaturateSIMD(delta);
	fltx4 tmins = MulSIMD( offsetMinsExpanded, invDelta );
	fltx4 tmaxs = MulSIMD( offsetMaxsExpanded, invDelta );
	fltx4 crossPlane = OrSIMD(XorSIMD(startOutMins,endOutMins), XorSIMD(startOutMaxs,endOutMaxs));

	// only consider axes where we crossed a plane
	tmins = MaskedAssign( crossPlane, tmins, Four_Negative_FLT_MAX );
	tmaxs = MaskedAssign( crossPlane, tmaxs, Four_FLT_MAX );

	// now sort the interval per axis
	fltx4 mint = MinSIMD( tmins, tmaxs );
	fltx4 maxt = MaxSIMD( tmins, tmaxs );

	// now find the intersection of the intervals on all axes
	fltx4 firstOut = FindLowestSIMD3(maxt);
	fltx4 lastIn = FindHighestSIMD3(mint);
	// NOTE: This is really a scalar quantity now [t0,t1] == [lastIn,firstOut]
	firstOut = MinSIMD(firstOut, Four_Ones);
	lastIn = MaxSIMD(lastIn, Four_Zeros);

	// If the final interval is valid lastIn<firstOut, check for separation
	fltx4 separation = CmpGtSIMD(lastIn, firstOut);

	return IsAllZeros(separation);
#else
	// On the x360, we force use of the SIMD functions.
#if defined(_X360) 
	if (IsX360())
	{
		fltx4 delta = LoadUnaligned3SIMD(vecDelta.Base());
		return IsBoxIntersectingRay( 
			LoadUnaligned3SIMD(boxMin.Base()), LoadUnaligned3SIMD(boxMax.Base()),
			LoadUnaligned3SIMD(origin.Base()), delta, ReciprocalSIMD(delta), // ray parameters
			ReplicateX4(flTolerance) ///< eg from ReplicateX4(flTolerance)
			);
	}
#endif
	Assert( boxMin[0] <= boxMax[0] );
	Assert( boxMin[1] <= boxMax[1] );
	Assert( boxMin[2] <= boxMax[2] );

	// FIXME: Surely there's a faster way
	float tmin = -FLT_MAX;
	float tmax = FLT_MAX;

	for (int i = 0; i < 3; ++i)
	{
		// Parallel case...
		if (FloatMakePositive(vecDelta[i]) < 1e-8)
		{
			// Check that origin is in the box
			// if not, then it doesn't intersect..
			if ( (origin[i] < boxMin[i] - flTolerance) || (origin[i] > boxMax[i] + flTolerance) )
				return false;

			continue;
		}

		// non-parallel case
		// Find the t's corresponding to the entry and exit of
		// the ray along x, y, and z. The find the furthest entry
		// point, and the closest exit point. Once that is done,
		// we know we don't collide if the closest exit point
		// is behind the starting location. We also don't collide if
		// the closest exit point is in front of the furthest entry point

		float invDelta = 1.0f / vecDelta[i];
		float t1 = (boxMin[i] - flTolerance - origin[i]) * invDelta;
		float t2 = (boxMax[i] + flTolerance - origin[i]) * invDelta;
		if (t1 > t2)
		{
			float temp = t1;
			t1 = t2;
			t2 = temp;
		}
		if (t1 > tmin)
			tmin = t1;
		if (t2 < tmax)
			tmax = t2;
		if (tmin > tmax)
			return false;
		if (tmax < 0)
			return false;
		if (tmin > 1)
			return false;
	}

	return true;
#endif
}









bool IntersectRayWithBox( const Ray_t &ray, const Vector &boxMins, const Vector &boxMaxs, 
						 float flTolerance, CBaseTrace *pTrace, float *pFractionLeftSolid )
{
	if ( !ray.m_IsRay )
	{
		Vector vecExpandedMins = boxMins - ray.m_Extents;
		Vector vecExpandedMaxs = boxMaxs + ray.m_Extents;
		bool bIntersects = IntersectRayWithBox( ray.m_Start, ray.m_Delta, vecExpandedMins, vecExpandedMaxs, flTolerance, pTrace, pFractionLeftSolid );
		pTrace->startpos += ray.m_StartOffset;
		pTrace->endpos += ray.m_StartOffset;
		return bIntersects;
	}
	return IntersectRayWithBox( ray.m_Start, ray.m_Delta, boxMins, boxMaxs, flTolerance, pTrace, pFractionLeftSolid );
}

bool IntersectRayWithOBB( const Vector &vecRayStart, const Vector &vecRayDelta, 
	const matrix3x4_t &matOBBToWorld, const Vector &vecOBBMins, const Vector &vecOBBMaxs, 
	float flTolerance, BoxTraceInfo_t *pTrace )
{
	// FIXME: Two transforms is pretty expensive. Should we optimize this?
	Vector start, delta;
	VectorITransform( vecRayStart, matOBBToWorld, start );
	VectorIRotate( vecRayDelta, matOBBToWorld, delta );

	return IntersectRayWithBox( start, delta, vecOBBMins, vecOBBMaxs, flTolerance, pTrace ); 
}

bool IsPointInBox( const Vector& pt, const Vector& boxMin, const Vector& boxMax )
{
	Assert( boxMin[0] <= boxMax[0] );
	Assert( boxMin[1] <= boxMax[1] );
	Assert( boxMin[2] <= boxMax[2] );

	// on x360, force use of SIMD version.
	if (IsX360())
	{
		return IsPointInBox( LoadUnaligned3SIMD(pt.Base()), LoadUnaligned3SIMD(boxMin.Base()), LoadUnaligned3SIMD(boxMax.Base()) ) ;
	}

	if ( (pt[0] > boxMax[0]) || (pt[0] < boxMin[0]) )
		return false;
	if ( (pt[1] > boxMax[1]) || (pt[1] < boxMin[1]) )
		return false;
	if ( (pt[2] > boxMax[2]) || (pt[2] < boxMin[2]) )
		return false;
	return true;
}


//-----------------------------------------------------------------------------
// Intersects a ray with a ray, return true if they intersect
// t, s = parameters of closest approach (if not intersecting!)
//-----------------------------------------------------------------------------
bool IntersectRayWithRay( const Ray_t &ray0, const Ray_t &ray1, float &t, float &s )
{
	Assert( ray0.m_IsRay && ray1.m_IsRay );

	//
	// r0 = p0 + v0t
	// r1 = p1 + v1s
	//
	// intersection : r0 = r1 :: p0 + v0t = p1 + v1s
	// NOTE: v(0,1) are unit direction vectors
	//
	// subtract p0 from both sides and cross with v1 (NOTE: v1 x v1 = 0)
	//  (v0 x v1)t = ((p1 - p0 ) x v1)
	//
	// dotting  with (v0 x v1) and dividing by |v0 x v1|^2
	//	t = Det | (p1 - p0) , v1 , (v0 x v1) | / |v0 x v1|^2
	//  s = Det | (p1 - p0) , v0 , (v0 x v1) | / |v0 x v1|^2
	//
	//  Det | A B C | = -( A x C ) dot B or -( C x B ) dot A
	// 
	//  NOTE: if |v0 x v1|^2 = 0, then the lines are parallel
	//
	Vector v0( ray0.m_Delta );
	Vector v1( ray1.m_Delta );
	VectorNormalize( v0 );
	VectorNormalize( v1 );

	Vector v0xv1 = v0.Cross( v1 );
	float lengthSq = v0xv1.LengthSqr();
	if( lengthSq == 0.0f )
	{
		t = 0; s = 0;
		return false;		// parallel
	}

	Vector p1p0 = ray1.m_Start - ray0.m_Start;

	Vector AxC = p1p0.Cross( v0xv1 );
	AxC.Negate();
	float detT = AxC.Dot( v1 );
	
	AxC = p1p0.Cross( v0xv1 );
	AxC.Negate();
	float detS = AxC.Dot( v0 );

	t = detT / lengthSq;
	s = detS / lengthSq;

	// intersection????
	Vector i0, i1;
	i0 = v0 * t;
	i1 = v1 * s;
	i0 += ray0.m_Start;
	i1 += ray1.m_Start;
	if( i0.x == i1.x && i0.y == i1.y && i0.z == i1.z )
		return true;

	return false;
}


float IntersectRayWithAAPlane( const Vector& vecStart, const Vector& vecEnd, int nAxis, float flSign, float flDist )
{
	float denom	= flSign * (vecEnd[nAxis] - vecStart[nAxis]);
	if (denom == 0.0f)
		return 0.0f;

	denom = 1.0f / denom;
	return (flDist - flSign * vecStart[nAxis]) * denom;
}

