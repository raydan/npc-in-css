
#include "CEntity.h"
#include "CCombatCharacter.h"
#include "ammodef.h"
#include "CCombatWeapon.h"
#include "player_pickup.h"
#include "CNPCBaseWeapon.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



Relationship_t**  *CCombatCharacter::m_DefaultRelationship = NULL;
int				  *CCombatCharacter::m_lastInteraction	   = NULL;

SH_DECL_MANUALHOOK1(IRelationType, 0, 0, 0, Disposition_t, CBaseEntity *);
DECLARE_HOOK(IRelationType, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, IRelationType, Disposition_t, (CBaseEntity *pTarget), (pTarget));

SH_DECL_MANUALHOOK1(IRelationPriority, 0, 0, 0, int, CBaseEntity *);
DECLARE_HOOK(IRelationPriority, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, IRelationPriority, int, (CBaseEntity *pTarget), (pTarget));

SH_DECL_MANUALHOOK1(FInAimCone_Entity, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(FInAimCone_Entity, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, FInAimCone_Entity, bool, (CBaseEntity *pEntity), (pEntity));

SH_DECL_MANUALHOOK0(EyeDirection3D, 0, 0, 0, Vector);
DECLARE_HOOK(EyeDirection3D, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, EyeDirection3D, Vector, (), ());

SH_DECL_MANUALHOOK1(CorpseGib, 0, 0, 0, bool, const CTakeDamageInfo &);
DECLARE_HOOK(CorpseGib, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, CorpseGib, bool, (const CTakeDamageInfo &info), (info));

SH_DECL_MANUALHOOK1(OnTakeDamage_Alive, 0, 0, 0, int, const CTakeDamageInfo &);
DECLARE_HOOK(OnTakeDamage_Alive, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, OnTakeDamage_Alive, int, (const CTakeDamageInfo &info), (info));

SH_DECL_MANUALHOOK3(HandleInteraction, 0, 0, 0, bool, int, void *, CBaseEntity *);
DECLARE_HOOK(HandleInteraction, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, HandleInteraction, bool, (int interactionType, void *data, CBaseEntity* sourceEnt), (interactionType, data, sourceEnt));

SH_DECL_MANUALHOOK1(OnTakeDamage_Dying, 0, 0, 0, int, const CTakeDamageInfo &);
DECLARE_HOOK(OnTakeDamage_Dying, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, OnTakeDamage_Dying, int, (const CTakeDamageInfo &info), (info));

SH_DECL_MANUALHOOK1(OnTakeDamage_Dead, 0, 0, 0, int, const CTakeDamageInfo &);
DECLARE_HOOK(OnTakeDamage_Dead, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, OnTakeDamage_Dead, int, (const CTakeDamageInfo &info), (info));

SH_DECL_MANUALHOOK7(CheckTraceHullAttack_Float, 0, 0, 0, CBaseEntity *, float , const Vector &, const Vector &, int, int, float, bool);
DECLARE_HOOK(CheckTraceHullAttack_Float, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, CheckTraceHullAttack_Float, CBaseEntity *, (float flDist, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float forceScale, bool bDamageAnyNPC), (flDist, mins, maxs, iDamage, iDmgType, forceScale, bDamageAnyNPC));

SH_DECL_MANUALHOOK8(CheckTraceHullAttack_Vector, 0, 0, 0, CBaseEntity *, const Vector &, const Vector &, const Vector &, const Vector &, int , int , float, bool);
DECLARE_HOOK(CheckTraceHullAttack_Vector, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, CheckTraceHullAttack_Vector, CBaseEntity *, (const Vector &vStart, const Vector &vEnd, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float flForceScale, bool bDamageAnyNPC), (vStart, vEnd, mins, maxs, iDamage, iDmgType, flForceScale, bDamageAnyNPC));

SH_DECL_MANUALHOOK2(BecomeRagdoll, 0, 0, 0, bool, const CTakeDamageInfo &, const Vector &);
DECLARE_HOOK(BecomeRagdoll, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, BecomeRagdoll, bool, (const CTakeDamageInfo &info, const Vector &forceVector), (info, forceVector));

SH_DECL_MANUALHOOK1(BecomeRagdollOnClient, 0, 0, 0, bool, const Vector &);
DECLARE_HOOK(BecomeRagdollOnClient, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, BecomeRagdollOnClient, bool, (const Vector &force), (force));

SH_DECL_MANUALHOOK0(IsInAVehicle, 0, 0, 0, bool);
DECLARE_HOOK(IsInAVehicle, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, IsInAVehicle, bool, (), ());

SH_DECL_MANUALHOOK0(GetVehicleEntity, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(GetVehicleEntity, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, GetVehicleEntity, CBaseEntity *, (), ());

SH_DECL_MANUALHOOK1(FInViewCone_Entity, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(FInViewCone_Entity, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, FInViewCone_Entity, bool, (CBaseEntity *pEntity), (pEntity));

SH_DECL_MANUALHOOK1(FInViewCone_Vector, 0, 0, 0, bool, const Vector &);
DECLARE_HOOK(FInViewCone_Vector, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, FInViewCone_Vector, bool, (const Vector &vecSpot), (vecSpot));

SH_DECL_MANUALHOOK0(CanBecomeServerRagdoll, 0, 0, 0, bool);
DECLARE_HOOK(CanBecomeServerRagdoll, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, CanBecomeServerRagdoll, bool, (), ());

SH_DECL_MANUALHOOK1_void(Event_Dying, 0, 0, 0, CTakeDamageInfo const&);
DECLARE_HOOK(Event_Dying, CCombatCharacter);
DECLARE_DEFAULTHANDLER_void(CCombatCharacter, Event_Dying, (CTakeDamageInfo const& info), (info));

SH_DECL_MANUALHOOK1(ShouldGib, 0, 0, 0, bool, const CTakeDamageInfo &);
DECLARE_HOOK(ShouldGib, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, ShouldGib, bool, (const CTakeDamageInfo &info ), (info));

SH_DECL_MANUALHOOK0(BodyAngles, 0, 0, 0, QAngle);
DECLARE_HOOK(BodyAngles, CCombatCharacter);
DECLARE_DEFAULTHANDLER_SPECIAL(CCombatCharacter, BodyAngles, QAngle, (), (), vec3_angle);

SH_DECL_MANUALHOOK0(BodyDirection3D, 0, 0, 0, Vector);
DECLARE_HOOK(BodyDirection3D, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, BodyDirection3D, Vector, (), ());

SH_DECL_MANUALHOOK0(BodyDirection2D, 0, 0, 0, Vector);
DECLARE_HOOK(BodyDirection2D, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, BodyDirection2D, Vector, (), ());

SH_DECL_MANUALHOOK2_void(OnChangeActiveWeapon, 0, 0, 0, CBaseEntity *, CBaseEntity *);
DECLARE_HOOK(OnChangeActiveWeapon, CCombatCharacter);
DECLARE_DEFAULTHANDLER_void(CCombatCharacter, OnChangeActiveWeapon,(CBaseEntity *pOldWeapon, CBaseEntity *pNewWeapon), (pOldWeapon, pNewWeapon));

SH_DECL_MANUALHOOK2_void(OnFriendDamaged, 0, 0, 0, CBaseEntity *, CBaseEntity *);
DECLARE_HOOK(OnFriendDamaged, CCombatCharacter);
DECLARE_DEFAULTHANDLER_void(CCombatCharacter, OnFriendDamaged, (CBaseEntity *pSquadmate, CBaseEntity *pAttacker), (pSquadmate, pAttacker));

SH_DECL_MANUALHOOK3_void(AddClassRelationship, 0, 0, 0, Class_T , Disposition_t , int );
DECLARE_HOOK(AddClassRelationship, CCombatCharacter);
DECLARE_DEFAULTHANDLER_void(CCombatCharacter, AddClassRelationship, (Class_T nClass, Disposition_t nDisposition, int nPriority), (nClass, nDisposition, nPriority));

SH_DECL_MANUALHOOK3_void(AddEntityRelationship, 0, 0, 0, CBaseEntity *, Disposition_t , int  );
DECLARE_HOOK(AddEntityRelationship, CCombatCharacter);
DECLARE_DEFAULTHANDLER_void(CCombatCharacter, AddEntityRelationship, (CBaseEntity *pEntity, Disposition_t nDisposition, int nPriority), (pEntity, nDisposition, nPriority));

SH_DECL_MANUALHOOK1(RemoveEntityRelationship, 0, 0, 0, bool, CBaseEntity * );
DECLARE_HOOK(RemoveEntityRelationship, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, RemoveEntityRelationship, bool, (CBaseEntity *pEntity), (pEntity));

SH_DECL_MANUALHOOK3_void(Weapon_Drop, 0, 0, 0, CBaseEntity *, const Vector * , const Vector *);
DECLARE_HOOK(Weapon_Drop, CCombatCharacter);
DECLARE_DEFAULTHANDLER_void(CCombatCharacter, Weapon_Drop, (CBaseEntity *pWeapon, const Vector *pvecTarget , const Vector *pVelocity), (pWeapon, pvecTarget, pVelocity));

SH_DECL_MANUALHOOK1_void(Weapon_Equip, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(Weapon_Equip, CCombatCharacter);
DECLARE_DEFAULTHANDLER_void(CCombatCharacter, Weapon_Equip, (CBaseEntity *pWeapon), (pWeapon));

SH_DECL_MANUALHOOK1(Weapon_GetSlot, 0, 0, 0, CBaseEntity *, int);
DECLARE_HOOK(Weapon_GetSlot, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, Weapon_GetSlot, CBaseEntity *, (int slot) const, (slot));

SH_DECL_MANUALHOOK0(Weapon_ShootPosition, 0, 0, 0, Vector);
DECLARE_HOOK(Weapon_ShootPosition, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, Weapon_ShootPosition, Vector, (), ());

SH_DECL_MANUALHOOK4(BecomeRagdollBoogie, 0, 0, 0, bool, CBaseEntity *, const Vector &, float , int );
DECLARE_HOOK(BecomeRagdollBoogie, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, BecomeRagdollBoogie, bool, (CBaseEntity *pKiller, const Vector &forceVector, float duration, int flags), (pKiller, forceVector, duration, flags));

SH_DECL_MANUALHOOK1_void(NotifyFriendsOfDamage, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(NotifyFriendsOfDamage, CCombatCharacter);
DECLARE_DEFAULTHANDLER_void(CCombatCharacter, NotifyFriendsOfDamage, (CBaseEntity *pAttackerEntity), (pAttackerEntity));

SH_DECL_MANUALHOOK2(Weapon_Switch, 0, 0, 0, bool, CBaseEntity *, int);
DECLARE_HOOK(Weapon_Switch, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, Weapon_Switch, bool, (CBaseEntity *pWeapon, int viewmodelindex), (pWeapon, viewmodelindex));

SH_DECL_MANUALHOOK2(GetAttackSpread, 0, 0, 0, Vector, CBaseEntity *, CBaseEntity *);
DECLARE_HOOK(GetAttackSpread, CCombatCharacter);
DECLARE_DEFAULTHANDLER_SPECIAL(CCombatCharacter, GetAttackSpread, Vector, (CBaseEntity *pWeapon, CBaseEntity *pTarget), (pWeapon, pTarget), vec3_origin);

SH_DECL_MANUALHOOK1(ShouldShootMissTarget, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(ShouldShootMissTarget, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, ShouldShootMissTarget, bool, (CBaseEntity *pAttacker), (pAttacker));

SH_DECL_MANUALHOOK0(FindMissTarget, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(FindMissTarget, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, FindMissTarget, CBaseEntity *, (), ());

SH_DECL_MANUALHOOK2(GetSpreadBias, 0, 0, 0, float, CBaseEntity *, CBaseEntity *);
DECLARE_HOOK(GetSpreadBias, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, GetSpreadBias, float , (CBaseEntity *pWeapon, CBaseEntity *pTarget), (pWeapon, pTarget));

SH_DECL_MANUALHOOK0(HeadDirection2D, 0, 0, 0, Vector);
DECLARE_HOOK(HeadDirection2D, CCombatCharacter);
DECLARE_DEFAULTHANDLER_SPECIAL(CCombatCharacter, HeadDirection2D, Vector, (), (), vec3_origin);

SH_DECL_MANUALHOOK0(HeadDirection3D, 0, 0, 0, Vector);
DECLARE_HOOK(HeadDirection3D, CCombatCharacter);
DECLARE_DEFAULTHANDLER_SPECIAL(CCombatCharacter, HeadDirection3D, Vector, (), (), vec3_origin);

SH_DECL_MANUALHOOK0(EyeDirection2D, 0, 0, 0, Vector);
DECLARE_HOOK(EyeDirection2D, CCombatCharacter);
DECLARE_DEFAULTHANDLER_SPECIAL(CCombatCharacter, EyeDirection2D, Vector, (), (), vec3_origin);

SH_DECL_MANUALHOOK1(CalcWeaponProficiency, 0, 0, 0, WeaponProficiency_t, CBaseEntity *);
DECLARE_HOOK(CalcWeaponProficiency, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, CalcWeaponProficiency, WeaponProficiency_t, (CBaseEntity *pWeapon) , (pWeapon));

SH_DECL_MANUALHOOK1(FInAimCone_Vector, 0, 0, 0, bool, const Vector &);
DECLARE_HOOK(FInAimCone_Vector, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, FInAimCone_Vector, bool, (const Vector &vecSpot), (vecSpot));

SH_DECL_MANUALHOOK1(Event_Gibbed, 0, 0, 0, bool, const CTakeDamageInfo &);
DECLARE_HOOK(Event_Gibbed, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, Event_Gibbed, bool, (const CTakeDamageInfo &info), (info));

SH_DECL_MANUALHOOK2(Weapon_TranslateActivity, 0, 0, 0, Activity, Activity , bool *);
DECLARE_HOOK(Weapon_TranslateActivity, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, Weapon_TranslateActivity, Activity, (Activity baseAct, bool *pRequired), (baseAct, pRequired));

SH_DECL_MANUALHOOK1(NPC_TranslateActivity, 0, 0, 0, Activity, Activity);
DECLARE_HOOK(NPC_TranslateActivity, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, NPC_TranslateActivity, Activity, (Activity eNewActivity), (eNewActivity));

SH_DECL_MANUALHOOK1(Weapon_CanUse, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(Weapon_CanUse, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, Weapon_CanUse, bool, (CBaseEntity *pWeapon), (pWeapon));

SH_DECL_MANUALHOOK0_void(DoMuzzleFlash, 0, 0, 0);
DECLARE_HOOK(DoMuzzleFlash, CCombatCharacter);
DECLARE_DEFAULTHANDLER_void(CCombatCharacter, DoMuzzleFlash, (), ());

SH_DECL_MANUALHOOK1_void(OnKilledNPC, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(OnKilledNPC, CCombatCharacter);
DECLARE_DEFAULTHANDLER_void(CCombatCharacter, OnKilledNPC, (CBaseEntity *pKilled), (pKilled));

SH_DECL_MANUALHOOK2_void(OnPlayerKilledOther, 0, 0, 0, CBaseEntity *, const CTakeDamageInfo &);
DECLARE_HOOK(OnPlayerKilledOther, CCombatCharacter);
DECLARE_DEFAULTHANDLER_void(CCombatCharacter, OnPlayerKilledOther, ( CBaseEntity *pVictim, const CTakeDamageInfo &info ), (pVictim, info)) ;

SH_DECL_MANUALHOOK1(Weapon_CanSwitchTo, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(Weapon_CanSwitchTo, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, Weapon_CanSwitchTo, bool, (CBaseEntity *pWeapon), (pWeapon));

SH_DECL_MANUALHOOK3(GiveAmmo, 0, 0, 0, int, int, int, bool);
DECLARE_HOOK(GiveAmmo, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, GiveAmmo, int, (int iCount, int iAmmoIndex, bool bSuppressSound), (iCount, iAmmoIndex, bSuppressSound));

SH_DECL_MANUALHOOK0(GetPhysicsImpactDamageTable, 0, 0, 0, const impactdamagetable_t	&);
DECLARE_HOOK(GetPhysicsImpactDamageTable, CCombatCharacter);
DECLARE_DEFAULTHANDLER_REFERENCE(CCombatCharacter, GetPhysicsImpactDamageTable, const impactdamagetable_t	&, (), ());

SH_DECL_MANUALHOOK1(GetAmmoCount, 0, 0, 0, int, int);
DECLARE_HOOK(GetAmmoCount, CCombatCharacter);
DECLARE_DEFAULTHANDLER(CCombatCharacter, GetAmmoCount, int, (int iAmmoIndex) const, (iAmmoIndex));

SH_DECL_MANUALHOOK2_void(RemoveAmmo, 0, 0, 0, int, int);
DECLARE_HOOK(RemoveAmmo, CCombatCharacter);
DECLARE_DEFAULTHANDLER_void(CCombatCharacter, RemoveAmmo, (int iCount, int iAmmoIndex), (iCount, iAmmoIndex));



// Sendprops
DEFINE_PROP(m_flNextAttack, CCombatCharacter);
DEFINE_PROP(m_hMyWeapons, CCombatCharacter);
DEFINE_PROP(m_hActiveWeapon, CCombatCharacter);


//Datamaps
DEFINE_PROP(m_eHull, CCombatCharacter);
DEFINE_PROP(m_bloodColor, CCombatCharacter);
DEFINE_PROP(m_flFieldOfView, CCombatCharacter);
DEFINE_PROP(m_impactEnergyScale, CCombatCharacter);
DEFINE_PROP(m_HackedGunPos, CCombatCharacter);
DEFINE_PROP(m_RelationshipString, CCombatCharacter);
DEFINE_PROP(m_iAmmo, CCombatCharacter);
DEFINE_PROP(m_bPreventWeaponPickup, CCombatCharacter);
DEFINE_PROP(m_flDamageAccumulator, CCombatCharacter);
DEFINE_PROP(m_LastHitGroup, CCombatCharacter);





//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pHandleEntity - 
//			contentsMask - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTraceFilterMelee::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
		return false;

	if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
		return false;

	// Don't test if the game code tells us we should ignore this collision...
	CEntity *pEntity = CE_EntityFromEntityHandle( pHandleEntity );
	
	if ( pEntity )
	{
		if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
			return false;
		
		if ( !g_helpfunc.GameRules_ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
			return false;

		if ( pEntity->m_takedamage == DAMAGE_NO )
			return false;

		// FIXME: Do not translate this to the driver because the driver only accepts damage from the vehicle
		// Translate the vehicle into its driver for damage
		/*
		if ( pEntity->GetServerVehicle() != NULL )
		{
			CBaseEntity *pDriver = pEntity->GetServerVehicle()->GetPassenger();

			if ( pDriver != NULL )
			{
				pEntity = pDriver;
			}
		}
		*/
		
		CEntity *attacker = CEntity::Instance(m_dmgInfo->GetAttacker());
		Vector	attackDir = pEntity->WorldSpaceCenter() - attacker->WorldSpaceCenter();
		VectorNormalize( attackDir );

		CTakeDamageInfo info = (*m_dmgInfo);				
		CalculateMeleeDamageForce( &info, attackDir, attacker->WorldSpaceCenter(), m_flForceScale );

		CCombatCharacter *pBCC = attacker->MyCombatCharacterPointer();
		CCombatCharacter *pVictimBCC = pEntity->MyCombatCharacterPointer();

		// Only do these comparisons between NPCs
		if ( pBCC && pVictimBCC )
		{
			// Can only damage other NPCs that we hate
			if ( m_bDamageAnyNPC || pBCC->IRelationType( pEntity->BaseEntity() ) == D_HT )
			{
				if ( info.GetDamage() )
				{
					pEntity->TakeDamage( info );
				}
				
				// Put a combat sound in
				g_helpfunc.CSoundEnt_InsertSound( SOUND_COMBAT, info.GetDamagePosition(), 200, 0.2f, info.GetAttacker() );

				m_pHit = pEntity;
				return true;
			}
		}
		else
		{
			m_pHit = pEntity;

			// Make sure if the player is holding this, he drops it
			Pickup_ForcePlayerToDropThisObject( pEntity );

			// Otherwise just damage passive objects in our way
			if ( info.GetDamage() )
			{
				pEntity->TakeDamage( info );
			}
		}
	}

	return false;
}





extern ConVar *ai_use_visibility_cache;
#define ShouldUseVisibilityCache() ai_use_visibility_cache->GetBool()


void CCombatCharacter::SetBloodColor(int nBloodColor)
{
	m_bloodColor = nBloodColor;
}

IServerVehicle *CCombatCharacter::GetVehicle()
{
	return g_helpfunc.GetVehicle(BaseEntity());
}

Vector CCombatCharacter::CalcDamageForceVector( const CTakeDamageInfo &info )
{
	// Already have a damage force in the data, use that.
	bool bNoPhysicsForceDamage = g_helpfunc.GameRules_Damage_NoPhysicsForce( info.GetDamageType() );
	if ( info.GetDamageForce() != vec3_origin || bNoPhysicsForceDamage )
	{
		if( info.GetDamageType() & DMG_BLAST )
		{
			// Fudge blast forces a little bit, so that each
			// victim gets a slightly different trajectory. 
			// This simulates features that usually vary from
			// person-to-person variables such as bodyweight,
			// which are all indentical for characters using the same model.
			float scale = enginerandom->RandomFloat( 0.85, 1.15 );
			Vector force = info.GetDamageForce();
			force.x *= scale;
			force.y *= scale;
			// Try to always exaggerate the upward force because we've got pretty harsh gravity
			force.z *= (force.z > 0) ? 1.15 : scale;
			return force;
		}

		return info.GetDamageForce();
	}

	CBaseEntity *_pForce = info.GetInflictor();
	if ( !_pForce )
	{
		_pForce = info.GetAttacker();
	}

	CEntity *pForce = CEntity::Instance(_pForce);

	if ( pForce )
	{
		// Calculate an impulse large enough to push a 75kg man 4 in/sec per point of damage
		float forceScale = info.GetDamage() * 75 * 4;

		Vector forceVector;
		// If the damage is a blast, point the force vector higher than usual, this gives 
		// the ragdolls a bodacious "really got blowed up" look.
		if( info.GetDamageType() & DMG_BLAST )
		{
			// exaggerate the force from explosions a little (37.5%)
			forceVector = (GetLocalOrigin() + Vector(0, 0, WorldAlignSize().z) ) - pForce->GetLocalOrigin();
			VectorNormalize(forceVector);
			forceVector *= 1.375f;
		}
		else
		{
			// taking damage from self?  Take a little enginerandom force, but still try to collapse on the spot.
			if ( this == pForce )
			{
				forceVector.x = enginerandom->RandomFloat( -1.0f, 1.0f );
				forceVector.y = enginerandom->RandomFloat( -1.0f, 1.0f );
				forceVector.z = 0.0;
				forceScale = enginerandom->RandomFloat( 1000.0f, 2000.0f );
			}
			else
			{
				// UNDONE: Collision forces are baked in to CTakeDamageInfo now
				// UNDONE: Is this MOVETYPE_VPHYSICS code still necessary?
				if ( pForce->GetMoveType() == MOVETYPE_VPHYSICS )
				{
					// killed by a physics object
					IPhysicsObject *pPhysics = VPhysicsGetObject();
					if ( !pPhysics )
					{
						pPhysics = pForce->VPhysicsGetObject();
					}
					pPhysics->GetVelocity( &forceVector, NULL );
					forceScale = pPhysics->GetMass();
				}
				else
				{
					forceVector = GetLocalOrigin() - pForce->GetLocalOrigin();
					VectorNormalize(forceVector);
				}
			}
		}
		return forceVector * forceScale;
	}
	return vec3_origin;
}


void CCombatCharacter::AddAmmo( int iAmmoIndex , int iAmmount)
{
	*(int *)(((uint8_t *)(BaseEntity())) + m_iAmmo.offset + (iAmmoIndex*4)) += iAmmount;
}

void CCombatCharacter::SetAmmo(int nAmmoIndex, int iAmmount)
{
	*(int *)(((uint8_t *)(BaseEntity())) + m_iAmmo.offset + (nAmmoIndex*4)) = iAmmount;
}

CCombatWeapon *CCombatCharacter::GetWeapon( int i ) const
{
	CBaseHandle &hndl = *(CBaseHandle *)(((uint8_t *)(BaseEntity())) + m_hMyWeapons.offset + (i*4));
	return (CCombatWeapon *)CEntity::Instance(hndl);
}

CCombatWeapon *CCombatCharacter::GetActiveWeapon() const
{
	return m_hActiveWeapon.ptr->Get();
}

void CCombatCharacter::ThrowDirForWeaponStrip( CCombatWeapon *pWeapon, const Vector &vecForward, Vector *pVecThrowDir )
{
	// Nowhere in particular; just drop it.
	VMatrix zRot;
	MatrixBuildRotateZ( zRot, enginerandom->RandomFloat( -60.0f, 60.0f ) );

	Vector vecThrow;
	Vector3DMultiply( zRot, vecForward, *pVecThrowDir );

	pVecThrowDir->z = enginerandom->RandomFloat( -0.5f, 0.5f );
	VectorNormalize( *pVecThrowDir );
}


void CCombatCharacter::DropWeaponForWeaponStrip( CCombatWeapon *pWeapon, 
	const Vector &vecForward, const QAngle &vecAngles, float flDiameter )
{
	Vector vecOrigin;
	CollisionProp()->RandomPointInBounds( Vector( 0.5f, 0.5f, 0.5f ), Vector( 0.5f, 0.5f, 1.0f ), &vecOrigin );

	// Nowhere in particular; just drop it.
	Vector vecThrow;
	ThrowDirForWeaponStrip( pWeapon, vecForward, &vecThrow );

	Vector vecOffsetOrigin;
	VectorMA( vecOrigin, flDiameter, vecThrow, vecOffsetOrigin );

	trace_t	tr;
	UTIL_TraceLine( vecOrigin, vecOffsetOrigin, MASK_SOLID_BRUSHONLY, BaseEntity(), COLLISION_GROUP_NONE, &tr );
		
	if ( tr.startsolid || tr.allsolid || ( tr.fraction < 1.0f && tr.m_pEnt != pWeapon->BaseEntity() ) )
	{
		//FIXME: Throw towards a known safe spot?
		vecThrow.Negate();
		VectorMA( vecOrigin, flDiameter, vecThrow, vecOffsetOrigin );
	}

	vecThrow *= enginerandom->RandomFloat( 400.0f, 600.0f );

	pWeapon->SetAbsOrigin( vecOrigin );
	pWeapon->SetAbsAngles( vecAngles );
	pWeapon->Drop( vecThrow );
	pWeapon->SetRemoveable( false );
	Weapon_Detach( pWeapon );
}

bool CCombatCharacter::Weapon_Detach( CCombatWeapon *pWeapon )
{
	CCombatWeapon *current = GetActiveWeapon();
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if ( pWeapon == GetWeapon(i) )
		{
			CBaseHandle &hndl = *(CBaseHandle *)(((uint8_t *)(BaseEntity())) + m_hMyWeapons.offset + (i*4));
			hndl.Set(NULL);
			pWeapon->SetOwner( NULL );

			if ( pWeapon == current )
				ClearActiveWeapon();
			return true;
		}
	}

	return false;
}

void CCombatCharacter::SetActiveWeapon( CCombatWeapon *pNewWeapon )
{
	CBaseEntity *new_cbase = (pNewWeapon)?pNewWeapon->BaseEntity():NULL;
	CCombatWeapon *pOldWeapon = GetActiveWeapon();
	CBaseEntity *old_base = (pOldWeapon)?pOldWeapon->BaseEntity():NULL;

	if ( new_cbase != old_base )
	{
		m_hActiveWeapon->Set(new_cbase);
		OnChangeActiveWeapon( old_base, new_cbase );
	}
}


void CCombatCharacter::Weapon_DropAll( bool bDisallowWeaponPickup )
{
	if ( GetFlags() & FL_NPC )
	{
		for (int i=0; i<MAX_WEAPONS; ++i) 
		{
			CCombatWeapon *pWeapon = GetWeapon(i);
			if (!pWeapon)
				continue;

			Weapon_Drop( pWeapon->BaseEntity() );
		}
		return;
	}

	QAngle gunAngles;
	VectorAngles( BodyDirection2D(), gunAngles );

	Vector vecForward;
	AngleVectors( gunAngles, &vecForward, NULL, NULL );

	float flDiameter = sqrt( CollisionProp_Actual()->OBBSize().x * CollisionProp_Actual()->OBBSize().x +
		CollisionProp_Actual()->OBBSize().y * CollisionProp_Actual()->OBBSize().y );


	CCombatWeapon *pActiveWeapon = GetActiveWeapon();
	for (int i=0; i<MAX_WEAPONS; ++i) 
	{
		CCombatWeapon *pWeapon = GetWeapon(i);
		if (!pWeapon)
			continue;

		// Have to drop this after we've dropped everything else, so autoswitch doesn't happen
		if ( pWeapon == pActiveWeapon )
			continue;

		DropWeaponForWeaponStrip( pWeapon, vecForward, gunAngles, flDiameter );

		// HACK: This hack is required to allow weapons to be disintegrated
		// in the citadel weapon-strip scene
		// Make them not pick-uppable again. This also has the effect of allowing weapons
		// to collide with triggers. 
		if ( bDisallowWeaponPickup )
		{
			pWeapon->RemoveSolidFlags( FSOLID_TRIGGER );
			
			IPhysicsObject *pObj = pWeapon->VPhysicsGetObject();
			
			if ( pObj != NULL )
			{	
				pObj->SetGameFlags( FVPHYSICS_NO_PLAYER_PICKUP );
			}
		}
	}

	// Drop the active weapon normally...
	if ( pActiveWeapon )
	{
		// Nowhere in particular; just drop it.
		Vector vecThrow;
		ThrowDirForWeaponStrip( pActiveWeapon, vecForward, &vecThrow );

		// Throw a little more vigorously; it starts closer to the player
		vecThrow *= enginerandom->RandomFloat( 800.0f, 1000.0f );

		Weapon_Drop( pActiveWeapon->BaseEntity(), NULL, &vecThrow );
		pActiveWeapon->SetRemoveable( false );

		// HACK: This hack is required to allow weapons to be disintegrated
		// in the citadel weapon-strip scene
		// Make them not pick-uppable again. This also has the effect of allowing weapons
		// to collide with triggers. 
		if ( bDisallowWeaponPickup )
		{
			pActiveWeapon->RemoveSolidFlags( FSOLID_TRIGGER );
		}
	}

}

bool CCombatCharacter::HaveThisWeaponType(CCombatWeapon *pWeapon)
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CCombatWeapon *weapon = GetWeapon(i);
		if ( weapon )
		{
			if(weapon->ClassMatches(pWeapon->GetClassname()))
				return true;
		}
	}
	return false;
}

CCombatWeapon *CCombatCharacter::GetThisWeaponType(CCombatWeapon *pWeapon)
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CCombatWeapon *weapon = GetWeapon(i);
		if ( weapon )
		{
			if(weapon->ClassMatches(pWeapon->GetClassname()))
				return weapon;
		}
	}
	return NULL;
}

int CCombatCharacter::FAKE_OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	Vector vecDir = vec3_origin;
	CEntity *inflictor = CEntity::Instance(info.GetInflictor());
	if (inflictor)
	{
		vecDir = inflictor->WorldSpaceCenter() - Vector ( 0, 0, 10 ) - WorldSpaceCenter();
		VectorNormalize(vecDir);
	}
	*(g_vecAttackDir) = vecDir;

	//!!!LATER - make armor consideration here!
	// do the damage
	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
		// Separate the fractional amount of damage from the whole
		float flFractionalDamage = info.GetDamage() - floor( info.GetDamage() );
		float flIntegerDamage = info.GetDamage() - flFractionalDamage;

		// Add fractional damage to the accumulator
		m_flDamageAccumulator += flFractionalDamage;

		// If the accumulator is holding a full point of damage, move that point
		// of damage into the damage we're about to inflict.
		if( m_flDamageAccumulator >= 1.0 )
		{
			flIntegerDamage += 1.0;
			m_flDamageAccumulator -= 1.0;
		}

		if ( flIntegerDamage <= 0 )
			return 0;

		m_iHealth -= flIntegerDamage;
	}

	return 1;
}

CCombatWeapon* CCombatCharacter::Weapon_OwnsThisType( const char *pszWeapon, int iSubType ) const
{
	// Check for duplicates
	for (int i=0;i<MAX_WEAPONS;i++) 
	{
		CCombatWeapon *weapon = GetWeapon(i);
		if ( weapon && FClassnameIs( weapon, pszWeapon ) )
		{
			// Make sure it matches the subtype
			if ( weapon->GetSubType() == iSubType )
				return weapon;
		}
	}
	return NULL;
}

bool CCombatCharacter::SwitchToNextBestWeapon(CCombatWeapon *pCurrent)
{
	CCombatWeapon *pNewWeapon = g_helpfunc.GameRules_GetNextBestWeapon(BaseEntity(), (pCurrent)?pCurrent->BaseEntity():NULL);
	
	if ( ( pNewWeapon != NULL ) && ( pNewWeapon != pCurrent ) )
	{
		return Weapon_Switch( pNewWeapon->BaseEntity() );
	}

	return false;
}

void CCombatCharacter::Weapon_SetActivity( Activity newActivity, float duration )
{
	CCombatWeapon *current = GetActiveWeapon();
	if ( current )
	{
		current->Weapon_SetActivity( newActivity, duration );
	}
}

CCombatWeapon *CCombatCharacter::Weapon_Create( const char *pWeaponName )
{
	CCombatWeapon *pWeapon = static_cast<CCombatWeapon *>( Create( pWeaponName, GetLocalOrigin(), GetLocalAngles(), BaseEntity() ) );
	return pWeapon;
}

CEntity *CCombatCharacter::FindHealthItem( const Vector &vecPosition, const Vector &range )
{
	CBaseEntity *list[1024];
	int count = UTIL_EntitiesInBox( list, 1024, vecPosition - range, vecPosition + range, 0 );

	for ( int i = 0; i < count; i++ )
	{
		CEntity *cent = CEntity::Instance(list[i]);
		ICItem *pItem = dynamic_cast<ICItem *>(cent);

		if( pItem )
		{
			// Healthkits and healthvials
			if( cent->ClassMatches( "item_health*" ) && FVisible_Entity( list[ i ] ) )
			{
				return cent;
			}
		}
	}

	return NULL;
}

CEntity	*CCombatCharacter::Weapon_FindUsable( const Vector &range )
{
	bool bConservative = false;

#ifdef HL2_DLL
	if( hl2_episodic->GetBool() && !GetActiveWeapon() )
	{
		// Unarmed citizens are conservative in their weapon finding
		if ( Classify() != CLASS_PLAYER_ALLY_VITAL )
		{
			bConservative = true;
		}
	}
#endif

	CCombatWeapon *weaponList[64];
	CCombatWeapon *pBestWeapon = NULL;

	Vector mins = GetAbsOrigin() - range;
	Vector maxs = GetAbsOrigin() + range;
	int listCount = CCombatWeapon::GetAvailableWeaponsInBox( weaponList, ARRAYSIZE(weaponList), mins, maxs );

	float fBestDist = 1e6;

	for ( int i = 0; i < listCount; i++ )
	{
		// Make sure not moving (ie flying through the air)
		Vector velocity;

		CCombatWeapon *pWeapon = weaponList[i];
		Assert(pWeapon);
		pWeapon->GetVelocity( &velocity, NULL );

		if ( pWeapon->CanBePickedUpByNPCs() == false )
			continue;

		if ( velocity.LengthSqr() > 1 || !Weapon_CanUse(pWeapon->BaseEntity()) )
			continue;

		if ( pWeapon->IsLocked(this) )
			continue;

		if ( GetActiveWeapon() )
		{
			// Already armed. Would picking up this weapon improve my situation?
			if( *(GetActiveWeapon()->m_iClassname) == *(pWeapon->m_iClassname) )
			{
				// No, I'm already using this type of weapon.
				continue;
			}

			if( FClassnameIs( pWeapon, WEAPON_PISTOL_REPLACE_NAME ) )
			{
				// No, it's a pistol.
				continue;
			}
		}

		float fCurDist = (pWeapon->GetLocalOrigin() - GetLocalOrigin()).Length();

		// Give any reserved weapon a bonus
		if( pWeapon->HasSpawnFlags( SF_WEAPON_NO_PLAYER_PICKUP ) )
		{
			fCurDist *= 0.5f;
		}

		if ( pBestWeapon )
		{
			// UNDONE: Better heuristic needed here
			//			Need to pick by power of weapons
			//			Don't want to pick a weapon right next to a NPC!

			// Give the AR2 a bonus to be selected by making it seem closer.
			if( FClassnameIs( pWeapon, WEAPON_AR2_REPLACE_NAME ) )
			{
				fCurDist *= 0.5;
			}

			// choose the last range attack weapon you find or the first available other weapon
			if ( ! (pWeapon->Weapon_CapabilitiesGet() & bits_CAP_RANGE_ATTACK_GROUP) )
			{
				continue;
			}
			else if (fCurDist > fBestDist ) 
			{
				continue;
			}
		}

		if( Weapon_IsOnGround(pWeapon) )
		{
			// Weapon appears to be lying on the ground. Make sure this weapon is reachable
			// by tracing out a human sized hull just above the weapon.  If not, reject
			trace_t tr;

			Vector	vAboveWeapon = pWeapon->GetAbsOrigin();
			UTIL_TraceEntity( this, vAboveWeapon, vAboveWeapon + Vector( 0, 0, 1 ), MASK_SOLID, pWeapon->BaseEntity(), COLLISION_GROUP_NONE, &tr );

			if ( tr.startsolid || (tr.fraction < 1.0) )
				continue;
		}
		else if( bConservative )
		{
			// Skip it.
			continue;
		}

		if( FVisible_Entity(pWeapon->BaseEntity()) )
		{
			fBestDist   = fCurDist;
			pBestWeapon = pWeapon;
		}
	}

	if( pBestWeapon )
	{
		// Lock this weapon for my exclusive use. Lock it for just a couple of seconds because my AI 
		// might not actually be able to go pick it up right now.
		pBestWeapon->Lock( 2.0, this );
	}
	return pBestWeapon;

}


bool CCombatCharacter::Weapon_IsOnGround( CCombatWeapon *pWeapon )
{
	if( pWeapon->IsConstrained() )
	{
		// Constrained to a rack.
		return false;
	}

	if( fabs(pWeapon->WorldSpaceCenter().z - GetAbsOrigin().z) >= 12.0f )
	{
		return false;
	}

	return true;
}

CCombatWeapon *CCombatCharacter::Weapon_GetWpnForAmmo( int iAmmoIndex )
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CCombatWeapon *weapon = GetWeapon( i );
		if ( !weapon )
			continue;

		if ( weapon->GetPrimaryAmmoType() == iAmmoIndex )
			return weapon;
		if ( weapon->GetSecondaryAmmoType() == iAmmoIndex )
			return weapon;
	}

	return NULL;
}



void CCombatCharacter::AllocateDefaultRelationships()
{
	if(m_DefaultRelationship == NULL)
		return;

	if (!*m_DefaultRelationship)
	{
		*m_DefaultRelationship = new Relationship_t*[NUM_AI_CLASSES];

		for (int i=0; i<NUM_AI_CLASSES; ++i)
		{
			// Be default all relationships are neutral of priority zero
			Relationship_t *ptr = new Relationship_t[NUM_AI_CLASSES];
			memset(ptr, 0, sizeof(Relationship_t[NUM_AI_CLASSES]));
			(*m_DefaultRelationship)[i] = ptr;
		}
	}
}

void CCombatCharacter::Shutdown()
{
	if (m_DefaultRelationship == NULL || !*m_DefaultRelationship )
		return;

	for ( int i=0; i<NUM_AI_CLASSES; ++i )
	{
		delete[] (*m_DefaultRelationship)[i];
	}

	delete[] *m_DefaultRelationship;
	*m_DefaultRelationship = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Return an interaction ID (so we have no collisions)
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CCombatCharacter::SetDefaultRelationship(Class_T nClass, Class_T nClassTarget, Disposition_t nDisposition, int nPriority)
{
	if(m_DefaultRelationship == NULL)
		return;

	if (*m_DefaultRelationship )
	{
		(*m_DefaultRelationship)[nClass][nClassTarget].disposition	= nDisposition;
		(*m_DefaultRelationship)[nClass][nClassTarget].priority	= nPriority;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return an interaction ID (so we have no collisions)
//-----------------------------------------------------------------------------
int	CCombatCharacter::GetInteractionID(void)
{
	*(m_lastInteraction) += 1;
	return *(m_lastInteraction);
}





void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	// NOTE: I did this this way so I wouldn't have to change a whole bunch of
	// code unnecessarily. We need TF2 specific rules for RadiusDamage, so I moved
	// the implementation of radius damage into gamerules. All existing code calls
	// this method, which calls the game rules method
	g_helpfunc.GameRules_RadiusDamage( info, vecSrc, flRadius, iClassIgnore, pEntityIgnore );

	// Let the world know if this was an explosion.
	if( info.GetDamageType() & DMG_BLAST )
	{
		// Even the tiniest explosion gets attention. Don't let the radius
		// be less than 128 units.
		float soundRadius = MAX( 128.0f, flRadius * 1.5 );

		g_helpfunc.CSoundEnt_InsertSound( SOUND_COMBAT | SOUND_CONTEXT_EXPLOSION, vecSrc, (int)soundRadius, 0.25, info.GetInflictor() );
	}
}
