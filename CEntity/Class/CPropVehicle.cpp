
#include "CPropVehicle.h"
#include "vehicles.h"
#include "fourwheelvehiclephysics.h"
#include "vehicle_base.h"
#include "CPlayer.h"
#include "CSprite.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(CPropVehicle, CE_CPropVehicle);
CE_LINK_ENTITY_TO_CLASS(CPropVehicleDriveable, CE_CPropVehicleDriveable);

// CE_CPropVehicle
DEFINE_PROP(m_nVehicleType, CE_CPropVehicle);
DEFINE_PROP(m_VehiclePhysics, CE_CPropVehicle);





// CE_CPropVehicleDriveable
DEFINE_PROP(m_nSpeed, CE_CPropVehicleDriveable);
DEFINE_PROP(m_nRPM, CE_CPropVehicleDriveable);
DEFINE_PROP(m_flThrottle, CE_CPropVehicleDriveable);
DEFINE_PROP(m_nBoostTimeLeft, CE_CPropVehicleDriveable);
DEFINE_PROP(m_nHasBoost, CE_CPropVehicleDriveable);
DEFINE_PROP(m_vecEyeExitEndpoint, CE_CPropVehicleDriveable);
DEFINE_PROP(m_vecGunCrosshair, CE_CPropVehicleDriveable);
DEFINE_PROP(m_bUnableToFire, CE_CPropVehicleDriveable);
DEFINE_PROP(m_bHasGun, CE_CPropVehicleDriveable);
DEFINE_PROP(m_nScannerDisabledWeapons, CE_CPropVehicleDriveable);
DEFINE_PROP(m_nScannerDisabledVehicle, CE_CPropVehicleDriveable);
DEFINE_PROP(m_bEnterAnimOn, CE_CPropVehicleDriveable);
DEFINE_PROP(m_bExitAnimOn, CE_CPropVehicleDriveable);
DEFINE_PROP(m_hPlayer, CE_CPropVehicleDriveable);

DEFINE_PROP(m_pServerVehicle, CE_CPropVehicleDriveable);
DEFINE_PROP(m_bEngineLocked, CE_CPropVehicleDriveable);
DEFINE_PROP(m_bLocked, CE_CPropVehicleDriveable);
DEFINE_PROP(m_flMinimumSpeedToEnterExit, CE_CPropVehicleDriveable);
DEFINE_PROP(m_flTurnOffKeepUpright, CE_CPropVehicleDriveable);
DEFINE_PROP(m_hNPCDriver, CE_CPropVehicleDriveable);




// CE_CPropVehicleDriveable
SH_DECL_MANUALHOOK2_void(ProcessMovement, 0, 0, 0, CBaseEntity *, CMoveData *);
DECLARE_DEFAULTHANDLER_SUBCLASS_void(CE_CPropVehicleDriveable, IDrivableVehicle, ProcessMovement, (CBaseEntity *pPlayer, CMoveData *pMoveData), (pPlayer, pMoveData));
DECLARE_HOOK_SUBCLASS(ProcessMovement, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK4_void(SetupMove, 0, 0, 0, CBaseEntity *, CUserCmd *, IMoveHelper *, CMoveData *);
DECLARE_DEFAULTHANDLER_SUBCLASS_void(CE_CPropVehicleDriveable, IDrivableVehicle, SetupMove, (CBaseEntity *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move), (player, ucmd, pHelper, move));
DECLARE_HOOK_SUBCLASS(SetupMove, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK2(AllowBlockedExit, 0, 0, 0, bool, CBaseEntity *, int);
DECLARE_DEFAULTHANDLER_SUBCLASS(CE_CPropVehicleDriveable, IDrivableVehicle, AllowBlockedExit, bool, (CBaseEntity *pPlayer, int nRole), (pPlayer, nRole));
DECLARE_HOOK_SUBCLASS(AllowBlockedExit, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK1(CanExitVehicle, 0, 0, 0, bool, CBaseEntity *);
DECLARE_DEFAULTHANDLER_SUBCLASS(CE_CPropVehicleDriveable, IDrivableVehicle, CanExitVehicle, bool, (CBaseEntity *pEntity), (pEntity));
DECLARE_HOOK_SUBCLASS(CanExitVehicle, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK1_void(EnterVehicle, 0, 0, 0, CBaseEntity *);
DECLARE_DEFAULTHANDLER_SUBCLASS_void(CE_CPropVehicleDriveable, IDrivableVehicle, EnterVehicle, (CBaseEntity *pPassenger), (pPassenger));
DECLARE_HOOK_SUBCLASS(EnterVehicle, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK1_void(ExitVehicle, 0, 0, 0, int );
DECLARE_DEFAULTHANDLER_SUBCLASS_void(CE_CPropVehicleDriveable, IDrivableVehicle, ExitVehicle, (int nRole), (nRole));
DECLARE_HOOK_SUBCLASS(ExitVehicle, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK1(PassengerShouldReceiveDamage, 0, 0, 0, bool, CTakeDamageInfo & );
DECLARE_DEFAULTHANDLER_SUBCLASS(CE_CPropVehicleDriveable, IDrivableVehicle, PassengerShouldReceiveDamage, bool,(CTakeDamageInfo &info), (info));
DECLARE_HOOK_SUBCLASS(PassengerShouldReceiveDamage, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK0(GetDriver, 0, 0, 0, CBaseEntity * );
DECLARE_DEFAULTHANDLER_SUBCLASS(CE_CPropVehicleDriveable, IDrivableVehicle, GetDriver, CBaseEntity *, (), ());
DECLARE_HOOK_SUBCLASS(GetDriver, CE_CPropVehicleDriveable, IDrivableVehicle);






SH_DECL_MANUALHOOK4_void(DriveVehicle, 0, 0, 0, float , CUserCmd *, int, int );
DECLARE_HOOK(DriveVehicle, CE_CPropVehicleDriveable);
DECLARE_DEFAULTHANDLER_void(CE_CPropVehicleDriveable, DriveVehicle, (float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased), (flFrameTime, ucmd, iButtonsDown, iButtonsReleased));

SH_DECL_MANUALHOOK2_void(DampenEyePosition, 0, 0, 0, Vector &, QAngle &);
DECLARE_HOOK(DampenEyePosition, CE_CPropVehicleDriveable);
DECLARE_DEFAULTHANDLER_void(CE_CPropVehicleDriveable, DampenEyePosition, (Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles), (vecVehicleEyePos, vecVehicleEyeAngles));

SH_DECL_MANUALHOOK0(IsVehicleBodyInWater, 0, 0, 0, bool);
DECLARE_HOOK(IsVehicleBodyInWater, CE_CPropVehicleDriveable);
DECLARE_DEFAULTHANDLER(CE_CPropVehicleDriveable, IsVehicleBodyInWater, bool, (), ());

SH_DECL_MANUALHOOK0_void(CreateServerVehicle, 0, 0, 0);
DECLARE_HOOK(CreateServerVehicle, CE_CPropVehicleDriveable);
DECLARE_DEFAULTHANDLER_void(CE_CPropVehicleDriveable, CreateServerVehicle, (), ());

SH_DECL_MANUALHOOK0(IsOverturned, 0, 0, 0, bool);
DECLARE_HOOK(IsOverturned, CE_CPropVehicleDriveable);
DECLARE_DEFAULTHANDLER(CE_CPropVehicleDriveable, IsOverturned, bool, (), ());



void CE_CPropVehicleDriveable::StartEngine( void )
{
	if ( m_bEngineLocked )
	{
		m_VehiclePhysics->SetHandbrake( true );
		return;
	}

	m_VehiclePhysics->TurnOn();
}

void CE_CPropVehicleDriveable::StopEngine( void )
{
	m_VehiclePhysics->TurnOff();
}

bool CE_CPropVehicleDriveable::IsEngineOn( void )
{
	return m_VehiclePhysics->IsOn();
}



void CE_CPropVehicleDriveable::UpdateOnRemove()
{
	UTIL_Remove(m_hPointViewControl);
	m_hPointViewControl.Set(NULL);

	UTIL_Remove(m_hDriverRagdoll);
	m_hDriverRagdoll.Set(NULL);

	CBaseEntity *driver = GetDriver();
	if(driver)
	{
		CPlayer	*pPlayer = ToBasePlayer(CEntity::Instance(driver));
		if(pPlayer)
		{
			pPlayer->LeaveVehicle();
		}
	}
	BaseClass::UpdateOnRemove();
}

void CE_CPropVehicleDriveable::RemoveDriverRagdoll()
{
	UTIL_Remove(m_hDriverRagdoll);
	m_hDriverRagdoll.Set(NULL);

}

void CE_CPropVehicleDriveable::CreateDriverRagdoll(CPlayer *pPlayer)
{
	if(!pPlayer)
		return;

	if(m_hDriverRagdoll)
	{
		Assert(0);
		UTIL_Remove(m_hDriverRagdoll);
		m_hDriverRagdoll.Set(NULL);
	}

	CEntity *body = CreateEntityByName("prop_physics_override");
	if(!body)
		return;

	pPlayer->SetRenderMode(kRenderTransTexture);
	pPlayer->SetRenderColor(0,0,0,0);

	m_hDriverRagdoll.Set(body->BaseEntity());
	body->DispatchKeyValue("model", STRING(pPlayer->GetModelName()));
	body->DispatchKeyValue("skin", 0);

	DispatchSpawn(body->BaseEntity());
	body->Activate();

	body->AddEffects(1);
	body->AddEffects(128);
	body->AddEffects(512);
	body->SetParent(BaseEntity());
	body->SetParentAttachment("SetParentAttachment","vehicle_driver_eyes", false);

}

void CE_CPropVehicleDriveable::RemovePointViewControl(CPlayer *pPlayer)
{
	if(m_hPointViewControl)
	{
		m_hPointViewControl->SetParent(NULL);
		UTIL_Remove(m_hPointViewControl);
		m_hPointViewControl.Set(NULL);
	}
	m_bInDriverView = false;
}

void CE_CPropVehicleDriveable::PointViewToggle(CPlayer *pPlayer)
{
	if(m_bInDriverView)
	{
		Vector vecAttachPoint,forward;
		QAngle vecAttachAngles;
		if(GetAttachmentLocal(LookupAttachment( "vehicle_3rd" ), vecAttachPoint, vecAttachAngles))
		{
			pPlayer->SetLocalOrigin(vecAttachPoint);
			pPlayer->SetLocalAngles(vecAttachAngles);
			m_bInDriverView = false;
		}
		return;
	}

	if(!m_bInDriverView)
	{
		Vector vecAttachPoint,forward;
		QAngle vecAttachAngles;
		GetAttachmentLocal(LookupAttachment( "vehicle_driver_eyes" ), vecAttachPoint, vecAttachAngles);
		pPlayer->SetLocalOrigin(vecAttachPoint);
		pPlayer->SetLocalAngles(vecAttachAngles);
		m_bInDriverView = true;
		return;
	}
}

void CE_CPropVehicleDriveable::SetPointViewControl(CPlayer *pPlayer)
{
	if(m_hPointViewControl)
	{
		UTIL_Remove(m_hPointViewControl);
		m_hPointViewControl.Set(NULL);
	}

	if(!pPlayer)
		return;

	Vector vec = pPlayer->GetAbsOrigin();
	QAngle angle = pPlayer->GetAbsAngles();

	Vector vecAttachPoint;
	QAngle vecAttachAngles;
	GetAttachmentLocal(LookupAttachment( "vehicle_driver_eyes" ), vecAttachPoint, vecAttachAngles);

	CE_CSprite *pointview = CE_CSprite::SpriteCreate("materials/sprites/dot.vmt", vec, false);
	m_hPointViewControl.Set(pointview->BaseEntity());

	pointview->SetRenderColor(0, 0, 0, 0);
	pointview->SetAbsOrigin(vec);
	pointview->SetAbsAngles(angle);
	DispatchSpawn(pointview->BaseEntity());
	
	engine->SetView(pPlayer->edict(), pointview->edict());

	pointview->SetParent(pPlayer->BaseEntity());

	//vecAttachPoint.z = 0.0f;
	pPlayer->SetLocalOrigin(vecAttachPoint);
	pPlayer->SetLocalAngles(vecAttachAngles);
	m_bInDriverView = true;
}

