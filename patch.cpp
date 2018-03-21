#include "CDetour/detours.h"
#include "GameSystem.h"
#include "CCombatCharacter.h"
#include "player_pickup.h"
#include "CPlayer.h"
#include "CAI_Network.h"
#include "MonsterConfig.h"
#include "CPhysicsProp.h"
#include "CTrigger.h"



void PatchFunction()
{
}

class CCleanupUtlSymbolTable : public CBaseGameSystem
{
public:
	CCleanupUtlSymbolTable(const char *name) : CBaseGameSystem(name)
	{
	}
	bool CUtlSymbol_Initialize()
	{
		void *func = NULL;
		if(!func)
		{
			if(!g_pGameConf->GetMemSig("CUtlSymbol::Initialize", &func))
			{
				assert(0);
				return false;
			}
		}
		typedef void (*_func)();
		_func thisfunc = (_func)func;
		thisfunc();
		return true;
	}

	void SDKShutdown()
	{
		// server.dll CCleanupUtlSymbolTable will delete s_pSymbolTable at exit
		// we set our s_pSymbolTable to NULL
		CUtlSymbol::s_pSymbolTable = old_s_pSymbolTable;
	}

	bool SDKInit()
	{
		// link server.dll s_pSymbolTable our s_pSymbolTable (tier1.lib)
		if(!CUtlSymbol_Initialize()) // init server.dll s_pSymbolTable
			return false;

		CUtlSymbol::Initialize(); // init out s_pSymbolTable, set symbolsInitialized = true
		old_s_pSymbolTable = CUtlSymbol::s_pSymbolTable;
		CUtlSymbolTableMT *s_pSymbolTable;
		GET_VARIABLE_POINTER(s_pSymbolTable, CUtlSymbolTableMT *);
		CUtlSymbol::s_pSymbolTable = s_pSymbolTable;

		return true;
	}
	
	CUtlSymbolTableMT *old_s_pSymbolTable;
};

static CCleanupUtlSymbolTable g_CCleanupUtlSymbolTable("CUtlSymbol_Patch");

void CleanupUtlSymbolTable_Shutdown()
{
	g_CCleanupUtlSymbolTable.SDKShutdown();
}

class PatchSystem : public CBaseGameSystem
{
public:
	PatchSystem(const char *name) : CBaseGameSystem(name)
	{
	}
	bool SDKInit();
	void SDKShutdown();
	void FixParentedPreCleanup();
	void FixParentedPostCleanup();
};

static PatchSystem g_patchsystem("PatchSystem");


CDetour *AllocateDefaultRelationships_CDetour = NULL;
CDetour *UTIL_BloodDrips_CDetour = NULL;
CDetour *ShouldRemoveThisRagdoll_CDetour = NULL;
CDetour *FindInList_CDetour = NULL;
CDetour *Pickup_ForcePlayerToDropThisObject_CDetour = NULL;
CDetour *UTIL_GetLocalPlayer_CDetour = NULL;
CDetour *CleanUpMap_CDetour = NULL;
CDetour *CAI_NetworkBuilder_Build_CDetour = NULL;
CDetour *CAI_NetworkBuilder_InitNodePosition_CDetour = NULL;
CDetour *CBreakableProp_HandleFirstCollisionInteractions_CDetour = NULL;
CDetour *CCSPlayer_GetBulletTypeParameters_CDetour = NULL;
CDetour *CTriggerHurt_HurtAllTouchers_CDetour = NULL;


#define MAX_BROKEN_PARENTING_ENTS		4
const char broken_parenting_ents[][32] = {"move_rope",
   "keyframe_rope",
   "info_target",
   "func_brush"};


// fix CLASS_
DETOUR_DECL_MEMBER0(CDetour_AllocateDefaultRelationships, void)
{
	CCombatCharacter::AllocateDefaultRelationships();
}

// fix npc blood
DETOUR_DECL_STATIC4(CDetour_UTIL_BloodDrips, void, const Vector &, origin, const Vector &, direction, int, color, int, amount)
{
	IPredictionSystem::SuppressHostEvents(NULL);
	DETOUR_STATIC_CALL(CDetour_UTIL_BloodDrips)(origin, direction, color, amount);
}

// fix crash
DETOUR_DECL_STATIC0(CDetour_ShouldRemoveThisRagdoll, bool)
{
	return true;
}

// fix env_gunfire target miss on CleanUpMap()
DETOUR_DECL_STATIC2(CDetour_FindInList, bool, const char **,pStrings, const char *,pToFind)
{
	if(strcmp(pToFind,"info_target") == 0 ||
		strcmp(pToFind,"func_brush") == 0 ||
		strcmp(pToFind,"move_rope") == 0 ||
		strcmp(pToFind,"keyframe_rope") == 0 ||
		strcmp(pToFind,"env_beam") == 0)
		return true;

	if(strcmp(pToFind, "info_player_start") == 0)
		return true;

	bool ret = DETOUR_STATIC_CALL(CDetour_FindInList)(pStrings,pToFind);
	return ret;
}

// fix UTIL_GetLocalPlayer
DETOUR_DECL_STATIC1(CDetour_Pickup_ForcePlayerToDropThisObject, void, CBaseEntity *, pTarget)
{
	Pickup_ForcePlayerToDropThisObject(CEntity::Instance(pTarget));
}

// fix All UTIL_GetLocalPlayer
DETOUR_DECL_STATIC0(CDetour_UTIL_GetLocalPlayer, CBaseEntity *)
{
	for(int i=1;i<=gpGlobals->maxClients;i++)
	{
		CPlayer *pPlayer = UTIL_PlayerByIndex(i);
		if(!pPlayer)
			continue;
		return pPlayer->BaseEntity();
	}

	g_pSM->LogError(myself, "UTIL_GetLocalPlayer return NULL!");
	return NULL;
}

// fix parent entity
DETOUR_DECL_MEMBER0(CDetour_CleanUpMap, void)
{
	if(g_MonsterConfig.m_bEnable_CleanUpMap)
	{
		g_patchsystem.FixParentedPreCleanup();
		DETOUR_MEMBER_CALL(CDetour_CleanUpMap)();
		g_patchsystem.FixParentedPostCleanup();
	}
}

// patch CAI_NetworkBuildHelper
static bool g_In_CAI_NetworkBuilder_Build = false;
extern void AdjustStriderNodePosition( CAI_Network *pNetwork, CAI_Node *pNode );

DETOUR_DECL_MEMBER1(CDetour_CAI_NetworkBuilder_Build, void, CAI_Network *,pNetwork )
{
	g_In_CAI_NetworkBuilder_Build = true;
	DETOUR_MEMBER_CALL(CDetour_CAI_NetworkBuilder_Build)(pNetwork);
	g_In_CAI_NetworkBuilder_Build = false;
}

DETOUR_DECL_MEMBER2(CDetour_CAI_NetworkBuilder_InitNodePosition, void, CAI_Network *, pNetwork, CAI_Node *, pNode)
{
	DETOUR_MEMBER_CALL(CDetour_CAI_NetworkBuilder_InitNodePosition)(pNetwork, pNode);
	if(g_In_CAI_NetworkBuilder_Build)
	{
		AdjustStriderNodePosition(pNetwork, pNode);
	}
}

// patch IParentPropInteraction
DETOUR_DECL_MEMBER2(CDetour_CBreakableProp_HandleFirstCollisionInteractions, void, int, index, gamevcollisionevent_t *, pEvent)
{
	DETOUR_MEMBER_CALL(CDetour_CBreakableProp_HandleFirstCollisionInteractions)(index, pEvent);
	CEntity *pEntity = CEntity::Instance((CBaseEntity *)this);
	Assert(pEntity);
	if(pEntity) {
		CE_CPhysicsProp *prop = dynamic_cast<CE_CPhysicsProp *>(pEntity);
		Assert(prop);
		if(prop)
		{
			prop->HandleFirstCollisionInteractions(index, pEvent);
		}
	}
}


DETOUR_DECL_MEMBER3(CDetour_CCSPlayer_GetBulletTypeParameters, void, int, iBulletType, float &, float_1, float &, float_2)
{
	DETOUR_MEMBER_CALL(CDetour_CCSPlayer_GetBulletTypeParameters)(iBulletType, float_1, float_2);
	if(g_MonsterConfig.m_bEnable_CSS_Weapon_Penetration == false)
	{
		float_2 = 0.0f;
	}
}

// fix crash
DETOUR_DECL_MEMBER1(CDetour_CTriggerHurt_HurtAllTouchers, int, float, dt)
{
	CTriggerHurt *trigger = (CTriggerHurt *)CEntity::Instance((CBaseEntity *)this);
	Assert(trigger);
	return trigger->HurtAllTouchers(dt);
}

//fix bsp ain
#define NETWORK_GRAPH_PATCH_SIZE 7
unsigned char backup_network_graph_data[NETWORK_GRAPH_PATCH_SIZE];
void *network_graph_callback_addr = NULL;
void *network_graph_patch_addr = NULL;

void Local_LoadNetworkGraph(CUtlBuffer &buf)
{
	int version = buf.GetInt();

	if(version != 37) {
		char szNrpFilename[MAX_PATH];
		snprintf(szNrpFilename, sizeof(szNrpFilename), "maps/graphs/%s.ain", gpGlobals->mapname);
		FileHandle_t fh = filesystem->Open(szNrpFilename, "r", "MOD");
		if(fh) {
			buf.Clear();
			filesystem->ReadToBuffer(fh, buf);
			filesystem->Close(fh);
		}
	}
	buf.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
}

__declspec(naked) void patch_LoadNetworkGraph()
{
	__asm {
		lea		eax, [ebp-3Ch]
		push	eax
		call	Local_LoadNetworkGraph
		add		esp, 4
		call	network_graph_callback_addr
	}
}

void UnLoadNetworkGraph_Fix()
{
	if(network_graph_patch_addr == NULL) {
		return;
	}

	DWORD oldflags;
    VirtualProtect(network_graph_patch_addr, NETWORK_GRAPH_PATCH_SIZE, PAGE_EXECUTE_READWRITE, &oldflags);
	
	memcpy(network_graph_patch_addr, &backup_network_graph_data, NETWORK_GRAPH_PATCH_SIZE);

	VirtualProtect(network_graph_patch_addr, NETWORK_GRAPH_PATCH_SIZE, oldflags, &oldflags);
}

void LoadNetworkGraph_Fix()
{
	void *func = NULL;
	if(!g_pGameConf->GetMemSig("CAI_NetworkManager::LoadNetworkGraph", &func)) {
		return;
	}

	int offset = 389;
	network_graph_patch_addr = reinterpret_cast<void *>(((int)func)+offset);
	memcpy(&backup_network_graph_data, network_graph_patch_addr, sizeof(backup_network_graph_data));
	if(backup_network_graph_data[0] != 0x68) {
		META_CONPRINTF("[%s] LoadNetworkGraph_Fix fail!\n",g_Monster.GetLogTag());
		return;
	}

	DWORD oldflags;
    VirtualProtect(network_graph_patch_addr, NETWORK_GRAPH_PATCH_SIZE, PAGE_EXECUTE_READWRITE, &oldflags);
	memset(network_graph_patch_addr,0x90,NETWORK_GRAPH_PATCH_SIZE);

	unsigned char patch_data[5];
	patch_data[0] = 0xE9;
	*((long*)((long)(patch_data) + sizeof(unsigned char))) = (char*)patch_LoadNetworkGraph - (char*)network_graph_patch_addr - 5;

	memcpy(network_graph_patch_addr, &patch_data, sizeof(patch_data));

    VirtualProtect(network_graph_patch_addr, NETWORK_GRAPH_PATCH_SIZE, oldflags, &oldflags);
	network_graph_callback_addr = reinterpret_cast<void *>(((int)network_graph_patch_addr)+5);
}

bool PatchSystem::SDKInit()
{
	GET_DETOUR(AllocateDefaultRelationships, DETOUR_CREATE_MEMBER(CDetour_AllocateDefaultRelationships, "AllocateDefaultRelationships"));

	GET_DETOUR(UTIL_BloodDrips, DETOUR_CREATE_STATIC(CDetour_UTIL_BloodDrips, "UTIL_BloodDrips"));
		
	GET_DETOUR(ShouldRemoveThisRagdoll, DETOUR_CREATE_STATIC(CDetour_ShouldRemoveThisRagdoll, "ShouldRemoveThisRagdoll"));
	
	GET_DETOUR(FindInList, DETOUR_CREATE_STATIC(CDetour_FindInList, "FindInList"));

	GET_DETOUR(Pickup_ForcePlayerToDropThisObject, DETOUR_CREATE_STATIC(CDetour_Pickup_ForcePlayerToDropThisObject, "Pickup_ForcePlayerToDropThisObject"));

	GET_DETOUR(UTIL_GetLocalPlayer, DETOUR_CREATE_STATIC(CDetour_UTIL_GetLocalPlayer, "UTIL_GetLocalPlayer"));

	GET_DETOUR(CleanUpMap, DETOUR_CREATE_MEMBER(CDetour_CleanUpMap, "CleanUpMap"));

	GET_DETOUR(CAI_NetworkBuilder_Build, DETOUR_CREATE_MEMBER(CDetour_CAI_NetworkBuilder_Build, "CAI_NetworkBuilder::Build"));

	GET_DETOUR(CAI_NetworkBuilder_InitNodePosition, DETOUR_CREATE_MEMBER(CDetour_CAI_NetworkBuilder_InitNodePosition, "CAI_NetworkBuilder::InitNodePosition"));

	GET_DETOUR(CBreakableProp_HandleFirstCollisionInteractions, DETOUR_CREATE_MEMBER(CDetour_CBreakableProp_HandleFirstCollisionInteractions, "CBreakableProp::HandleFirstCollisionInteractions"));

	GET_DETOUR(CCSPlayer_GetBulletTypeParameters, DETOUR_CREATE_MEMBER(CDetour_CCSPlayer_GetBulletTypeParameters, "CCSPlayer::GetBulletTypeParameters"));

	GET_DETOUR(CTriggerHurt_HurtAllTouchers, DETOUR_CREATE_MEMBER(CDetour_CTriggerHurt_HurtAllTouchers, "CTriggerHurt::HurtAllTouchers"));

	LoadNetworkGraph_Fix();

	return true;
}

#define DestoryDetour(d) \
	if(d != NULL) \
		d->DisableDetour(); \
	d = NULL;


void PatchSystem::SDKShutdown()
{
	DestoryDetour(AllocateDefaultRelationships_CDetour);

	DestoryDetour(UTIL_BloodDrips_CDetour);
	DestoryDetour(ShouldRemoveThisRagdoll_CDetour);
	DestoryDetour(FindInList_CDetour);
	DestoryDetour(Pickup_ForcePlayerToDropThisObject_CDetour);
	DestoryDetour(UTIL_GetLocalPlayer_CDetour);
	DestoryDetour(CleanUpMap_CDetour);
	DestoryDetour(CAI_NetworkBuilder_Build_CDetour);
	DestoryDetour(CAI_NetworkBuilder_InitNodePosition_CDetour);
	DestoryDetour(CBreakableProp_HandleFirstCollisionInteractions_CDetour);
	DestoryDetour(CCSPlayer_GetBulletTypeParameters_CDetour);
	DestoryDetour(CTriggerHurt_HurtAllTouchers_CDetour);

	UnLoadNetworkGraph_Fix();
}

void PatchSystem::FixParentedPreCleanup()
{
	for(int i=1;i<=gpGlobals->maxClients;i++)
	{
		CPlayer *pPlayer = UTIL_PlayerByIndex(i);
		if(!pPlayer)
			continue;

		pPlayer->LeaveVehicle();
	}

	for(int i=0;i<MAX_BROKEN_PARENTING_ENTS;i++)
	{
		const char *name = broken_parenting_ents[i];
		CEntity *pSearch = NULL;
		while ( ( pSearch = g_helpfunc.FindEntityByClassname( pSearch, name ) ) != NULL )
		{
			CEntity *parent = pSearch->GetParent();
			if(parent)
			{
				pSearch->szCashedParentName =  parent->GetEntityName_String();
				pSearch->SetParent(NULL);
				if(pSearch->m_vecOriginalPos == vec3_origin)
				{
					pSearch->m_vecOriginalPos = pSearch->GetLocalOrigin();
				}
			}
		}
	}
}

void PatchSystem::FixParentedPostCleanup()
{
	for(int i=0;i<MAX_BROKEN_PARENTING_ENTS;i++)
	{
		const char *name = broken_parenting_ents[i];
		CEntity *pSearch = NULL;
		while ( ( pSearch = g_helpfunc.FindEntityByClassname( pSearch, name ) ) != NULL )
		{
			if(pSearch->szCashedParentName == NULL_STRING)
				continue;

			if(pSearch->m_vecOriginalPos != vec3_origin)
			{
				pSearch->SetLocalOrigin(pSearch->m_vecOriginalPos);
			}
			CEntity *parent = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, STRING(pSearch->szCashedParentName) );
			if(parent)
			{
				pSearch->SetParent(parent->BaseEntity());
			}
		}
	}
}




