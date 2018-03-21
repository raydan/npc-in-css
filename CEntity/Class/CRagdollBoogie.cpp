
#include "CRagdollBoogie.h"
#include "CRagdollProp.h"

CE_LINK_ENTITY_TO_CLASS(env_ragdoll_boogie, CE_CRagdollBoogie);

// Datamaps
DEFINE_PROP(m_flStartTime, CE_CRagdollBoogie);
DEFINE_PROP(m_flBoogieLength, CE_CRagdollBoogie);
DEFINE_PROP(m_flMagnitude, CE_CRagdollBoogie);


void CE_CRagdollBoogie::PostConstructor()
{
	BaseClass::PostConstructor();
	m_nSuppressionCount.offset = m_flMagnitude.offset + 4;
	m_nSuppressionCount.ptr = (int *)(((uint8_t *)(BaseEntity())) + m_nSuppressionCount.offset);
}

CE_CRagdollBoogie *CE_CRagdollBoogie::Create( CEntity *pTarget, float flMagnitude, 
	float flStartTime, float flLengthTime, int nSpawnFlags )
{
	CE_CRagdollProp *pRagdoll = dynamic_cast< CE_CRagdollProp* >( pTarget );
	if ( !pRagdoll )
		return NULL;

	CE_CRagdollBoogie *pBoogie = (CE_CRagdollBoogie *)CreateEntityByName( "env_ragdoll_boogie" );
	if ( pBoogie == NULL )
		return NULL;

	pBoogie->AddSpawnFlags( nSpawnFlags );
	pBoogie->AttachToEntity( pTarget );
	pBoogie->SetBoogieTime( flStartTime, flLengthTime );
	pBoogie->SetMagnitude( flMagnitude );
	pBoogie->Spawn();
	return pBoogie;
}

void CE_CRagdollBoogie::AttachToEntity( CEntity *pTarget )
{
	m_nSuppressionCount = 0;

	// Look for other boogies on the ragdoll + kill them
	CEntity *pNext;
	for ( CEntity *pChild = pTarget->FirstMoveChild(); pChild; pChild = pNext )
	{
		pNext = pChild->NextMovePeer();
		CE_CRagdollBoogie *pBoogie = dynamic_cast<CE_CRagdollBoogie*>(pChild);
		if ( !pBoogie )
			continue;

		*(m_nSuppressionCount) = pBoogie->m_nSuppressionCount;
		UTIL_Remove( pChild );
	}

	FollowEntity( pTarget->BaseEntity() );
}


void CE_CRagdollBoogie::SetBoogieTime( float flStartTime, float flLengthTime )
{
	m_flStartTime = flStartTime;
	m_flBoogieLength = flLengthTime;
}

void CE_CRagdollBoogie::SetMagnitude( float flMagnitude )
{
	m_flMagnitude = flMagnitude;
}

