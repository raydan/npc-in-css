
#include "CAI_NPC.h"
#include "eventqueue.h"
#include "CCombatWeapon.h"
#include "effect_dispatch_data.h"
#include "CE_recipientfilter.h"
#include "CGib.h"
#include "grenade_frag.h"
#include "weapon_rpg.h"
#include "CPropDoor.h"
#include "weapon_rpg_replace.h"
#include "CAI_Route.h"
#include "CPropVehicle.h"
#include "soundenvelope.h"
#include "physics_shared.h"
#include "physics_collisionevent.h"
#include "sceneentity.h"


ConVar *sv_gravity = NULL;
ConVar *phys_pushscale = NULL;
ConVar *npc_height_adjust = NULL;
ConVar *ai_path_adjust_speed_on_immediate_turns = NULL;
ConVar *ai_path_insert_pause_at_obstruction = NULL;
ConVar *ai_path_insert_pause_at_est_end = NULL;
ConVar *scene_flatturn = NULL;
ConVar *ai_use_clipped_paths = NULL;
ConVar *ai_moveprobe_usetracelist = NULL;
ConVar *ai_use_visibility_cache = NULL;
ConVar *ai_strong_optimizations = NULL;

ConVar *violence_hblood = NULL;
ConVar *violence_ablood = NULL;
ConVar *violence_hgibs = NULL;
ConVar *violence_agibs = NULL;

ConVar *sv_suppress_viewpunch = NULL;
ConVar *ai_navigator_generate_spikes = NULL;
ConVar *ai_navigator_generate_spikes_strength = NULL;

ConVar *ai_no_node_cache = NULL;
ConVar *sv_stepsize = NULL;
ConVar *hl2_episodic = NULL;
ConVar *ai_follow_use_points = NULL;
ConVar *ai_LOS_mode = NULL;
ConVar *ai_follow_use_points_when_moving = NULL;

ConVar *ammo_hegrenade_max = NULL;

ConVar *sv_strict_notarget = NULL;

ConVar *ai_shot_bias_min = NULL;
ConVar *ai_shot_bias_max = NULL;
ConVar *ai_shot_bias = NULL;
ConVar *ai_spread_pattern_focus_time = NULL;

ConVar *ai_lead_time = NULL;

ConVar *scene_clamplookat = NULL;
ConVar *scene_showfaceto = NULL;
ConVar *flex_maxawaytime = NULL;
ConVar *flex_minawaytime = NULL;
ConVar *flex_maxplayertime = NULL;
ConVar *flex_minplayertime = NULL;
ConVar *ai_find_lateral_los = NULL;
ConVar *npc_sentences = NULL;
ConVar *ai_find_lateral_cover = NULL;
ConVar *rr_debugresponses = NULL;

ConVar *sk_ally_regen_time = NULL;
ConVar *sv_npc_talker_maxdist = NULL;
ConVar *ai_no_talk_delay = NULL;
ConVar *rr_debug_qa = NULL;
ConVar *npc_ally_deathmessage = NULL;

ConVar *ai_enable_fear_behavior = NULL;
ConVar *ai_fear_player_dist = NULL;
ConVar *g_debug_transitions = NULL;
ConVar *ai_no_local_paths = NULL;

ConVar *ai_use_think_optimizations = NULL;
ConVar *ai_use_efficiency = NULL;
ConVar *ai_efficiency_override = NULL;
ConVar *ai_frametime_limit = NULL;
ConVar *ai_default_efficient = NULL;
ConVar *ai_shot_stats = NULL;
ConVar *ai_shot_stats_term = NULL;
ConVar *ai_spread_cone_focus_time = NULL;
ConVar *ai_spread_defocused_cone_multiplier = NULL;

ConVar *mat_dxlevel = NULL;
ConVar *npc_vphysics = NULL;

ConVar *ai_no_steer = NULL;
ConVar *ai_debug_directnavprobe = NULL;

ConVar *r_JeepViewZHeight = NULL;
ConVar *r_JeepViewDampenFreq = NULL;
ConVar *r_JeepViewDampenDamp = NULL;
ConVar *r_VehicleViewDampen = NULL;
ConVar *r_vehicleBrakeRate = NULL;

ConVar *xbox_throttlebias = NULL;
ConVar *xbox_throttlespoof = NULL;
ConVar *xbox_autothrottle = NULL;
ConVar *xbox_steering_deadzone = NULL;

ConVar *g_debug_vehicledriver = NULL;
ConVar *g_debug_npc_vehicle_roles = NULL;
ConVar *g_jeepexitspeed = NULL;
ConVar *hud_jeephint_numentries = NULL;

ConVar *sv_client_predict = NULL;

extern INetworkStringTable *g_pStringTableParticleEffectNames;
extern CCollisionEvent *g_Collisions;


CEntity *g_cent;

ConVar npc_create_equipment("npc_create_equipment", "");


void CC_NPC_Create( const CCommand &args )
{
	CPlayer* pPlayer = UTIL_PlayerByIndex(g_Monster.GetCommandClient());
	if(pPlayer == NULL)
		return;

	// Try to create entity
	CAI_NPC *baseNPC = dynamic_cast< CAI_NPC * >( CreateEntityByName(args[1]) );
	if (baseNPC)
	{
		baseNPC->DispatchKeyValue( "additionalequipment", npc_create_equipment.GetString() );
		baseNPC->Precache();

		if ( args.ArgC() == 3 )
		{
			baseNPC->SetName(args[2]);
		}

		DispatchSpawn(baseNPC->BaseEntity());
		// Now attempt to drop into the world		
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
			pPlayer->BaseEntity(), COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0)
		{
			if (baseNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY)
			{
				Vector pos = tr.endpos - forward * 36;
				baseNPC->Teleport( &pos, NULL, NULL );
			}
			else
			{
				// Raise the end position a little up off the floor, place the npc and drop him down
				tr.endpos.z += 12;
				baseNPC->Teleport( &tr.endpos, NULL, NULL );
				UTIL_DropToFloor( baseNPC, MASK_NPCSOLID );
			}

			// Now check that this is a valid location for the new npc to be
			Vector	vUpBit = baseNPC->GetAbsOrigin();
			vUpBit.z += 1;

			UTIL_TraceHull( baseNPC->GetAbsOrigin(), vUpBit, baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 
				MASK_NPCSOLID, baseNPC->BaseEntity(), COLLISION_GROUP_NONE, &tr );
			if ( tr.startsolid || (tr.fraction < 1.0) )
			{
				baseNPC->SUB_Remove();
				DevMsg("Can't create %s.  Bad Position!\n",args[1]);
			}
		}

		baseNPC->Activate();
	}
}

#include <iostream>
#include <fstream>
#include "prop_combine_ball.h"

int g_node = 1;
void cmd1_CommandCallback(const CCommand &command)
{
	int client = g_Monster.GetCommandClient();
	if(client)
	{
		Vector vec(501.0f,22.7f,70.21f);
		///vec.z -= 50.0f;
	
		CPlayer *player = ToBasePlayer(CEntity::Instance(client));
		if(!player)
			return;


		/*CCombatWeapon *weapon = player->GetActiveWeapon();
		if(!weapon)
			return;
		
		CWeaponRPG *rpg = ToCWeaponRPG(weapon);
		if(!rpg)
			return;

		rpg->CreateRPG(player);*/

		
		std::ofstream program3data;
		std::ofstream outputFile;
		
		outputFile.open("add.txt", std::ios_base::out | std::ios_base::app);
	
		Vector v = player->GetAbsOrigin();
		v.z += 1.0f;
		
		outputFile << "add:\n";
		outputFile << "{\n";
		outputFile << "\"origin\" \"" << (int)v.x << " " << (int)v.y << " " << (int)v.z << "\"\n";
		outputFile << "\"nodeid\" \"" << g_node << "\"\n";
		outputFile << "\"classname\" \"info_node\"\n";
		outputFile << "}\n\n";

		outputFile.close();

		g_node++;

	} else {
		Vector vec(501.0f,22.7f,70.21f);

		//vec.Init(6220.0f, 2813.0f, 1090.0f);

		//vec.Init(73.18,-54.81,-60.0);

		//vec.Init(952.65466,61.566082,-58.339985);


		//CEntity *cent = CreateEntityByName("npc_headcrab");
		//CEntity *cent = CreateEntityByName("npc_headcrab_fast");
		//CEntity *cent = CreateEntityByName("npc_headcrab_black");

		//CEntity *cent = CreateEntityByName("npc_fastzombie");
		//CEntity cent = CreateEntityByName("npc_fastzombie_torso");
		//CEntity *cent = CreateEntityByName("npc_zombie_torso");
		//CEntity *cent = CreateEntityByName("npc_zombie");
		//CEntity *cent = CreateEntityByName("npc_poisonzombie");
		
		//CEntity *cent = CreateEntityByName("npc_manhack");
		CEntity *cent = CreateEntityByName("npc_antlionguard");

		//CEntity *cent = CreateEntityByName("npc_stalker");

		//CEntity *cent = CreateEntityByName("npc_antlion");
		//cent->AddSpawnFlags(( 1 << 18 ));

		//CEntity *cent = CreateEntityByName("npc_vortigaunt");

		//CEntity *cent = CreateEntityByName("npc_rollermine");
		
		//CEntity *cent = CreateEntityByName("npc_test");
		
		/*CEntity *cent = CreateEntityByName("env_headcrabcanister");
	
		cent->DispatchKeyValue("HeadcrabType", "0");
		cent->DispatchKeyValue("HeadcrabCount", "10");
		
		cent->DispatchKeyValue("SmokeLifetime","60");
		cent->DispatchKeyValue("SkyboxCannisterCount","1");
		cent->DispatchKeyValue("DamageRadius","0");
		cent->DispatchKeyValue("Damage","100");*/

		//CEntity *cent = CreateEntityByName("npc_turret_floor");

		//CEntity *cent = CreateEntityByName("npc_combine");
			
		//CEntity *cent = CreateEntityByName("npc_combine_s");

		//cent->CustomDispatchKeyValue("model","models/combine_super_soldier.mdl");
		//cent->CustomDispatchKeyValue("NumGrenades","5");
		//cent->CustomDispatchKeyValue("additionalequipment","weapon_smg1");

		//cent->DispatchKeyValue("tacticalvariant","1");

		//CEntity *cent = CreateEntityByName("npc_helicopter");
		
		//CEntity *cent = CreateEntityByName("npc_enemyfinder");
		
		//CEntity *cent = CreateEntityByName("npc_enemyfinder_combinecannon");

		//CEntity *cent = CreateEntityByName("npc_cscanner");

		//CEntity *cent = CreateEntityByName("npc_clawscanner");
		
		//CEntity *cent = CreateEntityByName("npc_barnacle");

		//CEntity *cent = CreateEntityByName("npc_combinegunship");

		//CEntity *cent = CreateEntityByName("npc_strider");
		
		//CEntity *cent = CreateEntityByName("npc_combine_camera");

		//CEntity *cent = CreateEntityByName("bounce_bomb");
		
		//CEntity *cent = CreateEntityByName("npc_zombine");

		//CEntity *cent = CreateEntityByName("npc_combinedropship");

		//CEntity *cent = CreateEntityByName("npc_turret_ceiling");

		//CEntity *cent = CreateEntityByName("npc_turret_ground");
		
		//CEntity *cent = CreateEntityByName("npc_barney");
		
		//CEntity *cent = CreateEntityByName("npc_alyx");

		//CEntity *cent = CreateEntityByName("npc_sniper");

		//CEntity *cent = CreateEntityByName("npc_metropolice");
		//cent->CustomDispatchKeyValue("additionalequipment","weapon_stunstick");

		//CEntity *cent = CreateEntityByName("npc_citizen");
		//cent->CustomDispatchKeyValue("additionalequipment","weapon_smg1");


		/*CEntity *cent = CreateEntityByName("prop_vehicle_jeep");
		
		cent->CustomDispatchKeyValue("vehiclescript", "scripts/vehicles/jeep_test.txt");
		cent->CustomDispatchKeyValue("model", "models/buggy.mdl");
		
		//cent->CustomDispatchKeyValue("vehiclescript", "scripts/vehicles/zx2_natalyas_mustang.txt");		
		//cent->CustomDispatchKeyValue("model", "models/natalya/vehicles/natalyas_mustang_2.mdl");
		//cent->CustomDispatchKeyValue("model", "models/natalya/vehicles/charger_2.mdl");
		

		cent->CustomDispatchKeyValue("solid","6");
		cent->CustomDispatchKeyValue("skin","0");
		cent->CustomDispatchKeyValue("actionScale","1");
		cent->CustomDispatchKeyValue("EnableGun","1");
		cent->CustomDispatchKeyValue("ignorenormals","0");
		cent->CustomDispatchKeyValue("fadescale","1");
		cent->CustomDispatchKeyValue("fademindist","-1");
		cent->CustomDispatchKeyValue("VehicleLocked","0");
		cent->CustomDispatchKeyValue("screenspacefade","0");
		//cent->CustomDispatchKeyValue("spawnflags", "256" );

		*/

		//CEntity *cent = CreateEntityByName("prop_dynamic_override");

		//CEntity *cent = CreateEntityByName("npc_hunter");

		//CEntity *cent = CreateEntityByName("prop_combine_ball");

		CBaseEntity *cbase = cent->BaseEntity();

		CAI_NPC *hc = dynamic_cast<CAI_NPC *>(cent);
		//hc->AddSpawnFlags(( 1 << 18 ));

		cent->Teleport(&vec, NULL,NULL);

		//cent->AddSpawnFlags(4096);

		cent->Spawn();
		cent->Activate();

		g_cent = cent;


		/*CPlayer *pPlayer = UTIL_GetNearestPlayer(vec);
		if(pPlayer)
		{
			Vector vv = vec;
			vv.x -= 100;
			vv.y -= 100;
			pPlayer->Teleport(&vv, NULL, NULL);
		}*/

		/*vec.z += 100.0f;
		cent = CreateEntityByName("npc_fastzombie");

		cent->Teleport(&vec, NULL,NULL);
		cent->Spawn();*/

		/*CCheckTransmitInfo *info = new CCheckTransmitInfo();
		info->m_pTransmitEdict = new CBitVec<MAX_EDICTS>;
		info->m_pTransmitAlways =  new CBitVec<MAX_EDICTS>;



		cent->SetTransmit(info, true);*/

		//hc->GetSequenceKeyValues( 0 );
		

		//g_CEventQueue->AddEvent( cbase, "SelfDestruct", 0.5f, cbase,cbase );

		//hc->Dissolve(NULL, gpGlobals->curtime, false, 0 );

		edict_t *pEdict = servergameents->BaseEntityToEdict(cbase);
		META_CONPRINTF("%p %d %d\n",cbase, cent->entindex_non_network(), engine->IndexOfEdict(pEdict));

	}
}

#define GET_ACT 0

#if GET_ACT
#include <iostream>
#include <fstream>
#endif

void Test(char *name)
{
	CEntity *cent = CreateEntityByName(name);
	META_CONPRINTF("%s\n",name);
}

void cmd2_CommandCallback(const CCommand &command)
{
#if GET_ACT
	std::ofstream program3data;
	std::ofstream outputFile("act.txt");
	for(int i=0;i<=3000;i++)
	{
		const char *name = ActivityList_NameForIndex(i);	
		if(name)
		{
			//outputFile << "ADD_ACTIVITY_TO_SR( " << name << " ); " << i;
			//outputFile << "ADD_ACTIVITY_TO_SR( " << name << " );";
			//outputFile << name << ",";
			outputFile << "\n";
		}
	}
	outputFile.close();

#endif
}

void monster_dump_CommandCallback(const CCommand &command)
{
	GetEntityManager()->PrintDump();
}

#define GET_CONVAR(name) \
	name = g_pCVar->FindVar(#name); \
	if(name == NULL) { \
		META_CONPRINTF("[%s] %s - FAIL\n",g_Monster.GetLogTag(), #name); \
		return false; \
	}

void e1_CommandCallback(const CCommand &command)
{
	engine->ServerCommand("exec 1\n");

}

void e2_CommandCallback(const CCommand &command)
{
	engine->ServerCommand("e5\n");
}

void e3_CommandCallback(const CCommand &command)
{
	//engine->ServerCommand("exec 2\n");
	Assert(0);
}

bool CommandInitialize()
{
#ifdef _DEBUG
	new ConCommand("e1",e1_CommandCallback, "", 0);
	new ConCommand("e2",e2_CommandCallback, "", 0);
	new ConCommand("e3",e3_CommandCallback, "", 0);

	new ConCommand("e5",cmd1_CommandCallback, "", FCVAR_GAMEDLL);
	new ConCommand("e6",cmd2_CommandCallback, "", 0);
	new ConCommand("pp",monster_dump_CommandCallback, "", 0);
#endif
	new ConCommand("monster_dump",monster_dump_CommandCallback, "", 0);
	new ConVar("zx2_monster_build",__DATE__" "__TIME__,FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY);

	new ConCommand("npc_create", CC_NPC_Create, "Creates an NPC of the given type where the player is looking (if the given NPC can actually stand at that location).  Note that this only works for npc classes that are already in the world.  You can not create an entity that doesn't have an instance in the level.\n\tArguments:	{npc_class_name}", FCVAR_CHEAT|FCVAR_GAMEDLL);

	GET_CONVAR(sv_gravity);
	GET_CONVAR(phys_pushscale);
	GET_CONVAR(npc_height_adjust);

	GET_CONVAR(ai_path_adjust_speed_on_immediate_turns);
	GET_CONVAR(ai_path_insert_pause_at_obstruction);
	GET_CONVAR(ai_path_insert_pause_at_est_end);
	GET_CONVAR(ai_use_clipped_paths);
	GET_CONVAR(ai_moveprobe_usetracelist);
	GET_CONVAR(scene_flatturn);

	GET_CONVAR(violence_hblood);
	GET_CONVAR(violence_ablood);
	GET_CONVAR(violence_hgibs);
	GET_CONVAR(violence_agibs);

	GET_CONVAR(sv_suppress_viewpunch);
	GET_CONVAR(ai_use_visibility_cache);
	GET_CONVAR(ai_strong_optimizations);
	GET_CONVAR(ai_navigator_generate_spikes);
	GET_CONVAR(ai_navigator_generate_spikes_strength);
	GET_CONVAR(ai_no_node_cache);

	GET_CONVAR(sv_stepsize);
	GET_CONVAR(hl2_episodic);
	GET_CONVAR(ai_follow_use_points);
	GET_CONVAR(ai_LOS_mode);
	GET_CONVAR(ai_follow_use_points_when_moving);

	GET_CONVAR(ammo_hegrenade_max);
	
	GET_CONVAR(sv_strict_notarget);

	GET_CONVAR(ai_shot_bias_min);
	GET_CONVAR(ai_shot_bias_max);
	GET_CONVAR(ai_shot_bias);
	GET_CONVAR(ai_spread_pattern_focus_time);

	GET_CONVAR(ai_lead_time);

	GET_CONVAR(scene_clamplookat);
	GET_CONVAR(scene_showfaceto);
	GET_CONVAR(flex_maxawaytime);
	GET_CONVAR(flex_minawaytime);
	GET_CONVAR(flex_maxplayertime);
	GET_CONVAR(flex_minplayertime);

	GET_CONVAR(ai_find_lateral_los);
	GET_CONVAR(npc_sentences);

	GET_CONVAR(ai_find_lateral_cover);

	GET_CONVAR(rr_debugresponses);

	GET_CONVAR(sk_ally_regen_time);
	GET_CONVAR(sv_npc_talker_maxdist);
	GET_CONVAR(ai_no_talk_delay);
	GET_CONVAR(rr_debug_qa);
	GET_CONVAR(npc_ally_deathmessage);

	GET_CONVAR(ai_enable_fear_behavior);
	GET_CONVAR(ai_fear_player_dist);
	GET_CONVAR(g_debug_transitions);
	GET_CONVAR(ai_no_local_paths);

	GET_CONVAR(ai_use_think_optimizations);
	GET_CONVAR(ai_use_efficiency);
	GET_CONVAR(ai_efficiency_override);
	GET_CONVAR(ai_frametime_limit);
	GET_CONVAR(ai_default_efficient);
	GET_CONVAR(ai_shot_stats);
	GET_CONVAR(ai_shot_stats_term);
	GET_CONVAR(ai_spread_cone_focus_time);
	GET_CONVAR(ai_spread_defocused_cone_multiplier);

	GET_CONVAR(mat_dxlevel);
	GET_CONVAR(npc_vphysics);

	GET_CONVAR(ai_no_steer);
	GET_CONVAR(ai_debug_directnavprobe);

	GET_CONVAR(r_JeepViewZHeight);
	GET_CONVAR(r_JeepViewDampenFreq);
	GET_CONVAR(r_JeepViewDampenDamp);
	GET_CONVAR(r_VehicleViewDampen);
	GET_CONVAR(r_vehicleBrakeRate);

	GET_CONVAR(xbox_throttlebias);
	GET_CONVAR(xbox_throttlespoof);
	GET_CONVAR(xbox_autothrottle);
	GET_CONVAR(xbox_steering_deadzone);

	GET_CONVAR(g_debug_vehicledriver);
	GET_CONVAR(g_debug_npc_vehicle_roles);
	GET_CONVAR(g_jeepexitspeed);
	GET_CONVAR(hud_jeephint_numentries);

	GET_CONVAR(sv_client_predict);

	return true;
}
