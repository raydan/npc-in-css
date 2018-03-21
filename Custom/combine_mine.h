
#ifndef COMBINE_MINE_H
#define COMBINE_MINE_H

#include "CEntity.h"
#include "CSoda_Fix.h"
#include "player_pickup.h"


class CSoundPatch;
class CE_CSprite;


//---------------------------------------------------------
//---------------------------------------------------------
#define BOUNCEBOMB_HOOK_RANGE		64
#define BOUNCEBOMB_WARN_RADIUS		245.0	// Must be slightly less than physcannon!
#define BOUNCEBOMB_DETONATE_RADIUS	100.0

#define BOUNCEBOMB_EXPLODE_RADIUS	125
#define BOUNCEBOMB_EXPLODE_DAMAGE	150

class CBounceBomb : public CSoda_Fix, public CDefaultPlayerPickupVPhysics
{
public:
	CE_DECLARE_CLASS( CBounceBomb, CSoda_Fix );

	CBounceBomb() { m_pWarnSound = NULL; m_bPlacedByPlayer = false; }
	void UpdateOnRemove();
	void Precache();
	void Spawn();
	void OnRestore();
	void SetMineState( int iState );
	int GetMineState() { return m_iMineState; }
	bool IsValidLocation();
	void Flip( const Vector &vecForce, const AngularImpulse &torque );
	void SearchThink();
	void BounceThink();
	void SettleThink();
	void CaptiveThink();
	void ExplodeThink();
	void ExplodeTouch( CEntity *pOther );
	void CavernBounceThink(); ///< an alternative style of bouncing used for the citizen modded bouncers
	bool IsAwake() { return m_bAwake; }
	void Wake( bool bWake );
	float FindNearestNPC();
	void SetNearestNPC( CEntity *pNearest ) { m_hNearestNPC.Set( (pNearest)?pNearest->BaseEntity():NULL ); }
	int OnTakeDamage( const CTakeDamageInfo &info );
	bool IsFriend( CEntity *pEntity );

	void UpdateLight( bool bTurnOn, unsigned int r, unsigned int g, unsigned int b, unsigned int a );
	bool IsLightOn() { return m_hSprite.Get() != NULL; }

	void OnPhysGunPickup( CBaseEntity *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON );
	void OnPhysGunDrop( CBaseEntity *pPhysGunUser, PhysGunDrop_t reason );
	bool ForcePhysgunOpen( CBaseEntity *pPlayer ) { return true; }
	bool HasPreferredCarryAnglesForPlayer( CBaseEntity *pPlayer ) { return true; }
	virtual QAngle	PreferredCarryAngles( void ) { return vec3_angle; }
	CBaseEntity *HasPhysicsAttacker( float dt );

	bool IsPlayerPlaced() { return m_bPlacedByPlayer; }

	bool CreateVPhysics()
	{
		VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
		return true;
	}

	void Pickup();

	void OpenHooks( bool bSilent = false );
	void CloseHooks();

	DECLARE_DATADESC();

	static string_t gm_iszFloorTurretClassname;
	static string_t gm_iszGroundTurretClassname;

private:
	float		m_flExplosionDelay;

	bool	m_bAwake;
	bool	m_bBounce;
	CFakeHandle	m_hNearestNPC;
	CEFakeHandle<CE_CSprite> m_hSprite;
	Color 	m_LastSpriteColor;

	float	m_flHookPositions;
	int		m_iHookN;
	int		m_iHookE;
	int		m_iHookS;
	int		m_iAllHooks;

	CSoundPatch	*m_pWarnSound;

	bool	m_bLockSilently;
	bool	m_bFoeNearest;

	float	m_flIgnoreWorldTime;

	bool	m_bDisarmed;

	bool	m_bPlacedByPlayer;

	bool	m_bHeldByPhysgun;

	int		m_iFlipAttempts;
	int     m_iModification;

	CEFakeHandle<CPlayer>	m_hPhysicsAttacker;
	float					m_flLastPhysicsInfluenceTime;

	float					m_flTimeGrabbed;
	IPhysicsConstraint		*m_pConstraint;
	int						m_iMineState;

	COutputEvent	m_OnPulledUp;
	void InputDisarm( inputdata_t &inputdata );
};



#endif // COMBINE_MINE_H
