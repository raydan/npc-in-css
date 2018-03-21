
#include "CPhysicsProp.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(CPhysicsProp, CE_CPhysicsProp);


bool CE_CPhysicsProp::GetPropDataAngles( const char *pKeyName, QAngle &vecAngles )
{
	KeyValues *modelKeyValues = new KeyValues("");
	if ( modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
	{
		KeyValues *pkvPropData = modelKeyValues->FindKey( "physgun_interactions" );
		if ( pkvPropData )
		{
			char const *pszBase = pkvPropData->GetString( pKeyName );
			if ( pszBase && pszBase[0] )
			{
				UTIL_StringToVector( vecAngles.Base(), pszBase );
				modelKeyValues->deleteThis();
				return true;
			}
		}
	}

	modelKeyValues->deleteThis();
	return false;
}

void CE_CPhysicsProp::HandleFirstCollisionInteractions( int index, gamevcollisionevent_t *pEvent )
{
	if( HasInteraction( PROPINTER_PHYSGUN_FIRST_BREAK ) )
	{
		return;
	}

	if ( HasInteraction( PROPINTER_PHYSGUN_NOTIFY_CHILDREN ) )
	{
		CUtlVector<CEntity *> children;
		GetAllChildren( this, children );
		for (int i = 0; i < children.Count(); i++ )
		{
			CEntity *pent = children.Element( i );

			IParentPropInteraction *pPropInter = dynamic_cast<IParentPropInteraction *>( pent );
			if ( !pPropInter )
			{	
				pPropInter = dynamic_cast<IParentPropInteraction *>( pent->BaseEntity() );
			}
			if ( pPropInter )
			{
				pPropInter->OnParentCollisionInteraction( COLLISIONINTER_PARENT_FIRST_IMPACT, index, pEvent );
			}
		}
	}

}


void CE_CPhysicsProp::OnPhysGunDrop( CBaseEntity *pPhysGunUser, PhysGunDrop_t Reason )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(BaseEntity());
	Assert(pPickup);
	if(pPickup)
	{
		pPickup->OnPhysGunDrop(pPhysGunUser, Reason);
	}

	if ( HasInteraction( PROPINTER_PHYSGUN_NOTIFY_CHILDREN ) )
	{
		CUtlVector<CEntity *> children;
		GetAllChildren( this, children );
		for (int i = 0; i < children.Count(); i++ )
		{
			CEntity *pent = children.Element( i );

			IParentPropInteraction *pPropInter = dynamic_cast<IParentPropInteraction *>( pent );
			if ( !pPropInter )
			{	
				pPropInter = dynamic_cast<IParentPropInteraction *>( pent->BaseEntity() );
			}
			if ( pPropInter )
			{
				pPropInter->OnParentPhysGunDrop( pPhysGunUser, Reason );
			}
		}
	}
}

float CE_CPhysicsProp::GetMass() const
{
	return VPhysicsGetObject() ? VPhysicsGetObject()->GetMass() : 1.0f;
}

