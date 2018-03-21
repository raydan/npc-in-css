
#include "CEntity.h"
#include "my_soundenvelope.h"

#include "tier0/memdbgon.h"



bool CCopyRecipientFilter::IsReliable( void ) const
{
	return (m_Flags & FLAG_RELIABLE) != 0;
}

bool CCopyRecipientFilter::IsInitMessage( void ) const
{
	return (m_Flags & FLAG_INIT_MESSAGE) != 0;
}

int	CCopyRecipientFilter::GetRecipientCount( void ) const
{
	return m_Recipients.Count();
}

int	CCopyRecipientFilter::GetRecipientIndex( int slot ) const
{
	return m_Recipients[ slot ];
}


