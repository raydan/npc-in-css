
#include "CMomentaryRotButton.h"

CE_LINK_ENTITY_TO_CLASS(momentary_rot_button, CE_CMomentaryRotButton);


DEFINE_PROP(m_start, CE_CMomentaryRotButton);
DEFINE_PROP(m_IdealYaw, CE_CMomentaryRotButton);
DEFINE_PROP(m_end, CE_CMomentaryRotButton);
DEFINE_PROP(m_direction, CE_CMomentaryRotButton);
DEFINE_PROP(m_bDisabled, CE_CMomentaryRotButton);
DEFINE_PROP(m_bLocked, CE_CMomentaryRotButton);

/*
#define SF_DOOR_ROTATE_ROLL			64
#define SF_DOOR_ROTATE_PITCH		128

#define FBitSet(iBitVector, bits)		((iBitVector) & (bits))

float AxisDelta( int flags, const QAngle &angle1, const QAngle &angle2 )
{
	// UNDONE: Use AngleDistance() here?
	if ( FBitSet (flags, SF_DOOR_ROTATE_ROLL) )
		return angle1.z - angle2.z;
	
	if ( FBitSet (flags, SF_DOOR_ROTATE_PITCH) )
		return angle1.x - angle2.x;

	return angle1.y - angle2.y;
}

float CE_CMomentaryRotButton::test(const QAngle &vecAngles )
{
	float flScale = 1.0f;
	float flPos = flScale * AxisDelta( m_spawnflags, vecAngles, m_start ) / m_flMoveDistance;
	return( clamp( flPos, 0.f, 1.f ));
}

void CE_CMomentaryRotButton::Spawn()
{
	BaseClass::Spawn();

	QAngle ang(0,0,0);
	for(int i=0;i<=380;i++)
	{

		float gg = test(ang);

		META_CONPRINTF("%f %f\n", ang.y, gg);
		int ff = 0;

		ang.y += 1.0f;
	}

	int hh = 0;

}*/

bool CE_CMomentaryRotButton::DispatchKeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "distance"))
	{
		/*float distance = atof(szValue);
		if(distance > 170.0f)
			distance = 170.0f;
		m_flMoveDistance = 170.0f;
		return true;*/
	}
	return BaseClass::DispatchKeyValue(szKeyName, szValue);
}

