
#include "CEntity.h"
#include "CSoundent.h"


bool CSound::FIsSound ( void )
{
	switch( SoundTypeNoContext() )
	{
	case SOUND_COMBAT:
	case SOUND_WORLD:
	case SOUND_PLAYER:
	case SOUND_DANGER:
	case SOUND_DANGER_SNIPERONLY:
	case SOUND_THUMPER:
	case SOUND_BULLET_IMPACT:
	case SOUND_BUGBAIT:
	case SOUND_PHYSICS_DANGER:
	case SOUND_MOVE_AWAY:
	case SOUND_PLAYER_VEHICLE:
		return true;

	default:
		return false;
	}
}
