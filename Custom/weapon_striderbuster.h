//========= Copyright ?1996-2007, Valve Corporation, All rights reserved. ====
//
// Helper functions for the striderbuster weapon.
//
//=============================================================================

#ifndef WEAPON_STRIDERBUSTER_H
#define WEAPON_STRIDERBUSTER_H
#ifdef _WIN32
#pragma once
#endif

bool StriderBuster_IsAttachedStriderBuster( CEntity *pEntity, CEntity *pAttachedTo = NULL );
void StriderBuster_OnAddToCargoHold( CEntity *pEntity );
bool StriderBuster_OnFlechetteAttach( CEntity *pEntity,  Vector &vecForceDir );
int StriderBuster_NumFlechettesAttached( CEntity *pEntity );
float StriderBuster_GetPickupTime( CEntity *pEntity );
bool StriderBuster_WasKnockedOffStrider( CEntity *pEntity );

#endif // WEAPON_STRIDERBUSTER_H
