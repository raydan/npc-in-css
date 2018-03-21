//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Projectile shot by wasteland scanner 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	GRENADEPATHFOLLOWER_H
#define	GRENADEPATHFOLLOWER_H

#include "CEntity.h"
#include "CGrenade.h"

class CRocketTrail;

class CGrenadePathfollower : public CE_Grenade
{
public:
	CE_DECLARE_CLASS( CGrenadePathfollower, CE_Grenade );

	static CGrenadePathfollower* CreateGrenadePathfollower( string_t sModelName, string_t sFlySound, const Vector &vecOrigin, const QAngle &vecAngles, edict_t *pentOwner );

	CEFakeHandle<CRocketTrail>	m_hRocketTrail;
	CEntity*		m_pPathTarget;				// path corner we are heading towards
	float			m_flFlySpeed;
	string_t		m_sFlySound;
	float			m_flNextFlySoundTime;

	Class_T			Classify( void);
	void			Spawn( void );
	void			AimThink( void );
	void 			GrenadeTouch( CEntity *pOther );
	void			Event_Killed( const CTakeDamageInfo &info );
	void			Launch( float flLaunchSpeed, string_t sPathCornerName);
	void			PlayFlySound(void);

	void			Detonate(void);

	CGrenadePathfollower(void);
	~CGrenadePathfollower(void);

	virtual void Precache();

	DECLARE_DATADESC();
};

#endif	//GRENADEPATHFOLLOWER_H
