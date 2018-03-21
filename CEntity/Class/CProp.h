
#ifndef _INCLUDE_CPROP_H_
#define _INCLUDE_CPROP_H_

#include "CEntity.h"
#include "CAnimating.h"


// Spawnflags
#define SF_DYNAMICPROP_USEHITBOX_FOR_RENDERBOX		64
#define SF_DYNAMICPROP_NO_VPHYSICS					128
#define SF_DYNAMICPROP_DISABLE_COLLISION			256



class CE_Prop : public CAnimating
{
public:
	CE_DECLARE_CLASS( CE_Prop, CAnimating );

public:
	virtual bool OverridePropdata();


public:
	DECLARE_DEFAULTHEADER(OverridePropdata, bool, ());


};


#endif
