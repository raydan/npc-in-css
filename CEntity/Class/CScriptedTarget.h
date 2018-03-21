#ifndef _INCLUDE_CSCRIPTEDTARGET_H_
#define _INCLUDE_CSCRIPTEDTARGET_H_

#include "CEntity.h"
#include "CAI_NPC.h"

class CE_CScriptedTarget : public CAI_NPC
{
public:
	DECLARE_CLASS(CE_CScriptedTarget, CAI_NPC);

	void PostConstructor();

	CE_CScriptedTarget *NextScriptedTarget(void);

	void ScriptThink();

	float MoveSpeed(void) { return m_nMoveSpeed; }

	void TurnOn(void);
	void TurnOff(void);

private:
	static VALVE_BASEPTR		func_ScriptThink;

protected:
	DECLARE_DATAMAP(int, m_nMoveSpeed);
	DECLARE_DATAMAP(float, m_flPauseDuration);
	DECLARE_DATAMAP(float, m_flPauseDoneTime);
	DECLARE_DATAMAP(float, m_flEffectDuration);
	DECLARE_DATAMAP(COutputEvent, m_AtTarget);
	DECLARE_DATAMAP(COutputEvent, m_LeaveTarget);
	DECLARE_DATAMAP(int, m_iDisabled);
	DECLARE_DATAMAP(Vector, m_vLastPosition);

	
};


#endif
