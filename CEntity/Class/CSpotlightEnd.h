#ifndef _INCLUDE_CSPORTLIGHTEND_H
#define _INCLUDE_CSPORTLIGHTEND_H

#include "CEntity.h"


class CE_CSpotlightEnd : public CEntity
{
public:
	CE_DECLARE_CLASS(CE_CSpotlightEnd, CEntity);

public: //Sendprops
	DECLARE_SENDPROP(float, m_Radius);
	DECLARE_SENDPROP(float, m_flLightScale);


public: ///Datamaps
	DECLARE_DATAMAP(Vector, m_vSpotlightDir);
	DECLARE_DATAMAP(Vector, m_vSpotlightOrg);

};

#endif
