#include "CNPCBaseWeapon.h"
#include "CE_recipientfilter.h"
#include "CAI_NPC.h"
#include "globalstate.h"


class CNPCWeapon_AlyxGun : public CNPCBaseSelectFireMachineGunWeapon
{
public:
	DECLARE_CLASS( CNPCWeapon_AlyxGun, CNPCBaseSelectFireMachineGunWeapon );

	void Spawn();
	void Precache();
	void Drop( const Vector &vecVelocity );
	int		GetMinBurst( void );
	int		GetMaxBurst( void );
	float	GetMinRestTime( void );
	float	GetMaxRestTime( void );
	const Vector& GetBulletSpread( void );

	const char *NPCWeaponGetWorldModel() const;
	acttable_t*	NPCWeaponActivityList();
	int	NPCWeaponActivityListCount();
	void NPCWeaponOperator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator );
	void NPCWeaponOperator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary );
	const WeaponProficiencyInfo_t *NPCWeaponGetProficiencyValues();

	int		WeaponRangeAttack1Condition( float flDot, float flDist );
	int		WeaponRangeAttack2Condition( float flDot, float flDist );
	void OnNPCEquip(CCombatCharacter *owner);

private:
	void FireNPCPrimaryAttack( CCombatCharacter *pOperator, bool bUseWeaponAngles );

private:
	static acttable_t m_acttable[];

	float m_flTooCloseTimer;

};


static CEntityFactory_CE<CNPCWeapon_AlyxGun> WEAPON_ALYXGUN_REPLACE(WEAPON_ALYXGUN_REPLACE_NAME);



acttable_t	CNPCWeapon_AlyxGun::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE_PISTOL,				true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_PISTOL,			true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_PISTOL,		true },
	{ ACT_RELOAD,					ACT_RELOAD_PISTOL,				true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_PISTOL,			true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_PISTOL,				true },
	{ ACT_COVER_LOW,				ACT_COVER_PISTOL_LOW,			true },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_PISTOL_LOW,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_PISTOL,true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_PISTOL_LOW,			true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_PISTOL_LOW,	true },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_PISTOL,		true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL,				false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_STIMULATED,			false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_STEALTH,				ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_RELAXED,				ACT_WALK,						false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_STIMULATED,			false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_STEALTH,				ACT_WALK_STEALTH_PISTOL,		false },

	{ ACT_RUN_RELAXED,				ACT_RUN,						false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_STIMULATED,				false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_STEALTH,				ACT_RUN_STEALTH_PISTOL,			false },

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_PISTOL,				false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_ANGRY_PISTOL,			false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_AIM_STEALTH,			ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK,						false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_PISTOL,			false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_AIM_STEALTH,			ACT_WALK_AIM_STEALTH_PISTOL,	false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN,						false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_PISTOL,				false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_AIM_STEALTH,			ACT_RUN_AIM_STEALTH_PISTOL,		false },//always aims
	//End readiness activities

	// Crouch activities
	{ ACT_CROUCHIDLE_STIMULATED,	ACT_CROUCHIDLE_STIMULATED,		false },
	{ ACT_CROUCHIDLE_AIM_STIMULATED,ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims
	{ ACT_CROUCHIDLE_AGITATED,		ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims

	// Readiness translations
	{ ACT_READINESS_RELAXED_TO_STIMULATED, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED, false },
	{ ACT_READINESS_RELAXED_TO_STIMULATED_WALK, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK, false },
	{ ACT_READINESS_AGITATED_TO_STIMULATED, ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED, false },
	{ ACT_READINESS_STIMULATED_TO_RELAXED, ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED, false },


//	{ ACT_ARM,				ACT_ARM_PISTOL,					true },
//	{ ACT_DISARM,			ACT_DISARM_PISTOL,				true },
};


bool IsAlyxInInjuredMode( void )
{
	if ( hl2_episodic->GetBool() == false )
		return false;

	return ( GlobalEntity_GetState("ep2_alyx_injured") == GLOBAL_ON );
}


void CNPCWeapon_AlyxGun::Spawn()
{
	m_iWeaponModel = PrecacheModel("models/weapons/W_Alyx_Gun.mdl");
	BaseClass::Spawn();
}

void CNPCWeapon_AlyxGun::Precache()
{
	PrecacheScriptSound("Weapon_AlyxGun.Reload");
	PrecacheScriptSound("Weapon_AlyxGun.Reload");
	PrecacheScriptSound("Weapon_AlyxGun.Empty");
	PrecacheScriptSound("Weapon_AlyxGun.Burst");
	PrecacheScriptSound("Weapon_AlyxGun.Single");
	PrecacheScriptSound("Weapon_AlyxGun.Special1");
	PrecacheScriptSound("Weapon_AlyxGun.Special2");
	PrecacheScriptSound("Weapon_AlyxGun.Single");

	BaseClass::Precache();
}

const char *CNPCWeapon_AlyxGun::NPCWeaponGetWorldModel() const
{
	return "models/weapons/W_Alyx_Gun.mdl";
}

acttable_t*	CNPCWeapon_AlyxGun::NPCWeaponActivityList()
{
	return m_acttable;
}

int	CNPCWeapon_AlyxGun::NPCWeaponActivityListCount()
{
	return ARRAYSIZE(m_acttable);
}

void CNPCWeapon_AlyxGun::Drop( const Vector &vecVelocity )
{
	if(IsNPCUsing())
	{
		UTIL_Remove(this);
	} else {
		BaseClass::Drop(vecVelocity);
	}
}

#define TOOCLOSETIMER_OFF	0.0f
#define ALYX_TOOCLOSETIMER	1.0f		// Time an enemy must be tooclose before Alyx is allowed to shoot it.

void CNPCWeapon_AlyxGun::OnNPCEquip(CCombatCharacter *owner)
{
	m_fMinRange1		= 1;
	m_fMaxRange1		= 5000;

	m_flTooCloseTimer	= TOOCLOSETIMER_OFF;

#ifdef HL2_EPISODIC
	m_fMinRange1		= 60;
	m_fMaxRange1		= 2048;
#endif//HL2_EPISODIC

}

int	CNPCWeapon_AlyxGun::WeaponRangeAttack1Condition( float flDot, float flDist )
{
	if(!IsNPCUsing())
		return WeaponRangeAttack1Condition(flDot, flDist);

#ifdef HL2_EPISODIC
	
	if( flDist < m_fMinRange1 )
	{
		// If Alyx is not able to fire because an enemy is too close, start a timer.
		// If the condition persists, allow her to ignore it and defend herself. The idea
		// is to stop Alyx being content to fire point blank at enemies if she's able to move
		// away, without making her defenseless if she's not able to move.
		float flTime;

		if( m_flTooCloseTimer == TOOCLOSETIMER_OFF )
		{
			m_flTooCloseTimer = gpGlobals->curtime;
		}

		flTime = gpGlobals->curtime - m_flTooCloseTimer;

		if( flTime > ALYX_TOOCLOSETIMER )
		{
			// Fake the range to allow Alyx to shoot.
			flDist = m_fMinRange1 + 1.0f;
		}
	}
	else
	{
		m_flTooCloseTimer = TOOCLOSETIMER_OFF;
	}

	int nBaseCondition = BaseClass::WeaponRangeAttack1Condition( flDot, flDist );

	// While in a vehicle, we extend our aiming cone (this relies on COND_NOT_FACING_ATTACK 
	// TODO: This needs to be rolled in at the animation level
	if ( GetOwner()->IsInAVehicle() )
	{
		Vector vecRoughDirection = ( GetOwner()->GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter() );
		Vector vecRight;
		GetVectors( NULL, &vecRight, NULL );
		bool bRightSide = ( DotProduct( vecRoughDirection, vecRight ) > 0.0f );
		float flTargetDot = ( bRightSide ) ? -0.7f : 0.0f;
		
		if ( nBaseCondition == COND_NOT_FACING_ATTACK && flDot >= flTargetDot )
		{
			nBaseCondition = COND_CAN_RANGE_ATTACK1;
		}
	}

	return nBaseCondition;

#else 

	return BaseClass::WeaponRangeAttack1Condition( flDot, flDist );

#endif//HL2_EPISODIC

}

int	CNPCWeapon_AlyxGun::WeaponRangeAttack2Condition( float flDot, float flDist )
{
	if(!IsNPCUsing())
		return WeaponRangeAttack2Condition(flDot, flDist);

	return COND_NONE;
}

void CNPCWeapon_AlyxGun::NPCWeaponOperator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_PISTOL_FIRE:
		{
			CCombatCharacter *owner = (CCombatCharacter *)CEntity::Instance(pOperator);
			FireNPCPrimaryAttack( owner, false );
			break;
		}
		
		default:
			CCombatWeapon::Operator_HandleAnimEvent(pEvent, pOperator);
			break;
	}
}

void CNPCWeapon_AlyxGun::FireNPCPrimaryAttack( CCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	Vector vecShootOrigin, vecShootDir;
	CAI_NPC *npc = pOperator->MyNPCPointer();
	Assert( npc != NULL );

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
		AngleVectors( angShootDir, &vecShootDir );
	}
	else 
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
 		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
	}

	CustomWeaponSound(pOperator->entindex(), vecShootOrigin, "Weapon_AlyxGun.Single");
	g_helpfunc.CSoundEnt_InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2f, pOperator->BaseEntity(), SOUNDENT_CHANNEL_WEAPON, pOperator->CB_GetEnemy() );

	if( hl2_episodic->GetBool() )
	{
		pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1, entindex(), 0, 5.0f );
	}
	else
	{
		pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2,  entindex(), 0, 5.0f  );
	}

	pOperator->DoMuzzleFlash();

	if( hl2_episodic->GetBool() )
	{
		// Never fire Alyx's last bullet just in case there's an emergency
		// and she needs to be able to shoot without reloading.
		if( m_iClip1 > 1 )
		{
			*(m_iClip1) = *(m_iClip1) - 1;
		}
	}
	else
	{
		*(m_iClip1) = *(m_iClip1) - 1;
	}
}

void CNPCWeapon_AlyxGun::NPCWeaponOperator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary )
{
	CCombatCharacter *owner = (CCombatCharacter *)CEntity::Instance(pOperator);

	// Ensure we have enough rounds in the clip
	*(m_iClip1) += 1;

	// HACK: We need the gun to fire its muzzle flash
	if ( bSecondary == false )
	{
		Weapon_SetActivity( ACT_RANGE_ATTACK_PISTOL, 0.0f );
	}

	FireNPCPrimaryAttack( owner, true );
}

int	CNPCWeapon_AlyxGun::GetMinBurst( void )
{
	if(IsNPCUsing())
		return 4;
	else
		return BaseClass::GetMinBurst();
}

int	CNPCWeapon_AlyxGun::GetMaxBurst( void )
{
	if(IsNPCUsing())
		return 7;
	else
		return BaseClass::GetMaxBurst();

}

float CNPCWeapon_AlyxGun::GetMinRestTime( void )
{
	if(IsNPCUsing())
	{
		if ( IsAlyxInInjuredMode() )
			return 1.5f;
		return BaseClass::GetMinRestTime();
	} else
		return BaseClass::GetMinRestTime();

}

float CNPCWeapon_AlyxGun::GetMaxRestTime( void )
{
	if(IsNPCUsing())
	{
		if ( IsAlyxInInjuredMode() )
			return 3.0f;
		return BaseClass::GetMaxRestTime();
	} else
		return BaseClass::GetMaxRestTime();
}

const Vector& CNPCWeapon_AlyxGun::GetBulletSpread( void )
{
	if(!IsNPCUsing())
		return BaseClass::GetBulletSpread();

	static const Vector cone = VECTOR_CONE_2DEGREES;
	static const Vector injuredCone = VECTOR_CONE_6DEGREES;

	if ( IsAlyxInInjuredMode() )
		return injuredCone;

	return cone;
}

const WeaponProficiencyInfo_t *CNPCWeapon_AlyxGun::NPCWeaponGetProficiencyValues()
{
	// Weapon proficiency table. Keep this in sync with WeaponProficiency_t enum in the header!!
	static WeaponProficiencyInfo_t g_BaseWeaponProficiencyTable[] =
	{
		{ 2.50, 1.0	},
		{ 2.00, 1.0	},
		{ 1.50, 1.0	},
		{ 1.25, 1.0 },
		{ 1.00, 1.0	},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(g_BaseWeaponProficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return g_BaseWeaponProficiencyTable;
}

