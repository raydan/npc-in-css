//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "CEntity.h"
#include "CFlex.h"
#include "sceneentity.h"
#include "CAI_playerally.h"
#include "saverestore_utlmap.h"
#include "eventqueue.h"
#include "CAI_behavior_lead.h"
#include "CPlayer.h"
#include "fmtstr.h"
#include "CAI_speech.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//extern CServerGameDLL g_ServerGameDLL;

extern ConVar *rr_debugresponses;

//-----------------------------------------------------------------------------

extern ConVar *sk_ally_regen_time;
extern ConVar *sv_npc_talker_maxdist;
extern ConVar *ai_no_talk_delay;
extern ConVar *rr_debug_qa;
extern ConVar *npc_ally_deathmessage;

//-----------------------------------------------------------------------------

ConceptCategoryInfo_t g_ConceptCategoryInfos[] =
{
	{	10,	20,	0, 0 },
	{	0,	0,	0, 0 },
	{	0,	0,	0, 0 },
};

ConceptInfo_t g_ConceptInfos[] =
{
	{ 	TLK_ANSWER,			SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT | AICF_ANSWER,	},
	{ 	TLK_ANSWER_HELLO,	SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT | AICF_ANSWER,	},
	{ 	TLK_QUESTION,		SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT | AICF_QUESTION,	},
	{ 	TLK_IDLE,			SPEECH_IDLE, 		-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT | AICF_TARGET_PLAYER, },
	{ 	TLK_STARE,			SPEECH_IDLE, 		-1,		-1,		-1,		-1,		180,	 0,		AICF_DEFAULT | AICF_TARGET_PLAYER, },
	{ 	TLK_LOOK,			SPEECH_PRIORITY, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT | AICF_TARGET_PLAYER, },
	{ 	TLK_HELLO,			SPEECH_IDLE, 		5,		10,		-1,		-1,		-1,		-1,		AICF_DEFAULT | AICF_SPEAK_ONCE | AICF_PROPAGATE_SPOKEN | AICF_TARGET_PLAYER,	},
	{ 	TLK_PHELLO,			SPEECH_IDLE, 		-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT | AICF_SPEAK_ONCE | AICF_PROPAGATE_SPOKEN | AICF_TARGET_PLAYER,	},
	{ 	TLK_HELLO_NPC,		SPEECH_IDLE, 		5,		10,		-1,		-1,		-1,		-1,		AICF_DEFAULT | AICF_QUESTION | AICF_SPEAK_ONCE,	},
	{ 	TLK_PIDLE,			SPEECH_IDLE, 		-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_PQUESTION,		SPEECH_IDLE, 		-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_SMELL,			SPEECH_IDLE, 		-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{	TLK_USE,			SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_STARTFOLLOW,	SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_STOPFOLLOW,		SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_JOINPLAYER,		SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},	
	{ 	TLK_STOP,			SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_NOSHOOT,		SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_PLHURT1,		SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_PLHURT2,		SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_PLHURT3,		SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_PLHURT,			SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_PLPUSH,			SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		15,		30,		AICF_DEFAULT,	},
	{ 	TLK_PLRELOAD,		SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,	    60,		 0,		AICF_DEFAULT,	},
	{ 	TLK_SHOT,			SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_WOUND,			SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_MORTAL,			SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT | AICF_SPEAK_ONCE,	},
	{ 	TLK_SEE_COMBINE,	SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_ENEMY_DEAD,		SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_ALYX_ENEMY_DEAD,SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_SELECTED,		SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_COMMANDED,		SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_COMMAND_FAILED,	SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_DENY_COMMAND,	SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_BETRAYED,		SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_ALLY_KILLED,	SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		15,		30,		AICF_DEFAULT,	},
	{ 	TLK_ATTACKING,		SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_HEAL,			SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_GIVEAMMO,		SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{	TLK_HELP_ME,		SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		 7,		10,		AICF_DEFAULT,	},
	{	TLK_PLYR_PHYSATK,	SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{	TLK_NEWWEAPON,		SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{	TLK_STARTCOMBAT,	SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		30,		 0,		AICF_DEFAULT,	},
	{	TLK_WATCHOUT,		SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		30,		45,		AICF_DEFAULT,	},
	{	TLK_MOBBED,			SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		10,		12,		AICF_DEFAULT,	},
	{	TLK_MANY_ENEMIES,	SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		45,		60,		AICF_DEFAULT,	},
	{ 	TLK_DANGER,			SPEECH_PRIORITY,	-1,		-1,		-1,		-1,		5,		7,		AICF_DEFAULT,	},
	{ 	TLK_PLDEAD,			SPEECH_PRIORITY,	-1,		-1,		-1,		-1,		100,	 0,		AICF_DEFAULT,	},
	{ 	TLK_HIDEANDRELOAD,	SPEECH_PRIORITY,	-1,		-1,		-1,		-1,		 45,	60,		AICF_DEFAULT,	},
	{ 	TLK_FLASHLIGHT_ILLUM,	SPEECH_PRIORITY,-1,		-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_FLASHLIGHT_ON,	SPEECH_PRIORITY,	-1,		-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_FLASHLIGHT_OFF,	SPEECH_PRIORITY,	-1,		-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_FOUNDPLAYER,	SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		 -1,	-1,		AICF_TARGET_PLAYER,	},
	{ 	TLK_PLAYER_KILLED_NPC,				SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_ENEMY_BURNING,					SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_SPOTTED_ZOMBIE_WAKEUP,			SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_SPOTTED_HEADCRAB_LEAVING_ZOMBIE,SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_DANGER_ZOMBINE_GRENADE,			SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
	{ 	TLK_BALLSOCKETED,					SPEECH_IMPORTANT, 	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},
		
	// Darkness mode
	{ 	TLK_DARKNESS_LOSTPLAYER, SPEECH_IMPORTANT,-1,	-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_DARKNESS_FOUNDPLAYER, SPEECH_IMPORTANT,-1,	-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_DARKNESS_UNKNOWN_WOUND, SPEECH_IMPORTANT,-1,-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_DARKNESS_HEARDSOUND, SPEECH_IMPORTANT,-1,	-1,		-1,		-1,		 20,	30,		AICF_DEFAULT,	},
	{ 	TLK_DARKNESS_LOSTENEMY_BY_FLASHLIGHT, SPEECH_IMPORTANT,-1,	-1,	-1,	-1,	 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_DARKNESS_LOSTENEMY_BY_FLASHLIGHT_EXPIRED, SPEECH_IMPORTANT,-1,	-1,	-1,	-1,	 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_DARKNESS_FOUNDENEMY_BY_FLASHLIGHT, SPEECH_IMPORTANT,-1,	-1,	-1,	-1,	 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_DARKNESS_FLASHLIGHT_EXPIRED, SPEECH_IMPORTANT,	-1,	-1,	-1,	-1,	 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_SPOTTED_INCOMING_HEADCRAB, SPEECH_IMPORTANT,-1,		-1,		-1,	-1,	 -1,	-1,		AICF_DEFAULT,	},

	// Lead behaviour
	{ 	TLK_LEAD_START,		SPEECH_IMPORTANT,-1,	-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_LEAD_ARRIVAL,	SPEECH_IMPORTANT,-1,	-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_LEAD_SUCCESS,	SPEECH_IMPORTANT,-1,	-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_LEAD_FAILURE,	SPEECH_IMPORTANT,-1,	-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_LEAD_COMINGBACK,SPEECH_IMPORTANT,-1,	-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_LEAD_CATCHUP,	SPEECH_IMPORTANT,-1,	-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_LEAD_RETRIEVE,	SPEECH_IMPORTANT,-1,	-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_LEAD_ATTRACTPLAYER, SPEECH_IMPORTANT,-1,-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_LEAD_WAITOVER,	SPEECH_IMPORTANT,-1,	-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_LEAD_MISSINGWEAPON,	SPEECH_IMPORTANT,-1,-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},
	{ 	TLK_LEAD_IDLE,		SPEECH_IMPORTANT,-1,	-1,		-1,		-1,		 -1,	-1,		AICF_DEFAULT,	},

	// Passenger behaviour
	{ TLK_PASSENGER_NEW_RADAR_CONTACT,		SPEECH_IMPORTANT,	-1,		-1,		-1,		-1,		-1,		-1,		AICF_DEFAULT,	},	
};

//-----------------------------------------------------------------------------

bool ConceptStringLessFunc( const string_t &lhs, const string_t &rhs )	
{ 
	return CaselessStringLessThan( STRING(lhs), STRING(rhs) ); 
}

//-----------------------------------------------------------------------------

class CConceptInfoMap : public CUtlMap<AIConcept_t, ConceptInfo_t *> {
public:
	CConceptInfoMap() :
	  CUtlMap<AIConcept_t, ConceptInfo_t *>( CaselessStringLessThan )
	  {
		  for ( size_t i = 0; i < ARRAYSIZE(g_ConceptInfos); i++ )
		  {
			  Insert( g_ConceptInfos[i].concept, &g_ConceptInfos[i] );
		  }
	  }
};

static CConceptInfoMap g_ConceptInfoMap;

CAI_AllySpeechManager::CAI_AllySpeechManager()
{
	Assert( !gm_pSpeechManager );
	gm_pSpeechManager = this;
}

CAI_AllySpeechManager::~CAI_AllySpeechManager()
{
	gm_pSpeechManager = NULL;
}

void CAI_AllySpeechManager::PostConstructor()
{
	BaseClass::PostConstructor();
	m_ConceptTimers->SetLessFunc( ConceptStringLessFunc );
}

void CAI_AllySpeechManager::Spawn()
{
	BaseClass::Spawn();
	//Assert( g_ConceptInfoMap.Count() != 0 );
	//for ( size_t i = 0; i < ARRAYSIZE(g_ConceptInfos); i++ )
	//	m_ConceptTimers->Insert( AllocPooledString( g_ConceptInfos[i].concept ), CSimpleSimTimer() );
}

void CAI_AllySpeechManager::AddCustomConcept( const ConceptInfo_t &conceptInfo )
{
	Assert( g_ConceptInfoMap.Count() != 0 );
	Assert( m_ConceptTimers->Count() != 0 );

	if ( g_ConceptInfoMap.Find( conceptInfo.concept ) == g_ConceptInfoMap.InvalidIndex() )
	{
		g_ConceptInfoMap.Insert( conceptInfo.concept, new ConceptInfo_t( conceptInfo ) );
	}

	if ( m_ConceptTimers->Find( AllocPooledString(conceptInfo.concept) ) == m_ConceptTimers->InvalidIndex() )
	{
		m_ConceptTimers->Insert( AllocPooledString( conceptInfo.concept ), CSimpleSimTimer() );
	}
}

ConceptCategoryInfo_t *CAI_AllySpeechManager::GetConceptCategoryInfo( ConceptCategory_t category )
{
	return &g_ConceptCategoryInfos[category];
}

ConceptInfo_t *CAI_AllySpeechManager::GetConceptInfo( AIConcept_t concept )
{
	int iResult = g_ConceptInfoMap.Find( concept );

	return ( iResult != g_ConceptInfoMap.InvalidIndex() ) ? g_ConceptInfoMap[iResult] : NULL;
}

void CAI_AllySpeechManager::OnSpokeConcept( CAI_PlayerAlly *pPlayerAlly, AIConcept_t concept, AI_Response *response )
{
	ConceptInfo_t *			pConceptInfo	= GetConceptInfo( concept );
	ConceptCategory_t		category		= ( pConceptInfo ) ? pConceptInfo->category : SPEECH_IDLE;
	ConceptCategoryInfo_t *	pCategoryInfo	= GetConceptCategoryInfo( category );

	if ( pConceptInfo )
	{
		if ( pConceptInfo->flags & AICF_PROPAGATE_SPOKEN ) 
		{
			CAI_NPC **ppAIs = g_AI_Manager.AccessAIs();
			CAI_PlayerAlly *pTalker;
			for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
			{
				pTalker = dynamic_cast<CAI_PlayerAlly *>(ppAIs[i]);

				if ( pTalker && pTalker != pPlayerAlly && 
					 (pTalker->GetAbsOrigin() - pPlayerAlly->GetAbsOrigin()).LengthSqr() < Square(TALKRANGE_MIN * 2) && 
					 pPlayerAlly->FVisible_Entity( pTalker->BaseEntity() ) )
				{
					// Tell this guy he's already said the concept to the player, too.
					pTalker->GetExpresser()->SetSpokeConcept( concept, NULL, false );
				}
			}
		}
	}

	if ( !ai_no_talk_delay->GetBool() )
	{
		if ( pConceptInfo && pConceptInfo->minGlobalCategoryDelay != -1 )
		{
			Assert( pConceptInfo->maxGlobalCategoryDelay != -1 );
			SetCategoryDelay( pConceptInfo->category, pConceptInfo->minGlobalCategoryDelay, pConceptInfo->maxGlobalCategoryDelay );
		}
		else if ( pCategoryInfo->maxGlobalDelay > 0 )
		{
			SetCategoryDelay( category, pCategoryInfo->minGlobalDelay, pCategoryInfo->maxGlobalDelay );
		}

		if ( pConceptInfo && pConceptInfo->minPersonalCategoryDelay != -1 )
		{
			Assert( pConceptInfo->maxPersonalCategoryDelay != -1 );
			pPlayerAlly->SetCategoryDelay( pConceptInfo->category, pConceptInfo->minPersonalCategoryDelay, pConceptInfo->maxPersonalCategoryDelay );
		}
		else if ( pCategoryInfo->maxPersonalDelay > 0 )
		{
			pPlayerAlly->SetCategoryDelay( category, pCategoryInfo->minPersonalDelay, pCategoryInfo->maxPersonalDelay );
		}

		if ( pConceptInfo && pConceptInfo->minConceptDelay != -1 )
		{
			Assert( pConceptInfo->maxConceptDelay != -1 );
			char iConceptTimer = m_ConceptTimers->Find( MAKE_STRING(concept) );
			if ( iConceptTimer != m_ConceptTimers->InvalidIndex() )
				m_ConceptTimers->Element(iConceptTimer).Set( pConceptInfo->minConceptDelay, pConceptInfo->minConceptDelay );
		}
	}
}

void CAI_AllySpeechManager::SetCategoryDelay( ConceptCategory_t category, float minDelay, float maxDelay )	
{ 
	if ( category != SPEECH_PRIORITY )
		Get_m_ConceptCategoryTimers(category)->Set( minDelay, maxDelay ); 
}

bool CAI_AllySpeechManager::CategoryDelayExpired( ConceptCategory_t category )										
{ 
	if ( category == SPEECH_PRIORITY )
		return true;
	return Get_m_ConceptCategoryTimers(category)->Expired(); 
}

bool CAI_AllySpeechManager::ConceptDelayExpired( AIConcept_t concept )
{
	char iConceptTimer = m_ConceptTimers->Find( MAKE_STRING(concept) );
	if ( iConceptTimer != m_ConceptTimers->InvalidIndex() )
		return m_ConceptTimers->Element(iConceptTimer).Expired();
	return true;
}

CSimpleSimTimer *CAI_AllySpeechManager::Get_m_ConceptCategoryTimers(int index)
{
	CSimpleSimTimer *timer = (CSimpleSimTimer *)(((uint8_t *)(BaseEntity())) + m_ConceptCategoryTimers.offset + (sizeof(CSimpleSimTimer)*index));
	return timer;
}

//-----------------------------------------------------------------------------

CE_LINK_ENTITY_TO_CLASS( ai_ally_speech_manager, CAI_AllySpeechManager );

DEFINE_PROP(m_ConceptCategoryTimers, CAI_AllySpeechManager);
DEFINE_PROP(m_ConceptTimers, CAI_AllySpeechManager);


//-----------------------------------------------------------------------------

CAI_AllySpeechManager *CAI_AllySpeechManager::gm_pSpeechManager = NULL;

//-----------------------------------------------------------------------------

CAI_AllySpeechManager *GetAllySpeechManager()
{
	if ( !CAI_AllySpeechManager::gm_pSpeechManager )
	{
		CreateEntityByName( "ai_ally_speech_manager" );
		Assert( CAI_AllySpeechManager::gm_pSpeechManager );
		if ( CAI_AllySpeechManager::gm_pSpeechManager )
			DispatchSpawn( CAI_AllySpeechManager::gm_pSpeechManager->BaseEntity() );
	}

	return CAI_AllySpeechManager::gm_pSpeechManager;
}

//-----------------------------------------------------------------------------
//
// CLASS: CAI_PlayerAlly
//
//-----------------------------------------------------------------------------

BEGIN_DATADESC( CAI_PlayerAlly )

	DEFINE_EMBEDDED( m_PendingResponse ),
	DEFINE_STDSTRING( m_PendingConcept ),
	DEFINE_FIELD( m_TimePendingSet, FIELD_TIME ),
	DEFINE_FIELD( m_hTalkTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flNextRegenTime, FIELD_TIME ),
	DEFINE_FIELD( m_flTimePlayerStartStare, FIELD_TIME ),
	DEFINE_FIELD( m_hPotentialSpeechTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flNextIdleSpeechTime, FIELD_TIME ),
	DEFINE_FIELD( m_iQARandomNumber, FIELD_INTEGER ),
	DEFINE_FIELD( m_hSpeechFilter, FIELD_EHANDLE ),
	//DEFINE_EMBEDDED_AUTO_ARRAY(m_ConceptCategoryTimers),

	DEFINE_KEYFIELD( m_bGameEndAlly, FIELD_BOOLEAN, "GameEndAlly" ),
	DEFINE_FIELD( m_bCanSpeakWhileScripting, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_flHealthAccumulator, FIELD_FLOAT ),
	DEFINE_FIELD( m_flTimeLastRegen, FIELD_TIME ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "IdleRespond", InputIdleRespond ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"SpeakResponseConcept",	InputSpeakResponseConcept ),
	DEFINE_INPUTFUNC( FIELD_VOID, "MakeGameEndAlly", InputMakeGameEndAlly ),
	DEFINE_INPUTFUNC( FIELD_VOID, "MakeRegularAlly", InputMakeRegularAlly ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AnswerQuestion", InputAnswerQuestion ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AnswerQuestionHello", InputAnswerQuestionHello ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableSpeakWhileScripting", InputEnableSpeakWhileScripting ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableSpeakWhileScripting", InputDisableSpeakWhileScripting ),

END_DATADESC()

//CBaseEntity *CreatePlayerLoadSave( Vector vOrigin, float flDuration, float flHoldTime, float flLoadTime );


void CAI_PlayerAlly::InputMakeGameEndAlly( inputdata_t &inputdata )
{
	m_bGameEndAlly = true;
}

void CAI_PlayerAlly::InputMakeRegularAlly( inputdata_t &inputdata )
{
	m_bGameEndAlly = false;
}

void CAI_PlayerAlly::DisplayDeathMessage( void )
{
	//CE_REMOVER
	/*if ( m_bGameEndAlly == false )
		return;

	if ( npc_ally_deathmessage.GetBool() == 0 )
		return;

	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
			continue;
	
		UTIL_ShowMessage( GetDeathMessageText(), pPlayer );
		pPlayer->NotifySinglePlayerGameEnding();
	}

	CBaseEntity *pReload = CreatePlayerLoadSave( GetAbsOrigin(), 1.5f, 8.0f, 4.5f );

	if ( pReload )
	{
		pReload->SetRenderColor( 0, 0, 0, 255 );

		g_EventQueue.AddEvent( pReload, "Reload", 1.5f, pReload, pReload );
	}

	// clear any pending autosavedangerous
	g_ServerGameDLL.m_fAutoSaveDangerousTime = 0.0f;
	g_ServerGameDLL.m_fAutoSaveDangerousMinHealthToCommit = 0.0f;
*/
}

//-----------------------------------------------------------------------------
// NPCs derived from CAI_PlayerAlly should call this in precache()
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::TalkInit( void )
{
}	

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::GatherConditions( void )
{
	BaseClass::GatherConditions();
	
	if ( !HasCondition( COND_SEE_PLAYER ) )
	{
		SetCondition( COND_TALKER_CLIENTUNSEEN );
	}

	CPlayer *pLocalPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());

	if ( pLocalPlayer && !pLocalPlayer->IsAlive() )
	{
		SetCondition( COND_TALKER_PLAYER_DEAD );
	}
	
	if ( HasCondition( COND_SEE_PLAYER ) )
	{
				
		bool bPlayerIsLooking = false;
		if ( pLocalPlayer && ( pLocalPlayer->GetAbsOrigin() - GetAbsOrigin() ).Length2DSqr() < Square(TALKER_STARE_DIST) )
		{
			if ( pLocalPlayer->FInViewCone_Vector( EyePosition() ) )
			{
				if ( pLocalPlayer->GetSmoothedVelocity().LengthSqr() < Square( 100 ) )
					bPlayerIsLooking = true;
			}
		}
		
		if ( bPlayerIsLooking )
		{
			SetCondition( COND_TALKER_PLAYER_STARING );
			if ( m_flTimePlayerStartStare == 0 )
				m_flTimePlayerStartStare = gpGlobals->curtime;
		}
		else
		{
			m_flTimePlayerStartStare = 0;
			ClearCondition( COND_TALKER_PLAYER_STARING );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	BaseClass::GatherEnemyConditions( pEnemy );
	if ( GetLastEnemyTime() == 0 || gpGlobals->curtime - GetLastEnemyTime() > 30 )
	{
#ifdef HL2_DLL
		CEntity *cent_pEnemy = CEntity::Instance(pEnemy);
		if ( HasCondition( COND_SEE_ENEMY ) && ( cent_pEnemy->Classify() != CLASS_BULLSEYE ) )
		{
			if( Classify() == CLASS_PLAYER_ALLY_VITAL && hl2_episodic->GetBool() )
			{
				CPlayer *pPlayer = UTIL_GetNearestVisiblePlayer(this);
				if( pPlayer )
				{				
					// If I can see the player, and the player would see this enemy if he turned around...
					if( !pPlayer->FInViewCone_Entity(pEnemy) && FVisible_Entity( pPlayer->BaseEntity() ) && pPlayer->FVisible_Entity(pEnemy) )
					{
						Vector2D vecPlayerView = pPlayer->EyeDirection2D().AsVector2D();
						Vector2D vecToEnemy = ( cent_pEnemy->GetAbsOrigin() - pPlayer->GetAbsOrigin() ).AsVector2D();
						Vector2DNormalize( vecToEnemy );
						
						if( DotProduct2D(vecPlayerView, vecToEnemy) <= -0.75 )
						{
							SpeakIfAllowed( TLK_WATCHOUT, "dangerloc:behind" );
							return;
						}
					}
				}
			}
			SpeakIfAllowed( TLK_STARTCOMBAT );
		}
#else
		if ( HasCondition( COND_SEE_ENEMY ) )
		{
			SpeakIfAllowed( TLK_STARTCOMBAT );
		}
#endif //HL2_DLL
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	BaseClass::OnStateChange( OldState, NewState );

	if ( OldState == NPC_STATE_COMBAT )
	{
		DeferAllIdleSpeech();
	}
 
	if ( GetState() == NPC_STATE_IDLE || GetState() == NPC_STATE_ALERT )
	{
		m_flNextIdleSpeechTime = gpGlobals->curtime + RandomFloat(5,10);
	}
	else
	{
		m_flNextIdleSpeechTime = 0;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

#ifdef HL2_DLL
	// Vital allies regenerate
	if( GetHealth() >= GetMaxHealth() )
	{
		// Avoid huge deltas on first regeneration of health after long period of time at full health.
		m_flTimeLastRegen = gpGlobals->curtime;
	}
	else if ( ShouldRegenerateHealth() )
	{
		float flDelta = gpGlobals->curtime - m_flTimeLastRegen;
		float flHealthPerSecond = 1.0f / sk_ally_regen_time->GetFloat();

		float flHealthRegen = flHealthPerSecond * flDelta;

		if ( g_helpfunc.GameRules_IsSkillLevel(SKILL_HARD) )
			flHealthRegen *= 0.5f;
		else if ( g_helpfunc.GameRules_IsSkillLevel(SKILL_EASY) )
			flHealthRegen *= 1.5f;

		m_flTimeLastRegen = gpGlobals->curtime;

		TakeHealth( flHealthRegen, DMG_GENERIC );
	}

#ifdef HL2_EPISODIC
	if ( (GetState() == NPC_STATE_IDLE || GetState() == NPC_STATE_ALERT) 
		 && !HasCondition(COND_RECEIVED_ORDERS) && !IsInAScript() )
	{
		if ( m_flNextIdleSpeechTime && m_flNextIdleSpeechTime < gpGlobals->curtime )
		{
			AISpeechSelection_t selection;
			if ( SelectNonCombatSpeech( &selection ) )
			{
				SetSpeechTarget( selection.hSpeechTarget );
				SpeakDispatchResponse( selection.concept.c_str(), selection.pResponse );
				m_flNextIdleSpeechTime = gpGlobals->curtime + RandomFloat( 20,30 );
			}
			else
			{
				m_flNextIdleSpeechTime = gpGlobals->curtime + RandomFloat( 10,20 );
			}
		}
	}
#endif // HL2_EPISODIC

#endif // HL2_DLL
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CAI_PlayerAlly::SelectSchedule( void )
{
	if ( !HasCondition(COND_RECEIVED_ORDERS) )
	{
		// sustained light wounds?
		if ( m_iHealth <= m_iMaxHealth * 0.75 && IsAllowedToSpeak( TLK_WOUND ) && !GetExpresser()->SpokeConcept(TLK_WOUND) )
		{
			CTakeDamageInfo info;
			PainSound( info );
		}
		// sustained heavy wounds?
		else if ( m_iHealth <= m_iMaxHealth * 0.5 && IsAllowedToSpeak( TLK_MORTAL) )
		{
			Speak( TLK_MORTAL );
		}
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::SelectSpeechResponse( AIConcept_t concept, const char *pszModifiers, CEntity *pTarget, AISpeechSelection_t *pSelection )
{
	if ( IsAllowedToSpeak( concept ) )
	{
		AI_Response *pResponse = SpeakFindResponse( concept, pszModifiers );
		if ( pResponse )
		{
			pSelection->Set( concept, pResponse, (pTarget)?pTarget->BaseEntity():NULL );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::SetPendingSpeech( AIConcept_t concept, AI_Response *pResponse )
{
	m_PendingResponse = *pResponse;
	pResponse->Release();
	m_PendingConcept = concept;
	m_TimePendingSet = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::ClearPendingSpeech()
{
	m_PendingConcept.erase();
	m_TimePendingSet = 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CAI_PlayerAlly::SelectIdleSpeech( AISpeechSelection_t *pSelection )
{
	if ( !IsOkToSpeak( SPEECH_IDLE ) )
		return false;

	CPlayer *pTarget = assert_cast<CPlayer *>(FindSpeechTarget( AIST_PLAYERS | AIST_FACING_TARGET ));
	if ( pTarget )
	{
		if ( SelectSpeechResponse( TLK_HELLO, NULL, pTarget, pSelection ) )
			return true;
		
		if ( GetTimePlayerStaring() > 6 && !IsMoving() )
		{
			if ( SelectSpeechResponse( TLK_STARE, NULL, pTarget, pSelection ) )
				return true;
		}

		int chance = ( IsMoving() ) ? 20 : 2;
		if ( ShouldSpeakRandom( TLK_IDLE, chance ) && SelectSpeechResponse( TLK_IDLE, NULL, pTarget, pSelection ) )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::SelectAlertSpeech( AISpeechSelection_t *pSelection )
{
#ifdef HL2_EPISODIC
	CBasePlayer *pTarget = assert_cast<CBasePlayer *>(FindSpeechTarget( AIST_PLAYERS | AIST_FACING_TARGET ));
	if ( pTarget )
	{
		if ( pTarget->IsAlive() )
		{
			float flHealthPerc = ((float)pTarget->m_iHealth / (float)pTarget->m_iMaxHealth);
			if ( flHealthPerc < 1.0 )
			{
				if ( SelectSpeechResponse( TLK_PLHURT, NULL, pTarget, pSelection ) )
					return true;
			}
		}
	}
#endif

	return SelectIdleSpeech( pSelection );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CAI_PlayerAlly::SelectInterjection()
{
	if ( HasPendingSpeech() )
		return false;

	if ( HasCondition(COND_RECEIVED_ORDERS) )
		return false;

	if ( GetState() == NPC_STATE_IDLE || GetState() == NPC_STATE_ALERT )
	{
		AISpeechSelection_t selection;

		if ( SelectIdleSpeech( &selection ) )
		{
			SetSpeechTarget( selection.hSpeechTarget );
			SpeakDispatchResponse( selection.concept.c_str(), selection.pResponse );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CAI_PlayerAlly::SelectPlayerUseSpeech()
{
	if( IsOkToSpeakInResponseToPlayer() )
	{
		if ( Speak( TLK_USE ) )
			DeferAllIdleSpeech();
		else
			return Speak( ( !GetExpresser()->SpokeConcept( TLK_HELLO ) ) ? TLK_HELLO : TLK_IDLE );
	}	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::SelectQuestionAndAnswerSpeech( AISpeechSelection_t *pSelection )
{
	if ( !IsOkToSpeak( SPEECH_IDLE ) )
		return false;

	if ( IsMoving() )
		return false;

	// if there is a friend nearby to speak to, play sentence, set friend's response time, return
	CAI_PlayerAlly *pFriend = dynamic_cast<CAI_PlayerAlly *>(FindSpeechTarget( AIST_NPCS ));
	if ( pFriend && !pFriend->IsMoving() && !pFriend->HasSpawnFlags(SF_NPC_GAG) )
		return SelectQuestionFriend( pFriend, pSelection );

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::PostSpeakDispatchResponse( AIConcept_t concept, AI_Response *response )
{
#ifdef HL2_EPISODIC
	CAI_AllySpeechManager *pSpeechManager = GetAllySpeechManager();
	ConceptInfo_t *pConceptInfo	= pSpeechManager->GetConceptInfo( concept );
	if ( pConceptInfo && (pConceptInfo->flags & AICF_QUESTION) && GetSpeechTarget() )
	{
		bool bSaidHelloToNPC = !Q_strcmp(concept, "TLK_HELLO_NPC");

		float duration = GetExpresser()->GetSemaphoreAvailableTime(this) - gpGlobals->curtime;

		if ( rr_debug_qa.GetBool() )
		{
			if ( bSaidHelloToNPC )
			{
				Warning("Q&A: '%s' said Hello to '%s' (concept %s)\n", GetDebugName(), GetSpeechTarget()->GetDebugName(), concept );
			}
			else
			{
				Warning("Q&A: '%s' questioned '%s' (concept %s)\n", GetDebugName(), GetSpeechTarget()->GetDebugName(), concept );
			}
			NDebugOverlay::HorzArrow( GetAbsOrigin(), GetSpeechTarget()->GetAbsOrigin(), 8, 0, 255, 0, 64, true, duration );
		}

		// If we spoke a Question, tell our friend to answer
		const char *pszInput;
		if ( bSaidHelloToNPC )
		{
			pszInput = "AnswerQuestionHello";
		}
		else
		{
			pszInput = "AnswerQuestion";
		}

		// Set the input parameter to the enginerandom number we used to find the Question
		variant_t value;
		value.SetInt( m_iQARandomNumber );
		g_EventQueue.AddEvent( GetSpeechTarget(), pszInput, value, duration + .2, this, this );

		if ( GetSpeechTarget()->MyNPCPointer() )
		{
			AddLookTarget( GetSpeechTarget()->MyNPCPointer(), 1.0, duration + enginerandom->RandomFloat( 0.4, 1.2 ), 0.5 );
			GetSpeechTarget()->MyNPCPointer()->AddLookTarget( this, 1.0, duration + enginerandom->RandomFloat( 0.4, 1 ), 0.7 );
		}

		// Don't let anyone else butt in.
		DeferAllIdleSpeech( enginerandom->RandomFloat( TALKER_DEFER_IDLE_SPEAK_MIN, TALKER_DEFER_IDLE_SPEAK_MAX ), GetSpeechTarget()->MyNPCPointer() );
	}
	else if ( pConceptInfo && (pConceptInfo->flags & AICF_ANSWER) && GetSpeechTarget() )
	{
		float duration = GetExpresser()->GetSemaphoreAvailableTime(this) - gpGlobals->curtime;
		if ( rr_debug_qa.GetBool() )
		{
			NDebugOverlay::HorzArrow( GetAbsOrigin(), GetSpeechTarget()->GetAbsOrigin(), 8, 0, 255, 0, 64, true, duration );
		}
		if ( GetSpeechTarget()->MyNPCPointer() )
		{
			AddLookTarget( GetSpeechTarget()->MyNPCPointer(), 1.0, duration + enginerandom->RandomFloat( 0, 0.3 ), 0.5 );
			GetSpeechTarget()->MyNPCPointer()->AddLookTarget( this, 1.0, duration + enginerandom->RandomFloat( 0.2, 0.5 ), 0.7 );
		}
	}

	m_hPotentialSpeechTarget = NULL;
#endif // HL2_EPISODIC
}

//-----------------------------------------------------------------------------
// Purpose: Find a concept to question a nearby friend
//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::SelectQuestionFriend( CEntity *pFriend, AISpeechSelection_t *pSelection )
{
	if ( !ShouldSpeakRandom( TLK_QUESTION, 3 ) )
		return false;

	// Tell the response rules who we're trying to question
	m_hPotentialSpeechTarget.Set((pFriend)?pFriend->BaseEntity():NULL);
	m_iQARandomNumber = RandomInt(0,100);

	// If we haven't said hello, say hello first.
	// Only ever say hello to NPCs other than my type.
	if ( !GetExpresser()->SpokeConcept( TLK_HELLO_NPC  ) && !FClassnameIs( this, pFriend->GetClassname()) )
	{
		if ( SelectSpeechResponse( TLK_HELLO_NPC, NULL, pFriend, pSelection ) )
			return true;

		GetExpresser()->SetSpokeConcept( TLK_HELLO_NPC, NULL );
	}

	return SelectSpeechResponse( TLK_QUESTION, NULL, pFriend, pSelection );
}

//-----------------------------------------------------------------------------
// Purpose: Find a concept to answer our friend's question
//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::SelectAnswerFriend( CEntity *pFriend, AISpeechSelection_t *pSelection, bool bRespondingToHello )
{
	// Tell the response rules who we're trying to answer
	m_hPotentialSpeechTarget.Set((pFriend)?pFriend->BaseEntity():NULL);

	if ( bRespondingToHello )
	{
		if ( SelectSpeechResponse( TLK_ANSWER_HELLO, NULL, pFriend, pSelection ) )
			return true;
	}

	return SelectSpeechResponse( TLK_ANSWER, NULL, pFriend, pSelection );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::InputAnswerQuestion( inputdata_t &inputdata )
{
	CEntity *cent = CEntity::Instance(inputdata.pActivator);
	AnswerQuestion( dynamic_cast<CAI_PlayerAlly *>(cent), inputdata.value.Int(), false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::InputAnswerQuestionHello( inputdata_t &inputdata )
{
	CEntity *cent = CEntity::Instance(inputdata.pActivator);
	AnswerQuestion( dynamic_cast<CAI_PlayerAlly *>(cent), inputdata.value.Int(), true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::AnswerQuestion( CAI_PlayerAlly *pQuestioner, int iQARandomNum, bool bAnsweringHello )
{
	// Original questioner may have died
	if ( !pQuestioner )
		return;

	AISpeechSelection_t selection;

	// Use the enginerandom number that the questioner used to determine his Question (so we can match answers via response rules)
	m_iQARandomNumber = iQARandomNum;

	// The activator is the person we're responding to
 	if ( SelectAnswerFriend( pQuestioner, &selection, bAnsweringHello ) )
	{
		if ( rr_debug_qa->GetBool() )
		{
			if ( bAnsweringHello )
			{
				Warning("Q&A: '%s' answered the Hello from '%s'\n", GetDebugName(), pQuestioner->GetDebugName() );
			}
			else
			{
				Warning("Q&A: '%s' answered the Question from '%s'\n", GetDebugName(), pQuestioner->GetDebugName() );
			}
		}

		Assert( selection.pResponse );
		SetSpeechTarget( selection.hSpeechTarget );
		SpeakDispatchResponse( selection.concept.c_str(), selection.pResponse );

		// Prevent idle speech for a while
		DeferAllIdleSpeech( enginerandom->RandomFloat( TALKER_DEFER_IDLE_SPEAK_MIN, TALKER_DEFER_IDLE_SPEAK_MAX ), GetSpeechTarget()->MyNPCPointer() );
	}
	else if ( rr_debug_qa->GetBool() )
	{
		Warning("Q&A: '%s' couldn't answer '%s'\n", GetDebugName(), pQuestioner->GetDebugName() );
	}
}

//-----------------------------------------------------------------------------
// Output : int
//-----------------------------------------------------------------------------
int CAI_PlayerAlly::SelectNonCombatSpeech( AISpeechSelection_t *pSelection )
{
	bool bResult = false;

	#ifdef HL2_EPISODIC
	// See if we can Q&A first
	if ( GetState() == NPC_STATE_IDLE || GetState() == NPC_STATE_ALERT )
	{
		bResult = SelectQuestionAndAnswerSpeech( pSelection );
	}
	#endif // HL2_EPISODIC

	if ( !bResult )
	{
		if ( GetState() == NPC_STATE_ALERT ) 
		{
			bResult = SelectAlertSpeech( pSelection );
		}
		else
		{
			bResult = SelectIdleSpeech( pSelection );
		}
	}

	return bResult;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CAI_PlayerAlly::SelectNonCombatSpeechSchedule()
{
	if ( !HasPendingSpeech() )
	{
		AISpeechSelection_t selection;
		if ( SelectNonCombatSpeech( &selection ) )
		{
			Assert( selection.pResponse );
			SetSpeechTarget( selection.hSpeechTarget );
			SetPendingSpeech( selection.concept.c_str(), selection.pResponse );
		}
	}
	
	if ( HasPendingSpeech() )
	{
		if ( m_TimePendingSet == gpGlobals->curtime || IsAllowedToSpeak( m_PendingConcept.c_str() ) )
			return SCHED_TALKER_SPEAK_PENDING_IDLE;
	}
	
	return SCHED_NONE;
} 

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CAI_PlayerAlly::TranslateSchedule( int schedule )
{
	if ( ( GetState() == NPC_STATE_IDLE || GetState() == NPC_STATE_ALERT ) && 
		 ConditionInterruptsSchedule( schedule, COND_IDLE_INTERRUPT ) &&
		 !HasCondition(COND_RECEIVED_ORDERS) )
	{
		int speechSchedule = SelectNonCombatSpeechSchedule();
		if ( speechSchedule != SCHED_NONE )
			return speechSchedule;
	}

	switch( schedule )
	{
	case SCHED_CHASE_ENEMY_FAILED:
		{
			int baseType = BaseClass::TranslateSchedule(schedule);
			if ( baseType != SCHED_CHASE_ENEMY_FAILED )
				return baseType;

			return SCHED_TAKE_COVER_FROM_ENEMY;
		}
		break;
	}
	return BaseClass::TranslateSchedule( schedule );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::OnStartSchedule( int schedule )
{
	if ( schedule == SCHED_HIDE_AND_RELOAD )
		SpeakIfAllowed( TLK_HIDEANDRELOAD );
	BaseClass::OnStartSchedule( schedule );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_MOVE_AWAY_PATH:
		{
			if ( HasCondition( COND_PLAYER_PUSHING )  )
			{
				CPlayer *player = UTIL_GetNearestVisiblePlayer(this);
				if(player)
				{
					GetMotor()->SetIdealYawToTarget( player->WorldSpaceCenter() );
				}
			}
			
			BaseClass::StartTask( pTask );
			break;
		}

	case TASK_PLAY_SCRIPT:
		SetSpeechTarget( NULL );
		BaseClass::StartTask( pTask );
		break;

	case TASK_TALKER_SPEAK_PENDING:
		if ( !m_PendingConcept.empty() )
		{
			AI_Response *pResponse = new AI_Response;
			*pResponse = m_PendingResponse;
			SpeakDispatchResponse( m_PendingConcept.c_str(), pResponse );
			m_PendingConcept.erase();
			TaskComplete();
		}
		else
			TaskFail( FAIL_NO_SOUND );
		break;

	default:
		BaseClass::StartTask( pTask );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::RunTask( const Task_t *pTask )
{
	BaseClass::RunTask( pTask );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::TaskFail( AI_TaskFailureCode_t code )
{
	if ( IsCurSchedule( SCHED_TALKER_SPEAK_PENDING_IDLE, false ) )
		ClearPendingSpeech();
	BaseClass::TaskFail( code );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::ClearTransientConditions()
{
	CAI_NPC::ClearTransientConditions();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::Touch( CEntity *pOther )
{
	BaseClass::Touch( pOther );

	// Did the player touch me?
	if ( pOther->IsPlayer() )
	{
		// Ignore if pissed at player
		if ( m_afMemory & bits_MEMORY_PROVOKED )
			return;

		// Stay put during speech
		if ( GetExpresser()->IsSpeaking() )
			return;
			
		TestPlayerPushing( pOther );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::OnKilledNPC( CBaseEntity *pKilled )
{
	if ( pKilled )
	{
		CCombatCharacter *cent = (CCombatCharacter *)CEntity::Instance(pKilled);
		if ( !cent->IsNPC() || 
			( cent->MyNPCPointer()->GetLastPlayerDamageTime() == 0 ||
			  gpGlobals->curtime - cent->MyNPCPointer()->GetLastPlayerDamageTime() > 5 ) )
		{
			SpeakIfAllowed( TLK_ENEMY_DEAD );
		}
	}
}

//-----------------------------------------------------------------------------
void CAI_PlayerAlly::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *dump )
{
	const char *pszHitLocCriterion = NULL;

	if ( ptr->hitgroup == HITGROUP_LEFTLEG || ptr->hitgroup == HITGROUP_RIGHTLEG )
	{
		pszHitLocCriterion = "shotloc:leg";
	}
	else if ( ptr->hitgroup == HITGROUP_LEFTARM || ptr->hitgroup == HITGROUP_RIGHTARM )
	{
		pszHitLocCriterion = "shotloc:arm";
	}
	else if ( ptr->hitgroup == HITGROUP_STOMACH )
	{
		pszHitLocCriterion = "shotloc:gut";
	}

	// set up the speech modifiers
	CFmtStrN<128> modifiers( "%s,damageammo:%s", pszHitLocCriterion, info.GetAmmoName() );

	SpeakIfAllowed( TLK_SHOT, modifiers );

	BaseClass::TraceAttack( info, vecDir, ptr, dump );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CAI_PlayerAlly::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo subInfo = info;
	// Vital allies never take more than 25% of their health in a single hit (except for physics damage)
#ifdef HL2_DLL
	// Don't do damage reduction for DMG_GENERIC. This allows SetHealth inputs to still do full damage.
	if ( subInfo.GetDamageType() != DMG_GENERIC )
	{
		if ( Classify() == CLASS_PLAYER_ALLY_VITAL && !(subInfo.GetDamageType() & DMG_CRUSH) )
		{
			float flDamage = subInfo.GetDamage();
			if ( flDamage > ( GetMaxHealth() * 0.25 ) )
			{
				flDamage = ( GetMaxHealth() * 0.25 );
				subInfo.SetDamage( flDamage );
			}
		}
	}
#endif

	return BaseClass::OnTakeDamage_Alive( subInfo );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CAI_PlayerAlly::TakeHealth( float flHealth, int bitsDamageType )
{
	int intPortion;
	float floatPortion;

	intPortion = ((int)flHealth);
	floatPortion = flHealth - intPortion;

	m_flHealthAccumulator += floatPortion;

	while ( m_flHealthAccumulator > 1.0f )
	{
		m_flHealthAccumulator -= 1.0f;
		intPortion += 1;
	}

	return BaseClass::TakeHealth( ((float)intPortion), bitsDamageType );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::Event_Killed( const CTakeDamageInfo &info )
{
	// notify the player
	if ( IsInPlayerSquad() )
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CPlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;
		
			variant_t emptyVariant;
			pPlayer->CustomAcceptInput( "OnSquadMemberKilled", BaseEntity(), BaseEntity(), emptyVariant, 0 );
		}
	}

	if ( GetSpeechSemaphore( this )->GetOwner() == BaseEntity() )
		GetSpeechSemaphore( this )->Release();

	CAI_PlayerAlly *pMourner = dynamic_cast<CAI_PlayerAlly *>(FindSpeechTarget( AIST_NPCS ));
	if ( pMourner )
	{
		pMourner->SpeakIfAllowed( TLK_ALLY_KILLED );
	}

	SetTarget( NULL );
	// Don't finish that sentence
	SentenceStop();
	SetUse( NULL );
	BaseClass::Event_Killed( info );

	DisplayDeathMessage();
}

// Player allies should use simple shadows to save CPU.  This means they can't 
// be killed by crush damage.
bool CAI_PlayerAlly::CreateVPhysics()
{
	bool bRet = BaseClass::CreateVPhysics();
	return bRet;
}

//-----------------------------------------------------------------------------
void CAI_PlayerAlly::PainSound( const CTakeDamageInfo &info )
{
	SpeakIfAllowed( TLK_WOUND );
}

//-----------------------------------------------------------------------------
// Purpose: Implemented to look at talk target
//-----------------------------------------------------------------------------
CBaseEntity *CAI_PlayerAlly::EyeLookTarget( void )
{
	// FIXME: this should be in the VCD
	// FIXME: this is dead code
	if (GetExpresser()->IsSpeaking() && GetSpeechTarget() != NULL)
	{
		return GetSpeechTarget()->BaseEntity();
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: returns who we're talking to for vcd's
//-----------------------------------------------------------------------------
CBaseEntity *CAI_PlayerAlly::FindNamedEntity( const char *pszName, IEntityFindFilter *pFilter )
{
	if ( !stricmp( pszName, "!speechtarget" ))
	{
		CEntity *cent = GetSpeechTarget();
		return (cent)?cent->BaseEntity():NULL;
	}

	if ( !stricmp( pszName, "!friend" ))
	{
		CEntity *cent = FindSpeechTarget( AIST_NPCS );
		return (cent)?cent->BaseEntity():NULL;
	}


	return BaseClass::FindNamedEntity( pszName, pFilter );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::IsValidSpeechTarget( int flags, CEntity *pEntity )
{
	if ( pEntity == this )
		return false;

	if ( !(flags & AIST_IGNORE_RELATIONSHIP) )
	{
		if ( pEntity->IsPlayer() )
		{
			if ( !IsPlayerAlly( pEntity->BaseEntity() ) )
				return false;
		}
		else
		{
			if ( IRelationType( pEntity->BaseEntity() ) != D_LI )
				return false;
		}
	}		

	if ( !pEntity->IsAlive() )
		// don't dead people
		return false;

	// Ignore no-target entities
	if ( pEntity->GetFlags() & FL_NOTARGET )
		return false;

	CAI_NPC *pNPC = pEntity->MyNPCPointer();
	if ( pNPC )
	{
		// If not a NPC for some reason, or in a script.
		if ( (pNPC->m_NPCState == NPC_STATE_SCRIPT || pNPC->m_NPCState == NPC_STATE_PRONE))
			return false;

		if ( pNPC->IsInAScript() )
			return false;

		// Don't bother people who don't want to be bothered
		if ( !pNPC->CanBeUsedAsAFriend() )
			return false;
	}
	
	if ( flags & AIST_FACING_TARGET )
	{
		if ( pEntity->IsPlayer() )
			return HasCondition( COND_SEE_PLAYER );
		else if ( !FInViewCone_Entity( pEntity->BaseEntity() ) )
			return false;
	}

	return FVisible_Entity( pEntity->BaseEntity() );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CEntity *CAI_PlayerAlly::FindSpeechTarget( int flags )
{
	const Vector &	vAbsOrigin 		= GetAbsOrigin();
	float 			closestDistSq 	= FLT_MAX;
	CEntity *		pNearest 		= NULL;
	float			distSq;
	int				i;
	
	if ( flags & AIST_PLAYERS )
	{
		for ( i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CEntity *pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer )
			{
				distSq = ( vAbsOrigin - pPlayer->GetAbsOrigin() ).LengthSqr();
				
				if ( distSq > Square(TALKRANGE_MIN) )
					continue;
					
				if ( !(flags & AIST_ANY_QUALIFIED) && distSq > closestDistSq )
					continue;

				if ( IsValidSpeechTarget( flags, pPlayer ) )
				{
					if ( flags & AIST_ANY_QUALIFIED )
						return pPlayer;

					closestDistSq = distSq;
					pNearest = pPlayer;
				}
			}
		}
	}
	
	if ( flags & AIST_NPCS )
	{
		for ( i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
			CAI_NPC *pNPC = (g_AI_Manager.AccessAIs())[i];

			distSq = ( vAbsOrigin - pNPC->GetAbsOrigin() ).LengthSqr();
			
			if ( distSq > Square(TALKRANGE_MIN) )
				continue;
				
			if ( !(flags & AIST_ANY_QUALIFIED) && distSq > closestDistSq )
				continue;

			if ( IsValidSpeechTarget( flags, pNPC ) )
			{
				if ( flags & AIST_ANY_QUALIFIED )
					return pNPC;

				closestDistSq = distSq;
				pNearest = pNPC;
			}
		}
	}

	return pNearest;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::CanPlaySentence( bool fDisregardState ) 
{ 
	if ( fDisregardState )
		return BaseClass::CanPlaySentence( fDisregardState );
	return IsOkToSpeak(); 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CAI_PlayerAlly::PlayScriptedSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener )
{
	ClearCondition( COND_PLAYER_PUSHING );	// Forget about moving!  I've got something to say!
	int sentenceIndex = BaseClass::PlayScriptedSentence( pszSentence, delay, volume, soundlevel, bConcurrent, pListener );
	SetSpeechTarget( pListener );
	
	return sentenceIndex;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CAI_PlayerAlly::DeferAllIdleSpeech( float flDelay, CAI_NPC *pIgnore )
{
	CAI_AllySpeechManager *pSpeechManager = GetAllySpeechManager();
	if ( flDelay == -1 )
	{
		ConceptCategoryInfo_t *pCategoryInfo = pSpeechManager->GetConceptCategoryInfo( SPEECH_IDLE );
		pSpeechManager->SetCategoryDelay( SPEECH_IDLE, pCategoryInfo->minGlobalDelay, pCategoryInfo->maxGlobalDelay );
	}
	else
		pSpeechManager->SetCategoryDelay( SPEECH_IDLE, flDelay );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool CAI_PlayerAlly::IsOkToSpeak( ConceptCategory_t category, bool fRespondingToPlayer )
{
	CAI_AllySpeechManager *pSpeechManager = GetAllySpeechManager();
	
	// if not alive, certainly don't speak
	if ( !IsAlive() )
		return false;

	if ( m_spawnflags & SF_NPC_GAG )
		return false;

	// Don't speak if playing a script.
	if ( ( m_NPCState == NPC_STATE_SCRIPT ) && !m_bCanSpeakWhileScripting )
		return false;

	// Don't speak if being eaten by a barnacle
	if ( IsEFlagSet( EFL_IS_BEING_LIFTED_BY_BARNACLE ) )
		return false;

	if ( IsInAScript() && !m_bCanSpeakWhileScripting )
		return false;

	if ( !fRespondingToPlayer )
	{
		if ( !pSpeechManager->CategoryDelayExpired( category ) || !CategoryDelayExpired( category ) )
			return false;
	}

	if ( category == SPEECH_IDLE )
	{
		if ( GetState() != NPC_STATE_IDLE && GetState() != NPC_STATE_ALERT )
			return false;
		if ( GetSpeechFilter() && GetSpeechFilter()->GetIdleModifier() < 0.001 )
			return false;
	}

	// if player is not in pvs, don't speak
	if ( !UTIL_FindClientInPVS(edict()) )
		return false;

	if ( category != SPEECH_PRIORITY )
	{
		// if someone else is talking, don't speak
		if ( !GetExpresser()->SemaphoreIsAvailable( BaseEntity() ) )
			return false;

		if ( fRespondingToPlayer )
		{
			if ( !GetExpresser()->CanSpeakAfterMyself() )
				return false;
		}
		else
		{
			if ( !GetExpresser()->CanSpeak() )
				return false;
		}

		// Don't talk if we're too far from the player
		CEntity *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());

		if ( pPlayer )
		{
			float flDist = sv_npc_talker_maxdist->GetFloat();
			flDist *= flDist;
			if ( (pPlayer->WorldSpaceCenter() - WorldSpaceCenter()).LengthSqr() > flDist )
				return false;
		}
	}

	if ( fRespondingToPlayer )
	{
		// If we're responding to the player, don't respond if the scene has speech in it
		if ( IsRunningScriptedSceneWithSpeechAndNotPaused( this ) )
		{
			if( rr_debugresponses->GetInt() > 0 )
			{
				DevMsg("%s not allowed to speak because they are in a scripted scene\n", GetDebugName() );
			}

			return false;
		}
	}
	else
	{
		// If we're not responding to the player, don't talk if running a logic_choreo
		if ( IsRunningScriptedSceneAndNotPaused( this ) )
		{
			if( rr_debugresponses->GetInt() > 0 )
			{
				DevMsg("%s not allowed to speak because they are in a scripted scene\n", GetDebugName() );
			}

			return false;
		}
	}

	return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool CAI_PlayerAlly::IsOkToSpeak( void )
{
	return IsOkToSpeak( SPEECH_IDLE );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool CAI_PlayerAlly::IsOkToCombatSpeak( void )
{
	return IsOkToSpeak( SPEECH_IMPORTANT );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool CAI_PlayerAlly::IsOkToSpeakInResponseToPlayer( void )
{
	return IsOkToSpeak( SPEECH_IMPORTANT, true );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if I should speak based on the chance & the speech filter's modifier
//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::ShouldSpeakRandom( AIConcept_t concept, int iChance )
{
	CAI_AllySpeechManager *	pSpeechManager	= GetAllySpeechManager();
	ConceptInfo_t *			pInfo			= pSpeechManager->GetConceptInfo( concept );
	ConceptCategory_t		category		= ( pInfo ) ? pInfo->category : SPEECH_IDLE;

	if ( GetSpeechFilter() )
	{
		if ( category == SPEECH_IDLE )
		{
			float flModifier = GetSpeechFilter()->GetIdleModifier();
			if ( flModifier < 0.001 )
				return false;
				
			iChance = (int)floor( iChance / flModifier );
		}
	}
	
	if ( iChance < 1 )
		return false;

	if ( iChance == 1 )
		return true;
		
	return (enginerandom->RandomInt(1,iChance) == 1);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::IsAllowedToSpeak( AIConcept_t concept, bool bRespondingToPlayer ) 
{ 
	CAI_AllySpeechManager *	pSpeechManager	= GetAllySpeechManager();
	ConceptInfo_t *			pInfo			= pSpeechManager->GetConceptInfo( concept );
	ConceptCategory_t		category		= ( pInfo ) ? pInfo->category : SPEECH_IDLE;

	if ( !IsOkToSpeak( category, bRespondingToPlayer ) )
		return false;

	if ( GetSpeechFilter() && GetSpeechFilter()->NeverSayHello() )
	{
		if ( CompareConcepts( concept, TLK_HELLO ) )
			return false;
		if ( CompareConcepts( concept, TLK_HELLO_NPC ) )
			return false;
	}
			
	if ( !pSpeechManager->ConceptDelayExpired( concept ) )
		return false;

	if ( ( pInfo && pInfo->flags & AICF_SPEAK_ONCE ) && GetExpresser()->SpokeConcept( concept ) )
		return false;

	if ( !GetExpresser()->CanSpeakConcept( concept ) )
		return false;
	
	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::SpeakIfAllowed( AIConcept_t concept, const char *modifiers, bool bRespondingToPlayer, char *pszOutResponseChosen, size_t bufsize ) 
{ 
	if ( IsAllowedToSpeak( concept, bRespondingToPlayer ) )
	{
		return Speak( concept, modifiers, pszOutResponseChosen, bufsize );
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::ModifyOrAppendCriteria( AI_CriteriaSet& set )
{
	BaseClass::ModifyOrAppendCriteria( set );

	if ( m_hPotentialSpeechTarget )
	{
		set.AppendCriteria( "speechtarget", m_hPotentialSpeechTarget->GetClassname() );
		set.AppendCriteria( "speechtargetname", m_hPotentialSpeechTarget->GetEntityName() );
		set.AppendCriteria( "randomnum", UTIL_VarArgs("%d", m_iQARandomNumber) );
	}

	// Do we have a speech filter? If so, append it's criteria too
	if ( GetSpeechFilter() )
	{
		GetSpeechFilter()->AppendContextToCriteria( set );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::OnSpokeConcept( AIConcept_t concept, AI_Response *response )
{
	CAI_AllySpeechManager *pSpeechManager = GetAllySpeechManager();
	pSpeechManager->OnSpokeConcept( this, concept, response );

	if( response != NULL && (response->GetParams()->flags & AI_ResponseParams::RG_WEAPONDELAY) )
	{
		// Stop shooting, as instructed, so that my speech can be heard.
		GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + response->GetWeaponDelay() );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::OnStartSpeaking()
{
	// If you say anything, don't greet the player - you may have already spoken to them
	if ( !GetExpresser()->SpokeConcept( TLK_HELLO ) )
	{
		GetExpresser()->SetSpokeConcept( TLK_HELLO, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Mapmaker input to force this NPC to speak a response rules concept
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::InputSpeakResponseConcept( inputdata_t &inputdata )
{
	SpeakMapmakerInterruptConcept( inputdata.value.StringID() );
}


//-----------------------------------------------------------------------------
// Allows mapmakers to override NPC_STATE_SCRIPT or IsScripting() for responses.
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::InputEnableSpeakWhileScripting( inputdata_t &inputdata )
{
	m_bCanSpeakWhileScripting = true;
}

void CAI_PlayerAlly::InputDisableSpeakWhileScripting( inputdata_t &inputdata )
{
	m_bCanSpeakWhileScripting = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::SpeakMapmakerInterruptConcept( string_t iszConcept )
{
	// Let behaviors override
	if ( BaseClass::SpeakMapmakerInterruptConcept(iszConcept) )
		return false;

	if (!IsOkToSpeakInResponseToPlayer())
		return false;

	Speak( STRING(iszConcept) );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::CanRespondToEvent( const char *ResponseConcept )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::RespondedTo( const char *ResponseConcept, bool bForce, bool bCancelScene )
{
	if ( bForce )
	{
		// We're being forced to respond to the event, probably because it's the
		// player dying or something equally important. 
		AI_Response *result = SpeakFindResponse( ResponseConcept, NULL );
		if ( result )
		{
			// We've got something to say. Stop any scenes we're in, and speak the response.
			if ( bCancelScene )
				RemoveActorFromScriptedScenes( this, false );

			bool spoke = SpeakDispatchResponse( ResponseConcept, result );
			return spoke;
		}

		return false;
	}

	if ( SpeakIfAllowed( ResponseConcept, NULL, true ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC(talk_monster_base,CAI_PlayerAlly)

	DECLARE_TASK(TASK_TALKER_SPEAK_PENDING)

	DECLARE_CONDITION(COND_TALKER_CLIENTUNSEEN)
	DECLARE_CONDITION(COND_TALKER_PLAYER_DEAD)
	DECLARE_CONDITION(COND_TALKER_PLAYER_STARING)

	//=========================================================
	// > SCHED_TALKER_SPEAK_PENDING_IDLE
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_SPEAK_PENDING_IDLE,

		"	Tasks"
		"		TASK_TALKER_SPEAK_PENDING		0"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		"		TASK_WAIT_FOR_SPEAK_FINISH		0"
		"		TASK_WAIT_RANDOM			0.5"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_PLAYER_PUSHING"
		"		COND_GIVE_WAY"
	)

	//=========================================================
	// > SCHED_TALKER_SPEAK_PENDING_ALERT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_SPEAK_PENDING_ALERT,

		"	Tasks"
		"		TASK_TALKER_SPEAK_PENDING		0"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		"		TASK_WAIT_FOR_SPEAK_FINISH		0"
		"		TASK_WAIT_RANDOM				0.5"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_PLAYER_PUSHING"
		"		COND_GIVE_WAY"
	)

	//=========================================================
	// > SCHED_TALKER_SPEAK_PENDING_COMBAT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_SPEAK_PENDING_COMBAT,

		"	Tasks"
		"		TASK_TALKER_SPEAK_PENDING		0"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		"		TASK_WAIT_FOR_SPEAK_FINISH		0"
		""
		"	Interrupts"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
	)

AI_END_CUSTOM_NPC()

//-----------------------------------------------------------------------------