#ifndef _INCLUDE_CFIRE_H_
#define _INCLUDE_CFIRE_H_

#include "CEntity.h"

#define	MASK_FIRE_SOLID	 ( MASK_SOLID & (~(CONTENTS_MONSTER|CONTENTS_GRATE)) )


class CE_CFire : public CEntity
{
public:
	CE_DECLARE_CLASS(CE_CFire, CEntity);

public:
	bool GetFireDimensions( Vector *pFireMins, Vector *pFireMaxs );

	
protected:
	DECLARE_DATAMAP(float, m_flHeatLevel);
	DECLARE_DATAMAP(float, m_flMaxHeat);
	DECLARE_DATAMAP(float, m_flFireSize);

};


bool FireSystem_GetFireDamageDimensions( CBaseEntity *pFire, Vector *pFireMins, Vector *pFireMaxs );


#endif
