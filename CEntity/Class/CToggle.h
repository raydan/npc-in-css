
#ifndef _INCLUDE_CTOGGLE_H_
#define _INCLUDE_CTOGGLE_H_

#include "CEntity.h"


class CToggle : public CEntity
{
public:
	CE_DECLARE_CLASS(CToggle, CEntity);

public:
	DECLARE_DATAMAP(TOGGLE_STATE,m_toggle_state);
	DECLARE_DATAMAP(QAngle,m_vecMoveAng);
	DECLARE_DATAMAP(float,m_flMoveDistance);

};



#endif
