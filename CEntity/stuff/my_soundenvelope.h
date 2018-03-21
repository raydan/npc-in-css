
#ifndef MY_SOUNDENVELOPE_H
#define MY_SOUNDENVELOPE_H

#include "extension.h"


class CSoundEnvelope
{
public:
	
private:
	float	m_current;
	float	m_target;
	float	m_rate;
	bool	m_forceupdate;
};


class CCopyRecipientFilter : public IRecipientFilter
{
public:
	CCopyRecipientFilter() : m_Flags(0) {}
	virtual bool IsReliable( void ) const;
	virtual bool IsInitMessage( void ) const;
	virtual int	GetRecipientCount( void ) const;
	virtual int	GetRecipientIndex( int slot ) const;


private:
	enum
	{
		FLAG_ACTIVE = 0x1,
		FLAG_RELIABLE = 0x2,
		FLAG_INIT_MESSAGE = 0x4,
	};

	int m_Flags;
	CUtlVector< int > m_Recipients;
};




class CSoundPatch
{
public:

	CSoundEnvelope	m_pitch;
	CSoundEnvelope	m_volume;

	soundlevel_t	m_soundlevel;
	float			m_shutdownTime;
	float			m_flLastTime;
	string_t		m_iszSoundName;
	string_t		m_iszSoundScriptName;
	EHANDLE			m_hEnt;
	int				m_entityChannel;
	int				m_flags;
	int				m_baseFlags;
	int				m_isPlaying;
	float			m_flScriptVolume;	// Volume for this sound in sounds.txt
	CCopyRecipientFilter m_Filter;

	float			m_flCloseCaptionDuration;


};



#endif
