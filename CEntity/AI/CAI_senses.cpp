
#include "CAI_NPC.h"
#include "CAI_senses.h"
#include "CPlayer.h"
#include "CESoundEnt.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#pragma pack(push, 1)

struct AISightIterVal_t
{
	char  array;
	short iNext;
	char  SeenArray;
};

#pragma pack(pop)

const float AI_STANDARD_NPC_SEARCH_TIME = .25;
const float AI_EFFICIENT_NPC_SEARCH_TIME = .35;
const float AI_HIGH_PRIORITY_SEARCH_TIME = 0.15;
const float AI_MISC_SEARCH_TIME  = 0.45;

//-----------------------------------------------------------------------------

CEntity *CAI_Senses::GetNextSeenEntity( AISightIter_t *pIter ) const	
{ 
	if ( ((int)*pIter) != -1 )
	{
		AISightIterVal_t *pIterVal = (AISightIterVal_t *)pIter;
		
		for ( int i = pIterVal->array;  i < (int)ARRAYSIZE( m_SeenArrays ); i++ )
		{
			for ( int j = pIterVal->iNext; j < m_SeenArrays[i]->Count(); j++ )
			{
				if ( (*m_SeenArrays[i])[j].Get() != NULL )
				{
					pIterVal->array = i;
					pIterVal->iNext = j+1;
					return CEntity::Instance((*m_SeenArrays[i])[j]);
				}
			}
			pIterVal->iNext = 0;

			// If we're searching for a specific type, don't move to the next array
			if ( pIterVal->SeenArray != SEEN_ALL )
				break;
		}
		(*pIter) = (AISightIter_t)(-1); 
	}
	return NULL;
}


//-----------------------------------------------------------------------------

CEntity *CAI_Senses::GetFirstSeenEntity( AISightIter_t *pIter, seentype_t iSeenType ) const
{ 
	COMPILE_TIME_ASSERT( sizeof( AISightIter_t ) == sizeof( AISightIterVal_t ) );
	
	AISightIterVal_t *pIterVal = (AISightIterVal_t *)pIter;
	
	// If we're searching for a specific type, start in that array
	pIterVal->SeenArray = (char)iSeenType;
	int iFirstArray = ( iSeenType == SEEN_ALL ) ? 0 : iSeenType;

	for ( int i = iFirstArray; i < (int)ARRAYSIZE( m_SeenArrays ); i++ )
	{
		if ( m_SeenArrays[i]->Count() != 0 )
		{
			pIterVal->array = i;
			pIterVal->iNext = 1;
			return CEntity::Instance((*m_SeenArrays[i])[0]);
		}
	}
	
	(*pIter) = (AISightIter_t)(-1); 
	return NULL;
}

bool CAI_Senses::DidSeeEntity( CEntity *pSightEnt ) const
{
	AISightIter_t iter;
	CEntity *pTestEnt;

	pTestEnt = GetFirstSeenEntity( &iter );

	while( pTestEnt )
	{
		if ( pSightEnt == pTestEnt )
			return true;
		pTestEnt = GetNextSeenEntity( &iter );
	}
	return false;
}

bool CAI_Senses::CanSeeEntity( CBaseEntity *pSightEnt )
{
	return ( GetOuter()->FInViewCone_Entity( pSightEnt ) && GetOuter()->FInViewCone_Entity( pSightEnt ) );
}

bool CAI_Senses::Look( CBaseEntity *pSightEnt )
{
	if ( WaitingUntilSeen( pSightEnt ) )
		return false;
	
	if ( ShouldSeeEntity( pSightEnt ) && CanSeeEntity( pSightEnt ) )
	{
		return SeeEntity( pSightEnt );
	}
	return false;
}


bool CAI_Senses::SeeEntity( CBaseEntity *pSightEnt )
{
	GetOuter()->OnSeeEntity( pSightEnt );

	// insert at the head of my sight list
	NoteSeenEntity( pSightEnt );

	return true;
}

void CAI_Senses::NoteSeenEntity( CBaseEntity *pSightEnt )
{
	*(CEntity::Instance(pSightEnt)->m_pLink.ptr) = *(GetOuter()->m_pLink.ptr);
	GetOuter()->m_pLink = pSightEnt;
}

bool CAI_Senses::ShouldSeeEntity( CBaseEntity *pSightEnt )
{
	CEntity *cent = CEntity::Instance(pSightEnt);
	if ( cent ==NULL || cent == GetOuter() || !cent->IsAlive() )
		return false;

	if ( cent->IsPlayer() && ( cent->GetFlags() & FL_NOTARGET ) )
		return false;

	// don't notice anyone waiting to be seen by the player
	if ( cent->m_spawnflags & SF_NPC_WAIT_TILL_SEEN )
		return false;

	if ( !cent->CanBeSeenBy( GetOuter()->BaseEntity() ) )
		return false;
	
	if ( !GetOuter()->QuerySeeEntity( pSightEnt, true ) )
		return false;

	return true;
}

bool CAI_Senses::WaitingUntilSeen( CBaseEntity *pSightEnt )
{
	CEntity *cent = CEntity::Instance(pSightEnt);
	if ( GetOuter()->m_spawnflags & SF_NPC_WAIT_TILL_SEEN )
	{
		if ( cent->IsPlayer() )
		{
			CPlayer *pPlayer = ToBasePlayer( cent );
			Vector zero =  Vector(0,0,0);
			// don't link this client in the list if the npc is wait till seen and the player isn't facing the npc
			if ( pPlayer
				// && pPlayer->FVisible( GetOuter() ) 
				&& pPlayer->FInViewCone_Entity( GetOuter()->BaseEntity() )
				&& FBoxVisible( cent, static_cast<CEntity*>(GetOuter()), zero ) )
			{
				// player sees us, become normal now.
				GetOuter()->m_spawnflags &= ~SF_NPC_WAIT_TILL_SEEN;
				return false;
			}
		}
		return true;
	}

	return false;
}


void CAI_Senses::Look( int iDistance )
{
	if ( m_TimeLastLook != gpGlobals->curtime || m_LastLookDist != iDistance )
	{
		//-----------------------------
		
		LookForHighPriorityEntities( iDistance );
		LookForNPCs( iDistance);
		LookForObjects( iDistance );
		
		//-----------------------------
		
		m_LastLookDist = iDistance;
		m_TimeLastLook = gpGlobals->curtime;
	}
	
	GetOuter()->OnLooked( iDistance );
}

int CAI_Senses::LookForObjects( int iDistance )
{	
	const int BOX_QUERY_MASK = FL_OBJECT;
	int	nSeen = 0;

	if ( gpGlobals->curtime - m_TimeLastLookMisc > AI_MISC_SEARCH_TIME )
	{
		m_TimeLastLookMisc = gpGlobals->curtime;
		
		BeginGather();

		float distSq = ( iDistance * iDistance );
		const Vector &origin = GetAbsOrigin();
		int iter;
		CEntity *pEnt = CEntity::Instance(g_AI_SensedObjectsManager->GetFirst( &iter ));
		while ( pEnt )
		{
			if ( pEnt->GetFlags() & BOX_QUERY_MASK )
			{
				if ( origin.DistToSqr(pEnt->GetAbsOrigin()) < distSq && Look( pEnt->BaseEntity()) )
				{
					nSeen++;
				}
			}
			pEnt = CEntity::Instance(g_AI_SensedObjectsManager->GetNext( &iter ));
		}
		
		EndGather( nSeen, &m_SeenMisc );
	}
    else
    {
    	for ( int i = m_SeenMisc.Count() - 1; i >= 0; --i )
    	{
    		if ( m_SeenMisc[i].Get() == NULL )
    			m_SeenMisc.FastRemove( i );    			
    	}
    	nSeen = m_SeenMisc.Count();
    }

	return nSeen;
}

void CAI_Senses::EndGather( int nSeen, CUtlVector<EHANDLE> *pResult )
{
	pResult->SetCount( nSeen );
	if ( nSeen )
	{
		CBaseEntity *pCurrent = GetOuter()->m_pLink;
		for (int i = 0; i < nSeen; i++ )
		{
			Assert( pCurrent );
			(*pResult)[i].Set( pCurrent );
			pCurrent = CEntity::Instance(pCurrent)->m_pLink;
		}
		GetOuter()->m_pLink = NULL;
	}
}

void CAI_Senses::BeginGather()
{
	// clear my sight list
	GetOuter()->m_pLink = NULL;
}

int CAI_Senses::LookForNPCs( int iDistance )
{
	bool bRemoveStaleFromCache = false;
	float distSq = ( iDistance * iDistance );
	const Vector &origin = GetAbsOrigin();
	AI_Efficiency_t efficiency = GetOuter()->GetEfficiency();
	float timeNPCs = ( efficiency < AIE_VERY_EFFICIENT ) ? AI_STANDARD_NPC_SEARCH_TIME : AI_EFFICIENT_NPC_SEARCH_TIME;
	if ( gpGlobals->curtime - m_TimeLastLookNPCs > timeNPCs )
	{
		m_TimeLastLookNPCs = gpGlobals->curtime;

		if ( efficiency < AIE_SUPER_EFFICIENT )
		{
			int i, nSeen = 0;

			BeginGather();

			CAI_NPC **ppAIs = g_AI_Manager.AccessAIs();
			
			for ( i = 0; i < g_AI_Manager.NumAIs(); i++ )
			{
				if ( ppAIs[i] != GetOuter() && ( ppAIs[i]->ShouldNotDistanceCull() || origin.DistToSqr(ppAIs[i]->GetAbsOrigin()) < distSq ) )
				{
					if ( Look( ppAIs[i]->BaseEntity() ) )
					{
						nSeen++;
					}
				}
			}

			EndGather( nSeen, &m_SeenNPCs );

			return nSeen;
		}

		bRemoveStaleFromCache = true;
		// Fall through
	}

    for ( int i = m_SeenNPCs.Count() - 1; i >= 0; --i )
    {
    	if ( m_SeenNPCs[i].Get() == NULL )
		{
    		m_SeenNPCs.FastRemove( i );
		}
		else if ( bRemoveStaleFromCache )
		{
			CAI_NPC *npc = (CAI_NPC *)CEntity::Instance(m_SeenNPCs[i].Get());
			if ( ( !npc->ShouldNotDistanceCull() && origin.DistToSqr(npc->GetAbsOrigin()) > distSq ) ||
				 !Look( m_SeenNPCs[i] ) )
			{
	    		m_SeenNPCs.FastRemove( i );
			}
		}
    }

    return m_SeenNPCs.Count();
}

int CAI_Senses::LookForHighPriorityEntities( int iDistance )
{
	int nSeen = 0;
	if ( gpGlobals->curtime - m_TimeLastLookHighPriority > AI_HIGH_PRIORITY_SEARCH_TIME )
	{
		m_TimeLastLookHighPriority = gpGlobals->curtime;
		
		BeginGather();
	
		float distSq = ( iDistance * iDistance );
		const Vector &origin = GetAbsOrigin();
		
		// Players
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CPlayer *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer )
			{
				if ( origin.DistToSqr(pPlayer->GetAbsOrigin()) < distSq && Look( pPlayer->BaseEntity() ) )
				{
					nSeen++;
				}
			}
		}
	
		EndGather( nSeen, &m_SeenHighPriority );
    }
    else
    {
    	for ( int i = m_SeenHighPriority.Count() - 1; i >= 0; --i )
    	{
    		if ( m_SeenHighPriority[i].Get() == NULL )
    			m_SeenHighPriority.FastRemove( i );    			
    	}
    	nSeen = m_SeenHighPriority.Count();
    }
	
	return nSeen;
}

CSound* CAI_Senses::GetNextHeardSound( AISoundIter_t *pIter )
{
	if ( !*pIter )
		return NULL;

	int iCurrent = (int)*pIter;
	
	Assert( iCurrent != SOUNDLIST_EMPTY );
	if ( iCurrent == SOUNDLIST_EMPTY )
	{
		*pIter = NULL;
		return NULL;
	}
	
	iCurrent = CE_CSoundEnt::SoundPointerForIndex( iCurrent )->m_iNextAudible;
	if ( iCurrent == SOUNDLIST_EMPTY )
	{
		*pIter = NULL;
		return NULL;
	}
	
	*pIter = (AISoundIter_t)iCurrent;
	return CE_CSoundEnt::SoundPointerForIndex( iCurrent );
}

CSound* CAI_Senses::GetFirstHeardSound( AISoundIter_t *pIter )
{
	int iFirst = GetAudibleList(); 

	if ( iFirst == SOUNDLIST_EMPTY )
	{
		*pIter = NULL;
		return NULL;
	}
	
	*pIter = (AISoundIter_t)iFirst;
	return CE_CSoundEnt::SoundPointerForIndex( iFirst );
}

//-----------------------------------------------------------------------------
// Listen - npcs dig through the active sound list for
// any sounds that may interest them. (smells, too!)

void CAI_Senses::Listen( void )
{
	m_iAudibleList = SOUNDLIST_EMPTY; 

	int iSoundMask = GetOuter()->GetSoundInterests();
	
	if ( iSoundMask != SOUND_NONE && !(GetOuter()->HasSpawnFlags(SF_NPC_WAIT_TILL_SEEN)) )
	{
		int	iSound = CE_CSoundEnt::ActiveList();
		
		while ( iSound != SOUNDLIST_EMPTY )
		{
			CSound *pCurrentSound = CE_CSoundEnt::SoundPointerForIndex( iSound );

			if ( pCurrentSound	&& (iSoundMask & pCurrentSound->SoundType()) && CanHearSound( pCurrentSound ) )
			{
	 			// the npc cares about this sound, and it's close enough to hear.
				pCurrentSound->m_iNextAudible = m_iAudibleList;
				m_iAudibleList = iSound;
			}

			iSound = pCurrentSound->NextSound();
		}
	}
	
	GetOuter()->OnListened();
}

bool CAI_Senses::CanHearSound( CSound *pSound )
{
	if ( pSound->m_hOwner.Get() == GetOuter()->BaseEntity() )
		return false;

	if( GetOuter()->GetState() == NPC_STATE_SCRIPT && pSound->IsSoundType( SOUND_DANGER ) )
	{
		// For now, don't hear danger in scripted sequences. This probably isn't a
		// good long term solution, but it makes the Bank Exterior work better.
		return false;
	}

	if ( GetOuter()->IsInAScript() )
		return false;

	// @TODO (toml 10-18-02): what about player sounds and notarget?
	float flHearDistanceSq = pSound->Volume() * GetOuter()->HearingSensitivity();
	flHearDistanceSq *= flHearDistanceSq;
	if( pSound->GetSoundOrigin().DistToSqr( GetOuter()->EarPosition() ) <= flHearDistanceSq )
	{
		return GetOuter()->QueryHearSound( pSound );
	}

	return false;
}




CBaseEntity *CAI_SensedObjectsManager::GetFirst( int *pIter )
{
	if ( m_SensedObjects.Count() )
	{
		*pIter = 1;
		return m_SensedObjects[0];
	}
	
	*pIter = 0;
	return NULL;
}

CBaseEntity *CAI_SensedObjectsManager::GetNext( int *pIter )
{
	int i = *pIter;
	if ( i && i < m_SensedObjects.Count() )
	{
		(*pIter)++;
		return m_SensedObjects[i];
	}

	*pIter = 0;
	return NULL;
}