
#include "CAI_Memory.h"
#include "CAI_NPC.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	EMEMORY_POOL_SIZE		  64
#define AI_FREE_KNOWLEDGE_DURATION 1.75


AI_EnemyInfo_t::AI_EnemyInfo_t(void) 
{
	hEnemy				= NULL;
	vLastKnownLocation	= vec3_origin;
	vLastSeenLocation	= vec3_origin;
	timeLastSeen = 0;
	timeFirstSeen = 0;
	timeLastReacquired = 0;
	timeValidEnemy = 0;
	timeLastReceivedDamageFrom = 0;
	timeAtFirstHand = AI_INVALID_TIME;
	bDangerMemory = 0;
	bEludedMe = 0;
	bUnforgettable = 0;
	bMobbedMe = 0;
}


CEAI_Enemies::CEAI_Enemies(void)
{
	m_flFreeKnowledgeDuration = AI_FREE_KNOWLEDGE_DURATION;
	m_flEnemyDiscardTime = AI_DEF_ENEMY_DISCARD_TIME;
	m_vecDefaultLKP = vec3_invalid;
	m_vecDefaultLSP = vec3_invalid;
	m_serial = 0;
	SetDefLessFunc( m_Map );
}


CEAI_Enemies::~CEAI_Enemies()
{
	for ( CMemMap::IndexType_t i = m_Map.FirstInorder(); i != m_Map.InvalidIndex(); i = m_Map.NextInorder( i ) )
	{
		g_pMemAlloc->Free(m_Map[i]);
	}
}

void CEAI_Enemies::SetFreeKnowledgeDuration( float flDuration )
{ 
	m_flFreeKnowledgeDuration = flDuration;	

	if ( m_flFreeKnowledgeDuration >= m_flEnemyDiscardTime )
	{
		// If your free knowledge time is greater than your discard time,
		// you'll forget about secondhand enemies passed to you by squadmates
		// as soon as you're given them.
		//Assert( m_flFreeKnowledgeDuration < m_flEnemyDiscardTime );

		m_flFreeKnowledgeDuration = m_flEnemyDiscardTime - .1;
	}
}



//-----------------------------------------------------------------------------
// Purpose: Returns last known posiiton of given enemy
//-----------------------------------------------------------------------------
const Vector &CEAI_Enemies::LastKnownPosition( CBaseEntity *pEnemy )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy, true );
	if ( pMemory )
	{
		m_vecDefaultLKP = pMemory->vLastKnownLocation;
	}
	else
	{
		DevWarning( 2,"Asking LastKnownPosition for enemy that's not in my memory!!\n");
	}
	return m_vecDefaultLKP;
}


//-----------------------------------------------------------------------------

AI_EnemyInfo_t *CEAI_Enemies::Find( CBaseEntity *pEntity, bool bTryDangerMemory )
{
	if ( pEntity == AI_UNKNOWN_ENEMY )
		pEntity = NULL;

	CMemMap::IndexType_t i = m_Map.Find( pEntity );
	if ( i == m_Map.InvalidIndex() )
	{
		if ( !bTryDangerMemory || ( i = m_Map.Find( NULL ) ) == m_Map.InvalidIndex() )
			return NULL;
		Assert(m_Map[i]->bDangerMemory == true);
	}
	return m_Map[i];
}


void CEAI_Enemies::SetEnemyDiscardTime( float flTime )
{ 
	m_flEnemyDiscardTime = flTime;			

	if ( m_flFreeKnowledgeDuration >= m_flEnemyDiscardTime )
	{
		// If your free knowledge time is greater than your discard time,
		// you'll forget about secondhand enemies passed to you by squadmates
		// as soon as you're given them.
		Assert( m_flFreeKnowledgeDuration < m_flEnemyDiscardTime );

		m_flFreeKnowledgeDuration = m_flEnemyDiscardTime - .1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Notes that the given enemy has eluded me
//-----------------------------------------------------------------------------
void CEAI_Enemies::MarkAsEluded( CBaseEntity *pEnemy )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy );
	if ( pMemory )
	{
		pMemory->bEludedMe = true;
	}
}

float CEAI_Enemies::LastTimeSeen( CBaseEntity *pEnemy, bool bCheckDangerMemory /*= true*/ )
{
	// I've never seen something that doesn't exist
	if (!pEnemy)
		return 0;

	AI_EnemyInfo_t *pMemory = Find( pEnemy, bCheckDangerMemory );
	if ( pMemory )
		return pMemory->timeLastSeen;

	if ( pEnemy != AI_UNKNOWN_ENEMY )
		DevWarning( 2,"Asking LastTimeSeen for enemy that's not in my memory!!\n");
	return AI_INVALID_TIME;
}


//-----------------------------------------------------------------------------
// Purpose:	Clear information about our enemy
//-----------------------------------------------------------------------------
void CEAI_Enemies::ClearMemory(CBaseEntity *pEnemy)
{
	CMemMap::IndexType_t i = m_Map.Find( pEnemy );
	if ( i != m_Map.InvalidIndex() )
	{
		g_pMemAlloc->Free( m_Map[i] );
		m_Map.RemoveAt( i );
	}
}

AI_EnemyInfo_t *CEAI_Enemies::GetNext( AIEnemiesIter_t *pIter )
{
	CMemMap::IndexType_t i = (CMemMap::IndexType_t)((unsigned)(*pIter));

	if ( i == m_Map.InvalidIndex() )
		return NULL;

	i = m_Map.NextInorder( i );
	*pIter = (AIEnemiesIter_t)(unsigned)i;
	if ( i == m_Map.InvalidIndex() )
		return NULL;

	if ( m_Map[i]->hEnemy == NULL )
		return GetNext( pIter );

	return m_Map[i];
}


//-----------------------------------------------------------------------------
// Purpose:	Purges any dead enemies from memory
//-----------------------------------------------------------------------------

AI_EnemyInfo_t *CEAI_Enemies::GetFirst( AIEnemiesIter_t *pIter )
{
	CMemMap::IndexType_t i = m_Map.FirstInorder();
	*pIter = (AIEnemiesIter_t)(unsigned)i;

	if ( i == m_Map.InvalidIndex() )
		return NULL;

	if ( m_Map[i]->hEnemy == NULL )
		return GetNext( pIter );

	return m_Map[i];
}

//------------------------------------------------------------------------------
// Purpose : Returns true if this enemy is part of my memory
//------------------------------------------------------------------------------
bool CEAI_Enemies::HasMemory( CBaseEntity *pEnemy )
{
	return ( Find( pEnemy ) != NULL );
}

//------------------------------------------------------------------------------
// Purpose : Returns true if this enemy is part of my memory
//------------------------------------------------------------------------------
void CEAI_Enemies::OnTookDamageFrom( CBaseEntity *pEnemy )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy, true );
	if ( pMemory )
		pMemory->timeLastReceivedDamageFrom = gpGlobals->curtime;
}

void CEAI_Enemies::RefreshMemories(void)
{
	if ( m_flFreeKnowledgeDuration >= m_flEnemyDiscardTime )
	{
		m_flFreeKnowledgeDuration = m_flEnemyDiscardTime - .1;
	}

	// -------------------
	// Check each record
	// -------------------
	
	CMemMap::IndexType_t i = m_Map.FirstInorder();
	while ( i != m_Map.InvalidIndex() )
	{	
		AI_EnemyInfo_t *pMemory = m_Map[i];
		
		CMemMap::IndexType_t iNext = m_Map.NextInorder( i ); // save so can remove
		if ( ShouldDiscardMemory( pMemory ) )
		{
			g_pMemAlloc->Free(pMemory);
			m_Map.RemoveAt(i);
		}
		else if ( pMemory->hEnemy )
		{
			CEntity *enemy = CEntity::Instance(pMemory->hEnemy);

			if ( gpGlobals->curtime <= pMemory->timeLastSeen + m_flFreeKnowledgeDuration )
			{
				// Free knowledge is ignored if the target has notarget on
				if ( !(enemy->GetFlags() & FL_NOTARGET) )
				{
					pMemory->vLastKnownLocation = enemy->GetAbsOrigin();
				}
			}

			if ( gpGlobals->curtime <= pMemory->timeLastSeen )
			{
				pMemory->vLastSeenLocation = enemy->GetAbsOrigin();
			}
		}
		i = iNext;
	}
}

bool CEAI_Enemies::ShouldDiscardMemory( AI_EnemyInfo_t *pMemory )
{
	CEntity *pEnemy = CEntity::Instance(pMemory->hEnemy);

	if ( pEnemy )
	{
		CAI_NPC *pEnemyNPC = pEnemy->MyNPCPointer();
		if ( pEnemyNPC && pEnemyNPC->GetState() == NPC_STATE_DEAD )
			return true;
	}
	else
	{
		if ( !pMemory->bDangerMemory )
			return true;
	}

	if ( !pMemory->bUnforgettable &&
		 gpGlobals->curtime > pMemory->timeLastSeen + m_flEnemyDiscardTime )
	{
		return true;
	}

	return false;
}


bool CEAI_Enemies::HasEludedMe( CBaseEntity *pEnemy )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy );
	if ( pMemory )
		return pMemory->bEludedMe;
	return false;
}

const Vector &CEAI_Enemies::LastSeenPosition( CBaseEntity *pEnemy )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy, true );
	if ( pMemory )
	{
		m_vecDefaultLSP = pMemory->vLastSeenLocation;
	}
	else
	{
		DevWarning( 2,"Asking LastSeenPosition for enemy that's not in my memory!!\n");
	}
	return m_vecDefaultLSP;
}

float CEAI_Enemies::TimeAtFirstHand( CBaseEntity *pEnemy )
{
	// I've never seen something that doesn't exist
	if (!pEnemy)
		return 0;

	AI_EnemyInfo_t *pMemory = Find( pEnemy, true );
	if ( pMemory )
		return pMemory->timeAtFirstHand;

	if ( pEnemy != AI_UNKNOWN_ENEMY )
		DevWarning( 2,"Asking TimeAtFirstHand for enemy that's not in my memory!!\n");
	return AI_INVALID_TIME;
}

float CEAI_Enemies::FirstTimeSeen( CBaseEntity *pEnemy)
{
	// I've never seen something that doesn't exist
	if (!pEnemy)
		return 0;

	AI_EnemyInfo_t *pMemory = Find( pEnemy, true );
	if ( pMemory )
		return pMemory->timeFirstSeen;

	if ( pEnemy != AI_UNKNOWN_ENEMY )
		DevWarning( 2,"Asking FirstTimeSeen for enemy that's not in my memory!!\n");
	return AI_INVALID_TIME;
}

AI_EnemyInfo_t *CEAI_Enemies::GetDangerMemory()
{
	CMemMap::IndexType_t i = m_Map.Find( NULL );
	if ( i == m_Map.InvalidIndex() )
		return NULL;
	Assert(m_Map[i]->bDangerMemory == true);
	return m_Map[i];
}


void CEAI_Enemies::SetUnforgettable( CBaseEntity *pEnemy, bool bUnforgettable )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy );
	if ( pMemory )
		pMemory->bUnforgettable = bUnforgettable;
}

void CEAI_Enemies::SetTimeValidEnemy( CBaseEntity *pEnemy, float flTime )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy );
	if ( pMemory )
		pMemory->timeValidEnemy = flTime;
}

bool CEAI_Enemies::UpdateMemory(CAI_Network* pAINet, CBaseEntity *pEnemy, const Vector &vPosition, float reactionDelay, bool firstHand )
{
	if ( pEnemy == AI_UNKNOWN_ENEMY )
		pEnemy = NULL;

	const float DIST_TRIGGER_REACQUIRE_SQ			= Square(20.0 * 12.0);
	const float TIME_TRIGGER_REACQUIRE				= 4.0;
	const float MIN_DIST_TIME_TRIGGER_REACQUIRE_SQ 	= Square(4.0 * 12.0);

	AI_EnemyInfo_t *pMemory = Find( pEnemy );
	// -------------------------------------------
	//  Otherwise just update my own
	// -------------------------------------------
	// Update enemy information
	if ( pMemory )
	{
		Assert(pEnemy || pMemory->bDangerMemory == true);

		if ( firstHand )
			pMemory->timeLastSeen = gpGlobals->curtime;
		pMemory->bEludedMe = false;
		
		float deltaDist = (pMemory->vLastKnownLocation - vPosition).LengthSqr();
		
		if (deltaDist>DIST_TRIGGER_REACQUIRE_SQ || ( deltaDist>MIN_DIST_TIME_TRIGGER_REACQUIRE_SQ && ( gpGlobals->curtime - pMemory->timeLastSeen ) > TIME_TRIGGER_REACQUIRE ) )
		{
			pMemory->timeLastReacquired = gpGlobals->curtime;
		}

		// Only update if the enemy has moved
		if (deltaDist>Square(12.0))
		{
			pMemory->vLastKnownLocation = vPosition;

		}

		// Update the time at which we first saw him firsthand
		if ( firstHand && pMemory->timeAtFirstHand == AI_INVALID_TIME )
		{
			pMemory->timeAtFirstHand = gpGlobals->curtime;
		}

		return false;
	}

	// If not on my list of enemies add it
	AI_EnemyInfo_t *pAddMemory = new AI_EnemyInfo_t;
	pAddMemory->vLastKnownLocation = vPosition;

	if ( firstHand )
	{
		pAddMemory->timeLastReacquired = pAddMemory->timeFirstSeen = pAddMemory->timeLastSeen = pAddMemory->timeAtFirstHand = gpGlobals->curtime;
	}
	else
	{
		// Block free knowledge
		pAddMemory->timeLastReacquired = pAddMemory->timeFirstSeen = pAddMemory->timeLastSeen = ( gpGlobals->curtime - (m_flFreeKnowledgeDuration + 0.01) );
		pAddMemory->timeAtFirstHand = AI_INVALID_TIME;
	}

	if ( reactionDelay > 0.0 )
		pAddMemory->timeValidEnemy = gpGlobals->curtime + reactionDelay;

	pAddMemory->bEludedMe = false;
	// I'm either remembering a postion of an enmey of just a danger position
	pAddMemory->hEnemy = pEnemy;
	pAddMemory->bDangerMemory = ( pEnemy == NULL );

	// add to the list
	m_Map.Insert( pEnemy, pAddMemory );
	m_serial++;

	return true;
}