
#include "extension.h"
#include "GameSystem.h"
#include "tier3.h"
#include "datacache/imdlcache.h"
#include "scenefilecache/ISceneFileCache.h"
#include "MonsterTools.h"
#include "MonsterConfig.h"


Monster g_Monster;

bool g_bUseNetworkVars = false;

SMEXT_LINK(&g_Monster);

CGlobalVars *gpGlobals = NULL;
INetworkStringTableContainer *netstringtables = NULL;
INetworkStringTable *g_pStringTableParticleEffectNames = NULL;
INetworkStringTable *g_pStringTableMaterials = NULL;
IEngineSound *engsound = NULL;
IEngineTrace *enginetrace = NULL;
IServerGameClients *gameclients = NULL;
ICvar *icvar = NULL;
IUniformRandomStream *enginerandom = NULL;
IUniformRandomStream *random = NULL;
IStaticPropMgrServer *staticpropmgr = NULL;
IVModelInfo *modelinfo = NULL;
IPhysicsObjectPairHash *my_g_EntityCollisionHash = NULL;
ISpatialPartition *partition = NULL;
IPhysicsSurfaceProps *physprops = NULL;
IPhysicsCollision *physcollision = NULL;
IPhysicsEnvironment *physenv;
IPhysics *iphysics = NULL;
ISoundEmitterSystemBase *soundemitterbase = NULL;
IFileSystem *filesystem = NULL;
IEffects *g_pEffects = NULL;
IDecalEmitterSystem *decalsystem = NULL;
IEngineSound *enginesound = NULL;
ITempEntsSystem *te = NULL;
CSharedEdictChangeInfo *g_pSharedChangeInfo = NULL;
IGameMovement *g_pGameMovement = NULL;
IGameConfig *g_pGameConf = NULL;
IServerTools *servertools = NULL;
ISceneFileCache *scenefilecache = NULL;
IGameEventManager2 *gameeventmanager = NULL;

CBaseEntityList *g_pEntityList = NULL;

CEntity *my_g_WorldEntity = NULL;
CBaseEntity *my_g_WorldEntity_cbase = NULL;

int gCmdIndex;
int gMaxClients;

unsigned long serverdll_addr;
short g_sModelIndexSmoke;
short g_sModelIndexBubbles;
short g_sModelIndexFireball;
short g_sModelIndexBlueLight;
short g_sModelIndexBlueLaser;
short g_sModelIndexLaser;

bool CommandInitialize();
void PatchFunction();

//extern sp_nativeinfo_t g_MonsterNatives[];


SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, false, bool, const char *, const char *, const char *, const char *, bool, bool);
SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);
SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, 0);


#define	TEST_SIGNATURE		1

#if TEST_SIGNATURE

void *serveraddr;
bool CheckScanFunction(const char *sign_value, void *laddr);

size_t UTIL_DecodeHexString(unsigned char *buffer, size_t maxlength, const char *hexstr)
{
	size_t written = 0;
	size_t length = strlen(hexstr);

	for (size_t i = 0; i < length; i++)
	{
		if (written >= maxlength)
			break;
		buffer[written++] = hexstr[i];
		if (hexstr[i] == '\\' && hexstr[i + 1] == 'x')
		{
			if (i + 3 >= length)
				continue;
			/* Get the hex part. */
			char s_byte[3];
			int r_byte;
			s_byte[0] = hexstr[i + 2];
			s_byte[1] = hexstr[i + 3];
			s_byte[2] = '\0';
			/* Read it as an integer */
			sscanf(s_byte, "%x", &r_byte);
			/* Save the value */
			buffer[written - 1] = r_byte;
			/* Adjust index */
			i += 3;
		}
	}

	return written;
}

class Test_Signature : public ITextListener_SMC
{
	virtual void ReadSMC_ParseStart()
	{
		has_error = false;
		addrInBase = (void *)g_SMAPI->GetServerFactory(false);
		ignore = true;
	}

	virtual SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name)
	{
		if(strcmp(name,"Signatures") == 0)
		{
			ignore = false;
		}

		strncpy(current_name,name, strlen(name));
		current_name[strlen(name)] = '\0';

		return SMCResult_Continue;
	}

	virtual SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
	{
		if(!ignore && strcmp(key,"windows") == 0)
		{
			if(!CheckScanFunction(value, serveraddr)) {
				has_error = true;
				META_CONPRINTF("[%s DEBUG] %s - FAIL\n",g_Monster.GetLogTag(), current_name);
			}
			/*unsigned char real_sig[511];
			size_t real_bytes;
			size_t length;

			real_bytes = 0;
			length = strlen(value);

			real_bytes = UTIL_DecodeHexString(real_sig, sizeof(real_sig), value);
			if (real_bytes >= 1)
			{
				void *addr = memutils->FindPattern(addrInBase,(char *)real_sig,real_bytes);
				if(addr == NULL)
				{
					has_error = true;
					META_CONPRINTF("[%s DEBUG] %s - FAIL\n",g_Monster.GetLogTag(), current_name);
				}
			}*/
		}
		return SMCResult_Continue;
	}

public:
	bool HasError() { return has_error; }

private:
	bool ignore;
	void *addrInBase;
	char current_name[128];
	bool has_error;
} g_Test_Signature;
#endif


bool Monster::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	void *__laddr = reinterpret_cast<void *>(g_SMAPI->GetServerFactory(false));
	MEMORY_BASIC_INFORMATION mem;
	if(!VirtualQuery(__laddr, &mem, sizeof(MEMORY_BASIC_INFORMATION))) {
		return false;
	}
	if(mem.AllocationBase == NULL) {
		return false;
	}
	HMODULE dll = (HMODULE)mem.AllocationBase;
	serverdll_addr = (unsigned long)dll;

	//0EB829DE 
	//META_CONPRINTF("%p\n",reinterpret_cast<void *>(0x00100CE0 +serverdll_addr));
	//META_CONPRINTF("%p\n",reinterpret_cast<void *>(0x0EB15E6E -serverdll_addr));

	const char *game_foler = g_pSM->GetGameFolderName();

	/*if(stricmp(game_foler,"hl2mp") == 0 || 
	   stricmp(game_foler,"garrysmod") == 0 ||
	   stricmp(game_foler,"obsidian") == 0
	)*/

	if(stricmp(game_foler,"cstrike") != 0)
	{
		g_pSM->Format(error, maxlength, "NOT allow load this extension: %s", game_foler);
		return false;
	}

	char conf_error[255] = "";
	char config_path[255];
	snprintf(config_path, sizeof(config_path),"monster/monster.%s.games",game_foler);
	if (!gameconfs->LoadGameConfigFile(config_path, &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		if (conf_error[0])
		{
			g_pSM->Format(error, maxlength, "Could not read monster.%s.games: %s", game_foler, conf_error);
		}
		return false;
	}

#if TEST_SIGNATURE

	serveraddr = reinterpret_cast<void *>(g_SMAPI->GetServerFactory(false));


	char path[512];
	g_pSM->BuildPath(Path_SM,path, sizeof(path),"gamedata/monster/monster.%s.games.txt",game_foler);
	SMCStates state = {0, 0};
	textparsers->ParseFile_SMC(path, &g_Test_Signature, &state);

	if(g_Test_Signature.HasError())
	{
		g_pSM->LogError(myself, "Some Signature counld not found.");
		return false;
	}

#endif
	
	PatchFunction();

	if (!GetEntityManager()->Init(g_pGameConf))
	{
		Assert(0);
		g_pSM->LogError(myself, "CEntity failed to Initialize.");
		return false;
	}

	g_pSharedChangeInfo = engine->GetSharedEdictChangeInfo();

	g_pEntityList = (CBaseEntityList *)gamehelpers->GetGlobalEntityList();
	
	Assert(g_pEntityList);

	if(!IGameSystem::SDKInitAllSystems())
	{
		Assert(0);
		g_pSM->LogError(myself, "SDKInitAllSystems failed to Initialize. Server may Crash!");
		return false;
	}

	if(!CommandInitialize())
	{
		Assert(0);
		g_pSM->LogError(myself, "Command failed to Initialize. Server may Crash!");
		return false;
	}

	if(!g_helpfunc.Initialize())
	{
		Assert(0);
		g_pSM->LogError(myself, "Helper failed to Initialize. Server may Crash!");
		return false;
	}

	engine->ServerCommand("exec skill.cfg\n");
	engine->ServerExecute();

	IGameSystem::SDKInitPostAllSystems();

	//sharesys->AddNatives(myself, g_MonsterNatives);

 	return true;
}



void Monster::SDK_OnUnload()
{
	if(g_pGameConf != NULL)
	{
		gameconfs->CloseGameConfigFile(g_pGameConf);
		g_pGameConf = NULL;
	}
}

void Monster::SDK_OnAllLoaded()
{
}

bool Monster::QueryRunning(char *error, size_t maxlength)
{
	return true;
}

bool Monster::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	CreateInterfaceFn appSystemFactory = ismm->GetEngineFactory();
	ConnectTier1Libraries(&appSystemFactory, 1);
	ConnectTier2Libraries(&appSystemFactory, 1);
	ConnectTier3Libraries(&appSystemFactory, 1);

	MathLib_Init(2.2f, 2.2f, 0.0f, 2);
	
	memset(&g_MonsterConfig, 1, sizeof(MonsterConfig));

	g_MonsterConfig.m_bEnableRemoveDropWeapon = false;
	g_MonsterConfig.m_fNPCWeaponRemoveDelay = 15.0f;


	gpGlobals = ismm->GetCGlobals();

	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);

	GET_V_IFACE_CURRENT(GetEngineFactory, netstringtables, INetworkStringTableContainer, INTERFACENAME_NETWORKSTRINGTABLESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, engsound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
	GET_V_IFACE_CURRENT(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_CURRENT(GetEngineFactory, enginerandom, IUniformRandomStream, VENGINE_SERVER_RANDOM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, staticpropmgr, IStaticPropMgrServer, INTERFACEVERSION_STATICPROPMGR_SERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, modelinfo, IVModelInfo, VMODELINFO_SERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, partition, ISpatialPartition, INTERFACEVERSION_SPATIALPARTITION);
	GET_V_IFACE_CURRENT(GetPhysicsFactory, physprops, IPhysicsSurfaceProps,  VPHYSICS_SURFACEPROPS_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, soundemitterbase, ISoundEmitterSystemBase,  SOUNDEMITTERSYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetPhysicsFactory, physcollision, IPhysicsCollision,  VPHYSICS_COLLISION_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetServerFactory, g_pEffects, IEffects, IEFFECTS_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetPhysicsFactory, iphysics, IPhysics,  VPHYSICS_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, enginesound, IEngineSound,  IENGINESOUND_SERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, mdlcache, IMDLCache,  MDLCACHE_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetServerFactory, g_pGameMovement, IGameMovement, INTERFACENAME_GAMEMOVEMENT);
	GET_V_IFACE_ANY(GetServerFactory, servertools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, scenefilecache, ISceneFileCache, SCENE_FILE_CACHE_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, gameeventmanager, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);

	random = enginerandom;

	g_pCVar = icvar;
	
	ConVar_Register(0, this);

	ismm->AddListener(this, this);

	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, gamedll, this, &Monster::ServerActivate, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, serverclients, this, &Monster::SetCommandClient, true);

	IGameSystem::InitAllSystems();

	return true;
}

bool Monster::RegisterConCommandBase(ConCommandBase *pCommand)
{
	META_REGCVAR(pCommand);

	return true;
}

bool Monster::SDK_OnMetamodUnload(char *error, size_t maxlength)
{
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, gamedll, this, &Monster::ServerActivate, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, serverclients, this, &Monster::SetCommandClient, true);

	MonsterTools::OnMonsterShutdown();

	IGameSystem::SDKShutdownAllSystems();

	GetEntityManager()->Shutdown();

	DisconnectTier1Libraries();
	DisconnectTier2Libraries();
	DisconnectTier3Libraries();

	return true;
}

void Monster::Precache()
{
	g_sModelIndexSmoke = engine->PrecacheModel("sprites/steam1.vmt",true);
	
	g_sModelIndexBubbles = engine->PrecacheModel("sprites/bubble.vmt",true);
	
	g_sModelIndexFireball = engine->PrecacheModel("sprites/zerogxplode.vmt",true);

	g_sModelIndexBlueLight = engine->PrecacheModel("sprites/bluelight1.vmt",true);
	g_sModelIndexBlueLaser = engine->PrecacheModel("sprites/bluelaser1.vmt",true);

	g_sModelIndexLaser = engine->PrecacheModel("sprites/laser.vmt",true);

	engsound->PrecacheSound("player/pl_fallpain1.wav",true);
	engsound->PrecacheSound("player/pl_fallpain3.wav",true);


	soundemitterbase->AddSoundOverrides("scripts/sm_monster/game_sounds_BaseNpc.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_headcrab.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_fastheadcrab.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_blackheadcrab.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_zombie.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_fastzombie.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_poisonzombie.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_manhack.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_antlionguard.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_antlionguard_episodic2.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_antlionguard_episodic.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_stalker.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_antlion.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/game_sounds_weapons.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/game_sounds_items.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_vortigaunt.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_rollermine.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_antlion_episodic.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_combine_cannon.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_env_headcrabcanister.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_turret.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_attackheli.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_attackheli_episodic2.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_soldier.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_scanner.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_barnacle.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_gunship.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_strider.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_combinecamera.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/game_sounds_vehicles.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_combine_mine.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_zombine.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_dropship.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_birds.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_alyx.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_eli.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_gman.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_barney.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_dog.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_sniper.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_combine_ball.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/hl2_game_sounds_player.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_citizen.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_hunter.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_houndeye.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_bullsquid.txt");
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/level_sounds_coast.txt");
	//soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_advisor.txt");

	g_helpfunc.UTIL_PrecacheOther("npc_headcrab");
	g_helpfunc.UTIL_PrecacheOther("npc_headcrab_fast");
	g_helpfunc.UTIL_PrecacheOther("npc_headcrab_black");
	g_helpfunc.UTIL_PrecacheOther("npc_fastzombie");
	g_helpfunc.UTIL_PrecacheOther("npc_fastzombie_torso");
	g_helpfunc.UTIL_PrecacheOther("npc_zombie_torso");
	g_helpfunc.UTIL_PrecacheOther("npc_zombie");
	g_helpfunc.UTIL_PrecacheOther("npc_poisonzombie");
	g_helpfunc.UTIL_PrecacheOther("npc_manhack");
	g_helpfunc.UTIL_PrecacheOther("npc_antlionguard");
	g_helpfunc.UTIL_PrecacheOther("npc_stalker");
	g_helpfunc.UTIL_PrecacheOther("npc_antlion");
	g_helpfunc.UTIL_PrecacheOther("npc_vortigaunt");
	g_helpfunc.UTIL_PrecacheOther("npc_rollermine");
	g_helpfunc.UTIL_PrecacheOther("env_headcrabcanister");
	g_helpfunc.UTIL_PrecacheOther("npc_turret_floor");
	g_helpfunc.UTIL_PrecacheOther("npc_combine_s");
	g_helpfunc.UTIL_PrecacheOther("npc_helicopter");
	g_helpfunc.UTIL_PrecacheOther("npc_enemyfinder");
	g_helpfunc.UTIL_PrecacheOther("npc_enemyfinder_combinecannon");
	g_helpfunc.UTIL_PrecacheOther("npc_cscanner");
	g_helpfunc.UTIL_PrecacheOther("npc_clawscanner");
	g_helpfunc.UTIL_PrecacheOther("npc_barnacle");
	g_helpfunc.UTIL_PrecacheOther("npc_combinegunship");
	g_helpfunc.UTIL_PrecacheOther("npc_strider");
	g_helpfunc.UTIL_PrecacheOther("npc_combine_camera");
	g_helpfunc.UTIL_PrecacheOther("bounce_bomb");
	g_helpfunc.UTIL_PrecacheOther("npc_zombine");
	g_helpfunc.UTIL_PrecacheOther("npc_turret_ceiling");
	g_helpfunc.UTIL_PrecacheOther("npc_turret_ground");
	g_helpfunc.UTIL_PrecacheOther("npc_crow");
	g_helpfunc.UTIL_PrecacheOther("npc_seagull");
	g_helpfunc.UTIL_PrecacheOther("npc_pigeon");
	g_helpfunc.UTIL_PrecacheOther("npc_breen");
	g_helpfunc.UTIL_PrecacheOther("npc_barney");
	g_helpfunc.UTIL_PrecacheOther("npc_alyx");
	g_helpfunc.UTIL_PrecacheOther("npc_eli");
	g_helpfunc.UTIL_PrecacheOther("npc_gman");
	g_helpfunc.UTIL_PrecacheOther("npc_kleiner");
	g_helpfunc.UTIL_PrecacheOther("npc_dog");
	g_helpfunc.UTIL_PrecacheOther("npc_monk");
	g_helpfunc.UTIL_PrecacheOther("npc_mossman");
	g_helpfunc.UTIL_PrecacheOther("npc_sniper");
	g_helpfunc.UTIL_PrecacheOther("npc_metropolice");
	g_helpfunc.UTIL_PrecacheOther("npc_citizen");
	g_helpfunc.UTIL_PrecacheOther("npc_bullsquid");
	g_helpfunc.UTIL_PrecacheOther("npc_houndeye");
	g_helpfunc.UTIL_PrecacheOther("npc_hunter");
	//g_helpfunc.UTIL_PrecacheOther("npc_advisor");
}

void Monster::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	gMaxClients = clientMax;

	RETURN_META(MRES_IGNORED);
}

void Monster::SetCommandClient( int cmd )
{
	gCmdIndex = cmd + 1;
}

int Monster::GetCommandClient()
{
	return gCmdIndex;
}

int Monster::GetMaxClients()
{
	return gMaxClients;
}


void *Monster::OnMetamodQuery(const char *iface, int *ret)
{
	if(strcmp(iface, MM_INTERFACE_MONSTER_NAME) == 0)
	{
		if(ret)
		{
			*ret = IFACE_OK;
		}
		return static_cast<void *>(&g_MonsterTools);
	}

	return NULL;
}
