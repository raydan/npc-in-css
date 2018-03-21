
#ifndef _INCLUDE_CDYNAMICPROP_H_
#define _INCLUDE_CDYNAMICPROP_H_

#include "CBreakableProp.h"

class CE_CDynamicProp : public CE_CBreakableProp
{
public:
	CE_DECLARE_CLASS( CE_CDynamicProp, CE_CBreakableProp );

	void PostConstructor();
	//bool AcceptInput(const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID);

	void AnimThink();
	void PropSetSequence( int nSequence );
	void FinishSetSequence( int nSequence );

public:
	static VALVE_BASEPTR		func_AnimThink;
public:
	DECLARE_DATAMAP(bool, m_bUseHitboxesForRenderBox);
	DECLARE_DATAMAP(int, m_iGoalSequence);
	DECLARE_DATAMAP(int, m_iTransitionDirection);


};


#endif
