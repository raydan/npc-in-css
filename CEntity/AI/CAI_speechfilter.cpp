
#include "CAI_speechfilter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(ai_speechfilter, CE_AI_SpeechFilter);

DEFINE_PROP(m_flIdleModifier, CE_AI_SpeechFilter);
DEFINE_PROP(m_bNeverSayHello, CE_AI_SpeechFilter);


