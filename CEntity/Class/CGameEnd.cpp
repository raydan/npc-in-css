
#include "CGameEnd.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(game_end, CE_CGameEnd);


bool CE_CGameEnd::AcceptInput(const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID)
{
	bool ret = BaseClass::AcceptInput(szInputName, pActivator, pCaller, Value, outputID);

	if(FStrEq(szInputName, "EndGame")) {
		engine->ServerCommand("monster_map_end\n");
	}
	return ret;
}

