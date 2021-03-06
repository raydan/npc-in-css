//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "CEntity.h"
#include "CAI_NPC.h"

#include "npc_playercompanion.h"

#include "func_tank.h"
#include "npcevent.h"
#include "CAI_Hint.h"
#include "CAI_Memory.h"
#include "CAI_pathfinder.h"
#include "CAI_Route.h"
#include "CAI_senses.h"
#include "CAI_Squad.h"
#include "CAI_tacticalservices.h"
#include "CAI_Interactions.h"
#include <KeyValues.h>
#include "CCombatWeapon.h"
#include "CNPCBaseWeapon.h"
#include "CFire.h"
#include "CBreakableProp.h"
#include "grenade_frag.h"
#include "fmtstr.h"
#include "globalstate.h"
#include "combine_mine.h"
#include "weapon_rpg_replace.h"

int *g_interactionHitByPlayerThrownPhysObj = NULL;
int g_interactionPlayerPuntedHeavyObject = 0;

ConVar ai_debug_readiness("ai_debug_readiness", "0" );
ConVar ai_use_readiness("ai_use_readiness", "1" ); // 0 = off, 1 = on, 2 = on for player squad only
ConVar ai_readiness_decay( "ai_readiness_decay", "120" );// How many seconds it takes to relax completely
ConVar ai_new_aiming( "ai_new_aiming", "1" );

#define GetReadinessUse()	ai_use_readiness.GetInt()

extern ConVar *g_debug_transitions;

#define PLAYERCOMPANION_TRANSITION_SEARCH_DISTANCE		(100*12)

int AE_COMPANION_PRODUCE_FLARE;
int AE_COMPANION_LIGHT_FLARE;
int AE_COMPANION_RELEASE_FLARE;

#define MAX_TIME_BETWEEN_BARRELS_EXPLODING			5.0f
#define MAX_TIME_BETWEEN_CONSECUTIVE_PLAYER_KILLS	3.0f

//-----------------------------------------------------------------------------
// An aimtarget becomes invalid if it gets this close
//-----------------------------------------------------------------------------
#define COMPANION_AIMTARGET_NEAREST		24.0f
#define COMPANION_AIMTARGET_NEAREST_SQR	576.0f

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

BEGIN_DATADESC( CNPC_PlayerCompanion )

	DEFINE_FIELD( 	m_bMovingAwayFromPlayer, 	FIELD_BOOLEAN ),
	DEFINE_EMBEDDED( m_SpeechWatch_PlayerLooking ),
	DEFINE_EMBEDDED( m_FakeOutMortarTimer ),

// (recomputed)
//						m_bWeightPathsInCover	

// These are auto-saved by AI
//	DEFINE_FIELD( m_AssaultBehavior,	CAI_AssaultBehavior ),
//	DEFINE_FIELD( m_FollowBehavior,		CAI_FollowBehavior ),
//	DEFINE_FIELD( m_StandoffBehavior,	CAI_StandoffBehavior ),
//	DEFINE_FIELD( m_LeadBehavior,		CAI_LeadBehavior ),
//  DEFINE_FIELD( m_OperatorBehavior,	FIELD_EMBEDDED ),
//					m_ActBusyBehavior
//					m_PassengerBehavior
//					m_FearBehavior

	DEFINE_INPUTFUNC( FIELD_VOID,	"OutsideTransition",	InputOutsideTransition ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetReadinessPanic",	InputSetReadinessPanic ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetReadinessStealth",	InputSetReadinessStealth ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetReadinessLow",		InputSetReadinessLow ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetReadinessMedium",	InputSetReadinessMedium ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SetReadinessHigh",		InputSetReadinessHigh ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"LockReadiness",		InputLockReadiness ),

//------------------------------------------------------------------------------
#ifdef HL2_EPISODIC
	DEFINE_FIELD( m_hFlare, FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( FIELD_STRING,	"EnterVehicle",				InputEnterVehicle ),
	DEFINE_INPUTFUNC( FIELD_STRING, "EnterVehicleImmediately",	InputEnterVehicleImmediately ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ExitVehicle",				InputExitVehicle ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"CancelEnterVehicle",		InputCancelEnterVehicle ),
#endif	// HL2_EPISODIC
//------------------------------------------------------------------------------

	DEFINE_INPUTFUNC( FIELD_STRING, "GiveWeapon",			InputGiveWeapon ),

	DEFINE_FIELD( m_flReadiness,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flReadinessSensitivity,	FIELD_FLOAT ),
	DEFINE_FIELD( m_bReadinessCapable,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flReadinessLockedUntil, FIELD_TIME ),
	DEFINE_FIELD( m_fLastBarrelExploded,	FIELD_TIME ),
	DEFINE_FIELD( m_iNumConsecutiveBarrelsExploded, FIELD_INTEGER ),
	DEFINE_FIELD( m_fLastPlayerKill, FIELD_TIME ),
	DEFINE_FIELD( m_iNumConsecutivePlayerKills, FIELD_INTEGER ),

	//					m_flBoostSpeed (recomputed)

	DEFINE_EMBEDDED( m_AnnounceAttackTimer ),

	DEFINE_FIELD( m_hAimTarget,				FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_bAlwaysTransition, FIELD_BOOLEAN, "AlwaysTransition" ),
	DEFINE_KEYFIELD( m_bDontPickupWeapons, FIELD_BOOLEAN, "DontPickupWeapons" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "EnableAlwaysTransition", InputEnableAlwaysTransition ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableAlwaysTransition", InputDisableAlwaysTransition ),

	DEFINE_INPUTFUNC( FIELD_VOID, "EnableWeaponPickup", InputEnableWeaponPickup ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableWeaponPickup", InputDisableWeaponPickup ),


#if HL2_EPISODIC
	DEFINE_INPUTFUNC( FIELD_VOID, "ClearAllOutputs", InputClearAllOuputs ),
#endif

	DEFINE_OUTPUT( m_OnWeaponPickup, "OnWeaponPickup" ),

END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CNPC_PlayerCompanion::eCoverType CNPC_PlayerCompanion::gm_fCoverSearchType;
bool CNPC_PlayerCompanion::gm_bFindingCoverFromAllEnemies;
string_t CNPC_PlayerCompanion::gm_iszMortarClassname;
string_t CNPC_PlayerCompanion::gm_iszFloorTurretClassname;
string_t CNPC_PlayerCompanion::gm_iszGroundTurretClassname;
string_t CNPC_PlayerCompanion::gm_iszShotgunClassname;
string_t CNPC_PlayerCompanion::gm_iszRollerMineClassname;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CNPC_PlayerCompanion::CreateBehaviors()
{
#ifdef HL2_EPISODIC
	AddBehavior( &m_FearBehavior );
	AddBehavior( &m_PassengerBehavior );
#endif // HL2_EPISODIC	

	AddBehavior( &m_ActBusyBehavior );

#ifdef HL2_EPISODIC
	AddBehavior( &m_OperatorBehavior );
	AddBehavior( &m_StandoffBehavior );
	AddBehavior( &m_AssaultBehavior );
	AddBehavior( &m_FollowBehavior );
	AddBehavior( &m_LeadBehavior );
#else
	AddBehavior( &m_AssaultBehavior );
	AddBehavior( &m_StandoffBehavior );
	AddBehavior( &m_FollowBehavior );
	AddBehavior( &m_LeadBehavior );
#endif//HL2_EPISODIC
	
	return BaseClass::CreateBehaviors();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::Precache()
{
	gm_iszMortarClassname = AllocPooledString( "func_tankmortar" );
	gm_iszFloorTurretClassname = AllocPooledString( "npc_turret_floor" );
	gm_iszGroundTurretClassname = AllocPooledString( "npc_turret_ground" );
	gm_iszShotgunClassname = AllocPooledString( "weapon_shotgun" );
	gm_iszRollerMineClassname = AllocPooledString( "npc_rollermine" );

	PrecacheModel( STRING( GetModelName() ) );
	
#ifdef HL2_EPISODIC
	// The flare we're able to pull out
	PrecacheModel( "models/props_junk/flare.mdl" );
#endif // HL2_EPISODIC

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::Spawn()
{
	SelectModel();

	Precache();

	SetModel( STRING( GetModelName() ) );

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetBloodColor( BLOOD_COLOR_RED );
	m_flFieldOfView		= 0.02;
	m_NPCState		= NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_SQUAD );

	if ( !HasSpawnFlags( SF_NPC_START_EFFICIENT ) )
	{
		CapabilitiesAdd( bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD );
		CapabilitiesAdd( bits_CAP_USE_WEAPONS | bits_CAP_AIM_GUN | bits_CAP_MOVE_SHOOT );
		CapabilitiesAdd( bits_CAP_DUCK | bits_CAP_DOORS_GROUP );
		CapabilitiesAdd( bits_CAP_USE_SHOT_REGULATOR );
	}
	CapabilitiesAdd( bits_CAP_NO_HIT_PLAYER | bits_CAP_NO_HIT_SQUADMATES | bits_CAP_FRIENDLY_DMG_IMMUNE );
	CapabilitiesAdd( bits_CAP_MOVE_GROUND );
	SetMoveType( MOVETYPE_STEP );

	m_HackedGunPos = Vector( 0, 0, 55 );
	
	SetAimTarget(NULL);
	m_bReadinessCapable = IsReadinessCapable();
	SetReadinessValue( 0.0f );
	SetReadinessSensitivity( enginerandom->RandomFloat( 0.7, 1.3 ) );
	m_flReadinessLockedUntil = 0.0f;

	m_AnnounceAttackTimer.Set( 10, 30 );

#ifdef HL2_EPISODIC
	// We strip this flag because it's been made obsolete by the StartScripting behavior
	if ( HasSpawnFlags( SF_NPC_ALTCOLLISION ) )
	{
		Warning( "NPC %s using alternate collision! -- DISABLED\n", STRING( GetEntityName() ) );
		RemoveSpawnFlags( SF_NPC_ALTCOLLISION );
	}

	m_hFlare = NULL;
#endif // HL2_EPISODIC

	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_PlayerCompanion::Restore( IRestore &restore )
{
	int baseResult = BaseClass::Restore( restore );

	if ( gpGlobals->eLoadType == MapLoad_Transition )
	{
		m_StandoffBehavior.SetActive( false );
	}

#ifdef HL2_EPISODIC
	// We strip this flag because it's been made obsolete by the StartScripting behavior
	if ( HasSpawnFlags( SF_NPC_ALTCOLLISION ) )
	{
		Warning( "NPC %s using alternate collision! -- DISABLED\n", STRING( GetEntityName() ) );
		RemoveSpawnFlags( SF_NPC_ALTCOLLISION );
	}
#endif // HL2_EPISODIC

	return baseResult;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_PlayerCompanion::ObjectCaps() 
{ 
	int caps = UsableNPCObjectCaps( BaseClass::ObjectCaps() );
	return caps; 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::ShouldAlwaysThink() 
{ 
	return ( BaseClass::ShouldAlwaysThink() || ( GetFollowBehavior().GetFollowTarget() && GetFollowBehavior().GetFollowTarget()->IsPlayer() ) ); 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Disposition_t CNPC_PlayerCompanion::IRelationType( CBaseEntity *pTarget )
{
	if ( !pTarget )
		return D_NU;

	Disposition_t baseRelationship = BaseClass::IRelationType( pTarget );

	if ( baseRelationship != D_LI )
	{
		CEntity *cent_pTarget = CEntity::Instance(pTarget);
		if ( IsTurret( cent_pTarget ) )
		{
			// Citizens are afeared of turrets, so long as the turret
			// is active... that is, not classifying itself as CLASS_NONE
			if( cent_pTarget->Classify() != CLASS_NONE )
			{
				if( !hl2_episodic->GetBool() && IsSafeFromFloorTurret(GetAbsOrigin(), cent_pTarget) )
				{
					return D_NU;
				}

				return D_FR;
			}
		}
		else if ( baseRelationship == D_HT && 
				  cent_pTarget->IsNPC() && 
				  ((CAI_NPC *)cent_pTarget)->GetActiveWeapon() && 
				  ((CAI_NPC *)cent_pTarget)->GetActiveWeapon()->ClassMatches( gm_iszShotgunClassname ) &&
				  ( !GetActiveWeapon() || !GetActiveWeapon()->ClassMatches( gm_iszShotgunClassname ) ) )
		{
			if ( (cent_pTarget->GetAbsOrigin() - GetAbsOrigin()).LengthSqr() < Square( 25 * 12 ) )
			{
				// Ignore enemies on the floor above us
				if ( fabs(cent_pTarget->GetAbsOrigin().z - GetAbsOrigin().z) < 100 )
					return D_FR;
			}
		}
	}

	return baseRelationship;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsSilentSquadMember() const
{
	const CAI_Squad *squad = GetSquad();

	if ( (const_cast<CNPC_PlayerCompanion *>(this))->Classify() == CLASS_PLAYER_ALLY_VITAL && squad && MAKE_STRING(squad->GetName()) == GetPlayerSquadName() )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::GatherConditions()
{
	BaseClass::GatherConditions();

	CAI_Squad *squad = GetSquad();
	CPlayer *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
	if ( pPlayer )
	{
		if ( Classify() == CLASS_PLAYER_ALLY_VITAL )
		{
			bool bInPlayerSquad = ( squad && MAKE_STRING(squad->GetName()) == GetPlayerSquadName() );
			if ( bInPlayerSquad )
			{
				if ( GetState() == NPC_STATE_SCRIPT || ( !HasCondition( COND_SEE_PLAYER ) && (GetAbsOrigin() - pPlayer->GetAbsOrigin()).LengthSqr() > Square(50 * 12) ) )
				{
					RemoveFromSquad();
				}
			}
			else if ( GetState() != NPC_STATE_SCRIPT )
			{
				if ( HasCondition( COND_SEE_PLAYER ) && (GetAbsOrigin() - pPlayer->GetAbsOrigin()).LengthSqr() < Square(25 * 12) )
				{
					if ( hl2_episodic->GetBool() )
					{
						// Don't stomp our squad if we're in one
						if ( GetSquad() == NULL )
						{
							AddToSquad( GetPlayerSquadName() );
						}
					}
					else
					{
						AddToSquad( GetPlayerSquadName() );
					}
				}
			}
		}

		m_flBoostSpeed = 0;

		if ( m_AnnounceAttackTimer.Expired() &&
			 ( GetLastEnemyTime() == 0.0 || gpGlobals->curtime - GetLastEnemyTime() > 20 ) )
		{
			// Always delay when an encounter begins
			m_AnnounceAttackTimer.Set( 4, 8 );
		}

		if ( GetFollowBehavior().GetFollowTarget() && 
			 ( GetFollowBehavior().GetFollowTarget()->IsPlayer() || GetCommandGoal() != vec3_invalid ) && 
			 GetFollowBehavior().IsMovingToFollowTarget() && 
			 GetFollowBehavior().GetGoalRange() > 0.1 &&
			 BaseClass::GetIdealSpeed() > 0.1 )
		{
			Vector vPlayerToFollower = GetAbsOrigin() - pPlayer->GetAbsOrigin();
			float dist = vPlayerToFollower.NormalizeInPlace();

			bool bDoSpeedBoost = false;
			if ( !HasCondition( COND_IN_PVS ) )
				bDoSpeedBoost = true;
			else if ( GetFollowBehavior().GetFollowTarget()->IsPlayer() )
			{
				if ( dist > GetFollowBehavior().GetGoalRange() * 2 )
				{
					float dot = vPlayerToFollower.Dot( pPlayer->EyeDirection3D() );
					if ( dot < 0 )
					{
						bDoSpeedBoost = true;
					}
				}
			}

			if ( bDoSpeedBoost )
			{
				float lag = dist / GetFollowBehavior().GetGoalRange();

				float mult;
				
				if ( lag > 10.0 )
					mult = 2.0;
				else if ( lag > 5.0 )
					mult = 1.5;
				else if ( lag > 3.0 )
					mult = 1.25;
				else
					mult = 1.1;

				m_flBoostSpeed = pPlayer->GetSmoothedVelocity().Length();

				if ( m_flBoostSpeed < BaseClass::GetIdealSpeed() )
					m_flBoostSpeed = BaseClass::GetIdealSpeed();

				m_flBoostSpeed *= mult;
			}
		}
	}

	// Update our readiness if we're 
	if ( IsReadinessCapable() )
	{
		UpdateReadiness();
	}

	PredictPlayerPush();

	// Grovel through memories, don't forget enemies parented to func_tankmortar entities.
	// !!!LATER - this should really call out and ask if I want to forget the enemy in question.
	AIEnemiesIter_t	iter;
	for( AI_EnemyInfo_t *pMemory = GetEnemies()->GetFirst(&iter); pMemory != NULL; pMemory = GetEnemies()->GetNext(&iter) )
	{
		CEntity *cent = CEntity::Instance(pMemory->hEnemy);
		if ( IsMortar( cent ) || IsSniper( cent ) )
		{
			pMemory->bUnforgettable = ( IRelationType( pMemory->hEnemy ) < D_LI );
			pMemory->bEludedMe = false;
		}
	}

	if ( GetMotor()->IsDeceleratingToGoal() && IsCurTaskContinuousMove() && 
		 HasCondition( COND_PLAYER_PUSHING) && IsCurSchedule( SCHED_MOVE_AWAY ) )
	{
		ClearSchedule( "Being pushed by player" );
	}

	CEntity *pEnemy = GetEnemy();
	m_bWeightPathsInCover = false;
	if ( pEnemy )
	{
		if ( IsMortar( pEnemy ) || IsSniper( pEnemy ) )
		{
			m_bWeightPathsInCover = true;
		}
	}

	ClearCondition( COND_PC_SAFE_FROM_MORTAR );
	if ( IsCurSchedule( SCHED_TAKE_COVER_FROM_BEST_SOUND ) )
	{
		CSound *pSound = GetBestSound( SOUND_DANGER );

		if ( pSound && (pSound->SoundType() & SOUND_CONTEXT_MORTAR) )
		{
			float flDistSq = (pSound->GetSoundOrigin() - GetAbsOrigin() ).LengthSqr();
			if ( flDistSq > Square( MORTAR_BLAST_RADIUS + GetHullWidth() * 2 ) )
				SetCondition( COND_PC_SAFE_FROM_MORTAR );
		}
	}
	
	// Handle speech AI. Don't do AI speech if we're in scripts unless permitted by the EnableSpeakWhileScripting input.
	if ( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT || m_NPCState == NPC_STATE_COMBAT ||
		( ( m_NPCState == NPC_STATE_SCRIPT ) && CanSpeakWhileScripting() ) )
	{
		DoCustomSpeechAI();
	}

	if ( hl2_episodic->GetBool() && !GetEnemy() && HasCondition( COND_HEAR_PLAYER ) )
	{
		Vector los = ( pPlayer->EyePosition() - EyePosition() );
		los.z = 0;
		VectorNormalize( los );

		if ( DotProduct( los, EyeDirection2D() ) > DOT_45DEGREE )
		{
			ClearCondition( COND_HEAR_PLAYER );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::DoCustomSpeechAI( void )
{
	CPlayer *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
	
	// Don't allow this when we're getting in the car
#ifdef HL2_EPISODIC
	bool bPassengerInTransition = ( IsInAVehicle() && ( m_PassengerBehavior.GetPassengerState() == PASSENGER_STATE_ENTERING || m_PassengerBehavior.GetPassengerState() == PASSENGER_STATE_EXITING ) );
#else
	bool bPassengerInTransition = false;
#endif

	Vector vecEyePosition = EyePosition();
	if ( bPassengerInTransition == false && pPlayer && pPlayer->FInViewCone_Vector( vecEyePosition ) && pPlayer->FVisible_Vector( vecEyePosition ) )
	{
		if ( m_SpeechWatch_PlayerLooking.Expired() )
		{
			SpeakIfAllowed( TLK_LOOK );
			m_SpeechWatch_PlayerLooking.Stop();
		}
	}
	else
	{
		m_SpeechWatch_PlayerLooking.Start( 1.0f );
	}	

	// Mention the player is dead
	if ( HasCondition( COND_TALKER_PLAYER_DEAD ) )
	{
		SpeakIfAllowed( TLK_PLDEAD );
	}
}

//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::PredictPlayerPush()
{
	CPlayer *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
	if ( pPlayer && pPlayer->GetSmoothedVelocity().LengthSqr() >= Square(140))
	{
		Vector predictedPosition = pPlayer->WorldSpaceCenter() + pPlayer->GetSmoothedVelocity() * .4;
		Vector delta = WorldSpaceCenter() - predictedPosition;
		if ( delta.z < GetHullHeight() * .5 && delta.Length2DSqr() < Square(GetHullWidth() * 1.414)  )
			TestPlayerPushing( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();
	
	// Always interrupt to get into the car
	SetCustomInterruptCondition( COND_PC_BECOMING_PASSENGER );

	if ( IsCurSchedule(SCHED_RANGE_ATTACK1) )
	{
		SetCustomInterruptCondition( COND_PLAYER_PUSHING );
	}

	if ( ( ConditionInterruptsCurSchedule( COND_GIVE_WAY ) || 
		   IsCurSchedule(SCHED_HIDE_AND_RELOAD ) || 
		   IsCurSchedule(SCHED_RELOAD ) || 
		   IsCurSchedule(SCHED_STANDOFF ) || 
		   IsCurSchedule(SCHED_TAKE_COVER_FROM_ENEMY ) || 
		   IsCurSchedule(SCHED_COMBAT_FACE ) || 
		   IsCurSchedule(SCHED_ALERT_FACE )  ||
		   IsCurSchedule(SCHED_COMBAT_STAND ) || 
		   IsCurSchedule(SCHED_ALERT_FACE_BESTSOUND) ||
		   IsCurSchedule(SCHED_ALERT_STAND) ) )
	{
		SetCustomInterruptCondition( COND_HEAR_MOVE_AWAY );
		SetCustomInterruptCondition( COND_PLAYER_PUSHING );
		SetCustomInterruptCondition( COND_PC_HURTBYFIRE );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CSound *CNPC_PlayerCompanion::GetBestSound( int validTypes )
{
	AISoundIter_t iter;

	CSound *pCurrentSound = GetSenses()->GetFirstHeardSound( &iter );
	while ( pCurrentSound )
	{
		// the npc cares about this sound, and it's close enough to hear.
		if ( pCurrentSound->FIsSound() )
		{
			if( pCurrentSound->SoundContext() & SOUND_CONTEXT_MORTAR )
			{
				return pCurrentSound;
			}
		}

		pCurrentSound = GetSenses()->GetNextHeardSound( &iter );
	}

	return BaseClass::GetBestSound( validTypes );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::QueryHearSound( CSound *pSound )
{
	if( !BaseClass::QueryHearSound(pSound) )
		return false;

	switch( pSound->SoundTypeNoContext() )
	{
	case SOUND_READINESS_LOW:
		SetReadinessLevel( AIRL_RELAXED, false, true );
		return false;

	case SOUND_READINESS_MEDIUM:
		SetReadinessLevel( AIRL_STIMULATED, false, true );
		return false;

	case SOUND_READINESS_HIGH:
		SetReadinessLevel( AIRL_AGITATED, false, true );
		return false;

	default:
		return true;
	}
}

//-----------------------------------------------------------------------------

bool CNPC_PlayerCompanion::QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC )
{
	CAI_NPC *pOther = CEntity::Instance(pEntity)->MyNPCPointer(); 
	if ( pOther && 
		 ( pOther->GetState() == NPC_STATE_ALERT || GetState() == NPC_STATE_ALERT ||  pOther->GetState() == NPC_STATE_COMBAT || GetState() == NPC_STATE_COMBAT ) && 
		 pOther->IsPlayerAlly() )
	{
		return true;
	}

	return BaseClass::QuerySeeEntity( pEntity, bOnlyHateOrFearIfNPC );
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::ShouldIgnoreSound( CSound *pSound )
{
	if ( !BaseClass::ShouldIgnoreSound( pSound ) )
	{
		if ( pSound->IsSoundType( SOUND_DANGER ) && !SoundIsVisible(pSound) )
			return true;

#ifdef HL2_EPISODIC
		// Ignore vehicle sounds when we're driving in them
		if ( pSound->m_hOwner && pSound->m_hOwner->GetServerVehicle() != NULL )
		{
			if ( m_PassengerBehavior.GetPassengerState() == PASSENGER_STATE_INSIDE && 
				m_PassengerBehavior.GetTargetVehicle() == pSound->m_hOwner->GetServerVehicle()->GetVehicleEnt() )
				return true;
		}
#endif // HL2_EPISODIC
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_PlayerCompanion::SelectSchedule()
{
	m_bMovingAwayFromPlayer = false;

#ifdef HL2_EPISODIC
	// Always defer to passenger if it's running
	if ( ShouldDeferToPassengerBehavior() )
	{
		DeferSchedulingToBehavior( &m_PassengerBehavior );
		return BaseClass::SelectSchedule();
	}
#endif // HL2_EPISODIC

	if ( m_ActBusyBehavior.IsRunning() && m_ActBusyBehavior.NeedsToPlayExitAnim() )
	{
		trace_t tr;
		Vector	vUp = GetAbsOrigin();
		vUp.z += .25;

		UTIL_TraceHull( GetAbsOrigin(), vUp, GetHullMins(),
			GetHullMaxs(), MASK_SOLID, BaseEntity(), COLLISION_GROUP_NONE, &tr );

		if ( tr.startsolid )
		{
			if ( HasCondition( COND_HEAR_DANGER ) )
			{
				m_ActBusyBehavior.StopBusying();
			}
			DeferSchedulingToBehavior( &m_ActBusyBehavior );
			return BaseClass::SelectSchedule();
		}
	}

	int nSched = SelectFlinchSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	int schedule = SelectScheduleDanger();
	if ( schedule != SCHED_NONE )
		return schedule;
	
	schedule = SelectSchedulePriorityAction();
	if ( schedule != SCHED_NONE )
		return schedule;

	if ( ShouldDeferToFollowBehavior() )
	{
		DeferSchedulingToBehavior( &(GetFollowBehavior()) );
	}
	else if ( !BehaviorSelectSchedule() )
	{
		if ( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT )
		{
			schedule = SelectScheduleNonCombat();
			if ( schedule != SCHED_NONE )
				return schedule;
		}
		else if ( m_NPCState == NPC_STATE_COMBAT )
		{
			schedule = SelectScheduleCombat();
			if ( schedule != SCHED_NONE )
				return schedule;
		}
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_PlayerCompanion::SelectScheduleDanger()
{
	if( HasCondition( COND_HEAR_DANGER ) )
	{
		CSound *pSound;
		pSound = GetBestSound( SOUND_DANGER );

		Assert( pSound != NULL );

		if ( pSound && (pSound->m_iType & SOUND_DANGER) )
		{
			if ( !(pSound->SoundContext() & (SOUND_CONTEXT_MORTAR|SOUND_CONTEXT_FROM_SNIPER)) || IsOkToCombatSpeak() )
				SpeakIfAllowed( TLK_DANGER );

			if ( HasCondition( COND_PC_SAFE_FROM_MORTAR ) )
			{
				// Just duck and cover if far away from the explosion, or in cover.
				return SCHED_COWER;
			}
#if 1
			else if( pSound && (pSound->m_iType & SOUND_CONTEXT_FROM_SNIPER) )
			{
				return SCHED_COWER;
			}
#endif

			return SCHED_TAKE_COVER_FROM_BEST_SOUND;
		}
	}

	if ( GetEnemy() && 
		m_FakeOutMortarTimer.Expired() && 
		GetFollowBehavior().GetFollowTarget() && 
		IsMortar( GetEnemy() ) && 
		assert_cast<CAI_NPC *>(GetEnemy())->GetEnemy() == this && 
		assert_cast<CAI_NPC *>(GetEnemy())->FInViewCone_Entity( BaseEntity() ) &&
		assert_cast<CAI_NPC *>(GetEnemy())->FVisible_Entity( BaseEntity() ) )
	{
		m_FakeOutMortarTimer.Set( 7 );
		return SCHED_PC_FAKEOUT_MORTAR;
	}

	if ( HasCondition( COND_HEAR_MOVE_AWAY ) )
		return SCHED_MOVE_AWAY;

	if ( HasCondition( COND_PC_HURTBYFIRE ) )
	{
		ClearCondition( COND_PC_HURTBYFIRE );
		return SCHED_MOVE_AWAY;
	}
	
	return SCHED_NONE;	
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_PlayerCompanion::SelectSchedulePriorityAction()
{
	if ( GetGroundEntity() && !IsInAScript() )
	{
		if ( GetGroundEntity()->IsPlayer() )
		{
			return SCHED_PC_GET_OFF_COMPANION;
		}

		if ( GetGroundEntity()->IsNPC() && 
			IRelationType( GetGroundEntity()->BaseEntity() ) == D_LI && 
			 WorldSpaceCenter().z - GetGroundEntity()->WorldSpaceCenter().z > GetHullHeight() * .5 )
		{
			return SCHED_PC_GET_OFF_COMPANION;
		}
	}

	int schedule = SelectSchedulePlayerPush();
	if ( schedule != SCHED_NONE )
	{
		if ( GetFollowBehavior().IsRunning() )
			KeepRunningBehavior();
		return schedule;
	}

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_PlayerCompanion::SelectSchedulePlayerPush()
{
	if ( HasCondition( COND_PLAYER_PUSHING ) && !IsInAScript() && !IgnorePlayerPushing() )
	{
		// Ignore move away before gordon becomes the man
		if ( GlobalEntity_GetState("gordon_precriminal") != GLOBAL_ON )
		{
			m_bMovingAwayFromPlayer = true;
			return SCHED_MOVE_AWAY;
		}
	}

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IgnorePlayerPushing( void )
{
	if ( hl2_episodic->GetBool() )
	{
		// Ignore player pushes if we're leading him
		if ( m_LeadBehavior.IsRunning() && m_LeadBehavior.HasGoal() )
			return true;
		if ( m_AssaultBehavior.IsRunning() && m_AssaultBehavior.OnStrictAssault() )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_PlayerCompanion::SelectScheduleCombat()
{
	if ( CanReload() && (HasCondition ( COND_NO_PRIMARY_AMMO ) || HasCondition(COND_LOW_PRIMARY_AMMO)) )
	{
		return SCHED_HIDE_AND_RELOAD;
	}
	
	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::CanReload( void )
{
	if ( IsRunningDynamicInteraction() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::ShouldDeferToFollowBehavior()
{
	if ( !GetFollowBehavior().CanSelectSchedule() || !GetFollowBehavior().FarFromFollowTarget() )
		return false;
		
	if ( m_StandoffBehavior.CanSelectSchedule() && !m_StandoffBehavior.IsBehindBattleLines( GetFollowBehavior().GetFollowTarget()->GetAbsOrigin() ) )
		return false;

	if ( HasCondition(COND_BETTER_WEAPON_AVAILABLE) && !GetActiveWeapon() )
	{
		// Unarmed allies should arm themselves as soon as the opportunity presents itself.
		return false;
	}

	// Even though assault and act busy are placed ahead of the follow behavior in precedence, the below
	// code is necessary because we call ShouldDeferToFollowBehavior BEFORE we call the generic
	// BehaviorSelectSchedule, which tries the behaviors in priority order.
	if ( m_AssaultBehavior.CanSelectSchedule() && hl2_episodic->GetBool() )
	{
		return false;
	}

	if ( hl2_episodic->GetBool() )
	{
		if ( m_ActBusyBehavior.CanSelectSchedule() && m_ActBusyBehavior.IsCombatActBusy() )
		{
			return false;
		}
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// CalcReasonableFacing() is asking us if there's any reason why we wouldn't
// want to look in this direction. 
//
// Right now this is used to help prevent citizens aiming their guns at each other
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsValidReasonableFacing( const Vector &vecSightDir, float sightDist )
{
	if( !GetActiveWeapon() )
	{
		// If I'm not armed, it doesn't matter if I'm looking at another citizen.
		return true;
	}

	if( ai_new_aiming.GetBool() )
	{
		Vector vecEyePositionCentered = GetAbsOrigin();
		vecEyePositionCentered.z = EyePosition().z;

		if( IsSquadmateInSpread(vecEyePositionCentered, vecEyePositionCentered + vecSightDir * 240.0f, VECTOR_CONE_15DEGREES.x, 12.0f * 3.0f) )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_PlayerCompanion::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_IDLE_STAND:
	case SCHED_ALERT_STAND:
		if( GetActiveWeapon() )
		{
			// Everyone with less than half a clip takes turns reloading when not fighting.
			CCombatWeapon *pWeapon = GetActiveWeapon();

			if( CanReload() && pWeapon->UsesClipsForAmmo1() && pWeapon->Clip1() < ( pWeapon->GetMaxClip1() * .5 ) && OccupyStrategySlot( SQUAD_SLOT_EXCLUSIVE_RELOAD ) )
			{
				CPlayer *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
				if ( pPlayer )
				{
					pWeapon = pPlayer->GetActiveWeapon();
					if( pWeapon && pWeapon->UsesClipsForAmmo1() && 
						pWeapon->Clip1() < ( pWeapon->GetMaxClip1() * .75 ) &&
						pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() ) )
					{
						SpeakIfAllowed( TLK_PLRELOAD );
					}
				}
				return SCHED_RELOAD;
			}
		}
		break;

	case SCHED_COWER:
		return SCHED_PC_COWER;

	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			CSound *pSound = GetBestSound(SOUND_DANGER);

			if( pSound && pSound->m_hOwner )
			{
				CEntity *cent = CEntity::Instance(pSound->m_hOwner);
				if( cent->IsNPC() && FClassnameIs( cent, "npc_zombine" ) )
				{
					// Run fully away from a Zombine with a grenade.
					return SCHED_PC_TAKE_COVER_FROM_BEST_SOUND;
				}
			}

			return SCHED_PC_MOVE_TOWARDS_COVER_FROM_BEST_SOUND;
		}

	case SCHED_FLEE_FROM_BEST_SOUND:
		return SCHED_PC_FLEE_FROM_BEST_SOUND;

	case SCHED_ESTABLISH_LINE_OF_FIRE:
	case SCHED_MOVE_TO_WEAPON_RANGE:
		if ( IsMortar( GetEnemy() ) )
			return SCHED_TAKE_COVER_FROM_ENEMY;
		break;

	case SCHED_CHASE_ENEMY:
		if ( IsMortar( GetEnemy() ) )
			return SCHED_TAKE_COVER_FROM_ENEMY;
		if ( GetEnemy() && FClassnameIs( GetEnemy(), "npc_combinegunship" ) )
			return SCHED_ESTABLISH_LINE_OF_FIRE;
		break;

	case SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK:
		// If we're fighting a gunship, try again
		if ( GetEnemy() && FClassnameIs( GetEnemy(), "npc_combinegunship" ) )
			return SCHED_ESTABLISH_LINE_OF_FIRE;
		break;

	case SCHED_RANGE_ATTACK1:
		if ( IsMortar( GetEnemy() ) )
			return SCHED_TAKE_COVER_FROM_ENEMY;
			
		if ( GetShotRegulator()->IsInRestInterval() )
			return SCHED_STANDOFF;

		if( !OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return SCHED_STANDOFF;
		break;

	case SCHED_FAIL_TAKE_COVER:
		if ( IsEnemyTurret() )
		{
			return SCHED_PC_FAIL_TAKE_COVER_TURRET;
		}
		break;
	case SCHED_RUN_FROM_ENEMY_FALLBACK:
		{
			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			{
				return SCHED_RANGE_ATTACK1;
			}
			break;
		}
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SOUND_WAKE:
		LocateEnemySound();
		SetWait( 0.5 );
		break;

	case TASK_ANNOUNCE_ATTACK:
		{
			if ( GetActiveWeapon() && m_AnnounceAttackTimer.Expired() )
			{
				if ( SpeakIfAllowed( TLK_ATTACKING, UTIL_VarArgs("attacking_with_weapon:%s", GetActiveWeapon()->GetClassname()) ) )
				{
					m_AnnounceAttackTimer.Set( 10, 30 );
				}
			}

			BaseClass::StartTask( pTask );
			break;
		}

	case TASK_PC_WAITOUT_MORTAR:
		if ( HasCondition( COND_NO_HEAR_DANGER ) )
			TaskComplete();
		break;

	case TASK_MOVE_AWAY_PATH:
		{
			if ( m_bMovingAwayFromPlayer )
				SpeakIfAllowed( TLK_PLPUSH );

			BaseClass::StartTask( pTask );
		}
		break;

	case TASK_PC_GET_PATH_OFF_COMPANION:
		{
			Assert( ( GetGroundEntity() && ( GetGroundEntity()->IsPlayer() || ( GetGroundEntity()->IsNPC() && IRelationType( GetGroundEntity()->BaseEntity() ) == D_LI ) ) ) );
			GetNavigator()->SetAllowBigStep( GetGroundEntity() );
			ChainStartTask( TASK_MOVE_AWAY_PATH, 48 );
			
			/*
			trace_t tr;
			UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin(), GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.startsolid && tr.m_pEnt == GetGroundEntity() )
			{
				// Allow us to move through the entity for a short time
				NPCPhysics_CreateSolver( this, GetGroundEntity(), true, 2.0f );
			}
			*/
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
		case TASK_SOUND_WAKE:
			if( IsWaitFinished() )
			{
				TaskComplete();
			}
			break;

		case TASK_PC_WAITOUT_MORTAR:
			{
				if ( HasCondition( COND_NO_HEAR_DANGER ) )
					TaskComplete();
			}
			break;

		case TASK_MOVE_AWAY_PATH:
			{
				BaseClass::RunTask( pTask );

				if ( GetNavigator()->IsGoalActive() && !GetEnemy() )
				{
					AddFacingTarget_V_F_F_F( EyePosition() + BodyDirection2D() * 240, 1.0, 2.0 );
				}
			}
			break;

		case TASK_PC_GET_PATH_OFF_COMPANION:
			{
				//if ( AI_IsSinglePlayer() )
				{
					GetNavigator()->SetAllowBigStep(UTIL_GetNearestPlayer(GetAbsOrigin()) );
				}
				ChainRunTask( TASK_MOVE_AWAY_PATH, 48 );
			}
			break;

		default:
			BaseClass::RunTask( pTask );
			break;
	}
}

//-----------------------------------------------------------------------------
// Parses this NPC's activity remap from the actremap.txt file
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::PrepareReadinessRemap( void )
{
	CUtlVector< CActivityRemap > entries;
	UTIL_LoadActivityRemapFile( "scripts/sm_monster/actremap.txt", "npc_playercompanion", entries );

	for ( int i = 0; i < entries.Count(); i++ )
	{
		CCompanionActivityRemap ActRemap;
		Q_memcpy( &ActRemap, &entries[i], sizeof( CActivityRemap ) );

		KeyValues *pExtraBlock = ActRemap.GetExtraKeyValueBlock();

		if ( pExtraBlock )
		{
			KeyValues *pKey = pExtraBlock->GetFirstSubKey();

			while ( pKey )
			{
				const char *pKeyName = pKey->GetName();
				const char *pKeyValue = pKey->GetString();

				if ( !stricmp( pKeyName, "readiness" ) )
				{
					ActRemap.m_fUsageBits |= bits_REMAP_READINESS;

					if ( !stricmp( pKeyValue, "AIRL_PANIC" ) )
					{
						ActRemap.m_readiness = AIRL_PANIC;
					}
					else if ( !stricmp( pKeyValue, "AIRL_STEALTH" ) )
					{
						ActRemap.m_readiness = AIRL_STEALTH;
					}
					else if ( !stricmp( pKeyValue, "AIRL_RELAXED" ) )
					{
						ActRemap.m_readiness = AIRL_RELAXED;
					}
					else if ( !stricmp( pKeyValue, "AIRL_STIMULATED" ) )
					{
						ActRemap.m_readiness = AIRL_STIMULATED;
					}
					else if ( !stricmp( pKeyValue, "AIRL_AGITATED" ) )
					{
						ActRemap.m_readiness = AIRL_AGITATED;
					}
				}
				else if ( !stricmp( pKeyName, "aiming" ) )
				{
					ActRemap.m_fUsageBits |= bits_REMAP_AIMING;

					if ( !stricmp( pKeyValue, "TRS_NONE" ) )
					{
						// This is the new way to say that we don't care, the tri-state was abandoned (jdw)
						ActRemap.m_fUsageBits &= ~bits_REMAP_AIMING;
					}
					else if ( !stricmp( pKeyValue, "TRS_FALSE" ) || !stricmp( pKeyValue, "FALSE" ) )
					{
						ActRemap.m_bAiming = false;
					}
					else if ( !stricmp( pKeyValue, "TRS_TRUE" ) || !stricmp( pKeyValue, "TRUE" ) )
					{
						ActRemap.m_bAiming = true;
					}
				} 
				else if ( !stricmp( pKeyName, "weaponrequired" ) )
				{
					ActRemap.m_fUsageBits |= bits_REMAP_WEAPON_REQUIRED;

					if ( !stricmp( pKeyValue, "TRUE" ) )
					{
						ActRemap.m_bWeaponRequired = true;
					}
					else if ( !stricmp( pKeyValue, "FALSE" ) )
					{
						ActRemap.m_bWeaponRequired = false;
					}
				}
				else if ( !stricmp( pKeyName, "invehicle" ) )
				{
					ActRemap.m_fUsageBits |= bits_REMAP_IN_VEHICLE;

					if ( !stricmp( pKeyValue, "TRUE" ) )
					{
						ActRemap.m_bInVehicle = true;
					}
					else if ( !stricmp( pKeyValue, "FALSE" ) )
					{
						ActRemap.m_bInVehicle = false;
					}
				}

				pKey = pKey->GetNextKey();
			}
		}

		const char *pActName = ActivityList_NameForIndex( (int)ActRemap.mappedActivity );

		if ( GetActivityID( pActName ) == ACT_INVALID )
		{
			AddActivityToSR( pActName, (int)ActRemap.mappedActivity );
		}

		m_activityMappings.AddToTail( ActRemap );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::Activate( void )
{
	BaseClass::Activate();

	PrepareReadinessRemap();
}

//-----------------------------------------------------------------------------
// Purpose: Translate an activity given a list of criteria
//-----------------------------------------------------------------------------
Activity CNPC_PlayerCompanion::TranslateActivityReadiness( Activity activity )
{
	// If we're in an actbusy, we don't want to mess with this
	if ( m_ActBusyBehavior.IsActive() )
		return activity;

	if ( m_bReadinessCapable && 
		 ( GetReadinessUse() == AIRU_ALWAYS || 
		   ( GetReadinessUse() == AIRU_ONLY_PLAYER_SQUADMATES && (IsInPlayerSquad()||Classify()==CLASS_PLAYER_ALLY_VITAL) ) ) )
	{
		bool bShouldAim = ShouldBeAiming();

		for ( int i = 0; i < m_activityMappings.Count(); i++ )
		{
			// Get our activity remap
			CCompanionActivityRemap actremap = m_activityMappings[i];

			// Activity must match
			if ( activity != actremap.activity )
				continue;

			// Readiness must match
			if ( ( actremap.m_fUsageBits & bits_REMAP_READINESS ) && GetReadinessLevel() != actremap.m_readiness )
				continue;

			// Deal with weapon state
			if ( ( actremap.m_fUsageBits & bits_REMAP_WEAPON_REQUIRED ) && actremap.m_bWeaponRequired )
			{
				// Must have a weapon
				if ( GetActiveWeapon() == NULL )
					continue;
				
				// Must either not care about aiming, or agree on aiming
				if ( actremap.m_fUsageBits & bits_REMAP_AIMING )
				{
					if ( bShouldAim && actremap.m_bAiming == false )
						continue;

					if ( bShouldAim == false && actremap.m_bAiming )
						continue;
				}
			}

			// Must care about vehicle status
			if ( actremap.m_fUsageBits & bits_REMAP_IN_VEHICLE )
			{
				// Deal with the two vehicle states
				if ( actremap.m_bInVehicle && IsInAVehicle() == false )
					continue;

				if ( actremap.m_bInVehicle == false && IsInAVehicle() )
					continue;
			}

			// We've successfully passed all criteria for remapping this 
			return actremap.mappedActivity;
		}
	}

	return activity;
}


//-----------------------------------------------------------------------------
// Purpose: Override base class activiites
//-----------------------------------------------------------------------------
Activity CNPC_PlayerCompanion::NPC_TranslateActivity( Activity activity )
{
	if ( activity == ACT_COWER )
		return ACT_COVER_LOW;

	if ( activity == ACT_RUN && ( IsCurSchedule( SCHED_TAKE_COVER_FROM_BEST_SOUND ) || IsCurSchedule( SCHED_FLEE_FROM_BEST_SOUND ) ) )
	{
		if ( enginerandom->RandomInt( 0, 1 ) && HaveSequenceForActivity( ACT_RUN_PROTECTED ) )
			activity = ACT_RUN_PROTECTED;
	}

	activity = BaseClass::NPC_TranslateActivity( activity );

	if ( activity == ACT_IDLE  )
	{
		if ( (m_NPCState == NPC_STATE_COMBAT || m_NPCState == NPC_STATE_ALERT) && gpGlobals->curtime - m_flLastAttackTime < 3)
		{
			activity = ACT_IDLE_ANGRY;
		}
	}

	return TranslateActivityReadiness( activity );
}

//------------------------------------------------------------------------------
// Purpose: Handle animation events
//------------------------------------------------------------------------------
void CNPC_PlayerCompanion::HandleAnimEvent( animevent_t *pEvent )
{
#ifdef HL2_EPISODIC
	// Create a flare and parent to our hand
	if ( pEvent->event == AE_COMPANION_PRODUCE_FLARE )
	{
		m_hFlare = static_cast<CPhysicsProp *>(CreateEntityByName( "prop_physics" ));
		if ( m_hFlare != NULL )
		{
			// Set the model
			m_hFlare->SetModel( "models/props_junk/flare.mdl" );
			
			// Set the parent attachment
			m_hFlare->SetParent( this );
			m_hFlare->SetParentAttachment( "SetParentAttachment", pEvent->options, false );
		}

		return;
	}

	// Start the flare up with proper fanfare
	if ( pEvent->event == AE_COMPANION_LIGHT_FLARE )
	{
		if ( m_hFlare != NULL )
		{
			m_hFlare->CreateFlare( 5*60.0f );
		}
		
		return;
	}

	// Drop the flare to the ground
	if ( pEvent->event == AE_COMPANION_RELEASE_FLARE )
	{
		// Detach
		m_hFlare->SetParent( NULL );
		m_hFlare->Spawn();
		m_hFlare->RemoveInteraction( PROPINTER_PHYSGUN_CREATE_FLARE );

		// Disable collisions between the NPC and the flare
		PhysDisableEntityCollisions( this, m_hFlare );

		// TODO: Find the velocity of the attachment point, at this time, in the animation cycle

		// Construct a toss velocity
		Vector vecToss;
		AngleVectors( GetAbsAngles(), &vecToss );
		VectorNormalize( vecToss );
		vecToss *= enginerandom->RandomFloat( 64.0f, 72.0f );
		vecToss[2] += 64.0f;

		// Throw it
		IPhysicsObject *pObj = m_hFlare->VPhysicsGetObject();
		pObj->ApplyForceCenter( vecToss );

		// Forget about the flare at this point
		m_hFlare = NULL;

		return;
	}
#endif // HL2_EPISODIC

	switch( pEvent->event )
	{
	case EVENT_WEAPON_RELOAD:
		if ( GetActiveWeapon() )
		{
			GetActiveWeapon()->WeaponSound( RELOAD_NPC );
			GetActiveWeapon()->m_iClip1 = GetActiveWeapon()->GetMaxClip1(); 
			ClearCondition(COND_LOW_PRIMARY_AMMO);
			ClearCondition(COND_NO_PRIMARY_AMMO);
			ClearCondition(COND_NO_SECONDARY_AMMO);
		}
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::HandleInteraction(int interactionType, void *data, CBaseEntity* sourceEnt)
{
	if (interactionType == *(g_interactionHitByPlayerThrownPhysObj) )
	{
		if ( IsOkToSpeakInResponseToPlayer() )
		{
			Speak( TLK_PLYR_PHYSATK );
		}
		return true;
	}

	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int CNPC_PlayerCompanion::GetSoundInterests()
{
	return	SOUND_WORLD				|
			SOUND_COMBAT			|
			SOUND_PLAYER			|
			SOUND_DANGER			|
			SOUND_BULLET_IMPACT		|
			SOUND_MOVE_AWAY			|
			SOUND_READINESS_LOW		|
			SOUND_READINESS_MEDIUM	|
			SOUND_READINESS_HIGH;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_PlayerCompanion::Touch( CEntity *pOther )
{
	BaseClass::Touch( pOther );

	// Did the player touch me?
	if ( pOther->IsPlayer() || ( pOther->VPhysicsGetObject() && (pOther->VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD ) ) )
	{
		// Ignore if pissed at player
		if ( m_afMemory & bits_MEMORY_PROVOKED )
			return;
			
		TestPlayerPushing( ( pOther->IsPlayer() ) ? pOther : UTIL_GetNearestPlayer(GetAbsOrigin()) );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::ModifyOrAppendCriteria( AI_CriteriaSet& set )
{
	BaseClass::ModifyOrAppendCriteria( set );
	if ( GetEnemy() && IsMortar( GetEnemy() ) )
	{
		set.RemoveCriteria( "enemy" );
		set.AppendCriteria( "enemy", STRING(gm_iszMortarClassname) );
	}

	if ( HasCondition( COND_PC_HURTBYFIRE ) )
	{
		set.AppendCriteria( "hurt_by_fire", "1" );
	}

	if ( m_bReadinessCapable )
	{
		switch( GetReadinessLevel() )
		{
		case AIRL_PANIC:
			set.AppendCriteria( "readiness", "panic" );
			break;

		case AIRL_STEALTH:
			set.AppendCriteria( "readiness", "stealth" );
			break;

		case AIRL_RELAXED:
			set.AppendCriteria( "readiness", "relaxed" );
			break;

		case AIRL_STIMULATED:
			set.AppendCriteria( "readiness", "stimulated" );
			break;

		case AIRL_AGITATED:
			set.AppendCriteria( "readiness", "agitated" );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsReadinessCapable()
{
	if ( GlobalEntity_GetState("gordon_precriminal") == GLOBAL_ON )
		return false;

#ifndef HL2_EPISODIC
	// Allow episodic companions to use readiness even if unarmed. This allows for the panicked 
	// citizens in ep1_c17_05 (sjb)
	if( !GetActiveWeapon() )
		return false;
#endif

	if( GetActiveWeapon() && LookupActivity("ACT_IDLE_AIM_RIFLE_STIMULATED") == ACT_INVALID )
		return false;

	if( GetActiveWeapon() && ToCWeaponRPG(GetActiveWeapon()) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::AddReadiness( float flAdd, bool bOverrideLock )
{
	if( IsReadinessLocked() && !bOverrideLock )
		return;

	SetReadinessValue( GetReadinessValue() + flAdd );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::SubtractReadiness( float flSub, bool bOverrideLock )
{
 	if( IsReadinessLocked() && !bOverrideLock )
		return;

	// Prevent readiness from going below 0 (below 0 is only for scripted states)
	SetReadinessValue( MAX(GetReadinessValue() - flSub, 0) );
}

//-----------------------------------------------------------------------------
// This method returns false if the NPC is not allowed to change readiness at this point.
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::AllowReadinessValueChange( void )
{
	if ( GetIdealActivity() == ACT_IDLE || GetIdealActivity() == ACT_WALK || GetIdealActivity() == ACT_RUN )
		return true;

	if ( HasActiveLayer() == true )
		return false;

	return false;
}

//-----------------------------------------------------------------------------
// NOTE: This function ignores the lock. Use the interface functions.
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::SetReadinessValue( float flSet )
{
	if ( AllowReadinessValueChange() == false )
		return;

	int priorReadiness = GetReadinessLevel();

	flSet = MIN( 1.0f, flSet );
	flSet = MAX( READINESS_MIN_VALUE, flSet );

	m_flReadiness = flSet;

	if( GetReadinessLevel() != priorReadiness )
	{
		// We've been bumped up into a different readiness level.
		// Interrupt IDLE schedules (if we're playing one) so that 
		// we can pick the proper animation.
		SetCondition( COND_IDLE_INTERRUPT );

		// Force us to recalculate our animation. If we don't do this,
		// our translated activity may change, but not our root activity,
		// and then we won't actually visually change anims.
		ResetActivity();

		//Force the NPC to recalculate it's arrival sequence since it'll most likely be wrong now that we changed readiness level.
		GetNavigator()->SetArrivalSequence( ACT_INVALID );

		ReadinessLevelChanged( priorReadiness );
	}
}

//-----------------------------------------------------------------------------
// if bOverrideLock, you'll change the readiness level even if we're within
// a time period during which someone else has locked the level.
//
// if bSlam, you'll allow the readiness level to be set lower than current. 
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::SetReadinessLevel( int iLevel, bool bOverrideLock, bool bSlam )
{
	if( IsReadinessLocked() && !bOverrideLock )
		return;

	switch( iLevel )
	{
	case AIRL_PANIC:
		if( bSlam )
			SetReadinessValue( READINESS_MODE_PANIC );
		break;
	case AIRL_STEALTH:
		if( bSlam )
			SetReadinessValue( READINESS_MODE_STEALTH );
		break;
	case AIRL_RELAXED:
		if( bSlam || GetReadinessValue() < READINESS_VALUE_RELAXED )
			SetReadinessValue( READINESS_VALUE_RELAXED );
		break;
	case AIRL_STIMULATED:
		if( bSlam || GetReadinessValue() < READINESS_VALUE_STIMULATED )
			SetReadinessValue( READINESS_VALUE_STIMULATED );
		break;
	case AIRL_AGITATED:
		if( bSlam || GetReadinessValue() < READINESS_VALUE_AGITATED )
			SetReadinessValue( READINESS_VALUE_AGITATED );
		break;
	default:
		DevMsg("ERROR: Bad readiness level\n");
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int	CNPC_PlayerCompanion::GetReadinessLevel()
{
	if ( m_bReadinessCapable == false )
		return AIRL_RELAXED;

	if( m_flReadiness == READINESS_MODE_PANIC )
	{
		return AIRL_PANIC;
	}

	if( m_flReadiness == READINESS_MODE_STEALTH )
	{
		return AIRL_STEALTH;
	}

	if( m_flReadiness <= READINESS_VALUE_RELAXED )
	{
		return AIRL_RELAXED;
	}

	if( m_flReadiness <= READINESS_VALUE_STIMULATED )
	{
		return AIRL_STIMULATED;
	}

	return AIRL_AGITATED;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::UpdateReadiness()
{
	// Only update readiness if it's not in a scripted state
	if ( !IsInScriptedReadinessState() )
	{
		if( HasCondition(COND_HEAR_COMBAT) || HasCondition(COND_HEAR_BULLET_IMPACT)	)
			SetReadinessLevel( AIRL_STIMULATED, false, false );

		if( HasCondition(COND_HEAR_DANGER) || HasCondition(COND_SEE_ENEMY) )
			SetReadinessLevel( AIRL_AGITATED, false, false );

		if( m_flReadiness > 0.0f && GetReadinessDecay() > 0 )
		{
			// Decay.
			SubtractReadiness( ( 0.1 * (1.0f/GetReadinessDecay())) * m_flReadinessSensitivity );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPC_PlayerCompanion::GetReadinessDecay()
{
	return ai_readiness_decay.GetFloat();
}

//-----------------------------------------------------------------------------
// Passing NULL to clear the aim target is acceptible.
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::SetAimTarget( CEntity *pTarget )
{
	if( pTarget != NULL && IsAllowedToAim() )
	{
		m_hAimTarget.Set(pTarget->BaseEntity());
	}
	else
	{
		m_hAimTarget.Set(NULL);
	}

	Activity NewActivity = NPC_TranslateActivity(GetActivity());

	//Don't set the ideal activity to an activity that might not be there.
	if ( SelectWeightedSequence( NewActivity ) == ACT_INVALID )
		 return;

	if (NewActivity != GetActivity() )
	{
		SetIdealActivity( NewActivity );
	}

#if 0
	if( m_hAimTarget )
	{
		Msg("New Aim Target: %s\n", m_hAimTarget->GetClassname() );
		NDebugOverlay::Line(EyePosition(), m_hAimTarget->WorldSpaceCenter(), 255, 255, 0, false, 0.1 );
	}
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::StopAiming( const char *pszReason )
{
#if 0
	if( pszReason )
	{	
		Msg("Stopped aiming because %s\n", pszReason );
	}
#endif

	SetAimTarget(NULL);

	Activity NewActivity = NPC_TranslateActivity(GetActivity());
	if (NewActivity != GetActivity())
	{
		SetIdealActivity( NewActivity );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define COMPANION_MAX_LOOK_TIME	3.0f
#define COMPANION_MIN_LOOK_TIME	1.0f
#define COMPANION_MAX_TACTICAL_TARGET_DIST	1800.0f // 150 feet

bool CNPC_PlayerCompanion::PickTacticalLookTarget( AILookTargetArgs_t *pArgs )
{
	if( HasCondition( COND_SEE_ENEMY ) )
	{
		// Don't bother. We're dealing with our enemy.
		return false;
	}

	float flMinLookTime;
	float flMaxLookTime;

	// Excited companions will look at each target only briefly and then find something else to look at.
	flMinLookTime = COMPANION_MIN_LOOK_TIME + ((COMPANION_MAX_LOOK_TIME-COMPANION_MIN_LOOK_TIME) * (1.0f - GetReadinessValue()) );

	switch( GetReadinessLevel() )
	{
	case AIRL_RELAXED:
		// Linger on targets, look at them for quite a while.
		flMinLookTime = COMPANION_MAX_LOOK_TIME + enginerandom->RandomFloat( 0.0f, 2.0f );
		break;

	case AIRL_STIMULATED:
		// Look around a little quicker.
		flMinLookTime = COMPANION_MIN_LOOK_TIME + enginerandom->RandomFloat( 0.0f, COMPANION_MAX_LOOK_TIME - 1.0f );
		break;

	case AIRL_AGITATED:
		// Look around very quickly
		flMinLookTime = COMPANION_MIN_LOOK_TIME;
		break;
	}

	flMaxLookTime = flMinLookTime + enginerandom->RandomFloat( 0.0f, 0.5f );
	pArgs->flDuration = enginerandom->RandomFloat( flMinLookTime, flMaxLookTime );

	if( HasCondition(COND_SEE_PLAYER) && hl2_episodic->GetBool() )
	{
		// 1/3rd chance to authoritatively look at player
		if( enginerandom->RandomInt( 0, 2 ) == 0 )
		{
			CPlayer *player = UTIL_GetNearestVisiblePlayer(this);
			pArgs->hTarget.Set((player)?player->BaseEntity():NULL);

			return true;
		}
	}

	// Use hint nodes
	CE_AI_Hint *pHint;
	CHintCriteria hintCriteria;

	hintCriteria.AddHintType( HINT_WORLD_VISUALLY_INTERESTING );
	hintCriteria.AddHintType( HINT_WORLD_VISUALLY_INTERESTING_DONT_AIM );
	hintCriteria.AddHintType( HINT_WORLD_VISUALLY_INTERESTING_STEALTH );
	hintCriteria.SetFlag( bits_HINT_NODE_VISIBLE | bits_HINT_NODE_IN_VIEWCONE | bits_HINT_NPC_IN_NODE_FOV );
	hintCriteria.AddIncludePosition( GetAbsOrigin(), COMPANION_MAX_TACTICAL_TARGET_DIST );

	{
  		pHint = CAI_HintManager::FindHint( this, hintCriteria );
	}
	
	if( pHint )
	{
		pArgs->hTarget.Set(pHint->BaseEntity());
		
		// Turn this node off for a few seconds to stop others aiming at the same thing (except for stealth nodes)
		if ( pHint->HintType() != HINT_WORLD_VISUALLY_INTERESTING_STEALTH )
		{
			pHint->DisableForSeconds( 5.0f );
		}
		return true;
	}

	// See what the base class thinks.
	return BaseClass::PickTacticalLookTarget( pArgs );
}

//-----------------------------------------------------------------------------
// Returns true if changing target.
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::FindNewAimTarget()
{
	if( GetEnemy() )
	{
		// Don't bother. Aim at enemy.
		return false;
	}

	if( !m_bReadinessCapable || GetReadinessLevel() == AIRL_RELAXED )
	{
		// If I'm relaxed (don't want to aim), or physically incapable,
		// don't run this hint node searching code.
		return false;
	}

	CE_AI_Hint *pHint;
	CHintCriteria hintCriteria;
	CEntity *pPriorAimTarget = GetAimTarget();

	hintCriteria.SetHintType( HINT_WORLD_VISUALLY_INTERESTING );
	hintCriteria.SetFlag( bits_HINT_NODE_VISIBLE | bits_HINT_NODE_IN_VIEWCONE | bits_HINT_NPC_IN_NODE_FOV );
	hintCriteria.AddIncludePosition( GetAbsOrigin(), COMPANION_MAX_TACTICAL_TARGET_DIST );
	pHint = CAI_HintManager::FindHint( this, hintCriteria );

	if( pHint )
	{
		if( (pHint->GetAbsOrigin() - GetAbsOrigin()).Length2D() < COMPANION_AIMTARGET_NEAREST )
		{
			// Too close!
			return false;
		}

		if( !HasAimLOS(pHint) )
		{
			// No LOS
			return false;
		}

		if( pHint != pPriorAimTarget )
		{
			// Notify of the change.
			SetAimTarget( pHint );
			return true;
		}
	}

	// Didn't find an aim target, or found the same one.
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::OnNewLookTarget()
{
	if( ai_new_aiming.GetBool() )
	{
		CEntity *cent = CEntity::Instance(GetLooktarget());
		if( cent )
		{
			// See if our looktarget is a reasonable aim target.
			CE_AI_Hint *pHint = dynamic_cast<CE_AI_Hint*>( cent );

			if( pHint )
			{
				if( pHint->HintType() == HINT_WORLD_VISUALLY_INTERESTING &&
					(pHint->GetAbsOrigin() - GetAbsOrigin()).Length2D() > COMPANION_AIMTARGET_NEAREST  &&
					FInAimCone_Vector(pHint->GetAbsOrigin())	&&
					HasAimLOS(pHint) )
				{
					SetAimTarget( pHint );
					return;
				}
			}
		}

		// Search for something else.
		FindNewAimTarget();
	}
	else
	{
		if( GetLooktarget() )
		{
			// Have picked a new entity to look at. Should we copy it to the aim target?
			if( IRelationType( GetLooktarget() ) == D_LI )
			{
				// Don't aim at friends, just keep the old target (if any)
				return;
			}
			CEntity *cent = CEntity::Instance(GetLooktarget());

			if( (cent->GetAbsOrigin() - GetAbsOrigin()).Length2D() < COMPANION_AIMTARGET_NEAREST )
			{
				// Too close!
				return;
			}

			if( !HasAimLOS( cent ) )
			{
				// No LOS
				return;
			}

			SetAimTarget( cent );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::ShouldBeAiming() 
{
	if( !IsAllowedToAim() )
	{
		return false;
	}

	if( !GetEnemy() && !GetAimTarget() )
	{
		return false;
	}

	if( GetEnemy() && !HasCondition(COND_SEE_ENEMY) )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define PC_MAX_ALLOWED_AIM	2
bool CNPC_PlayerCompanion::IsAllowedToAim()
{
	CAI_Squad *squad = GetSquad();
	if( !squad )
		return true;

	if( GetReadinessLevel() == AIRL_AGITATED )
	{
		// Agitated companions can always aim. This makes the squad look
		// more alert as a whole when something very serious/dangerous has happened.
		return true;
	}

	int count = 0;
	
	// If I'm in a squad, only a certain number of us can aim.
	AISquadIter_t iter;
	for ( CBaseEntity *pSquadmate = squad->GetFirstMember(&iter); pSquadmate; pSquadmate = squad->GetNextMember(&iter) )
	{
		CAI_NPC *npc = (CAI_NPC *)CEntity::Instance(pSquadmate);
		CNPC_PlayerCompanion *pCompanion = dynamic_cast<CNPC_PlayerCompanion*>(npc);
		if( pCompanion && pCompanion != this && pCompanion->GetAimTarget() != NULL )
		{
			count++;
		}
	}

	if( count < PC_MAX_ALLOWED_AIM )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::HasAimLOS( CEntity *pAimTarget )
{
	trace_t tr;
	UTIL_TraceLine( Weapon_ShootPosition(), pAimTarget->WorldSpaceCenter(), MASK_SHOT, BaseEntity(), COLLISION_GROUP_NONE, &tr );

	CEntity *cent = CEntity::Instance(tr.m_pEnt);
	if( tr.fraction < 0.5 || (cent && (cent->IsNPC()||cent->IsPlayer())) )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::AimGun()
{
	Vector vecAimDir;

	if( !GetEnemy() )
	{
		CEntity *cent = GetAimTarget();
		if( cent && FInViewCone_Entity(cent->BaseEntity()) )
		{
			float flDist; 
			Vector vecAimTargetLoc = cent->WorldSpaceCenter();

			flDist = (vecAimTargetLoc - GetAbsOrigin()).Length2DSqr();

			// Throw away a looktarget if it gets too close. We don't want guys turning around as
			// they walk through doorways which contain a looktarget.
			if( flDist < COMPANION_AIMTARGET_NEAREST_SQR )
			{
				StopAiming("Target too near");
				return;
			}

			// Aim at my target if it's in my cone
			vecAimDir = vecAimTargetLoc - Weapon_ShootPosition();;
			VectorNormalize( vecAimDir );
			SetAim( vecAimDir);

			if( !HasAimLOS(GetAimTarget()) )
			{
				// LOS is broken.
				if( !FindNewAimTarget() )
				{	
					// No alternative available right now. Stop aiming.
					StopAiming("No LOS");
				}
			}

			return;
		}
		else
		{
			if( GetAimTarget() )
			{
				// We're aiming at something, but we're about to stop because it's out of viewcone.
				// Try to find something else.
				if( FindNewAimTarget() )
				{
					// Found something else to aim at.
					return;
				}
				else
				{
					// ditch the aim target, it's gone out of view.
					StopAiming("Went out of view cone");
				}
			}

			if( GetReadinessLevel() == AIRL_AGITATED )
			{
				// Aim down! Agitated animations don't have non-aiming versions, so 
				// just point the weapon down.
				Vector vecSpot = EyePosition();
				Vector forward, up;
				GetVectors( &forward, NULL, &up );
				vecSpot += forward * 128 + up * -64;

				vecAimDir = vecSpot - Weapon_ShootPosition();
				VectorNormalize( vecAimDir );
				SetAim( vecAimDir);
				return;
			}
		}
	}

	BaseClass::AimGun();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_PlayerCompanion::GetAlternateMoveShootTarget()
{
	CEntity *cent = GetAimTarget();
	if( cent && !cent->IsNPC() && GetReadinessLevel() != AIRL_RELAXED )
	{
		return cent->BaseEntity();
	}

	return BaseClass::GetAlternateMoveShootTarget();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsValidEnemy( CBaseEntity *pEnemy )
{
	CEntity *cent = CEntity::Instance(pEnemy);
	if ( GetFollowBehavior().GetFollowTarget() && GetFollowBehavior().GetFollowTarget()->IsPlayer() && IsSniper( cent ) )
	{
		AI_EnemyInfo_t *pInfo = GetEnemies()->Find( pEnemy );
		if ( pInfo )
		{
			if ( gpGlobals->curtime - pInfo->timeLastSeen > 10 )
			{
				if ( !((CAI_NPC*)cent)->HasCondition( COND_IN_PVS ) )
					return false;
			}
		}
	}

	return BaseClass::IsValidEnemy( pEnemy );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsSafeFromFloorTurret( const Vector &vecLocation, CEntity *pTurret )
{
	float dist = ( vecLocation - pTurret->EyePosition() ).LengthSqr();

	if ( dist > Square( 4.0*12.0 ) )
	{
		if ( !pTurret->MyNPCPointer()->FInViewCone_Vector( vecLocation ) )
		{
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool CNPC_PlayerCompanion::ShouldMoveAndShoot( void )
{
	return BaseClass::ShouldMoveAndShoot();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define PC_LARGER_BURST_RANGE	(12.0f * 10.0f) // If an enemy is this close, player companions fire larger continuous bursts.
void CNPC_PlayerCompanion::OnUpdateShotRegulator()
{
	BaseClass::OnUpdateShotRegulator();

	if( GetEnemy() && HasCondition(COND_CAN_RANGE_ATTACK1) )
	{
		if( GetAbsOrigin().DistTo( GetEnemy()->GetAbsOrigin() ) <= PC_LARGER_BURST_RANGE )
		{
			if( hl2_episodic->GetBool() )
			{
				// Longer burst
				int longBurst = enginerandom->RandomInt( 10, 15 );
				GetShotRegulator()->SetBurstShotsRemaining( longBurst );
				GetShotRegulator()->SetRestInterval( 0.1, 0.2 );
			}
			else
			{
				// Longer burst
				GetShotRegulator()->SetBurstShotsRemaining( GetShotRegulator()->GetBurstShotsRemaining() * 2 );

				// Shorter Rest interval
				float flMinInterval, flMaxInterval;
				GetShotRegulator()->GetRestInterval( &flMinInterval, &flMaxInterval );
				GetShotRegulator()->SetRestInterval( flMinInterval * 0.6f, flMaxInterval * 0.6f );
			}
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_PlayerCompanion::DecalTrace( trace_t *pTrace, char const *decalName )
{
	// Do not decal a player companion's head or face, no matter what.
	if( pTrace->hitgroup == HITGROUP_HEAD )
		return;

	BaseClass::DecalTrace( pTrace, decalName );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool CNPC_PlayerCompanion::FCanCheckAttacks()
{
	if( GetEnemy() && ( IsSniper(GetEnemy()) || IsMortar(GetEnemy()) || IsTurret(GetEnemy()) ) )
	{
		// Don't attack the sniper or the mortar.
		return false;
	}

	return BaseClass::FCanCheckAttacks();
}

//-----------------------------------------------------------------------------
// Purpose: Return the actual position the NPC wants to fire at when it's trying
//			to hit it's current enemy.
//-----------------------------------------------------------------------------
#define CITIZEN_HEADSHOT_FREQUENCY	3 // one in this many shots at a zombie will be aimed at the zombie's head
Vector CNPC_PlayerCompanion::GetActualShootPosition( const Vector &shootOrigin )
{
	if( GetEnemy() && GetEnemy()->Classify() == CLASS_ZOMBIE && enginerandom->RandomInt( 1, CITIZEN_HEADSHOT_FREQUENCY ) == 1 )
	{
		return GetEnemy()->HeadTarget( shootOrigin );
	}

	return BaseClass::GetActualShootPosition( shootOrigin );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
WeaponProficiency_t CNPC_PlayerCompanion::CalcWeaponProficiency( CBaseEntity *pWeapon )
{
	CCombatWeapon *weapon = (CCombatWeapon *)CEntity::Instance(pWeapon);
	if( FClassnameIs( weapon, WEAPON_AR2_REPLACE_NAME ) )
	{
		return WEAPON_PROFICIENCY_VERY_GOOD;
	}

	return WEAPON_PROFICIENCY_PERFECT;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::Weapon_CanUse( CBaseEntity *pWeapon )
{
	CCombatWeapon *weapon = (CCombatWeapon *)CEntity::Instance(pWeapon);
	if( BaseClass::Weapon_CanUse( pWeapon ) )
	{
		// If this weapon is a shotgun, take measures to control how many
		// are being used in this squad. Don't allow a companion to pick up
		// a shotgun if a squadmate already has one.
		if( weapon->ClassMatches( gm_iszShotgunClassname ) )
		{
			return (NumWeaponsInSquad(WEAPON_SHOTGUN_REPLACE_NAME) < 1 );
		}
		else
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::ShouldLookForBetterWeapon()
{
	if ( m_bDontPickupWeapons )
		return false;

	return BaseClass::ShouldLookForBetterWeapon();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::Weapon_Equip( CBaseEntity *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );
	m_bReadinessCapable = IsReadinessCapable();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_PlayerCompanion::PickupWeapon( CBaseEntity *pWeapon )
{
	BaseClass::PickupWeapon( pWeapon );
	SpeakIfAllowed( TLK_NEWWEAPON );
	m_OnWeaponPickup.FireOutput( BaseEntity(), BaseEntity() );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

const int MAX_NON_SPECIAL_MULTICOVER = 2;

CUtlVector<AI_EnemyInfo_t *>	g_MultiCoverSearchEnemies;
CNPC_PlayerCompanion *			g_pMultiCoverSearcher;

//-------------------------------------

int __cdecl MultiCoverCompare( AI_EnemyInfo_t * const *ppLeft, AI_EnemyInfo_t * const *ppRight )
{
	const AI_EnemyInfo_t *pLeft = *ppLeft;
	const AI_EnemyInfo_t *pRight = *ppRight;

	if ( !pLeft->hEnemy && !pRight->hEnemy)
		return 0;

	if ( !pLeft->hEnemy )
		return 1;

	if ( !pRight->hEnemy )
		return -1;

	CEntity *left_hEnemy = CEntity::Instance(pLeft->hEnemy);
	CEntity *right_hEnemy = CEntity::Instance(pRight->hEnemy);

	if ( left_hEnemy == g_pMultiCoverSearcher->GetEnemy() )
		return -1;

	if ( right_hEnemy == g_pMultiCoverSearcher->GetEnemy() )
		return 1;

	bool bLeftIsSpecial = ( CNPC_PlayerCompanion::IsMortar( left_hEnemy ) || CNPC_PlayerCompanion::IsSniper( left_hEnemy ) );
	bool bRightIsSpecial = ( CNPC_PlayerCompanion::IsMortar( right_hEnemy ) || CNPC_PlayerCompanion::IsSniper( right_hEnemy ) );

	if ( !bLeftIsSpecial && bRightIsSpecial )
		return 1;

	if ( bLeftIsSpecial && !bRightIsSpecial )
		return -1;

	float leftRelevantTime = ( pLeft->timeLastSeen == AI_INVALID_TIME || pLeft->timeLastSeen == 0 ) ? -99999 : pLeft->timeLastSeen;
	if ( pLeft->timeLastReceivedDamageFrom != AI_INVALID_TIME && pLeft->timeLastReceivedDamageFrom > leftRelevantTime )
		leftRelevantTime = pLeft->timeLastReceivedDamageFrom;

	float rightRelevantTime = ( pRight->timeLastSeen == AI_INVALID_TIME || pRight->timeLastSeen == 0 ) ? -99999 : pRight->timeLastSeen;
	if ( pRight->timeLastReceivedDamageFrom != AI_INVALID_TIME && pRight->timeLastReceivedDamageFrom > rightRelevantTime )
		rightRelevantTime = pRight->timeLastReceivedDamageFrom;

	if ( leftRelevantTime < rightRelevantTime )
		return -1;

	if ( leftRelevantTime > rightRelevantTime )
		return 1;

	float leftDistSq = g_pMultiCoverSearcher->GetAbsOrigin().DistToSqr( left_hEnemy->GetAbsOrigin() );
	float rightDistSq = g_pMultiCoverSearcher->GetAbsOrigin().DistToSqr( right_hEnemy->GetAbsOrigin() );

	if ( leftDistSq < rightDistSq )
		return -1;

	if ( leftDistSq > rightDistSq )
		return 1;

	return 0;
}

//-------------------------------------

void CNPC_PlayerCompanion::SetupCoverSearch( CEntity *pEntity )
{
	if ( IsTurret( pEntity ) )
		gm_fCoverSearchType = CT_TURRET;
	
	gm_bFindingCoverFromAllEnemies = false;
	g_pMultiCoverSearcher = this;

	if ( Classify() == CLASS_PLAYER_ALLY_VITAL || IsInPlayerSquad() )
	{
		if ( GetEnemy() )
		{
			if ( !pEntity || GetEnemies()->NumEnemies() > 1 )
			{
				if ( !pEntity ) // if pEntity is NULL, test is against a point in space, so always to search against current enemy too
					gm_bFindingCoverFromAllEnemies = true;

				AIEnemiesIter_t iter;
				for ( AI_EnemyInfo_t *pEnemyInfo = GetEnemies()->GetFirst(&iter); pEnemyInfo != NULL; pEnemyInfo = GetEnemies()->GetNext(&iter) )
				{
					CEntity *pEnemy = CEntity::Instance(pEnemyInfo->hEnemy);
					if ( pEnemy )
					{
						if ( pEnemy != GetEnemy() )
						{
							if ( pEnemyInfo->timeAtFirstHand == AI_INVALID_TIME || gpGlobals->curtime - pEnemyInfo->timeLastSeen > 10.0 )
								continue;
							gm_bFindingCoverFromAllEnemies = true;
						}
						g_MultiCoverSearchEnemies.AddToTail( pEnemyInfo );
					}
				}

				if ( g_MultiCoverSearchEnemies.Count() == 0 )
				{
					gm_bFindingCoverFromAllEnemies = false;
				}
				else if ( gm_bFindingCoverFromAllEnemies )
				{
					g_MultiCoverSearchEnemies.Sort( MultiCoverCompare );
					Assert( g_MultiCoverSearchEnemies[0]->hEnemy == GetEnemy_CBase() );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::CleanupCoverSearch()
{
	gm_fCoverSearchType = CT_NORMAL;
	g_MultiCoverSearchEnemies.RemoveAll();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::FindCoverPos_Entity( CBaseEntity *pEntity, Vector *pResult)
{
	bool result = false;

	SetupCoverSearch( CEntity::Instance(pEntity) );
	
	if ( gm_bFindingCoverFromAllEnemies )
	{
		result = BaseClass::FindCoverPos_Entity( pEntity, pResult );
		gm_bFindingCoverFromAllEnemies = false;
	}
	
	if ( !result ) {
		//CE_FIX
		//m_bInFindCoverPos_Entity = false;
		result = BaseClass::FindCoverPos_Entity( pEntity, pResult );
	}
	
	CleanupCoverSearch();

	return result;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CNPC_PlayerCompanion::FindCoverPosInRadius( CBaseEntity *pEntity, const Vector &goalPos, float coverRadius, Vector *pResult )
{
	bool result = false;

	SetupCoverSearch( CEntity::Instance(pEntity) );

	if ( gm_bFindingCoverFromAllEnemies )
	{
		result = BaseClass::FindCoverPosInRadius( pEntity, goalPos, coverRadius, pResult );
		gm_bFindingCoverFromAllEnemies = false;
	}

	if ( !result )
	{
		//CE_FIX
		//m_bInFindCoverPosInRadius = false;
		result = BaseClass::FindCoverPosInRadius( pEntity, goalPos, coverRadius, pResult );
	}
	
	CleanupCoverSearch();

	return result;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CNPC_PlayerCompanion::FindCoverPos_Sound( CSound *pSound, Vector *pResult )
{
	bool result = false;
	bool bIsMortar = ( pSound->SoundContext() == SOUND_CONTEXT_MORTAR );

	SetupCoverSearch( NULL );

	if ( gm_bFindingCoverFromAllEnemies )
	{
		result = ( bIsMortar ) ? FindMortarCoverPos( pSound, pResult ) : 
								 BaseClass::FindCoverPos_Sound( pSound, pResult );
		gm_bFindingCoverFromAllEnemies = false;
	}

	if ( !result )
	{
		result = ( bIsMortar ) ? FindMortarCoverPos( pSound, pResult ) : 
								 BaseClass::FindCoverPos_Sound( pSound, pResult );
	}

	CleanupCoverSearch();

	return result;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CNPC_PlayerCompanion::FindMortarCoverPos( CSound *pSound, Vector *pResult )
{
	bool result = false;

	Assert( pSound->SoundContext() == SOUND_CONTEXT_MORTAR );
	gm_fCoverSearchType = CT_MORTAR;
	result = GetTacticalServices()->FindLateralCover( pSound->GetSoundOrigin(), 0, pResult );
	if ( !result )
	{
		result = GetTacticalServices()->FindCoverPos( pSound->GetSoundOrigin(), 
													  pSound->GetSoundOrigin(), 
													  0, 
													  CoverRadius(), 
													  pResult );
	}
	gm_fCoverSearchType = CT_NORMAL;
	
	return result;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsCoverPosition( const Vector &vecThreat, const Vector &vecPosition )
{
	if ( gm_bFindingCoverFromAllEnemies )
	{
		for ( int i = 0; i < g_MultiCoverSearchEnemies.Count(); i++ )
		{
			// @TODO (toml 07-27-04): Should skip checking points near already checked points
			AI_EnemyInfo_t *pEnemyInfo = g_MultiCoverSearchEnemies[i];
			Vector testPos;
			CEntity *pEnemy = CEntity::Instance(pEnemyInfo->hEnemy);
			if ( !pEnemy )
				continue;

			if ( pEnemy == GetEnemy() || IsMortar( pEnemy ) || IsSniper( pEnemy ) || i < MAX_NON_SPECIAL_MULTICOVER )
			{
				testPos = pEnemyInfo->vLastKnownLocation + pEnemy->GetViewOffset();
			}
			else
				break;

			gm_bFindingCoverFromAllEnemies = false;
			bool result = IsCoverPosition( testPos, vecPosition );
			gm_bFindingCoverFromAllEnemies = true;
			
			if ( !result )
				return false;
		}

		if ( gm_fCoverSearchType != CT_MORTAR &&  GetEnemy() && vecThreat.DistToSqr( GetEnemy()->EyePosition() ) < 1 )
			return true;

		// else fall through
	}

	if ( gm_fCoverSearchType == CT_TURRET && GetEnemy() && IsSafeFromFloorTurret( vecPosition, GetEnemy() ) )
	{
		return true;
	}

	if ( gm_fCoverSearchType == CT_MORTAR )
	{
		CSound *pSound = GetBestSound( SOUND_DANGER );
		Assert ( pSound && pSound->SoundContext() == SOUND_CONTEXT_MORTAR );
		if( pSound  )
		{
			// Don't get closer to the shell
			Vector vecToSound = vecThreat - GetAbsOrigin();
			Vector vecToPosition = vecPosition - GetAbsOrigin();
			VectorNormalize( vecToPosition );
			VectorNormalize( vecToSound );

			if ( vecToPosition.AsVector2D().Dot( vecToSound.AsVector2D() ) > 0 )
				return false;

			// Anything outside the radius is okay
			float flDistSqr = (vecPosition - vecThreat).Length2DSqr();
			float radiusSq = Square( pSound->Volume() );
			if( flDistSqr > radiusSq )
			{
				return true;
			}
		}
	}

	//CE_FIX
	//m_bInIsCoverPosition = false;
	return BaseClass::IsCoverPosition( vecThreat, vecPosition );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsMortar( CEntity *pEntity )
{
	if ( !pEntity )
		return false;
	CEntity *pEntityParent = pEntity->GetParent();
	return ( pEntityParent && pEntityParent->GetClassname() == STRING(gm_iszMortarClassname) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsSniper( CEntity *pEntity )
{
	if ( !pEntity )
		return false;
	return ( pEntity->Classify() == CLASS_PROTOSNIPER );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsTurret( CEntity *pEntity )
{
	if ( !pEntity )
		return false;
	const char *pszClassname = pEntity->GetClassname();
	return ( pszClassname == STRING(gm_iszFloorTurretClassname) || pszClassname == STRING(gm_iszGroundTurretClassname) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsGunship( CEntity *pEntity )
{
	if( !pEntity )
		return false;
	return (pEntity->Classify() == CLASS_COMBINE_GUNSHIP );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_PlayerCompanion::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if( info.GetAttacker() )
	{
		CEntity *attacker = CEntity::Instance(info.GetAttacker());
		bool bIsEnvFire;
		if( ( bIsEnvFire = FClassnameIs( attacker, "env_fire" ) ) != false || FClassnameIs( attacker, "entityflame" ) || FClassnameIs( attacker, "env_entity_igniter" ) )
		{
			GetMotor()->SetIdealYawToTarget( attacker->GetAbsOrigin() );
			SetCondition( COND_PC_HURTBYFIRE );
		}

		// @Note (toml 07-25-04): there isn't a good solution to player companions getting injured by
		//						  fires that have huge damage radii that extend outside the rendered
		//						  fire. Recovery from being injured by fire will also not be done
		//						  before we ship/ Here we trade one bug (guys standing around dying
		//						  from flames they appear to not be near), for a lesser one
		//						  this guy was standing in a fire and didn't react. Since
		//						  the levels are supposed to have the centers of all the fires
		//						  npc clipped, this latter case should be rare.
		if ( bIsEnvFire )
		{
			if ( ( GetAbsOrigin() - attacker->GetAbsOrigin() ).Length2DSqr() > Square(12 + GetHullWidth() * .5 ) )
			{
				return 0;
			}
		}
	}

	return BaseClass::OnTakeDamage_Alive( info );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::OnFriendDamaged( CBaseEntity *pSquadmate, CBaseEntity *pAttackerEnt )
{
	BaseClass::OnFriendDamaged( pSquadmate, pAttackerEnt );

	CCombatCharacter *cent_pSquadmate = (CCombatCharacter *)CEntity::Instance(pSquadmate);
	CAI_NPC *pAttacker = CEntity::Instance(pAttackerEnt)->MyNPCPointer();
	if ( pAttacker )
	{
		bool bDirect = ( cent_pSquadmate->FInViewCone_Entity(pAttackerEnt) &&
						 ( ( cent_pSquadmate->IsPlayer() && HasCondition(COND_SEE_PLAYER) ) || 
						 ( cent_pSquadmate->MyNPCPointer() && cent_pSquadmate->MyNPCPointer()->IsPlayerAlly() && 
						   GetSenses()->DidSeeEntity( cent_pSquadmate ) ) ) );
		if ( bDirect )
		{
			UpdateEnemyMemory( pAttackerEnt, pAttacker->GetAbsOrigin(), pSquadmate );
		}
		else
		{
			if ( FVisible_Entity( pSquadmate ) )
			{
				AI_EnemyInfo_t *pInfo = GetEnemies()->Find( pAttackerEnt );
				if ( !pInfo || ( gpGlobals->curtime - pInfo->timeLastSeen ) > 15.0 )
					UpdateEnemyMemory( pAttackerEnt, cent_pSquadmate->GetAbsOrigin(), pSquadmate );
			}
		}

		CPlayer *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
		if ( pPlayer && IsInPlayerSquad() && ( pPlayer->GetAbsOrigin().AsVector2D() - GetAbsOrigin().AsVector2D() ).LengthSqr() < Square( 25*12 ) && IsAllowedToSpeak( TLK_WATCHOUT ) )
		{
			if ( !pPlayer->FInViewCone_Entity( pAttackerEnt ) )
			{
				Vector2D vPlayerDir = pPlayer->EyeDirection2D().AsVector2D();
				Vector2D vEnemyDir = pAttacker->EyePosition().AsVector2D() - pPlayer->EyePosition().AsVector2D();
				vEnemyDir.NormalizeInPlace();
				float dot = vPlayerDir.Dot( vEnemyDir );
				if ( dot < 0 )
					Speak( TLK_WATCHOUT, "dangerloc:behind" );
				else if ( ( pPlayer->GetAbsOrigin().AsVector2D() - pAttacker->GetAbsOrigin().AsVector2D() ).LengthSqr() > Square( 40*12 ) )
					Speak( TLK_WATCHOUT, "dangerloc:far" );
			}
			else if ( pAttacker->GetAbsOrigin().z - pPlayer->GetAbsOrigin().z > 128 )
			{
				Speak( TLK_WATCHOUT, "dangerloc:above" );
			}
			else if ( pAttacker->GetHullType() <= HULL_TINY && ( pPlayer->GetAbsOrigin().AsVector2D() - pAttacker->GetAbsOrigin().AsVector2D() ).LengthSqr() > Square( 100*12 ) )
			{
				Speak( TLK_WATCHOUT, "dangerloc:far" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsValidMoveAwayDest( const Vector &vecDest )
{
	// Don't care what the destination is unless I have an enemy and 
	// that enemy is a sniper (for now).
	if( !GetEnemy() )
	{
		return true;
	}

	if( GetEnemy()->Classify() != CLASS_PROTOSNIPER )
	{
		return true;
	}

	if( IsCoverPosition( GetEnemy()->EyePosition(), vecDest + GetViewOffset() ) )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::FValidateHintType( CBaseEntity *pHint )
{
	CE_AI_Hint *cent = (CE_AI_Hint *)CEntity::Instance(pHint);
	switch( cent->HintType() )
	{
	case HINT_PLAYER_SQUAD_TRANSITON_POINT:
	case HINT_WORLD_VISUALLY_INTERESTING_DONT_AIM:
	case HINT_PLAYER_ALLY_MOVE_AWAY_DEST:
	case HINT_PLAYER_ALLY_FEAR_DEST:
		return true;
		break;

	default:
		break;
	}

	return BaseClass::FValidateHintType( pHint );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::ValidateNavGoal()
{
	bool result;
	if ( GetNavigator()->GetGoalType() == GOALTYPE_COVER )
	{
		if ( IsEnemyTurret() )
			gm_fCoverSearchType = CT_TURRET;
	}
	result = BaseClass::ValidateNavGoal();
	gm_fCoverSearchType = CT_NORMAL;
	return result;
}

const float AVOID_TEST_DIST = 18.0f*12.0f;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define COMPANION_EPISODIC_AVOID_ENTITY_FLAME_RADIUS	18.0f
bool CNPC_PlayerCompanion::OverrideMove( float flInterval )
{
	bool overrode = BaseClass::OverrideMove( flInterval );

	if ( !overrode && GetNavigator()->GetGoalType() != GOALTYPE_NONE )
	{
		string_t iszEnvFire = AllocPooledString( "env_fire" );
		string_t iszBounceBomb = AllocPooledString( "combine_mine" );

#ifdef HL2_EPISODIC			
		string_t iszNPCTurretFloor = AllocPooledString( "npc_turret_floor" );
		string_t iszEntityFlame = AllocPooledString( "entityflame" );
#endif // HL2_EPISODIC

		if ( IsCurSchedule( SCHED_TAKE_COVER_FROM_BEST_SOUND ) )
		{
			CSound *pSound = GetBestSound( SOUND_DANGER );
			if( pSound && pSound->SoundContext() == SOUND_CONTEXT_MORTAR )
			{
				// Try not to get any closer to the center
				GetLocalNavigator()->AddObstacle( pSound->GetSoundOrigin(), (pSound->GetSoundOrigin() - GetAbsOrigin()).Length2D() * 0.5, AIMST_AVOID_DANGER );
			}
		}

		CBaseEntity *pEntity = NULL;
		CEntity *cent_pEntity = NULL;
		trace_t tr;
		
		// For each possible entity, compare our known interesting classnames to its classname, via ID
		while( ( pEntity = OverrideMoveCache_FindTargetsInRadius( pEntity, GetAbsOrigin(), AVOID_TEST_DIST ) ) != NULL )
		{
			cent_pEntity = CEntity::Instance(pEntity);
			// Handle each type
			if ( *(cent_pEntity->m_iClassname) == iszEnvFire )
			{
				Vector vMins, vMaxs;
				if ( FireSystem_GetFireDamageDimensions( pEntity, &vMins, &vMaxs ) )
				{
					UTIL_TraceLine( WorldSpaceCenter(), cent_pEntity->WorldSpaceCenter(), MASK_FIRE_SOLID, pEntity, COLLISION_GROUP_NONE, &tr );
					if (tr.fraction == 1.0 && !tr.startsolid)
					{
						GetLocalNavigator()->AddObstacle( cent_pEntity->GetAbsOrigin(), ( ( vMaxs.x - vMins.x ) * 1.414 * 0.5 ) + 6.0, AIMST_AVOID_DANGER );
					}
				}
			}
#ifdef HL2_EPISODIC			
			else if ( pEntity->m_iClassname == iszNPCTurretFloor )
			{
				UTIL_TraceLine( WorldSpaceCenter(), pEntity->WorldSpaceCenter(), MASK_BLOCKLOS, pEntity, COLLISION_GROUP_NONE, &tr );
				if (tr.fraction == 1.0 && !tr.startsolid)
				{
					float radius = 1.4 * pEntity->CollisionProp()->BoundingRadius2D(); 
					GetLocalNavigator()->AddObstacle( pEntity->WorldSpaceCenter(), radius, AIMST_AVOID_OBJECT );
				}
			}
			else if( pEntity->m_iClassname == iszEntityFlame && pEntity->GetParent() && !pEntity->GetParent()->IsNPC() )
			{
				float flDist = pEntity->WorldSpaceCenter().DistTo( WorldSpaceCenter() );

				if( flDist > COMPANION_EPISODIC_AVOID_ENTITY_FLAME_RADIUS )
				{
					// If I'm not in the flame, prevent me from getting close to it.
					// If I AM in the flame, avoid placing an obstacle until the flame frightens me away from itself.
					UTIL_TraceLine( WorldSpaceCenter(), pEntity->WorldSpaceCenter(), MASK_BLOCKLOS, pEntity, COLLISION_GROUP_NONE, &tr );
					if (tr.fraction == 1.0 && !tr.startsolid)
					{
						GetLocalNavigator()->AddObstacle( pEntity->WorldSpaceCenter(), COMPANION_EPISODIC_AVOID_ENTITY_FLAME_RADIUS, AIMST_AVOID_OBJECT );
					}
				}
			}
#endif // HL2_EPISODIC
			else if ( *(cent_pEntity->m_iClassname) == iszBounceBomb )
			{
				CBounceBomb *pBomb = static_cast<CBounceBomb *>(cent_pEntity);
				if ( pBomb && !pBomb->IsPlayerPlaced() && pBomb->IsAwake() )
				{
					UTIL_TraceLine( WorldSpaceCenter(), cent_pEntity->WorldSpaceCenter(), MASK_BLOCKLOS, pEntity, COLLISION_GROUP_NONE, &tr );
					if (tr.fraction == 1.0 && !tr.startsolid)
					{
						GetLocalNavigator()->AddObstacle( cent_pEntity->GetAbsOrigin(), BOUNCEBOMB_DETONATE_RADIUS * .8, AIMST_AVOID_DANGER );
					}
				}
			}
		}
	}

	return overrode;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost )
{
	bool bResult = BaseClass::MovementCost( moveType, vecStart, vecEnd, pCost );
	if ( moveType == bits_CAP_MOVE_GROUND )
	{
		if ( IsCurSchedule( SCHED_TAKE_COVER_FROM_BEST_SOUND ) )
		{
			CSound *pSound = GetBestSound( SOUND_DANGER );
			if( pSound && (pSound->SoundContext() & (SOUND_CONTEXT_MORTAR|SOUND_CONTEXT_FROM_SNIPER)) )
			{
				Vector vecToSound = pSound->GetSoundReactOrigin() - GetAbsOrigin();
				Vector vecToPosition = vecEnd - GetAbsOrigin();
				VectorNormalize( vecToPosition );
				VectorNormalize( vecToSound );

				if ( vecToPosition.AsVector2D().Dot( vecToSound.AsVector2D() ) > 0 )
				{
					*pCost *= 1.5;
					bResult = true;
				}
			}
		}

		if ( m_bWeightPathsInCover && GetEnemy() )
		{
			if ( BaseClass::IsCoverPosition( GetEnemy()->EyePosition(), vecEnd ) )
			{
				*pCost *= 0.1;
				bResult = true;
			}
		}
	}
	return bResult;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPC_PlayerCompanion::GetIdealSpeed() const
{
	float baseSpeed = BaseClass::GetIdealSpeed();

	if ( baseSpeed < m_flBoostSpeed )
		return m_flBoostSpeed;

	return baseSpeed;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPC_PlayerCompanion::GetIdealAccel() const
{
	float multiplier = 1.0;
	CPlayer *player = UTIL_GetNearestPlayer(GetAbsOrigin());
	if ( player )
	{
		if ( m_bMovingAwayFromPlayer && (player->GetAbsOrigin() - GetAbsOrigin()).Length2DSqr() < Square(3.0*12.0) )
			multiplier = 2.0;
	}
	return BaseClass::GetIdealAccel() * multiplier;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( pMoveGoal->directTrace.flTotalDist - pMoveGoal->directTrace.flDistObstructed < GetHullWidth() * 1.5 )
	{
		CAI_NPC *pBlocker = CEntity::Instance(pMoveGoal->directTrace.pObstruction)->MyNPCPointer();
		if ( pBlocker && pBlocker->IsPlayerAlly() && !pBlocker->IsMoving() && !pBlocker->IsInAScript() &&
			 ( IsCurSchedule( SCHED_NEW_WEAPON ) || 
			   IsCurSchedule( SCHED_GET_HEALTHKIT ) || 
			   pBlocker->IsCurSchedule( SCHED_FAIL ) || 
			   ( IsInPlayerSquad() && !pBlocker->IsInPlayerSquad() ) ||
			   Classify() == CLASS_PLAYER_ALLY_VITAL ||
			   IsInAScript() ) )

		{
			if ( pBlocker->ConditionInterruptsCurSchedule( COND_GIVE_WAY ) || 
				 pBlocker->ConditionInterruptsCurSchedule( COND_PLAYER_PUSHING ) )
			{
				// HACKHACK
				pBlocker->GetMotor()->SetIdealYawToTarget( WorldSpaceCenter() );
				pBlocker->SetSchedule( SCHED_MOVE_AWAY );
			}

		}
	}

	if ( pMoveGoal->directTrace.pObstruction )
	{
	}

	return BaseClass::OnObstructionPreSteer( pMoveGoal, distClear, pResult );
}

//-----------------------------------------------------------------------------
// Purpose: Whether or not we should always transition with the player
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::ShouldAlwaysTransition( void )
{
	// No matter what, come through
	if ( m_bAlwaysTransition )
		return true;

	// Squadmates always come with
	if ( IsInPlayerSquad() )
		return true;

	// If we're following the player, then come along
	if ( GetFollowBehavior().GetFollowTarget() && GetFollowBehavior().GetFollowTarget()->IsPlayer() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputOutsideTransition( inputdata_t &inputdata )
{
	// Must want to do this
	if ( ShouldAlwaysTransition() == false )
		return;

	// If we're in a vehicle, that vehicle will transition with us still inside (which is preferable)
	if ( IsInAVehicle() )
		return;

	CEntity *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
	if(!pPlayer)
		return;

	const Vector &playerPos = pPlayer->GetAbsOrigin();

	// Mark us as already having succeeded if we're vital or always meant to come with the player
	bool bAlwaysTransition = ( ( Classify() == CLASS_PLAYER_ALLY_VITAL ) || m_bAlwaysTransition );
	bool bPathToPlayer = bAlwaysTransition;

	if ( bAlwaysTransition == false )
	{
		AI_Waypoint_t *pPathToPlayer = GetPathfinder()->BuildRoute( GetAbsOrigin(), playerPos, pPlayer->BaseEntity(), 0 );

		if ( pPathToPlayer )
		{
			bPathToPlayer = true;
			CAI_Path tempPath;
			tempPath.SetWaypoints( pPathToPlayer ); // path object will delete waypoints
			GetPathfinder()->UnlockRouteNodes( pPathToPlayer );
		}
	}


#ifdef USE_PATHING_LENGTH_REQUIREMENT_FOR_TELEPORT
	float pathLength = tempPath.GetPathDistanceToGoal( GetAbsOrigin() );

	if ( pathLength > 150 * 12 )
		return;
#endif

	bool bMadeIt = false;
	Vector teleportLocation;

	CE_AI_Hint *pHint = CAI_HintManager::FindHint( this, HINT_PLAYER_SQUAD_TRANSITON_POINT, bits_HINT_NODE_NEAREST, PLAYERCOMPANION_TRANSITION_SEARCH_DISTANCE, &playerPos );
	while ( pHint )
	{
		pHint->Lock(BaseEntity());
		pHint->Unlock(0.5); // prevent other squadmates and self from using during transition. 

		pHint->GetPosition( GetHullType(), &teleportLocation );
		if ( GetNavigator()->CanFitAtPosition( teleportLocation, MASK_NPCSOLID ) )
		{
			bMadeIt = true;
			if ( !bPathToPlayer && ( playerPos - GetAbsOrigin() ).LengthSqr() > Square(40*12) )
			{
				AI_Waypoint_t *pPathToTeleport = GetPathfinder()->BuildRoute( GetAbsOrigin(), teleportLocation, pPlayer->BaseEntity(), 0 );

				if ( !pPathToTeleport )
				{
					DevMsg( 2, "NPC \"%s\" failed to teleport to transition a point because there is no path\n", GetEntityName() );
					bMadeIt = false;
				}
				else
				{
					CAI_Path tempPath;
					GetPathfinder()->UnlockRouteNodes( pPathToTeleport );
					tempPath.SetWaypoints( pPathToTeleport ); // path object will delete waypoints
				}
			}

			if ( bMadeIt )
			{
				DevMsg( 2, "NPC \"%s\" teleported to transition point %d\n", GetEntityName(), pHint->GetNodeId() );
				break;
			}
		}
		pHint = CAI_HintManager::FindHint( this, HINT_PLAYER_SQUAD_TRANSITON_POINT, bits_HINT_NODE_NEAREST, PLAYERCOMPANION_TRANSITION_SEARCH_DISTANCE, &playerPos );
	}
	if ( !bMadeIt )
	{
		// Force us if we didn't find a normal route
		if ( bAlwaysTransition )
		{
			bMadeIt = FindSpotForNPCInRadius( &teleportLocation, pPlayer->GetAbsOrigin(), this, 32.0*1.414, true );
			if ( !bMadeIt )
				bMadeIt = FindSpotForNPCInRadius( &teleportLocation, pPlayer->GetAbsOrigin(), this, 32.0*1.414, false );
		}
	}

	if ( bMadeIt )
	{
		Teleport( &teleportLocation, NULL, NULL );
	}
	else
	{
		DevMsg( 2, "NPC \"%s\" failed to find a suitable transition a point\n", GetEntityName() );
	}

	BaseClass::InputOutsideTransition( inputdata );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputSetReadinessPanic( inputdata_t &inputdata )
{
	SetReadinessLevel( AIRL_PANIC, true, true );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputSetReadinessStealth( inputdata_t &inputdata )
{
	SetReadinessLevel( AIRL_STEALTH, true, true );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputSetReadinessLow( inputdata_t &inputdata )
{
	SetReadinessLevel( AIRL_RELAXED, true, true );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputSetReadinessMedium( inputdata_t &inputdata )
{
	SetReadinessLevel( AIRL_STIMULATED, true, true );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputSetReadinessHigh( inputdata_t &inputdata )
{
	SetReadinessLevel( AIRL_AGITATED, true, true );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputLockReadiness( inputdata_t &inputdata )
{
	float value = inputdata.value.Float();
	LockReadiness( value );
}

//-----------------------------------------------------------------------------
// Purpose: Locks the readiness state of the NCP
// Input  : time - if -1, the lock is effectively infinite
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::LockReadiness( float duration )
{
	if ( duration == -1.0f )
	{
		m_flReadinessLockedUntil = FLT_MAX;
	}
	else
	{
		m_flReadinessLockedUntil = gpGlobals->curtime + duration;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Unlocks the readiness state
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::UnlockReadiness( void )
{
	// Set to the past
	m_flReadinessLockedUntil = gpGlobals->curtime - 0.1f;
}

//------------------------------------------------------------------------------
#ifdef HL2_EPISODIC

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::ShouldDeferToPassengerBehavior( void )
{
	if ( m_PassengerBehavior.CanSelectSchedule() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Determines if this player companion is capable of entering a vehicle
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::CanEnterVehicle( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::CanExitVehicle( void )
{
	// See if we can exit our vehicle
	CPropJeepEpisodic *pVehicle = dynamic_cast<CPropJeepEpisodic *>(m_PassengerBehavior.GetTargetVehicle());
	if ( pVehicle != NULL && pVehicle->NPC_CanExitVehicle( this, true ) == false )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *lpszVehicleName - 
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::EnterVehicle( CBaseEntity *pEntityVehicle, bool bImmediately )
{
	// Must be allowed to do this
	if ( CanEnterVehicle() == false )
		return;

	// Find the target vehicle
	CPropJeepEpisodic *pVehicle = dynamic_cast<CPropJeepEpisodic *>(pEntityVehicle);

	// Get in the car if it's valid
	if ( pVehicle != NULL && pVehicle->NPC_CanEnterVehicle( this, true ) )
	{
		// Set her into a "passenger" behavior
		m_PassengerBehavior.Enable( pVehicle, bImmediately );
		m_PassengerBehavior.EnterVehicle();

		// Only do this if we're outside the vehicle
		if ( m_PassengerBehavior.GetPassengerState() == PASSENGER_STATE_OUTSIDE )
		{
			SetCondition( COND_PC_BECOMING_PASSENGER );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get into the requested vehicle
// Input  : &inputdata - contains the entity name of the vehicle to enter
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputEnterVehicle( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = FindNamedEntity( inputdata.value.String() );
	EnterVehicle( pEntity, false );
}

//-----------------------------------------------------------------------------
// Purpose: Get into the requested vehicle immediately (no animation, pop)
// Input  : &inputdata - contains the entity name of the vehicle to enter
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputEnterVehicleImmediately( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = FindNamedEntity( inputdata.value.String() );
	EnterVehicle( pEntity, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputExitVehicle( inputdata_t &inputdata )
{
	// See if we're allowed to exit the vehicle
	if ( CanExitVehicle() == false )
		return;

	m_PassengerBehavior.ExitVehicle();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputCancelEnterVehicle( inputdata_t &inputdata )
{
	m_PassengerBehavior.CancelEnterVehicle();
}

//-----------------------------------------------------------------------------
// Purpose: Forces the NPC out of the vehicle they're riding in
// Input  : bImmediate - If we need to exit immediately, teleport to any exit location
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::ExitVehicle( void )
{
	// For now just get out
	m_PassengerBehavior.ExitVehicle();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsInAVehicle( void ) const
{
	// Must be active and getting in/out of vehicle
	if ( m_PassengerBehavior.IsEnabled() && m_PassengerBehavior.GetPassengerState() != PASSENGER_STATE_OUTSIDE )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : IServerVehicle - 
//-----------------------------------------------------------------------------
IServerVehicle *CNPC_PlayerCompanion::GetVehicle( void )
{
	if ( IsInAVehicle() )
	{
		CPropVehicleDriveable *pDriveableVehicle = m_PassengerBehavior.GetTargetVehicle();
		if ( pDriveableVehicle != NULL )
			return pDriveableVehicle->GetServerVehicle();
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_PlayerCompanion::GetVehicleEntity( void )
{
	if ( IsInAVehicle() )
	{
		CPropVehicleDriveable *pDriveableVehicle = m_PassengerBehavior.GetTargetVehicle();
			return pDriveableVehicle;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Override our efficiency so that we don't jitter when we're in the middle
//			of our enter/exit animations.
// Input  : bInPVS - Whether we're in the PVS or not
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::UpdateEfficiency( bool bInPVS )
{ 
	// If we're transitioning and in the PVS, we override our efficiency
	if ( IsInAVehicle() && bInPVS )
	{
		PassengerState_e nState = m_PassengerBehavior.GetPassengerState();
		if ( nState == PASSENGER_STATE_ENTERING || nState == PASSENGER_STATE_EXITING )
		{
			SetEfficiency( AIE_NORMAL );
			return;
		}
	}

	// Do the default behavior
	BaseClass::UpdateEfficiency( bInPVS );
}

//-----------------------------------------------------------------------------
// Purpose: Whether or not we can dynamically interact with another NPC
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::CanRunAScriptedNPCInteraction( bool bForced /*= false*/ )
{
	// TODO: Allow this but only for interactions who stem from being in a vehicle?
	if ( IsInAVehicle() )
		return false;

	return BaseClass::CanRunAScriptedNPCInteraction( bForced );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsAllowedToDodge( void )
{
	// TODO: Allow this but only for interactions who stem from being in a vehicle?
	if ( IsInAVehicle() )
		return false;

	return BaseClass::IsAllowedToDodge();
}

#endif	//HL2_EPISODIC
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Always transition along with the player
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputEnableAlwaysTransition( inputdata_t &inputdata )
{
	m_bAlwaysTransition = true;
}

//-----------------------------------------------------------------------------
// Purpose: Stop always transitioning along with the player
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputDisableAlwaysTransition( inputdata_t &inputdata )
{
	m_bAlwaysTransition = false;
}

//-----------------------------------------------------------------------------
// Purpose: Stop picking up weapons from the ground
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputEnableWeaponPickup( inputdata_t &inputdata )
{
	m_bDontPickupWeapons = false;
}

//-----------------------------------------------------------------------------
// Purpose: Return to default behavior of picking up better weapons on the ground
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputDisableWeaponPickup( inputdata_t &inputdata )
{
	m_bDontPickupWeapons = true;
}

//------------------------------------------------------------------------------
// Purpose: Give the NPC in question the weapon specified
//------------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputGiveWeapon( inputdata_t &inputdata )
{
	// Give the NPC the specified weapon
	string_t iszWeaponName = inputdata.value.StringID();
	if ( iszWeaponName != NULL_STRING )
	{
		if( Classify() == CLASS_PLAYER_ALLY_VITAL )
		{
			m_iszPendingWeapon = iszWeaponName;
		}
		else
		{
			GiveWeapon( iszWeaponName );
		}
	}
}

#if HL2_EPISODIC
//------------------------------------------------------------------------------
// Purpose: Delete all outputs from this NPC.
//------------------------------------------------------------------------------
void CNPC_PlayerCompanion::InputClearAllOuputs( inputdata_t &inputdata )
{
	datamap_t *dmap = GetDataDescMap();
	while ( dmap )
	{
		int fields = dmap->dataNumFields;
		for ( int i = 0; i < fields; i++ )
		{
			typedescription_t *dataDesc = &dmap->dataDesc[i];
			if ( ( dataDesc->fieldType == FIELD_CUSTOM ) && ( dataDesc->flags & FTYPEDESC_OUTPUT ) )
			{
				CBaseEntityOutput *pOutput = (CBaseEntityOutput *)((int)this + (int)dataDesc->fieldOffset[0]);
				pOutput->DeleteAllElements();
				/*
				int nConnections = pOutput->NumberOfElements();
				for ( int j = 0; j < nConnections; j++ )
				{

				}
				*/
			}
		}

		dmap = dmap->baseMap;
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Player in our squad killed something
// Input  : *pVictim - Who he killed
//			&info - How they died
//-----------------------------------------------------------------------------
void CNPC_PlayerCompanion::OnPlayerKilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	// filter everything that comes in here that isn't an NPC
	CAI_NPC *pCombatVictim = dynamic_cast<CAI_NPC *>( CEntity::Instance(pVictim) );
	if ( !pCombatVictim )
	{
		return;
	}

	CEntity *pInflictor = CEntity::Instance(info.GetInflictor());
	int		iNumBarrels = 0;
	int		iConsecutivePlayerKills = 0;
	bool	bPuntedGrenade = false;
	bool	bVictimWasEnemy = false;
	bool	bVictimWasMob = false;
	bool	bVictimWasAttacker = false;
	bool	bHeadshot = false;
	bool	bOneShot = false;

	if ( dynamic_cast<CE_CBreakableProp *>( pInflictor ) && ( info.GetDamageType() & DMG_BLAST ) )
	{
		// if a barrel explodes that was initiated by the player within a few seconds of the previous one,
		// increment a counter to keep track of how many have exploded in a row.
		if ( gpGlobals->curtime - m_fLastBarrelExploded >= MAX_TIME_BETWEEN_BARRELS_EXPLODING )
		{
			m_iNumConsecutiveBarrelsExploded = 0;
		}
		m_iNumConsecutiveBarrelsExploded++;
		m_fLastBarrelExploded = gpGlobals->curtime;

		iNumBarrels = m_iNumConsecutiveBarrelsExploded;
	}
	else
	{
		// if player kills an NPC within a few seconds of the previous kill,
		// increment a counter to keep track of how many he's killed in a row.
		if ( gpGlobals->curtime - m_fLastPlayerKill >= MAX_TIME_BETWEEN_CONSECUTIVE_PLAYER_KILLS )
		{
			m_iNumConsecutivePlayerKills = 0;
		}
		m_iNumConsecutivePlayerKills++;
		m_fLastPlayerKill = gpGlobals->curtime;
		iConsecutivePlayerKills = m_iNumConsecutivePlayerKills;
	}

	// don't comment on kills when she can't see the victim
	if ( !FVisible_Entity( pVictim ) )
	{
		return;
	}

	//CE_MODIFY
	// check if the player killed an enemy by punting a grenade
	if ( pInflictor /*&& Fraggrenade_WasPunted( pInflictor ) */ && Fraggrenade_WasCreatedByCombine( pInflictor ) )
	{
		bPuntedGrenade = true;
	}

	// check if the victim was Alyx's enemy
	if ( GetEnemy() == pCombatVictim )
	{
		bVictimWasEnemy = true;
	}

	AI_EnemyInfo_t *pEMemory = GetEnemies()->Find( pVictim );
	if ( pEMemory != NULL ) 
	{
		// was Alyx being mobbed by this enemy?
		bVictimWasMob = pEMemory->bMobbedMe;

		// has Alyx recieved damage from this enemy?
		if ( pEMemory->timeLastReceivedDamageFrom > 0 ) {
			bVictimWasAttacker = true;
		}
	}

	// Was it a headshot?
	if ( ( pCombatVictim->LastHitGroup() == HITGROUP_HEAD ) && ( info.GetDamageType() & DMG_BULLET ) )
	{
		bHeadshot = true;
	}

	// Did the player kill the enemy with 1 shot?
	if ( ( pCombatVictim->GetDamageCount() == 1 ) && ( info.GetDamageType() & DMG_BULLET ) )
	{
		bOneShot = true;
	}

	// set up the speech modifiers
	CFmtStrN<512> modifiers( "num_barrels:%d,distancetoplayerenemy:%f,playerAmmo:%s,consecutive_player_kills:%d,"
		"punted_grenade:%d,victim_was_enemy:%d,victim_was_mob:%d,victim_was_attacker:%d,headshot:%d,oneshot:%d",
		iNumBarrels, EnemyDistance( pCombatVictim ), info.GetAmmoName(), iConsecutivePlayerKills,
		bPuntedGrenade, bVictimWasEnemy, bVictimWasMob, bVictimWasAttacker, bHeadshot, bOneShot );

	SpeakIfAllowed( TLK_PLAYER_KILLED_NPC, modifiers );

	BaseClass::OnPlayerKilledOther( pVictim, info );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerCompanion::IsNavigationUrgent( void )
{
	bool bBase = BaseClass::IsNavigationUrgent();

	// Consider follow & assault behaviour urgent
	if ( !bBase && (m_FollowBehavior.IsActive() || ( m_AssaultBehavior.IsRunning() && m_AssaultBehavior.IsUrgent() )) && Classify() == CLASS_PLAYER_ALLY_VITAL ) 
	{
		// But only if the blocker isn't the player, and isn't a physics object that's still moving
		CEntity *pBlocker = GetNavigator()->GetBlockingEntity();
		if ( pBlocker && !pBlocker->IsPlayer() )
		{
			IPhysicsObject *pPhysObject = pBlocker->VPhysicsGetObject();
			if ( pPhysObject && !pPhysObject->IsAsleep() )
				return false;
			if ( pBlocker->IsNPC() )
				return false;
		}

		// If we're within the player's viewcone, then don't teleport.

		// This test was made more general because previous iterations had cases where characters
		// could not see the player but the player could in fact see them.  Now the NPC's facing is
		// irrelevant and the player's viewcone is more authorative. -- jdw

		for (int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CPlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;

			if ( pPlayer->FInViewCone_Vector( EyePosition() ) )
				return false;
		}

		return true;
	}

	return bBase;
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( player_companion_base, CNPC_PlayerCompanion )

	// AI Interaction for being hit by a physics object
	DECLARE_INTERACTION(*g_interactionHitByPlayerThrownPhysObj)
	DECLARE_INTERACTION(g_interactionPlayerPuntedHeavyObject)

	DECLARE_CONDITION( COND_PC_HURTBYFIRE )
	DECLARE_CONDITION( COND_PC_SAFE_FROM_MORTAR )
	DECLARE_CONDITION( COND_PC_BECOMING_PASSENGER )

	DECLARE_TASK( TASK_PC_WAITOUT_MORTAR )
	DECLARE_TASK( TASK_PC_GET_PATH_OFF_COMPANION )

	DECLARE_ANIMEVENT( AE_COMPANION_PRODUCE_FLARE )
	DECLARE_ANIMEVENT( AE_COMPANION_LIGHT_FLARE )
	DECLARE_ANIMEVENT( AE_COMPANION_RELEASE_FLARE )

	//=========================================================
	// > TakeCoverFromBestSound
	//
	//	Find cover and move towards it, but only do so for a short
	//  time. This is appropriate when the dangerous item is going
	//  to detonate very soon. This way our NPC doesn't run a great
	//  distance from an object that explodes shortly after the NPC
	//  gets underway.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PC_MOVE_TOWARDS_COVER_FROM_BEST_SOUND,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_FLEE_FROM_BEST_SOUND"
		"		 TASK_STOP_MOVING					0"
		"		 TASK_SET_TOLERANCE_DISTANCE		24"
		"		 TASK_STORE_BESTSOUND_REACTORIGIN_IN_SAVEPOSITION	0"
		"		 TASK_FIND_COVER_FROM_BEST_SOUND	0"
		"		 TASK_RUN_PATH_TIMED				1.0"
		"		 TASK_STOP_MOVING					0"
		"		 TASK_FACE_SAVEPOSITION				0"
		"		 TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"	// Translated to cover
		""
		"	Interrupts"
		"		COND_PC_SAFE_FROM_MORTAR"
	)

	DEFINE_SCHEDULE
	(
	SCHED_PC_TAKE_COVER_FROM_BEST_SOUND,

	"	Tasks"
	"		 TASK_SET_FAIL_SCHEDULE								SCHEDULE:SCHED_FLEE_FROM_BEST_SOUND"
	"		 TASK_STOP_MOVING									0"
	"		 TASK_SET_TOLERANCE_DISTANCE						24"
	"		 TASK_STORE_BESTSOUND_REACTORIGIN_IN_SAVEPOSITION	0"
	"		 TASK_FIND_COVER_FROM_BEST_SOUND					0"
	"		 TASK_RUN_PATH										0"
	"		 TASK_WAIT_FOR_MOVEMENT								0"
	"		 TASK_STOP_MOVING									0"
	"		 TASK_FACE_SAVEPOSITION								0"
	"		 TASK_SET_ACTIVITY									ACTIVITY:ACT_IDLE"	// Translated to cover
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_PC_SAFE_FROM_MORTAR"
	)

	DEFINE_SCHEDULE	
	(
		SCHED_PC_COWER,
		  
		"	Tasks"
		"		TASK_WAIT_RANDOM			0.1"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_COWER"
		"		TASK_PC_WAITOUT_MORTAR		0"
		"		TASK_WAIT					0.1"	
		"		TASK_WAIT_RANDOM			0.5"	
		""
		"	Interrupts"
		"		"
	)

	//=========================================================
	//
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PC_FLEE_FROM_BEST_SOUND,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_COWER"
		"		 TASK_GET_PATH_AWAY_FROM_BEST_SOUND	600"
		"		 TASK_RUN_PATH_TIMED				1.5"
		"		 TASK_STOP_MOVING					0"
		"		 TASK_TURN_LEFT						179"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_PC_SAFE_FROM_MORTAR"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PC_FAIL_TAKE_COVER_TURRET,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_COWER"
		"		 TASK_STOP_MOVING					0"
		"		 TASK_MOVE_AWAY_PATH				600"
		"		 TASK_RUN_PATH_FLEE					100"
		"		 TASK_STOP_MOVING					0"
		"		 TASK_TURN_LEFT						179"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PC_FAKEOUT_MORTAR,

		"	Tasks"
		"		TASK_MOVE_AWAY_PATH						300"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		""
		"	Interrupts"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PC_GET_OFF_COMPANION,

		"	Tasks"
		"		TASK_PC_GET_PATH_OFF_COMPANION				0"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		""
		"	Interrupts"
		""
	)

AI_END_CUSTOM_NPC()



//
// Special movement overrides for player companions
//

#define NUM_OVERRIDE_MOVE_CLASSNAMES	4

class COverrideMoveCache : public IEntityListener
{
public:
	void LevelInitPreEntity( void )
	{ 
		CacheClassnames();
		g_helpfunc.AddListenerEntity( this );
		Clear(); 
	}
	void LevelShutdownPostEntity( void  )
	{
		g_helpfunc.RemoveListenerEntity( this );
		Clear();
	}

	inline void Clear( void )
	{ 
		m_Cache.Purge(); 
	}

	inline bool MatchesCriteria( CBaseEntity *pEntity )
	{
		CEntity *cent = CEntity::Instance(pEntity);
		if(cent == NULL) {
			return false;
		}

		string_t class_name = cent->m_iClassname;
		for ( int i = 0; i < NUM_OVERRIDE_MOVE_CLASSNAMES; i++ )
		{
			if ( class_name == m_Classname[i] )
				return true;
		}

		return false;
	}

	virtual void OnEntitySpawned( CBaseEntity *pEntity )
	{
		if ( MatchesCriteria( pEntity ) )
		{
			m_Cache.AddToTail( pEntity );
		}
	};

	virtual void OnEntityDeleted( CBaseEntity *pEntity )
	{
		if ( !m_Cache.Count() )
			return;

		if ( MatchesCriteria( pEntity ) )
		{
			m_Cache.FindAndRemove( pEntity );
		}
	};

	CBaseEntity *FindTargetsInRadius( CBaseEntity *pFirstEntity, const Vector &vecOrigin, float flRadius )
	{
		if ( !m_Cache.Count() )
			return NULL;

		int nIndex = m_Cache.InvalidIndex();

		// If we're starting with an entity, start there and move past it
		if ( pFirstEntity != NULL ) 
		{
			nIndex = m_Cache.Find( pFirstEntity );
			nIndex = m_Cache.Next( nIndex );
			if ( nIndex == m_Cache.InvalidIndex() )
				return NULL;
		}
		else 
		{
			nIndex = m_Cache.Head();
		}

		CEntity *pTarget = NULL;
		const float flRadiusSqr = Square( flRadius );

		// Look through each cached target, looking for one in our range
		while ( nIndex != m_Cache.InvalidIndex() )
		{
			pTarget = CEntity::Instance(m_Cache[nIndex]);
			if ( pTarget && ( pTarget->GetAbsOrigin() - vecOrigin ).LengthSqr() < flRadiusSqr )
				return pTarget->BaseEntity();

			nIndex = m_Cache.Next( nIndex );
		}

		return NULL;
	}

	void ForceRepopulateList( void )
	{
		Clear();
		CacheClassnames();

		CBaseEntity *pEnt = g_helpfunc.FirstEnt();
		while( pEnt )
		{
			if( MatchesCriteria( pEnt ) )
			{
				m_Cache.AddToTail( pEnt );
			}

			pEnt = g_helpfunc.NextEnt( pEnt );
		}
	}

private:
	inline void CacheClassnames( void )
	{
		m_Classname[0] = AllocPooledString( "env_fire" );
		m_Classname[1] = AllocPooledString( "combine_mine" );
		m_Classname[2] = AllocPooledString( "npc_turret_floor" );
		m_Classname[3] = AllocPooledString( "entityflame" );
	}

	CUtlLinkedList<EHANDLE>	m_Cache;
	string_t				m_Classname[NUM_OVERRIDE_MOVE_CLASSNAMES];
};

// Singleton for access
COverrideMoveCache g_OverrideMoveCache;
COverrideMoveCache *OverrideMoveCache( void ) { return &g_OverrideMoveCache; }

CBaseEntity *OverrideMoveCache_FindTargetsInRadius( CBaseEntity *pFirstEntity, const Vector &vecOrigin, float flRadius )
{
	return g_OverrideMoveCache.FindTargetsInRadius( pFirstEntity, vecOrigin, flRadius );
}

void OverrideMoveCache_ForceRepopulateList( void )
{
	g_OverrideMoveCache.ForceRepopulateList();
}


class OverrideMoveCacheHelper : public CBaseGameSystem
{
public:
	OverrideMoveCacheHelper( char const *name ) : CBaseGameSystem( name )
	{
	}
	void LevelInitPreEntity()
	{
		g_OverrideMoveCache.LevelInitPreEntity();
	}
	void LevelShutdownPostEntity()
	{
		g_OverrideMoveCache.LevelShutdownPostEntity();
	}
};

static OverrideMoveCacheHelper g_OverrideMoveCacheHelper("OverrideMoveCacheHelper");
