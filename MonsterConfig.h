
#ifndef _INCLUDE_MONSTERCONFIG_H_
#define _INCLUDE_MONSTERCONFIG_H_


struct MonsterConfig
{
	bool m_bEnable_PlayerGiveSuit;
	bool m_bEnable_HL2DM_Damage_Style;
	bool m_bEnable_Item_Weapon_Respawn;
	bool m_bEnable_Prop_Pickup;
	bool m_bEnable_CleanUpMap;
	bool m_bEnableDefault_ViewVectors;
	bool m_bEnableBlast_Self_Damage;
	bool m_bEnableRemoveDropWeapon;
	float m_fNPCWeaponRemoveDelay;
	bool m_bEnable_CSS_Weapon_Penetration;
};

extern MonsterConfig g_MonsterConfig;

#endif;