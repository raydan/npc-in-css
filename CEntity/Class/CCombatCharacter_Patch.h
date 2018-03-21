
#ifndef _INCLUDE_CCOMBATCHARACTER_PATCH_H_
#define _INCLUDE_CCOMBATCHARACTER_PATCH_H_

#include "CEntity.h"
#include "CCombatCharacter.h"

class CCombatCharacter;

class CCombatCharacter_Patch : public CCombatCharacter
{
public:
	CE_DECLARE_CLASS(CCombatCharacter_Patch, CCombatCharacter);

public:
	virtual bool HandleInteraction( int interactionType, void *data, CBaseEntity* sourceEnt );
	virtual bool FVisible_Entity(CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL );


};


#endif
