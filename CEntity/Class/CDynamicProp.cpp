
#include "CDynamicProp.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(CDynamicProp, CE_CDynamicProp);


DEFINE_PROP(m_bUseHitboxesForRenderBox, CE_CDynamicProp);
DEFINE_PROP(m_iGoalSequence, CE_CDynamicProp);
DEFINE_PROP(m_iTransitionDirection, CE_CDynamicProp);


VALVE_BASEPTR CE_CDynamicProp::func_AnimThink = NULL;



void CE_CDynamicProp::PostConstructor()
{
	BaseClass::PostConstructor();
	if(func_AnimThink == NULL)
	{
		void *ptr = UTIL_FunctionFromName( GetDataDescMap_Real(), "CDynamicPropAnimThink");
		if(ptr)
		{
			memcpy(&func_AnimThink, &ptr, sizeof(void *));
		}
	}
	Assert(func_AnimThink);
}

/*bool CE_CDynamicProp::AcceptInput(const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID)
{
	string_t model_name = GetModelName();
	if(Q_stristr(STRING(model_name), "turret.mdl"))
	{
		if(strcmp(szInputName,"SetAnimation") == 0)
		{
			const char *value = Value.String();
			if(strcmp(value,"fire") == 0)
			{
				return true;
			}
		}
	}
	return BaseClass::AcceptInput(szInputName, pActivator, pCaller, Value, outputID);
}*/

void CE_CDynamicProp::PropSetSequence( int nSequence )
{
	m_iGoalSequence = nSequence;

	int nNextSequence;
	float nextCycle;
	float flInterval = 0.1f;

	if (GotoSequence( GetSequence(), GetCycle(), GetPlaybackRate(), m_iGoalSequence, nNextSequence, nextCycle, m_iTransitionDirection ))
	{
		FinishSetSequence( nNextSequence );
	}

	SetThink( &CE_CDynamicProp::AnimThink );
	if ( GetNextThink() <= gpGlobals->curtime )
		SetNextThink( gpGlobals->curtime + flInterval );
}

void CE_CDynamicProp::FinishSetSequence( int nSequence )
{
	SetCycle( 0 );
	m_flAnimTime = gpGlobals->curtime;
	ResetSequence( nSequence );
	ResetClientsideFrame();
	RemoveFlag( FL_STATICPROP );
	SetPlaybackRate( m_iTransitionDirection > 0 ? 1.0f : -1.0f );
	SetCycle( m_iTransitionDirection > 0 ? 0.0f : 0.999f );
}

void CE_CDynamicProp::AnimThink()
{
	(BaseEntity()->*func_AnimThink)();
}

