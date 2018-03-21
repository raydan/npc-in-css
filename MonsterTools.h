
#ifndef _INCLUDE_MONSTERTOOLS_H_
#define _INCLUDE_MONSTERTOOLS_H_

#include "Interface/IMonster.h"


enum Disposition_t;


class MonsterTools : public IMonster
{
public:
	int GetInterfaceVersion() { return MM_INTERFACE_MONSTER_VERSION; };

	void SetListener(IMonster_Listener *listener);

	void SetConfig(MConfig config, bool value);
	void SetConfig(MConfig config, int value);
	void SetConfig(MConfig config, float value);


	bool IsNPC(CBaseEntity *pEntity);


public:
	static void OnMonsterShutdown();
	static Disposition_t IRelationType(CBaseEntity *pEntity, CBaseEntity *pTarget, Disposition_t original_value);
	static void NPC_UpdateOnRemove(CBaseEntity *pEntity);
	static bool PlayerCanPickup(CBaseEntity *pPlayer, CBaseEntity *pEntity, const char *classname);
	static void OnVehicleOverturned(CBaseEntity *pPlayer, CBaseEntity *pEntity);
	static bool CanExitVehicle(CBaseEntity *pPlayer, CBaseEntity *pEntity, bool original_value);
	static bool IsPassengerVisible( int nRole,  bool original_value);
	static void OnNPCTakeDamage(int hitgroup, CTakeDamageInfo &info);
	static void OnPlayerKilledNPC(int client, int entity, int npc_class);

private:

};


extern MonsterTools g_MonsterTools;


#endif

