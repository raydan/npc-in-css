
#include "CAI_NPC.h"
#include "activitylist.h"
#include "sign_func.h"
#include "GameSystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CUtlDict< CActivityRemapCache, int > m_ActivityRemapDatabase;

class CActivityRemapCacheHelper : public CBaseGameSystem
{
public:
	CActivityRemapCacheHelper( char const *name ) : CBaseGameSystem( name )
	{
	}
	void LevelInitPreEntity()
	{
		m_ActivityRemapDatabase.Purge();
	}
};

static CActivityRemapCacheHelper g_CActivityRemapCacheHelper("CActivityRemapCacheHelper");


Activity ActivityList_RegisterPrivateActivity( const char *pszActivityName )
{
	return g_helpfunc.ActivityList_RegisterPrivateActivity(pszActivityName);
}

const char *ActivityList_NameForIndex( int activityIndex )
{
	return g_helpfunc.ActivityList_NameForIndex(activityIndex);
}

void UTIL_LoadActivityRemapFile( const char *filename, const char *section, CUtlVector <CActivityRemap> &entries )
{
	int iIndex = m_ActivityRemapDatabase.Find( filename );

	if ( iIndex != m_ActivityRemapDatabase.InvalidIndex() )
	{
		CActivityRemapCache *actRemap = &m_ActivityRemapDatabase[iIndex];
		entries.AddVectorToTail( actRemap->m_cachedActivityRemaps );
		return;
	}

	KeyValues *pkvFile = new KeyValues( section );

	if ( pkvFile->LoadFromFile( filesystem, filename, NULL ) )
	{
		KeyValues *pTestKey = pkvFile->GetFirstSubKey();

		CActivityRemapCache actRemap;

		while ( pTestKey )
		{
			Activity ActBase = (Activity)g_helpfunc.ActivityList_IndexForName( pTestKey->GetName() );

			if ( ActBase != ACT_INVALID )
			{
				KeyValues *pRemapKey = pTestKey->GetFirstSubKey();

				CActivityRemap actMap;
				actMap.mappedActivity = ACT_IDLE;
				actMap.activity = ActBase;

				while ( pRemapKey )
				{
					const char *pKeyName = pRemapKey->GetName();
					const char *pKeyValue = pRemapKey->GetString();

					if ( !stricmp( pKeyName, "remapactivity" ) )
					{
						Activity Act = (Activity)g_helpfunc.ActivityList_IndexForName( pKeyValue );

						if ( Act == ACT_INVALID )
						{
							actMap.mappedActivity = ActivityList_RegisterPrivateActivity( pKeyValue );
						}
						else
						{
							actMap.mappedActivity = Act;
						}
					}
					else if ( !stricmp( pKeyName, "extra" ) )
					{
						actMap.SetExtraKeyValueBlock( pRemapKey->MakeCopy() );
					}

					pRemapKey = pRemapKey->GetNextKey();
				}

				entries.AddToTail( actMap );
			}

			pTestKey = pTestKey->GetNextKey();
		}

		actRemap.m_cachedActivityRemaps.AddVectorToTail( entries );
		m_ActivityRemapDatabase.Insert( filename, actRemap );
	}
}

