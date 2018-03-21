#include "CEntity.h"
#include "CAnimating.h"
#include "CItem.h"
#include "CPlayer.h"
#include "ItemRespawnSystem.h"
#include "CE_recipientfilter.h"
#include "ammodef.h"
#include "vphysics/constraints.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool ITEM_GiveAmmo( CPlayer *pPlayer, float flCount, const char *pszAmmoName)
{
	int iAmmoType = GetAmmoDef()->Index(pszAmmoName);
	if (iAmmoType == -1)
		return false;

	bool ret = (pPlayer->GiveAmmo(flCount, iAmmoType) == 0) ? false : true;
	if(ret)
	{
		CPASAttenuationFilter filter( pPlayer, "BaseCombatCharacter.AmmoPickup" );
		CEntity::EmitSound( filter, pPlayer->entindex(), "BaseCombatCharacter.AmmoPickup" );
	}
	return ret;
}

typedef CItem<CAnimating> CAmmoItem;

class CAmmo : public CAmmoItem
{
public:
	CE_DECLARE_CLASS(CAmmo, CAmmoItem);
	DECLARE_DATADESC();
	CE_CUSTOM_ENTITY();

	virtual int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_WCEDIT_POSITION; };

};


BEGIN_DATADESC( CAmmo )
	DEFINE_KEYFIELD( CAmmoItem::m_bRespawn,		FIELD_BOOLEAN,	"mm_respawn" ),
END_DATADESC()


// awp
class CAmmo_338msg : public CAmmo
{
public:
	CE_DECLARE_CLASS(CAmmo_338msg, CAmmo);
public:
	void Spawn( void )
	{ 
		Precache();
		SetModel( "models/items/BoxSniperRounds.mdl");
		BaseClass::Spawn();
	}
	void Precache( void )
	{
		BaseClass::Precache();
		PrecacheModel ("models/items/BoxSniperRounds.mdl");
	}
	CEntity *MyTouch( CPlayer *pPlayer )
	{
		if(ITEM_GiveAmmo(pPlayer, 10, "BULLET_PLAYER_338MAG"))
			return this;
		return NULL;
	}
};

// p228
class CAmmo_357sig : public CAmmo
{
public:
	CE_DECLARE_CLASS(CAmmo_357sig, CAmmo);
public:
	void Spawn( void )
	{ 
		Precache();
		SetModel( "models/items/BoxSRounds.mdl");
		BaseClass::Spawn();
	}
	void Precache( void )
	{
		BaseClass::Precache();
		PrecacheModel ("models/items/BoxSRounds.mdl");
	}
	CEntity *MyTouch( CPlayer *pPlayer )
	{
		if(ITEM_GiveAmmo(pPlayer, 13, "BULLET_PLAYER_357SIG"))
			return this;
		return NULL;
	}
};

// usp mac10 ump45
class CAmmo_45acp : public CAmmo
{
public:
	CE_DECLARE_CLASS(CAmmo_45acp, CAmmo);
public:
	void Spawn( void )
	{ 
		Precache();
		SetModel( "models/items/BoxSRounds.mdl");
		BaseClass::Spawn();
	}
	void Precache( void )
	{
		BaseClass::Precache();
		PrecacheModel ("models/items/BoxSRounds.mdl");
	}
	CEntity *MyTouch( CPlayer *pPlayer )
	{
		if(ITEM_GiveAmmo(pPlayer, 30, "BULLET_PLAYER_45ACP"))
			return this;
		return NULL;
	}
};

// deagle
class CAmmo_50ae : public CAmmo
{
public:
	CE_DECLARE_CLASS(CAmmo_50ae, CAmmo);
public:
	void Spawn( void )
	{ 
		Precache();
		SetModel( "models/items/357ammo.mdl");
		BaseClass::Spawn();
	}
	void Precache( void )
	{
		BaseClass::Precache();
		PrecacheModel ("models/items/357ammo.mdl");
	}
	CEntity *MyTouch( CPlayer *pPlayer )
	{
		if(ITEM_GiveAmmo(pPlayer, 7, "BULLET_PLAYER_50AE"))
			return this;
		return NULL;
	}
};

// gail famas m4a1 sg550 sg552
class CAmmo_556mm : public CAmmo
{
public:
	CE_DECLARE_CLASS(CAmmo_556mm, CAmmo);
public:
	void Spawn( void )
	{ 
		Precache();
		SetModel( "models/items/BoxMRounds.mdl");
		BaseClass::Spawn();
	}
	void Precache( void )
	{
		BaseClass::Precache();
		PrecacheModel ("models/items/BoxMRounds.mdl");
	}
	CEntity *MyTouch( CPlayer *pPlayer )
	{
		if(ITEM_GiveAmmo(pPlayer, 30, "BULLET_PLAYER_556MM"))
			return this;
		return NULL;
	}
};

// m249
class CAmmo_556mm_box : public CAmmo
{
public:
	CE_DECLARE_CLASS(CAmmo_556mm_box, CAmmo);
public:
	void Spawn( void )
	{ 
		Precache();
		SetModel( "models/items/BoxMRounds.mdl");
		BaseClass::Spawn();
	}
	void Precache( void )
	{
		BaseClass::Precache();
		PrecacheModel ("models/items/BoxMRounds.mdl");
	}
	CEntity *MyTouch( CPlayer *pPlayer )
	{
		if(ITEM_GiveAmmo(pPlayer, 50, "BULLET_PLAYER_556MM_BOX"))
			return this;
		return NULL;
	}
};

// fiveseven p90
class CAmmo_57mm : public CAmmo
{
public:
	CE_DECLARE_CLASS(CAmmo_57mm, CAmmo);
public:
	 CEntity *MyTouch( CPlayer *pPlayer )
	 {
		if(ITEM_GiveAmmo(pPlayer, 50, "BULLET_PLAYER_57MM"))
			return this;
		return NULL;
	 }
};

// ak47 g3sg1 scout
class CAmmo_762mm : public CAmmo
{
public:
	CE_DECLARE_CLASS(CAmmo_762mm, CAmmo);
public:
	void Spawn( void )
	{ 
		Precache();
		SetModel( "models/items/BoxMRounds.mdl");
		BaseClass::Spawn();
	}
	void Precache( void )
	{
		BaseClass::Precache();
		PrecacheModel ("models/items/BoxMRounds.mdl");
	}
	CEntity *MyTouch( CPlayer *pPlayer )
	{
		if(ITEM_GiveAmmo(pPlayer, 30, "BULLET_PLAYER_762MM"))
			return this;
		return NULL;
	}
};

// elite glock mp5navy tmp
class CAmmo_9mm : public CAmmo
{
public:
	CE_DECLARE_CLASS(CAmmo_9mm, CAmmo);
public:
	void Spawn( void )
	{ 
		Precache();
		SetModel( "models/items/BoxMRounds.mdl");
		BaseClass::Spawn();
	}
	void Precache( void )
	{
		BaseClass::Precache();
		PrecacheModel ("models/items/BoxMRounds.mdl");
	}
	CEntity *MyTouch( CPlayer *pPlayer )
	{
		if(ITEM_GiveAmmo(pPlayer, 30, "BULLET_PLAYER_9MM"))
			return this;
		return NULL;
	}
};

// m3 xm1014
class CAmmo_buckshot : public CAmmo
{
public:
	CE_DECLARE_CLASS(CAmmo_buckshot, CAmmo);
public:
	void Spawn( void )
	{ 
		Precache();
		SetModel( "models/items/BoxBuckshot.mdl");
		BaseClass::Spawn();
	}
	void Precache( void )
	{
		BaseClass::Precache();
		PrecacheModel ("models/items/BoxBuckshot.mdl");
	}
	CEntity *MyTouch( CPlayer *pPlayer )
	{
		if(ITEM_GiveAmmo(pPlayer, 8, "BULLET_PLAYER_BUCKSHOT"))
			return this;
		return NULL;
	}
};

CE_LINK_ENTITY_TO_CLASS( ammo_338mag, CAmmo_338msg ); // awp
CE_LINK_ENTITY_TO_CLASS( ammo_357sig, CAmmo_357sig ); // p228
CE_LINK_ENTITY_TO_CLASS( ammo_45acp, CAmmo_45acp ); // usp mac10 ump45
CE_LINK_ENTITY_TO_CLASS( ammo_50ae, CAmmo_50ae ); // deagle
CE_LINK_ENTITY_TO_CLASS( ammo_556mm, CAmmo_556mm ); // gail famas m4a1 sg550 sg552
CE_LINK_ENTITY_TO_CLASS( ammo_556mm_box, CAmmo_556mm_box ); // m249
CE_LINK_ENTITY_TO_CLASS( ammo_57mm, CAmmo_57mm ); // fiveseven p90
CE_LINK_ENTITY_TO_CLASS( ammo_762mm, CAmmo_762mm ); // ak47 g3sg1 scout
CE_LINK_ENTITY_TO_CLASS( ammo_9mm, CAmmo_9mm ); // elite glock mp5navy tmp
CE_LINK_ENTITY_TO_CLASS( ammo_buckshot, CAmmo_buckshot ); // m3 xm1014









// Controls the application of the robus radius damage model.
ConVar	sv_robust_explosions( "sv_robust_explosions","1", 0 );

// Damage scale for damage inflicted by the player on each skill level.
ConVar	sk_dmg_inflict_scale1( "sk_dmg_inflict_scale1", "1.50", 0 );
ConVar	sk_dmg_inflict_scale2( "sk_dmg_inflict_scale2", "1.00", 0 );
ConVar	sk_dmg_inflict_scale3( "sk_dmg_inflict_scale3", "0.75", 0 );

// Damage scale for damage taken by the player on each skill level.
ConVar	sk_dmg_take_scale1( "sk_dmg_take_scale1", "0.50", 0 );
ConVar	sk_dmg_take_scale2( "sk_dmg_take_scale2", "1.00", 0 );
#ifdef HL2_EPISODIC
	ConVar	sk_dmg_take_scale3( "sk_dmg_take_scale3", "2.0", 0 );
#else
	ConVar	sk_dmg_take_scale3( "sk_dmg_take_scale3", "1.50", 0 );
#endif//HL2_EPISODIC

ConVar	sk_allow_autoaim( "sk_allow_autoaim", "1", FCVAR_REPLICATED | 0 );

// Autoaim scale
ConVar	sk_autoaim_scale1( "sk_autoaim_scale1", "1.0", 0 );
ConVar	sk_autoaim_scale2( "sk_autoaim_scale2", "1.0", 0 );
//ConVar	sk_autoaim_scale3( "sk_autoaim_scale3", "0.0", 0 ); NOT CURRENTLY OFFERED ON SKILL 3

// Quantity scale for ammo received by the player.
ConVar	sk_ammo_qty_scale1 ( "sk_ammo_qty_scale1", "1.20", 0 );
ConVar	sk_ammo_qty_scale2 ( "sk_ammo_qty_scale2", "1.00", 0 );
ConVar	sk_ammo_qty_scale3 ( "sk_ammo_qty_scale3", "0.60", 0 );

ConVar	sk_plr_health_drop_time		( "sk_plr_health_drop_time", "30", 0 );
ConVar	sk_plr_grenade_drop_time	( "sk_plr_grenade_drop_time", "30", 0 );

ConVar	sk_plr_dmg_ar2			( "sk_plr_dmg_ar2","0", 0 );
ConVar	sk_npc_dmg_ar2			( "sk_npc_dmg_ar2","0", 0);
ConVar	sk_max_ar2				( "sk_max_ar2","0", 0);
ConVar	sk_max_ar2_altfire		( "sk_max_ar2_altfire","0", 0);

ConVar	sk_plr_dmg_alyxgun		( "sk_plr_dmg_alyxgun","0", 0 );
ConVar	sk_npc_dmg_alyxgun		( "sk_npc_dmg_alyxgun","0", 0);
ConVar	sk_max_alyxgun			( "sk_max_alyxgun","0", 0);

ConVar	sk_plr_dmg_pistol		( "sk_plr_dmg_pistol","0", 0 );
ConVar	sk_npc_dmg_pistol		( "sk_npc_dmg_pistol","0", 0);
ConVar	sk_max_pistol			( "sk_max_pistol","0", 0);

ConVar	sk_plr_dmg_smg1			( "sk_plr_dmg_smg1","0", 0 );
ConVar	sk_npc_dmg_smg1			( "sk_npc_dmg_smg1","0", 0);
ConVar	sk_max_smg1				( "sk_max_smg1","0", 0);

// FIXME: remove these
//ConVar	sk_plr_dmg_flare_round	( "sk_plr_dmg_flare_round","0", 0);
//ConVar	sk_npc_dmg_flare_round	( "sk_npc_dmg_flare_round","0", 0);
//ConVar	sk_max_flare_round		( "sk_max_flare_round","0", 0);

ConVar	sk_plr_dmg_buckshot		( "sk_plr_dmg_buckshot","0", 0);	
ConVar	sk_npc_dmg_buckshot		( "sk_npc_dmg_buckshot","0", 0);
ConVar	sk_max_buckshot			( "sk_max_buckshot","0", 0);
ConVar	sk_plr_num_shotgun_pellets( "sk_plr_num_shotgun_pellets","7", 0);

ConVar	sk_plr_dmg_rpg_round	( "sk_plr_dmg_rpg_round","0", 0);
ConVar	sk_npc_dmg_rpg_round	( "sk_npc_dmg_rpg_round","0", 0);
ConVar	sk_max_rpg_round		( "sk_max_rpg_round","0", 0);

ConVar	sk_plr_dmg_sniper_round	( "sk_plr_dmg_sniper_round","0", 0);	
ConVar	sk_npc_dmg_sniper_round	( "sk_npc_dmg_sniper_round","0", 0);
ConVar	sk_max_sniper_round		( "sk_max_sniper_round","0", 0);

//ConVar	sk_max_slam				( "sk_max_slam","0", 0);
//ConVar	sk_max_tripwire			( "sk_max_tripwire","0", 0);

//ConVar	sk_plr_dmg_molotov		( "sk_plr_dmg_molotov","0", 0);
//ConVar	sk_npc_dmg_molotov		( "sk_npc_dmg_molotov","0", 0);
//ConVar	sk_max_molotov			( "sk_max_molotov","0", 0);

ConVar	sk_plr_dmg_grenade		( "sk_plr_dmg_grenade","0", 0);
ConVar	sk_npc_dmg_grenade		( "sk_npc_dmg_grenade","0", 0);
ConVar	sk_max_grenade			( "sk_max_grenade","0", 0);

#ifdef HL2_EPISODIC
ConVar	sk_max_hopwire			( "sk_max_hopwire", "3", 0);
ConVar	sk_max_striderbuster	( "sk_max_striderbuster", "3", 0);
#endif

//ConVar sk_plr_dmg_brickbat	( "sk_plr_dmg_brickbat","0", 0);
//ConVar sk_npc_dmg_brickbat	( "sk_npc_dmg_brickbat","0", 0);
//ConVar sk_max_brickbat		( "sk_max_brickbat","0", 0);

ConVar	sk_plr_dmg_smg1_grenade	( "sk_plr_dmg_smg1_grenade","0", 0);
ConVar	sk_npc_dmg_smg1_grenade	( "sk_npc_dmg_smg1_grenade","0", 0);
ConVar	sk_max_smg1_grenade		( "sk_max_smg1_grenade","0", 0 );

ConVar	sk_plr_dmg_357			( "sk_plr_dmg_357", "0", 0 );
ConVar	sk_npc_dmg_357			( "sk_npc_dmg_357", "0", 0 );
ConVar	sk_max_357				( "sk_max_357", "0", 0 );

ConVar	sk_plr_dmg_crossbow		( "sk_plr_dmg_crossbow", "0", 0 );
ConVar	sk_npc_dmg_crossbow		( "sk_npc_dmg_crossbow", "0", 0 );
ConVar	sk_max_crossbow			( "sk_max_crossbow", "0", 0 );

ConVar	sk_dmg_sniper_penetrate_plr( "sk_dmg_sniper_penetrate_plr","0", 0);
ConVar	sk_dmg_sniper_penetrate_npc( "sk_dmg_sniper_penetrate_npc","0", 0);

ConVar	sk_plr_dmg_airboat		( "sk_plr_dmg_airboat", "0", 0 );
ConVar	sk_npc_dmg_airboat		( "sk_npc_dmg_airboat", "0", 0 );

ConVar	sk_max_gauss_round		( "sk_max_gauss_round", "0" );

// Gunship & Dropship cannons
ConVar	sk_npc_dmg_gunship			( "sk_npc_dmg_gunship", "0" );
ConVar	sk_npc_dmg_gunship_to_plr	( "sk_npc_dmg_gunship_to_plr", "0" );


