
#ifndef _INCLUDE_CPROPVEHICLE_H
#define _INCLUDE_CPROPVEHICLE_H

#include "CEntity.h"
#include "CProp.h"
#include "vehicles.h"
#include "vehicle_base.h"

class CPlayer;

class CE_CPropVehicle : public CE_Prop
{
public:
	CE_DECLARE_CLASS( CE_CPropVehicle, CE_Prop );

	CFourWheelVehiclePhysics *GetPhysics( void ) { return &(*(m_VehiclePhysics)); }

	void SetVehicleType( unsigned int nVehicleType )			{ m_nVehicleType = nVehicleType; }
	unsigned int GetVehicleType( void )							{ return m_nVehicleType; }

protected:
	DECLARE_DATAMAP(unsigned int, m_nVehicleType);
	DECLARE_DATAMAP(CFourWheelVehiclePhysics, m_VehiclePhysics);

};



class CE_CPropVehicleDriveable : public CE_CPropVehicle
{
public:
	CE_DECLARE_CLASS( CE_CPropVehicleDriveable, CE_CPropVehicle );

public:
	//IDrivableVehicle
	virtual void	ProcessMovement( CBaseEntity *pPlayer, CMoveData *pMoveData );
	virtual void	SetupMove( CBaseEntity *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual bool	AllowBlockedExit( CBaseEntity *pPlayer, int nRole );
	virtual bool	CanExitVehicle( CBaseEntity *pEntity );
	virtual void	EnterVehicle( CBaseEntity *pPassenger );
	virtual void	ExitVehicle( int nRole );
	virtual bool	PassengerShouldReceiveDamage( CTakeDamageInfo &info );
	virtual CBaseEntity *GetDriver( void );

	virtual void	DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased );
	virtual void	DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles );
	virtual bool	IsVehicleBodyInWater();
	virtual void	CreateServerVehicle( void );
	virtual bool	IsOverturned( void );

public:
	void	StartEngine( void );
	void	StopEngine( void );
	bool	IsEngineOn( void );

	bool IsEnterAnimOn( void ) { return m_bEnterAnimOn; }
	bool IsExitAnimOn( void ) { return m_bExitAnimOn; }
	const Vector &GetEyeExitEndpoint( void ) { return m_vecEyeExitEndpoint; }

public:
	void		UpdateOnRemove();
	void		SetPointViewControl(CPlayer *pPlayer);
	void		CreateDriverRagdoll(CPlayer *pPlayer);
	void		RemoveDriverRagdoll();
	void		RemovePointViewControl(CPlayer *pPlayer);
	void		PointViewToggle(CPlayer *pPlayer);

private:
	CFakeHandle		m_hPointViewControl;
	CFakeHandle		m_hDriverRagdoll;
	bool			m_bInDriverView;

public:
	DECLARE_DEFAULTHEADER(ProcessMovement, void, ( CBaseEntity *pPlayer, CMoveData *pMoveData ));
	DECLARE_DEFAULTHEADER(SetupMove, void, (CBaseEntity *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ));
	DECLARE_DEFAULTHEADER(AllowBlockedExit, bool, (CBaseEntity *pPlayer, int nRole));
	DECLARE_DEFAULTHEADER(CanExitVehicle, bool, (CBaseEntity *pEntity));
	DECLARE_DEFAULTHEADER(EnterVehicle, void, (CBaseEntity *pPassenger ));
	DECLARE_DEFAULTHEADER(ExitVehicle, void, (int nRole ));
	DECLARE_DEFAULTHEADER(GetDriver, CBaseEntity *, ());

	DECLARE_DEFAULTHEADER(DriveVehicle, void, (float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased ));
	DECLARE_DEFAULTHEADER(DampenEyePosition, void, (Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles));
	DECLARE_DEFAULTHEADER(IsVehicleBodyInWater, bool, ());
	DECLARE_DEFAULTHEADER(PassengerShouldReceiveDamage, bool, ( CTakeDamageInfo &info ));
	DECLARE_DEFAULTHEADER(CreateServerVehicle, void, ());
	DECLARE_DEFAULTHEADER(IsOverturned, bool, ());

public:
	DECLARE_SENDPROP( int, m_nSpeed );
	DECLARE_SENDPROP( int, m_nRPM );
	DECLARE_SENDPROP( float, m_flThrottle );
	DECLARE_SENDPROP( int, m_nBoostTimeLeft );
	DECLARE_SENDPROP( int, m_nHasBoost );
	DECLARE_SENDPROP( Vector, m_vecEyeExitEndpoint );
	DECLARE_SENDPROP( Vector, m_vecGunCrosshair );
	DECLARE_SENDPROP( bool, m_bUnableToFire );
	DECLARE_SENDPROP( bool, m_bHasGun );
	DECLARE_SENDPROP( bool, m_nScannerDisabledWeapons );
	DECLARE_SENDPROP( bool, m_nScannerDisabledVehicle );
	DECLARE_SENDPROP( bool, m_bEnterAnimOn );
	DECLARE_SENDPROP( bool, m_bExitAnimOn );
	DECLARE_SENDPROP(CFakeHandle, m_hPlayer);

public:
	DECLARE_DATAMAP(CFourWheelServerVehicle	*, m_pServerVehicle);
	DECLARE_DATAMAP(bool, m_bEngineLocked);
	DECLARE_DATAMAP(bool, m_bLocked);
	DECLARE_DATAMAP(float, m_flMinimumSpeedToEnterExit);
	DECLARE_DATAMAP(float, m_flTurnOffKeepUpright);
	DECLARE_DATAMAP(CFakeHandle, m_hNPCDriver);


};


#endif
