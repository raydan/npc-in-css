
#ifndef _INCLUDE_CPHYSICSPROP_H_
#define _INCLUDE_CPHYSICSPROP_H_

#include "CEntity.h"
#include "CBreakableProp.h"
#include "player_pickup.h"


// An interface so that objects parented to props can receive collision interaction events.
enum parentCollisionInteraction_t
{
	COLLISIONINTER_PARENT_FIRST_IMPACT = 1,
};


abstract_class IParentPropInteraction
{
public:
	virtual void OnParentCollisionInteraction( parentCollisionInteraction_t eType, int index, gamevcollisionevent_t *pEvent ) = 0;
	virtual void OnParentPhysGunDrop( CBaseEntity *pPhysGunUser, PhysGunDrop_t Reason ) = 0;
};


class CE_CPhysicsProp : public CE_CBreakableProp
{
public:
	CE_DECLARE_CLASS(CE_CPhysicsProp, CE_CBreakableProp);

	bool HasInteraction( propdata_interactions_t Interaction ) { return ( *(m_iInteractions) & (1 << Interaction) ) != 0; }
	void HandleFirstCollisionInteractions( int index, gamevcollisionevent_t *pEvent );

	void OnPhysGunDrop( CBaseEntity *pPhysGunUser, PhysGunDrop_t Reason );

	float GetMass() const;


public:
	bool GetPropDataAngles( const char *pKeyName, QAngle &vecAngles );

};


#endif

