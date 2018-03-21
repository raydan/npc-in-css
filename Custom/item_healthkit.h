#ifndef _INCLUDE_ITEM_HEALTHKIT_H
#define _INCLUDE_ITEM_HEALTHKIT_H

#include "CEntityManager.h"
#include "CEntity.h"
#include "CItem.h"
#include "CSoda_Fix.h"


class CHealthKit : public CItem<CSoda_Fix>
{
public:
	CE_DECLARE_CLASS( CHealthKit, CItem<CSoda_Fix> );

	void Spawn( void );
	void Precache( void );
	CEntity *MyTouch( CPlayer *pPlayer );

	int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_WCEDIT_POSITION; };

};


class CHealthVial : public CItem<CSoda_Fix>
{
public:
	CE_DECLARE_CLASS( CHealthVial, CItem<CSoda_Fix> );

	void Spawn( void );
	void Precache( void );
	CEntity *MyTouch( CPlayer *pPlayer );

	int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_WCEDIT_POSITION; };

};

#endif
