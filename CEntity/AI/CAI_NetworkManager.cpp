
#include "CAI_NetworkManager.h"
#include "CAI_Hint.h"
#include "CAI_Node.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CEAI_NetworkManager *g_pAINetworkManager;


CE_LINK_ENTITY_TO_CLASS(CAI_NetworkManager, CEAI_NetworkManager);

class CE_AI_Hint;

//Datamaps
DEFINE_PROP(m_pNetwork,CEAI_NetworkManager);


void CEAI_NetworkManager::InitializeAINetworks()
{
	CEAI_NetworkManager *pNetwork;
	g_pAINetworkManager = pNetwork = dynamic_cast<CEAI_NetworkManager *>(g_helpfunc.FindEntityByClassname((CBaseEntity *)NULL, "ai_network"));
	assert(pNetwork);

	g_pBigAINet = pNetwork->GetNetwork();

}

Vector PointOnLineNearestPoint(const Vector& vStartPos, const Vector& vEndPos, const Vector& vPoint)
{
	Vector	vEndToStart		= (vEndPos - vStartPos);
	Vector	vOrgToStart		= (vPoint  - vStartPos);
	float	fNumerator		= DotProduct(vEndToStart,vOrgToStart);
	float	fDenominator	= vEndToStart.Length() * vOrgToStart.Length();
	float	fIntersectDist	= vOrgToStart.Length()*(fNumerator/fDenominator);
	VectorNormalize( vEndToStart ); 
	Vector	vIntersectPos	= vStartPos + vEndToStart * fIntersectDist;

	return vIntersectPos;
}


