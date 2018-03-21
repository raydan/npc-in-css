
#ifndef _INCLUDE_CBUTTON_H_
#define _INCLUDE_CBUTTON_H_

#include "CEntity.h"
#include "CToggle.h"


class CE_CMomentaryRotButton : public CToggle
{
public:
	CE_DECLARE_CLASS( CE_CMomentaryRotButton, CToggle );

	bool DispatchKeyValue( const char *szKeyName, const char *szValue );

private:
	

public:
	DECLARE_DATAMAP(QAngle,m_start);
	DECLARE_DATAMAP(float,m_IdealYaw);
	DECLARE_DATAMAP(QAngle,m_end);
	DECLARE_DATAMAP(int,m_direction);
	DECLARE_DATAMAP(bool,m_bDisabled);
	DECLARE_DATAMAP(bool,m_bLocked);
	

};


#endif
