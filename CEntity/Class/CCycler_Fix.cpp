
#include "CCycler_Fix.h"
#include "MonsterTools.h"
#include "CNPCBaseWeapon.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


BEGIN_DATADESC( CE_Cycler_Fix )

	DEFINE_KEYFIELD( m_iNpchealth, FIELD_INTEGER, "npchealth" ),

END_DATADESC()


void CE_Cycler_Fix::PostConstructor()
{
	BaseClass::PostConstructor();
	*(m_pfnThink) = NULL;
}

void CE_Cycler_Fix::Spawn(void)
{
	//according CBaseCombatCharacter
	SetBlocksLOS( false );
}

void CE_Cycler_Fix::Precache(void)
{
	g_helpfunc.CAI_BaseNPC_Precache(BaseEntity());

	gm_iszPlayerSquad = AllocPooledString( PLAYER_SQUADNAME );
}

int CE_Cycler_Fix::ObjectCaps()
{
	int base = BaseClass::ObjectCaps();
	base &= ~FCAP_IMPULSE_USE;
	return base;
}

void CE_Cycler_Fix::Think(void)
{
	VALVE_BASEPTR original_think = m_pfnThink;
	if(original_think != NULL)
	{
		(BaseEntity()->*original_think)();
		return;
	}
}

int CE_Cycler_Fix::OnTakeDamage(const CTakeDamageInfo& info)
{
	MonsterTools::OnNPCTakeDamage(LastHitGroup(), (CTakeDamageInfo &)info);
	return g_helpfunc.CBaseCombatCharacter_OnTakeDamage(BaseEntity(), info);
}

bool CE_Cycler_Fix::IsAlive(void)
{
	return (m_lifeState == LIFE_ALIVE); 
}

Disposition_t CE_Cycler_Fix::IRelationType ( CBaseEntity *pTarget )
{
	Disposition_t o_ret = BaseClass::IRelationType(pTarget);
	Disposition_t m_ret = MonsterTools::IRelationType(BaseEntity(), pTarget, o_ret);
	return m_ret;
}

void CE_Cycler_Fix::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	g_helpfunc.CBaseEntity_Use(BaseEntity(), pActivator, pCaller, useType, value);
}

void CE_Cycler_Fix::NPCInit()
{
	if(m_iNpchealth > 0) {
		m_iHealth = m_iNpchealth;
	}
	BaseClass::NPCInit();
}

void CE_Cycler_Fix::UpdateOnRemove()
{
	MonsterTools::NPC_UpdateOnRemove(BaseEntity());
	BaseClass::UpdateOnRemove();
}

const char *NPC_WeaponReplace(const char *szValue)
{
	const char *newszValue = szValue;
	if(FStrEq(szValue, "weapon_ar2"))
		newszValue = WEAPON_AR2_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_shotgun"))
		newszValue = WEAPON_SHOTGUN_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_smg1"))
		newszValue = WEAPON_SMG1_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_stunstick"))
		newszValue = WEAPON_STUNSTICK_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_alyxgun"))
		newszValue = WEAPON_ALYXGUN_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_pistol"))
		newszValue = WEAPON_PISTOL_REPLACE_NAME;
	/*else {
		Assert(0);
		META_CONPRINTF("CE_Cycler_Fix have invalid additionalequipment: %s\n", szValue);
		newszValue = WEAPON_SMG1_REPLACE_NAME;
	}*/
	return newszValue;
}

bool CE_Cycler_Fix::DispatchKeyValue( const char *szKeyName, const char *szValue )
{
	if(FStrEq(szKeyName, "additionalequipment") && strlen(szValue) > 0 && strcmp(szValue, "0"))
	{	
		return BaseClass::DispatchKeyValue(szKeyName, NPC_WeaponReplace(szValue));
	}

	return BaseClass::DispatchKeyValue(szKeyName, szValue);
}

