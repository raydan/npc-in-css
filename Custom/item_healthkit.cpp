
#include "item_healthkit.h"
#include "CPlayer.h"
#include "CE_recipientfilter.h"
#include "ItemRespawnSystem.h"
#include "vphysics/constraints.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CUSTOM_CLASS( item_healthkit, item_sodacan, CHealthKit );
LINK_ENTITY_TO_CUSTOM_CLASS( item_healthvial, item_sodacan, CHealthVial );


ConVar	sk_healthkit( "sk_healthkit","0" );		
ConVar	sk_healthvial( "sk_healthvial","0" );		
ConVar	sk_healthcharger( "sk_healthcharger","0" );

ConVar sk_healthcharger_rechargetime( "sk_healthcharger_rechargetime","0" );



void CHealthKit::Spawn( void )
{
	m_bRespawn = true;
	Precache();
	SetModel( "models/items/healthkit.mdl" );
	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHealthKit::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel("models/items/healthkit.mdl");
	PrecacheScriptSound( "HealthKit.Touch" );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : 
//-----------------------------------------------------------------------------
CEntity *CHealthKit::MyTouch( CPlayer *pPlayer )
{
	if ( pPlayer->TakeHealth( sk_healthkit.GetFloat(), DMG_GENERIC ) )
	{
		CPASAttenuationFilter filter( pPlayer, "HealthKit.Touch" );
		EmitSound( filter, pPlayer->entindex(), "HealthKit.Touch" );

		Respawn();
		return this;
	}
	return NULL;
}




void CHealthVial::Spawn( void )
{
	m_bRespawn = true;
	Precache();
	SetModel( "models/healthvial.mdl" );
	BaseClass::Spawn();
}

void CHealthVial::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel("models/healthvial.mdl");
	PrecacheScriptSound( "HealthVial.Touch" );
}

CEntity *CHealthVial::MyTouch( CPlayer *pPlayer )
{
	if ( pPlayer->TakeHealth( sk_healthvial.GetFloat(), DMG_GENERIC ) )
	{
		CPASAttenuationFilter filter( pPlayer, "HealthVial.Touch" );
		EmitSound( filter, pPlayer->entindex(), "HealthVial.Touch" );

		Respawn();
		
		return this;
	}
	return NULL;
}



//-----------------------------------------------------------------------------
// Wall mounted health kit. Heals the player when used.
//-----------------------------------------------------------------------------
class CNewWallHealth : public CSoda_Fix
{
public:
	CE_DECLARE_CLASS( CNewWallHealth, CSoda_Fix );
	
	CNewWallHealth();

	void Spawn( );
	void Precache( void );
	bool CreateVPhysics(void);
	void Off(void);
	void Recharge(void);
	bool DispatchKeyValue( const char *szKeyName, const char *szValue );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_WCEDIT_POSITION | m_iCaps; };

	float m_flNextCharge; 
	int		m_iReactivate ; // DeathMatch Delay until reactvated
	int		m_iJuice;
	int		m_iOn;			// 0 = off, 1 = startup, 2 = going
	float   m_flSoundTime;

	int		m_nState;
	int		m_iCaps;

	COutputFloat m_OutRemainingHealth;
	COutputEvent m_OnPlayerUse;

	void StudioFrameAdvance ( void );

	float m_flJuice;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CUSTOM_CLASS( item_healthcharger, item_sodacan, CNewWallHealth );


BEGIN_DATADESC( CNewWallHealth )

	DEFINE_FIELD( m_flNextCharge, FIELD_TIME),
	DEFINE_FIELD( m_iReactivate, FIELD_INTEGER),
	DEFINE_FIELD( m_iJuice, FIELD_INTEGER),
	DEFINE_FIELD( m_iOn, FIELD_INTEGER),
	DEFINE_FIELD( m_flSoundTime, FIELD_TIME),
	DEFINE_FIELD( m_nState, FIELD_INTEGER ),
	DEFINE_FIELD( m_iCaps, FIELD_INTEGER ),
	DEFINE_FIELD( m_flJuice, FIELD_FLOAT ),

	// Function Pointers
	//DEFINE_FUNCTION( Off ),
	//DEFINE_FUNCTION( Recharge ),

	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse" ),
	DEFINE_OUTPUT( m_OutRemainingHealth, "OutRemainingHealth"),

END_DATADESC()

#define HEALTH_CHARGER_MODEL_NAME "models/props_combine/health_charger001.mdl"
#define CHARGE_RATE 0.25f
#define CHARGES_PER_SECOND 1.0f / CHARGE_RATE
#define CALLS_PER_SECOND 7.0f * CHARGES_PER_SECOND


CNewWallHealth::CNewWallHealth()
{
	m_flNextCharge = 0.0f;
	m_iReactivate = 0;
	m_iJuice = 0;
	m_iOn = 0;
	m_flSoundTime = 0.0f;
	m_nState = 0;
	m_iCaps = 0;
	m_flJuice = 0;

}

bool CNewWallHealth::DispatchKeyValue(  const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "style") ||
		FStrEq(szKeyName, "height") ||
		FStrEq(szKeyName, "value1") ||
		FStrEq(szKeyName, "value2") ||
		FStrEq(szKeyName, "value3"))
	{
		return(true);
	}
	else if (FStrEq(szKeyName, "dmdelay"))
	{
		m_iReactivate = atoi(szValue);
		return(true);
	}

	return(BaseClass::DispatchKeyValue( szKeyName, szValue ));
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNewWallHealth::Spawn(void)
{
	Precache( );

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_VPHYSICS );
	CreateVPhysics();

	SetModel( HEALTH_CHARGER_MODEL_NAME );
	AddEffects( EF_NOSHADOW );

	ResetSequence( LookupSequence( "idle" ) );

	m_iJuice = sk_healthcharger.GetInt();

	m_nState = 0;	
	
	m_iReactivate = 0;
	m_iCaps	= FCAP_CONTINUOUS_USE;

	CreateVPhysics();

	m_flJuice = m_iJuice;
	SetCycle( 1.0f - ( m_flJuice /  sk_healthcharger.GetFloat() ) );
}


//-----------------------------------------------------------------------------

bool CNewWallHealth::CreateVPhysics(void)
{
	VPhysicsInitStatic();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNewWallHealth::Precache(void)
{
	PrecacheModel( HEALTH_CHARGER_MODEL_NAME );

	PrecacheScriptSound("AlyxEmp.Charge");

	PrecacheScriptSound( "WallHealth.Deny" );
	PrecacheScriptSound( "WallHealth.Start" );
	PrecacheScriptSound( "WallHealth.LoopingContinueCharge" );
	PrecacheScriptSound( "WallHealth.Recharge" );
}

void CNewWallHealth::StudioFrameAdvance( void )
{
	m_flPlaybackRate = 0;

	float flMaxJuice = sk_healthcharger.GetFloat();
	
	SetCycle( 1.0f - (float)( m_flJuice / flMaxJuice ) );
//	Msg( "Cycle: %f - Juice: %d - m_flJuice :%f - Interval: %f\n", (float)GetCycle(), (int)m_iJuice, (float)m_flJuice, GetAnimTimeInterval() );

	if ( !m_flPrevAnimTime )
	{
		m_flPrevAnimTime = gpGlobals->curtime;
	}

	// Latch prev
	*(m_flPrevAnimTime) = m_flAnimTime;
	// Set current
	m_flAnimTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CNewWallHealth::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	CEntity *cent_pActivator = CEntity::Instance(pActivator);
	// Make sure that we have a caller
	if (!cent_pActivator)
		return;

	// if it's not a player, ignore
	if ( !cent_pActivator->IsPlayer() )
		return;

	CPlayer *pPlayer = dynamic_cast<CPlayer *>(cent_pActivator);

	// Reset to a state of continuous use.
	m_iCaps = FCAP_CONTINUOUS_USE;

	if ( m_iOn )
	{
		float flCharges = CHARGES_PER_SECOND;
		float flCalls = CALLS_PER_SECOND;

		m_flJuice -= flCharges / flCalls;
		StudioFrameAdvance();
	}

	// if there is no juice left, turn it off
	if (m_iJuice <= 0)
	{
		ResetSequence( LookupSequence( "emptyclick" ) );
		m_nState = 1;			
		Off();
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise.
	// disabled HEV suit dependency for now.
	//if ((m_iJuice <= 0) || (!(pActivator->m_bWearingSuit)))
	if (m_iJuice <= 0)
	{
		if (m_flSoundTime <= gpGlobals->curtime)
		{
			m_flSoundTime = gpGlobals->curtime + 0.62;
			EmitSound( "WallHealth.Deny" );
		}
		return;
	}

	if( cent_pActivator->GetHealth() >= cent_pActivator->GetMaxHealth() )
	{
		if( pPlayer )
		{
			pPlayer->m_afButtonPressed &= ~IN_USE;
		}

		// Make the user re-use me to get started drawing health.
		m_iCaps = FCAP_IMPULSE_USE;
		
		EmitSound( "WallHealth.Deny" );
		return;
	}

	SetNextThink( gpGlobals->curtime + CHARGE_RATE );
	SetThink( &CNewWallHealth::Off );

	// Time to recharge yet?

	if (m_flNextCharge >= gpGlobals->curtime)
		return;

	// Play the on sound or the looping charging sound
	if (!m_iOn)
	{
		m_iOn++;
		EmitSound( "WallHealth.Start" );
		m_flSoundTime = 0.56 + gpGlobals->curtime;

		m_OnPlayerUse.FireOutput( pActivator, this );
	}
	if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->curtime))
	{
		m_iOn++;
		CPASAttenuationFilter filter( this, "WallHealth.LoopingContinueCharge" );
		filter.MakeReliable();
		EmitSound( filter, entindex(), "WallHealth.LoopingContinueCharge" );
	}

	// charge the player
	if ( cent_pActivator->TakeHealth( 1, DMG_GENERIC ) )
	{
		m_iJuice--;
	}

	// Send the output.
	float flRemaining = m_iJuice / sk_healthcharger.GetFloat();
	m_OutRemainingHealth.Set(flRemaining, pActivator, BaseEntity());

	// govern the rate of charge
	m_flNextCharge = gpGlobals->curtime + 0.1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNewWallHealth::Recharge(void)
{
	EmitSound( "WallHealth.Recharge" );
	m_flJuice = m_iJuice = sk_healthcharger.GetInt();
	m_nState = 0;

	ResetSequence( LookupSequence( "idle" ) );
	StudioFrameAdvance();

	m_iReactivate = 0;

	SetThink( NULL );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNewWallHealth::Off(void)
{
	// Stop looping sound.
	if (m_iOn > 1)
		StopSound( "WallHealth.LoopingContinueCharge" );

	if ( m_nState == 1 )
	{
		SetCycle( 1.0f );
	}

	m_iOn = 0;
	m_flJuice = m_iJuice;

	if ( m_iReactivate == 0 )
	{
		if ((!m_iJuice) && sk_healthcharger_rechargetime.GetFloat() > 0 )
		{
			m_iReactivate = (int)sk_healthcharger_rechargetime.GetFloat();
			SetNextThink( gpGlobals->curtime + m_iReactivate );
			SetThink(&CNewWallHealth::Recharge);
		}
		else
			SetThink( NULL );
	}
}


