
#ifndef _INCLUDE_CGAMEEND_H_
#define _INCLUDE_CGAMEEND_H_

#include "CEntity.h"


class CE_CGameEnd : public CEntity 
{
public:
	CE_DECLARE_CLASS( CE_CGameEnd, CEntity );

	virtual bool AcceptInput(const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID);
};


#endif
