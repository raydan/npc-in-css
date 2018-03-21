#include "CItem.h"
#include "CSoda_Fix.h"
#include "CPlayer.h"
#include "CE_recipientfilter.h"
#include "ItemRespawnSystem.h"
#include "weapon_rpg_replace.h"
#include "ammodef.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class CItem_RPG_Round : public CItem<CSoda_Fix>
{
public:
	CE_DECLARE_CLASS( CItem_RPG_Round, CItem<CSoda_Fix> );

	void Spawn( void )
	{ 
		m_bRespawn = true;
		Precache();
		SetModel( "models/weapons/w_missile_closed.mdl" );
		BaseClass::Spawn();
	}
	void Precache( void )
	{
		BaseClass::Precache();
		PrecacheModel ("models/weapons/w_missile_closed.mdl");

	}
	CEntity *MyTouch( CPlayer *pPlayer )
	{
		if(pPlayer->m_bHaveRPG)
		{
			int iAmmoType = GetAmmoDef()->Index("AMMO_TYPE_FLASHBANG");
			if (iAmmoType != -1)
			{
				bool ret = false;
				int current = pPlayer->GetAmmoCount(iAmmoType);
				if(current == 0)
				{
					pPlayer->GiveNamedItem("weapon_rpg");
					ret = true;
				} else {
					ret = (pPlayer->BaseClass::GiveAmmo(1, iAmmoType) == 0) ? false : true;
				}

				if(ret)
				{
					CPASAttenuationFilter filter( pPlayer, "BaseCombatCharacter.AmmoPickup" );
					CEntity::EmitSound( filter, pPlayer->entindex(), "BaseCombatCharacter.AmmoPickup" );
					return this;
				}
			}
		}
		return NULL;
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( item_rpg_round, item_sodacan, CItem_RPG_Round );


