#ifndef _INCLUDE_SOURCE_IMONSTER_INTERFACE_H
#define _INCLUDE_SOURCE_IMONSTER_INTERFACE_H


#define MM_INTERFACE_MONSTER_NAME			"IMONSTER"
#define MM_INTERFACE_MONSTER_VERSION		2

class CTakeDamageInfo;

enum MConfig {
	MC_b_PlayGiveSuit = 0,
	MC_b_HL2DM_Damage_Style,
	MC_b_Item_Weapon_Respawn,
	MC_b_Prop_PickUp,
	MC_b_CleanUpMap,
	MC_b_DefaultViewVectors,
	MC_b_Blast_Self_Damage,
	MC_b_RemoveDropWeapon,
	MC_f_NPCWeaponRemoveDelay,
	MC_b_CSS_Weapon_Penetration,
};

abstract_class IMonster_Listener
{
public:
	virtual void OnMonsterShutdown() { }
	virtual int IRelationType ( CBaseEntity *pEntity, CBaseEntity *pTarget, int original_value) { return original_value; }
	virtual void NPC_UpdateOnRemove(CBaseEntity *pEntity) { }
	virtual bool PlayerCanPickup(CBaseEntity *pPlayer, CBaseEntity *pEntity, const char *classname) { return true; }
	virtual void OnVehicleOverturned(CBaseEntity *pPlayer, CBaseEntity *pEntity) { }
	virtual bool CanExitVehicle(CBaseEntity *pPlayer, CBaseEntity *pEntity, bool original_value) { return original_value; }
	virtual bool IsPassengerVisible( int nRole, bool original_value) { return original_value; }
	virtual void OnNPCTakeDamage(int hitgroup, CTakeDamageInfo &info) { }
	virtual void OnPlayerKilledNPC(int client, int entity, int npc_class) { }
	
};

abstract_class IMonster
{
public:
	virtual int GetInterfaceVersion() =0;
	virtual void SetListener(IMonster_Listener *listener) =0;

	virtual void SetConfig(MConfig config, bool value) =0;
	virtual void SetConfig(MConfig config, int value) =0;
	virtual void SetConfig(MConfig config, float value) =0;

	virtual bool IsNPC(CBaseEntity *pEntity) =0;


};

#endif

