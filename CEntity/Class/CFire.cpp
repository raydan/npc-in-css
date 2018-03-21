
#include "CFire.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(CFire, CE_CFire);

//Datamaps
DEFINE_PROP(m_flHeatLevel, CE_CFire);
DEFINE_PROP(m_flMaxHeat, CE_CFire);
DEFINE_PROP(m_flFireSize, CE_CFire);


#define	FIRE_HEIGHT				256.0f
#define FIRE_SCALE_FROM_SIZE(firesize)		(firesize * (1/FIRE_HEIGHT))

#define	FIRE_MAX_GROUND_OFFSET	24.0f	//(2 feet)

#define	DEFAULT_ATTACK_TIME	4.0f
#define	DEFAULT_DECAY_TIME	8.0f

// UNDONE: This shouldn't be constant but depend on specific fire
#define	FIRE_WIDTH				128
#define	FIRE_MINS				Vector(-20,-20,0 )   // Sould be FIRE_WIDTH in size
#define FIRE_MAXS				Vector( 20, 20,20)	 // Sould be FIRE_WIDTH in size
#define FIRE_SPREAD_DAMAGE_MULTIPLIER 2.0

#define FIRE_MAX_HEAT_LEVEL		64.0f
#define	FIRE_NORMAL_ATTACK_TIME	20.0f
#define FIRE_THINK_INTERVAL		0.1


bool CE_CFire::GetFireDimensions( Vector *pFireMins, Vector *pFireMaxs )
{
	if ( m_flHeatLevel <= 0 )
	{
		pFireMins->Init();
		pFireMaxs->Init();
		return false;
	}

	float scale = m_flHeatLevel / m_flMaxHeat;
	float damageRadius = scale * m_flFireSize * FIRE_WIDTH / FIRE_HEIGHT * 0.5;	

	damageRadius *= FIRE_SPREAD_DAMAGE_MULTIPLIER; //FIXME: Trying slightly larger radius for burning

	if ( damageRadius < 16 )
	{
		damageRadius = 16;
	}

	pFireMins->Init(-damageRadius,-damageRadius,0);
	pFireMaxs->Init(damageRadius,damageRadius,m_flFireSize*scale);

	return true;
}


bool FireSystem_GetFireDamageDimensions( CBaseEntity *pEntity, Vector *pFireMins, Vector *pFireMaxs )
{
	CE_CFire *pFire = dynamic_cast<CE_CFire *>(CEntity::Instance(pEntity));

	if ( pFire && pFire->GetFireDimensions( pFireMins, pFireMaxs ) )
	{
		*pFireMins /= FIRE_SPREAD_DAMAGE_MULTIPLIER;
		*pFireMaxs /= FIRE_SPREAD_DAMAGE_MULTIPLIER;
		return true;
	}
	pFireMins->Init();
	pFireMaxs->Init();
	return false;
}

