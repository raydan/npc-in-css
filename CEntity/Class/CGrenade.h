
#ifndef _INCLUDE_CGRENADE_H_
#define _INCLUDE_CGRENADE_H_

#include "CEntity.h"
#include "CAnimating.h"


class CE_Grenade : public CAnimating
{
public:
	CE_DECLARE_CLASS( CE_Grenade, CAnimating );
	
	CE_Grenade();
	void PostConstructor();

	CCombatCharacter *GetThrower( void );
	void DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void SetThrower( CBaseEntity *pThrower );
	void SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );

	
	void DangerSoundThink( void );
	void ExplodeTouch( CEntity *pOther );
	
	// is virtual
	void SetDamageRadius(float flDamageRadius);

public:
	virtual void Detonate( void );
	virtual float GetShakeAmplitude();
	virtual float GetShakeRadius();
	virtual void Explode( trace_t *pTrace, int bitsDamageType );

public:
	DECLARE_DEFAULTHEADER(Detonate, void, ());
	DECLARE_DEFAULTHEADER(GetShakeAmplitude, float, ());
	DECLARE_DEFAULTHEADER(GetShakeRadius, float, ());
	DECLARE_DEFAULTHEADER(Explode, void, (trace_t *pTrace, int bitsDamageType));

protected: //Sendprops
	DECLARE_SENDPROP(float, m_flDamage);
	DECLARE_SENDPROP(float, m_DmgRadius);
	DECLARE_SENDPROP(CFakeHandle, m_hThrower);

public: //Datamaps
	DECLARE_DATAMAP_OFFSET(CFakeHandle,m_hOriginalThrower);
	DECLARE_DATAMAP(float,m_flDetonateTime);
	DECLARE_DATAMAP(float,m_flWarnAITime);
	DECLARE_DATAMAP(string_t,m_iszBounceSound);
	DECLARE_DATAMAP(bool,m_bHasWarnedAI);


private:


};


#endif

