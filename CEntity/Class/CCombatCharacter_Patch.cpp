
#include "CCombatCharacter_Patch.h"


CE_LINK_ENTITY_TO_CLASS(CBaseCombatCharacter, CCombatCharacter_Patch);


extern int g_interactionBarnacleVictimReleased;

bool CCombatCharacter_Patch::HandleInteraction( int interactionType, void *data, CBaseEntity* sourceEnt )
{
	if(interactionType == g_interactionBarnacleVictimReleased )
	{
		// For now, throw away the NPC and leave the ragdoll.
		UTIL_Remove( this );
		return true;
	}
	return BaseClass::HandleInteraction(interactionType, data, sourceEnt);
}

bool CCombatCharacter_Patch::FVisible_Entity(CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker)
{
	if(Classify() == CLASS_BULLSEYE || CEntity::Instance(pEntity)->Classify() == CLASS_BULLSEYE)
	{
		bool ret = g_helpfunc.CBaseEntity_FVisible_Entity(BaseEntity(), pEntity, traceMask, ppBlocker);
		return ret;
	}
	return BaseClass::FVisible_Entity( pEntity, traceMask, ppBlocker );
}
