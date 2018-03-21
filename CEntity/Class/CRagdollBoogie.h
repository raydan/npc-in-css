#ifndef _INCLUDE_CRAGDOLLBOOGIE_H_
#define _INCLUDE_CRAGDOLLBOOGIE_H_

#include "CEntity.h"


class CE_CRagdollBoogie : public CEntity
{
public:
	CE_DECLARE_CLASS( CE_CRagdollBoogie, CEntity );
	
	void PostConstructor();

public:
	static CE_CRagdollBoogie *Create( CEntity *pTarget, float flMagnitude, float flStartTime, float flLengthTime = 0.0f, int nSpawnFlags = 0 );

private:
	void	AttachToEntity( CEntity *pTarget );
	void	SetBoogieTime( float flStartTime, float flLengthTime );
	void	SetMagnitude( float flMagnitude );


protected: // Datamaps
	DECLARE_DATAMAP(float, m_flStartTime);
	DECLARE_DATAMAP(float, m_flBoogieLength);
	DECLARE_DATAMAP(float, m_flMagnitude);
	DECLARE_DATAMAP_OFFSET(int,	m_nSuppressionCount);

};


#endif
