#pragma semicolon 1

#include <sourcemod>
#include <sdktools>
#include <cstrike>
#include <mapchooser>
#include <setname>

#define YELLOW 0x01
#define TEAMCOLOR 0x03
#define GREEN 0x04
	
public Plugin:myinfo =
{
	name = "Monster",
	author = "",
	description = "Monster",
	version = "1.0.0.0",
	url = ""
};


new Handle:Respawn_Timer[MAXPLAYERS+1];
new g_player_last_amin_ent[MAXPLAYERS+1];
new Float:g_player_last_amin_time[MAXPLAYERS+1];
new Handle:Health_Display_Timer = INVALID_HANDLE;
new time_count;
new UserMsg:radiotext;
new UserMsg:sendaudio;
new UserMsg:saytext2;
new m_OffsetCollisionGroup;
new Handle:mp_playercollide_Handle;
new Handle:hGiveNamedItem;
new g_bot_client;
new bool:g_show_deathevent = false;
new m_iMaxHealth = -1;
new m_iHealth = -1;
new bool:g_block_namechange = false;
new bool:g_allow_votemap = true;

public OnPluginStart( )
{
	HookEvent("player_death",Event_PlayerDeath, EventHookMode_Pre);
	HookEvent("player_team",Event_PlayerTeam);
	HookEvent("player_spawn",Event_Player_Spawn);
	HookEvent("entity_killed",Event_Entity_Killed);
	HookEvent("round_start",Event_Round_Start);
	
	RegConsoleCmd("join", JoinTeam);
	
	RegServerCmd("monster_map_end", monster_map_end);
	
	mp_playercollide_Handle = CreateConVar("mp_playercollide","0","",0, true, 0.0, true, 1.0);
	
	m_OffsetCollisionGroup = FindSendPropOffs("CBasePlayer","m_CollisionGroup");

 
	radiotext = GetUserMessageId("RadioText");
	if(!(radiotext == INVALID_MESSAGE_ID))
	{
		HookUserMessage(radiotext, UserMsgRadioText, true);
	}
	
	sendaudio = GetUserMessageId("SendAudio");
	if(!(sendaudio == INVALID_MESSAGE_ID))
	{
		HookUserMessage(sendaudio, UserMsgSendAudio, true);
	}
	
	saytext2 = GetUserMessageId("SayText2");
	if(!(saytext2 == INVALID_MESSAGE_ID))
	{
		HookUserMessage(saytext2, UserMsgSayText2, true);
	}
	
	StartPrepSDKCall(SDKCall_Player);
	PrepSDKCall_SetVirtual(400);
	PrepSDKCall_SetReturnInfo(SDKType_CBaseEntity, SDKPass_Pointer);
	PrepSDKCall_AddParameter(SDKType_String, SDKPass_Pointer);
	PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Plain);
	hGiveNamedItem = EndPrepSDKCall();
}

public OnPluginEnd()
{

}


public OnMapStart()
{
	g_show_deathevent = false;
	
	if(Health_Display_Timer != INVALID_HANDLE)
		KillTimer(Health_Display_Timer);
	Health_Display_Timer = INVALID_HANDLE;

	time_count = 0;
	Health_Display_Timer = CreateTimer(0.1, HealthDisplay, 0, TIMER_REPEAT);
	
	CreateTimer(5.0, DelayMapStart, 0, TIMER_FLAG_NO_MAPCHANGE);
}

public Action:DelayMapStart(Handle:timer)
{
	if(TrySetNextMap() == false) {
		g_allow_votemap = true;
		ServerCommand("mp_chattime 40");
	} else {
		g_allow_votemap = false;
		ServerCommand("mp_chattime 5");
	}
	return Plugin_Handled;
}

public GiveItem(client, const String:item[])
{
	return SDKCall(hGiveNamedItem, client, item, 0);
}

public bool:ShouldUseThisAttacker(attacker)
{
	if(attacker == 0)
		return false;

	if(Entity_IsPlayer(attacker))
		return false;
	
	return true;
}

public Event_Entity_Killed(Handle:event, const String:name[], bool:dontBroadcast)
{
	decl entindex_killed, entindex_attacker, entindex_inflictor, victim, target_attacker, attacker;
	
	entindex_killed = GetEventInt(event,"entindex_killed");
	entindex_attacker = GetEventInt(event,"entindex_attacker");
	entindex_inflictor = GetEventInt(event,"entindex_inflictor");
	
	victim = entindex_killed;
	if(!Entity_IsPlayer(victim))
		return;
	
	target_attacker = 0;
	attacker = entindex_attacker;
	if(!ShouldUseThisAttacker(attacker))
	{
		attacker = entindex_inflictor;
	}
	
	if(attacker != 0 && attacker != victim)
	{
		new String:classname[128];
		if(GetEdictClassname(attacker, classname, sizeof(classname)))
		{
			CS_SetClientName_Private(g_bot_client, classname, true);
			target_attacker = g_bot_client;
		}
	}
	
	new Handle:hndl = CreateDataPack();
	WritePackCell(hndl, target_attacker);
	WritePackCell(hndl, victim);
	CreateTimer(0.1, DeathEventTimer, hndl);
}

public Action:DeathEventTimer(Handle:timer, Handle:hndl)
{
	ResetPack(hndl);
	new attacker = ReadPackCell(hndl);
	new victim = ReadPackCell(hndl);
	CloseHandle(hndl);
	
	if(!Entity_IsPlayer(victim) || !IsClientInGame(victim))
		return Plugin_Stop;
	
	if(attacker > 0)
	{
		if(!Entity_IsPlayer(attacker) || !IsClientInGame(attacker))
			return Plugin_Stop;
	}
	
	g_show_deathevent = true;
	RunDeathEvent(attacker, victim, "NPC", false);
	g_show_deathevent = false;
	
	return Plugin_Stop;
}

public Event_Round_Start(Handle:event, const String:name[], bool:dontBroadcast)
{
	SetupBot();
}

public Event_Player_Spawn(Handle:event, const String:name[], bool:dontBroadcast)
{
	decl client;
	client = GetClientOfUserId(GetEventInt(event,"userid"));
	if(GetConVarBool(mp_playercollide_Handle) == false)
	{
		if (IsClientInGame(client) && GetClientTeam(client) >= 2)
		{
			SetEntData(client,m_OffsetCollisionGroup,3);
		}
	}
}

public Action:UserMsgSayText2(UserMsg:msg_id, Handle:bf, const players[], playersNum, bool:reliable, bool:init)
{
	if(g_block_namechange)
	{
		decl String:msg_str[256];
		BfReadByte(bf);
		BfReadByte(bf);
		
		BfReadString(bf, msg_str, sizeof(msg_str));
		if(StrEqual(msg_str, "#Cstrike_Name_Change"))
		{
			g_block_namechange = false;
			return Plugin_Handled;
		}
	}
	return Plugin_Continue;
}

public Action:UserMsgSendAudio(UserMsg:msg_id, Handle:bf, const players[], playersNum, bool:reliable, bool:init)
{
	decl String:msg_str[256];
	BfReadString(bf, msg_str, sizeof(msg_str));
	if(!strcmp(msg_str, "Radio.FireInTheHole", false))
		return Plugin_Handled;
	return Plugin_Continue;
}

public Action:UserMsgRadioText(UserMsg:msg_id, Handle:bf, const players[], playersNum, bool:reliable, bool:init)
{
	decl String:radio_text[256];
	BfReadWord(bf);
	BfReadString(bf, radio_text, sizeof(radio_text));
	if(!strcmp(radio_text, "#Game_radio_location", false))
		BfReadString(bf, radio_text, sizeof(radio_text));
	BfReadString(bf, radio_text, sizeof(radio_text));
	BfReadString(bf, radio_text, sizeof(radio_text));
	if(!strcmp(radio_text, "#Cstrike_TitlesTXT_Fire_in_the_hole", false))
		return Plugin_Handled;
	return Plugin_Continue;
}

public Action:monster_map_end(args)
{
	if(g_allow_votemap)
	{
		InitiateMapChooserVote(MapChange_Instant);
	}
	return Plugin_Handled;
}

public Action:JoinTeam(client, args)
{
	if(GetClientTeam(client) < 2)
	{
		ChangeClientTeam(client,3);
	}
	return Plugin_Handled;
}

public OnMapEnd()
{
	if(Health_Display_Timer != INVALID_HANDLE)
		KillTimer(Health_Display_Timer);
	Health_Display_Timer = INVALID_HANDLE;
}

public Action:HealthDisplay(Handle:timer)
{
	decl health, armor, aim_ent, ent_max_health, ent_health;
	decl String:classname[65];
	decl String:npc_name[65];
	decl bool:show_npc;
	decl Float:game_time;
	
	game_time = GetGameTime();
	for (new i=1; i<=MaxClients; i++)
	{
		if (IsClientInGame(i) && !IsFakeClient(i))
		{
			health = 0;
			armor = 0;
			show_npc = false;
			if(IsPlayerAlive(i))
			{
				health = GetClientHealth(i);
				if((time_count % 10) == 0)
				{
					if(health < 100)
					{
						health = health+1;
						SetEntityHealth(i, health);
					}
				}
				armor = GetClientArmor(i);
			}
			
			aim_ent = GetClientAimTarget(i, false);
			if(Entity_IsPlayer(aim_ent))
				aim_ent = 0;
			
			if(aim_ent <= 0)
			{
				if(g_player_last_amin_time[i] >= game_time) {
					aim_ent = g_player_last_amin_ent[i];
				}
			} else {
				g_player_last_amin_time[i] = game_time + 3.0;
			}
			
			if(IsValidEntity(aim_ent))
			{
				g_player_last_amin_ent[i] = aim_ent;
				classname[0] = 0;
				npc_name[0] = 0;
				GetEdictClassname(aim_ent, classname, sizeof(classname));
				if(strlen(classname) > 4 && classname[0] == 'n' && classname[1] == 'p' && classname[2] == 'c' && classname[3] == '_') {
					strcopy(npc_name, sizeof(npc_name),classname[4]);
					npc_name[0] = CharToUpper(npc_name[0]);
					
					if(m_iMaxHealth == -1) {
						m_iMaxHealth = FindDataMapOffs(aim_ent, "m_iMaxHealth");
					}
					if(m_iHealth == -1) {
						m_iHealth = FindDataMapOffs(aim_ent, "m_iHealth");
					}
					ent_max_health = GetEntData(aim_ent,m_iMaxHealth);
					ent_health = GetEntData(aim_ent,m_iHealth);
					
					if(ent_health < 0)
						ent_health = 0;
					show_npc = true;
				}
			}
			
			
			if(show_npc) {
				PrintKeyHintText(i, "==============\nHP: %d\nAP: %d\n==============\n%s (%.0f/100)",health, armor, npc_name,(float(ent_health)/float(ent_max_health))*100);
			} else {
				PrintKeyHintText(i,"==============\nHP: %d\nAP: %d\n==============",health, armor);
			}
		}
	}
	time_count++;
	return Plugin_Continue;
}

public OnClientPutInServer(client)
{
	g_player_last_amin_ent[client] = 0;
	g_player_last_amin_time[client] = 0.0;
	ResetRespawnTimer(client);
}

public OnClientDisconnect(client)
{
	ResetRespawnTimer(client);
}

public Action:Event_PlayerDeath(Handle:event, const String:name[], bool:dontBroadcast)
{
	decl victim;
	victim = GetClientOfUserId(GetEventInt(event, "userid"));
	if(!IsFakeClient(victim))
	{
		ResetRespawnTimer(victim);
		Respawn_Timer[victim] = CreateTimer(1.5, ClientRespawn, victim);
	}
	
	if(g_show_deathevent)
		return Plugin_Continue;
	else {
		SetEventBroadcast(event, true);
		return Plugin_Continue;
	}
}

public Action:ClientRespawn(Handle:timer, any:data)
{
	if(IsClientInGame(data) && !IsFakeClient(data) && !IsPlayerAlive(data) && GetClientTeam(data) >= 2)
	{
		CS_RespawnPlayer(data);
	}
	Respawn_Timer[data] = INVALID_HANDLE;
	return Plugin_Handled;
}

public Action:Event_PlayerTeam(Handle:event, const String:name[], bool:dontBroadcast)
{
	decl client;
	decl team;
	client = GetClientOfUserId(GetEventInt(event, "userid"));
	team = GetEventInt(event, "team");
	if (team >= 2)
	{
		ResetRespawnTimer(client);
		CreateTimer(1.0, ClientRespawn, client);
	}
	return Plugin_Continue;
}

public SetupBot()
{
	new maxclients = MaxClients;
	new count = 0;
	
	for(new i=1;i<=maxclients;i++)
	{
		if(!IsClientConnected(i) || !IsClientInGame(i))
			continue;

		if(IsFakeClient(i))
		{
			if(IsPlayerAlive(i))
			{
				ForcePlayerSuicide(i);
			}
			g_bot_client = i;
			count++;
		}
	}
	
	if(count == 0)
	{
		new client = CreateFakeClient("Cooperative");
		if(client)
		{
			g_bot_client = client;
			CS_SwitchTeam(client, 2);
			if(IsPlayerAlive(client))
			{
				ForcePlayerSuicide(client);
			}
		}
	}
}

stock ResetRespawnTimer(client)
{
	if(Respawn_Timer[client] != INVALID_HANDLE)
		KillTimer(Respawn_Timer[client]);
	Respawn_Timer[client] = INVALID_HANDLE;
}

stock PrintKeyHintText(client, const String:format[], any:...)
{
	decl String:buffer[192];
	VFormat(buffer, sizeof(buffer), format, 3);
	new Handle:hBuffer = StartMessageOne("KeyHintText", client); 
	BfWriteByte(hBuffer, 1); 
	BfWriteString(hBuffer, buffer); 
	EndMessage();
}

stock SendMsg_SayText2(target, color, const String:szMsg[], any:...)
{
	/*if (strlen(szMsg) > 191){
	LogError("Disallow string len(%d) > 191", strlen(szMsg));
	return;
	}*/
	decl String:buffer[192];
	VFormat(buffer, 192, szMsg, 4);
	decl Handle:hBf;
	if (!target)
	{
		hBf = StartMessageAll("SayText2");
	} else {
		hBf = StartMessageOne("SayText2", target);
	}
	if (hBf != INVALID_HANDLE)
	{
		BfWriteByte(hBf, color); // Players index, to send a global message from the server make it 0
		BfWriteByte(hBf, 0); // 0 to phrase for colour 1 to ignore it
		BfWriteString(hBf, buffer); // the message itself
		CloseHandle(hBf);
		EndMessage();
	}
}

stock RunDeathEvent(attacker,victim, String:weapon[], bool:headshot)
{
	new Handle:hndl = CreateEvent("player_death");
	if(hndl != INVALID_HANDLE)
	{
		SetEventInt(hndl, "userid", GetClientUserId(victim));
		SetEventInt(hndl, "attacker", (attacker)?GetClientUserId(attacker):0);
		SetEventBool(hndl, "headshot",headshot);
		SetEventString(hndl, "weapon",weapon);
		FireEvent(hndl, false);
	}
}

stock CS_SetClientName_Private(client, const String:name[], bool:silent=false) {
	if(silent)
	{
		g_block_namechange = true;
	}
	CS_SetClientName(client, name);

	/*decl String:oldname[MAX_NAME_LENGTH];
	GetClientName(client, oldname, sizeof(oldname));
	SetClientInfo(client, "name", name);
	SetEntPropString(client, Prop_Data, "m_szNetname", name);
	new Handle:event = CreateEvent("player_changename");
	if (event != INVALID_HANDLE)
	{
		SetEventInt(event, "userid", GetClientUserId(client));
		SetEventString(event, "oldname", oldname);
		SetEventString(event, "newname", name);
		FireEvent(event, true);
	}
	if(silent)
		return;
	new Handle:msg = StartMessageAll("SayText2");
	if(msg != INVALID_HANDLE)
	{
		BfWriteByte(msg, client);
		BfWriteByte(msg, true);
		BfWriteString(msg, "Cstrike_Name_Change");
		BfWriteString(msg, oldname);
		BfWriteString(msg, name);
		EndMessage();
	}*/
}

stock bool:Entity_IsPlayer(entity)
{
	if (entity < 1 || entity > MaxClients) {
		return false;
	}
	
	return true;
}

public bool:TrySetNextMap()
{
	new String:currentmap[65];
	GetCurrentMap(currentmap, sizeof(currentmap));
	
	if(StrEqual(currentmap,"syn_lvcoop_part1")) {
		SetNextMap("syn_lvcoop_part2");
		return true;
	}
	
	return false;
}
