// gameAI.cpp

#include "CvGameCoreDLL.h"
#include "CvGameAI.h"
#include "CvPlayerAI.h"
#include "CvTeamAI.h"
#include "CvGlobals.h"
#include "CvInfos.h"

// Public Functions...

CvGameAI::CvGameAI()
{
	AI_reset();
}


CvGameAI::~CvGameAI()
{
	AI_uninit();
}


void CvGameAI::AI_init()
{
	AI_reset();

	//--------------------------------
	// Init other game data
}


void CvGameAI::AI_uninit()
{
}


void CvGameAI::AI_reset()
{
	AI_uninit();

	m_iPad = 0;
}


void CvGameAI::AI_makeAssignWorkDirty()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).AI_makeAssignWorkDirty();
		}
	}
}


void CvGameAI::AI_updateAssignWork()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (GET_TEAM(kLoopPlayer.getTeam()).isHuman() && kLoopPlayer.isAlive())
		{
			kLoopPlayer.AI_updateAssignWork();
		}
	}
}


int CvGameAI::AI_combatValue(UnitTypes eUnit) const
{
	int iValue;

	iValue = 100;

	if (GC.getUnitInfo(eUnit).getDomainType() == DOMAIN_AIR)
	{
		iValue *= GC.getUnitInfo(eUnit).getAirCombat();
	}
	else
	{
		iValue *= GC.getUnitInfo(eUnit).getCombat();

		iValue *= ((((GC.getUnitInfo(eUnit).getFirstStrikes() * 2) + GC.getUnitInfo(eUnit).getChanceFirstStrikes()) * (GC.getCOMBAT_DAMAGE() / 5)) + 100);
		iValue /= 100;
	}

	iValue /= getBestLandUnitCombat();

	return iValue;
}


int CvGameAI::AI_turnsPercent(int iTurns, int iPercent)
{
	FAssert(iPercent > 0);
	if (iTurns != MAX_INT)
	{
		iTurns *= (iPercent);
		iTurns /= 100;
	}

	return std::max(1, iTurns);
}

// early war: the AI starts no war plans of its own before this turn (see CvTeamAI::AI_doWar).
// Normal-speed turns from GlobalDefinesAlt.xml, scaled by the game speed's barbarian percent like K-Mod's original
// 30-turn rule (Quick 67%, Epic 150%, Marathon and Marathon Accelerated 400%).
int CvGameAI::AI_earlyWarGateTurns() const
{
	int iSpeedPercent = GC.getGameSpeedInfo(getGameSpeedType()).getBarbPercent();
	return GC.getDefineINT("AI_EARLY_WAR_GATE_TURNS", 30) * iSpeedPercent / 100;
}

// early war: the AI's war rands are multiplied by this percentage, so war plans are less likely early in the game.
// AI_EARLY_WAR_RAND_PERCENT at the end of the turn gate, falling linearly to 100 at AI_EARLY_WAR_RAMP_TURNS
// (Normal-speed turns, scaled like the gate).
int CvGameAI::AI_earlyWarRandPercent() const
{
	int iMaxPercent = GC.getDefineINT("AI_EARLY_WAR_RAND_PERCENT", 100);
	int iSpeedPercent = GC.getGameSpeedInfo(getGameSpeedType()).getBarbPercent();
	int iRampStart = AI_earlyWarGateTurns();
	int iRampEnd = GC.getDefineINT("AI_EARLY_WAR_RAMP_TURNS", 0) * iSpeedPercent / 100;
	int iTurn = getElapsedGameTurns();

	if (iMaxPercent <= 100 || iTurn >= iRampEnd)
		return 100;
	if (iTurn <= iRampStart)
		return iMaxPercent;
	return iMaxPercent - (iMaxPercent - 100) * (iTurn - iRampStart) / (iRampEnd - iRampStart);
}


void CvGameAI::read(FDataStreamBase* pStream)
{
	CvGame::read(pStream);

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iPad);
}


void CvGameAI::write(FDataStreamBase* pStream)
{
	CvGame::write(pStream);

	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iPad);
}

// Protected Functions...

// Private Functions...
