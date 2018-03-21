
#include "CItem.h"
#include "CSoda_Fix.h"
#include "CPlayer.h"
#include "ItemRespawnSystem.h"
#include "vphysics/constraints.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CItemBattery : public CItem<CSoda_Fix>
{
public:
	CE_DECLARE_CLASS( CItemBattery, CItem<CSoda_Fix> );

	void Spawn( void )
	{ 
		m_bRespawn = true;
		Precache();
		SetModel( "models/items/battery.mdl" );
		BaseClass::Spawn( );
		UTIL_DropToFloor( this, MASK_SOLID );
	}
	void Precache( void )
	{
		BaseClass::Precache();
		PrecacheModel ("models/items/battery.mdl");
		PrecacheScriptSound( "ItemBattery.Touch" );

	}
	CEntity *MyTouch( CPlayer *pPlayer )
	{
		if(pPlayer->ApplyBattery())
			return this;
		return NULL;
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( item_battery, item_sodacan, CItemBattery );


