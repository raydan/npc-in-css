
#include "CProp.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(CBaseProp, CE_Prop);


SH_DECL_MANUALHOOK0(OverridePropdata, 0, 0, 0, bool);
DECLARE_HOOK(OverridePropdata, CE_Prop);
DECLARE_DEFAULTHANDLER(CE_Prop, OverridePropdata, bool, (), ());


