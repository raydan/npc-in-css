
#include "CGrenade.h"
#include "CCombatCharacter.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(CBaseGrenade, CE_Grenade);





SH_DECL_MANUALHOOK0_void(Detonate, 0, 0, 0);
DECLARE_HOOK(Detonate, CE_Grenade);
DECLARE_DEFAULTHANDLER_void(CE_Grenade, Detonate, (), ());

SH_DECL_MANUALHOOK0(GetShakeAmplitude, 0, 0, 0, float);
DECLARE_HOOK(GetShakeAmplitude, CE_Grenade);
DECLARE_DEFAULTHANDLER(CE_Grenade, GetShakeAmplitude, float, (), ());

SH_DECL_MANUALHOOK0(GetShakeRadius, 0, 0, 0, float);
DECLARE_HOOK(GetShakeRadius, CE_Grenade);
DECLARE_DEFAULTHANDLER(CE_Grenade, GetShakeRadius, float, (), ());

SH_DECL_MANUALHOOK2_void(Explode, 0, 0, 0, trace_t *, int);
DECLARE_HOOK(Explode, CE_Grenade);
DECLARE_DEFAULTHANDLER_void(CE_Grenade, Explode, (trace_t *pTrace, int bitsDamageType), (pTrace, bitsDamageType));



// Sendprops
DEFINE_PROP(m_flDamage, CE_Grenade);
DEFINE_PROP(m_DmgRadius, CE_Grenade);
DEFINE_PROP(m_hThrower, CE_Grenade);


//Datamaps
DEFINE_PROP(m_flDetonateTime, CE_Grenade);
DEFINE_PROP(m_flWarnAITime, CE_Grenade);
DEFINE_PROP(m_iszBounceSound, CE_Grenade);
DEFINE_PROP(m_bHasWarnedAI, CE_Grenade);



CE_Grenade::CE_Grenade()
{

}

void CE_Grenade::PostConstructor()
{
	BaseClass::PostConstructor();
	m_hOriginalThrower.offset = m_hThrower.offset + 4;
	m_hOriginalThrower.ptr = (CFakeHandle *)(((uint8_t *)(BaseEntity())) + m_hOriginalThrower.offset);
}

CCombatCharacter *CE_Grenade::GetThrower( void )
{
	CCombatCharacter *pResult = ToBaseCombatCharacter( m_hThrower );
	if ( !pResult && GetOwnerEntity() != NULL )
	{
		pResult = ToBaseCombatCharacter( GetOwnerEntity() );
	}
	return pResult;
}

void CE_Grenade::DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CE_Grenade::Detonate );
	SetNextThink( gpGlobals->curtime );
}


void CE_Grenade::SetThrower( CBaseEntity *pThrower )
{
	m_hThrower.ptr->Set(pThrower);

	// if this is the first thrower, set it as the original thrower
	if ( m_hOriginalThrower.ptr->Get() == NULL)
	{
		m_hOriginalThrower.ptr->Set(pThrower);
	}
}


void CE_Grenade::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
}

void CE_Grenade::DangerSoundThink( void )
{
	if (!IsInWorld())
	{
		Remove( );
		return;
	}

	g_helpfunc.CSoundEnt_InsertSound ( SOUND_DANGER, GetAbsOrigin() + GetAbsVelocity() * 0.5, (int)GetAbsVelocity().Length( ), 0.2, BaseEntity() );

	SetNextThink( gpGlobals->curtime + 0.2 );

	if (GetWaterLevel() != 0)
	{
		SetAbsVelocity( GetAbsVelocity() * 0.5 );
	}
}

void CE_Grenade::ExplodeTouch( CEntity *pOther )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	Assert( pOther );
	if ( !pOther->IsSolid() )
		return;

	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID_BRUSHONLY, BaseEntity(), COLLISION_GROUP_NONE, &tr );

	Explode( &tr, DMG_BLAST );
}


void CE_Grenade::SetDamageRadius(float flDamageRadius)
{
	m_DmgRadius = flDamageRadius;
}


