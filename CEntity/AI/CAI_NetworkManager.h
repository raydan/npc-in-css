#ifndef	AI_NETWORKMANAGER_H
#define	AI_NETWORKMANAGER_H

#include "CEntity.h"
#include "CAI_Network.h"


class CAI_TestHull;

class CEAI_NetworkManager : public CEntity
{
public:
	CE_DECLARE_CLASS(CEAI_NetworkManager, CEntity);

	CAI_Network *GetNetwork() { return m_pNetwork; }

public:
	static void	InitializeAINetworks();
	

protected: // Datamaps
	DECLARE_DATAMAP_OFFSET(CAI_Network *, m_pNetwork); // ****** MAP CONTAINS DUPLICATE HAMMER NODE IDS! CHECK FOR PROBLEM


};


#endif
