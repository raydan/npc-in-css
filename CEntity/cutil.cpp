/**
* =============================================================================
* CEntity Entity Handling Framework
* Copyright (C) 2011 Matt Woodrow.  All rights reserved.
* =============================================================================
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License, version 3.0, as published by the
* Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "extension.h"
#include "cutil.h"
#include "util.h"
#include <vphysics_interface.h>
#include "vphysics/object_hash.h"
#include "model_types.h"
#include "CPlayer.h"
#include "CAI_NPC.h"
#include "particle_parse.h"
#include "effect_dispatch_data.h"
#include "CE_recipientfilter.h"
#include "GameSystem.h"
#include "collisionutils.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int ENTINDEX( CBaseEntity *pEnt )
{
	CEntity *cent = CEntity::Instance(pEnt);
	// This works just like ENTINDEX for edicts.
	if ( cent )
		return cent->entindex();
	else
		return 0;
}

IChangeInfoAccessor *CBaseEdict::GetChangeAccessor()
{
	return engine->GetChangeAccessor( (const edict_t *)this );
}

bool IsValidEdict(edict_t *pEdict)
{
	if (!pEdict)
	{
		return false;
	}
	return pEdict->IsFree() ? false : true;
}

bool IsValidEntity(edict_t *pEdict)
{
	if(!pEdict)
	{
		return false;
	}

	IServerUnknown *pUnknown = pEdict->GetUnknown();
	if (!pUnknown)
	{
		return false;
	}
	CBaseEntity *pEntity = pUnknown->GetBaseEntity();
	return (pEntity != NULL) ? true : false;
}

const char *variant_t::ToString( void ) const
{
	COMPILE_TIME_ASSERT( sizeof(string_t) == sizeof(int) );

	static char szBuf[512];

	switch (fieldType)
	{
	case FIELD_STRING:
		{
			return(STRING(iszVal));
		}

	case FIELD_BOOLEAN:
		{
			if (bVal == 0)
			{
				Q_strncpy(szBuf, "false",sizeof(szBuf));
			}
			else
			{
				Q_strncpy(szBuf, "true",sizeof(szBuf));
			}
			return(szBuf);
		}

	case FIELD_INTEGER:
		{
			Q_snprintf( szBuf, sizeof( szBuf ), "%i", iVal );
			return(szBuf);
		}

	case FIELD_FLOAT:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "%g", flVal);
			return(szBuf);
		}

	case FIELD_COLOR32:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "%d %d %d %d", (int)rgbaVal.r, (int)rgbaVal.g, (int)rgbaVal.b, (int)rgbaVal.a);
			return(szBuf);
		}

	case FIELD_VECTOR:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "[%g %g %g]", (double)vecVal[0], (double)vecVal[1], (double)vecVal[2]);
			return(szBuf);
		}

	case FIELD_VOID:
		{
			szBuf[0] = '\0';
			return(szBuf);
		}

	case FIELD_EHANDLE:
		{
			const char *pszName = NULL;
			CEntity *pEnt = CEntity::Instance(eVal);
			if (pEnt)
			{
				pszName = pEnt->GetClassname();
			}
			else
			{
				pszName = "<<null entity>>";
			}

			Q_strncpy( szBuf, pszName, 512 );
			return (szBuf);
		}

	default:
		break;
	}

	return("No conversion to string");
}

void variant_t::Set( fieldtype_t ftype, void *data )
{
	fieldType = ftype;

	switch ( ftype )
	{
	case FIELD_BOOLEAN:		bVal = *((bool *)data);				break;
	case FIELD_CHARACTER:	iVal = *((char *)data);				break;
	case FIELD_SHORT:		iVal = *((short *)data);			break;
	case FIELD_INTEGER:		iVal = *((int *)data);				break;
	case FIELD_STRING:		iszVal = *((string_t *)data);		break;
	case FIELD_FLOAT:		flVal = *((float *)data);			break;
	case FIELD_COLOR32:		rgbaVal = *((color32 *)data);		break;

	case FIELD_VECTOR:
	case FIELD_POSITION_VECTOR:
		{
			vecVal[0] = ((float *)data)[0];
			vecVal[1] = ((float *)data)[1];
			vecVal[2] = ((float *)data)[2];
			break;
		}

	case FIELD_EHANDLE:		eVal = *((EHANDLE *)data);			break;
	case FIELD_CLASSPTR:	eVal = *((CBaseEntity **)data);		break;
	case FIELD_VOID:		
	default:
		iVal = 0; fieldType = FIELD_VOID;	
		break;
	}
}

void variant_t::SetOther( void *data )
{
	switch ( fieldType )
	{
	case FIELD_BOOLEAN:		*((bool *)data) = bVal != 0;		break;
	case FIELD_CHARACTER:	*((char *)data) = iVal;				break;
	case FIELD_SHORT:		*((short *)data) = iVal;			break;
	case FIELD_INTEGER:		*((int *)data) = iVal;				break;
	case FIELD_STRING:		*((string_t *)data) = iszVal;		break;
	case FIELD_FLOAT:		*((float *)data) = flVal;			break;
	case FIELD_COLOR32:		*((color32 *)data) = rgbaVal;		break;

	case FIELD_VECTOR:
	case FIELD_POSITION_VECTOR:
	{
		((float *)data)[0] = vecVal[0];
		((float *)data)[1] = vecVal[1];
		((float *)data)[2] = vecVal[2];
		break;
	}

	case FIELD_EHANDLE:		*((EHANDLE *)data) = eVal;			break;
	case FIELD_CLASSPTR:	*((CBaseEntity **)data) = eVal;		break;
	default:	break;
	}
}

bool variant_t::Convert( fieldtype_t newType )
{
	if ( newType == fieldType )
	{
		return true;
	}

	//
	// Converting to a null value is easy.
	//
	if ( newType == FIELD_VOID )
	{
		Set( FIELD_VOID, NULL );
		return true;
	}

	//
	// FIELD_INPUT accepts the variant type directly.
	//
	if ( newType == FIELD_INPUT )
	{
		return true;
	}

	switch ( fieldType )
	{
		case FIELD_INTEGER:
		{
			switch ( newType )
			{
				case FIELD_FLOAT:
				{
					SetFloat( (float) iVal );
					return true;
				}

				case FIELD_BOOLEAN:
				{
					SetBool( iVal != 0 );
					return true;
				}

				default:
					break;
			}
			break;

			default:
				break;
		}

		case FIELD_FLOAT:
		{
			switch ( newType )
			{
				case FIELD_INTEGER:
				{
					SetInt( (int) flVal );
					return true;
				}

				case FIELD_BOOLEAN:
				{
					SetBool( flVal != 0 );
					return true;
				}

				default:
					break;
			}
			break;
		}

		//
		// Everyone must convert from FIELD_STRING if possible, since
		// parameter overrides are always passed as strings.
		//
		case FIELD_STRING:
		{
			switch ( newType )
			{
				case FIELD_INTEGER:
				{
					if (iszVal != NULL_STRING)
					{
						SetInt(atoi(STRING(iszVal)));
					}
					else
					{
						SetInt(0);
					}
					return true;
				}

				case FIELD_FLOAT:
				{
					if (iszVal != NULL_STRING)
					{
						SetFloat(atof(STRING(iszVal)));
					}
					else
					{
						SetFloat(0);
					}
					return true;
				}

				case FIELD_BOOLEAN:
				{
					if (iszVal != NULL_STRING)
					{
						SetBool( atoi(STRING(iszVal)) != 0 );
					}
					else
					{
						SetBool(false);
					}
					return true;
				}

				case FIELD_VECTOR:
				{
					Vector tmpVec = vec3_origin;
					if (sscanf(STRING(iszVal), "[%f %f %f]", &tmpVec[0], &tmpVec[1], &tmpVec[2]) == 0)
					{
						// Try sucking out 3 floats with no []s
						sscanf(STRING(iszVal), "%f %f %f", &tmpVec[0], &tmpVec[1], &tmpVec[2]);
					}
					SetVector3D( tmpVec );
					return true;
				}

				case FIELD_COLOR32:
				{
					int nRed = 0;
					int nGreen = 0;
					int nBlue = 0;
					int nAlpha = 255;

					sscanf(STRING(iszVal), "%d %d %d %d", &nRed, &nGreen, &nBlue, &nAlpha);
					SetColor32( nRed, nGreen, nBlue, nAlpha );
					return true;
				}

				case FIELD_EHANDLE:
				{
					// convert the string to an entity by locating it by classname
					CBaseEntity *ent = NULL;
					if ( iszVal != NULL_STRING )
					{
						// FIXME: do we need to pass an activator in here?
						CEntity *cent = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, iszVal );
						ent = (cent)? cent->BaseEntity() : NULL;
					}
					SetEntity( ent );
					return true;
				}

				default:
					break;
			}
		
			break;
		}

		case FIELD_EHANDLE:
		{
			switch ( newType )
			{
				case FIELD_STRING:
				{
					// take the entities targetname as the string
					string_t iszStr = NULL_STRING;
					if ( eVal != NULL )
					{
						SetString( MAKE_STRING(CEntity::Instance(eVal)->GetEntityName()) );
					}
					return true;
				}

				default:
					break;
			}
			break;
		}
	}

	// invalid conversion
	return false;
}

void variant_t::SetEntity( CBaseEntity *val ) 
{ 
	eVal = val;
	fieldType = FIELD_EHANDLE; 
}



/**
 * This is the worst util ever, incredibly specific usage.
 * Searches a datamap for output types and swaps the SaveRestoreOps pointer for the global eventFuncs one.
 * Reason for this is we didn't have the eventFuncs pointer available statically (when the datamap structure was generated)
 */
void UTIL_PatchOutputRestoreOps(datamap_t *pMap)
{
	for (int i=0; i<pMap->dataNumFields; i++)
	{
		if (pMap->dataDesc[i].flags & FTYPEDESC_OUTPUT && pMap->dataDesc[i].pSaveRestoreOps == NULL)
		{
			pMap->dataDesc[i].pSaveRestoreOps = eventFuncs;
		}

		if (pMap->dataDesc[i].td)
		{
			UTIL_PatchOutputRestoreOps(pMap->dataDesc[i].td);
		}
	}
}

bool CGameTrace::DidHitWorld() const
{
	return m_pEnt == GetWorldEntity()->BaseEntity();
}

bool CGameTrace::DidHitNonWorldEntity() const
{
	return m_pEnt != NULL && !DidHitWorld();
}



//-----------------------------------------------------------------------------
//
// Shared client/server trace filter code
//
//-----------------------------------------------------------------------------
bool PassServerEntityFilter( const IHandleEntity *pTouch, const IHandleEntity *pPass ) 
{
	if ( !pPass )
		return true;

	if ( pTouch == pPass )
		return false;

	CEntity *pEntTouch = CE_EntityFromEntityHandle( pTouch );
	CEntity *pEntPass = CE_EntityFromEntityHandle( pPass );
	if ( !pEntTouch || !pEntPass )
		return true;

	// don't clip against own missiles
	if ( pEntTouch->GetOwnerEntity() == pEntPass )
		return false;
	
	// don't clip against owner
	if ( pEntPass->GetOwnerEntity() == pEntTouch )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// A standard filter to be applied to just about everything.
//-----------------------------------------------------------------------------
bool StandardFilterRules( IHandleEntity *pHandleEntity, int fContentsMask )
{
	CEntity *pCollide = CE_EntityFromEntityHandle( pHandleEntity );

	// Static prop case...
	if ( !pCollide )
		return true;

	SolidType_t solid = pCollide->GetSolid();
	const model_t *pModel = pCollide->GetModel();

	if ( ( modelinfo->GetModelType( pModel ) != mod_brush ) || (solid != SOLID_BSP && solid != SOLID_VPHYSICS) )
	{
		if ( (fContentsMask & CONTENTS_MONSTER) == 0 )
			return false;
	}

	// This code is used to cull out tests against see-thru entities
	if ( !(fContentsMask & CONTENTS_WINDOW) && pCollide->IsTransparent() )
		return false;

	// FIXME: this is to skip BSP models that are entities that can be 
	// potentially moved/deleted, similar to a monster but doors don't seem to 
	// be flagged as monsters
	// FIXME: the FL_WORLDBRUSH looked promising, but it needs to be set on 
	// everything that's actually a worldbrush and it currently isn't
	if ( !(fContentsMask & CONTENTS_MOVEABLE) && (pCollide->GetMoveType() == MOVETYPE_PUSH))// !(touch->flags & FL_WORLDBRUSH) )
		return false;

	return true;
}



class CTracePassFilter : public CTraceFilter
{
public:
	CTracePassFilter( IHandleEntity *pPassEnt ) : m_pPassEnt( pPassEnt ) {}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
			return false;

		if (!PassServerEntityFilter( pHandleEntity, m_pPassEnt ))
			return false;

		return true;
	}

private:
	IHandleEntity *m_pPassEnt;
};

bool CTraceFilterOnlyNPCsAndPlayer::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( CE_CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask ) )
	{
		CEntity *pEntity = CE_EntityFromEntityHandle( pHandleEntity );
		if ( !pEntity )
			return false;

//#ifdef CSTRIKE_DLL
		if ( pEntity->Classify() == CLASS_PLAYER_ALLY )
			return true; // CS hostages are CLASS_PLAYER_ALLY but not IsNPC()
//#endif // CSTRIKE_DLL
		return (pEntity->IsNPC() || pEntity->IsPlayer());
	}
	return false;
}


//-----------------------------------------------------------------------------
// Trace filter that can take a list of entities to ignore
//-----------------------------------------------------------------------------
CTraceFilterSimpleList::CTraceFilterSimpleList( int collisionGroup ) :
	CE_CTraceFilterSimple( NULL, collisionGroup )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTraceFilterSimpleList::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( m_PassEntities.Find(pHandleEntity) != m_PassEntities.InvalidIndex() )
		return false;

	return CE_CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}


//-----------------------------------------------------------------------------
// Purpose: Add an entity to my list of entities to ignore in the trace
//-----------------------------------------------------------------------------
void CTraceFilterSimpleList::AddEntityToIgnore( IHandleEntity *pEntity )
{
	m_PassEntities.AddToTail( pEntity );
}



//-----------------------------------------------------------------------------
// Sweep an entity from the starting to the ending position 
//-----------------------------------------------------------------------------
class CTraceFilterEntity : public CE_CTraceFilterSimple
{
	DECLARE_CLASS( CTraceFilterEntity, CE_CTraceFilterSimple );

public:
	CTraceFilterEntity( CBaseEntity *pEntity, int nCollisionGroup ) 
		: CE_CTraceFilterSimple( pEntity, nCollisionGroup )
	{
		m_pRootParent = CEntity::Instance(pEntity)->GetRootMoveParent();
		m_pEntity = CEntity::Instance(pEntity);
		m_checkHash = my_g_EntityCollisionHash->IsObjectInHash(pEntity);
	}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		CEntity *pEntity = CE_EntityFromEntityHandle( pHandleEntity );
		if ( !pEntity )
			return false;

		// Check parents against each other
		// NOTE: Don't let siblings/parents collide.
		if ( UTIL_EntityHasMatchingRootParent( m_pRootParent, pEntity ) )
			return false;

		if ( m_checkHash )
		{
			if ( my_g_EntityCollisionHash->IsObjectPairInHash( m_pEntity->BaseEntity(), pEntity->BaseEntity() ) )
				return false;
		}

		if ( m_pEntity->IsNPC() )
		{
			if ( NPC_CheckBrushExclude( m_pEntity, pEntity ) )
				 return false;

		}

		return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
	}

private:

	CEntity *m_pRootParent;
	CEntity *m_pEntity;
	bool		m_checkHash;
};

class CTraceFilterEntityIgnoreOther : public CTraceFilterEntity
{
	DECLARE_CLASS( CTraceFilterEntityIgnoreOther, CTraceFilterEntity );
public:
	CTraceFilterEntityIgnoreOther( CBaseEntity *pEntity, const IHandleEntity *pIgnore, int nCollisionGroup ) : 
		CTraceFilterEntity( pEntity, nCollisionGroup ), m_pIgnoreOther( pIgnore )
	{
	}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( pHandleEntity == m_pIgnoreOther )
			return false;

		return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
	}

private:
	const IHandleEntity *m_pIgnoreOther;
};




//-----------------------------------------------------------------------------
// Simple trace filter
//-----------------------------------------------------------------------------
CE_CTraceFilterSimple::CE_CTraceFilterSimple( const IHandleEntity *passedict, int collisionGroup )
{
	m_pPassEnt = passedict;
	m_collisionGroup = collisionGroup;
}

//-----------------------------------------------------------------------------
// The trace filter!
//-----------------------------------------------------------------------------
bool CE_CTraceFilterSimple::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
		return false;

	if ( m_pPassEnt )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
		{
			return false;
		}
	}

	// Don't test if the game code tells us we should ignore this collision...
	CEntity *pEntity = CE_EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity )
		return false;
	if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
		return false;
	if ( pEntity && !g_helpfunc.GameRules_ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
		return false;

	return true;
}


void UTIL_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, 
					 const IHandleEntity *ignore, int collisionGroup, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd );
	CE_CTraceFilterSimple traceFilter( ignore, collisionGroup );

	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );
}

void UTIL_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, 
					 ITraceFilter *pFilter, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd );

	enginetrace->TraceRay( ray, mask, pFilter, ptr );
}

void UTIL_TraceHull( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, ITraceFilter *pFilter, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, hullMin, hullMax );

	enginetrace->TraceRay( ray, mask, pFilter, ptr );
}


void UTIL_TraceHull( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, const IHandleEntity *ignore, 
					 int collisionGroup, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, hullMin, hullMax );
	CE_CTraceFilterSimple traceFilter( ignore, collisionGroup );

	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );

}

void UTIL_TraceEntity( CEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, unsigned int mask, trace_t *ptr )
{
	ICollideable *pCollision = pEntity->GetCollideable();

	// Adding this assertion here so game code catches it, but really the assertion belongs in the engine
	// because one day, rotated collideables will work!
	//Assert( pCollision->GetCollisionAngles() == vec3_angle );

	CTraceFilterEntity traceFilter( pEntity->BaseEntity(), pCollision->GetCollisionGroup() );

	enginetrace->SweepCollideable( pCollision, vecAbsStart, vecAbsEnd, pCollision->GetCollisionAngles(), mask, &traceFilter, ptr );
}

void UTIL_TraceEntity( CEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, const IHandleEntity *pIgnore, int nCollisionGroup, trace_t *ptr )
{
	ICollideable *pCollision;
	pCollision = pEntity->GetCollideable();

	// Adding this assertion here so game code catches it, but really the assertion belongs in the engine
	// because one day, rotated collideables will work!
	//Assert( pCollision->GetCollisionAngles() == vec3_angle );

	CTraceFilterEntityIgnoreOther traceFilter( pEntity->BaseEntity(), pIgnore, nCollisionGroup );

	enginetrace->SweepCollideable( pCollision, vecAbsStart, vecAbsEnd, pCollision->GetCollisionAngles(), mask, &traceFilter, ptr );
}

void UTIL_TraceLineFilterEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					   unsigned int mask, int nCollisionGroup, trace_t *ptr )
{
	CTraceFilterEntity traceFilter( pEntity, nCollisionGroup );
	UTIL_TraceLine( vecAbsStart, vecAbsEnd, mask, &traceFilter, ptr );
}

void UTIL_SetOrigin(CEntity *entity, const Vector &vecOrigin, bool bFireTriggers)
{
	entity->SetLocalOrigin( vecOrigin );
	if ( bFireTriggers )
	{
		entity->PhysicsTouchTriggers();
	}
}

bool UTIL_EntityHasMatchingRootParent( CEntity *pRootParent, CEntity *pEntity )
{
	if ( pRootParent )
	{
		// NOTE: Don't let siblings/parents collide.
		if ( pRootParent == pEntity->GetRootMoveParent() )
			return true;
		if ( pEntity->GetOwnerEntity() && pRootParent == pEntity->GetOwnerEntity()->GetRootMoveParent() )
			return true;
	}
	return false;
}

float UTIL_VecToYaw( const Vector &vec )
{
	if (vec.y == 0 && vec.x == 0)
		return 0;
	
	float yaw = atan2( vec.y, vec.x );

	yaw = RAD2DEG(yaw);

	if (yaw < 0)
		yaw += 360;

	return yaw;
}




// returns a CBaseEntity pointer to a player by index.  Only returns if the player is spawned and connected
// otherwise returns NULL
// Index is 1 based
CPlayer	*UTIL_PlayerByIndex( int playerIndex )
{
	CPlayer *pPlayer = NULL;

	if ( playerIndex > 0 && playerIndex <= gpGlobals->maxClients )
	{
		edict_t *pPlayerEdict = engine->PEntityOfEntIndex( playerIndex );
		if ( pPlayerEdict && !pPlayerEdict->IsFree() )
		{
			pPlayer = (CPlayer*)CEntity::Instance(pPlayerEdict);
		}
	}
	
	return pPlayer;
}


// computes gravity scale for an absolute gravity.  Pass the result into CBaseEntity::SetGravity()
float UTIL_ScaleForGravity( float desiredGravity )
{
	float worldGravity = sv_gravity->GetFloat();
	return worldGravity > 0 ? desiredGravity / worldGravity : 0;
}


void UTIL_DecalTrace( trace_t *pTrace, char const *decalName )
{
	if (pTrace->fraction == 1.0)
		return;

	CEntity *pEntity = CEntity::Instance(pTrace->m_pEnt);
	assert(pEntity);
	pEntity->DecalTrace( pTrace, decalName );
}

CEntitySphereQuery::CEntitySphereQuery( const Vector &center, float radius, int flagMask )
{
	m_listIndex = 0;
	m_listCount = UTIL_EntitiesInSphere( m_pList, ARRAYSIZE(m_pList), center, radius, flagMask );
}

CEntity *CEntitySphereQuery::GetCurrentEntity()
{
	if ( m_listIndex < m_listCount )
		return CEntity::Instance(m_pList[m_listIndex]);
	return NULL;
}


//-----------------------------------------------------------------------------
// class CFlaggedEntitiesEnum
//-----------------------------------------------------------------------------

CFlaggedEntitiesEnum::CFlaggedEntitiesEnum( CBaseEntity **pList, int listMax, int flagMask )
{
	m_pList = pList;
	m_listMax = listMax;
	m_flagMask = flagMask;
	m_count = 0;
}

bool CFlaggedEntitiesEnum::AddToList( CBaseEntity *pEntity )
{
	if ( m_count >= m_listMax )
	{
		AssertMsgOnce( 0, "reached enumerated list limit.  Increase limit, decrease radius, or make it so entity flags will work for you" );
		return false;
	}
	m_pList[m_count] = pEntity;
	m_count++;
	return true;
}

bool CFlaggedEntitiesEnum::AddToList( CEntity *pEntity )
{
	Assert(pEntity->BaseEntity());
	return AddToList((pEntity) ? pEntity->BaseEntity() : NULL);
}

IterationRetval_t CFlaggedEntitiesEnum::EnumElement( IHandleEntity *pHandleEntity )
{
	CEntity *pEntity = CE_EntityFromEntityHandle( pHandleEntity);
	if ( pEntity )
	{
		if ( m_flagMask && !(pEntity->GetFlags() & m_flagMask) )	// Does it meet the criteria?
			return ITERATION_CONTINUE;

		if ( !AddToList( pEntity ) )
			return ITERATION_STOP;
	}

	return ITERATION_CONTINUE;
}


//-----------------------------------------------------------------------------
// Compute shake amplitude
//-----------------------------------------------------------------------------
inline float ComputeShakeAmplitude( const Vector &center, const Vector &shakePt, float amplitude, float radius ) 
{
	if ( radius <= 0 )
		return amplitude;

	float localAmplitude = -1;
	Vector delta = center - shakePt;
	float distance = delta.Length();

	if ( distance <= radius )
	{
		// Make the amplitude fall off over distance
		float flPerc = 1.0 - (distance / radius);
		localAmplitude = amplitude * flPerc;
	}

	return localAmplitude;
}

void UTIL_ScreenShake(int players[], int total, float localAmplitude, float frequency, float duration, ShakeCommand_t eCommand)
{
	static int shake_id = -1;
	if(shake_id == -1)
	{
		shake_id = usermsgs->GetMessageIndex("Shake");
	}

	if (( localAmplitude > 0 ) || ( eCommand == SHAKE_STOP ))
	{
		if ( eCommand == SHAKE_STOP )
			localAmplitude = 0;

		bf_write *pBitBuf = usermsgs->StartBitBufMessage(shake_id, players, total, USERMSG_RELIABLE);
		if(pBitBuf == NULL) return; 

		pBitBuf->WriteByte(eCommand);
		pBitBuf->WriteFloat(localAmplitude);
		pBitBuf->WriteFloat(frequency);
		pBitBuf->WriteFloat(duration);
		usermsgs->EndMessage();
	}
}


const float MAX_SHAKE_AMPLITUDE = 16.0f;
void UTIL_ScreenShake( const Vector &center, float amplitude, float frequency, float duration, float radius, ShakeCommand_t eCommand, bool bAirShake )
{
	int			i;
	float		localAmplitude;

	if ( amplitude > MAX_SHAKE_AMPLITUDE )
	{
		amplitude = MAX_SHAKE_AMPLITUDE;
	}
	
	int players[128];
	int total = 0;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CPlayer *pPlayer = UTIL_PlayerByIndex( i );

		//
		// Only start shakes for players that are on the ground unless doing an air shake.
		//
		if ( !pPlayer || (!bAirShake && (eCommand == SHAKE_START) && !(pPlayer->GetFlags() & FL_ONGROUND)) )
		{
			continue;
		}

		localAmplitude = ComputeShakeAmplitude( center, pPlayer->WorldSpaceCenter(), amplitude, radius );

		// This happens if the player is outside the radius, in which case we should ignore 
		// all commands
		if (localAmplitude < 0)
			continue;

		players[total] = i;
		total++;
	}

	if(total > 0)
		UTIL_ScreenShake(players, total, localAmplitude, frequency, duration, eCommand );
}

void UTIL_CreateDust(const Vector &pos, const QAngle &angle, float size, float speed)
{
	Vector dir;
	AngleVectors(angle, &dir);
	CPVSFilter filter( pos );
	te->Dust(filter, 0.0f, pos, dir, size, speed);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int UTIL_EntitiesInBox( const Vector &mins, const Vector &maxs, CFlaggedEntitiesEnum *pEnum )
{
	partition->EnumerateElementsInBox( PARTITION_ENGINE_NON_STATIC_EDICTS, mins, maxs, false, pEnum );
	return pEnum->GetCount();
}


int UTIL_EntitiesAlongRay( const Ray_t &ray, CFlaggedEntitiesEnum *pEnum )
{
	partition->EnumerateElementsAlongRay( PARTITION_ENGINE_NON_STATIC_EDICTS, ray, false, pEnum );
	return pEnum->GetCount();
}

int UTIL_EntitiesInSphere( const Vector &center, float radius, CFlaggedEntitiesEnum *pEnum )
{
	partition->EnumerateElementsInSphere( PARTITION_ENGINE_NON_STATIC_EDICTS, center, radius, false, pEnum );
	return pEnum->GetCount();
}


bool UTIL_ShouldShowBlood( int color )
{
	if ( color != DONT_BLEED )
	{
		return true;
	}
	return false;
}

void UTIL_BloodDrips( const Vector &origin, const Vector &direction, int color, int amount )
{
	//fixed!!
	IPredictionSystem::SuppressHostEvents(NULL);

	if ( !UTIL_ShouldShowBlood( color ) )
		return;

	if ( color == DONT_BLEED || amount == 0 )
		return;

	amount *= 5;

	if ( amount > 255 )
		amount = 255;

	if (color == BLOOD_COLOR_MECH)
	{
		g_pEffects->Sparks(origin);
		if (enginerandom->RandomFloat(0, 2) >= 1)
		{
			UTIL_Smoke(origin, enginerandom->RandomInt(10, 15), 10);
		}
	}
	else
	{
		// Normal blood impact
		UTIL_BloodImpact( origin, direction, color, amount );
	}
}	

extern short g_sModelIndexSmoke;
void UTIL_Smoke( const Vector &origin, const float scale, const float framerate )
{
	g_pEffects->Smoke( origin, g_sModelIndexSmoke, scale, framerate );
}

void UTIL_BloodImpact( const Vector &pos, const Vector &dir, int color, int amount )
{
	CEffectData	data;

	data.m_vOrigin = pos;
	data.m_vNormal = dir;
	data.m_flScale = (float)amount;
	data.m_nColor = (unsigned char)color;

	g_helpfunc.DispatchEffect("bloodimpact", data );
}

void UTIL_ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	CEntity *pEntity = CEntity::Instance(pTrace->m_pEnt);

	// Is the entity valid, is the surface sky?
	if ( !pEntity || !UTIL_IsValidEntity( pEntity ) || (pTrace->surface.flags & SURF_SKY) )
		return;

	if ( pTrace->fraction == 1.0 )
		return;

	pEntity->ImpactTrace( pTrace, iDamageType, pCustomImpactName );
}

bool UTIL_IsValidEntity( CEntity *pEnt )
{
	if(!pEnt)
		return false;
	edict_t *pEdict = pEnt->edict();
	if ( !pEdict || pEdict->IsFree() )
		return false;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Spawn some blood particles
//-----------------------------------------------------------------------------
void SpawnBlood(Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage)
{
	UTIL_BloodDrips( vecSpot, vecDir, bloodColor, (int)flDamage );
}

//-----------------------------------------------------------------------------
// Purpose: Slightly modified strtok. Does not modify the input string. Does
//			not skip over more than one separator at a time. This allows parsing
//			strings where tokens between separators may or may not be present:
//
//			Door01,,,0 would be parsed as "Door01"  ""  ""  "0"
//			Door01,Open,,0 would be parsed as "Door01"  "Open"  ""  "0"
//
// Input  : token - Returns with a token, or zero length if the token was missing.
//			str - String to parse.
//			sep - Character to use as separator. UNDONE: allow multiple separator chars
// Output : Returns a pointer to the next token to be parsed.
//-----------------------------------------------------------------------------
const char *nexttoken(char *token, const char *str, char sep)
{
	if ((str == NULL) || (*str == '\0'))
	{
		*token = '\0';
		return(NULL);
	}

	//
	// Copy everything up to the first separator into the return buffer.
	// Do not include separators in the return buffer.
	//
	while ((*str != sep) && (*str != '\0'))
	{
		*token++ = *str++;
	}
	*token = '\0';

	//
	// Advance the pointer unless we hit the end of the input string.
	//
	if (*str == '\0')
	{
		return(str);
	}

	return(++str);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void UTIL_BloodSpray( const Vector &pos, const Vector &dir, int color, int amount, int flags )
{
	if( color == DONT_BLEED )
		return;

	CEffectData	data;

	data.m_vOrigin = pos;
	data.m_vNormal = dir;
	data.m_flScale = (float)amount;
	data.m_fFlags = flags;
	data.m_nColor = color;

	g_helpfunc.DispatchEffect("bloodspray", data );
}

void UTIL_BloodDecalTrace( trace_t *pTrace, int bloodColor )
{
	if ( UTIL_ShouldShowBlood( bloodColor ) )
	{
		if ( bloodColor == BLOOD_COLOR_RED )
		{
			UTIL_DecalTrace( pTrace, "Blood" );
		}
		else
		{
			UTIL_DecalTrace( pTrace, "YellowBlood" );
		}
	}
}

extern ConVar *violence_hblood;
extern ConVar *violence_ablood;
extern ConVar *violence_hgibs;
extern ConVar *violence_agibs;

bool UTIL_IsLowViolence( void )
{
	// These convars are no longer necessary -- the engine is the final arbiter of
	// violence settings -- but they're here for legacy support and for testing low
	// violence when the engine is in normal violence mode.
	if ( !violence_hblood->GetBool() || !violence_ablood->GetBool() || !violence_hgibs->GetBool() || !violence_agibs->GetBool() )
		return true;

	return engine->IsLowViolence();
}

Vector UTIL_YawToVector( float yaw )
{
	Vector ret;
	
	ret.z = 0;
	float angle = DEG2RAD( yaw );
	SinCos( angle, &ret.y, &ret.x );

	return ret;
}

CEntity *CreateRagGib( const char *szModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecForce, float flFadeTime, bool bShouldIgnite )
{
	return g_helpfunc.CreateRagGib(szModel,vecOrigin, vecAngles,vecForce, flFadeTime, bShouldIgnite);
}

void UTIL_SetSize( CBaseEntity *pEnt, const Vector &vecMin, const Vector &vecMax )
{
	g_helpfunc.SetMinMaxSize (pEnt, vecMin, vecMax);
}

void StopSoundByHandle( int entindex, const char *soundname, HSOUNDSCRIPTHANDLE& handle )
{
	if ( handle == SOUNDEMITTER_INVALID_HANDLE )
	{
		handle = (HSOUNDSCRIPTHANDLE)soundemitterbase->GetSoundIndex( soundname );
	}

	if ( handle == SOUNDEMITTER_INVALID_HANDLE )
		return;

	CSoundParametersInternal *params;

	params = soundemitterbase->InternalGetParametersForSound( (int)handle );
	if ( !params )
	{
		return;
	}

	// HACK:  we have to stop all sounds if there are > 1 in the rndwave section...
	int c = params->NumSoundNames();
	for ( int i = 0; i < c; ++i )
	{
		char const *wavename = soundemitterbase->GetWaveName( params->GetSoundNames()[ i ].symbol );
		Assert( wavename );

		enginesound->StopSound( 
			entindex, 
			params->GetChannel(), 
			wavename );

	}
}

void Sound_StopSound( int entindex, const char *soundname )
{
	HSOUNDSCRIPTHANDLE handle = (HSOUNDSCRIPTHANDLE)soundemitterbase->GetSoundIndex( soundname );
	if ( handle == SOUNDEMITTER_INVALID_HANDLE )
	{
		return;
	}
	StopSoundByHandle( entindex, soundname, handle );
}

void Sound_StopSound( int iEntIndex, int iChannel, const char *pSample )
{
	if ( pSample && ( Q_stristr( pSample, ".wav" ) || Q_stristr( pSample, ".mp3" ) || pSample[0] == '!' ) )
	{
		enginesound->StopSound( iEntIndex, iChannel, pSample );
	} else {
		// Look it up in sounds.txt and ignore other parameters
		Sound_StopSound( iEntIndex, pSample );
	}
}

bool UTIL_CheckBottom( CEntity *pEntity, ITraceFilter *pTraceFilter, float flStepSize )
{
	Vector	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;

	Assert( pEntity );

	CTracePassFilter traceFilter(pEntity->BaseEntity());
	if ( !pTraceFilter )
	{
		pTraceFilter = &traceFilter;
	}

	unsigned int mask = pEntity->PhysicsSolidMaskForEntity();

	VectorAdd (pEntity->GetAbsOrigin(), pEntity->WorldAlignMins(), mins);
	VectorAdd (pEntity->GetAbsOrigin(), pEntity->WorldAlignMaxs(), maxs);

	// if all of the points under the corners are solid world, don't bother
	// with the tougher checks
	// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for	(x=0 ; x<=1 ; x++)
	{
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (enginetrace->GetPointContents(start) != CONTENTS_SOLID)
				goto realcheck;
		}
	}
	return true;		// we got out easy

realcheck:
	// check it for real...
	start[2] = mins[2] + flStepSize; // seems to help going up/down slopes.
	
	// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5;
	stop[2] = start[2] - 2*flStepSize;
	
	UTIL_TraceLine( start, stop, mask, pTraceFilter, &trace );

	if (trace.fraction == 1.0)
		return false;
	mid = bottom = trace.endpos[2];

	// the corners must be within 16 of the midpoint	
	for	(x=0 ; x<=1 ; x++)
	{
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];
			
			UTIL_TraceLine( start, stop, mask, pTraceFilter, &trace );
			
			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > flStepSize)
				return false;
		}
	}
	return true;
}


void PropBreakableCreateAll( int modelindex, IPhysicsObject *pPhysics, const breakablepropparams_t &params, CBaseEntity *pEntity, int iPrecomputedBreakableCount, bool bIgnoreGibLimit, bool defaultLocation )
{
	g_helpfunc.PropBreakableCreateAll(modelindex, pPhysics, params, pEntity, iPrecomputedBreakableCount, bIgnoreGibLimit, defaultLocation);
}

void PropBreakableCreateAll( int modelindex, IPhysicsObject *pPhysics, const Vector &origin, const QAngle &angles, const Vector &velocity, const AngularImpulse &angularVelocity, float impactEnergyScale, float defBurstScale, int defCollisionGroup, CBaseEntity *pEntity, bool defaultLocation )
{
	breakablepropparams_t params( origin, angles, velocity, angularVelocity );
	params.impactEnergyScale = impactEnergyScale;
	params.defBurstScale = defBurstScale;
	params.defCollisionGroup = defCollisionGroup;
	PropBreakableCreateAll( modelindex, pPhysics, params, pEntity, -1, false, defaultLocation );
}

int PropBreakablePrecacheAll( string_t modelName )
{
	return g_helpfunc.PropBreakablePrecacheAll(modelName);
}


void UTIL_Remove(CEntity *pEntity)
{
	if(!pEntity)
		return;
	UTIL_Remove(pEntity->BaseEntity());
}

void UTIL_Remove(CBaseEntity *oldObj)
{
	servertools->RemoveEntity(oldObj);
}

void UTIL_RemoveImmediate(CEntity *pEntity)
{
	if(!pEntity)
		return;
	UTIL_RemoveImmediate(pEntity->BaseEntity());
}

void UTIL_RemoveImmediate(CBaseEntity *oldObj)
{
	servertools->RemoveEntityImmediate(oldObj);
}

CPlayer *UTIL_GetNearestPlayer( const Vector &origin )
{
    float distToNearest = 999999.0f;
    CPlayer *pNearest = NULL;

    for (int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        CPlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer || !pPlayer->IsAlive())
            continue;

        float flDist = (pPlayer->GetAbsOrigin() - origin).LengthSqr();
        if ( flDist < distToNearest )

        {
            pNearest = pPlayer;
            distToNearest = flDist;

        }
    }
    return pNearest;
}

CPlayer *UTIL_GetNearestVisiblePlayer(CEntity *pLooker, int mask)
{
	float distToNearest = 999999.0f;
    CPlayer *pNearest = NULL;

    for (int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        CPlayer *pPlayer = UTIL_PlayerByIndex( i );
        if ( !pPlayer  || !pPlayer->IsAlive())
            continue;

        float flDist = (pPlayer->GetAbsOrigin() - pLooker->GetAbsOrigin()).LengthSqr();
		if ( flDist < distToNearest && pLooker->FVisible_Entity( pPlayer->BaseEntity(), mask ) )
        {
            pNearest = pPlayer;
            distToNearest = flDist;
        }  
    }

    return pNearest;
}

float UTIL_VecToPitch( const Vector &vec )
{
	if (vec.y == 0 && vec.x == 0)
	{
		if (vec.z < 0)
			return 180.0;
		else
			return -180.0;
	}

	float dist = vec.Length2D();
	float pitch = atan2( -vec.z, dist );

	pitch = RAD2DEG(pitch);

	return pitch;
}

//-----------------------------------------------------------------------------
// Purpose: Get the predicted postion of an entity of a certain number of seconds
//			Use this function with caution, it has great potential for annoying the player, especially
//			if used for target firing predition
// Input  : *pTarget - target entity to predict
//			timeDelta - amount of time to predict ahead (in seconds)
//			&vecPredictedPosition - output
//-----------------------------------------------------------------------------
void UTIL_PredictedPosition( CEntity *pTarget, float flTimeDelta, Vector *vecPredictedPosition )
{
	if ( ( pTarget == NULL ) || ( vecPredictedPosition == NULL ) )
		return;

	Vector	vecPredictedVel;

	//FIXME: Should we look at groundspeed or velocity for non-clients??

	//Get the proper velocity to predict with
	CPlayer	*pPlayer = ToBasePlayer( pTarget );

	//Player works differently than other entities
	if ( pPlayer != NULL )
	{
		if ( pPlayer->IsInAVehicle() )
		{
			//Calculate the predicted position in this vehicle
			vecPredictedVel = CEntity::Instance(pPlayer->GetVehicleEntity())->GetSmoothedVelocity();
		}
		else
		{
			//Get the player's stored velocity
			vecPredictedVel = pPlayer->GetSmoothedVelocity();
		}
	}
	else
	{
		// See if we're a combat character in a vehicle
		CCombatCharacter *pCCTarget = pTarget->MyCombatCharacterPointer();
		if ( pCCTarget != NULL && pCCTarget->IsInAVehicle() )
		{
			//Calculate the predicted position in this vehicle
			vecPredictedVel = CEntity::Instance(pCCTarget->GetVehicleEntity())->GetSmoothedVelocity();
		}
		else
		{
			// See if we're an animating entity
			CAnimating *pAnimating = dynamic_cast<CAnimating *>(pTarget);
			if ( pAnimating != NULL )
			{
				vecPredictedVel = pAnimating->GetGroundSpeedVelocity();
			}
			else
			{
				// Otherwise we're a vanilla entity
				vecPredictedVel = pTarget->GetSmoothedVelocity();				
			}
		}
	}

	//Get the result
	(*vecPredictedPosition) = pTarget->GetAbsOrigin() + ( vecPredictedVel * flTimeDelta );
}

static unsigned short FixedUnsigned16( float value, float scale )
{
	int output;

	output = (int)(value * scale);
	if ( output < 0 )
		output = 0;
	if ( output > 0xFFFF )
		output = 0xFFFF;

	return (unsigned short)output;
}

void UTIL_ScreenFadeBuild( ScreenFade_t &fade, const color32 &color, float fadeTime, float fadeHold, int flags )
{
	fade.duration = FixedUnsigned16( fadeTime, 1<<SCREENFADE_FRACBITS );		// 7.9 fixed
	fade.holdTime = FixedUnsigned16( fadeHold, 1<<SCREENFADE_FRACBITS );		// 7.9 fixed
	fade.r = color.r;
	fade.g = color.g;
	fade.b = color.b;
	fade.a = color.a;
	fade.fadeFlags = flags;
}

void UTIL_ScreenFadeWrite( const ScreenFade_t &fade, CEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsNetClient() )
		return;

	static int fade_id = -1;
	if(fade_id == -1)
	{
		fade_id = usermsgs->GetMessageIndex("Fade");
	}

	int players[1] = {pEntity->entindex()};
	bf_write *pBitBuf = usermsgs->StartBitBufMessage(fade_id, players, 1, USERMSG_RELIABLE);
	if(pBitBuf == NULL) return;
	
	pBitBuf->WriteShort(fade.duration);
	pBitBuf->WriteShort(fade.holdTime);
	pBitBuf->WriteShort(fade.fadeFlags);
	pBitBuf->WriteByte(fade.r);
	pBitBuf->WriteByte(fade.g);
	pBitBuf->WriteByte(fade.b);
	pBitBuf->WriteByte(fade.a);
	usermsgs->EndMessage();
}

void UTIL_ScreenFade( CEntity *pEntity, const color32 &color, float fadeTime, float fadeHold, int flags )
{
	ScreenFade_t	fade;

	UTIL_ScreenFadeBuild( fade, color, fadeTime, fadeHold, flags );
	UTIL_ScreenFadeWrite( fade, pEntity );
}

float UTIL_WaterLevel( const Vector &position, float minz, float maxz )
{
	Vector midUp = position;
	midUp.z = minz;

	if ( !(UTIL_PointContents(midUp) & MASK_WATER) )
		return minz;

	midUp.z = maxz;
	if ( UTIL_PointContents(midUp) & MASK_WATER )
		return maxz;

	float diff = maxz - minz;
	while (diff > 1.0)
	{
		midUp.z = minz + diff/2.0;
		if ( UTIL_PointContents(midUp) & MASK_WATER )
		{
			minz = midUp.z;
		}
		else
		{
			maxz = midUp.z;
		}
		diff = maxz - minz;
	}

	return midUp.z;
}


extern short	g_sModelIndexBubbles;

void UTIL_Bubbles( const Vector& mins, const Vector& maxs, int count )
{
	Vector mid =  (mins + maxs) * 0.5;

	float flHeight = UTIL_WaterLevel( mid,  mid.z, mid.z + 1024 );
	flHeight = flHeight - mins.z;

	CPASFilter filter( mid );

	te->Bubbles( filter, 0.0,
		&mins, &maxs, flHeight, g_sModelIndexBubbles, count, 8.0 );
}

void UTIL_BubbleTrail( const Vector& from, const Vector& to, int count )
{
	// Find water surface will return from.z if the from point is above water
	float flStartHeight = UTIL_FindWaterSurface( from, from.z, from.z + 256 );
	flStartHeight = flStartHeight - from.z;

	float flEndHeight = UTIL_FindWaterSurface( to, to.z, to.z + 256 );
	flEndHeight = flEndHeight - to.z;

	if ( ( flStartHeight == 0 ) && ( flEndHeight == 0 ) )
		return;

	float flWaterZ = flStartHeight + from.z;

	const Vector *pFrom = &from;
	const Vector *pTo = &to;
	Vector vecWaterPoint;
	if ( ( flStartHeight == 0 ) || ( flEndHeight == 0 ) )
	{
		if ( flStartHeight == 0 )
		{
			flWaterZ = flEndHeight + to.z;
		}

		float t = IntersectRayWithAAPlane( from, to, 2, 1.0f, flWaterZ );
		Assert( (t >= -1e-3f) && ( t <= 1.0f ) );
		VectorLerp( from, to, t, vecWaterPoint );
		if ( flStartHeight == 0 )
		{
			pFrom = &vecWaterPoint;

			// Reduce the count by the actual length
			count = (int)( count * ( 1.0f - t ) );
		}
		else
		{
			pTo = &vecWaterPoint;

			// Reduce the count by the actual length
			count = (int)( count * t );
		}
	}

	CBroadcastRecipientFilter filter;
	te->BubbleTrail( filter, 0.0, pFrom, pTo, flWaterZ, g_sModelIndexBubbles, count, 8.0 );
}


class CWaterTraceFilter : public CTraceFilter
{
public:
	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		CEntity *pCollide = CE_EntityFromEntityHandle( pHandleEntity );

		// Static prop case...
		if ( !pCollide )
			return false;

		// Only impact water stuff...
		if ( pCollide->GetSolidFlags() & FSOLID_VOLUME_CONTENTS )
			return true;

		return false;
	}
};


float UTIL_FindWaterSurface( const Vector &position, float minz, float maxz )
{
	Vector vecStart, vecEnd;
	vecStart.Init( position.x, position.y, maxz );
	vecEnd.Init( position.x, position.y, minz );

	Ray_t ray;
	trace_t tr;
	CWaterTraceFilter waterTraceFilter;
	ray.Init( vecStart, vecEnd );
	enginetrace->TraceRay( ray, MASK_WATER, &waterTraceFilter, &tr );

	return tr.endpos.z;
}

void UTIL_StringToVector( float *pVector, const char *pString )
{
	UTIL_StringToFloatArray( pVector, 3, pString );
}

void UTIL_StringToColor32( color32 *color, const char *pString )
{
	int tmp[4];
	UTIL_StringToIntArray( tmp, 4, pString );
	color->r = tmp[0];
	color->g = tmp[1];
	color->b = tmp[2];
	color->a = tmp[3];
}

void UTIL_StringToFloatArray( float *pVector, int count, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int	j;

	Q_strncpy( tempString, pString, sizeof(tempString) );
	pstr = pfront = tempString;

	for ( j = 0; j < count; j++ )			// lifted from pr_edict.c
	{
		pVector[j] = atof( pfront );

		// skip any leading whitespace
		while ( *pstr && *pstr <= ' ' )
			pstr++;

		// skip to next whitespace
		while ( *pstr && *pstr > ' ' )
			pstr++;

		if (!*pstr)
			break;

		pstr++;
		pfront = pstr;
	}
	for ( j++; j < count; j++ )
	{
		pVector[j] = 0;
	}
}


void UTIL_StringToIntArray( int *pVector, int count, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int	j;

	Q_strncpy( tempString, pString, sizeof(tempString) );
	pstr = pfront = tempString;

	for ( j = 0; j < count; j++ )			// lifted from pr_edict.c
	{
		pVector[j] = atoi( pfront );

		while ( *pstr && *pstr != ' ' )
			pstr++;
		if (!*pstr)
			break;
		pstr++;
		pfront = pstr;
	}

	for ( j++; j < count; j++ )
	{
		pVector[j] = 0;
	}
}

char *UTIL_VarArgs( const char *format, ... )
{
	va_list		argptr;
	static char		string[1024];
	
	va_start (argptr, format);
	Q_vsnprintf(string, sizeof(string), format,argptr);
	va_end (argptr);

	return string;	
}


EmitSound_t::EmitSound_t( const CSoundParameters &src )
{
	m_nChannel = src.channel;
	m_pSoundName = src.soundname;
	m_flVolume = src.volume;
	m_SoundLevel = src.soundlevel;
	m_nFlags = 0;
	m_nPitch = src.pitch;
	m_nSpecialDSP = 0;
	m_pOrigin = 0;
	m_flSoundTime = ( src.delay_msec == 0 ) ? 0.0f : gpGlobals->curtime + ( (float)src.delay_msec / 1000.0f );
	m_pflSoundDuration = 0;
	m_bEmitCloseCaption = true;
	m_bWarnOnMissingCloseCaption = false;
	m_bWarnOnDirectWaveReference = false;
	m_nSpeakerEntity = -1;
}

int UTIL_DropToFloor( CEntity *pEntity, unsigned int mask, CEntity *pIgnore)
{
	if(!pEntity)
		return -1;

	CBaseEntity *cbase_pIgnore = (pIgnore) ? pIgnore->BaseEntity() : NULL;

	// Assume no ground
	pEntity->SetGroundEntity( NULL );

	trace_t	trace;
	// HACK: is this really the only sure way to detect crossing a terrain boundry?
	UTIL_TraceEntity( pEntity, pEntity->GetAbsOrigin(), pEntity->GetAbsOrigin(), mask, cbase_pIgnore, pEntity->GetCollisionGroup(), &trace );
	if (trace.fraction == 0.0)
		return -1;

	UTIL_TraceEntity( pEntity, pEntity->GetAbsOrigin(), pEntity->GetAbsOrigin() - Vector(0,0,256), mask, cbase_pIgnore, pEntity->GetCollisionGroup(), &trace );

	if (trace.allsolid)
		return -1;

	if (trace.fraction == 1)
		return 0;

	pEntity->SetAbsOrigin( trace.endpos );
	pEntity->SetGroundEntity( trace.m_pEnt );

	return 1;
}

Vector UTIL_PointOnLineNearestPoint(const Vector& vStartPos, const Vector& vEndPos, const Vector& vPoint, bool clampEnds )
{
	Vector	vEndToStart		= (vEndPos - vStartPos);
	Vector	vOrgToStart		= (vPoint  - vStartPos);
	float	fNumerator		= DotProduct(vEndToStart,vOrgToStart);
	float	fDenominator	= vEndToStart.Length() * vOrgToStart.Length();
	float	fIntersectDist	= vOrgToStart.Length()*(fNumerator/fDenominator);
	float	flLineLength	= VectorNormalize( vEndToStart ); 
	
	if ( clampEnds )
	{
		fIntersectDist = clamp( fIntersectDist, 0.0f, flLineLength );
	}
	
	Vector	vIntersectPos	= vStartPos + vEndToStart * fIntersectDist;

	return vIntersectPos;
}


//-----------------------------------------------------------------------------
// Purpose: Used to tell whether an item may be picked up by the player.  This
//			accounts for solid obstructions being in the way.
// Input  : *pItem - item in question
//			*pPlayer - player attempting the pickup
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_ItemCanBeTouchedByPlayer( CEntity *pItem, CPlayer *pPlayer )
{
	if ( pItem == NULL || pPlayer == NULL )
		return false;

	// For now, always allow a vehicle riding player to pick up things they're driving over
	if ( pPlayer->IsInAVehicle() )
		return true;

	// Get our test positions
	Vector vecStartPos;
	IPhysicsObject *pPhysObj = pItem->VPhysicsGetObject();
	if ( pPhysObj != NULL )
	{
		// Use the physics hull's center
		QAngle vecAngles;
		pPhysObj->GetPosition( &vecStartPos, &vecAngles );
	}
	else
	{
		// Use the generic bbox center
		vecStartPos = pItem->WorldSpaceCenter();
	}

	Vector vecEndPos = pPlayer->EyePosition();

	// FIXME: This is the simple first try solution towards the problem.  We need to take edges and shape more into account
	//		  for this to be fully robust.

	// Trace between to see if we're occluded
	trace_t tr;
	CTraceFilterSkipTwoEntities filter( pPlayer->BaseEntity(), pItem->BaseEntity(), COLLISION_GROUP_PLAYER_MOVEMENT );
	UTIL_TraceLine( vecStartPos, vecEndPos, MASK_SOLID, &filter, &tr );

	// Occluded
	// FIXME: For now, we exclude starting in solid because there are cases where this doesn't matter
	if ( tr.fraction < 1.0f )
		return false;

	return true;
}

AngularImpulse WorldToLocalRotation( const VMatrix &localToWorld, const Vector &worldAxis, float rotation )
{
	// fix axes of rotation to match axes of vector
	Vector rot = worldAxis * rotation;
	// since the matrix maps local to world, do a transpose rotation to get world to local
	AngularImpulse ang = localToWorld.VMul3x3Transpose( rot );

	return ang;
}

CEntity *CreateEntityByName(const char *entityname)
{
	CBaseEntity *cbase = (CBaseEntity *)servertools->CreateEntityByName(entityname);
	return CEntity::Instance(cbase);
}

void DispatchSpawn(CBaseEntity *pEntity)
{
	servertools->DispatchSpawn(pEntity);
}


extern INetworkStringTable *g_pStringTableParticleEffectNames;


int GetParticleSystemIndex( const char *pParticleSystemName )
{
	if ( pParticleSystemName )
	{
		int nIndex = g_pStringTableParticleEffectNames->FindStringIndex( pParticleSystemName );
		if (nIndex != INVALID_STRING_INDEX )
			return nIndex;

		DevWarning("Server: Missing precache for particle system \"%s\"!\n", pParticleSystemName );
	}

	// This is the invalid string index
	return 0;
}

void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CEntity *pEntity )
{
	int iIndex = GetParticleSystemIndex( pszParticleName );
	DispatchParticleEffect( iIndex, vecOrigin, vecOrigin, vecAngles, pEntity );
}

void DispatchParticleEffect( int iEffectIndex, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CEntity *pEntity )
{
	CEffectData	data;

	data.m_nHitBox = iEffectIndex;
	data.m_vOrigin = vecOrigin;
	data.m_vStart = vecStart;
	data.m_vAngles = vecAngles;

	if ( pEntity )
	{
		data.m_nEntIndex = pEntity->entindex();
		data.m_fFlags |= PARTICLE_DISPATCH_FROM_ENTITY;
		data.m_nDamageType = PATTACH_CUSTOMORIGIN;
	}
	else
	{
		data.m_nEntIndex = 0;
	}

	g_helpfunc.DispatchEffect( "ParticleEffect", data );
}

void DispatchParticleEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CEntity *pEntity, const char *pszAttachmentName, bool bResetAllParticlesOnEntity )
{
	int iAttachment = -1;
	if ( pEntity && pEntity->GetBaseAnimating() )
	{
		// Find the attachment point index
		iAttachment = pEntity->GetBaseAnimating()->LookupAttachment( pszAttachmentName );
		if ( iAttachment == -1 )
		{
			Warning("Model '%s' doesn't have attachment '%s' to attach particle system '%s' to.\n", STRING(pEntity->GetBaseAnimating()->GetModelName()), pszAttachmentName, pszParticleName );
			return;
		}
	}

	DispatchParticleEffect( pszParticleName, iAttachType, pEntity, iAttachment, bResetAllParticlesOnEntity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DispatchParticleEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CEntity *pEntity, int iAttachmentPoint, bool bResetAllParticlesOnEntity )
{
	CEffectData	data;

	data.m_nHitBox = GetParticleSystemIndex( pszParticleName );
	if ( pEntity )
	{
		data.m_nEntIndex = pEntity->entindex();
		data.m_fFlags |= PARTICLE_DISPATCH_FROM_ENTITY;
	}
	data.m_nDamageType = iAttachType;
	data.m_nAttachmentIndex = iAttachmentPoint;

	if ( bResetAllParticlesOnEntity )
	{
		data.m_fFlags |= PARTICLE_DISPATCH_RESET_PARTICLES;
	}

	g_helpfunc.DispatchEffect( "ParticleEffect", data );
}


void StopParticleEffects( CEntity *pEntity )
{
	CEffectData	data;

	if ( pEntity )
	{
		data.m_nEntIndex = pEntity->entindex();
	}

	g_helpfunc.DispatchEffect( "ParticleEffectStop", data );
}


extern INetworkStringTable *g_pStringTableParticleEffectNames;

void PrecacheParticleSystem( const char *pParticleSystemName )
{
	g_pStringTableParticleEffectNames->AddString(true, pParticleSystemName );
}

extern INetworkStringTable *g_pStringTableMaterials;
void PrecacheMaterial( const char *pMaterialName )
{
	g_pStringTableMaterials->AddString( true, pMaterialName );
}


bool UTIL_IsMasterTriggered(string_t sMaster, CEntity *pActivator)
{
	if (sMaster != NULL_STRING)
	{
		CEntity *pMaster = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, sMaster, NULL, (pActivator)?pActivator->BaseEntity():NULL );
	
		if ( pMaster && (pMaster->ObjectCaps() & FCAP_MASTER) )
		{
			return pMaster->IsTriggered( (pActivator)?pActivator->BaseEntity():NULL );
		}

		Warning( "Master was null or not a master!\n");
	}

	// if this isn't a master entity, just say yes.
	return true;
}

void EntityMatrix::InitFromEntity( CEntity *pEntity, int iAttachment )
{
	if ( !pEntity )
	{
		Identity();
		return;
	}

	// Get an attachment's matrix?
	if ( iAttachment != 0 )
	{
		CAnimating *pAnimating = pEntity->GetBaseAnimating();
		if ( pAnimating && pAnimating->GetModelPtr() )
		{
			Vector vOrigin;
			QAngle vAngles;
			if ( pAnimating->GetAttachment( iAttachment, vOrigin, vAngles ) )
			{
				((VMatrix *)this)->SetupMatrixOrgAngles( vOrigin, vAngles );
				return;
			}
		}
	}

	((VMatrix *)this)->SetupMatrixOrgAngles( pEntity->GetAbsOrigin(), pEntity->GetAbsAngles() );
	
}



class CCheckClient : public CValveAutoGameSystem
{
public:
	CCheckClient( char const *name ) : CValveAutoGameSystem( name )
	{
	}

	void LevelInitPreEntity()
	{
		m_checkCluster = -1;
		m_lastcheck = 1;
		m_lastchecktime = -1;
		m_bClientPVSIsExpanded = false;
	}

	byte	m_checkPVS[MAX_MAP_LEAFS/8];
	byte	m_checkVisibilityPVS[MAX_MAP_LEAFS/8];
	int		m_checkCluster;
	int		m_lastcheck;
	float	m_lastchecktime;
	bool	m_bClientPVSIsExpanded;
};

CCheckClient *g_CheckClient;

static int UTIL_GetNewCheckClient( int check )
{
	int		i;
	edict_t	*ent;
	Vector	org;

// cycle to the next one

	if (check < 1)
		check = 1;
	if (check > gpGlobals->maxClients)
		check = gpGlobals->maxClients;

	if (check == gpGlobals->maxClients)
		i = 1;
	else
		i = check + 1;

	for ( ;  ; i++)
	{
		if ( i > gpGlobals->maxClients )
		{
			i = 1;
		}

		ent = engine->PEntityOfEntIndex( i );
		if ( !ent )
			continue;

		// Looped but didn't find anything else
		if ( i == check )
			break;	

		if ( !ent->GetUnknown() )
			continue;

		CEntity *entity = CEntity::Instance(GetContainingEntity( ent ));
		if ( !entity )
			continue;

		if ( entity->GetFlags() & FL_NOTARGET )
			continue;

		// anything that is a client, or has a client as an enemy
		break;
	}

	if ( i != check )
	{
		memset( g_CheckClient->m_checkVisibilityPVS, 0, sizeof(g_CheckClient->m_checkVisibilityPVS) );
		g_CheckClient->m_bClientPVSIsExpanded = false;
	}

	if ( ent )
	{
		// get the PVS for the entity
		CEntity *pce = CEntity::Instance(GetContainingEntity( ent ));
		if ( !pce )
			return i;

		org = pce->EyePosition();

		int clusterIndex = engine->GetClusterForOrigin( org );
		if ( clusterIndex != g_CheckClient->m_checkCluster )
		{
			g_CheckClient->m_checkCluster = clusterIndex;
			engine->GetPVSForCluster( clusterIndex, sizeof(g_CheckClient->m_checkPVS), g_CheckClient->m_checkPVS );
		}
	}
	
	return i;
}



static edict_t *UTIL_GetCurrentCheckClient()
{
	edict_t	*ent;

	// find a new check if on a new frame
	float delta = gpGlobals->curtime - g_CheckClient->m_lastchecktime;
	if ( delta >= 0.1 || delta < 0 )
	{
		g_CheckClient->m_lastcheck = UTIL_GetNewCheckClient( g_CheckClient->m_lastcheck );
		g_CheckClient->m_lastchecktime = gpGlobals->curtime;
	}

	// return check if it might be visible	
	ent = engine->PEntityOfEntIndex( g_CheckClient->m_lastcheck );

	// Allow dead clients -- JAY
	// Our monsters know the difference, and this function gates alot of behavior
	// It's annoying to die and see monsters stop thinking because you're no longer
	// "in" their PVS
	if ( !ent || ent->IsFree() || !ent->GetUnknown())
	{
		return NULL;
	}

	return ent;
}


extern ConVar *sv_strict_notarget;

static edict_t *UTIL_FindClientInPVSGuts(edict_t *pEdict, unsigned char *pvs, unsigned pvssize )
{
	Vector	view;

	edict_t	*ent = UTIL_GetCurrentCheckClient();
	if ( !ent )
	{
		return NULL;
	}

	CEntity *pPlayerEntity = CEntity::Instance(GetContainingEntity( ent ));
	if( (!pPlayerEntity || (pPlayerEntity->GetFlags() & FL_NOTARGET)) && sv_strict_notarget->GetBool() )
	{
		return NULL;
	}
	// if current entity can't possibly see the check entity, return 0
	// UNDONE: Build a box for this and do it over that box
	// UNDONE: Use CM_BoxLeafnums()
	CEntity *pe = CEntity::Instance(GetContainingEntity( pEdict ));
	if ( pe )
	{
		view = pe->EyePosition();
		
		if ( !engine->CheckOriginInPVS( view, pvs, pvssize ) )
		{
			return NULL;
		}
	}

	// might be able to see it
	return ent;
}


edict_t *UTIL_FindClientInPVS(edict_t *pEdict)
{
	return UTIL_FindClientInPVSGuts( pEdict, g_CheckClient->m_checkPVS, sizeof( g_CheckClient->m_checkPVS ) );
}

bool UTIL_ClientPVSIsExpanded()
{
	return g_CheckClient->m_bClientPVSIsExpanded;
}

edict_t *UTIL_FindClientInVisibilityPVS( edict_t *pEdict )
{
	return UTIL_FindClientInPVSGuts( pEdict, g_CheckClient->m_checkVisibilityPVS, sizeof( g_CheckClient->m_checkVisibilityPVS ) );
}

int SENTENCEG_Lookup(const char *sample)
{
	return engine->SentenceIndexFromName( sample + 1 );
}

int SENTENCEG_PlayRndSz(edict_t *entity, const char *szgroupname, 
					  float volume, soundlevel_t soundlevel, int flags, int pitch)
{
	char name[64];
	int ipick;
	int isentenceg;

	name[0] = 0;

	isentenceg = engine->SentenceGroupIndexFromName(szgroupname);
	if (isentenceg < 0)
	{
		Warning( "No such sentence group %s\n", szgroupname );
		return -1;
	}

	ipick = engine->SentenceGroupPick(isentenceg, name, sizeof( name ));
	if (ipick >= 0 && name[0])
	{
		int sentenceIndex = SENTENCEG_Lookup( name );
		CPASAttenuationFilter filter( CEntity::Instance( entity ), soundlevel );
		CEntity::EmitSentenceByIndex( filter, ENTINDEX(entity), CHAN_VOICE, sentenceIndex, volume, soundlevel, flags, pitch );
		return sentenceIndex;
	}

	return -1;
}

void SENTENCEG_PlaySentenceIndex( edict_t *entity, int iSentenceIndex, float volume, soundlevel_t soundlevel, int flags, int pitch )
{
	if ( iSentenceIndex >= 0 )
	{
		CPASAttenuationFilter filter( CEntity::Instance( entity ), soundlevel );
		CEntity::EmitSentenceByIndex( filter, ENTINDEX(entity), CHAN_VOICE, iSentenceIndex, volume, soundlevel, flags, pitch );
	}
}

int SENTENCEG_PickRndSz(const char *szgroupname)
{
	char name[64];
	int ipick;
	int isentenceg;

	name[0] = 0;

	isentenceg = engine->SentenceGroupIndexFromName(szgroupname);
	if (isentenceg < 0)
	{
		Warning( "No such sentence group %s\n", szgroupname );
		return -1;
	}

	ipick = engine->SentenceGroupPick(isentenceg, name, sizeof( name ));
	if (ipick >= 0 && name[0])
	{
		return SENTENCEG_Lookup( name );
	}
	return -1;
}



void ShakeRopes( const Vector &vCenter, float flRadius, float flMagnitude )
{
	CEffectData shakeData;
	shakeData.m_vOrigin = vCenter;
	shakeData.m_flRadius = flRadius;
	shakeData.m_flMagnitude = flMagnitude;
	g_helpfunc.DispatchEffect( "ShakeRopes", shakeData );
}


//-----------------------------------------------------------------------------
//
// IntersectRayWithSphere
//
// Returns whether or not there was an intersection. 
// Returns the two intersection points, clamped to (0,1)
//
//-----------------------------------------------------------------------------
bool IntersectRayWithSphere( const Vector &vecRayOrigin, const Vector &vecRayDelta, 
	const Vector &vecSphereCenter, float flRadius, float *pT1, float *pT2 )
{
	if ( !IntersectInfiniteRayWithSphere( vecRayOrigin, vecRayDelta, vecSphereCenter, flRadius, pT1, pT2 ) )
		return false;

	if (( *pT1 > 1.0f ) || ( *pT2 < 0.0f ))
		return false;

	// Clamp it!
	if ( *pT1 < 0.0f )
		*pT1 = 0.0f;
	if ( *pT2 > 1.0f )
		*pT2 = 1.0f;

	return true;
}

//-----------------------------------------------------------------------------
//
// IntersectInfiniteRayWithSphere
//
// Returns whether or not there was an intersection. 
// Returns the two intersection points
//
//-----------------------------------------------------------------------------
bool IntersectInfiniteRayWithSphere( const Vector &vecRayOrigin, const Vector &vecRayDelta, 
	const Vector &vecSphereCenter, float flRadius, float *pT1, float *pT2 )
{
	// Solve using the ray equation + the sphere equation
	// P = o + dt
	// (x - xc)^2 + (y - yc)^2 + (z - zc)^2 = r^2
	// (ox + dx * t - xc)^2 + (oy + dy * t - yc)^2 + (oz + dz * t - zc)^2 = r^2
	// (ox - xc)^2 + 2 * (ox-xc) * dx * t + dx^2 * t^2 +
	//		(oy - yc)^2 + 2 * (oy-yc) * dy * t + dy^2 * t^2 +
	//		(oz - zc)^2 + 2 * (oz-zc) * dz * t + dz^2 * t^2 = r^2
	// (dx^2 + dy^2 + dz^2) * t^2 + 2 * ((ox-xc)dx + (oy-yc)dy + (oz-zc)dz) t +
	//		(ox-xc)^2 + (oy-yc)^2 + (oz-zc)^2 - r^2 = 0
	// or, t = (-b +/- sqrt( b^2 - 4ac)) / 2a
	// a = DotProduct( vecRayDelta, vecRayDelta );
	// b = 2 * DotProduct( vecRayOrigin - vecCenter, vecRayDelta )
	// c = DotProduct(vecRayOrigin - vecCenter, vecRayOrigin - vecCenter) - flRadius * flRadius;

	Vector vecSphereToRay;
	VectorSubtract(	vecRayOrigin, vecSphereCenter, vecSphereToRay );

	float a = DotProduct( vecRayDelta, vecRayDelta );

	// This would occur in the case of a zero-length ray
	if ( a == 0.0f )
	{
		*pT1 = *pT2 = 0.0f;
		return vecSphereToRay.LengthSqr() <= flRadius * flRadius;
	}

	float b = 2 * DotProduct( vecSphereToRay, vecRayDelta );
	float c = DotProduct( vecSphereToRay, vecSphereToRay ) - flRadius * flRadius;
	float flDiscrim = b * b - 4 * a * c;
	if ( flDiscrim < 0.0f )
		return false;

	flDiscrim = sqrt( flDiscrim );
	float oo2a = 0.5f / a;
	*pT1 = ( - b - flDiscrim ) * oo2a;
	*pT2 = ( - b + flDiscrim ) * oo2a;
	return true;
}

//-----------------------------------------------------------------------------
// Clears the trace
//-----------------------------------------------------------------------------
static void Collision_ClearTrace( const Vector &vecRayStart, const Vector &vecRayDelta, CBaseTrace *pTrace )
{
	pTrace->startpos = vecRayStart;
	pTrace->endpos = vecRayStart;
	pTrace->endpos += vecRayDelta;
	pTrace->startsolid = false;
	pTrace->allsolid = false;
	pTrace->fraction = 1.0f;
	pTrace->contents = 0;
}


//-----------------------------------------------------------------------------
// Intersects a ray against a box
//-----------------------------------------------------------------------------
bool IntersectRayWithBox( const Vector &vecRayStart, const Vector &vecRayDelta, 
	const Vector &boxMins, const Vector &boxMaxs, float flTolerance, BoxTraceInfo_t *pTrace )
{
	int			i;
	float		d1, d2;
	float		f;

	pTrace->t1 = -1.0f;
	pTrace->t2 = 1.0f;
	pTrace->hitside = -1;

	// UNDONE: This makes this code a little messy
	pTrace->startsolid = true;

	for ( i = 0; i < 6; ++i )
	{
		if ( i >= 3 )
		{
			d1 = vecRayStart[i-3] - boxMaxs[i-3];
			d2 = d1 + vecRayDelta[i-3];
		}
		else
		{
			d1 = -vecRayStart[i] + boxMins[i];
			d2 = d1 - vecRayDelta[i];
		}

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 > 0)
		{
			// UNDONE: Have to revert this in case it's still set
			// UNDONE: Refactor to have only 2 return points (true/false) from this function
			pTrace->startsolid = false;
			return false;
		}

		// completely inside, check next face
		if (d1 <= 0 && d2 <= 0)
			continue;

		if (d1 > 0)
		{
			pTrace->startsolid = false;
		}

		// crosses face
		if (d1 > d2)
		{
			f = d1 - flTolerance;
			if ( f < 0 )
			{
				f = 0;
			}
			f = f / (d1-d2);
			if (f > pTrace->t1)
			{
				pTrace->t1 = f;
				pTrace->hitside = i;
			}
		}
		else
		{ 
			// leave
			f = (d1 + flTolerance) / (d1-d2);
			if (f < pTrace->t2)
			{
				pTrace->t2 = f;
			}
		}
	}

	return pTrace->startsolid || (pTrace->t1 < pTrace->t2 && pTrace->t1 >= 0.0f);
}



//-----------------------------------------------------------------------------
// Intersects a ray against a box
//-----------------------------------------------------------------------------
bool IntersectRayWithBox( const Vector &vecRayStart, const Vector &vecRayDelta, 
	const Vector &boxMins, const Vector &boxMaxs, float flTolerance, CBaseTrace *pTrace, float *pFractionLeftSolid )
{
	Collision_ClearTrace( vecRayStart, vecRayDelta, pTrace );

	BoxTraceInfo_t trace;

	if ( IntersectRayWithBox( vecRayStart, vecRayDelta, boxMins, boxMaxs, flTolerance, &trace ) )
	{
		pTrace->startsolid = trace.startsolid;
		if (trace.t1 < trace.t2 && trace.t1 >= 0.0f)
		{
			pTrace->fraction = trace.t1;
			VectorMA( pTrace->startpos, trace.t1, vecRayDelta, pTrace->endpos );
			pTrace->contents = CONTENTS_SOLID;
			pTrace->plane.normal = vec3_origin;
			if ( trace.hitside >= 3 )
			{
				trace.hitside -= 3;
				pTrace->plane.dist = boxMaxs[trace.hitside];
				pTrace->plane.normal[trace.hitside] = 1.0f;
				pTrace->plane.type = trace.hitside;
			}
			else
			{
				pTrace->plane.dist = -boxMins[trace.hitside];
				pTrace->plane.normal[trace.hitside] = -1.0f;
				pTrace->plane.type = trace.hitside;
			}
			return true;
		}

		if ( pTrace->startsolid )
		{
			pTrace->allsolid = (trace.t2 <= 0.0f) || (trace.t2 >= 1.0f);
			pTrace->fraction = 0;
			if ( pFractionLeftSolid )
			{
				*pFractionLeftSolid = trace.t2;
			}
			pTrace->endpos = pTrace->startpos;
			pTrace->contents = CONTENTS_SOLID;
			pTrace->plane.dist = pTrace->startpos[0];
			pTrace->plane.normal.Init( 1.0f, 0.0f, 0.0f );
			pTrace->plane.type = 0;
			pTrace->startpos = vecRayStart + (trace.t2 * vecRayDelta);
			return true;
		}
	}

	return false;
}


bool IsInWorld(const Vector &pos)
{
	if (pos.x >= MAX_COORD_INTEGER) return false;
	if (pos.y >= MAX_COORD_INTEGER) return false;
	if (pos.z >= MAX_COORD_INTEGER) return false;
	if (pos.x <= MIN_COORD_INTEGER) return false;
	if (pos.y <= MIN_COORD_INTEGER) return false;
	if (pos.z <= MIN_COORD_INTEGER) return false;

	return true;
}


CEntity *UTIL_FindClientInPVS( const Vector &vecBoxMins, const Vector &vecBoxMaxs )
{
	edict_t	*ent = UTIL_GetCurrentCheckClient();
	if ( !ent )
	{
		return NULL;
	}

	if ( !engine->CheckBoxInPVS( vecBoxMins, vecBoxMaxs, g_CheckClient->m_checkPVS, sizeof( g_CheckClient->m_checkPVS ) ) )
	{
		return NULL;
	}

	// might be able to see it
	return CEntity::Instance( ent );
}

void UTIL_Tracer( const Vector &vecStart, const Vector &vecEnd, int iEntIndex, 
				 int iAttachment, float flVelocity, bool bWhiz, const char *pCustomTracerName, int iParticleID )
{
	CEffectData data;
	data.m_vStart = vecStart;
	data.m_vOrigin = vecEnd;
	data.m_nEntIndex = iEntIndex;
	data.m_flScale = flVelocity;
	data.m_nHitBox = iParticleID;

	// Flags
	if ( bWhiz )
	{
		data.m_fFlags |= TRACER_FLAG_WHIZ;
	}

	if ( iAttachment != TRACER_DONT_USE_ATTACHMENT )
	{
		data.m_fFlags |= TRACER_FLAG_USEATTACHMENT;
		data.m_nAttachmentIndex = iAttachment;
	}

	// Fire it off
	if ( pCustomTracerName )
	{
		g_helpfunc.DispatchEffect( "Tracer", data );
		//g_helpfunc.DispatchEffect( pCustomTracerName, data );
	}
	else
	{
		g_helpfunc.DispatchEffect( "Tracer", data );
	}
}

bool CanCreateEntityClass( const char *pszClassname )
{
	return ( EntityFactoryDictionary_CE() != NULL && EntityFactoryDictionary_CE()->FindFactory( pszClassname ) != NULL );
}

void UTIL_SetModel( CEntity *pEntity, const char *pModelName )
{
	// check to see if model was properly precached
	int i = modelinfo->GetModelIndex( pModelName );
	if ( i < 0 )
	{
		Error("%i/%s - %s:  UTIL_SetModel:  not precached: %s\n", pEntity->entindex(),
			pEntity->GetEntityName(),
			pEntity->GetClassname(), pModelName);
	}

	pEntity->SetModelIndex( i ) ;
	pEntity->SetModelName( AllocPooledString( pModelName ) );

	// brush model
	const model_t *mod = modelinfo->GetModel( i );
	if ( mod )
	{
		Vector mins, maxs;
		modelinfo->GetModelBounds( mod, mins, maxs );
		g_helpfunc.SetMinMaxSize (pEntity->BaseEntity(), mins, maxs);
	}
	else
	{
		g_helpfunc.SetMinMaxSize (pEntity->BaseEntity(), vec3_origin, vec3_origin);
	}

	CAnimating *pAnimating = pEntity->GetBaseAnimating();
	if ( pAnimating )
	{
		pAnimating->m_nForceBone = 0;
	}
}


CPlayer *UTIL_GetLocalPlayer()
{
	for(int i=1;i<=gpGlobals->maxClients;i++)
	{
		CPlayer *pPlayer = UTIL_PlayerByIndex(i);
		if(!pPlayer)
			continue;
		return pPlayer;
	}
	return NULL;
}

void UTIL_TraceModel( const Vector &vecStart, const Vector &vecEnd, const Vector &hullMin, 
					  const Vector &hullMax, CEntity *pentModel, int collisionGroup, trace_t *ptr )
{
	// Cull it....
	if ( pentModel && pentModel->ShouldCollide( collisionGroup, MASK_ALL ) )
	{
		Ray_t ray;
		ray.Init( vecStart, vecEnd, hullMin, hullMax );
		enginetrace->ClipRayToEntity( ray, MASK_ALL, pentModel->BaseEntity(), ptr ); 
	}
	else
	{
		memset( ptr, 0, sizeof(trace_t) );
		ptr->fraction = 1.0f;
	}
}


#define EXTRACT_VOID_FUNCTIONPTR(x)		(*(void **)(&(x)))

void *UTIL_FunctionFromName( datamap_t *pMap, const char *pName )
{
	while ( pMap )
	{
		for ( int i = 0; i < pMap->dataNumFields; i++ )
		{
			Assert( sizeof(pMap->dataDesc[i].inputFunc) == sizeof(void *) );

			if ( pMap->dataDesc[i].flags & FTYPEDESC_FUNCTIONTABLE )
			{
				if ( FStrEq( pName, pMap->dataDesc[i].fieldName ) )
				{
					return EXTRACT_VOID_FUNCTIONPTR(pMap->dataDesc[i].inputFunc);
				}
			}
		}
		pMap = pMap->baseMap;
	}

	Msg( "Failed to find function %s\n", pName );

	return NULL;
}

static csurface_t	g_NullSurface = { "**empty**", 0 };

void UTIL_ClearTrace( trace_t &trace )
{
	memset( &trace, 0, sizeof(trace));
	trace.fraction = 1.f;
	trace.fractionleftsolid = 0;
	trace.surface = g_NullSurface;
}

byte *UTIL_LoadFileForMe( const char *filename, int *pLength )
{
	void *buffer = NULL;

	int length = filesystem->ReadFileEx( filename, "GAME", &buffer, true, true );

	if ( pLength )
	{
		*pLength = length;
	}

	return (byte *)buffer;
}

void UTIL_FreeFile( byte *buffer )
{
	filesystem->FreeOptimalReadBuffer( buffer );
}

void UTIL_ParentToWorldSpace( CEntity *pEntity, Vector &vecPosition, QAngle &vecAngles )
{
	if ( pEntity == NULL )
		return;

	// Construct the entity-to-world matrix
	// Start with making an entity-to-parent matrix
	matrix3x4_t matEntityToParent;
	AngleMatrix( vecAngles, matEntityToParent );
	MatrixSetColumn( vecPosition, 3, matEntityToParent );

	// concatenate with our parent's transform
	matrix3x4_t matScratch, matResult;
	matrix3x4_t matParentToWorld;
	
	if ( pEntity->GetParent() != NULL )
	{
		matParentToWorld = pEntity->GetParentToWorldTransform( matScratch );
	}
	else
	{
		matParentToWorld = pEntity->EntityToWorldTransform();
	}

	ConcatTransforms( matParentToWorld, matEntityToParent, matResult );

	// pull our absolute position out of the matrix
	MatrixGetColumn( matResult, 3, vecPosition );
	MatrixAngles( matResult, vecAngles );
}


void UTIL_ParentToWorldSpace( CEntity *pEntity, Vector &vecPosition, Quaternion &quat )
{
	if ( pEntity == NULL )
		return;

	QAngle vecAngles;
	QuaternionAngles( quat, vecAngles );
	UTIL_ParentToWorldSpace( pEntity, vecPosition, vecAngles );
	AngleQuaternion( vecAngles, quat );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bf_write *g_pMsgBuffer = NULL;

void EntityMessageBegin( CEntity * entity, bool reliable /*= false*/ ) 
{
	Assert( !g_pMsgBuffer );

	Assert ( entity );

	g_pMsgBuffer = engine->EntityMessageBegin( entity->entindex(), entity->GetServerClass(), reliable );
}

void UserMessageBegin( IRecipientFilter& filter, const char *messagename )
{
	Assert( !g_pMsgBuffer );

	Assert( messagename );

	int msg_type = usermsgs->GetMessageIndex(messagename);
	
	if ( msg_type == -1 )
	{
		Error( "UserMessageBegin:  Unregistered message '%s'\n", messagename );
	}

	g_pMsgBuffer = engine->UserMessageBegin( &filter, msg_type );
}

void MessageEnd( void )
{
	Assert( g_pMsgBuffer );

	engine->MessageEnd();

	g_pMsgBuffer = NULL;
}

void MessageWriteByte( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WRITE_BYTE called with no active message\n" );

	g_pMsgBuffer->WriteByte( iValue );
}

void MessageWriteChar( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WRITE_CHAR called with no active message\n" );

	g_pMsgBuffer->WriteChar( iValue );
}

void MessageWriteShort( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WRITE_SHORT called with no active message\n" );

	g_pMsgBuffer->WriteShort( iValue );
}

void MessageWriteWord( int iValue )
{
	if (!g_pMsgBuffer)
		Error( "WRITE_WORD called with no active message\n" );

	g_pMsgBuffer->WriteWord( iValue );
}

void MessageWriteLong( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteLong called with no active message\n" );

	g_pMsgBuffer->WriteLong( iValue );
}

void MessageWriteFloat( float flValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteFloat called with no active message\n" );

	g_pMsgBuffer->WriteFloat( flValue );
}

void MessageWriteAngle( float flValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteAngle called with no active message\n" );

	g_pMsgBuffer->WriteBitAngle( flValue, 8 );
}

void MessageWriteCoord( float flValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteCoord called with no active message\n" );

	g_pMsgBuffer->WriteBitCoord( flValue );
}

void MessageWriteVec3Coord( const Vector& rgflValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteVec3Coord called with no active message\n" );

	g_pMsgBuffer->WriteBitVec3Coord( rgflValue );
}

void MessageWriteVec3Normal( const Vector& rgflValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteVec3Normal called with no active message\n" );

	g_pMsgBuffer->WriteBitVec3Normal( rgflValue );
}

void MessageWriteAngles( const QAngle& rgflValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteVec3Normal called with no active message\n" );

	g_pMsgBuffer->WriteBitAngles( rgflValue );
}

void MessageWriteString( const char *sz )
{
	if (!g_pMsgBuffer)
		Error( "WriteString called with no active message\n" );

	g_pMsgBuffer->WriteString( sz );
}

void MessageWriteEntity( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteEntity called with no active message\n" );

	g_pMsgBuffer->WriteShort( iValue );
}

void MessageWriteEHandle( CBaseEntity *pEntity )
{
	if (!g_pMsgBuffer)
		Error( "WriteEHandle called with no active message\n" );

	long iEncodedEHandle;
	
	if( pEntity )
	{
		EHANDLE hEnt = pEntity;

		int iSerialNum = hEnt.GetSerialNumber() & (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1;
		iEncodedEHandle = hEnt.GetEntryIndex() | (iSerialNum << MAX_EDICT_BITS);
	}
	else
	{
		iEncodedEHandle = INVALID_NETWORKED_EHANDLE_VALUE;
	}
	
	g_pMsgBuffer->WriteLong( iEncodedEHandle );
}

// bitwise
void MessageWriteBool( bool bValue )
{
	if (!g_pMsgBuffer)
		Error( "WriteBool called with no active message\n" );

	g_pMsgBuffer->WriteOneBit( bValue ? 1 : 0 );
}

void MessageWriteUBitLong( unsigned int data, int numbits )
{
	if (!g_pMsgBuffer)
		Error( "WriteUBitLong called with no active message\n" );

	g_pMsgBuffer->WriteUBitLong( data, numbits );
}

void MessageWriteSBitLong( int data, int numbits )
{
	if (!g_pMsgBuffer)
		Error( "WriteSBitLong called with no active message\n" );

	g_pMsgBuffer->WriteSBitLong( data, numbits );
}

void MessageWriteBits( const void *pIn, int nBits )
{
	if (!g_pMsgBuffer)
		Error( "WriteBits called with no active message\n" );

	g_pMsgBuffer->WriteBits( pIn, nBits );
}


static void GetAllChildren_r( CEntity *pEntity, CUtlVector<CEntity *> &list )
{
	for ( ; pEntity != NULL; pEntity = pEntity->NextMovePeer() )
	{
		list.AddToTail( pEntity );
		GetAllChildren_r( pEntity->FirstMoveChild(), list );
	}
}

int GetAllChildren( CEntity *pParent, CUtlVector<CEntity *> &list )
{
	if ( !pParent )
		return 0;

	GetAllChildren_r( pParent->FirstMoveChild(), list );
	return list.Count();
}

const float k_flMaxVelocity = 2000.0f;
const float k_flMaxAngularVelocity = 360.0f * 10.0f;

float k_flMaxEntitySpeed = k_flMaxVelocity * 2.0f;
float k_flMaxEntitySpinRate = k_flMaxAngularVelocity * 10.0f;

// Utility func to throttle rate at which the "reasonable position" spew goes out
static double s_LastEntityReasonableEmitTime = 0;
bool CheckEmitReasonablePhysicsSpew()
{
	// Reported recently?
	double now = Plat_FloatTime();
	if ( now >= s_LastEntityReasonableEmitTime && now < s_LastEntityReasonableEmitTime + 5.0 )
	{
		// Already reported recently
		return false;
	}

	// Not reported recently.  Report it now
	s_LastEntityReasonableEmitTime = now;
	return true;
}

int CheckEntityVelocity( Vector &v )
{
	float r = k_flMaxEntitySpeed;
	if (
		v.x > -r && v.x < r &&
		v.y > -r && v.y < r &&
		v.z > -r && v.z < r)
	{
		// The usual case.  It's totally reasonable
		return 1;
	}
	float speed = v.Length();
	if ( speed < k_flMaxEntitySpeed * 100.0f )
	{
		// Sort of suspicious.  Clamp it
		v *= k_flMaxEntitySpeed / speed;
		return 0;
	}

	// A terrible, horrible, no good, very bad velocity.
	return -1;
}

