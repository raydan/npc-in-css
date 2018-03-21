//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Expose an IsAHunter function
//
//=============================================================================//

#ifndef NPC_HUNTER_H
#define NPC_HUNTER_H

#if defined( _WIN32 )
#pragma once
#endif

class CBaseEntity;
class CEntity;


/// true if given entity pointer is a hunter.
bool Hunter_IsHunter(CEntity *pEnt);

// call throughs for member functions

void Hunter_StriderBusterAttached( CEntity *pHunter, CEntity *pAttached );
void Hunter_StriderBusterDetached( CEntity *pHunter, CEntity *pAttached );
void Hunter_StriderBusterLaunched( CEntity *pBuster );

#endif
