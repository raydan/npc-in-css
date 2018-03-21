
#include "CEntity.h"
#include "ragdoll_shared.h"
#include "bone_setup.h"

void RagdollApplyAnimationAsVelocity( ragdoll_t &ragdoll, const matrix3x4_t *pPrevBones, const matrix3x4_t *pCurrentBones, float dt )
{
	for ( int i = 0; i < ragdoll.listCount; i++ )
	{
		Vector velocity;
		AngularImpulse angVel;
		int boneIndex = ragdoll.boneIndex[i];
		CalcBoneDerivatives( velocity, angVel, pPrevBones[boneIndex], pCurrentBones[boneIndex], dt );
		
		AngularImpulse localAngVelocity;

		// Angular velocity is always applied in local space in vphysics
		ragdoll.list[i].pObject->WorldToLocalVector( &localAngVelocity, angVel );
		ragdoll.list[i].pObject->AddVelocity( &velocity, &localAngVelocity );
	}
}
