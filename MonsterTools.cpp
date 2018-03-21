
#include "extension.h"
#include "MonsterTools.h"
#include "CEntity.h"
#include "CAI_NPC.h"
#include "MonsterConfig.h"


MonsterTools g_MonsterTools;
MonsterConfig g_MonsterConfig;

IMonster_Listener *s_MonsterListener = NULL;


extern CViewVectors *g_ViewVectors;
extern CViewVectors *g_DefaultViewVectors;

void MonsterTools::SetListener(IMonster_Listener *listener)
{
	s_MonsterListener = listener;
}

void MonsterTools::SetConfig(MConfig config, bool value)
{
	switch(config)
	{
		case MC_b_PlayGiveSuit:
			g_MonsterConfig.m_bEnable_PlayerGiveSuit = value;
			break;
		case MC_b_HL2DM_Damage_Style:
			g_MonsterConfig.m_bEnable_HL2DM_Damage_Style = value;
			break;
		case MC_b_Item_Weapon_Respawn:
			g_MonsterConfig.m_bEnable_Item_Weapon_Respawn = value;
			break;
		case MC_b_Prop_PickUp:
			g_MonsterConfig.m_bEnable_Prop_Pickup = value;
			break;
		case MC_b_CleanUpMap:
			g_MonsterConfig.m_bEnable_CleanUpMap = value;
			break;
		case MC_b_DefaultViewVectors:
			g_MonsterConfig.m_bEnableDefault_ViewVectors = value;
			if(g_ViewVectors && g_DefaultViewVectors)
			{
				if(g_MonsterConfig.m_bEnableDefault_ViewVectors)
				{
					memcpy(g_ViewVectors, g_DefaultViewVectors, sizeof(CViewVectors));
				} else {
					g_ViewVectors->m_vDuckHullMax.z = 36.0f;
				}
			}
			break;
		case MC_b_Blast_Self_Damage:
			g_MonsterConfig.m_bEnableBlast_Self_Damage = value;
			break;
		case MC_b_RemoveDropWeapon:
			g_MonsterConfig.m_bEnableRemoveDropWeapon = value;
			break;
		case MC_b_CSS_Weapon_Penetration:
			g_MonsterConfig.m_bEnable_CSS_Weapon_Penetration = value;
			break;
		default:
			break;
	}
}

void MonsterTools::SetConfig(MConfig config, int value)
{

}

void MonsterTools::SetConfig(MConfig config, float value)
{
	switch(config)
	{
		case MC_f_NPCWeaponRemoveDelay:
			g_MonsterConfig.m_fNPCWeaponRemoveDelay = value;
			break;
		default:
			break;
	}
}

bool MonsterTools::IsNPC(CBaseEntity *pEntity)
{
	CEntity *cent = CEntity::Instance(pEntity);
	if(cent)
	{
		return cent->IsNPC();
	}
	return false;
}


void MonsterTools::OnMonsterShutdown()
{
	if(s_MonsterListener)
	{
		s_MonsterListener->OnMonsterShutdown();
	}
}

Disposition_t MonsterTools::IRelationType(CBaseEntity *pEntity, CBaseEntity *pTarget, Disposition_t original_value)
{
	if(s_MonsterListener)
	{
		return (Disposition_t)s_MonsterListener->IRelationType(pEntity, pTarget, original_value);
	}
	return original_value;
}

void MonsterTools::NPC_UpdateOnRemove(CBaseEntity *pEntity)
{
	if(s_MonsterListener)
	{
		s_MonsterListener->NPC_UpdateOnRemove(pEntity);
	}
}

bool MonsterTools::PlayerCanPickup(CBaseEntity *pPlayer, CBaseEntity *pEntity, const char *classname)
{
	if(s_MonsterListener)
	{
		return s_MonsterListener->PlayerCanPickup(pPlayer, pEntity, classname);
	}
	return true;
}

void MonsterTools::OnVehicleOverturned(CBaseEntity *pPlayer, CBaseEntity *pEntity)
{
	if(s_MonsterListener)
	{
		s_MonsterListener->OnVehicleOverturned(pPlayer, pEntity);
	}
}

bool MonsterTools::CanExitVehicle(CBaseEntity *pPlayer, CBaseEntity *pEntity, bool original_value)
{
	if(s_MonsterListener)
	{
		return s_MonsterListener->CanExitVehicle(pPlayer, pEntity, original_value);
	}
	return original_value;
}

bool MonsterTools::IsPassengerVisible( int nRole,  bool original_value)
{
	if(s_MonsterListener)
	{
		return s_MonsterListener->IsPassengerVisible(nRole, original_value);
	}
	return original_value;
}

void MonsterTools::OnNPCTakeDamage(int hitgroup, CTakeDamageInfo &info)
{
	if(s_MonsterListener)
	{
		s_MonsterListener->OnNPCTakeDamage(hitgroup, info);
	}
}

void MonsterTools::OnPlayerKilledNPC(int client, int entity, int npc_class)
{
	if(s_MonsterListener)
	{
		s_MonsterListener->OnPlayerKilledNPC(client, entity, npc_class);
	}
}
