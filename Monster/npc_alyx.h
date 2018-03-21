//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "CAI_Baseactor.h"
#include "npc_playercompanion.h"

class CNPC_Alyx : public CNPC_PlayerCompanion
{
public:
	CE_DECLARE_CLASS( CNPC_Alyx, CNPC_PlayerCompanion );

	bool	CreateBehaviors();
	void	Spawn( void );
	void	SelectModel();
	void	Precache( void );
	void	SetupAlyxWithoutParent( void );
	void	CreateEmpTool( void );
	void	PrescheduleThink( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	Activity NPC_TranslateActivity ( Activity activity );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *dump );
	bool	ShouldLookForBetterWeapon() { return false; }
	bool	IsReadinessCapable() { return false; }
	void	DeathSound( const CTakeDamageInfo &info );

	CFakeHandle	m_hEmpTool;

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};
