#ifndef _INCLUDE_SOURCEMOD_EXTENSION_SIGN_FUNC_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_SIGN_FUNC_H_

#ifdef _WIN32
#pragma once
#endif

#include "extension.h"
#include "ai_activity.h"
#include "eventlist.h"
#include "CSoundent.h"


class VEmptyClass {};

class CEntity;
class CTakeDamageInfo;
struct AIMoveTrace_t;
class IServerVehicle;
enum Navigation_t;
class CAI_Hint;
class CAI_NPC;
class CBoneCache;
class CHintCriteria;
class CEffectData;
class IEntityFindFilter;
class CAmmoDef;
struct gamevcollisionevent_t;
struct breakablepropparams_t;
enum FlankType_t;
class IMapEntityFilter;
class CEventQueue;
class IEntityListener;
struct mstudioseqdesc_t;
class CCombatWeapon;
class CViewVectors;
struct AI_NavGoal_t;


class HelperFunction
{
public:
	HelperFunction();

	bool Initialize();
	bool FindAllValveGameSystem();
	void Shutdown();

	void LevelInitPreEntity();
	void LevelInitPostEntity();
	void LevelShutdownPreEntity();
	void LevelShutdownPostEntity();

public:
	void HookGameRules();
	void UnHookGameRules();
	int SelectWeightedSequence(void *pstudiohdr, int activity, int curSequence = -1);
	Activity ActivityList_RegisterPrivateActivity( const char *pszActivityName );
	Animevent EventList_RegisterPrivateEvent( const char *pszEventName );
	CBaseEntity *NPCPhysics_CreateSolver(CBaseEntity *pNPC, CBaseEntity *c, bool disableCollisions, float separationDuration);
	HSOUNDSCRIPTHANDLE PrecacheScriptSound(const char *soundname);
	void EmitSoundByHandle( IRecipientFilter& filter, int entindex, const EmitSound_t & ep, HSOUNDSCRIPTHANDLE& handle );
	CEntity *CreateRagGib( const char *szModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecForce, float flFadeTime, bool bShouldIgnite );
	void VerifySequenceIndex(void *ptr);
	void DispatchEffect( const char *pName, const CEffectData &data );
	CAmmoDef *GetAmmoDef();
	CEntity *CreateNoSpawn( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner);
	void SetMinMaxSize(CBaseEntity *pEnt, const Vector &vecMin, const Vector &vecMax);
	float CalculateDefaultPhysicsDamage( int index, gamevcollisionevent_t *pEvent, float energyScale, bool allowStaticDamage, int &damageType, string_t iszDamageTableName = NULL_STRING, bool bDamageFromHeldObjects = false );
	void PhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info, gamevcollisionevent_t &event, int hurtIndex );
	void PropBreakableCreateAll( int modelindex, IPhysicsObject *pPhysics, const breakablepropparams_t &params, CBaseEntity *pEntity, int iPrecomputedBreakableCount, bool bIgnoreGibLimit, bool defaultLocation );
	int PropBreakablePrecacheAll( string_t modelName );
	void CSoundEnt_InsertSound( int iType, const Vector &vecOrigin, int iVolume, float flDuration, CBaseEntity *pOwner = NULL, int soundChannelIndex = SOUNDENT_CHANNEL_UNSPECIFIED, CBaseEntity *pSoundTarget = NULL );
	CEntity *CreateServerRagdoll( CBaseEntity *pAnimating, int forceBone, const CTakeDamageInfo &info, int collisionGroup, bool bUseLRURetirement = false);
	int	PrecacheModel( const char *name, bool bPreload );
	const char *MapEntity_ParseEntity(CEntity *&pEntity, const char *pEntData, IMapEntityFilter *pFilter);
	void UTIL_PrecacheOther( const char *szClassname, const char *modelName = NULL);
	void SetEventIndexForSequence( mstudioseqdesc_t &seqdesc );
	string_t AllocPooledString( const char * pszValue );
	string_t FindPooledString( const char * pszValue );
	void PrecacheInstancedScene( char const *pszScene );
	const char *ActivityList_NameForIndex( int activityIndex );
	void SetActivityForSequence( CStudioHdr *pstudiohdr, int i );
	int ActivityList_IndexForName( const char *pszActivityName );
	CEntity *CreateServerRagdollAttached( CBaseEntity *pAnimating, const Vector &vecForce, int forceBone, int collisionGroup, IPhysicsObject *pAttached, CBaseEntity *pParentEntity, int boneAttach, const Vector &originAttached, int parentBoneAttach, const Vector &boneOrigin );


public: //CAI_HintManager
	CEntity *CAI_HintManager_FindHint(CBaseEntity *pNPC, const Vector &position, const CHintCriteria &hintCriteria );
	CEntity *CAI_HintManager_FindHintRandom(CBaseEntity *pNPC, const Vector &position, const CHintCriteria &hintCriteria );

public: // entity
	void SetNextThink(CBaseEntity *pEntity, float thinkTime, const char *szContext);
	void SimThink_EntityChanged(CBaseEntity *pEntity);
	void SetGroundEntity(CBaseEntity *pEntity, CBaseEntity *ground);
	void SetAbsVelocity(CBaseEntity *pEntity, const Vector &vecAbsVelocity);
	int DispatchUpdateTransmitState(CBaseEntity *pEntity);
	void SetAbsAngles(CBaseEntity *pEntity, const QAngle& absAngles);
	IServerVehicle *GetServerVehicle(CBaseEntity *pEntity);
	IServerVehicle *GetVehicle(CBaseEntity *pEntity);
	void EmitSound(CBaseEntity *pEntity, const char *soundname, float soundtime, float *duration);
	void EmitSound(IRecipientFilter& filter, int iEntIndex, const EmitSound_t & params);
	void EmitSound(IRecipientFilter& filter, int iEntIndex, const char *soundname, const Vector *pOrigin = NULL, float soundtime = 0.0f, float *duration = NULL );
	void CBaseEntity_Use(CBaseEntity *pEntity, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	bool CBaseEntity_FVisible_Entity(CBaseEntity *the_pEntity, CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker);
	void CalcAbsolutePosition(CBaseEntity *pEntity);
	void PhysicsMarkEntitiesAsTouching( CBaseEntity *pEntity, CBaseEntity *other, trace_t &trace );
	void *GetDataObject( CBaseEntity *pEntity, int type );
	void CheckHasGamePhysicsSimulation(CBaseEntity *pEntity);
	void InvalidatePhysicsRecursive( CBaseEntity *pEntity, int nChangeFlags );
	void PhysicsPushEntity( CBaseEntity *pEntity, const Vector& push, trace_t *pTrace );


public: // collision
	void SetSolid(void *collision_ptr, SolidType_t val);
	void SetSolidFlags(void *collision_ptr, int flags);
	void MarkPartitionHandleDirty(void *collision_ptr);

public: // gamerules
	bool GameRules_ShouldCollide(int collisionGroup0, int collisionGroup1);
	bool GameRules_ShouldCollide_Hook(int collisionGroup0, int collisionGroup1);
	bool CHalfLife2_ShouldCollide( int collisionGroup0, int collisionGroup1);
	bool CGameRules_ShouldCollide( int collisionGroup0, int collisionGroup1);
	bool GameRules_Damage_NoPhysicsForce(int iDmgType);
	bool GameRules_IsSkillLevel(int iLevel);
	void GameRules_RadiusDamage_Hook(const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore);
	void GameRules_RadiusDamage(const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore);
	CCombatWeapon *GameRules_GetNextBestWeapon(CBaseEntity *pPlayer, CBaseEntity *pCurrentWeapon);
	bool GameRules_FPlayerCanTakeDamage(CBaseEntity *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info);
	void GameRules_CleanUpMap();
	CViewVectors *GameRules_GetViewVectors();
	void CGameRules_RadiusDamage(const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore);
	
public:
	bool USE_HL2_RADIUS_DAMAGE;

public: // entlist
	void ReportEntityFlagsChanged(CBaseEntity *pEntity, unsigned int flagsOld, unsigned int flagsNow);
	
	CEntity *FindEntityByClassname(CBaseEntity *pStartEntity, const char *szName);
	CEntity *FindEntityByClassname(CEntity *pStartEntity, const char *szName);
	
	CEntity *FindEntityByName( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL, IEntityFindFilter *pFilter = NULL );
	CEntity *FindEntityByName( CBaseEntity *pStartEntity, string_t szName, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL, IEntityFindFilter *pFilter = NULL ) { return FindEntityByName( pStartEntity, STRING(szName), pSearchingEntity, pActivator, pCaller, pFilter); }
	
	CEntity *FindEntityByName( CEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL, IEntityFindFilter *pFilter = NULL );
	CEntity *FindEntityByName( CEntity *pStartEntity, string_t szName, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL, IEntityFindFilter *pFilter = NULL );

	CEntity *FindEntityInSphere( CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius );
	CEntity *FindEntityInSphere( CEntity *pStartEntity, const Vector &vecCenter, float flRadius );

	CEntity *FindEntityGeneric( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );

	CEntity *FindEntityGenericNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity = NULL, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );

	CBaseEntity *NextEnt(CBaseEntity *pCurrentEnt);
	CBaseEntity *FirstEnt();

	void AddListenerEntity( IEntityListener *pListener );
	void RemoveListenerEntity( IEntityListener *pListener );
	
	template< class T >
	T *NextEntByClass( T *start )
	{
		for ( CEntity *x = CEntity::Instance(NextEnt( (start)?start->BaseEntity():NULL )); x; x = CEntity::Instance(NextEnt( (x)?x->BaseEntity():NULL )) )
		{
			start = dynamic_cast<T*>( x );
			if ( start )
				return start;
		}
		return NULL;
	}


public: // basenpc
	void CAI_BaseNPC_Precache(CBaseEntity *pEntity);
	bool AutoMovement(CBaseEntity *pEntity, float flInterval, CBaseEntity *pTarget, AIMoveTrace_t *pTraceResult);
	void EndTaskOverlay(CBaseEntity *pEntity);
	void SetIdealActivity(CBaseEntity *pEntity, Activity NewActivity);
	bool HaveSequenceForActivity(CBaseEntity *pEntity, Activity activity);
	void TestPlayerPushing(CBaseEntity *pEntity, CBaseEntity *pPlayer);

public: // combatcharacter
	int CBaseCombatCharacter_OnTakeDamage(CBaseEntity *pEntity, const CTakeDamageInfo &info);
	void CBaseCombatCharacter_Event_Killed(CBaseEntity *pEntity, const CTakeDamageInfo &info );

public: // animate
	CBoneCache *GetBoneCache(void *ptr);

public:	// CCSPlayer
	void RoundRespawn(CBaseEntity *pEntity);

public: // CAI_MoveProbe
	bool ShouldBrushBeIgnored(void *ptr, CBaseEntity *pEntity);
	bool MoveLimit(void *ptr, Navigation_t navType, const Vector &vecStart, 
		const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, 
		float pctToCheckStandPositions, unsigned flags, AIMoveTrace_t* pTrace);
public:
	int CAI_TacticalServices_FindLosNode( void *ptr, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinThreatDist, float flMaxThreatDist, float flBlockTime, FlankType_t eFlankType, const Vector &vThreatFacing, float flFlankParam );
	int CAI_TacticalServices_FindCoverNode( void *ptr, const Vector &vNearPos, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist );

public:
	bool CAI_Navigator_UpdateGoalPos(void *ptr, const Vector &goalPos);
	bool CAI_Navigator_SetGoal( void *ptr, const AI_NavGoal_t &goal, unsigned flags );

public:
	void CBreakableProp_Break( void *ptr, CBaseEntity *pBreaker, const CTakeDamageInfo &info );

private:
	bool GameRules_FAllowNPCs();

private:
	bool CGameMovement_OnLadder( trace_t &trace );

public:
	void **my_g_pGameRules;
	void *g_SoundEmitterSystem;
};

extern HelperFunction g_helpfunc;
extern Vector *g_vecAttackDir;
extern CEventQueue *g_CEventQueue;

#endif
