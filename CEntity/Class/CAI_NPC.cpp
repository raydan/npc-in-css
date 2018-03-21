
#include "CAI_NPC.h"
#include "stringregistry.h"
#include "CPropDoor.h"
#include "CPlayer.h"
#include "in_buttons.h"
#include "CESoundEnt.h"
#include "shot_manipulator.h"
#include "npc_bullseye.h"
#include "CCombatWeapon.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CAI_Manager g_AI_Manager;

//-------------------------------------

CAI_Manager::CAI_Manager()
{
	m_AIs.EnsureCapacity( MAX_AIS );
}

//-------------------------------------

CAI_NPC **CAI_Manager::AccessAIs()
{
	if (m_AIs.Count())
		return &m_AIs[0];
	return NULL;
}

//-------------------------------------

int CAI_Manager::NumAIs()
{
	return m_AIs.Count();
}

//-------------------------------------

void CAI_Manager::AddAI( CAI_NPC *pAI )
{
	m_AIs.AddToTail( pAI );
}

//-------------------------------------

void CAI_Manager::RemoveAI( CAI_NPC *pAI )
{
	int i = m_AIs.Find( pAI );

	if ( i != -1 )
		m_AIs.FastRemove( i );
}

//-------------------------------------


extern ConVar *ai_strong_optimizations;
extern ConVar *sv_stepsize;

CAI_ClassScheduleIdSpace	CAI_NPC::gm_ClassScheduleIdSpace( true );
CAI_GlobalScheduleNamespace CAI_NPC::gm_SchedulingSymbols;
CAI_LocalIdSpace			CAI_NPC::gm_SquadSlotIdSpace( true );
CAI_GlobalNamespace			CAI_NPC::gm_SquadSlotNamespace;

CStringRegistry*		CAI_NPC::m_pActivitySR		= NULL;
int						*CAI_NPC::m_iNumActivities  = NULL;

CStringRegistry*		CAI_NPC::m_pEventSR			= NULL;
int						*CAI_NPC::m_iNumEvents		= NULL;

string_t CAI_NPC::gm_iszPlayerSquad = NULL_STRING;
VALVE_BASEPTR CAI_NPC::func_CallNPCThink = NULL;


DECLARE_DETOUR(SetHullSizeNormal, CAI_NPC);
DECLARE_DEFAULTHANDLER_DETOUR_void(CAI_NPC, SetHullSizeNormal, (bool force),(force));


SH_DECL_MANUALHOOK0(GetEnemies, 0, 0, 0, CEAI_Enemies *);
DECLARE_HOOK(GetEnemies, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetEnemies, CEAI_Enemies *, (), ());

SH_DECL_MANUALHOOK0_void(NPCInit, 0, 0, 0);
DECLARE_HOOK(NPCInit, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, NPCInit, (), ());

DECLARE_DETOUR(SetState, CAI_NPC);
DECLARE_DEFAULTHANDLER_DETOUR_void(CAI_NPC, SetState, (NPC_STATE State), (State));

SH_DECL_MANUALHOOK1_void(SetActivity, 0, 0, 0, Activity);
DECLARE_HOOK(SetActivity, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, SetActivity, (Activity NewActivity), (NewActivity));

DECLARE_DETOUR(SetSchedule_Int, CAI_NPC);
DECLARE_DEFAULTHANDLER_DETOUR(CAI_NPC, SetSchedule_Int, bool, (int localScheduleID), (localScheduleID));

SH_DECL_MANUALHOOK1_void(RunTask, 0, 0, 0, const Task_t *);
DECLARE_HOOK(RunTask, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, RunTask, (const Task_t *pTask), (pTask));

SH_DECL_MANUALHOOK1_void(OnChangeActivity, 0, 0, 0, Activity);
DECLARE_HOOK(OnChangeActivity, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnChangeActivity, (Activity eNewActivity), (eNewActivity));

SH_DECL_MANUALHOOK0(MaxYawSpeed, 0, 0, 0, float);
DECLARE_HOOK(MaxYawSpeed, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, MaxYawSpeed, float,(), ());


SH_DECL_MANUALHOOK0(GetClassScheduleIdSpace, 0, 0, 0, CAI_ClassScheduleIdSpace *);
DECLARE_HOOK(GetClassScheduleIdSpace, CAI_NPC);
//DECLARE_DEFAULTHANDLER(CAI_NPC,GetClassScheduleIdSpace, CAI_ClassScheduleIdSpace*, (), ());


CAI_ClassScheduleIdSpace *CAI_NPC::GetClassScheduleIdSpace()
{
	if(!m_bInGetClassScheduleIdSpace)
	{
		return SH_MCALL(BaseEntity(), GetClassScheduleIdSpace)();
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	if(thisptr != (void *)BaseEntity()) {
		RETURN_META_VALUE(MRES_SUPERCEDE, SH_MCALL(BaseEntity(), GetClassScheduleIdSpace)());
	}
	RETURN_META_VALUE(MRES_SUPERCEDE,(thisptr->*(__SoureceHook_FHM_GetRecallMFPGetClassScheduleIdSpace(thisptr)))());
}

CAI_ClassScheduleIdSpace *CAI_NPC::InternalGetClassScheduleIdSpace()
{
	SET_META_RESULT(MRES_SUPERCEDE);
	CAI_NPC *pEnt = (CAI_NPC *)(CEntity::Instance(META_IFACEPTR(CBaseEntity)));
	if (!pEnt)
	{
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	int index = pEnt->entindex_non_network();
	pEnt->m_bInGetClassScheduleIdSpace = true;
	CAI_ClassScheduleIdSpace *space = pEnt->GetClassScheduleIdSpace();
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInGetClassScheduleIdSpace = false;

	return space;
	//return &gm_ClassScheduleIdSpace;
}


SH_DECL_MANUALHOOK0_void(NPCThink, 0, 0, 0);
DECLARE_HOOK(NPCThink, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, NPCThink,(), ());

SH_DECL_MANUALHOOK1(CalcIdealYaw, 0, 0, 0, float, const Vector &);
DECLARE_HOOK(CalcIdealYaw, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CalcIdealYaw, float, (const Vector &vecTarget), (vecTarget));

SH_DECL_MANUALHOOK0_void(GatherConditions, 0, 0, 0);
DECLARE_HOOK(GatherConditions, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, GatherConditions, (), ());

SH_DECL_MANUALHOOK0_void(PrescheduleThink, 0, 0, 0);
DECLARE_HOOK(PrescheduleThink, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, PrescheduleThink, (), ());

SH_DECL_MANUALHOOK0_void(IdleSound, 0, 0, 0);
DECLARE_HOOK(IdleSound, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, IdleSound, (), ());

SH_DECL_MANUALHOOK0_void(AlertSound, 0, 0, 0);
DECLARE_HOOK(AlertSound, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, AlertSound, (), ());

SH_DECL_MANUALHOOK1_void(PainSound, 0, 0, 0, const CTakeDamageInfo &);
DECLARE_HOOK(PainSound, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, PainSound, (const CTakeDamageInfo &info), (info));

SH_DECL_MANUALHOOK1_void(DeathSound, 0, 0, 0, const CTakeDamageInfo &);
DECLARE_HOOK(DeathSound, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, DeathSound, (const CTakeDamageInfo &info), (info));

SH_DECL_MANUALHOOK0(BestEnemy, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(BestEnemy, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, BestEnemy, CBaseEntity *, (), ());

SH_DECL_MANUALHOOK1_void(TaskFail, 0, 0, 0, AI_TaskFailureCode_t);
DECLARE_HOOK(TaskFail, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, TaskFail, (AI_TaskFailureCode_t code), (code));

SH_DECL_MANUALHOOK1_void(StartTask, 0, 0, 0, const Task_t *);
DECLARE_HOOK(StartTask, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, StartTask, (const Task_t *pTask), (pTask));

SH_DECL_MANUALHOOK1(TranslateSchedule, 0, 0, 0, int, int);
DECLARE_HOOK(TranslateSchedule, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, TranslateSchedule, int, (int scheduleType), (scheduleType));

SH_DECL_MANUALHOOK0(SelectSchedule, 0, 0, 0, int);
DECLARE_HOOK(SelectSchedule, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, SelectSchedule, int, (), ());

SH_DECL_MANUALHOOK3(SelectFailSchedule, 0, 0, 0, int, int, int, AI_TaskFailureCode_t);
DECLARE_HOOK(SelectFailSchedule, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, SelectFailSchedule, int, (int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode), (failedSchedule, failedTask, taskFailCode));

SH_DECL_MANUALHOOK2_void(PlayerHasIlluminatedNPC, 0, 0, 0, CBaseEntity *, float);
DECLARE_HOOK(PlayerHasIlluminatedNPC, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, PlayerHasIlluminatedNPC, (CBaseEntity *pPlayer, float flDot), (pPlayer, flDot));

SH_DECL_MANUALHOOK2(QuerySeeEntity, 0, 0, 0, bool, CBaseEntity *, bool);
DECLARE_HOOK(QuerySeeEntity, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, QuerySeeEntity, bool, (CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC), (pEntity, bOnlyHateOrFearIfNPC));

SH_DECL_MANUALHOOK0(GetSoundInterests, 0, 0, 0, int);
DECLARE_HOOK(GetSoundInterests, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetSoundInterests, int, (), ());

SH_DECL_MANUALHOOK0_void(BuildScheduleTestBits, 0, 0, 0);
DECLARE_HOOK(BuildScheduleTestBits, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, BuildScheduleTestBits, (), ());


SH_DECL_MANUALHOOK1(GetSchedule, 0, 0, 0, CAI_Schedule *, int);
DECLARE_HOOK(GetSchedule, CAI_NPC);


CAI_Schedule *CAI_NPC::GetSchedule(int schedule)
{
	CAI_Schedule *ret = NULL;
	if (!GetClassScheduleIdSpace()->IsGlobalBaseSet())
	{
		ret = g_AI_SchedulesManager.GetScheduleFromID(SCHED_IDLE_STAND);
		return ret;
	}
	if ( AI_IdIsLocal( schedule ) )
	{
		schedule = GetClassScheduleIdSpace()->ScheduleLocalToGlobal(schedule);
	}
	
	ret = g_AI_SchedulesManager.GetScheduleFromID( schedule );
	return ret;

	/*if(!m_bInGetSchedule)
	{
		return SH_MCALL(BaseEntity(), GetSchedule)(schedule);
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	if(thisptr != (void *)BaseEntity()) {
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}
	RETURN_META_VALUE(MRES_SUPERCEDE,(thisptr->*(__SoureceHook_FHM_GetRecallMFPGetSchedule(thisptr)))(schedule));
*/
}

CAI_Schedule *CAI_NPC::InternalGetSchedule(int schedule)
{
	SET_META_RESULT(MRES_SUPERCEDE);
	
	CAI_NPC *pEnt = (CAI_NPC *)(CEntity::Instance(META_IFACEPTR(CBaseEntity)));
	if (!pEnt)
	{
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	int index = pEnt->entindex_non_network();
	pEnt->m_bInGetSchedule = true;
	CAI_Schedule *ret = pEnt->GetSchedule(schedule);
	/*CAI_Schedule *ret = NULL;

	if (!pEnt->GetClassScheduleIdSpace()->IsGlobalBaseSet())
	{
		ret = g_AI_SchedulesManager.GetScheduleFromID(SCHED_IDLE_STAND);
		if (pEnt == CEntity::Instance(index))
			pEnt->m_bInGetSchedule = false;
		return ret;
	}
	if ( AI_IdIsLocal( schedule ) )
	{
		schedule = pEnt->GetClassScheduleIdSpace()->ScheduleLocalToGlobal(schedule);
	}
	
	ret = g_AI_SchedulesManager.GetScheduleFromID( schedule );*/

	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInGetSchedule = false;
	return ret;
}


DECLARE_DETOUR(SetEnemy, CAI_NPC);
DECLARE_DEFAULTHANDLER_DETOUR_void(CAI_NPC, SetEnemy, (CBaseEntity *pEnemy, bool bSetCondNewEnemy), (pEnemy, bSetCondNewEnemy));

SH_DECL_MANUALHOOK3(UpdateEnemyMemory, 0, 0, 0, bool, CBaseEntity *, const Vector &, CBaseEntity *);
DECLARE_HOOK(UpdateEnemyMemory, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, UpdateEnemyMemory, bool, (CBaseEntity *pEnemy, const Vector &position, CBaseEntity *pInformer), (pEnemy, position, pInformer));

SH_DECL_MANUALHOOK1(OverrideMove, 0, 0, 0, bool, float);
DECLARE_HOOK(OverrideMove, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, OverrideMove, bool, (float flInterval), (flInterval));

SH_DECL_MANUALHOOK2(OverrideMoveFacing, 0, 0, 0, bool, const AILocalMoveGoal_t &, float );
DECLARE_HOOK(OverrideMoveFacing, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, OverrideMoveFacing, bool, (const AILocalMoveGoal_t &move, float flInterval), (move, flInterval));

SH_DECL_MANUALHOOK2(GetHitgroupDamageMultiplier, 0, 0, 0, float, int, const CTakeDamageInfo & );
DECLARE_HOOK(GetHitgroupDamageMultiplier, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetHitgroupDamageMultiplier, float, (int iHitGroup, const CTakeDamageInfo &info), (iHitGroup, info));

SH_DECL_MANUALHOOK2(OnBehaviorChangeStatus, 0, 0, 0, bool, CAI_BehaviorBase *, bool);
DECLARE_HOOK(OnBehaviorChangeStatus, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, OnBehaviorChangeStatus, bool, (CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule), (pBehavior, fCanFinishSchedule));

SH_DECL_MANUALHOOK0(IsInterruptable, 0, 0, 0, bool);
DECLARE_HOOK(IsInterruptable, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsInterruptable, bool, (), ());

SH_DECL_MANUALHOOK2_void(MakeAIFootstepSound, 0, 0, 0, float , float );
DECLARE_HOOK(MakeAIFootstepSound, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, MakeAIFootstepSound, (float volume, float duration), (volume, duration));

SH_DECL_MANUALHOOK0_void(OnScheduleChange, 0, 0, 0);
DECLARE_HOOK(OnScheduleChange, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnScheduleChange, (), ());

SH_DECL_MANUALHOOK2_void(TranslateNavGoal, 0, 0, 0, CBaseEntity *, Vector &);
DECLARE_HOOK(TranslateNavGoal, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, TranslateNavGoal, (CBaseEntity *pEnemy, Vector &chasePosition), (pEnemy, chasePosition));

SH_DECL_MANUALHOOK2(MeleeAttack1Conditions, 0, 0, 0, int, float, float);
DECLARE_HOOK(MeleeAttack1Conditions, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, MeleeAttack1Conditions, int, (float flDot, float flDist), (flDot, flDist));

DECLARE_DETOUR(SetupVPhysicsHull, CAI_NPC);
DECLARE_DEFAULTHANDLER_DETOUR_void(CAI_NPC, SetupVPhysicsHull, (), ());

SH_DECL_MANUALHOOK1(IsUnreachable, 0, 0, 0, bool, CBaseEntity* );
DECLARE_HOOK(IsUnreachable, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsUnreachable, bool, (CBaseEntity* pEntity) , (pEntity));

SH_DECL_MANUALHOOK4(OnObstructingDoor, 0, 0, 0, bool, AILocalMoveGoal_t *, CBaseEntity *, float , AIMoveResult_t *);
DECLARE_HOOK(OnObstructingDoor, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, OnObstructingDoor, bool, (AILocalMoveGoal_t *pMoveGoal, CBaseEntity *pDoor, float distClear, AIMoveResult_t *pResult), (pMoveGoal, pDoor, distClear, pResult));

SH_DECL_MANUALHOOK1(IsHeavyDamage, 0, 0, 0, bool, const CTakeDamageInfo &);
DECLARE_HOOK(IsHeavyDamage, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsHeavyDamage, bool, (const CTakeDamageInfo &info), (info));

SH_DECL_MANUALHOOK0(GetTimeToNavGoal, 0, 0, 0, float);
DECLARE_HOOK(GetTimeToNavGoal, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetTimeToNavGoal, float,(), ());

SH_DECL_MANUALHOOK0(StepHeight, 0, 0, 0, float);
DECLARE_HOOK(StepHeight, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, StepHeight, float,() const, ());

SH_DECL_MANUALHOOK0(GetJumpGravity, 0, 0, 0, float);
DECLARE_HOOK(GetJumpGravity, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetJumpGravity, float,() const, ());

SH_DECL_MANUALHOOK0(ShouldPlayerAvoid, 0, 0, 0, bool);
DECLARE_HOOK(ShouldPlayerAvoid, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ShouldPlayerAvoid, bool,(), ());

SH_DECL_MANUALHOOK1(ShouldProbeCollideAgainstEntity, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(ShouldProbeCollideAgainstEntity, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ShouldProbeCollideAgainstEntity, bool,(CBaseEntity *pEntity), (pEntity));

SH_DECL_MANUALHOOK0(CapabilitiesGet, 0, 0, 0, int);
DECLARE_HOOK(CapabilitiesGet, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CapabilitiesGet, int,() const, ());

SH_DECL_MANUALHOOK0(IsCrouching, 0, 0, 0, bool);
DECLARE_HOOK(IsCrouching, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsCrouching, bool,() , ());

DECLARE_DETOUR(CineCleanup, CAI_NPC);
DECLARE_DEFAULTHANDLER_DETOUR(CAI_NPC, CineCleanup, bool, (), ());

SH_DECL_MANUALHOOK2_void(OnChangeHintGroup, 0, 0, 0, string_t , string_t);
DECLARE_HOOK(OnChangeHintGroup, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnChangeHintGroup,(string_t oldGroup, string_t newGroup) , (oldGroup,newGroup));

SH_DECL_MANUALHOOK3(IsJumpLegal, 0, 0, 0, bool, const Vector &, const Vector &, const Vector &);
DECLARE_HOOK(IsJumpLegal, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsJumpLegal, bool, (const Vector &startPos, const Vector &apex, const Vector &endPos ) const, (startPos,  apex,  endPos ));

SH_DECL_MANUALHOOK1(ShouldFailNav, 0, 0, 0, bool , bool);
DECLARE_HOOK(ShouldFailNav, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ShouldFailNav, bool, (bool bMovementFailed), (bMovementFailed));

SH_DECL_MANUALHOOK0(IsNavigationUrgent, 0, 0, 0, bool);
DECLARE_HOOK(IsNavigationUrgent, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsNavigationUrgent, bool, (), ());

SH_DECL_MANUALHOOK0(GetDefaultNavGoalTolerance, 0, 0, 0, float);
DECLARE_HOOK(GetDefaultNavGoalTolerance, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetDefaultNavGoalTolerance, float, (), ());

SH_DECL_MANUALHOOK0_void(OnMovementFailed, 0, 0, 0);
DECLARE_HOOK(OnMovementFailed, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnMovementFailed, (), ());

SH_DECL_MANUALHOOK0(ShouldBruteForceFailedNav, 0, 0, 0, bool);
DECLARE_HOOK(ShouldBruteForceFailedNav, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ShouldBruteForceFailedNav,bool, (), ());

SH_DECL_MANUALHOOK2(IsUnusableNode, 0, 0, 0, bool, int , CBaseEntity *);
DECLARE_HOOK(IsUnusableNode, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsUnusableNode,bool, (int iNodeID, CBaseEntity *pHint), (iNodeID, pHint));

SH_DECL_MANUALHOOK4(MovementCost, 0, 0, 0, bool, int , const Vector &, const Vector &, float *);
DECLARE_HOOK(MovementCost, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, MovementCost, bool, (int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost), (moveType,  vecStart, vecEnd, pCost));

SH_DECL_MANUALHOOK0(GetNodeViewOffset, 0, 0, 0, Vector);
DECLARE_HOOK(GetNodeViewOffset, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetNodeViewOffset, Vector, (), ());

SH_DECL_MANUALHOOK0_void(PostNPCInit, 0, 0, 0);
DECLARE_HOOK(PostNPCInit, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, PostNPCInit, (), ());

SH_DECL_MANUALHOOK0(InnateRange1MaxRange, 0, 0, 0, float);
DECLARE_HOOK(InnateRange1MaxRange, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, InnateRange1MaxRange, float, (), ());

SH_DECL_MANUALHOOK0(InnateRange1MinRange, 0, 0, 0, float);
DECLARE_HOOK(InnateRange1MinRange, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, InnateRange1MinRange, float, (), ());

SH_DECL_MANUALHOOK2(RangeAttack1Conditions, 0, 0, 0, int, float, float );
DECLARE_HOOK(RangeAttack1Conditions, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, RangeAttack1Conditions, int, (float flDot, float flDist), (flDot, flDist));

SH_DECL_MANUALHOOK2(RangeAttack2Conditions, 0, 0, 0, int, float, float );
DECLARE_HOOK(RangeAttack2Conditions, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, RangeAttack2Conditions, int, (float flDot, float flDist), (flDot, flDist));

SH_DECL_MANUALHOOK2_void(OnStateChange, 0, 0, 0, NPC_STATE, NPC_STATE );
DECLARE_HOOK(OnStateChange, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnStateChange, ( NPC_STATE OldState, NPC_STATE NewState ), (OldState, NewState));

SH_DECL_MANUALHOOK0(ShouldPlayIdleSound, 0, 0, 0, bool);
DECLARE_HOOK(ShouldPlayIdleSound, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ShouldPlayIdleSound, bool, (), ());

SH_DECL_MANUALHOOK1(IsLightDamage, 0, 0, 0, bool, const CTakeDamageInfo &);
DECLARE_HOOK(IsLightDamage, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsLightDamage, bool, (const CTakeDamageInfo &info), (info));

SH_DECL_MANUALHOOK1(CanBeAnEnemyOf, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(CanBeAnEnemyOf, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CanBeAnEnemyOf, bool, (CBaseEntity *pEnemy), (pEnemy));

SH_DECL_MANUALHOOK0(AllowedToIgnite, 0, 0, 0, bool);
DECLARE_HOOK(AllowedToIgnite, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, AllowedToIgnite, bool, (), ());

SH_DECL_MANUALHOOK1_void(GatherEnemyConditions, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(GatherEnemyConditions, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, GatherEnemyConditions, (CBaseEntity *pEnemy), (pEnemy));

SH_DECL_MANUALHOOK0(IsSilentSquadMember, 0, 0, 0, bool);
DECLARE_HOOK(IsSilentSquadMember, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsSilentSquadMember, bool, () const, ());

SH_DECL_MANUALHOOK0_void(OnMovementComplete, 0, 0, 0);
DECLARE_HOOK(OnMovementComplete, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnMovementComplete, (), ());

SH_DECL_MANUALHOOK5_void(AddFacingTarget_E_V_F_F_F, 0, 0, 0, CBaseEntity *, const Vector &, float , float, float);
DECLARE_HOOK(AddFacingTarget_E_V_F_F_F, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, AddFacingTarget_E_V_F_F_F, (CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp), (pTarget, vecPosition, flImportance, flDuration, flRamp));

SH_DECL_MANUALHOOK1(QueryHearSound, 0, 0, 0, bool, CSound *);
DECLARE_HOOK(QueryHearSound, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, QueryHearSound, bool, (CSound *pSound), (pSound));

SH_DECL_MANUALHOOK1(IsPlayerAlly, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(IsPlayerAlly, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsPlayerAlly, bool, (CBaseEntity *pPlayer), (pPlayer));

SH_DECL_MANUALHOOK0(GetRunningBehavior, 0, 0, 0, CAI_BehaviorBase *);
DECLARE_HOOK(GetRunningBehavior, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetRunningBehavior, CAI_BehaviorBase *, (), ());


SH_DECL_MANUALHOOK0_void(OnUpdateShotRegulator, 0, 0, 0);
DECLARE_HOOK(OnUpdateShotRegulator, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnUpdateShotRegulator, (), ());

SH_DECL_MANUALHOOK2_void(CleanupOnDeath, 0, 0, 0, CBaseEntity *, bool);
DECLARE_HOOK(CleanupOnDeath, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, CleanupOnDeath, (CBaseEntity *pCulprit, bool bFireDeathOutput), (pCulprit, bFireDeathOutput));

SH_DECL_MANUALHOOK1_void(OnStartSchedule, 0, 0, 0, int);
DECLARE_HOOK(OnStartSchedule, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnStartSchedule, (int scheduleType), (scheduleType));

SH_DECL_MANUALHOOK0_void(AimGun, 0, 0, 0);
DECLARE_HOOK(AimGun, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, AimGun, (), ());

SH_DECL_MANUALHOOK0(GetSchedulingErrorName, 0, 0, 0, const char *);
DECLARE_HOOK(GetSchedulingErrorName, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetSchedulingErrorName, const char *, (), ());

SH_DECL_MANUALHOOK0(IsCurTaskContinuousMove, 0, 0, 0, bool);
DECLARE_HOOK(IsCurTaskContinuousMove, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsCurTaskContinuousMove, bool, (), ());

SH_DECL_MANUALHOOK1(IsValidEnemy, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(IsValidEnemy, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsValidEnemy, bool, (CBaseEntity *pEnemy), (pEnemy));

SH_DECL_MANUALHOOK2(IsValidCover, 0, 0, 0, bool, const Vector &, CBaseEntity const *);
DECLARE_HOOK(IsValidCover, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsValidCover, bool, (const Vector &vLocation, CBaseEntity const *pHint), (vLocation, pHint));

SH_DECL_MANUALHOOK3(IsValidShootPosition, 0, 0, 0, bool, const Vector &, CAI_Node *, CBaseEntity const *);
DECLARE_HOOK(IsValidShootPosition, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsValidShootPosition, bool, (const Vector &vLocation, CAI_Node *pNode, CBaseEntity const *pHint), (vLocation, pNode, pHint));

SH_DECL_MANUALHOOK0(GetMaxTacticalLateralMovement, 0, 0, 0, float);
DECLARE_HOOK(GetMaxTacticalLateralMovement, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetMaxTacticalLateralMovement, float, (), ());

SH_DECL_MANUALHOOK1(ShouldIgnoreSound, 0, 0, 0, bool, CSound *);
DECLARE_HOOK(ShouldIgnoreSound, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ShouldIgnoreSound, bool, (CSound *pSound), (pSound));

SH_DECL_MANUALHOOK1_void(OnSeeEntity, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(OnSeeEntity, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnSeeEntity, (CBaseEntity *pEntity), (pEntity));

SH_DECL_MANUALHOOK0(GetReasonableFacingDist, 0, 0, 0, float);
DECLARE_HOOK(GetReasonableFacingDist, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetReasonableFacingDist, float, (), ());

SH_DECL_MANUALHOOK0(CanFlinch, 0, 0, 0, bool);
DECLARE_HOOK(CanFlinch, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CanFlinch, bool, (), ());

SH_DECL_MANUALHOOK1(IsCrouchedActivity, 0, 0, 0, bool, Activity);
DECLARE_HOOK(IsCrouchedActivity, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsCrouchedActivity, bool, (Activity activity), (activity));

SH_DECL_MANUALHOOK1(CanRunAScriptedNPCInteraction, 0, 0, 0, bool, bool);
DECLARE_HOOK(CanRunAScriptedNPCInteraction, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CanRunAScriptedNPCInteraction, bool, (bool bForced), (bForced));

SH_DECL_MANUALHOOK2(GetFlinchActivity, 0, 0, 0, Activity, bool, bool);
DECLARE_HOOK(GetFlinchActivity, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetFlinchActivity, Activity, (bool bHeavyDamage, bool bGesture ), (bHeavyDamage, bGesture));

SH_DECL_MANUALHOOK0(ShouldAlwaysThink, 0, 0, 0, bool);
DECLARE_HOOK(ShouldAlwaysThink, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ShouldAlwaysThink, bool, (), ());

SH_DECL_MANUALHOOK3(ScheduledMoveToGoalEntity, 0, 0, 0, bool, int , CBaseEntity *, Activity);
DECLARE_HOOK(ScheduledMoveToGoalEntity, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ScheduledMoveToGoalEntity, bool, (int scheduleType, CBaseEntity *pGoalEntity, Activity movementActivity), (scheduleType, pGoalEntity, movementActivity));

SH_DECL_MANUALHOOK3(ScheduledFollowPath, 0, 0, 0, bool, int , CBaseEntity *, Activity);
DECLARE_HOOK(ScheduledFollowPath, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ScheduledFollowPath, bool, (int scheduleType, CBaseEntity *pPathStart, Activity movementActivity), (scheduleType, pPathStart, movementActivity));

SH_DECL_MANUALHOOK1(TaskName, 0, 0, 0, const char *, int);
DECLARE_HOOK(TaskName, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, TaskName, const char *, (int taskID), (taskID));

SH_DECL_MANUALHOOK0(GetNewSchedule, 0, 0, 0, CAI_Schedule *);
DECLARE_HOOK(GetNewSchedule, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetNewSchedule, CAI_Schedule *, (), ());

SH_DECL_MANUALHOOK0(GetFailSchedule, 0, 0, 0, CAI_Schedule *);
DECLARE_HOOK(GetFailSchedule, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetFailSchedule, CAI_Schedule *, (), ());

SH_DECL_MANUALHOOK0(AccessBehaviors, 0, 0, 0, CAI_BehaviorBase **);
DECLARE_HOOK(AccessBehaviors, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, AccessBehaviors, CAI_BehaviorBase **, (), ());

SH_DECL_MANUALHOOK0(NumBehaviors, 0, 0, 0, int);
DECLARE_HOOK(NumBehaviors, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, NumBehaviors, int, (), ());

SH_DECL_MANUALHOOK0(GetExpresser, 0, 0, 0, CAI_Expresser *);
DECLARE_HOOK(GetExpresser, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetExpresser, CAI_Expresser *, (), ());

SH_DECL_MANUALHOOK0(ValidateNavGoal, 0, 0, 0, bool);
DECLARE_HOOK(ValidateNavGoal, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ValidateNavGoal, bool, (), ());

SH_DECL_MANUALHOOK1_void(NotifyDeadFriend, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(NotifyDeadFriend, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, NotifyDeadFriend, (CBaseEntity *pFriend), (pFriend));

SH_DECL_MANUALHOOK1_void(SetAim, 0, 0, 0, const Vector &);
DECLARE_HOOK(SetAim, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, SetAim, (const Vector &aimDir), (aimDir));

SH_DECL_MANUALHOOK0_void(RelaxAim, 0, 0, 0);
DECLARE_HOOK(RelaxAim, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, RelaxAim, (), ());

SH_DECL_MANUALHOOK2(GetHintActivity, 0, 0, 0, Activity, short , Activity );
DECLARE_HOOK(GetHintActivity, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetHintActivity, Activity, (short sHintType, Activity HintsActivity), (sHintType, HintsActivity));

SH_DECL_MANUALHOOK1(FValidateHintType, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(FValidateHintType, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, FValidateHintType, bool, (CBaseEntity *pHint), (pHint));

SH_DECL_MANUALHOOK1(GetHintDelay, 0, 0, 0, float, short);
DECLARE_HOOK(GetHintDelay, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetHintDelay, float, (short sHintType), (sHintType));

SH_DECL_MANUALHOOK3(InnateWeaponLOSCondition, 0, 0, 0, bool, const Vector &, const Vector &, bool);
DECLARE_HOOK(InnateWeaponLOSCondition, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, InnateWeaponLOSCondition, bool, (const Vector &ownerPos, const Vector &targetPos, bool bSetConditions), (ownerPos, targetPos, bSetConditions));

SH_DECL_MANUALHOOK0(CoverRadius, 0, 0, 0, float);
DECLARE_HOOK(CoverRadius, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CoverRadius, float, (), ());

SH_DECL_MANUALHOOK0_void(SetTurnActivity, 0, 0, 0);
DECLARE_HOOK(SetTurnActivity, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, SetTurnActivity, (), ());

SH_DECL_MANUALHOOK0(FCanCheckAttacks, 0, 0, 0, bool);
DECLARE_HOOK(FCanCheckAttacks, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, FCanCheckAttacks, bool, (), ());

SH_DECL_MANUALHOOK1(GetGlobalScheduleId, 0, 0, 0, int, int);
DECLARE_HOOK(GetGlobalScheduleId, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetGlobalScheduleId, int, (int localScheduleID), (localScheduleID));

SH_DECL_MANUALHOOK4_void(AddFacingTarget_V_F_F_F, 0, 0, 0, const Vector &, float , float , float  );
DECLARE_HOOK(AddFacingTarget_V_F_F_F, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, AddFacingTarget_V_F_F_F, (const Vector &vecPosition, float flImportance, float flDuration, float flRamp), (vecPosition, flImportance, flDuration, flRamp));

SH_DECL_MANUALHOOK1_void(SetSquad, 0, 0, 0, CAI_Squad *);
DECLARE_HOOK(SetSquad, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, SetSquad, (CAI_Squad *pSquad), (pSquad));

SH_DECL_MANUALHOOK0_void(ClearCommandGoal, 0, 0, 0);
DECLARE_HOOK(ClearCommandGoal, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, ClearCommandGoal, (), ());

SH_DECL_MANUALHOOK4(FindCoverPosInRadius, 0, 0, 0, bool, CBaseEntity *, const Vector &, float , Vector *);
DECLARE_HOOK(FindCoverPosInRadius, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, FindCoverPosInRadius, bool, (CBaseEntity *pEntity, const Vector &goalPos, float coverRadius, Vector *pResult), (pEntity, goalPos, coverRadius, pResult));

SH_DECL_MANUALHOOK0_void(CheckAmmo, 0, 0, 0);
DECLARE_HOOK(CheckAmmo, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, CheckAmmo, (), ());

SH_DECL_MANUALHOOK0_void(OnRangeAttack1, 0, 0, 0);
DECLARE_HOOK(OnRangeAttack1, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnRangeAttack1, (), ());

SH_DECL_MANUALHOOK2(GetShootEnemyDir, 0, 0, 0, Vector, const Vector &, bool );
DECLARE_HOOK(GetShootEnemyDir, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetShootEnemyDir, Vector, (const Vector &shootOrigin, bool bNoisy), (shootOrigin, bNoisy));

SH_DECL_MANUALHOOK1_void(SetScriptedScheduleIgnoreConditions, 0, 0, 0, Interruptability_t);
DECLARE_HOOK(SetScriptedScheduleIgnoreConditions, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, SetScriptedScheduleIgnoreConditions, (Interruptability_t interrup), (interrup));

SH_DECL_MANUALHOOK0_void(OnStartScene, 0, 0, 0);
DECLARE_HOOK(OnStartScene, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnStartScene, (), ());

SH_DECL_MANUALHOOK0(SelectIdealState, 0, 0, 0, NPC_STATE);
DECLARE_HOOK(SelectIdealState, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, SelectIdealState, NPC_STATE, (), ());

SH_DECL_MANUALHOOK0_void(RunAI, 0, 0, 0);
DECLARE_HOOK(RunAI, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, RunAI, (), ());

SH_DECL_MANUALHOOK0(ShouldMoveAndShoot, 0, 0, 0, bool);
DECLARE_HOOK(ShouldMoveAndShoot, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ShouldMoveAndShoot, bool, (), ());

SH_DECL_MANUALHOOK0(CanBeUsedAsAFriend, 0, 0, 0, bool);
DECLARE_HOOK(CanBeUsedAsAFriend, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CanBeUsedAsAFriend, bool, (), ());

SH_DECL_MANUALHOOK2_void(OnClearGoal, 0, 0, 0, CAI_BehaviorBase *, CBaseEntity *);
DECLARE_HOOK(OnClearGoal, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnClearGoal, (CAI_BehaviorBase *pBehavior, CBaseEntity *pGoal), (pBehavior, pGoal));

SH_DECL_MANUALHOOK2(ShouldAcceptGoal, 0, 0, 0, bool, CAI_BehaviorBase *, CBaseEntity *);
DECLARE_HOOK(ShouldAcceptGoal, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ShouldAcceptGoal, bool, (CAI_BehaviorBase *pBehavior, CBaseEntity *pGoal), (pBehavior, pGoal));

SH_DECL_MANUALHOOK1_void(Wake, 0, 0, 0, bool);
DECLARE_HOOK(Wake, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, Wake, (bool bFireOutput), (bFireOutput));

SH_DECL_MANUALHOOK1(GetReactionDelay, 0, 0, 0, float, CBaseEntity * );
DECLARE_HOOK(GetReactionDelay, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetReactionDelay, float, (CBaseEntity *pEnemy ), (pEnemy));

SH_DECL_MANUALHOOK1(SquadSlotName, 0, 0, 0, const char*, int);
DECLARE_HOOK(SquadSlotName, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, SquadSlotName, const char*, (int slotID), (slotID));

SH_DECL_MANUALHOOK5(PlayerInSpread, 0, 0, 0, bool, const Vector &, const Vector &, float , float , bool );
DECLARE_HOOK(PlayerInSpread, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, PlayerInSpread, bool, (const Vector &sourcePos, const Vector &targetPos, float flSpread, float maxDistOffCenter, bool ignoreHatedPlayers), (sourcePos, targetPos, flSpread, maxDistOffCenter, ignoreHatedPlayers));

SH_DECL_MANUALHOOK1(EyeOffset, 0, 0, 0, Vector, Activity);
DECLARE_HOOK(EyeOffset, CAI_NPC);
DECLARE_DEFAULTHANDLER_SPECIAL(CAI_NPC, EyeOffset, Vector, (Activity nActivity), (nActivity), vec3_origin);

SH_DECL_MANUALHOOK2_void(CollectShotStats, 0, 0, 0, const Vector &, const Vector &);
DECLARE_HOOK(CollectShotStats, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, CollectShotStats, (const Vector &vecShootOrigin, const Vector &vecShootDir), (vecShootOrigin, vecShootDir));

SH_DECL_MANUALHOOK1_void(OnLooked, 0, 0, 0, int);
DECLARE_HOOK(OnLooked, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnLooked, (int iDistance), (iDistance));

SH_DECL_MANUALHOOK0(ShouldNotDistanceCull, 0, 0, 0, bool);
DECLARE_HOOK(ShouldNotDistanceCull, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ShouldNotDistanceCull, bool, (), ());

SH_DECL_MANUALHOOK0(HolsterWeapon, 0, 0, 0, int);
DECLARE_HOOK(HolsterWeapon, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, HolsterWeapon, int, (), ());

SH_DECL_MANUALHOOK0(UnholsterWeapon, 0, 0, 0, int);
DECLARE_HOOK(UnholsterWeapon, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, UnholsterWeapon, int, (), ());

SH_DECL_MANUALHOOK2(FindNamedEntity, 0, 0, 0, CBaseEntity *, const char *, IEntityFindFilter *);
DECLARE_HOOK(FindNamedEntity, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, FindNamedEntity, CBaseEntity *, (const char *pszName, IEntityFindFilter *pFilter), (pszName, pFilter));

SH_DECL_MANUALHOOK0(FacingPosition, 0, 0, 0, Vector);
DECLARE_HOOK(FacingPosition, CAI_NPC);
DECLARE_DEFAULTHANDLER_SPECIAL(CAI_NPC, FacingPosition, Vector, (), (), vec3_origin);

SH_DECL_MANUALHOOK4_void(AddFacingTarget_E_F_F_F, 0, 0, 0, CBaseEntity *, float , float , float);
DECLARE_HOOK(AddFacingTarget_E_F_F_F, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, AddFacingTarget_E_F_F_F, (CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp), (pTarget, flImportance, flDuration, flRamp));

SH_DECL_MANUALHOOK1(ValidEyeTarget, 0, 0, 0, bool, const Vector &);
DECLARE_HOOK(ValidEyeTarget, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ValidEyeTarget, bool, (const Vector &lookTargetPos), (lookTargetPos));

SH_DECL_MANUALHOOK2_void(SetHeadDirection, 0, 0, 0, const Vector &, float );
DECLARE_HOOK(SetHeadDirection, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, SetHeadDirection, (const Vector &vTargetPos, float flInterval), (vTargetPos, flInterval));

SH_DECL_MANUALHOOK0_void(MaintainTurnActivity, 0, 0, 0 );
DECLARE_HOOK(MaintainTurnActivity, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, MaintainTurnActivity, (), ());

SH_DECL_MANUALHOOK4_void(AddLookTarget_E, 0, 0, 0, CBaseEntity *, float , float , float );
DECLARE_HOOK(AddLookTarget_E, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, AddLookTarget_E, (CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp), (pTarget, flImportance, flDuration, flRamp));

SH_DECL_MANUALHOOK4_void(AddLookTarget_V, 0, 0, 0, const Vector &, float , float , float );
DECLARE_HOOK(AddLookTarget_V, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, AddLookTarget_V, (const Vector &vecPosition, float flImportance, float flDuration, float flRamp), (vecPosition, flImportance, flDuration, flRamp));

SH_DECL_MANUALHOOK1_void(MaintainLookTargets, 0, 0, 0, float);
DECLARE_HOOK(MaintainLookTargets, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, MaintainLookTargets, (float flInterval), (flInterval));

SH_DECL_MANUALHOOK0(Stand, 0, 0, 0, bool);
DECLARE_HOOK(Stand, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, Stand, bool, (), ());

SH_DECL_MANUALHOOK0(Crouch, 0, 0, 0, bool);
DECLARE_HOOK(Crouch, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, Crouch, bool, (), ());

SH_DECL_MANUALHOOK1(GetBestSound, 0, 0, 0, CSound *, int);
DECLARE_HOOK(GetBestSound, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetBestSound, CSound *, (int validTypes), (validTypes));

SH_DECL_MANUALHOOK1(FOkToMakeSound, 0, 0, 0, bool, int);
DECLARE_HOOK(FOkToMakeSound, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, FOkToMakeSound, bool, (int soundPriority), (soundPriority));

SH_DECL_MANUALHOOK2_void(JustMadeSound, 0, 0, 0, int, float);
DECLARE_HOOK(JustMadeSound, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, JustMadeSound, (int soundPriority, float flSoundLength), (soundPriority, flSoundLength));

SH_DECL_MANUALHOOK0_void(FearSound, 0, 0, 0);
DECLARE_HOOK(FearSound, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, FearSound, (), ());

SH_DECL_MANUALHOOK0(IsWaitingToRappel, 0, 0, 0, bool);
DECLARE_HOOK(IsWaitingToRappel, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsWaitingToRappel, bool, (), ());

SH_DECL_MANUALHOOK0_void(BeginRappel, 0, 0, 0);
DECLARE_HOOK(BeginRappel, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, BeginRappel, (), ());

SH_DECL_MANUALHOOK0_void(DesireCrouch, 0, 0, 0);
DECLARE_HOOK(DesireCrouch, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, DesireCrouch, (), ());

SH_DECL_MANUALHOOK3(WeaponLOSCondition, 0, 0, 0, bool, const Vector &, const Vector &, bool);
DECLARE_HOOK(WeaponLOSCondition, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, WeaponLOSCondition, bool, (const Vector &ownerPos, const Vector &targetPos, bool bSetConditions), (ownerPos, targetPos, bSetConditions));

SH_DECL_MANUALHOOK0(OnBeginMoveAndShoot, 0, 0, 0, bool);
DECLARE_HOOK(OnBeginMoveAndShoot, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, OnBeginMoveAndShoot, bool, (), ());

SH_DECL_MANUALHOOK1(GetSquadSlotDebugName, 0, 0, 0, const char *, int);
DECLARE_HOOK(GetSquadSlotDebugName, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetSquadSlotDebugName, const char *, (int iSquadSlot), (iSquadSlot));

SH_DECL_MANUALHOOK0_void(OnEndMoveAndShoot, 0, 0, 0);
DECLARE_HOOK(OnEndMoveAndShoot, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnEndMoveAndShoot, (), ());

SH_DECL_MANUALHOOK2(TestShootPosition, 0, 0, 0, bool, const Vector &, const Vector &);
DECLARE_HOOK(TestShootPosition, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, TestShootPosition, bool, (const Vector &vecShootPos, const Vector &targetPos), (vecShootPos, targetPos));

SH_DECL_MANUALHOOK0_void(ClearAttackConditions, 0, 0, 0);
DECLARE_HOOK(ClearAttackConditions, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, ClearAttackConditions, (), ());

SH_DECL_MANUALHOOK0_void(OnListened, 0, 0, 0);
DECLARE_HOOK(OnListened, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnListened, (), ());

SH_DECL_MANUALHOOK1_void(SpeakSentence, 0, 0, 0, int);
DECLARE_HOOK(SpeakSentence, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, SpeakSentence, (int sentenceType), (sentenceType));

SH_DECL_MANUALHOOK1(GetCoverActivity, 0, 0, 0, Activity, CBaseEntity* );
DECLARE_HOOK(GetCoverActivity, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetCoverActivity, Activity, (CBaseEntity* pHint), (pHint));

SH_DECL_MANUALHOOK2(IsCoverPosition, 0, 0, 0, bool, const Vector &, const Vector & );
DECLARE_HOOK(IsCoverPosition, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsCoverPosition, bool, (const Vector &vecThreat, const Vector &vecPosition), (vecThreat, vecPosition));

SH_DECL_MANUALHOOK0(CanHolsterWeapon, 0, 0, 0, bool);
DECLARE_HOOK(CanHolsterWeapon, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CanHolsterWeapon, bool, (), ());

SH_DECL_MANUALHOOK0(EyeLookTarget, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(EyeLookTarget, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, EyeLookTarget, CBaseEntity *, (), ());

SH_DECL_MANUALHOOK1(CanPlaySentence, 0, 0, 0, bool, bool);
DECLARE_HOOK(CanPlaySentence, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CanPlaySentence, bool, (bool fDisregardState), (fDisregardState));

SH_DECL_MANUALHOOK6(PlayScriptedSentence, 0, 0, 0, int, const char *, float , float , soundlevel_t , bool , CBaseEntity *);
DECLARE_HOOK(PlayScriptedSentence, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, PlayScriptedSentence, int, (const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener), (pszSentence, delay, volume, soundlevel, bConcurrent, pListener));

SH_DECL_MANUALHOOK1(CanRespondToEvent, 0, 0, 0, bool, const char *);
DECLARE_HOOK(CanRespondToEvent, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CanRespondToEvent, bool, (const char *ResponseConcept), (ResponseConcept));

SH_DECL_MANUALHOOK3(RespondedTo, 0, 0, 0, bool, const char *, bool, bool);
DECLARE_HOOK(RespondedTo, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, RespondedTo, bool, (const char *ResponseConcept, bool bForce, bool bCancelScene), (ResponseConcept,  bForce, bCancelScene));

SH_DECL_MANUALHOOK0_void(StartNPC, 0, 0, 0);
DECLARE_HOOK(StartNPC, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, StartNPC, (), ());

SH_DECL_MANUALHOOK1_void(UpdateEfficiency, 0, 0, 0, bool);
DECLARE_HOOK(UpdateEfficiency, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, UpdateEfficiency, (bool bInPVS), (bInPVS));

SH_DECL_MANUALHOOK0(ShouldChooseNewEnemy, 0, 0, 0, bool);
DECLARE_HOOK(ShouldChooseNewEnemy, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ShouldChooseNewEnemy, bool, (), ());

SH_DECL_MANUALHOOK0_void(PostRunStopMoving, 0, 0, 0);
DECLARE_HOOK(PostRunStopMoving, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, PostRunStopMoving, (), ());

SH_DECL_MANUALHOOK2(IsValidReasonableFacing, 0, 0, 0, bool, const Vector &, float );
DECLARE_HOOK(IsValidReasonableFacing, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsValidReasonableFacing,bool, (const Vector &vecSightDir, float sightDist), (vecSightDir,  sightDist));

SH_DECL_MANUALHOOK1(IsValidMoveAwayDest, 0, 0, 0, bool, const Vector &);
DECLARE_HOOK(IsValidMoveAwayDest, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsValidMoveAwayDest, bool, (const Vector &vecDest), (vecDest));

SH_DECL_MANUALHOOK0(ShouldLookForBetterWeapon, 0, 0, 0, bool);
DECLARE_HOOK(ShouldLookForBetterWeapon, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ShouldLookForBetterWeapon, bool, (), ());

SH_DECL_MANUALHOOK1_void(PickupWeapon, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(PickupWeapon, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, PickupWeapon, (CBaseEntity *pWeapon), (pWeapon));

SH_DECL_MANUALHOOK2(FindCoverPos_Entity, 0, 0, 0, bool, CBaseEntity *, Vector *);
DECLARE_HOOK(FindCoverPos_Entity, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, FindCoverPos_Entity, bool, (CBaseEntity *pEntity, Vector *pResult), (pEntity, pResult));

SH_DECL_MANUALHOOK2(FindCoverPos_Sound, 0, 0, 0, bool, CSound *, Vector *);
DECLARE_HOOK(FindCoverPos_Sound, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, FindCoverPos_Sound, bool, (CSound *pSound, Vector *pResult), (pSound, pResult));

SH_DECL_MANUALHOOK0(GetAlternateMoveShootTarget, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(GetAlternateMoveShootTarget, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetAlternateMoveShootTarget, CBaseEntity *, (), ());

SH_DECL_MANUALHOOK1_void(InputOutsideTransition, 0, 0, 0, inputdata_t &);
DECLARE_HOOK(InputOutsideTransition, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, InputOutsideTransition, (inputdata_t &inputdata), (inputdata));

SH_DECL_MANUALHOOK1_void(InputInsideTransition, 0, 0, 0, inputdata_t &);
DECLARE_HOOK(InputInsideTransition, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, InputInsideTransition, (inputdata_t &inputdata), (inputdata));

SH_DECL_MANUALHOOK1_void(GiveWeapon, 0, 0, 0, string_t);
DECLARE_HOOK(GiveWeapon, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, GiveWeapon, (string_t iszWeaponName), (iszWeaponName));

SH_DECL_MANUALHOOK0(HearingSensitivity, 0, 0, 0, float);
DECLARE_HOOK(HearingSensitivity, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, HearingSensitivity, float, (), ()); 

SH_DECL_MANUALHOOK0_void(BarnacleDeathSound, 0, 0, 0);
DECLARE_HOOK(BarnacleDeathSound, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, BarnacleDeathSound, (), ()); 

SH_DECL_MANUALHOOK0_void(CheckFlinches, 0, 0, 0);
DECLARE_HOOK(CheckFlinches, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, CheckFlinches, (), ()); 

SH_DECL_MANUALHOOK2(CurrentWeaponLOSCondition, 0, 0, 0, bool, const Vector &, bool);
DECLARE_HOOK(CurrentWeaponLOSCondition, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CurrentWeaponLOSCondition, bool,  (const Vector &targetPos, bool bSetConditions), (targetPos, bSetConditions)); 

SH_DECL_MANUALHOOK0_void(LostEnemySound, 0, 0, 0);
DECLARE_HOOK(LostEnemySound, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, LostEnemySound, (), ()); 

SH_DECL_MANUALHOOK1(IsActivityMovementPhased, 0, 0, 0, bool, Activity);
DECLARE_HOOK(IsActivityMovementPhased, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsActivityMovementPhased, bool, (Activity activity), (activity)); 

SH_DECL_MANUALHOOK0(CreateComponents, 0, 0, 0, bool);
DECLARE_HOOK(CreateComponents, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CreateComponents, bool, (), ());

SH_DECL_MANUALHOOK0(CreateSenses, 0, 0, 0, CAI_Senses *);
DECLARE_HOOK(CreateSenses, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CreateSenses, CAI_Senses *, (), ());

SH_DECL_MANUALHOOK0(CreateMoveProbe, 0, 0, 0, CAI_MoveProbe *);
DECLARE_HOOK(CreateMoveProbe, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CreateMoveProbe, CAI_MoveProbe *, (), ());

SH_DECL_MANUALHOOK0(CreateMotor, 0, 0, 0, CAI_Motor *);
DECLARE_HOOK(CreateMotor, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CreateMotor, CAI_Motor *, (), ());

SH_DECL_MANUALHOOK0(CreateLocalNavigator, 0, 0, 0, CAI_LocalNavigator *);
DECLARE_HOOK(CreateLocalNavigator, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CreateLocalNavigator, CAI_LocalNavigator *, (), ());

SH_DECL_MANUALHOOK0(CreateNavigator, 0, 0, 0, CAI_Navigator *);
DECLARE_HOOK(CreateNavigator, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CreateNavigator, CAI_Navigator *, (), ());

SH_DECL_MANUALHOOK0(CreatePathfinder, 0, 0, 0, CAI_Pathfinder *);
DECLARE_HOOK(CreatePathfinder, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CreatePathfinder, CAI_Pathfinder *, (), ());

SH_DECL_MANUALHOOK0(CreateTacticalServices, 0, 0, 0, CAI_TacticalServices *);
DECLARE_HOOK(CreateTacticalServices, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CreateTacticalServices, CAI_TacticalServices *, (), ());

SH_DECL_MANUALHOOK2(CreateCustomTarget, 0, 0, 0, CBaseEntity *, const Vector &, float);
DECLARE_HOOK(CreateCustomTarget, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, CreateCustomTarget, CBaseEntity *, (const Vector &vecOrigin, float duration), (vecOrigin, duration));

SH_DECL_MANUALHOOK1_void(StartTargetHandling, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(StartTargetHandling, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, StartTargetHandling, (CBaseEntity *pTargetEnt), (pTargetEnt));

SH_DECL_MANUALHOOK0_void(ClearSenseConditions, 0, 0, 0);
DECLARE_HOOK(ClearSenseConditions, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, ClearSenseConditions, (), ());

SH_DECL_MANUALHOOK0_void(PlayFlinchGesture, 0, 0, 0);
DECLARE_HOOK(PlayFlinchGesture, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, PlayFlinchGesture, (), ());

SH_DECL_MANUALHOOK0_void(FoundEnemySound, 0, 0, 0);
DECLARE_HOOK(FoundEnemySound, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, FoundEnemySound, (), ());

SH_DECL_MANUALHOOK0(UseAttackSquadSlots, 0, 0, 0, bool);
DECLARE_HOOK(UseAttackSquadSlots, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, UseAttackSquadSlots, bool, (), ());

SH_DECL_MANUALHOOK1_void(OnGivenWeapon, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(OnGivenWeapon, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnGivenWeapon, (CBaseEntity *pNewWeapon), (pNewWeapon));

SH_DECL_MANUALHOOK1_void(PickupItem, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(PickupItem, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, PickupItem, (CBaseEntity *pItem), (pItem));

SH_DECL_MANUALHOOK3_void(MoveOrder, 0, 0, 0, const Vector &, CBaseEntity **, int  );
DECLARE_HOOK(MoveOrder, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, MoveOrder, (const Vector &vecDest, CBaseEntity **Allies, int numAllies ), (vecDest, Allies, numAllies ));

SH_DECL_MANUALHOOK0_void(OnMoveOrder, 0, 0, 0);
DECLARE_HOOK(OnMoveOrder, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnMoveOrder, (), ());

SH_DECL_MANUALHOOK0_void(OnMoveToCommandGoalFailed, 0, 0, 0);
DECLARE_HOOK(OnMoveToCommandGoalFailed, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, OnMoveToCommandGoalFailed, (), ());

SH_DECL_MANUALHOOK0(IsCommandable, 0, 0, 0, bool);
DECLARE_HOOK(IsCommandable, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsCommandable, bool, (), ());

SH_DECL_MANUALHOOK0(IsCommandMoving, 0, 0, 0, bool);
DECLARE_HOOK(IsCommandMoving, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsCommandMoving, bool, (), ());

SH_DECL_MANUALHOOK0(ShouldAutoSummon, 0, 0, 0, bool);
DECLARE_HOOK(ShouldAutoSummon, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, ShouldAutoSummon, bool, (), ());

SH_DECL_MANUALHOOK1(IsValidCommandTarget, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(IsValidCommandTarget, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsValidCommandTarget, bool, (CBaseEntity *pTarget), (pTarget));

SH_DECL_MANUALHOOK3(TargetOrder, 0, 0, 0, bool, CBaseEntity *, CBaseEntity **, int );
DECLARE_HOOK(TargetOrder, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, TargetOrder, bool, (CBaseEntity *pTarget, CBaseEntity **Allies, int numAllies), (pTarget, Allies, numAllies));

SH_DECL_MANUALHOOK0(GetSquadCommandRepresentative, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(GetSquadCommandRepresentative, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetSquadCommandRepresentative, CBaseEntity *, (), ());

SH_DECL_MANUALHOOK0(IsMedic, 0, 0, 0, bool);
DECLARE_HOOK(IsMedic, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, IsMedic, bool, (), ());

SH_DECL_MANUALHOOK1_void(SetCommandGoal, 0, 0, 0, const Vector &);
DECLARE_HOOK(SetCommandGoal, CAI_NPC);
DECLARE_DEFAULTHANDLER_void(CAI_NPC, SetCommandGoal, (const Vector &vecGoal), (vecGoal));

SH_DECL_MANUALHOOK0(GetBestScent, 0, 0, 0, CSound *);
DECLARE_HOOK(GetBestScent, CAI_NPC);
DECLARE_DEFAULTHANDLER(CAI_NPC, GetBestScent, CSound *, (), ());


//Sendprops
DEFINE_PROP(m_bPerformAvoidance, CAI_NPC);




//Datamaps
DEFINE_PROP(m_NPCState, CAI_NPC);
DEFINE_PROP(m_afCapability, CAI_NPC);
DEFINE_PROP(m_Conditions, CAI_NPC);
DEFINE_PROP(m_hEnemy, CAI_NPC);
DEFINE_PROP(m_ScheduleState, CAI_NPC);
DEFINE_PROP(m_MoveAndShootOverlay, CAI_NPC);
DEFINE_PROP(m_pMotor, CAI_NPC);
DEFINE_PROP(m_flNextFlinchTime, CAI_NPC);
DEFINE_PROP(m_pMoveProbe, CAI_NPC);
DEFINE_PROP(m_Activity, CAI_NPC);
DEFINE_PROP(m_strHintGroup, CAI_NPC);
DEFINE_PROP(m_pHintNode, CAI_NPC);
DEFINE_PROP(m_SleepState, CAI_NPC);
DEFINE_PROP(m_pSchedule, CAI_NPC);
DEFINE_PROP(m_IdealSchedule, CAI_NPC);
DEFINE_PROP(m_iInteractionState, CAI_NPC);
DEFINE_PROP(m_hCine, CAI_NPC);
DEFINE_PROP(m_IdealActivity, CAI_NPC);
DEFINE_PROP(m_pNavigator, CAI_NPC);
DEFINE_PROP(m_pSquad, CAI_NPC);
DEFINE_PROP(m_iMySquadSlot, CAI_NPC);
DEFINE_PROP(m_afMemory, CAI_NPC);
DEFINE_PROP(m_CustomInterruptConditions, CAI_NPC);
DEFINE_PROP(m_IdealNPCState, CAI_NPC);
DEFINE_PROP(m_poseMove_Yaw, CAI_NPC);
DEFINE_PROP(m_InverseIgnoreConditions, CAI_NPC);
DEFINE_PROP(m_flWaitFinished, CAI_NPC);
DEFINE_PROP(m_hInteractionPartner, CAI_NPC);
DEFINE_PROP(m_SquadName, CAI_NPC);
DEFINE_PROP(m_vDefaultEyeOffset, CAI_NPC);
DEFINE_PROP(m_pLocalNavigator, CAI_NPC);
DEFINE_PROP(m_iszSceneCustomMoveSeq, CAI_NPC);
DEFINE_PROP(m_hTargetEnt, CAI_NPC);
DEFINE_PROP(m_scriptState, CAI_NPC);
DEFINE_PROP(m_hGoalEnt, CAI_NPC);
DEFINE_PROP(m_pTacticalServices, CAI_NPC);
DEFINE_PROP(m_bHintGroupNavLimiting, CAI_NPC);
DEFINE_PROP(m_flMoveWaitFinished, CAI_NPC);
DEFINE_PROP(m_pPathfinder, CAI_NPC);
DEFINE_PROP(m_flLastDamageTime, CAI_NPC);
DEFINE_PROP(m_vSavePosition, CAI_NPC);
DEFINE_PROP(m_UnreachableEnts, CAI_NPC);
DEFINE_PROP(m_EnemiesSerialNumber, CAI_NPC);
DEFINE_PROP(m_hEnemyOccluder, CAI_NPC);
DEFINE_PROP(m_pSenses, CAI_NPC);
DEFINE_PROP(m_OnLostPlayer, CAI_NPC);
DEFINE_PROP(m_OnLostEnemy, CAI_NPC);
DEFINE_PROP(m_bInAScript, CAI_NPC);
DEFINE_PROP(m_flDistTooFar, CAI_NPC);
DEFINE_PROP(m_flLastAttackTime, CAI_NPC);
DEFINE_PROP(m_spawnEquipment, CAI_NPC);
DEFINE_PROP(m_ShotRegulator, CAI_NPC);
DEFINE_PROP(m_vInterruptSavePosition, CAI_NPC);
DEFINE_PROP(m_Efficiency, CAI_NPC);
DEFINE_PROP(m_MoveEfficiency, CAI_NPC);
DEFINE_PROP(m_iDesiredWeaponState, CAI_NPC);
DEFINE_PROP(m_flEyeIntegRate, CAI_NPC);
DEFINE_PROP(m_bConditionsGathered, CAI_NPC);
DEFINE_PROP(m_iInteractionPlaying, CAI_NPC);
DEFINE_PROP(m_ScriptedInteractions, CAI_NPC);
DEFINE_PROP(m_bCannotDieDuringInteraction, CAI_NPC);
DEFINE_PROP(m_OnDamaged, CAI_NPC);
DEFINE_PROP(m_OnDamagedByPlayer, CAI_NPC);
DEFINE_PROP(m_OnDamagedByPlayerSquad, CAI_NPC);
DEFINE_PROP(m_OnHalfHealth, CAI_NPC);
DEFINE_PROP(m_bForceConditionsGather, CAI_NPC);
DEFINE_PROP(m_flSumDamage, CAI_NPC);
DEFINE_PROP(m_flLastPlayerDamageTime, CAI_NPC);
DEFINE_PROP(m_poseAim_Pitch, CAI_NPC);
DEFINE_PROP(m_poseAim_Yaw, CAI_NPC);
DEFINE_PROP(m_flSceneTime, CAI_NPC);
DEFINE_PROP(m_bCrouchDesired, CAI_NPC);
DEFINE_PROP(m_bForceCrouch, CAI_NPC);
DEFINE_PROP(m_ConditionsPreIgnore, CAI_NPC);
DEFINE_PROP(m_flLastEnemyTime, CAI_NPC);
DEFINE_PROP(m_flNextDecisionTime, CAI_NPC);
DEFINE_PROP(m_bSkippedChooseEnemy, CAI_NPC);
DEFINE_PROP(m_flTimeEnemyAcquired, CAI_NPC);
DEFINE_PROP(m_OnDeath, CAI_NPC);
DEFINE_PROP(m_flAcceptableTimeSeenEnemy, CAI_NPC);
DEFINE_PROP(m_OnRappelTouchdown, CAI_NPC);
DEFINE_PROP(m_vecCommandGoal, CAI_NPC);
DEFINE_PROP(m_iszPendingWeapon, CAI_NPC);
DEFINE_PROP(m_iDamageCount, CAI_NPC);
DEFINE_PROP(m_bInChoreo, CAI_NPC);
DEFINE_PROP(m_hEnemyFilter, CAI_NPC);
DEFINE_PROP(m_OnFoundEnemy, CAI_NPC);
DEFINE_PROP(m_OnFoundPlayer, CAI_NPC);
DEFINE_PROP(m_nIdealSequence, CAI_NPC);
DEFINE_PROP(m_IdealTranslatedActivity, CAI_NPC);
DEFINE_PROP(m_IdealWeaponActivity, CAI_NPC);
DEFINE_PROP(m_translatedActivity, CAI_NPC);
DEFINE_PROP(m_fNoDamageDecal, CAI_NPC);
DEFINE_PROP(m_bPlayerAvoidState, CAI_NPC);
DEFINE_PROP(m_flNextWeaponSearchTime, CAI_NPC);
DEFINE_PROP(m_OnDenyCommanderUse, CAI_NPC);
DEFINE_PROP(m_flLastStateChangeTime, CAI_NPC);
DEFINE_PROP(m_flSoundWaitTime, CAI_NPC);


//-----------------------------------------------------------------------------

CAI_NPC::CAI_NPC()
{
	g_AI_Manager.AddAI( this );
}

CAI_NPC::~CAI_NPC()
{
	g_AI_Manager.RemoveAI( this );
}

void CAI_NPC::PostConstructor()
{
	BaseClass::PostConstructor();
	if(func_CallNPCThink == NULL)
	{
		void *ptr = UTIL_FunctionFromName( GetDataDescMap_Real(), "CAI_BaseNPCCallNPCThink");
		if(ptr)
		{
			memcpy(&func_CallNPCThink, &ptr, sizeof(void *));
		}
	}
	Assert(func_CallNPCThink);
}

void CAI_NPC::CapabilitiesClear()
{
	m_afCapability = 0;
}

int CAI_NPC::CapabilitiesAdd(int capability)
{
	m_afCapability |= capability;
	return m_afCapability;
}

int CAI_NPC::CapabilitiesRemove(int capability)
{
	m_afCapability &= ~capability;
	return m_afCapability;
}

#define InterruptFromCondition( iCondition ) \
	AI_RemapFromGlobal( ( AI_IdIsLocal( iCondition ) ? GetClassScheduleIdSpace()->ConditionLocalToGlobal( iCondition ) : iCondition ) )
	
void CAI_NPC::SetCondition(int iCondition)
{
	int interrupt = InterruptFromCondition( iCondition );
	if ( interrupt == -1 )
	{
		Assert(0);
		return;
	}	
	m_Conditions->Set(interrupt);
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_NPC::HasCondition( int iCondition )
{
	int interrupt = InterruptFromCondition( iCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return false;
	}
	
	bool bReturn = m_Conditions->IsBitSet(interrupt);
	return (bReturn);
}

bool CAI_NPC::HasCondition( int iCondition, bool bUseIgnoreConditions )
{
	if ( bUseIgnoreConditions )
		return HasCondition( iCondition );
	
	int interrupt = InterruptFromCondition( iCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return false;
	}
	
	bool bReturn = m_ConditionsPreIgnore->IsBitSet(interrupt);
	return (bReturn);
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_NPC::ClearCondition( int iCondition )
{
	int interrupt = InterruptFromCondition( iCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return;
	}
	
	m_Conditions->Clear(interrupt);
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_NPC::ClearConditions( int *pConditions, int nConditions )
{
	for ( int i = 0; i < nConditions; ++i )
	{
		int iCondition = pConditions[i];
		int interrupt = InterruptFromCondition( iCondition );
		
		if ( interrupt == -1 )
		{
			Assert(0);
			continue;
		}
		
		m_Conditions->Clear( interrupt );
	}
}

bool CAI_NPC::HasInterruptCondition( int iCondition )
{
	if( !GetCurSchedule() )
	{
		return false;
	}

	int interrupt = InterruptFromCondition( iCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return false;
	}
	return ( m_Conditions->IsBitSet( interrupt ) && GetCurSchedule()->HasInterrupt( interrupt ) );
}


bool CAI_NPC::AutoMovement(CEntity *pTarget, AIMoveTrace_t *pTraceResult)
{
	return AutoMovement( GetAnimTimeInterval(), pTarget, pTraceResult );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//			 - 
//			*pTraceResult - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_NPC::AutoMovement( float flInterval, CEntity *pTarget, AIMoveTrace_t *pTraceResult )
{
	return g_helpfunc.AutoMovement(BaseEntity(), flInterval, (pTarget) ? pTarget->BaseEntity() : NULL, pTraceResult);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_NPC::TaskComplete(  bool fIgnoreSetFailedCondition )
{
	EndTaskOverlay();

	if ( fIgnoreSetFailedCondition || !HasCondition(COND_TASK_FAILED) )
	{
		SetTaskStatus( TASKSTATUS_COMPLETE );
	}
}

void CAI_NPC::EndTaskOverlay()
{
	g_helpfunc.EndTaskOverlay(BaseEntity());
}

void CAI_NPC::SetIdealActivity( Activity NewActivity )
{
	g_helpfunc.SetIdealActivity(BaseEntity(), NewActivity);
}

bool CAI_NPC::IsCurSchedule( int schedId, bool fIdeal )	
{ 
	if ( GetCurSchedule() == NULL )
		return ( schedId == SCHED_NONE || schedId == AI_RemapToGlobal(SCHED_NONE) );

	schedId = ( AI_IdIsLocal( schedId ) ) ? 
							GetClassScheduleIdSpace()->ScheduleLocalToGlobal(schedId) : 
							schedId;
	if ( fIdeal )
		return ( schedId == m_IdealSchedule );

	return ( GetCurSchedule()->GetId() == schedId ); 
}


//=========================================================
// FacingIdeal - tells us if a npc is facing its ideal
// yaw. Created this function because many spots in the
// code were checking the yawdiff against this magic
// number. Nicer to have it in one place if we're gonna
// be stuck with it.
//=========================================================
bool CAI_NPC::FacingIdeal( void )
{
	if ( fabs( GetMotor()->DeltaIdealYaw() ) <= 0.006 )//!!!BUGBUG - no magic numbers!!!
	{
		return true;
	}

	return false;
}

//=========================================================
// GetTask - returns a pointer to the current 
// scheduled task. NULL if there's a problem.
//=========================================================
const Task_t *CAI_NPC::GetTask( void ) 
{
	int iScheduleIndex = GetScheduleCurTaskIndex();
	if ( !GetCurSchedule() ||  iScheduleIndex < 0 || iScheduleIndex >= GetCurSchedule()->NumTasks() )
		// iScheduleIndex is not within valid range for the NPC's current schedule.
		return NULL;

	return &GetCurSchedule()->GetTaskList()[ iScheduleIndex ];
}


//-----------------------------------------------------------------------------

Activity CAI_NPC::TranslateActivity( Activity idealActivity, Activity *pIdealWeaponActivity )
{
	const int MAX_TRIES = 5;
	int count = 0;

	bool bIdealWeaponRequired = false;
	Activity idealWeaponActivity;
	Activity baseTranslation;
	bool bWeaponRequired = false;
	Activity weaponTranslation;
	Activity last;
	Activity current;

	idealWeaponActivity = Weapon_TranslateActivity( idealActivity, &bIdealWeaponRequired );
	if ( pIdealWeaponActivity )
		*pIdealWeaponActivity = idealWeaponActivity;

	baseTranslation	  = idealActivity;
	weaponTranslation = idealActivity;
	last			  = idealActivity;
	while ( count++ < MAX_TRIES )
	{
		current = NPC_TranslateActivity( last );
		if ( current != last )
			baseTranslation = current;

		weaponTranslation = Weapon_TranslateActivity( current, &bWeaponRequired );

		if ( weaponTranslation == last )
			break;

		last = weaponTranslation;
	}
	AssertMsg( count < MAX_TRIES, "Circular activity translation!" );

	if ( last == ACT_SCRIPT_CUSTOM_MOVE )
		return ACT_SCRIPT_CUSTOM_MOVE;
	
	if ( HaveSequenceForActivity( weaponTranslation ) )
		return weaponTranslation;
	
	if ( bWeaponRequired )
	{
		// only complain about an activity once
		static CUtlVector< Activity > sUniqueActivities;

		if (!sUniqueActivities.Find( weaponTranslation))
		{
			sUniqueActivities.AddToTail( weaponTranslation );
		}
	}

	if ( baseTranslation != weaponTranslation && HaveSequenceForActivity( baseTranslation ) )
		return baseTranslation;

	if ( idealWeaponActivity != baseTranslation && HaveSequenceForActivity( idealWeaponActivity ) )
		return idealActivity;

	if ( idealActivity != idealWeaponActivity && HaveSequenceForActivity( idealActivity ) )
		return idealActivity;

	Assert( !HaveSequenceForActivity( idealActivity ) );
	if ( idealActivity == ACT_RUN )
	{
		idealActivity = ACT_WALK;
	}
	else if ( idealActivity == ACT_WALK )
	{
		idealActivity = ACT_RUN;
	}

	return idealActivity;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_NPC::HandleInteraction(int interactionType, void *data, CBaseEntity* sourceEnt)
{
	if ( interactionType == g_interactionBarnacleVictimGrab )
	{
		// Make the victim stop thinking so they're as good as dead without 
		// shocking the system by destroying the entity.
		StopLoopingSounds();
		BarnacleDeathSound();
 		SetThink( NULL );

		// Gag the NPC so they won't talk anymore
		AddSpawnFlags( SF_NPC_GAG );

		// Drop any weapon they're holding
		CEntity *cent = GetActiveWeapon();
		if ( cent )
		{
			Weapon_Drop( cent->BaseEntity() );
		}

		return true;
	}

	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}

//-----------------------------------------------------------------------------
// TASK_CLEAR_HINTNODE
//-----------------------------------------------------------------------------
void CAI_NPC::ClearHintNode( float reuseDelay )
{
	if ( *(m_pHintNode.ptr) )
	{
		CE_AI_Hint *phint = (CE_AI_Hint *)CEntity::Instance(m_pHintNode);
		if ( phint->IsLockedBy(this) )
			phint->Unlock(reuseDelay);
		m_pHintNode.ptr = NULL;
	}
}


void CAI_NPC::SetHintNode( CE_AI_Hint *pHintNode )
{
	if(pHintNode)
		m_pHintNode->Set(pHintNode->BaseEntity());
	else
		m_pHintNode->Set(NULL);
	//m_pHintNode.ptr->Set(pHintNode);
	//*(m_pHintNode.ptr) = pHintNode;
}


//-----------------------------------------------------------------------------
// Purpose: Written by subclasses macro to load schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_NPC::LoadSchedules(void)
{
	return true;
}

//-----------------------------------------------------------------------------

bool CAI_NPC::LoadedSchedules(void)
{
	return true;
}


void CAI_NPC::AddActivityToSR(const char *actName, int actID)
{
	//g_helpfunc.AddActivityToSR(actName,actID);
	Assert( m_pActivitySR );
	if ( !m_pActivitySR )
		return;

	// technically order isn't dependent, but it's too damn easy to forget to add new ACT_'s to all three lists.

	// NOTE: This assertion generally means that the activity enums are out of order or that new enums were not added to all
	//		 relevant tables.  Make sure that you have included all new enums in:
	//			game_shared/ai_activity.h
	//			game_shared/activitylist.cpp
	//			dlls/ai_activity.cpp

	static int lastActID = -2;
	Assert( actID >= LAST_SHARED_ACTIVITY || actID == lastActID + 1 || actID == ACT_INVALID );
	lastActID = actID;

	m_pActivitySR->AddString(actName, actID);
	*(m_iNumActivities) += 1;
}

void CAI_NPC::AddEventToSR(const char *eventName, int eventID) 
{
	Assert( m_pEventSR );

	m_pEventSR->AddString( eventName, eventID );
	*(m_iNumEvents) += 1;
}

//-----------------------------------------------------------------------------
// Purpose: Given and schedule name, return the schedule ID
//-----------------------------------------------------------------------------
int CAI_NPC::GetScheduleID(const char* schedName)
{
	return GetSchedulingSymbols()->ScheduleSymbolToId(schedName);
}

//-----------------------------------------------------------------------------
// Purpose: Given and condition name, return the condition ID
//-----------------------------------------------------------------------------
int CAI_NPC::GetConditionID(const char* condName)
{
	return GetSchedulingSymbols()->ConditionSymbolToId(condName);
}

//-----------------------------------------------------------------------------
// Purpose: Given and activity name, return the activity ID
//-----------------------------------------------------------------------------
int CAI_NPC::GetActivityID(const char* actName) 
{
	Assert( m_pActivitySR );
	if ( !m_pActivitySR )
		return ACT_INVALID;

	return m_pActivitySR->GetStringID(actName);
}

//-----------------------------------------------------------------------------
// Purpose: Given and task name, return the task ID
//-----------------------------------------------------------------------------
int CAI_NPC::GetTaskID(const char* taskName)
{
	return GetSchedulingSymbols()->TaskSymbolToId( taskName );
}

//-----------------------------------------------------------------------------

bool CAI_NPC::OccupyStrategySlotRange( int slotIDStart, int slotIDEnd )
{
	// If I'm not in a squad a I don't fill slots
	return ( !GetSquad() || GetSquad()->OccupyStrategySlotRange( GetEnemy(), slotIDStart, slotIDEnd, m_iMySquadSlot.ptr ) );

}

void CAI_NPC::CallNPCThink()
{
	(BaseEntity()->*func_CallNPCThink)();
}

Navigation_t CAI_NPC::GetNavType() const
{
	return GetNavigator()->GetNavType();
}

void CAI_NPC::SetNavType( Navigation_t navType )
{
	GetNavigator()->SetNavType( navType );
}

//-----------------------------------------------------------------------------
bool CAI_NPC::IsCustomInterruptConditionSet( int nCondition )
{
	int interrupt = InterruptFromCondition( nCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return false;
	}
	
	return m_CustomInterruptConditions->IsBitSet( interrupt );
}

//-----------------------------------------------------------------------------
// Purpose: Sets a flag in the custom interrupt flags, translating the condition
//			to the proper global space, if necessary
//-----------------------------------------------------------------------------
void CAI_NPC::SetCustomInterruptCondition( int nCondition )
{
	int interrupt = InterruptFromCondition( nCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return;
	}
	
	m_CustomInterruptConditions->Set( interrupt );
}

//-----------------------------------------------------------------------------
// Purpose: Clears a flag in the custom interrupt flags, translating the condition
//			to the proper global space, if necessary
//-----------------------------------------------------------------------------
void CAI_NPC::ClearCustomInterruptCondition( int nCondition )
{
	int interrupt = InterruptFromCondition( nCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return;
	}
	
	m_CustomInterruptConditions->Clear( interrupt );
}


//-----------------------------------------------------------------------------
// Purpose: Clears all the custom interrupt flags.
//-----------------------------------------------------------------------------
void CAI_NPC::ClearCustomInterruptConditions()
{
	m_CustomInterruptConditions->ClearAll();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_NPC::IsMoving( void ) 
{ 
	return GetNavigator()->IsGoalSet(); 
}


//=========================================================
// ClearSchedule - blanks out the caller's schedule pointer
// and index.
//=========================================================
void CAI_NPC::ClearSchedule( const char *szReason )
{
	m_ScheduleState->timeCurTaskStarted = m_ScheduleState->timeStarted = 0;
	m_ScheduleState->bScheduleWasInterrupted = true;
	SetTaskStatus( TASKSTATUS_NEW );
	m_IdealSchedule = SCHED_NONE;
	*(m_pSchedule.ptr) =  NULL;
	ResetScheduleCurTaskIndex();
	m_InverseIgnoreConditions->SetAll();

}

bool CAI_NPC::IsWaitFinished()
{
	return ( gpGlobals->curtime >= m_flWaitFinished );
}

//-----------------------------------------------------------------------------

float CAI_NPC::SetWait( float minWait, float maxWait )
{
	int minThinks = Ceil2Int( minWait * 10 );

	if ( maxWait == 0.0 )
	{
		m_flWaitFinished = gpGlobals->curtime + ( 0.1 * minThinks );
	}
	else
	{
		if ( minThinks == 0 ) // enginerandom 0..n is almost certain to not return 0
			minThinks = 1;
		int maxThinks = Ceil2Int( maxWait * 10 );

		m_flWaitFinished = gpGlobals->curtime + ( 0.1 * enginerandom->RandomInt( minThinks, maxThinks ) );
	}
	return m_flWaitFinished;
}


CAI_NPC *CAI_NPC::GetInteractionPartner( void )
{
	if ( m_hInteractionPartner.ptr->Get() == NULL )
		return NULL;

	return m_hInteractionPartner.ptr->Get()->MyNPCPointer();
}




//=========================================================
// SetEyePosition
//
// queries the npc's model for $eyeposition and copies
// that vector to the npc's m_vDefaultEyeOffset and m_vecViewOffset
//
//=========================================================
void CAI_NPC::SetDefaultEyeOffset ( void )
{
	if  ( GetModelPtr() )
	{
		GetEyePosition( GetModelPtr(), m_vDefaultEyeOffset );

		if ( *(m_vDefaultEyeOffset) == vec3_origin )
		{
			if ( Classify() != CLASS_NONE )
			{
				DevMsg( "WARNING: %s(%s) has no eye offset in .qc!\n", GetClassname(), STRING(GetModelName()) );
			}
			VectorAdd( WorldAlignMins(), WorldAlignMaxs(), m_vDefaultEyeOffset );
			*(m_vDefaultEyeOffset)  *= 0.75;
		}
	}
	else
		m_vDefaultEyeOffset = vec3_origin;

	SetViewOffset( m_vDefaultEyeOffset );

}


//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_NPC::ConditionInterruptsCurSchedule( int iCondition )
{	
	if( !GetCurSchedule() )
	{
		return false;
	}

	int interrupt = InterruptFromCondition( iCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return false;
	}
	return ( GetCurSchedule()->HasInterrupt( interrupt ) );
}

bool CAI_NPC::ConditionInterruptsSchedule( int localScheduleID, int iCondition )
{
	CAI_Schedule *pSchedule = GetSchedule( localScheduleID );
	if ( !pSchedule )
		return false;

	int interrupt = InterruptFromCondition( iCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return false;
	}
	return ( pSchedule->HasInterrupt( interrupt ) );
}

void CAI_NPC::ClearTransientConditions()
{
	// if the npc didn't use these conditions during the above call to MaintainSchedule()
	// we throw them out cause we don't want them sitting around through the lifespan of a schedule
	// that doesn't use them.
	ClearCondition( COND_LIGHT_DAMAGE  );
	ClearCondition( COND_HEAVY_DAMAGE );
	ClearCondition( COND_PHYSICS_DAMAGE );
	ClearCondition( COND_PLAYER_PUSHING );
}


const Vector &CAI_NPC::GetEnemyLKP() const
{
	CBaseEntity *cbase = (const_cast<CAI_NPC *>(this))->GetEnemy_CBase();
	return (const_cast<CAI_NPC *>(this))->GetEnemies()->LastKnownPosition( cbase );
}

float CAI_NPC::GetEnemyLastTimeSeen() const
{
	CBaseEntity *cbase = (const_cast<CAI_NPC *>(this))->GetEnemy_CBase();
	return (const_cast<CAI_NPC *>(this))->GetEnemies()->LastTimeSeen(cbase);
}


//-----------------------------------------------------------------------------
// Purpose: For non-looping animations that may be replayed sequentially (like attacks)
//			Set the activity to ACT_RESET if this is a replay, otherwise just set ideal activity
// Input  : newIdealActivity - desired ideal activity
//-----------------------------------------------------------------------------
void CAI_NPC::ResetIdealActivity( Activity newIdealActivity )
{
	if ( m_Activity == newIdealActivity )
	{
		m_Activity = ACT_RESET;
	}

	SetIdealActivity( newIdealActivity );
}

float CAI_NPC::GetStepDownMultiplier() const
{
	return (*(m_pNavigator.ptr))->GetStepDownMultiplier();
}

//-----------------------------------------------------------------------------
// Purpose: For a specific delta, add a turn gesture and set the yaw speed
// Input  : yaw delta
//-----------------------------------------------------------------------------


bool CAI_NPC::UpdateTurnGesture( void )
{
	float flYD = GetMotor()->DeltaIdealYaw();
	return GetMotor()->AddTurnGesture( flYD );
}

Activity CAI_NPC::GetStoppedActivity( void )
{
	if (GetNavigator()->IsGoalActive())
	{
		Activity activity = GetNavigator()->GetArrivalActivity();

		if (activity > ACT_RESET)
		{
			return activity;
		}
	}

	return ACT_IDLE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAI_NPC::GetScriptCustomMoveSequence( void )
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;

	CEAI_ScriptedSequence *_m_hCine = Get_m_hCine();
	// If we have a scripted sequence entity, use it's custom move
	if ( _m_hCine != NULL )
	{
		iSequence = LookupSequence( STRING( *(_m_hCine->m_iszCustomMove) ) );
		if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		{
			DevMsg( "SCRIPT_CUSTOM_MOVE: %s has no sequence:%s\n", GetClassname(), STRING(*(_m_hCine->m_iszCustomMove)) );
		}
	}
	else if ( *(m_iszSceneCustomMoveSeq) != NULL_STRING )
	{
		// Otherwise, use the .vcd custom move
		iSequence = LookupSequence( STRING( *(m_iszSceneCustomMoveSeq) ) );
		if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		{
			Warning( "SCRIPT_CUSTOM_MOVE: %s failed scripted custom move. Has no sequence called: %s\n", GetClassname(), STRING(*(m_iszSceneCustomMoveSeq)) );
		}
	}

	// Failed? Use walk.
	if ( iSequence == ACTIVITY_NOT_AVAILABLE )
	{
		iSequence = SelectWeightedSequence( ACT_WALK );
	}

	return iSequence;
}

void CAI_NPC::SetTarget( CBaseEntity *pTarget )
{
	m_hTargetEnt.ptr->Set(pTarget);
}

//-----------------------------------------------------------------------------

void CAI_NPC::SetHintGroup( string_t newGroup, bool bHintGroupNavLimiting )	
{ 
	string_t oldGroup = m_strHintGroup;
	m_strHintGroup = newGroup;
	m_bHintGroupNavLimiting = bHintGroupNavLimiting;

	if ( oldGroup != newGroup )
		OnChangeHintGroup( oldGroup, newGroup );

}


//-----------------------------------------------------------------------------

bool CAI_NPC::IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos, 
							   float maxUp, float maxDown, float maxDist ) const
{
	if ((endPos.z - startPos.z) > maxUp + 0.1) 
		return false;
	if ((startPos.z - endPos.z) > maxDown + 0.1) 
		return false;

	if ((apex.z - startPos.z) > maxUp * 1.25 ) 
		return false;

	float dist = (startPos - endPos).Length();
	if ( dist > maxDist + 0.1) 
		return false;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the target entity
// Input  :
// Output :
//-----------------------------------------------------------------------------
CEntity *CAI_NPC::GetNavTargetEntity(void)
{
	if ( GetNavigator()->GetGoalType() == GOALTYPE_ENEMY )
		return m_hEnemy;
	else if ( GetNavigator()->GetGoalType() == GOALTYPE_TARGETENT )
		return m_hTargetEnt;
	return NULL;
}



//-----------------------------------------------------------------------------
// Purpose: Called by the navigator to initiate the opening of a prop_door
//			that is in our way.
//-----------------------------------------------------------------------------
void CAI_NPC::OpenPropDoorBegin( CPropDoor *pDoor )
{
	OpenPropDoorNow( pDoor );
}


//-----------------------------------------------------------------------------
// Purpose: Called when we are trying to open a prop_door and it's time to start
//			the door moving. This is called either in response to an anim event
//			or as a fallback when we don't have an appropriate open activity.
//-----------------------------------------------------------------------------
void CAI_NPC::OpenPropDoorNow( CPropDoor *pDoor )
{
	pDoor->NPCOpenDoor(this);

	m_flMoveWaitFinished = gpGlobals->curtime + pDoor->GetOpenInterval();
}

//=========================================================
// VecToYaw - turns a directional vector into a yaw value
// that points down that vector.
//=========================================================
float CAI_NPC::VecToYaw( const Vector &vecDir )
{
	if (vecDir.x == 0 && vecDir.y == 0 && vecDir.z == 0)
		return GetLocalAngles().y;

	return UTIL_VecToYaw( vecDir );
}

int CAI_NPC::FlyMove( const Vector& pfPosition, unsigned int mask )
{
	Vector		oldorg, neworg;
	trace_t		trace;

	// try the move	
	VectorCopy( GetAbsOrigin(), oldorg );
	VectorAdd( oldorg, pfPosition, neworg );
	UTIL_TraceEntity( this, oldorg, neworg, mask, &trace );				
	if (trace.fraction == 1)
	{
		if ( (GetFlags() & FL_SWIM) && enginetrace->GetPointContents(trace.endpos) == CONTENTS_EMPTY )
			return false;	// swim monster left water

		SetAbsOrigin( trace.endpos );
		PhysicsTouchTriggers();
		return true;
	}

	return false;
}



int CAI_NPC::WalkMove( const Vector& vecPosition, unsigned int mask )
{	
	if ( GetFlags() & (FL_FLY | FL_SWIM) )
	{
		return FlyMove( vecPosition, mask );
	}

	if ( (GetFlags() & FL_ONGROUND) == 0 )
	{
		return 0;
	}

	trace_t	trace;
	Vector oldorg, neworg, end;
	Vector move( vecPosition[0], vecPosition[1], 0.0f );
	VectorCopy( GetAbsOrigin(), oldorg );
	VectorAdd( oldorg, move, neworg );

	// push down from a step height above the wished position
	float flStepSize = sv_stepsize->GetFloat();
	neworg[2] += flStepSize;
	VectorCopy(neworg, end);
	end[2] -= flStepSize*2;

	UTIL_TraceEntity( this, neworg, end, mask, &trace );
	if ( trace.allsolid )
		return false;

	if (trace.startsolid)
	{
		neworg[2] -= flStepSize;
		UTIL_TraceEntity( this, neworg, end, mask, &trace );
		if ( trace.allsolid || trace.startsolid )
			return false;
	}

	if (trace.fraction == 1)
	{
		// if monster had the ground pulled out, go ahead and fall
		if ( GetFlags() & FL_PARTIALGROUND )
		{
			SetAbsOrigin( oldorg + move );
			PhysicsTouchTriggers();
			SetGroundEntity( NULL );
			return true;
		}
	
		return false;		// walked off an edge
	}

	// check point traces down for dangling corners
	SetAbsOrigin( trace.endpos );

	if (UTIL_CheckBottom( this, NULL, flStepSize ) == 0)
	{
		if ( GetFlags() & FL_PARTIALGROUND )
		{	
			// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			PhysicsTouchTriggers();
			return true;
		}

		// Reset to original position
		SetAbsOrigin( oldorg );
		return false;
	}

	if ( GetFlags() & FL_PARTIALGROUND )
	{
		// Con_Printf ("back on ground\n"); 
		RemoveFlag( FL_PARTIALGROUND );
	}

	// the move is ok
	SetGroundEntity( trace.m_pEnt );
	PhysicsTouchTriggers();
	return true;
}


void CAI_NPC::RememberUnreachable(CEntity *pEntity, float duration )
{
	if ( pEntity == GetEnemy() )
	{
		ForceChooseNewEnemy();
	}

	CBaseEntity *cbase = (pEntity) ? pEntity->BaseEntity() : NULL;

	const float NPC_UNREACHABLE_TIMEOUT = ( duration > 0.0 ) ? duration : 3;
	
	CUtlVector<UnreachableEnt_t> *data = m_UnreachableEnts;
	// Only add to list if not already on it
	for (int i=data->Size()-1;i>=0;i--)
	{
		// If record already exists just update mark time
		if (cbase == data->Element(i).hUnreachableEnt)
		{
			data->Element(i).fExpireTime	 = gpGlobals->curtime + NPC_UNREACHABLE_TIMEOUT;
			data->Element(i).vLocationWhenUnreachable = pEntity->GetAbsOrigin();
			return;
		}
	}

	// Add new unreachabe entity to list
	int nNewIndex = data->AddToTail();
	data->Element(nNewIndex).hUnreachableEnt = cbase;
	data->Element(nNewIndex).fExpireTime	 = gpGlobals->curtime + NPC_UNREACHABLE_TIMEOUT;
	data->Element(nNewIndex).vLocationWhenUnreachable = pEntity->GetAbsOrigin();
}

bool CAI_NPC::HasStrategySlotRange( int slotIDStart, int slotIDEnd )
{
	// If I wasn't taking up a squad slot I'm done
	if (m_iMySquadSlot < slotIDStart || m_iMySquadSlot > slotIDEnd)
	{
		return false;
	}
	return true;
}

void CAI_NPC::TaskMovementComplete( void )
{
	switch( GetTaskStatus() )
	{
	case TASKSTATUS_NEW:
	case TASKSTATUS_RUN_MOVE_AND_TASK:
		SetTaskStatus( TASKSTATUS_RUN_TASK );
		break;

	case TASKSTATUS_RUN_MOVE:
		TaskComplete();
		break;

	case TASKSTATUS_RUN_TASK:
		// FIXME: find out how to safely restart movement
		//Warning( "Movement completed twice!\n" );
		//Assert( 0 );
		break;

	case TASKSTATUS_COMPLETE:
		break;
	}

	// JAY: Put this back in.
	// UNDONE: Figure out how much of the timestep was consumed by movement
	// this frame and restart the movement/schedule engine if necessary
	if ( m_scriptState != SCRIPT_CUSTOM_MOVE_TO_MARK )
	{
		SetIdealActivity( GetStoppedActivity() );
	}

	// Advance past the last node (in case there is some event at this node)
	if ( GetNavigator()->IsGoalActive() )
	{
		GetNavigator()->AdvancePath();
	}

	// Now clear the path, it's done.
	GetNavigator()->ClearGoal();

	OnMovementComplete();
}

bool CAI_NPC::OccupyStrategySlot( int squadSlotID )
{
	return OccupyStrategySlotRange( squadSlotID, squadSlotID );
}

bool CAI_NPC::IsStrategySlotRangeOccupied( int slotIDStart, int slotIDEnd )
{
	return (GetSquad() && GetSquad()->IsStrategySlotRangeOccupied( GetEnemy(), slotIDStart, slotIDEnd ));
}

void CAI_NPC::VacateStrategySlot(void)
{
	if (GetSquad())
	{
		GetSquad()->VacateStrategySlot(GetEnemy(), m_iMySquadSlot);
		m_iMySquadSlot = SQUAD_SLOT_NONE;
	}
}

CCombatCharacter* CAI_NPC::GetEnemyCombatCharacterPointer()
{
	CEntity *enemy = GetEnemy();
	if ( enemy == NULL )
		return NULL;

	return enemy->MyCombatCharacterPointer();
}

int CAI_NPC::TaskIsRunning( void )
{
	if ( GetTaskStatus() != TASKSTATUS_COMPLETE &&
		 GetTaskStatus() != TASKSTATUS_RUN_MOVE )
		 return 1;

	return 0;
}

void CAI_NPC::SetEnemyOccluder(CEntity *pBlocker)
{
	m_hEnemyOccluder.ptr->Set((pBlocker) ? pBlocker->BaseEntity() : NULL);
}

void CAI_NPC::MarkEnemyAsEluded()
{
	GetEnemies()->MarkAsEluded( GetEnemy_CBase() );
}

void CAI_NPC::ClearEnemyMemory()
{
	GetEnemies()->ClearMemory( GetEnemy_CBase() );
}

void CAI_NPC::TestPlayerPushing( CBaseEntity *pPlayer )
{
	g_helpfunc.TestPlayerPushing(BaseEntity(), pPlayer);
}

void CAI_NPC::TestPlayerPushing( CEntity *pEntity )
{
	if ( HasSpawnFlags( SF_NPC_NO_PLAYER_PUSHAWAY ) )
		return;

	// Heuristic for determining if the player is pushing me away
	CPlayer *pPlayer = ToBasePlayer( pEntity );
	if ( pPlayer && !( pPlayer->GetFlags() & FL_NOTARGET ) )
	{
		if ( (pPlayer->m_nButtons & (IN_FORWARD|IN_BACK|IN_MOVELEFT|IN_MOVERIGHT)) || 
			 pPlayer->GetAbsVelocity().AsVector2D().LengthSqr() > 50*50 )
		{
			SetCondition( COND_PLAYER_PUSHING );
			Vector vecPush = GetAbsOrigin() - pPlayer->GetAbsOrigin();
			VectorNormalize( vecPush );
			CascadePlayerPush( vecPush, pPlayer->WorldSpaceCenter() );
		}
	}
}

void CAI_NPC::CascadePlayerPush( const Vector &push, const Vector &pushOrigin )
{
	//
	// Try to push any friends that are in the way.
	//
	float			hullWidth						= GetHullWidth();
	const Vector &	origin							= GetAbsOrigin();
	const Vector2D &origin2D						= origin.AsVector2D();

	const float		MIN_Z_TO_TRANSMIT				= GetHullHeight() * 0.5 + 0.1;
	const float		DIST_REQD_TO_TRANSMIT_PUSH_SQ	= Square( hullWidth * 5 + 0.1 );
	const float		DIST_FROM_PUSH_VECTOR_REQD_SQ	= Square( hullWidth + 0.1 );

	Vector2D		pushTestPoint = vec2_invalid;

	for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
	{
		CAI_NPC *pOther = g_AI_Manager.AccessAIs()[i];
		if ( pOther != this && pOther->IRelationType(BaseEntity()) == D_LI && !pOther->HasCondition( COND_PLAYER_PUSHING ) )
		{
			const Vector &friendOrigin = pOther->GetAbsOrigin();
			if ( fabsf( friendOrigin.z - origin.z ) < MIN_Z_TO_TRANSMIT &&
				 ( friendOrigin.AsVector2D() - origin.AsVector2D() ).LengthSqr() < DIST_REQD_TO_TRANSMIT_PUSH_SQ )
			{
				if ( pushTestPoint == vec2_invalid )
				{
					pushTestPoint = origin2D - pushOrigin.AsVector2D();
					// No normalize, since it wants to just be a big number and we can't be less that a hull away
					pushTestPoint *= 2000;
					pushTestPoint += origin2D;

				}
				float t;
				float distSq = CalcDistanceSqrToLine2D(  friendOrigin.AsVector2D(), origin2D, pushTestPoint, &t );
				if ( t > 0 && distSq < DIST_FROM_PUSH_VECTOR_REQD_SQ )
				{
					pOther->SetCondition( COND_PLAYER_PUSHING );
				}
			}
		}
	}
}

bool CAI_NPC::FindSpotForNPCInRadius( Vector *pResult, const Vector &vStartPos, CAI_NPC *pNPC, float radius, bool bOutOfPlayerViewcone )
{
	CPlayer *pPlayer = UTIL_GetNearestPlayer(pNPC->GetAbsOrigin());
	QAngle fan;

	fan.x = 0;
	fan.z = 0;

	for( fan.y = 0 ; fan.y < 360 ; fan.y += 18.0 )
	{
		Vector vecTest;
		Vector vecDir;

		AngleVectors( fan, &vecDir );

		vecTest = vStartPos + vecDir * radius;

		if ( bOutOfPlayerViewcone && pPlayer && !pPlayer->FInViewCone_Vector( vecTest ) )
			continue;

		trace_t tr;

		UTIL_TraceLine( vecTest, vecTest - Vector( 0, 0, 8192 ), MASK_SHOT, pNPC->BaseEntity(), COLLISION_GROUP_NONE, &tr );
		if( tr.fraction == 1.0 )
		{
			continue;
		}

		UTIL_TraceHull( tr.endpos,
						tr.endpos + Vector( 0, 0, 10 ),
						pNPC->GetHullMins(),
						pNPC->GetHullMaxs(),
						MASK_NPCSOLID,
						pNPC->BaseEntity(),
						COLLISION_GROUP_NONE,
						&tr );

		if( tr.fraction == 1.0 && pNPC->GetMoveProbe()->CheckStandPosition( tr.endpos, MASK_NPCSOLID ) )
		{
			*pResult = tr.endpos;
			return true;
		}
	}
	return false;
}

CSound *CAI_NPC::GetLoudestSoundOfType( int iType )
{
	return CE_CSoundEnt::GetLoudestSoundOfType( iType, EarPosition() );;
}

bool CAI_NPC::IsWeaponHolstered( void )
{
	if( !GetActiveWeapon() )
		return true;

	if( GetActiveWeapon()->IsEffectActive(EF_NODRAW) )
		return true;

	return false;
}

bool CAI_NPC::IsWeaponStateChanging( void )
{
	return ( m_iDesiredWeaponState == DESIREDWEAPONSTATE_CHANGING || m_iDesiredWeaponState == DESIREDWEAPONSTATE_CHANGING_DESTROY );
}

void CAI_NPC::SetIgnoreConditions( int *pConditions, int nConditions )
{
	for ( int i = 0; i < nConditions; ++i )
	{
		int iCondition = pConditions[i];
		int interrupt = InterruptFromCondition( iCondition );
		
		if ( interrupt == -1 )
		{
			Assert(0);
			continue;
		}
		
		m_InverseIgnoreConditions->Clear( interrupt ); // clear means ignore
	}
}

void CAI_NPC::ClearIgnoreConditions( int *pConditions, int nConditions )
{
	for ( int i = 0; i < nConditions; ++i )
	{
		int iCondition = pConditions[i];
		int interrupt = InterruptFromCondition( iCondition );
		
		if ( interrupt == -1 )
		{
			Assert(0);
			continue;
		}
		
		m_InverseIgnoreConditions->Set( interrupt ); // set means don't ignore
	}
}

void CAI_NPC::SetDistLook( float flDistLook )
{
	GetSenses()->SetDistLook( flDistLook );
}

float CAI_NPC::EnemyDistance( CEntity *pEnemy )
{
	Vector enemyDelta = pEnemy->WorldSpaceCenter() - WorldSpaceCenter();
	
	// NOTE: We ignore rotation for computing height.  Assume it isn't an effect
	// we care about, so we simply use OBBSize().z for height.  
	// Otherwise you'd do this:
	// pEnemy->CollisionProp()->WorldSpaceSurroundingBounds( &enemyMins, &enemyMaxs );
	// float enemyHeight = enemyMaxs.z - enemyMins.z;

	float enemyHeight = pEnemy->CollisionProp_Actual()->OBBSize().z;
	float myHeight = CollisionProp_Actual()->OBBSize().z;
	
	// max distance our centers can be apart with the boxes still overlapping
	float flMaxZDist = ( enemyHeight + myHeight ) * 0.5f;

	// see if the enemy is closer to my head, feet or in between
	if ( enemyDelta.z > flMaxZDist )
	{
		// enemy feet above my head, compute distance from my head to his feet
		enemyDelta.z -= flMaxZDist;
	}
	else if ( enemyDelta.z < -flMaxZDist )
	{
		// enemy head below my feet, return distance between my feet and his head
		enemyDelta.z += flMaxZDist;
	}
	else
	{
		// boxes overlap in Z, no delta
		enemyDelta.z = 0;
	}

	return enemyDelta.Length();
}

bool CAI_NPC::HasInteractionCantDie( void )
{
	return ( m_bCannotDieDuringInteraction && IsRunningDynamicInteraction() );
}

bool CAI_NPC::IsInPlayerSquad() const
{ 
	return ( GetSquad() && MAKE_STRING(GetSquad()->GetName()) == GetPlayerSquadName() && !CAI_Squad::IsSilentMember(this) ); 
}


bool CAI_NPC::IsSquadmateInSpread( const Vector &sourcePos, const Vector &targetPos, float flSpread, float maxDistOffCenter )
{
	CAI_Squad *squad = GetSquad();
	if( !squad ) 
		return false;

	AISquadIter_t iter;

	CBaseEntity *pEntity = squad->GetFirstMember( &iter );
	CAI_NPC *pSquadmate = (CAI_NPC *)CEntity::Instance(pEntity);
	while ( pSquadmate )
	{
		// Ignore squadmates that can't take damage. This is primarily to ignore npc_enemyfinders.
		if ( pSquadmate->m_takedamage != DAMAGE_NO )
		{
			if ( pSquadmate != this )
			{
				if ( PointInSpread( pSquadmate, sourcePos, targetPos, pSquadmate->GetAbsOrigin(), flSpread, maxDistOffCenter ) )
					return true;
			}
		}
		pEntity = squad->GetNextMember( &iter );
		pSquadmate = (CAI_NPC *)CEntity::Instance(pEntity);
	}
	return false;
}

bool CAI_NPC::PointInSpread( CCombatCharacter *pCheckEntity, const Vector &sourcePos, const Vector &targetPos, const Vector &testPoint, float flSpread, float maxDistOffCenter )
{
	float distOffLine = CalcDistanceToLine2D( testPoint.AsVector2D(), sourcePos.AsVector2D(), targetPos.AsVector2D() );
	if ( distOffLine < maxDistOffCenter )
	{
		Vector toTarget		= targetPos - sourcePos;
		float  distTarget	= VectorNormalize(toTarget);

		Vector toTest   = testPoint - sourcePos;
		float  distTest = VectorNormalize(toTest);
		// Only reject if target is on other side 
		if (distTarget > distTest)
		{
			toTarget.z = 0.0;
			toTest.z = 0.0;

			float dotProduct = DotProduct(toTarget,toTest);
			if (dotProduct > flSpread)
			{
				return true;
			}
			else if( dotProduct > 0.0f )
			{
				// If this guy is in front, do the hull/line test:
				if( pCheckEntity )
				{
					float flBBoxDist = NAI_Hull::Width( pCheckEntity->GetHullType() );
					flBBoxDist *= 1.414f; // sqrt(2)

					// !!!BUGBUG - this 2d check will stop a citizen shooting at a gunship or strider
					// if another citizen is between them, even though the strider or gunship may be
					// high up in the air (sjb)
					distOffLine = CalcDistanceToLine( testPoint, sourcePos, targetPos );
					if( distOffLine < flBBoxDist )
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

Vector CAI_NPC::GetActualShootTrajectory( const Vector &shootOrigin )
{
	if( !GetEnemy() )
		return GetShootEnemyDir( shootOrigin );

	// If we're above water shooting at a player underwater, bias some of the shots forward of
	// the player so that they see the cool bubble trails in the water ahead of them.
	if (GetEnemy()->IsPlayer() && (GetWaterLevel() != 3) && (GetEnemy()->GetWaterLevel() == 3))
	{
#if 1
		if (enginerandom->RandomInt(0, 4) < 3)
		{
			Vector vecEnemyForward;
			GetEnemy()->GetVectors( &vecEnemyForward, NULL, NULL );
			vecEnemyForward.z = 0;

			// Lead up to a second ahead of them unless they are moving backwards.
			Vector vecEnemyVelocity = GetEnemy()->GetSmoothedVelocity();
			VectorNormalize( vecEnemyVelocity );
			float flVelocityScale = vecEnemyForward.Dot( vecEnemyVelocity );
			if ( flVelocityScale < 0.0f )
			{
				flVelocityScale = 0.0f;
			}

			Vector vecAimPos = GetEnemy()->EyePosition() + ( 48.0f * vecEnemyForward ) + (flVelocityScale * GetEnemy()->GetSmoothedVelocity() );
			
			//vecAimPos.z = UTIL_WaterLevel( vecAimPos, vecAimPos.z, vecAimPos.z + 400.0f );

			Vector vecShotDir = vecAimPos - shootOrigin;
			VectorNormalize( vecShotDir );
			return vecShotDir;
		}
#else
		if (enginerandom->RandomInt(0, 4) < 3)
		{
			// Aim at a point a few feet in front of the player's eyes
			Vector vecEnemyForward;
			GetEnemy()->GetVectors( &vecEnemyForward, NULL, NULL );

			Vector vecAimPos = GetEnemy()->EyePosition() + (120.0f * vecEnemyForward );

			Vector vecShotDir = vecAimPos - shootOrigin;
			VectorNormalize( vecShotDir );

			CShotManipulator manipulator( vecShotDir );
			manipulator.ApplySpread( VECTOR_CONE_10DEGREES, 1 );
			vecShotDir = manipulator.GetResult();

			return vecShotDir;
		}
#endif
	}

	Vector vecProjectedPosition = GetActualShootPosition( shootOrigin );

	Vector shotDir = vecProjectedPosition - shootOrigin;
	VectorNormalize( shotDir );

	CollectShotStats( shootOrigin, shotDir );

	// NOW we have a shoot direction. Where a 100% accurate bullet should go.
	// Modify it by weapon proficiency.
	// construct a manipulator 
	CShotManipulator manipulator( shotDir );

	// Apply appropriate accuracy.
	bool bUsePerfectAccuracy = false;
	if ( GetEnemy() && GetEnemy()->Classify() == CLASS_BULLSEYE )
	{
		CNPC_Bullseye *pBullseye = dynamic_cast<CNPC_Bullseye*>(GetEnemy()); 
		if ( pBullseye && pBullseye->UsePerfectAccuracy() )
		{
			bUsePerfectAccuracy = true;
		}
	}

	if ( !bUsePerfectAccuracy )
	{
		CCombatWeapon *weapon = GetActiveWeapon();

		manipulator.ApplySpread( GetAttackSpread( (weapon)?weapon->BaseEntity():NULL, GetEnemy_CBase() ), GetSpreadBias( (weapon)?weapon->BaseEntity():NULL, GetEnemy_CBase() ) );
		shotDir = manipulator.GetResult();
	}

	// Look for an opportunity to make misses hit interesting things.
	CCombatCharacter *pEnemy;

	pEnemy = GetEnemy()->MyCombatCharacterPointer();

	if( pEnemy && pEnemy->ShouldShootMissTarget( BaseEntity() ) )
	{
		Vector vecEnd = shootOrigin + shotDir * 8192;
		trace_t tr;

		UTIL_TraceLine(shootOrigin, vecEnd, MASK_SHOT, BaseEntity(), COLLISION_GROUP_NONE, &tr);

		CEntity *cent = CEntity::Instance(tr.m_pEnt);
		if( tr.fraction != 1.0 && cent && cent->m_takedamage != DAMAGE_NO )
		{
			// Hit something we can harm. Just shoot it.
			return manipulator.GetResult();
		}

		// Find something interesting around the enemy to shoot instead of just missing.
		CEntity *pMissTarget = CEntity::Instance(pEnemy->FindMissTarget());
		
		if( pMissTarget )
		{
			shotDir = pMissTarget->WorldSpaceCenter() - shootOrigin;
			VectorNormalize( shotDir );
		}
	}

	return shotDir;

}

bool CAI_NPC::CheckPVSCondition()
{
	bool bInPVS = ( UTIL_FindClientInPVS( edict() ) != NULL ) || (UTIL_ClientPVSIsExpanded() && UTIL_FindClientInVisibilityPVS( edict() ));

	if ( bInPVS )
		SetCondition( COND_IN_PVS );
	else
		ClearCondition( COND_IN_PVS );

	return bInPVS;
}

extern ConVar *ai_lead_time;
Vector CAI_NPC::GetActualShootPosition( const Vector &shootOrigin )
{
	// Project the target's location into the future.
	Vector vecEnemyLKP = GetEnemyLKP();
	Vector vecEnemyOffset = GetEnemy()->BodyTarget( shootOrigin ) - GetEnemy()->GetAbsOrigin();
	Vector vecTargetPosition = vecEnemyOffset + vecEnemyLKP;

	// lead for some fraction of a second.
	return (vecTargetPosition + ( GetEnemy()->GetSmoothedVelocity() * ai_lead_time->GetFloat() ));
}

extern ConVar *ai_spread_cone_focus_time;
extern ConVar *ai_spread_defocused_cone_multiplier;

Vector CAI_NPC::GetAttackSpread( CBaseEntity *pWeapon, CBaseEntity *pTarget )
{
	Vector baseResult = BaseClass::GetAttackSpread( pWeapon, pTarget );

	AI_EnemyInfo_t *pEnemyInfo = GetEnemies()->Find( pTarget );
	if ( pEnemyInfo )
	{
		float timeToFocus = ai_spread_cone_focus_time->GetFloat();
		if ( timeToFocus > 0.0 )
		{
			float timeSinceValidEnemy = gpGlobals->curtime - pEnemyInfo->timeValidEnemy;
			if ( timeSinceValidEnemy < 0 )
				timeSinceValidEnemy = 0;
			if ( timeSinceValidEnemy < timeToFocus )
			{
				float coneMultiplier = ai_spread_defocused_cone_multiplier->GetFloat();
				if ( coneMultiplier > 1.0 )
				{
					float scale = 1.0 - timeSinceValidEnemy / timeToFocus;
					Assert( scale >= 0.0 && scale <= 1.0 );
					float multiplier = ( (coneMultiplier - 1.0) * scale ) + 1.0;
					baseResult *= multiplier;
				}
			}
		}
	}
	return baseResult;
}


extern ConVar *ai_shot_bias;
extern ConVar *ai_spread_pattern_focus_time;

float CAI_NPC::GetSpreadBias( CBaseEntity *pWeapon, CBaseEntity *pTarget )
{
	float bias = BaseClass::GetSpreadBias( pWeapon, pTarget );
	AI_EnemyInfo_t *pEnemyInfo = GetEnemies()->Find( pTarget );
	if ( ai_shot_bias->GetFloat() != 1.0 )
		bias = ai_shot_bias->GetFloat();
	if ( pEnemyInfo )
	{
		float timeToFocus = ai_spread_pattern_focus_time->GetFloat();
		if ( timeToFocus > 0.0 )
		{
			float timeSinceValidEnemy = gpGlobals->curtime - pEnemyInfo->timeValidEnemy;
			if ( timeSinceValidEnemy < 0.0f )
			{
				timeSinceValidEnemy = 0.0f;
			}
			float timeSinceReacquire = gpGlobals->curtime - pEnemyInfo->timeLastReacquired;
			if ( timeSinceValidEnemy < timeToFocus )
			{
				float scale = timeSinceValidEnemy / timeToFocus;
				Assert( scale >= 0.0 && scale <= 1.0 );
				bias *= scale;
			}
			else if ( timeSinceReacquire < timeToFocus ) // handled seperately as might be tuning seperately
			{
				float scale = timeSinceReacquire / timeToFocus;
				Assert( scale >= 0.0 && scale <= 1.0 );
				bias *= scale;
			}

		}
	}
	return bias;
}


bool CAI_NPC::ExitScriptedSequence()
{
	if ( m_lifeState == LIFE_DYING )
	{
		// is this legal?
		// BUGBUG -- This doesn't call Killed()
		SetIdealState( NPC_STATE_DEAD );
		return false;
	}

	CEAI_ScriptedSequence *cine = Get_m_hCine();
	if (cine)
	{
		cine->CancelScript( );
	}

	return true;
}

bool CAI_NPC::CouldShootIfCrouching( CEntity *pTarget )
{
	bool bWasStanding = !IsCrouching();
	Crouch();

	Vector vecTarget;
	if (GetActiveWeapon())
	{
		vecTarget = pTarget->BodyTarget( GetActiveWeapon()->GetLocalOrigin() );
	}
	else 
	{
		vecTarget = pTarget->BodyTarget( GetLocalOrigin() );
	}

	bool bResult = WeaponLOSCondition( GetLocalOrigin(), vecTarget, false );

	if ( bWasStanding )
	{
		Stand();
	}

	return bResult;
}

int CAI_NPC::SelectFlinchSchedule()
{
	if ( !HasCondition(COND_HEAVY_DAMAGE) )
		return SCHED_NONE;

	// If we've flinched recently, don't do it again. A gesture flinch will be played instead.
 	if ( HasMemory(bits_MEMORY_FLINCHED) )
		return SCHED_NONE;

	if ( !CanFlinch() )
		return SCHED_NONE;

	// Robin: This was in the original HL1 flinch code. Do we still want it?
	//if ( fabs( GetMotor()->DeltaIdealYaw() ) < (1.0 - m_flFieldOfView) * 60 ) // roughly in the correct direction
	//	return SCHED_TAKE_COVER_FROM_ORIGIN;

	// Heavy damage. Break out of my current schedule and flinch.
	Activity iFlinchActivity = GetFlinchActivity( true, false );
	if ( HaveSequenceForActivity( iFlinchActivity ) )
		return SCHED_BIG_FLINCH;

	/*
	// Not used anymore, because gesture flinches are played instead for heavy damage
	// taken shortly after we've already flinched full.
	//
	iFlinchActivity = GetFlinchActivity( false, false );
	if ( HaveSequenceForActivity( iFlinchActivity ) )
		return SCHED_SMALL_FLINCH;
	*/

	return SCHED_NONE;
}

const char *CAI_NPC::GetActivityName(int actID) 
{
	if ( actID == -1 )
		return "ACT_INVALID";

	// m_pActivitySR only contains public activities, ActivityList_NameForIndex() has them all
	const char *name = ActivityList_NameForIndex(actID);	

	if( !name )
	{
		AssertOnce( !"CAI_BaseNPC::GetActivityName() returning NULL!" );
	}

	return name;
}

bool CAI_NPC::HasConditionsToInterruptSchedule( int nLocalScheduleID )
{
	CAI_Schedule *pSchedule = GetSchedule( nLocalScheduleID );
	if ( !pSchedule )
		return false;

	CAI_ScheduleBits bitsMask;
	pSchedule->GetInterruptMask( &bitsMask );

	CAI_ScheduleBits bitsOut;
	m_Conditions->And( bitsMask, &bitsOut );
	
	return !bitsOut.IsAllClear();
}

CEntity *CAI_NPC::DropItem ( const char *pszItemName, Vector vecPos, QAngle vecAng )
{
	if ( !pszItemName )
	{
		DevMsg( "DropItem() - No item name!\n" );
		return NULL;
	}

	CEntity *pItem = CEntity::Create( pszItemName, vecPos, vecAng, BaseEntity() );

	if ( pItem )
	{
		IPhysicsObject *pPhys = pItem->VPhysicsGetObject();

		if ( pPhys )
		{
			// Add an extra push in a enginerandom direction
			Vector			vel		= RandomVector( -64.0f, 64.0f );
			AngularImpulse	angImp	= RandomAngularImpulse( -300.0f, 300.0f );

			vel[2] = 0.0f;
			pPhys->AddVelocity( &vel, &angImp );
		}
		else
		{
			// do we want this behavior to be default?! (sjb)
			pItem->ApplyAbsVelocityImpulse( GetAbsVelocity() );
			pItem->ApplyLocalAngularVelocityImpulse( AngularImpulse( 0, enginerandom->RandomFloat( 0, 100 ), 0 ) );
		}

		return pItem;
	}
	else
	{
		DevMsg( "DropItem() - Didn't create!\n" );
		return NULL;
	}
}

void CAI_NPC::SetSequenceByName( const char *szSequence )
{
	int iSequence = LookupSequence( szSequence );

	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
		SetSequenceById( iSequence );
	else
	{
		DevWarning( 2, "%s has no sequence to match request\n", GetClassname(), szSequence );
		SetSequence( 0 );	// Set to the reset anim (if it's there)
	}
}

void CAI_NPC::SetSequenceById( int iSequence )
{
	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( GetSequence() != iSequence || !SequenceLoops() )
		{
			SetCycle( 0 );
		}

		ResetSequence( iSequence );	// Set to the reset anim (if it's there)
		GetMotor()->RecalculateYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		DevWarning( 2, "%s invalid sequence requested\n", GetClassname() );
		SetSequence( 0 );	// Set to the reset anim (if it's there)
	}
}

int CAI_NPC::NumWeaponsInSquad( const char *pszWeaponClassname )
{
	string_t iszWeaponClassname = FindPooledString( pszWeaponClassname );

	CAI_Squad *squad = GetSquad();
	if( !squad )
	{
		if( GetActiveWeapon() && *(GetActiveWeapon()->m_iClassname) == iszWeaponClassname )
		{
			// I'm alone in my squad, but I do have this weapon.
			return 1;
		}

		return 0;
	}

	int count = 0;
	AISquadIter_t iter;
	CBaseEntity *pSquadmate = squad->GetFirstMember( &iter );
	while ( pSquadmate )
	{
		CAI_NPC *npc = (CAI_NPC *)CEntity::Instance(pSquadmate);
		if( npc->GetActiveWeapon() && *(npc->GetActiveWeapon()->m_iClassname) == iszWeaponClassname )
		{
			count++;
		}
		pSquadmate = squad->GetNextMember( &iter );
	}

	return count;
}

void CAI_NPC::AddToSquad( string_t name )
{
	g_AI_SquadManager->FindCreateSquad( this, name );
}

void CAI_NPC::RemoveFromSquad()
{
	CAI_Squad *squad = GetSquad();
	if ( squad )
	{
		squad->RemoveFromSquad( this, false );
		m_pSquad = NULL;
	}
}

bool CAI_NPC::SoundIsVisible( CSound *pSound )
{
	CBaseEntity *pBlocker = NULL;
	if ( !FVisible_Vector( pSound->GetSoundReactOrigin(), MASK_BLOCKLOS, &pBlocker ) )
	{
		// Is the blocker the sound owner?
		if ( pBlocker && pBlocker == pSound->m_hOwner )
			return true;

		return false;
	}
	return true;
}


void CAI_NPC::AddRelationship( const char *pszRelationship, CEntity *pActivator )
{
	// Parse the keyvalue data
	char parseString[1000];
	Q_strncpy(parseString, pszRelationship, sizeof(parseString));

	// Look for an entity string
	char *entityString = strtok(parseString," ");
	while (entityString)
	{
		// Get the disposition
		char *dispositionString = strtok(NULL," ");
		Disposition_t disposition = D_NU;
		if ( dispositionString )
		{
			if (!stricmp(dispositionString,"D_HT"))
			{
				disposition = D_HT;
			}
			else if (!stricmp(dispositionString,"D_FR"))
			{
				disposition = D_FR;
			}
			else if (!stricmp(dispositionString,"D_LI"))
			{
				disposition = D_LI;
			}
			else if (!stricmp(dispositionString,"D_NU"))
			{
				disposition = D_NU;
			}
			else
			{
				disposition = D_NU;
				Warning( "***ERROR***\nBad relationship type (%s) to unknown entity (%s)!\n", dispositionString,entityString );
				Assert( 0 );
				return;
			}
		}
		else
		{
			Warning("Can't parse relationship info (%s) - Expecting 'name [D_HT, D_FR, D_LI, D_NU] [1-99]'\n", pszRelationship );
			Assert(0);
			return;
		}

		// Get the priority
		char *priorityString	= strtok(NULL," ");
		int	priority = ( priorityString ) ? atoi(priorityString) : DEF_RELATIONSHIP_PRIORITY;

		bool bFoundEntity = false;

		// Try to get pointer to an entity of this name
		CEntity *entity = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, entityString );
		while( entity )
		{
			// make sure you catch all entities of this name.
			bFoundEntity = true;
			AddEntityRelationship(entity->BaseEntity(), disposition, priority );
			entity = g_helpfunc.FindEntityByName( entity, entityString );
		}

		if( !bFoundEntity )
		{
			// Need special condition for player as we can only have one
			if (!stricmp("player", entityString) || !stricmp("!player", entityString))
			{
				AddClassRelationship( CLASS_PLAYER, disposition, priority );
			}
			// Otherwise try to create one too see if a valid classname and get class type
			else
			{
				// HACKHACK:
				CEntity *pEntity = CanCreateEntityClass( entityString ) ? CreateEntityByName( entityString ) : NULL;
				if (pEntity)
				{
					AddClassRelationship( pEntity->Classify(), disposition, priority );
					UTIL_RemoveImmediate(pEntity->BaseEntity());
				}
				else
				{
					DevWarning( "Couldn't set relationship to unknown entity or class (%s)!\n", entityString );
				}
			}
		}
		// Check for another entity in the list
		entityString		= strtok(NULL," ");
	}
}


bool CAI_NPC::ChooseEnemy( void )
{
	//---------------------------------
	//
	// Gather initial conditions
	//

	CEntity *pInitialEnemy = GetEnemy();
	CEntity *pChosenEnemy  = pInitialEnemy;

	// Use memory bits in case enemy pointer altered outside this function, (e.g., ehandle goes NULL)
	bool fHadEnemy  	 = ( HasMemory( bits_MEMORY_HAD_ENEMY | bits_MEMORY_HAD_PLAYER ) );
	bool fEnemyWasPlayer = HasMemory( bits_MEMORY_HAD_PLAYER );
	bool fEnemyWentNull  = ( fHadEnemy && !pInitialEnemy );
	bool fEnemyEluded	 = ( fEnemyWentNull || ( pInitialEnemy && GetEnemies()->HasEludedMe( (pInitialEnemy)?pInitialEnemy->BaseEntity():NULL ) ) );

	//---------------------------------
	//
	// Establish suitability of choosing a new enemy
	//

	bool fHaveCondNewEnemy;
	bool fHaveCondLostEnemy;

	if ( !m_ScheduleState->bScheduleWasInterrupted && GetCurSchedule() && !FScheduleDone() )
	{
		Assert( InterruptFromCondition( COND_NEW_ENEMY ) == COND_NEW_ENEMY && InterruptFromCondition( COND_LOST_ENEMY ) == COND_LOST_ENEMY );
		fHaveCondNewEnemy  = GetCurSchedule()->HasInterrupt( COND_NEW_ENEMY );
		fHaveCondLostEnemy = GetCurSchedule()->HasInterrupt( COND_LOST_ENEMY );

		// See if they've been added as a custom interrupt
		if ( !fHaveCondNewEnemy )
		{
			fHaveCondNewEnemy = IsCustomInterruptConditionSet( COND_NEW_ENEMY );
		}
		if ( !fHaveCondLostEnemy )
		{
			fHaveCondLostEnemy = IsCustomInterruptConditionSet( COND_LOST_ENEMY );
		}
	}
	else
	{
		fHaveCondNewEnemy  = true; // not having a schedule is the same as being interruptable by any condition
		fHaveCondLostEnemy = true;
	}

	if ( !fEnemyWentNull )
	{
		if ( !fHaveCondNewEnemy && !( fHaveCondLostEnemy && fEnemyEluded ) )
		{
			// DO NOT mess with the npc's enemy pointer unless the schedule the npc is currently
			// running will be interrupted by COND_NEW_ENEMY or COND_LOST_ENEMY. This will
			// eliminate the problem of npcs getting a new enemy while they are in a schedule
			// that doesn't care, and then not realizing it by the time they get to a schedule
			// that does. I don't feel this is a good permanent fix.
			m_bSkippedChooseEnemy = true;

			return ( pChosenEnemy != NULL );
		}
	}


	m_bSkippedChooseEnemy = false;

	//---------------------------------
	//
	// Select a target
	//

	if ( ShouldChooseNewEnemy()	)
	{
		pChosenEnemy = CEntity::Instance(BestEnemy());
	}

	//---------------------------------
	//
	// React to result of selection
	//

	bool fChangingEnemy = ( pChosenEnemy != pInitialEnemy );

	if ( fChangingEnemy || fEnemyWentNull )
	{
		Forget( bits_MEMORY_HAD_ENEMY | bits_MEMORY_HAD_PLAYER );

		// Did our old enemy snuff it?
		if ( pInitialEnemy && !pInitialEnemy->IsAlive() )
		{
			SetCondition( COND_ENEMY_DEAD );
		}

		SetEnemy( (pChosenEnemy)?pChosenEnemy->BaseEntity():NULL );

		if ( fHadEnemy )
		{
			// Vacate any strategy slot on old enemy
			VacateStrategySlot();

			// Force output event for establishing LOS
			Forget( bits_MEMORY_HAD_LOS );
			// m_flLastAttackTime	= 0;
		}

		if ( !pChosenEnemy )
		{
			// Don't break on enemies going null if they've been killed
			if ( !HasCondition(COND_ENEMY_DEAD) )
			{
				SetCondition( COND_ENEMY_WENT_NULL );
			}

			if ( fEnemyEluded )
			{
				SetCondition( COND_LOST_ENEMY );
				LostEnemySound();
			}

			if ( fEnemyWasPlayer )
			{
				m_OnLostPlayer->FireOutput( pInitialEnemy, this );
			}
			m_OnLostEnemy->FireOutput( pInitialEnemy, this);
		}
		else
		{
			Remember( ( pChosenEnemy->IsPlayer() ) ? bits_MEMORY_HAD_PLAYER : bits_MEMORY_HAD_ENEMY );
		}
	}

	//---------------------------------

	return ( pChosenEnemy != NULL );
}

bool CAI_NPC::FScheduleDone ( void )
{
	Assert( GetCurSchedule() != NULL );
	
	if ( GetScheduleCurTaskIndex() == GetCurSchedule()->NumTasks() )
	{
		return true;
	}

	return false;
}


void CAI_NPC::MaintainActivity(void)
{
	if ( m_lifeState == LIFE_DEAD )
	{
		// Don't maintain activities if we're daid.
		// Blame Speyrer
		return;
	}

	if ((GetState() == NPC_STATE_SCRIPT))
	{
		// HACK: finish any transitions we might be playing before we yield control to the script
		if (GetActivity() != ACT_TRANSITION)
		{
			// Our animation state is being controlled by a script.
			return;
		}
	}

	if( m_IdealActivity == ACT_DO_NOT_DISTURB || !GetModelPtr() )
	{
		return;
	}

	// We may have work to do if we aren't playing our ideal activity OR if we
	// aren't playing our ideal sequence.
	if ((GetActivity() != m_IdealActivity) || (GetSequence() != m_nIdealSequence))
	{	
		bool bAdvance = false;

		// If we're in a transition activity, see if we are done with the transition.
		if (GetActivity() == ACT_TRANSITION)
		{
			// If the current sequence is finished, try to go to the next one
			// closer to our ideal sequence.
			if (IsSequenceFinished())
			{
				bAdvance = true;
			}
			// Else a transition sequence is in progress, do nothing.
		}
		// Else get a specific sequence for the activity and try to transition to that.
		else
		{
			// Save off a target sequence and translated activities to apply when we finish
			// playing all the transitions and finally arrive at our ideal activity.
			ResolveActivityToSequence(m_IdealActivity, m_nIdealSequence, m_IdealTranslatedActivity, m_IdealWeaponActivity);
			bAdvance = true;
		}
		
		if (bAdvance)
		{
			// Try to go to the next sequence closer to our ideal sequence.
			AdvanceToIdealActivity();
		}
	}
}

void CAI_NPC::ResolveActivityToSequence(Activity NewActivity, int &iSequence, Activity &translatedActivity, Activity &weaponActivity)
{
	iSequence = ACTIVITY_NOT_AVAILABLE;

	translatedActivity = TranslateActivity( NewActivity, &weaponActivity );

	if ( ( NewActivity == ACT_SCRIPT_CUSTOM_MOVE ) )
	{
		iSequence = GetScriptCustomMoveSequence();
	}
	else
	{
		iSequence = SelectWeightedSequence( translatedActivity );

		if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		{
			static CAI_NPC *pLastWarn;
			static Activity lastWarnActivity;
			static float timeLastWarn;

			if ( ( pLastWarn != this && lastWarnActivity != translatedActivity ) || gpGlobals->curtime - timeLastWarn > 5.0 )
			{
				DevWarning( "%s:%s:%s has no sequence for act:%s\n", GetClassname(), GetDebugName(), STRING( GetModelName() ), ActivityList_NameForIndex(translatedActivity) );
				pLastWarn = this;
				lastWarnActivity = translatedActivity;
				timeLastWarn = gpGlobals->curtime;
			}

			if ( translatedActivity == ACT_RUN )
			{
				translatedActivity = ACT_WALK;
				iSequence = SelectWeightedSequence( translatedActivity );
			}
		}
	}

	if ( iSequence == ACT_INVALID )
	{
		// Abject failure. Use sequence zero.
		iSequence = 0;
	}
}

void CAI_NPC::AdvanceToIdealActivity(void)
{
	// If there is a transition sequence between the current sequence and the ideal sequence...
	int nNextSequence = FindTransitionSequence(GetSequence(), m_nIdealSequence, NULL);
	if (nNextSequence != -1)
	{
		// We found a transition sequence or possibly went straight to
		// the ideal sequence.
		if (nNextSequence != m_nIdealSequence)
		{
//			DevMsg("%s: TRANSITION %s -> %s -> %s\n", GetClassname(), GetSequenceName(GetSequence()), GetSequenceName(nNextSequence), GetSequenceName(m_nIdealSequence));

			Activity eWeaponActivity = ACT_TRANSITION;
			Activity eTranslatedActivity = ACT_TRANSITION;

			// Figure out if the transition sequence has an associated activity that
			// we can use for our weapon. Do activity translation also.
			Activity eTransitionActivity = GetSequenceActivity(nNextSequence);
			if (eTransitionActivity != ACT_INVALID)
			{
				int nDiscard;
				ResolveActivityToSequence(eTransitionActivity, nDiscard, eTranslatedActivity, eWeaponActivity);
			}

			// Set activity and sequence to the transition stuff. Set the activity to ACT_TRANSITION
			// so we know we're in a transition.
			SetActivityAndSequence(ACT_TRANSITION, nNextSequence, eTranslatedActivity, eWeaponActivity);
		}
		else
		{
			//DevMsg("%s: IDEAL %s -> %s\n", GetClassname(), GetSequenceName(GetSequence()), GetSequenceName(m_nIdealSequence));

			// Set activity and sequence to the ideal stuff that was set up in MaintainActivity.
			SetActivityAndSequence(m_IdealActivity, m_nIdealSequence, m_IdealTranslatedActivity, m_IdealWeaponActivity);
		}
	}
	// Else go straight there to the ideal activity.
	else
	{
		//DevMsg("%s: Unable to get from sequence %s to %s!\n", GetClassname(), GetSequenceName(GetSequence()), GetSequenceName(m_nIdealSequence));
		SetActivity(m_IdealActivity);
	}
}


void CAI_NPC::SetActivityAndSequence(Activity NewActivity, int iSequence, Activity translatedActivity, Activity weaponActivity)
{
	m_translatedActivity = translatedActivity;

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( GetSequence() != iSequence || !SequenceLoops() )
		{
			//
			// Don't reset frame between movement phased animations
			if (!IsActivityMovementPhased( m_Activity ) || 
				!IsActivityMovementPhased( NewActivity ))
			{
				SetCycle( 0 );
			}
		}

		ResetSequence( iSequence );
		Weapon_SetActivity( weaponActivity, SequenceDuration( iSequence ) );
	}
	else
	{
		// Not available try to get default anim
		ResetSequence( 0 );
	}

	// Set the view position based on the current activity
	SetViewOffset( EyeOffset(m_translatedActivity) );

	if (m_Activity != NewActivity)
	{
		OnChangeActivity(NewActivity);
	}

	// NOTE: We DO NOT write the translated activity here.
	// This is to abstract the activity translation from the AI code.
	// As far as the code is concerned, a translation is merely a new set of sequences
	// that should be regarded as the activity in question.

	// Go ahead and set this so it doesn't keep trying when the anim is not present
	m_Activity = NewActivity;

	// this cannot be called until m_Activity stores NewActivity!
	GetMotor()->RecalculateYawSpeed();
}


bool CAI_NPC::ShouldMoveWait()
{
	return (m_flMoveWaitFinished > gpGlobals->curtime);
}

CEntity *CAI_NPC::PlayerInRange( const Vector &vecLocation, float flDist )
{
	// loop through all players
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CPlayer *pPlayer = UTIL_PlayerByIndex( i );

		if (pPlayer && (vecLocation - pPlayer->WorldSpaceCenter() ).Length2D() <= flDist)
		{
			return pPlayer;
		}
	}
	return NULL;
}

bool CAI_NPC::HasStrategySlot( int squadSlotID )
{
	// If I wasn't taking up a squad slot I'm done
	return (m_iMySquadSlot == squadSlotID);
}


void CAI_NPC::InitRelationshipTable(void)
{
	AddRelationship( STRING( (*(m_RelationshipString)) ), NULL );
}

float CAI_NPC::ThrowLimit(	const Vector &vecStart,
								const Vector &vecEnd,
								float		fGravity,
								float		fArcSize,
								const Vector &mins,
								const Vector &maxs,
								CEntity *pTarget,
								Vector		*jumpVel,
								CEntity **pBlocker)
{
	// Get my jump velocity
	Vector rawJumpVel	= CalcThrowVelocity(vecStart, vecEnd, fGravity, fArcSize);
	*jumpVel			= rawJumpVel;
	Vector vecFrom		= vecStart;

	// Calculate the total time of the jump minus a tiny fraction
	float jumpTime		= (vecStart - vecEnd).Length2D()/rawJumpVel.Length2D();
	float timeStep		= jumpTime / 10.0;

	Vector gravity = Vector(0,0,fGravity);

	// this loop takes single steps to the goal.
	for (float flTime = 0 ; flTime < jumpTime-0.1 ; flTime += timeStep )
	{
		// Calculate my position after the time step (average velocity over this time step)
		Vector nextPos = vecFrom + (rawJumpVel - 0.5 * gravity * timeStep) * timeStep;

		// If last time step make next position the target position
		if ((flTime + timeStep) > jumpTime)
		{
			nextPos = vecEnd;
		}

		trace_t tr;
		UTIL_TraceHull( vecFrom, nextPos, mins, maxs, MASK_SOLID, BaseEntity(), COLLISION_GROUP_NONE, &tr );

		if (tr.startsolid || tr.fraction < 1.0)
		{
			CEntity *pEntity = CEntity::Instance(tr.m_pEnt);

			// If we hit the target we are good to go!
			if (pEntity == pTarget)
			{
				return 0;
			}

			// ----------------------------------------------------------
			// If blocked by an npc remember
			// ----------------------------------------------------------
			*pBlocker = pEntity;

			// Return distance sucessfully traveled before block encountered
			return ((tr.endpos - vecStart).Length());
		}

		rawJumpVel  = rawJumpVel - gravity * timeStep;
		vecFrom		= nextPos;
	}
	return 0;
}

Vector CAI_NPC::CalcThrowVelocity(const Vector &startPos, const Vector &endPos, float fGravity, float fArcSize) 
{
	// Get the height I have to throw to get to the target
	float stepHeight = endPos.z - startPos.z;
	float throwHeight = 0;

	// -----------------------------------------------------------------
	// Now calcluate the distance to a point halfway between our current
	// and target position.  (the apex of our throwing arc)
	// -----------------------------------------------------------------
	Vector targetDir2D = endPos - startPos;
	targetDir2D.z = 0;

	float distance = VectorNormalize(targetDir2D);


	// If jumping up we want to throw a bit higher than the height diff	
	if (stepHeight > 0) 
	{
		throwHeight = stepHeight + fArcSize;	
	}	
	else
	{
		throwHeight = fArcSize;
	}
	// Make sure that I at least catch some air
	if (throwHeight < fArcSize)
	{
		throwHeight = fArcSize;
	}


	// -------------------------------------------------------------
	// calculate the vertical and horizontal launch velocities
	// -------------------------------------------------------------
	float velVert = (float)sqrt(2.0f*fGravity*throwHeight);

	float divisor = velVert;
	divisor += (float)sqrt((2.0f*(-fGravity)*(stepHeight-throwHeight)));

	float velHorz = (distance * fGravity)/divisor;

	// -----------------------------------------------------------
	// Make the horizontal throw vector and add vertical component
	// -----------------------------------------------------------
	Vector throwVel = targetDir2D * velHorz;
	throwVel.z = velVert;

	return throwVel;
}




//-------------------------------------------------------

bool NPC_CheckBrushExclude( CEntity *pEntity, CEntity *pBrush )
{
	CAI_NPC *pNPC = pEntity->MyNPCPointer();

	if ( pNPC )
	{
		return pNPC->GetMoveProbe()->ShouldBrushBeIgnored( pBrush->BaseEntity() );
	}

	return false;
}

bool AIStrongOpt( void )
{
	return ai_strong_optimizations->GetBool();
}








SH_DECL_MANUALHOOK3(OnCalcBaseMove, 0, 0, 0, bool, AILocalMoveGoal_t *, float , AIMoveResult_t *);
DECLARE_DEFAULTHANDLER_SUBCLASS(CAI_NPC, IAI_MovementSink, OnCalcBaseMove, bool, (AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult ), (pMoveGoal, distClear, pResult ));
DECLARE_HOOK_SUBCLASS(OnCalcBaseMove, CAI_NPC, IAI_MovementSink);
				
SH_DECL_MANUALHOOK3(OnObstructionPreSteer, 0, 0, 0, bool, AILocalMoveGoal_t *, float , AIMoveResult_t *);
DECLARE_DEFAULTHANDLER_SUBCLASS(CAI_NPC, IAI_MovementSink, OnObstructionPreSteer, bool, (AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult ), (pMoveGoal, distClear, pResult ));
DECLARE_HOOK_SUBCLASS(OnObstructionPreSteer, CAI_NPC, IAI_MovementSink);

SH_DECL_MANUALHOOK1(OnMoveBlocked, 0, 0, 0, bool, AIMoveResult_t *);
DECLARE_DEFAULTHANDLER_SUBCLASS(CAI_NPC, IAI_MovementSink, OnMoveBlocked, bool, (AIMoveResult_t *pResult), (pResult));
DECLARE_HOOK_SUBCLASS(OnMoveBlocked, CAI_NPC, IAI_MovementSink);

SH_DECL_MANUALHOOK3(OnFailedSteer, 0, 0, 0, bool, AILocalMoveGoal_t *, float , AIMoveResult_t *);
DECLARE_DEFAULTHANDLER_SUBCLASS(CAI_NPC, IAI_MovementSink, OnFailedSteer, bool, (AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult), (pMoveGoal, distClear, pResult));
DECLARE_HOOK_SUBCLASS(OnFailedSteer, CAI_NPC, IAI_MovementSink);

SH_DECL_MANUALHOOK3(OnFailedLocalNavigation, 0, 0, 0, bool, AILocalMoveGoal_t *, float, AIMoveResult_t *);
DECLARE_DEFAULTHANDLER_SUBCLASS(CAI_NPC, IAI_MovementSink, OnFailedLocalNavigation, bool, (AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult), (pMoveGoal, distClear, pResult));
DECLARE_HOOK_SUBCLASS(OnFailedLocalNavigation, CAI_NPC, IAI_MovementSink);

SH_DECL_MANUALHOOK3(OnInsufficientStopDist, 0, 0, 0, bool, AILocalMoveGoal_t *, float , AIMoveResult_t *);
DECLARE_DEFAULTHANDLER_SUBCLASS(CAI_NPC, IAI_MovementSink, OnInsufficientStopDist, bool, (AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult), (pMoveGoal, distClear, pResult));
DECLARE_HOOK_SUBCLASS(OnInsufficientStopDist, CAI_NPC, IAI_MovementSink);

SH_DECL_MANUALHOOK4(OnMoveExecuteFailed, 0, 0, 0, bool, const AILocalMoveGoal_t &, const AIMoveTrace_t &, AIMotorMoveResult_t, AIMoveResult_t *);
DECLARE_DEFAULTHANDLER_SUBCLASS(CAI_NPC, IAI_MovementSink, OnMoveExecuteFailed, bool, (const AILocalMoveGoal_t &move, const AIMoveTrace_t &trace, AIMotorMoveResult_t fMotorResult, AIMoveResult_t *pResult), (move, trace, fMotorResult, pResult));
DECLARE_HOOK_SUBCLASS(OnMoveExecuteFailed, CAI_NPC, IAI_MovementSink);

SH_DECL_MANUALHOOK0(CalcYawSpeed, 0, 0, 0, float);
DECLARE_DEFAULTHANDLER_SUBCLASS(CAI_NPC, IAI_MovementSink, CalcYawSpeed, float, (), ());
DECLARE_HOOK_SUBCLASS(CalcYawSpeed, CAI_NPC, IAI_MovementSink);

