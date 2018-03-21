
#include "CEntity.h"
#include "CPlayer.h"
#include "CCycler_Fix.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

abstract_class CE_Temp_NPC : public CE_Cycler_Fix
{
public:
	CE_DECLARE_CLASS(CE_Temp_NPC, CE_Cycler_Fix);

	virtual void Spawn()
	{
		Precache();
		BaseClass::Spawn();
		SetModel(STRING(GetModelName()));

		SetHullType(HULL_HUMAN);
		SetHullSizeNormal();

		SetSolid( SOLID_BBOX );
		AddSolidFlags( FSOLID_NOT_STANDABLE );
		SetMoveType( MOVETYPE_STEP );
		SetBloodColor( DONT_BLEED );

		m_NPCState			= NPC_STATE_NONE;
		SetImpactEnergyScale( 0.0f ); 

		CapabilitiesClear();
		NPCInit();
	}

	virtual Class_T	Classify ( void )
	{
		return CLASS_PLAYER_ALLY_VITAL;
	}

	virtual int OnTakeDamage(const CTakeDamageInfo& info)
	{
		return 0;
	}

	virtual float MaxYawSpeed( void )
	{
		return 0.0f;
	}
	virtual bool CanBeAnEnemyOf( CBaseEntity *pEnemy )
	{
		return false;
	}

};

