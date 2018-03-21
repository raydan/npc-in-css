
#ifndef	GRENADE_FRAG_H
#define	GRENADE_FRAG_H

#include "CEntity.h"
#include "CGrenade.h"


CE_Grenade *Fraggrenade_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CEntity *pOwner, float timer, bool combineSpawned );

bool Fraggrenade_WasCreatedByCombine( const CEntity *pEntity );

#endif


