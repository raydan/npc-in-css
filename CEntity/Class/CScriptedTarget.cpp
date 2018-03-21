
#include "CScriptedTarget.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(scripted_target, CE_CScriptedTarget);

DEFINE_PROP(m_nMoveSpeed, CE_CScriptedTarget);
DEFINE_PROP(m_flPauseDuration, CE_CScriptedTarget);
DEFINE_PROP(m_flPauseDoneTime, CE_CScriptedTarget);
DEFINE_PROP(m_flEffectDuration, CE_CScriptedTarget);
DEFINE_PROP(m_AtTarget, CE_CScriptedTarget);
DEFINE_PROP(m_LeaveTarget, CE_CScriptedTarget);
DEFINE_PROP(m_iDisabled, CE_CScriptedTarget);
DEFINE_PROP(m_vLastPosition, CE_CScriptedTarget);


VALVE_BASEPTR CE_CScriptedTarget::func_ScriptThink = NULL;

int *g_interactionScriptedTarget = NULL;

void CE_CScriptedTarget::PostConstructor()
{
	BaseClass::PostConstructor();
	if(func_ScriptThink == NULL)
	{
		void *ptr = UTIL_FunctionFromName( GetDataDescMap_Real(), "CScriptedTargetScriptThink");
		if(ptr)
		{
			memcpy(&func_ScriptThink, &ptr, sizeof(void *));
		}
	}
	Assert(func_ScriptThink);
}


CE_CScriptedTarget* CE_CScriptedTarget::NextScriptedTarget(void)
{
	// ----------------------------------------------------------------------
	// If I just hit my target, set how long I'm supposed to pause here
	// ----------------------------------------------------------------------
	if (*(m_flPauseDoneTime) == 0)
	{
		m_flPauseDoneTime = gpGlobals->curtime + m_flPauseDuration;
		m_AtTarget->FireOutput( GetTarget(), this );
	}

	// -------------------------------------------------------------
	// If I'm done pausing move on to next burn target
	// -------------------------------------------------------------
	if (gpGlobals->curtime >= m_flPauseDoneTime)
	{
		m_flPauseDoneTime = 0;

		// ----------------------------------------------------------
		//  Fire output that current Scripted target has been reached
		// ----------------------------------------------------------
		m_LeaveTarget->FireOutput( GetTarget(), this );

		// ------------------------------------------------------------
		//  Get next target.  
		// ------------------------------------------------------------
		CE_CScriptedTarget* pNextTarget = ((CE_CScriptedTarget*)GetNextTarget());

		// --------------------------------------------
		//	Fire output if last one has been reached
		// --------------------------------------------
		if (!pNextTarget)
		{
			TurnOff();
			SetTarget( NULL );
		}
		// ------------------------------------------------
		//	Otherwise, turn myself off, the next target on
		//  and pass on my target entity
		// ------------------------------------------------
		else
		{
			// ----------------------------------------------------
			//  Make sure there is a LOS between these two targets
			// ----------------------------------------------------
			trace_t tr;
			UTIL_TraceLine(GetAbsOrigin(), pNextTarget->GetAbsOrigin(), MASK_SHOT, BaseEntity(), COLLISION_GROUP_NONE, &tr);	
			if (tr.fraction != 1.0)
			{
				Warning( "WARNING: Scripted Target from (%s) to (%s) is occluded!\n",GetDebugName(),pNextTarget->GetDebugName() );
			}

			pNextTarget->TurnOn();
			pNextTarget->SetTarget( GetTarget()->BaseEntity() );

			SetTarget( NULL );
			TurnOff();
		}
		// --------------------------------------------
		//	Return new target
		// --------------------------------------------
		return pNextTarget;
	}
	// -------------------------------------------------------------
	//  Otherwise keep the same scripted target until pause is done
	// -------------------------------------------------------------
	else
	{
		return this;
	}
}


void CE_CScriptedTarget::TurnOff( void )
{
	SetThink( NULL );
	m_iDisabled	= true;

	// If I have a target entity, free him
	if (GetTarget())
	{
		CAI_NPC* pNPC = GetTarget()->MyNPCPointer();
		pNPC->DispatchInteraction( *(g_interactionScriptedTarget), NULL, NULL );
	}
}

void CE_CScriptedTarget::TurnOn( void )
{
	m_vLastPosition = GetAbsOrigin();
	SetThink( &CE_CScriptedTarget::ScriptThink );
	m_iDisabled		= false;
	SetNextThink( gpGlobals->curtime );
}

void CE_CScriptedTarget::ScriptThink()
{
	(BaseEntity()->*func_ScriptThink)();
}
