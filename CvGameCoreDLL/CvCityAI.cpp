// cityAI.cpp

#include "CvGameCoreDLL.h"
#include "CvGlobals.h"
#include "CvGameCoreUtils.h"
#include "CvCityAI.h"
#include "CvGameAI.h"
#include "CvPlot.h"
#include "CvArea.h"
#include "CvPlayerAI.h"
#include "CvTeamAI.h"
#include "CyCity.h"
#include "CyArgsList.h"
#include "CvInfos.h"
#include "FProfiler.h"

#include "CvDLLPythonIFaceBase.h"
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLFAStarIFaceBase.h"

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/02/09                                jdog5000      */
/*                                                                                              */
/* AI logging                                                                                   */
/************************************************************************************************/
#include "BetterBTSAI.h"
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

#define BUILDINGFOCUS_FOOD					(1 << 1)
#define BUILDINGFOCUS_PRODUCTION			(1 << 2)
#define BUILDINGFOCUS_GOLD					(1 << 3)
#define BUILDINGFOCUS_RESEARCH				(1 << 4)
#define BUILDINGFOCUS_CULTURE				(1 << 5)
#define BUILDINGFOCUS_DEFENSE				(1 << 6)
#define BUILDINGFOCUS_HAPPY					(1 << 7)
#define BUILDINGFOCUS_HEALTHY				(1 << 8)
#define BUILDINGFOCUS_EXPERIENCE			(1 << 9)
#define BUILDINGFOCUS_MAINTENANCE			(1 << 10)
#define BUILDINGFOCUS_SPECIALIST			(1 << 11)
#define BUILDINGFOCUS_ESPIONAGE				(1 << 12)
#define BUILDINGFOCUS_BIGCULTURE			(1 << 13)
#define BUILDINGFOCUS_WORLDWONDER			(1 << 14)
#define BUILDINGFOCUS_DOMAINSEA				(1 << 15)
#define BUILDINGFOCUS_WONDEROK				(1 << 16)
#define BUILDINGFOCUS_CAPITAL				(1 << 17)



// Public Functions...

CvCityAI::CvCityAI()
{
	m_aiEmphasizeYieldCount = new int[NUM_YIELD_TYPES];
	m_aiEmphasizeCommerceCount = new int[NUM_COMMERCE_TYPES];
	m_bForceEmphasizeCulture = false;
	m_aiSpecialYieldMultiplier = new int[NUM_YIELD_TYPES];
	m_aiPlayerCloseness = new int[MAX_PLAYERS];
	m_aiConstructionValue.assign(GC.getNumBuildingClassInfos(), -1); // K-Mod

	m_pbEmphasize = NULL;

	AI_reset();
}


CvCityAI::~CvCityAI()
{
	AI_uninit();

	SAFE_DELETE_ARRAY(m_aiEmphasizeYieldCount);
	SAFE_DELETE_ARRAY(m_aiEmphasizeCommerceCount);
	SAFE_DELETE_ARRAY(m_aiSpecialYieldMultiplier);
	SAFE_DELETE_ARRAY(m_aiPlayerCloseness);
}


void CvCityAI::AI_init()
{
	AI_reset();

	//--------------------------------
	// Init other game data
	AI_assignWorkingPlots();

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      11/14/09                                jdog5000      */
/*                                                                                              */
/* City AI, Worker AI                                                                           */
/************************************************************************************************/
/* original bts code
	AI_updateWorkersNeededHere();
	
	AI_updateBestBuild();
*/
	AI_updateBestBuild();

	AI_updateWorkersNeededHere();
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
}


void CvCityAI::AI_uninit()
{
	SAFE_DELETE_ARRAY(m_pbEmphasize);
}


// FUNCTION: AI_reset()
// Initializes data members that are serialized.
void CvCityAI::AI_reset()
{
	int iI;

	AI_uninit();

	m_iEmphasizeAvoidGrowthCount = 0;
	m_iEmphasizeGreatPeopleCount = 0;
	m_bForceEmphasizeCulture = false;

	m_bAssignWorkDirty = false;
	m_bChooseProductionDirty = false;

	m_routeToCity.reset();

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_aiEmphasizeYieldCount[iI] = 0;
	}

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		m_aiEmphasizeCommerceCount[iI] = 0;
	}

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		m_aiBestBuildValue[iI] = NO_BUILD;
	}

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		m_aeBestBuild[iI] = NO_BUILD;
	}
	
	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_aiSpecialYieldMultiplier[iI] = 0;
	}
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aiPlayerCloseness[iI] = 0;
	}
	AI_ClearConstructionValueCache(); // K-Mod

	m_iCachePlayerClosenessTurn = -1;
	m_iCachePlayerClosenessDistance = -1;
	
	m_iNeededFloatingDefenders = -1;
	m_iNeededFloatingDefendersCacheTurn = -1;

	m_iWorkersNeeded = 0;
	m_iWorkersHave = 0;

	FAssertMsg(m_pbEmphasize == NULL, "m_pbEmphasize not NULL!!!");
	FAssertMsg(GC.getNumEmphasizeInfos() > 0,  "GC.getNumEmphasizeInfos() is not greater than zero but an array is being allocated in CvCityAI::AI_reset");
	m_pbEmphasize = new bool[GC.getNumEmphasizeInfos()];
	for (iI = 0; iI < GC.getNumEmphasizeInfos(); iI++)
	{
		m_pbEmphasize[iI] = false;
	}
}


void CvCityAI::AI_doTurn()
{
	PROFILE_FUNC();

	int iI;

	if (!isHuman())
	{
		for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			setForceSpecialistCount(((SpecialistTypes)iI), 0);
		}
	}
	
    if (!isHuman())
	{
	    AI_stealPlots();
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      11/14/09                                jdog5000      */
/*                                                                                              */
/* City AI, Worker AI                                                                           */
/************************************************************************************************/
/* original bts code
	AI_updateWorkersNeededHere();
	
	AI_updateBestBuild();
*/
	if (!isDisorder()) // K-Mod
		AI_updateBestBuild();

	AI_updateWorkersNeededHere();
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	AI_updateRouteToCity();

	if (isHuman())
	{
	    if (isProductionAutomated())
	    {
	        AI_doHurry();
	    }
		return;
	}

	AI_doPanic();

	AI_doDraft();

	AI_doHurry();

	AI_doEmphasize();
}


void CvCityAI::AI_assignWorkingPlots()
{
	PROFILE_FUNC();

	CvPlot* pHomePlot;
	int iI;

	/* original bts code
	if (0 != GC.getDefineINT("AI_SHOULDNT_MANAGE_PLOT_ASSIGNMENT"))
	{
		return;
	} */ // K-Mod. that option would break a bunch of stuff.

	// remove all assigned plots if we automated
	/* original bts code
	if (!isHuman() || isCitizensAutomated())
	{
		for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			setWorkingPlot(iI, false);
		}
	} */ // Disabled by K-Mod (experimental change, for speed)

	//update the special yield multiplier to be current
	AI_updateSpecialYieldMultiplier();

	// remove any plots we can no longer work for any reason
	verifyWorkingPlots();

	// if forcing specialists, try to make all future specialists of the same type
	//bool bIsSpecialistForced = false;
	bool bIsSpecialistForced = isSpecialistForced();
	int iTotalForcedSpecialists = 0;

	// make sure at least the forced amount of specialists are assigned
	for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		int iForcedSpecialistCount = getForceSpecialistCount((SpecialistTypes)iI);
		if (iForcedSpecialistCount > 0)
		{
			bIsSpecialistForced = true;
			iTotalForcedSpecialists += iForcedSpecialistCount;
		}

		//if (!isHuman() || isCitizensAutomated() || (getSpecialistCount((SpecialistTypes)iI) < iForcedSpecialistCount))
		if (bIsSpecialistForced && !isHuman() || isCitizensAutomated() || getSpecialistCount((SpecialistTypes)iI) < iForcedSpecialistCount) // K-Mod
		{
			setSpecialistCount(((SpecialistTypes)iI), iForcedSpecialistCount);
		}
	}

	// if we have more specialists of any type than this city can have, reduce to the max
	for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		if (!isSpecialistValid((SpecialistTypes)iI))
		{
			if (getSpecialistCount((SpecialistTypes)iI) > getMaxSpecialistCount((SpecialistTypes)iI))
			{
				setSpecialistCount(((SpecialistTypes)iI), getMaxSpecialistCount((SpecialistTypes)iI));
			}
			FAssert(isSpecialistValid((SpecialistTypes)iI));
		}
	}
	
	// always work the home plot (center)
	pHomePlot = getCityIndexPlot(CITY_HOME_PLOT);
	if (pHomePlot != NULL)
	{
		setWorkingPlot(CITY_HOME_PLOT, ((getPopulation() > 0) && canWork(pHomePlot)));
	}
	
	// keep removing the worst citizen until we are not over the limit
	while (extraPopulation() < 0)
	{
		if (!AI_removeWorstCitizen())
		{
			FAssertMsg(false, "failed to remove extra population");
			break;
		}
	}
	
	// extraSpecialists() is less than extraPopulation()
	FAssertMsg(extraSpecialists() >= 0, "extraSpecialists() is expected to be non-negative (invalid Index)");

	// K-Mod. If the city is in disorder, nothing we do really matters.
	if (isDisorder())
	{
		FAssert((getWorkingPopulation() + getSpecialistPopulation()) <= (totalFreeSpecialists() + getPopulation()));
		AI_setAssignWorkDirty(false);

		if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(CitizenButtons_DIRTY_BIT, true);
		}
		return;
	}
	// K-Mod end

	// do we have population unassigned
	while (extraPopulation() > 0)
	{
		// (AI_addBestCitizen now handles forced specialist logic)
		if (!AI_addBestCitizen(/*bWorkers*/ true, /*bSpecialists*/ true))
		{
			FAssertMsg(false, "failed to assign extra population");
			break;
		}
	}

	// if forcing specialists, assign any other specialists that we must place based on forced specialists
	int iInitialExtraSpecialists = extraSpecialists();
	int iExtraSpecialists = iInitialExtraSpecialists;
	if (bIsSpecialistForced && iExtraSpecialists > 0)
	{
		FAssertMsg(iTotalForcedSpecialists > 0, "zero or negative total forced specialists");
		for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			if (isSpecialistValid((SpecialistTypes)iI, 1))
			{
				int iForcedSpecialistCount = getForceSpecialistCount((SpecialistTypes)iI);
				if (iForcedSpecialistCount > 0)
				{
					int iSpecialistCount = getSpecialistCount((SpecialistTypes)iI);
					int iMaxSpecialistCount = getMaxSpecialistCount((SpecialistTypes)iI);

					int iSpecialistsToAdd = ((iInitialExtraSpecialists * iForcedSpecialistCount) + (iTotalForcedSpecialists/2)) / iTotalForcedSpecialists;
					if (iExtraSpecialists < iSpecialistsToAdd)
					{
						iSpecialistsToAdd = iExtraSpecialists;
					}
					
					iSpecialistCount += iSpecialistsToAdd;
					iExtraSpecialists -= iSpecialistsToAdd;

					// if we cannot fit that many, then add as many as we can
					if (iSpecialistCount > iMaxSpecialistCount && !GET_PLAYER(getOwnerINLINE()).isSpecialistValid((SpecialistTypes)iI))
					{
						iExtraSpecialists += iSpecialistCount - iMaxSpecialistCount;
						iSpecialistCount = iMaxSpecialistCount;
					}

					setSpecialistCount((SpecialistTypes)iI, iSpecialistCount);
				}
			}
		}
	}
	FAssertMsg(iExtraSpecialists >= 0, "added too many specialists");

	// if we still have population to assign, assign specialists
	while (extraSpecialists() > 0)
	{
		if (!AI_addBestCitizen(/*bWorkers*/ false, /*bSpecialists*/ true))
		{
			FAssertMsg(false, "failed to assign extra specialist");
			break;
		}
	}

	// if automated, look for better choices than the current ones
	if (!isHuman() || isCitizensAutomated())
	{
		AI_juggleCitizens();
	}

	// at this point, we should not be over the limit
	FAssert((getWorkingPopulation() + getSpecialistPopulation()) <= (totalFreeSpecialists() + getPopulation()));
	FAssert(extraPopulation() == 0); // K-Mod.

	AI_setAssignWorkDirty(false);

	if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && isCitySelected())
	{
		gDLL->getInterfaceIFace()->setDirty(CitizenButtons_DIRTY_BIT, true);
	}
}


void CvCityAI::AI_updateAssignWork()
{
	if (AI_isAssignWorkDirty())
	{
		AI_assignWorkingPlots();
	}
}


bool CvCityAI::AI_avoidGrowth()
{
	//PROFILE_FUNC();

	if (AI_isEmphasizeAvoidGrowth())
	{
		return true;
	}

	return false; // K-Mod. short-circuit all the other stuff. That's all taken into account in other places.

	if (isFoodProduction())
	{
		return true;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/14/10                                jdog5000      */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
	//if (!AI_isEmphasizeYield(YIELD_FOOD) && !AI_isEmphasizeGreatPeople())
	// AI should avoid growth when it has angry citizens, even if emphasizing great people
	if( !(isHuman()) || (!AI_isEmphasizeYield(YIELD_FOOD) && !AI_isEmphasizeGreatPeople()) )
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	{
		int iExtra = (isHuman()) ? 0 : 1;
		int iHappinessLevel = happyLevel() - unhappyLevel(iExtra);
		
		// ignore military unhappy, since we assume it will be fixed quickly, grow anyway
		if (getMilitaryHappinessUnits() == 0)
		{
			iHappinessLevel += ((GC.getDefineINT("NO_MILITARY_PERCENT_ANGER") * (getPopulation() + 1)) / GC.getPERCENT_ANGER_DIVISOR());
		}
		

		// if we can pop rush, we want to grow one over the cap
		if (GET_PLAYER(getOwnerINLINE()).canPopRush())
		{
			iHappinessLevel++;
		}

		
		// if we have angry citizens
		if (iHappinessLevel < 0)
		{
			return true;
		}
	}

	return false;
}


bool CvCityAI::AI_ignoreGrowth()
{
	PROFILE_FUNC();

	if (!AI_isEmphasizeYield(YIELD_FOOD) && !AI_isEmphasizeGreatPeople())
	{
		if (!AI_foodAvailable((isHuman()) ? 0 : 1))
		{
			return true;
		}
	}

	return false;
}

// (this function has been edited heavily for K-Mod)
int CvCityAI::AI_specialistValue(SpecialistTypes eSpecialist, bool bAvoidGrowth, bool bRemove) const
{
	PROFILE_FUNC();

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	short aiYields[NUM_YIELD_TYPES];
	int iValue;
	int iNumCities = kOwner.getNumCities();
    
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		aiYields[iI] = kOwner.specialistYield(eSpecialist, ((YieldTypes)iI));
	}

	short int aiCommerceYields[NUM_COMMERCE_TYPES];
	
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		aiCommerceYields[iI] = kOwner.specialistCommerce(eSpecialist, ((CommerceTypes)iI));
	}
	
	iValue = AI_yieldValue(aiYields, aiCommerceYields, bAvoidGrowth, bRemove) * 100;

	int iGreatPeopleRate = GC.getSpecialistInfo(eSpecialist).getGreatPeopleRateChange();

	int iEmphasisCount = 0;
	if (iGreatPeopleRate != 0)
	{
		int iGPPValue = 4;
		if (AI_isEmphasizeGreatPeople())
		{
			//iGPPValue = isHuman() ? 30 : 20;
			iGPPValue = 12; // K-Mod. If the value is too high, it becomes worth more than city growth
		}
		else
		{
			if (AI_isEmphasizeYield(YIELD_COMMERCE))
			{
				iGPPValue = 2;
				iEmphasisCount++;			
			}
			if (AI_isEmphasizeYield(YIELD_PRODUCTION))
			{
				iGPPValue = 1;
				iEmphasisCount++;
			}
			if (AI_isEmphasizeYield(YIELD_FOOD))
			{
				iGPPValue = 1;
				iEmphasisCount++;
			}
		}
		
		int iTempValue = 100 * iGreatPeopleRate * iGPPValue;
		
//		if (isHuman() && (getGreatPeopleUnitRate(iGreatPeopleType) == 0)
//			&& (getForceSpecialistCount(eSpecialist) == 0) && !AI_isEmphasizeGreatPeople())
//		{
//			iTempValue -= (iGreatPeopleRate * 4);			
//		}

		if (!isHuman() || AI_isEmphasizeGreatPeople())
		{
			int iProgress = getGreatPeopleProgress();
			if (iProgress > 0)
			{
				int iThreshold = kOwner.greatPeopleThreshold();
				//iTempValue += 100*(iGreatPeopleRate * (isHuman() ? 1 : 4) * iGPPValue * iProgress * iProgress) / (iThreshold * iThreshold);
				// K-Mod. The original code overflows the int when iProgress is big.
				int iCloseBonus = 100 * iGreatPeopleRate * (isHuman() ? 1 : 4) * iGPPValue * iProgress / iThreshold;
				iCloseBonus *= iProgress;
				iCloseBonus /= iThreshold;
				iTempValue += iCloseBonus;
				// K-Mod end
			}
		}
		
		/*
		int iCurrentEra = kOwner.getCurrentEra();
		int iTotalEras = GC.getNumEraInfos();
		
		if (kOwner.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2))
		{
			int iUnitClass = GC.getSpecialistInfo(eSpecialist).getGreatPeopleUnitClass();
			FAssert(iUnitClass != NO_UNITCLASS);
			
			UnitTypes eGreatPeopleUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iUnitClass);
			if (eGreatPeopleUnit != NO_UNIT)
			{
				CvUnitInfo& kUnitInfo = GC.getUnitInfo(eGreatPeopleUnit);
				if (kUnitInfo.getGreatWorkCulture() > 0)
				{
					iTempValue += 100 * iGreatPeopleRate * kUnitInfo.getGreatWorkCulture() * (std::max(2*iTotalEras/3, (int)kOwner.getCurrentEra())) / ((kOwner.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3)) ? 500 : 700);
				}
			}
		}

        if (!isHuman() && (iCurrentEra <= ((iTotalEras * 2) / 3)))
        {
            // try to spawn a prophet for any shrines we have yet to build
            bool bNeedProphet = false;
            int iBestSpreadValue = 0;

			
			for (int iJ = 0; iJ < GC.getNumReligionInfos(); iJ++)
            {
                ReligionTypes eReligion = (ReligionTypes) iJ;

                if (isHolyCity(eReligion) && !hasShrine(eReligion)
                	&& ((iCurrentEra < iTotalEras / 2) || GC.getGameINLINE().countReligionLevels(eReligion) >= 10))
                {
					CvCivilizationInfo* pCivilizationInfo = &GC.getCivilizationInfo(getCivilizationType());
					
					int iUnitClass = GC.getSpecialistInfo(eSpecialist).getGreatPeopleUnitClass();
                    FAssert(iUnitClass != NO_UNITCLASS);
                    
					UnitTypes eGreatPeopleUnit = (UnitTypes) pCivilizationInfo->getCivilizationUnits(iUnitClass);
					if (eGreatPeopleUnit != NO_UNIT)
					{
						// note, for normal XML, this count will be one (there is only 1 shrine building for each religion)
						int	shrineBuildingCount = GC.getGameINLINE().getShrineBuildingCount(eReligion);
						for (int iI = 0; iI < shrineBuildingCount; iI++)
						{
							int eBuilding = (int) GC.getGameINLINE().getShrineBuilding(iI, eReligion);
							
							// if this unit builds or forceBuilds this building
							if (GC.getUnitInfo(eGreatPeopleUnit).getBuildings(eBuilding) || GC.getUnitInfo(eGreatPeopleUnit).getForceBuildings(eBuilding))
							{
								bNeedProphet = true;
								iBestSpreadValue = std::max(iBestSpreadValue, GC.getGameINLINE().countReligionLevels(eReligion));
							}
						}
				
					}
				}
			}

			if (bNeedProphet)
            {
                iTempValue += 100 * iGreatPeopleRate * iBestSpreadValue;
            }
		} */

		// K-Mod: I've replaced the above code with a new method for targeting particular great person types.
		if (!isHuman())
		{
			iTempValue *= kOwner.AI_getGreatPersonWeight((UnitClassTypes)GC.getSpecialistInfo(eSpecialist).getGreatPeopleUnitClass());
			iTempValue /= 100;
		}

		// Scale based on how often this city will actually get a great person.
		if (!AI_isEmphasizeGreatPeople())
		{
			int iCityRate = getGreatPeopleRate();
			int iHighestRate = 0;
			int iLoop;
			for (CvCity* pLoopCity = kOwner.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kOwner.nextCity(&iLoop))
			{
				iHighestRate = std::max(iHighestRate, pLoopCity->getGreatPeopleRate());
			}
			if (iHighestRate > iCityRate)
			{
				iTempValue *= 100;
				iTempValue /= (2*100*(iHighestRate+4))/(iCityRate+4) - 100;
			}
			// each successive great person costs more points. So the points are effectively worth less...
			// (note: I haven't tried to match this value decrease with the actual cost increase,
			// because the value of the great people changes as well.)
			iTempValue *= 100;
			iTempValue /= 90 + 10 * kOwner.getGreatPeopleCreated();
		}

		iTempValue *= getTotalGreatPeopleRateModifier();
		//iTempValue /= kOwner.AI_averageGreatPeopleMultiplier();
		// K-Mod note: ultimately, I don't think the value should be divided by the average multiplier.
		// because more great people points is always better, regardless of what the average multiplier is.
		// However, because of the flawed way that food is currently evaluated, I need to dilute the value of GPP
		// so that specialists don't get value more highly than food tiles. (I hope to correct this, later.)
		iTempValue /= (200 + kOwner.AI_averageGreatPeopleMultiplier())/3;
		
		iTempValue /= (1 + iEmphasisCount);
		iValue += iTempValue;
	}
	else
	{
		SpecialistTypes eGenericCitizen = (SpecialistTypes) GC.getDefineINT("DEFAULT_SPECIALIST");
		
		// are we the generic specialist?
		if (eSpecialist == eGenericCitizen)
		{
			iValue *= 60;
			iValue /= 100;
		}
	}
	
	int iExperience = GC.getSpecialistInfo(eSpecialist).getExperience();
	if (0 != iExperience)
	{
		int iProductionRank = findYieldRateRank(YIELD_PRODUCTION);

		iValue += 100 * iExperience * 4;
		if (iProductionRank <= iNumCities/2 + 1)
		{
			iValue += 100 * iExperience *  4;
		}
		iValue += (getMilitaryProductionModifier() * iExperience * 8);
	}

	return iValue;
}

// K-Mod. The value of a long-term specialist, for use in calculating great person value, and value of free specialists from buildings.
// value is roughly 4 * 100 * commerce
int CvCityAI::AI_permanentSpecialistValue(SpecialistTypes eSpecialist) const
{
	const CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());

	const int iCommerceValue = 4;
	const int iProdValue = 7;
	const int iFoodValue = 11;

	int iValue = 0;

	// AI_getYieldMultipliers takes a little bit of time, so lets only do it if we need to.
	bool bHasYield = false;
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		if (kPlayer.specialistYield(eSpecialist, (YieldTypes)iI) != 0)
		{
			bHasYield = true;
			break;
		}
	}

	if (bHasYield)
	{
		int iFoodX, iProdX, iComX, iFoodChange;
		AI_getYieldMultipliers(iFoodX, iProdX, iComX, iFoodChange);
		iValue += iFoodValue * kPlayer.specialistYield(eSpecialist, YIELD_FOOD) * AI_yieldMultiplier(YIELD_FOOD) * iFoodX / 100;
		iValue += iProdValue * kPlayer.specialistYield(eSpecialist, YIELD_PRODUCTION) * AI_yieldMultiplier(YIELD_PRODUCTION) * iProdX / 100;
		iValue += iCommerceValue * kPlayer.specialistYield(eSpecialist, YIELD_COMMERCE) * AI_yieldMultiplier(YIELD_COMMERCE) * iComX / 100;
	}

	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		int iTemp = iCommerceValue * kPlayer.specialistCommerce(eSpecialist, (CommerceTypes)iI);
		if (iTemp != 0)
		{
			iTemp *= getTotalCommerceRateModifier((CommerceTypes)iI);
			iTemp *= kPlayer.AI_commerceWeight((CommerceTypes)iI, this);
			iTemp /= 100;
			iValue += iTemp;
		}
	}
	
	int iGreatPeopleRate = GC.getSpecialistInfo(eSpecialist).getGreatPeopleRateChange();

	if (iGreatPeopleRate != 0)
	{
		int iGPPValue = 4;
		
		int iTempValue = 100 * iGreatPeopleRate * iGPPValue;
		
		// Scale based on how often this city will actually get a great person.
		/* Actually... don't do that. That's not the kind of thing we should take into account for permanent specialists.
		int iCityRate = getGreatPeopleRate();
		int iHighestRate = 0;
		int iLoop;
		for( CvCity* pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop) )
		{
			int x = pLoopCity->getGreatPeopleRate();
			if (x > iHighestRate)
				iHighestRate = x;
		}
		if (iHighestRate > iCityRate)
		{
			iTempValue *= 100;
			iTempValue /= (2*100*(iHighestRate+3))/(iCityRate+3) - 100;
		} */
		iTempValue *= kPlayer.AI_getGreatPersonWeight((UnitClassTypes)GC.getSpecialistInfo(eSpecialist).getGreatPeopleUnitClass());
		iTempValue /= 100;
		
		iTempValue *= getTotalGreatPeopleRateModifier();
		iTempValue /= 100;
		
		iValue += iTempValue;
	}
	
	int iExperience = GC.getSpecialistInfo(eSpecialist).getExperience();
	if (0 != iExperience)
	{
		int iProductionRank = findYieldRateRank(YIELD_PRODUCTION);
		int iHasMetCount = GET_TEAM(getTeam()).getHasMetCivCount(true);

		int iTempValue = 100 * iExperience * ((iHasMetCount > 0) ? 4 : 2);
		if (iProductionRank <= kPlayer.getNumCities()/2 + 1)
		{
			iTempValue += 100 * iExperience *  4;
		}
		iTempValue += (getMilitaryProductionModifier() * iExperience * 6); // was * 8

		iTempValue *= 100;
		iTempValue /= (100+15*(getFreeExperience()/5));

		iValue += iTempValue;
	}

	return iValue;
}

// Heavily edited by K-Mod. (note, I've deleted a lot of the old code from BtS and from BBAI, and some of my changes are unmarked.)
void CvCityAI::AI_chooseProduction()
{
	PROFILE_FUNC();

	bool bWasFoodProduction = isFoodProduction();
	bool bDanger = AI_isDanger();

	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());

	if (isProduction())
	{
		if (getProduction() > 0)
		{

			if ((getProductionUnitAI() == UNITAI_SETTLE) && kPlayer.AI_isFinancialTrouble())
			{

			}
			//if we are killing our growth to train this, then finish it.
			else if (!bDanger && isFoodProduction())
			{
				if ((area()->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE))
				{
					return;
				}
			}
			// if less than 3 turns left, keep building current item
			else if (getProductionTurnsLeft() <= 3)
			{
				return;
			}

			// if building a combat unit, and we have no defenders, keep building it
			UnitTypes eProductionUnit = getProductionUnit();
			if (eProductionUnit != NO_UNIT)
			{
				if (plot()->getNumDefenders(getOwnerINLINE()) == 0)
				{
					if (GC.getUnitInfo(eProductionUnit).getCombat() > 0)
					{
						return;
					}
				}
			}

			// if we are building a wonder, do not cancel, keep building it (if no danger)
			BuildingTypes eProductionBuilding = getProductionBuilding();
			if (!bDanger && eProductionBuilding != NO_BUILDING && 
				isLimitedWonderClass((BuildingClassTypes) GC.getBuildingInfo(eProductionBuilding).getBuildingClassType()))
			{
				return;
			}
		}

		clearOrderQueue();
	}

	//if (GET_PLAYER(getOwner()).isAnarchy()) // original bts code
	if (isDisorder()) // K-Mod
	{
		return;
	}

	// only clear the dirty bit if we actually do a check, multiple items might be queued
	AI_setChooseProductionDirty(false);
	if (bWasFoodProduction)
	{
		AI_assignWorkingPlots();
	}

	// allow python to handle it
	if (GC.getUSE_AI_CHOOSE_PRODUCTION_CALLBACK()) // K-Mod. block unused python callbacks
	{
		CyCity* pyCity = new CyCity(this);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));	// pass in city class
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_chooseProduction", argsList.makeFunctionArgs(), &lResult);
		delete pyCity;	// python fxn must not hold on to this pointer
		if (lResult == 1)
		{
			return;
		}
	}

	if (isHuman() && isProductionAutomated())
	{
		AI_buildGovernorChooseProduction();
		return;
	}

	if (isBarbarian())
	{
		AI_barbChooseProduction();
		return;
	}

	CvArea* pArea = area();
	CvArea* pWaterArea = waterArea(true);
	bool bMaybeWaterArea = false;
	bool bWaterDanger = false;

	if (pWaterArea != NULL)
	{
		bMaybeWaterArea = true;
		if (!GET_TEAM(getTeam()).AI_isWaterAreaRelevant(pWaterArea))
		{
			pWaterArea = NULL;
		}

		bWaterDanger = kPlayer.AI_getWaterDanger(plot(), 4) > 0;
	}

	bool bHasMetHuman = GET_TEAM(getTeam()).hasMetHuman();
	bool bLandWar = ((pArea->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) || (pArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE) || (pArea->getAreaAIType(getTeam()) == AREAAI_MASSING));
	bool bDefenseWar = (pArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE);
	bool bAssaultAssist = (pArea->getAreaAIType(getTeam()) == AREAAI_ASSAULT_ASSIST);
	bool bTotalWar = GET_TEAM(getTeam()).getWarPlanCount(WARPLAN_TOTAL, true); // K-Mod
	bool bAssault = bAssaultAssist || (pArea->getAreaAIType(getTeam()) == AREAAI_ASSAULT) || (pArea->getAreaAIType(getTeam()) == AREAAI_ASSAULT_MASSING);
	bool bPrimaryArea = kPlayer.AI_isPrimaryArea(pArea);
	bool bFinancialTrouble = kPlayer.AI_isFinancialTrouble();
	int iNumCitiesInArea = pArea->getCitiesPerPlayer(getOwnerINLINE());
	bool bImportantCity = false; //be very careful about setting this.
	int iCultureRateRank = findCommerceRateRank(COMMERCE_CULTURE);
	int iCulturalVictoryNumCultureCities = GC.getGameINLINE().culturalVictoryNumCultureCities();

	int iWarSuccessRating = GET_TEAM(getTeam()).AI_getWarSuccessRating();
	int iEnemyPowerPerc = GET_TEAM(getTeam()).AI_getEnemyPowerPercent(true);


	if( !bLandWar && !bAssault && GET_TEAM(getTeam()).isAVassal() )
	{
		bLandWar = GET_TEAM(getTeam()).isMasterPlanningLandWar(area());

		if( !bLandWar )
		{
			bAssault = GET_TEAM(getTeam()).isMasterPlanningSeaWar(area());
		}
	}

	bool bGetBetterUnits = kPlayer.AI_isDoStrategy(AI_STRATEGY_GET_BETTER_UNITS);
	bool bAggressiveAI = GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI);
	bool bAlwaysPeace = GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE);

	/* original bts code
	int iUnitCostPercentage = (kPlayer.calculateUnitCost() * 100) / std::max(1, kPlayer.calculatePreInflatedCosts()); */
	int iUnitSpending = kPlayer.AI_unitCostPerMil(); // K-Mod. (note, this is around 3x bigger than the original formula)
	int iWaterPercent = AI_calculateWaterWorldPercent();

	int iBuildUnitProb = AI_buildUnitProb();
	iBuildUnitProb /= kPlayer.AI_isDoStrategy(AI_STRATEGY_ECONOMY_FOCUS) ? 2 : 1; // K-Mod

	int iExistingWorkers = kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_WORKER);
	int iNeededWorkers = kPlayer.AI_neededWorkers(pArea);
	// Sea worker need independent of whether water area is militarily relevant
	int iNeededSeaWorkers = (bMaybeWaterArea) ? AI_neededSeaWorkers() : 0;
	int iExistingSeaWorkers = (waterArea(true) != NULL) ? kPlayer.AI_totalWaterAreaUnitAIs(waterArea(true), UNITAI_WORKER_SEA) : 0;


	int iTargetCulturePerTurn = AI_calculateTargetCulturePerTurn();

	int iAreaBestFoundValue;
	int iNumAreaCitySites = kPlayer.AI_getNumAreaCitySites(getArea(), iAreaBestFoundValue);

	int iWaterAreaBestFoundValue = 0;
	CvArea* pWaterSettlerArea = pWaterArea;
	if( pWaterSettlerArea == NULL )
	{
		pWaterSettlerArea = GC.getMap().findBiggestArea(true);

		if( GET_PLAYER(getOwnerINLINE()).AI_totalWaterAreaUnitAIs(pWaterSettlerArea, UNITAI_SETTLER_SEA) == 0 )
		{
			pWaterSettlerArea = NULL;
		}
	}
	int iNumWaterAreaCitySites = (pWaterSettlerArea == NULL) ? 0 : kPlayer.AI_getNumAdjacentAreaCitySites(pWaterSettlerArea->getID(), getArea(), iWaterAreaBestFoundValue);
	int iNumSettlers = kPlayer.AI_totalUnitAIs(UNITAI_SETTLE);

	bool bIsCapitalArea = false;
	int iNumCapitalAreaCities = 0;
	if (kPlayer.getCapitalCity() != NULL)
	{
		iNumCapitalAreaCities = kPlayer.getCapitalCity()->area()->getCitiesPerPlayer(getOwnerINLINE());
		if (getArea() == kPlayer.getCapitalCity()->getArea())
		{
			bIsCapitalArea = true;
		}
	}

	int iMaxSettlers = 0;
	if (!bFinancialTrouble)
	{
		iMaxSettlers= std::min((kPlayer.getNumCities() + 1) / 2, iNumAreaCitySites + iNumWaterAreaCitySites);
		if (bLandWar || bAssault)
		{
			iMaxSettlers = (iMaxSettlers + 2) / 3;
		}
	}

	bool bChooseWorker = false;

	if (iNumCitiesInArea > 2)
	{
		if (kPlayer.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2))
		{
			if (iCultureRateRank <= iCulturalVictoryNumCultureCities + 1)
			{
				// if we do not have enough cities, then the highest culture city will not get special attention
				if (iCultureRateRank > 1 || (kPlayer.getNumCities() > (iCulturalVictoryNumCultureCities + 1)))
				{
					if ((((iNumAreaCitySites + iNumWaterAreaCitySites) > 0) && (kPlayer.getNumCities() < 6)) && (GC.getGameINLINE().getSorenRandNum(2, "AI Less Culture More Expand") == 0))
					{
						bImportantCity = false;
					}
					else
					{
						bImportantCity = true;
					}
				}
			}
		}
	}

	// Free experience for various unit domains
	int iFreeLandExperience = getSpecialistFreeExperience() + getDomainFreeExperience(DOMAIN_LAND);
	int iFreeSeaExperience = getSpecialistFreeExperience() + getDomainFreeExperience(DOMAIN_SEA);
	int iFreeAirExperience = getSpecialistFreeExperience() + getDomainFreeExperience(DOMAIN_AIR);

	int iProductionRank = findYieldRateRank(YIELD_PRODUCTION);

	// K-Mod.
	BuildingTypes eBestBuilding = AI_bestBuildingThreshold(); // go go value cache!
	int iBestBuildingValue = (eBestBuilding == NO_BUILDING) ? 0 : AI_buildingValue(eBestBuilding);
	// for the purpose of adjusting production probabilities, scale the building value up for early eras
	// (because early game buildings are relatively weaker)
	if (GC.getNumEraInfos() > 1)
	{
		FAssert(kPlayer.getCurrentEra() < GC.getNumEraInfos());
		iBestBuildingValue *= 2*(GC.getNumEraInfos()-1) - kPlayer.getCurrentEra();
		iBestBuildingValue /= GC.getNumEraInfos()-1;
	}
	// also, reduce the value to encourage early expansion until we reach the recommend city target
	{
		int iTargetCities = GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities();
		if (iNumAreaCitySites > 0 && kPlayer.getNumCities() < iTargetCities)
		{
			iBestBuildingValue *= kPlayer.getNumCities() + iTargetCities;
			iBestBuildingValue /= 2*iTargetCities;
		}
	}

	// Check for military exemption for commerce cities and underdeveloped cities.
	// Don't give exemptions to cities that don't have anything good to build anyway.
	bool bUnitExempt = false;
	if (iBestBuildingValue >= 40)
	{
		if (iProductionRank-1 > kPlayer.getNumCities()/2)
		{
			bool bBelowMedian = true;
			for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
			{
				// I'd use the total commerce rank, but there currently isn't a cached value of that.
				int iRank = findCommerceRateRank((CommerceTypes)iI);
				if (iRank < iProductionRank)
				{
					bUnitExempt = true;
					break;
				}
				if (iRank-1 < kPlayer.getNumCities()/2)
					bBelowMedian = false;
			}

			if (bBelowMedian)
				bUnitExempt = true;
		}
	}
	// K-Mod end


	if( gCityLogLevel >= 3 ) logBBAI("      City %S pop %d considering new production: iProdRank %d, iBuildUnitProb %d%s, iBestBuildingValue %d", getName().GetCString(), getPopulation(), iProductionRank, iBuildUnitProb, bUnitExempt?"*":"", iBestBuildingValue);

	// if we need to pop borders, then do that immediately if we have drama and can do it
	if ((iTargetCulturePerTurn > 0) && (getCultureLevel() <= (CultureLevelTypes) 1))
	{
		// K-Mod. If our best building is a cultural building, just start building it.
		if (eBestBuilding != NO_BUILDING && AI_countGoodTiles(true, false) > 0)
		{
			const CvBuildingInfo& kBestBuilding = GC.getBuildingInfo(eBestBuilding);
			if (kBestBuilding.getCommerceChange(COMMERCE_CULTURE) + kBestBuilding.getObsoleteSafeCommerceChange(COMMERCE_CULTURE) > 0
				&& (GC.getNumCultureLevelInfos() < 2 || getProductionTurnsLeft(eBestBuilding, 0) <= GC.getGame().getCultureThreshold((CultureLevelTypes)2)))
			{
				pushOrder(ORDER_CONSTRUCT, eBestBuilding, -1, false, false, false);
				return;
			}
		}
		// K-Mod end
		if (AI_chooseProcess(COMMERCE_CULTURE))
		{
			return;
		}
	}

	if (plot()->getNumDefenders(getOwnerINLINE()) == 0) // XXX check for other team's units?
	{
		if( gCityLogLevel >= 2 ) logBBAI("      City %S uses no defenders", getName().GetCString());

		if (AI_chooseUnit(UNITAI_CITY_DEFENSE))
		{
			return;
		}

		if (AI_chooseUnit(UNITAI_CITY_COUNTER))
		{
			return;
		}

		if (AI_chooseUnit(UNITAI_CITY_SPECIAL))
		{
			return;
		}

		if (AI_chooseUnit(UNITAI_ATTACK))
		{
			return;
		}
	}

	if( kPlayer.isStrike() )
	{
		int iStrikeFlags = 0;
		iStrikeFlags |= BUILDINGFOCUS_GOLD;
		iStrikeFlags |= BUILDINGFOCUS_MAINTENANCE;

		if(AI_chooseBuilding(iStrikeFlags))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses strike building (w/ flags)", getName().GetCString());
			return;
		}

		if (AI_chooseBuilding())
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses strike building (w/o flags)", getName().GetCString());
			return;
		}
	}

	// K-Mod, short-circuit production choice if we already have something really good in mind
	if (kPlayer.getNumCities() > 1) // don't use this short circuit if this is our only city.
	{
		int iOdds = std::max(0, 100 * iBestBuildingValue / (3 * iBestBuildingValue + 300) - 10);
		if (AI_chooseBuilding(0, INT_MAX, 0, iOdds))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses building value short-circuit 1 (odds: %d)", getName().GetCString(), iOdds);
			return;
		}
	}
	// K-Mod end

	// So what's the right detection of defense which works in early game too?
	int iPlotSettlerCount = (iNumSettlers == 0) ? 0 : plot()->plotCount(PUF_isUnitAIType, UNITAI_SETTLE, -1, getOwnerINLINE());
	int iPlotCityDefenderCount = plot()->plotCount(PUF_isUnitAIType, UNITAI_CITY_DEFENSE, -1, getOwnerINLINE());
	if( kPlayer.getCurrentEra() == 0 )
	{
		// Warriors are blocked from UNITAI_CITY_DEFENSE, in early game this confuses AI city building
		if( kPlayer.AI_totalUnitAIs(UNITAI_CITY_DEFENSE) <= kPlayer.getNumCities() )
		{
			if( kPlayer.AI_bestCityUnitAIValue(UNITAI_CITY_DEFENSE, this) == 0 )
			{
				iPlotCityDefenderCount = plot()->plotCount(PUF_canDefend, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isDomainType, DOMAIN_LAND);
			}
		}
	}

	//minimal defense.
	//if (iPlotCityDefenderCount <= iPlotSettlerCount)
	if (!bUnitExempt && iPlotSettlerCount > 0 && iPlotCityDefenderCount <= iPlotSettlerCount)
	{
		if( gCityLogLevel >= 2 ) logBBAI("      City %S needs escort for existing settler", getName().GetCString());
		if (AI_chooseUnit(UNITAI_CITY_DEFENSE))
		{
			// BBAI TODO: Does this work right after settler is built???
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses escort existing settler 1 defense", getName().GetCString());
			return;
		}

		if (AI_chooseUnit(UNITAI_ATTACK))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses escort existing settler 1 attack", getName().GetCString());
			return;
		}
	}
    
	if (((iTargetCulturePerTurn > 0) || (getPopulation() > 5)) && (getCommerceRate(COMMERCE_CULTURE) == 0))
	{
		if( !(kPlayer.AI_isDoStrategy(AI_STRATEGY_TURTLE)) )
		{
			if (AI_chooseBuilding(BUILDINGFOCUS_CULTURE, 30))
			{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S uses zero culture build", getName().GetCString());
				return;
			}
		}
	}

	// Early game worker logic
/***
**** K-Mod, 10/sep/10, Karadoc
**** Changed "AI_totalBestBuildValue(area()) > 10" to "(AI_totalBestBuildValue(area())+50*iLandBonuses) > 100"
**** Also, I moved the iLandBonuses declaration here. It use to be lower down.
***/
	int iLandBonuses = AI_countNumImprovableBonuses(true, kPlayer.getCurrentResearch());

	if( isCapital() && (GC.getGame().getElapsedGameTurns() < ((30 * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent()) / 100)))
	{
		if( !bDanger && !(kPlayer.AI_isDoStrategy(AI_STRATEGY_TURTLE)) )
		{	
			if (!bWaterDanger && (getPopulation() < 3) && (iNeededSeaWorkers > 0))
			{
				if (iExistingSeaWorkers == 0)
				{
					// Build workboat first since it doesn't stop growth
					if (AI_chooseUnit(UNITAI_WORKER_SEA))
					{
						if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose worker sea 1a", getName().GetCString());
						return;
					}
				}
			}

			//if( iExistingWorkers == 0 && AI_totalBestBuildValue(area()) > 10)
			if (iExistingWorkers == 0 && AI_totalBestBuildValue(area())+50*iLandBonuses > 100)
			{
				if (!bChooseWorker && AI_chooseUnit(UNITAI_WORKER))
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose worker 1a", getName().GetCString());
					return;
				}
				bChooseWorker = true;
			}
		}
	}

	if( !(bDefenseWar && iWarSuccessRating < -50) && !bDanger )
	{
		if ((iExistingWorkers == 0))
		{
/***
**** K-Mod, 10/sep/10, Karadoc
**** I've taken iLandBonuses from here and moved it higher for use elsewhere.
***/
			//int iLandBonuses = AI_countNumImprovableBonuses(true, kPlayer.getCurrentResearch());
			if ((iLandBonuses > 1) || (getPopulation() > 3 && iNeededWorkers > 0))
			{
				if (!bChooseWorker && AI_chooseUnit(UNITAI_WORKER))
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose worker 1", getName().GetCString());
					return;
				}
				bChooseWorker = true;
			}

			if (!bWaterDanger && (iNeededSeaWorkers > iExistingSeaWorkers) && (getPopulation() < 3))
			{
				if (AI_chooseUnit(UNITAI_WORKER_SEA))
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose worker sea 1", getName().GetCString());
					return;
				}
			}

			if (iLandBonuses >= 1  && getPopulation() > 1)
    		{
				if (!bChooseWorker && AI_chooseUnit(UNITAI_WORKER))
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose worker 2", getName().GetCString());
					return;
				}
				bChooseWorker = true;
    		}
		}
	}

	/* original bts code (disabled by K-Mod. I don't think these are helpful)
	if ( kPlayer.AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION3) )
	{
        if ((goodHealth() - badHealth(true, 0)) < 1)
		{
			if ( AI_chooseBuilding(BUILDINGFOCUS_HEALTHY, 20, 0, (kPlayer.AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION4) ? 50 : 20)) )
			{
				return;
			}
		}
	}

	if( GET_TEAM(getTeam()).isAVassal() && GET_TEAM(getTeam()).isCapitulated() )
	{
		if( !bLandWar )
		{
			if ((goodHealth() - badHealth(true, 0)) < 1)
			{
				if (AI_chooseBuilding(BUILDINGFOCUS_HEALTHY, 30, 0, 3*getPopulation()))
				{
					return;
				}
			}

			if ((getPopulation() > 3) && (getCommerceRate(COMMERCE_CULTURE) < 5))
			{
				if (AI_chooseBuilding(BUILDINGFOCUS_CULTURE, 30, 0 + 3*iWarTroubleThreshold, 3*getPopulation()))
				{
					return;
				}
			}
		}
	}*/

	// -------------------- BBAI Notes -------------------------
	// Minimal attack force, both land and sea
    if (bDanger) 
    {
		int iAttackNeeded = 4;
		iAttackNeeded += std::max(0, AI_neededDefenders() - plot()->plotCount(PUF_isUnitAIType, UNITAI_CITY_DEFENSE, -1, getOwnerINLINE()));

		if( kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_ATTACK) <  iAttackNeeded)
		{
    		if (AI_chooseUnit(UNITAI_ATTACK))
    		{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S uses danger minimal attack", getName().GetCString());
    			return;
    		}
		}
    }
    
    if (bMaybeWaterArea)
	{
		if( !(bLandWar && iWarSuccessRating < -30) && !bDanger && !bFinancialTrouble )
		{
			if (kPlayer.AI_getNumTrainAIUnits(UNITAI_ATTACK_SEA) + kPlayer.AI_getNumTrainAIUnits(UNITAI_PIRATE_SEA) + kPlayer.AI_getNumTrainAIUnits(UNITAI_RESERVE_SEA) < std::min(3,kPlayer.getNumCities()))
			{
				if ((bMaybeWaterArea && bWaterDanger)
					|| (pWaterArea != NULL && bPrimaryArea && kPlayer.AI_countNumAreaHostileUnits(pWaterArea, true, false, false, false) > 0))
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses minimal naval", getName().GetCString());

					if (AI_chooseUnit(UNITAI_ATTACK_SEA))
					{
						return;
					}
					if (AI_chooseUnit(UNITAI_PIRATE_SEA))
					{
						return;
					}
					if (AI_chooseUnit(UNITAI_RESERVE_SEA))
					{
						return;
					}
				}
			}
		
			if (NULL != pWaterArea)
			{
				int iOdds = -1;
				if (iAreaBestFoundValue == 0 || iWaterAreaBestFoundValue > iAreaBestFoundValue)
				{
					iOdds = 100;
				}
				else if (iWaterPercent > 60)
				{
					iOdds = 13;
				}

				if( iOdds >= 0 )
				{
					// if (kPlayer.AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_EXPLORE_SEA) == 0) // original code
					if (kPlayer.AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_EXPLORE_SEA) == 0 && kPlayer.AI_neededExplorers(pWaterArea) > 0) // K-Mod!
					{
						if (AI_chooseUnit(UNITAI_EXPLORE_SEA, iOdds))
						{
							if( gCityLogLevel >= 2 ) logBBAI("      City %S uses early sea explore", getName().GetCString());
							return;
						}
					}

					// BBAI TODO: Really only want to do this if no good area city sites ... 13% chance on water heavy maps
					// of slow start, little benefit
					if (kPlayer.AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_SETTLER_SEA) == 0)
					{
						if (AI_chooseUnit(UNITAI_SETTLER_SEA, iOdds))
						{
							if( gCityLogLevel >= 2 ) logBBAI("      City %S uses early settler sea", getName().GetCString());
							return;
						}
					}
				}
			}
		}
	}
    
	// -------------------- BBAI Notes -------------------------
	// Top normal priorities
	
	/* if (!bPrimaryArea && !bLandWar)
	{
		if (AI_chooseBuilding(BUILDINGFOCUS_FOOD, 60, 10 + 2*iWarTroubleThreshold, 50))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose BUILDINGFOCUS_FOOD 1", getName().GetCString());
			return;
		}
	}
	
	if (!bDanger && ((kPlayer.getCurrentEra() > (GC.getGame().getStartEra() + iProductionRank / 2))) || (kPlayer.getCurrentEra() > (GC.getNumEraInfos() / 2)))
	{
		if (AI_chooseBuilding(BUILDINGFOCUS_PRODUCTION, 20 - iWarTroubleThreshold, 15, ((bLandWar || bAssault) ? 25 : -1)))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose BUILDINGFOCUS_PRODUCTION 1", getName().GetCString());
			return;	
		}

		if( !(bDefenseWar && iWarSuccessRatio < -30) )
		{
			if ((iExistingWorkers < ((iNeededWorkers + 1) / 2)))
			{
				if( getPopulation() > 3 || (iProductionRank < (kPlayer.getNumCities() + 1) / 2) )
				{
					if (!bChooseWorker && AI_chooseUnit(UNITAI_WORKER))
					{
						if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose worker 3", getName().GetCString());
						return;
					}
					bChooseWorker = true;
				}
			}
		}
	} */ // K-Mod disabled this stuff
	
	bool bCrushStrategy = kPlayer.AI_isDoStrategy(AI_STRATEGY_CRUSH);
	int iNeededFloatingDefenders = (isBarbarian() || bCrushStrategy) ?  0 : kPlayer.AI_getTotalFloatingDefendersNeeded(pArea);
	int iMaxUnitSpending = kPlayer.AI_maxUnitCostPerMil(area(), iBuildUnitProb); // K-Mod. (note: this has a different scale to the original code).

	if (kPlayer.AI_isDoStrategy(AI_STRATEGY_ECONOMY_FOCUS)) // K-Mod
	{
		iNeededFloatingDefenders = (2 * iNeededFloatingDefenders + 2)/3;
	}
 	int iTotalFloatingDefenders = (isBarbarian() ? 0 : kPlayer.AI_getTotalFloatingDefenders(pArea));

	UnitTypeWeightArray floatingDefenderTypes;
	floatingDefenderTypes.push_back(std::make_pair(UNITAI_CITY_DEFENSE, 125));
	floatingDefenderTypes.push_back(std::make_pair(UNITAI_CITY_COUNTER, 100));
	//floatingDefenderTypes.push_back(std::make_pair(UNITAI_CITY_SPECIAL, 0));
	floatingDefenderTypes.push_back(std::make_pair(UNITAI_RESERVE, 100));
	floatingDefenderTypes.push_back(std::make_pair(UNITAI_COLLATERAL, 80)); // K-Mod, down from 100

	if (iTotalFloatingDefenders < ((iNeededFloatingDefenders + 1) / (bGetBetterUnits ? 3 : 2)))
	{
		if (!bUnitExempt && iUnitSpending < iMaxUnitSpending + 5)
		{
			if (pArea->getAreaAIType(getTeam()) != AREAAI_NEUTRAL || kPlayer.AI_isDoStrategy(AI_STRATEGY_ALERT1) ||
				GC.getGameINLINE().getSorenRandNum(iNeededFloatingDefenders, "AI floating def 1") > iTotalFloatingDefenders * (2*iUnitSpending + iMaxUnitSpending)/std::max(1, 3*iMaxUnitSpending))
			if (AI_chooseLeastRepresentedUnit(floatingDefenderTypes))
			{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose floating defender 1", getName().GetCString());
				return;
			}
		}
	}

	// If losing badly in war, need to build up defenses and counter attack force
	//if( bLandWar && (iWarSuccessRatio < -30 || iEnemyPowerPerc > 150) )
	// K-Mod. I've changed the condition on this; but to be honest, I'm thinking it should just be completely removed.
	// The noraml unit selection function should know what kind of units we should be building...
	if (bLandWar && iWarSuccessRating < -10 && iEnemyPowerPerc - iWarSuccessRating > 150)
	{
		UnitTypeWeightArray defensiveTypes;
		defensiveTypes.push_back(std::make_pair(UNITAI_COUNTER, 100));
		defensiveTypes.push_back(std::make_pair(UNITAI_ATTACK, 100));
		defensiveTypes.push_back(std::make_pair(UNITAI_RESERVE, 60));
		//defensiveTypes.push_back(std::make_pair(UNITAI_COLLATERAL, 60));
		defensiveTypes.push_back(std::make_pair(UNITAI_COLLATERAL, 80));
		if ( bDanger || (iTotalFloatingDefenders < (5*iNeededFloatingDefenders)/(bGetBetterUnits ? 6 : 4)))
		{
			defensiveTypes.push_back(std::make_pair(UNITAI_CITY_DEFENSE, 200));
			defensiveTypes.push_back(std::make_pair(UNITAI_CITY_COUNTER, 50));
		}

		int iOdds = iBuildUnitProb;
		if( iWarSuccessRating < -50 )
		{
			iOdds += abs(iWarSuccessRating/3);
		}
		// K-Mod
		iOdds *= (-iWarSuccessRating+20 + iBestBuildingValue);
		iOdds /= (-iWarSuccessRating + 2 * iBestBuildingValue);
		// K-Mod end
		if( bDanger )
		{
			iOdds += 10;
		}

		if (AI_chooseLeastRepresentedUnit(defensiveTypes, iOdds))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose losing extra defense with odds %d", getName().GetCString(), iOdds);
			return;
		}
	}

	if( !(bDefenseWar && iWarSuccessRating < -50) )
	{
/***
**** K-Mod, 10/sep/10, Karadoc
**** Disabled iExistingWorkers == 0. Changed Pop > 3 to Pop >=3.
**** Changed Rank < (cities + 1)/2 to Rank <= ...  So that it can catch the case where we only have 1 city.
***/
		//if (!(iExistingWorkers == 0))
		{
			if (!bDanger && iExistingWorkers < (iNeededWorkers + 1) / 2)
			{
				if (getPopulation() >= 3 || (iProductionRank <= (kPlayer.getNumCities() + 1) / 2))
				{
					if (!bChooseWorker && AI_chooseUnit(UNITAI_WORKER))
					{
						if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose worker 4", getName().GetCString());
						return;
					}
					bChooseWorker = true;
				}
			}
		}
	}
	
	//do a check for one tile island type thing?
    //this can be overridden by "wait and grow more"
/***
**** K-Mod, 10/sep/10, Karadoc
**** It was "if (bDanger", I have changed it to "if (!bDanger"
***/
    if (!bDanger && iExistingWorkers == 0 && (isCapital() || (iNeededWorkers > 0) || (iNeededSeaWorkers > iExistingSeaWorkers)))
    {
		if( !(bDefenseWar && iWarSuccessRating < -30) && !(kPlayer.AI_isDoStrategy(AI_STRATEGY_TURTLE)) )
		{
			if ((AI_countNumBonuses(NO_BONUS, /*bIncludeOurs*/ true, /*bIncludeNeutral*/ true, -1, /*bLand*/ true, /*bWater*/ false) > 0) || 
				(isCapital() && (getPopulation() > 3) && iNumCitiesInArea > 1))
    		{
				if (!bChooseWorker && AI_chooseUnit(UNITAI_WORKER))
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose worker 5", getName().GetCString());
					return;
				}
				bChooseWorker = true;
    		}

			if (iNeededSeaWorkers > iExistingSeaWorkers)
			{
				if (AI_chooseUnit(UNITAI_WORKER_SEA))
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose worker sea 2", getName().GetCString());
					return;
				}
			}
		}
    }

	if( !(bDefenseWar && iWarSuccessRating < -30) )
	{
		if (!bWaterDanger && iNeededSeaWorkers > iExistingSeaWorkers)
		{
			if (AI_chooseUnit(UNITAI_WORKER_SEA))
			{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose worker sea 3", getName().GetCString());
				return;
			}
		}
	}

	if	(!bLandWar && !bAssault && (iTargetCulturePerTurn > getCommerceRate(COMMERCE_CULTURE)))
	{
		if (AI_chooseBuilding(BUILDINGFOCUS_CULTURE, bAggressiveAI ? 10 : 20, 0, bAggressiveAI ? 33 : 50))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses minimal culture rate", getName().GetCString());
			return;
		}
	}
	
	int iMinFoundValue = kPlayer.AI_getMinFoundValue();
	if (bDanger)
	{
		iMinFoundValue *= 3;
		iMinFoundValue /= 2;
	}

	// BBAI TODO: Check that this works to produce early rushes on tight maps
	if (!bUnitExempt && !bGetBetterUnits && (bIsCapitalArea) && (iAreaBestFoundValue < (iMinFoundValue * 2)))
	{
		//Building city hunting stack.

		if ((getDomainFreeExperience(DOMAIN_LAND) == 0) && (getYieldRate(YIELD_PRODUCTION) > 4))
		{
    		if (AI_chooseBuilding(BUILDINGFOCUS_EXPERIENCE, (kPlayer.getCurrentEra() > 1) ? 0 : 7, 33))
			{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S uses special BUILDINGFOCUS_EXPERIENCE 1a", getName().GetCString());
				return;
			}
		}

		int iStartAttackStackRand = 0;
		if (pArea->getCitiesPerPlayer(BARBARIAN_PLAYER) > 0)
		{
			iStartAttackStackRand += 15;
		}
		if ((pArea->getNumCities() - iNumCitiesInArea) > 0)
		{
			iStartAttackStackRand += iBuildUnitProb / 2;
		}

		if( iStartAttackStackRand > 0 )
		{
			int iAttackCityCount = kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_ATTACK_CITY);
			int iAttackCount = iAttackCityCount + kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_ATTACK);

			if( (iAttackCount) == 0 )
			{
				if( !bFinancialTrouble )
				{
					if (AI_chooseUnit(UNITAI_ATTACK, iStartAttackStackRand))
					{
						return;
					}
				}
			}
			else
			{
				if( (iAttackCount > 1) && (iAttackCityCount == 0) )
				{
					if (AI_chooseUnit(UNITAI_ATTACK_CITY))
					{
						if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose start city attack stack", getName().GetCString());
						return;
					}
				}
				else if (iAttackCount < (3 + iBuildUnitProb / 10))
				{
					if (AI_chooseUnit(UNITAI_ATTACK))
					{
						if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose add to city attack stack", getName().GetCString());
						return;
					}
				}
			}
		}
	}

	//opportunistic wonder build (1)
	if (!bDanger && (!hasActiveWorldWonder()) && (kPlayer.getNumCities() <= 3))
	{
		// For small civ at war, don't build wonders unless winning
		if( !bLandWar || (iWarSuccessRating > 30) )
		{
			int iWonderTime = GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getWonderConstructRand(), "Wonder Construction Rand");
			iWonderTime /= 5;
			iWonderTime += 7;
			if (AI_chooseBuilding(BUILDINGFOCUS_WORLDWONDER, iWonderTime))
			{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S uses oppurtunistic wonder build 1", getName().GetCString());
				return;
			}
		}
	}
	
	if (!bDanger && !bIsCapitalArea && area()->getCitiesPerPlayer(getOwnerINLINE()) > iNumCapitalAreaCities)
	{
		// BBAI TODO:  This check should be done by player, not by city and optimize placement
		// If losing badly in war, don't build big things
		if( !bLandWar || (iWarSuccessRating > -30) )
		{
			if( kPlayer.getCapitalCity() == NULL || area()->getPopulationPerPlayer(getOwnerINLINE()) > kPlayer.getCapitalCity()->area()->getPopulationPerPlayer(getOwnerINLINE()) )
			{
				if (AI_chooseBuilding(BUILDINGFOCUS_CAPITAL, 15))
				{
					return;
				}
			}
		}
	}
	
	int iSpreadUnitThreshold = 1000;

	if( bLandWar )
	{
		iSpreadUnitThreshold += 800 - 10*iWarSuccessRating;
	}
	iSpreadUnitThreshold += 300*plot()->plotCount(PUF_isUnitAIType, UNITAI_MISSIONARY, -1, getOwnerINLINE());

	UnitTypes eBestSpreadUnit = NO_UNIT;
	int iBestSpreadUnitValue = -1;

	if( !bDanger && !(kPlayer.AI_isDoStrategy(AI_STRATEGY_TURTLE)) )
	{
		int iSpreadUnitRoll = (100 - iBuildUnitProb) / 3;
		iSpreadUnitRoll += bLandWar ? 0 : 10;
		// K-Mod
		iSpreadUnitRoll *= (200 + iBestBuildingValue);
		iSpreadUnitRoll /= (100 + 3 * iBestBuildingValue);
		// K-Mod end
		
		if (AI_bestSpreadUnit(true, true, iSpreadUnitRoll, &eBestSpreadUnit, &iBestSpreadUnitValue))
		{
			if (iBestSpreadUnitValue > iSpreadUnitThreshold)
			{
				if (AI_chooseUnit(eBestSpreadUnit, UNITAI_MISSIONARY))
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose missionary 1", getName().GetCString());
					return;
				}
				FAssertMsg(false, "AI_bestSpreadUnit should provide a valid unit when it returns true");
			}
		}
	}
	
	if( !(bLandWar && iWarSuccessRating < 30) )
	{
		if (!bDanger && (iProductionRank <= ((kPlayer.getNumCities() / 5) + 1)))
		{
			// BBAI TODO: Temporary for testing
			//if( getOwnerINLINE()%2 == 1 )
			//{
				if (AI_chooseProject())
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose project 1", getName().GetCString());
					return;
				}
			//}
		}
	}

	//minimal defense.
	//if (!bUnitExempt && iPlotCityDefenderCount < (AI_minDefenders() + iPlotSettlerCount))
	// K-Mod.. take into account any defenders that are on their way. (recall that in AI_guardCityMinDefender, defenders can be shuffled around)
	// (I'm doing the min defender check twice for efficiency - so that we don't count targetmissionAIs when we don't need to)
	if (!bUnitExempt && !(bFinancialTrouble && iUnitSpending > iMaxUnitSpending)
		&& iPlotCityDefenderCount < (AI_minDefenders() + iPlotSettlerCount)
		&& iPlotCityDefenderCount < (AI_minDefenders() + iPlotSettlerCount - kPlayer.AI_plotTargetMissionAIs(plot(), MISSIONAI_GUARD_CITY)))
	// K-Mod end
	{
		if (AI_chooseUnit(UNITAI_CITY_DEFENSE))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose min defender", getName().GetCString());
			return;
		}

		if (AI_chooseUnit(UNITAI_ATTACK))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose min defender (attack ai)", getName().GetCString());
			return;
		}
	}
	
	if( !(bDefenseWar && iWarSuccessRating < -50) )
	{
		if ((iAreaBestFoundValue > iMinFoundValue) || (iWaterAreaBestFoundValue > iMinFoundValue))
		{
			// BBAI TODO: Needs logic to check for early settler builds, settler builds in small cities, whether settler sea exists for water area sites?
			if (pWaterArea != NULL)
			{
				int iTotalCities = kPlayer.getNumCities();
				int iSettlerSeaNeeded = std::min(iNumWaterAreaCitySites, ((iTotalCities + 4) / 8) + 1);
				if (kPlayer.getCapitalCity() != NULL)
				{
					int iOverSeasColonies = iTotalCities - kPlayer.getCapitalCity()->area()->getCitiesPerPlayer(getOwnerINLINE());
					int iLoop = 2;
					int iExtras = 0;
					while (iOverSeasColonies >= iLoop)
					{
						iExtras++;
						iLoop += iLoop + 2;
					}
					iSettlerSeaNeeded += std::min(kPlayer.AI_totalUnitAIs(UNITAI_WORKER) / 4, iExtras);
				}
				if (bAssault)
				{
					iSettlerSeaNeeded = std::min(1, iSettlerSeaNeeded);
				}
				
				if (kPlayer.AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_SETTLER_SEA) < iSettlerSeaNeeded)
				{
					if (AI_chooseUnit(UNITAI_SETTLER_SEA))
					{
						if( gCityLogLevel >= 2 ) logBBAI("      City %S uses main settler sea", getName().GetCString());
						return;
					}
				}
			}
			
			if (iPlotSettlerCount == 0)
			{
				if ((iNumSettlers < iMaxSettlers))
				{
					if (AI_chooseUnit(UNITAI_SETTLE, bLandWar ? 50 : -1))
					{
						if( gCityLogLevel >= 2 ) logBBAI("      City %S uses build settler 1", getName().GetCString());

						if (kPlayer.getNumMilitaryUnits() <= kPlayer.getNumCities() + 1)
						{
							if (AI_chooseUnit(UNITAI_CITY_DEFENSE))
							{
								if( gCityLogLevel >= 2 ) logBBAI("      City %S uses build settler 1 extra quick defense", getName().GetCString());
								return;
							}
						}
						
						return;
					}
				}
			}
		}
	}

	// don't build frivolous things if this is an important city unless we at war
    if (!bImportantCity || bLandWar || bAssault)
    {
        if (bPrimaryArea && !bFinancialTrouble)
        {
            if (kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_ATTACK) == 0)
            {
                if (AI_chooseUnit(UNITAI_ATTACK))
                {
                    return;
                }
            }
        }

        if (!bLandWar && !bDanger && !bFinancialTrouble)
        {
			if (kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_EXPLORE) < (kPlayer.AI_neededExplorers(pArea)))
			{
				if (AI_chooseUnit(UNITAI_EXPLORE))
				{
					return;
				}
			}
        }

		// K-Mod (the spies stuff used to be lower down)
		int iNumSpies = kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_SPY) + kPlayer.AI_getNumTrainAIUnits(UNITAI_SPY);
		int iNeededSpies = iNumCitiesInArea / 3;
		iNeededSpies += bPrimaryArea ? (kPlayer.getCommerceRate(COMMERCE_ESPIONAGE)+50)/100 : 0;
		iNeededSpies *= kPlayer.AI_getEspionageWeight();
		iNeededSpies /= 100;
		{
			const CvCity* pCapitalCity = kPlayer.getCapitalCity();
			if (pCapitalCity != NULL && pCapitalCity->area() == area())
				iNeededSpies++;
		}
		iNeededSpies -= (bDefenseWar ? 1 : 0);
		if (kPlayer.AI_isDoStrategy(AI_STRATEGY_ESPIONAGE_ECONOMY))
			iNeededSpies++;
		else
			iNeededSpies /= kPlayer.AI_isDoStrategy(AI_STRATEGY_ECONOMY_FOCUS) ? 2 : 1;

		if (iNumSpies < iNeededSpies)
		{
			int iOdds = (kPlayer.AI_isDoStrategy(AI_STRATEGY_ESPIONAGE_ECONOMY) || GET_TEAM(getTeam()).getAnyWarPlanCount(true)) ?45 : 35;
			iOdds *= 50 + iBestBuildingValue;
			iOdds /= 20 + 2 * iBestBuildingValue;
			iOdds *= iNeededSpies;
			iOdds /= 4*iNumSpies + iNeededSpies;
			iOdds -= bUnitExempt ? 10 : 0; // not completely exempt, but at least reduced probability.
			iOdds = std::max(0, iOdds);
			if (AI_chooseUnit(UNITAI_SPY, iOdds))
			{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S chooses spy with %d/%d needed, at %d odds", getName().GetCString(), iNumSpies, iNeededSpies, iOdds);
				return;
			}
		}
		// K-Mod end

		if( bDefenseWar || (bLandWar && (iWarSuccessRating < -30)) )
		{
			UnitTypeWeightArray panicDefenderTypes;
			panicDefenderTypes.push_back(std::make_pair(UNITAI_RESERVE, 100));
			panicDefenderTypes.push_back(std::make_pair(UNITAI_COUNTER, 100));
			panicDefenderTypes.push_back(std::make_pair(UNITAI_COLLATERAL, 100));
			panicDefenderTypes.push_back(std::make_pair(UNITAI_ATTACK, 100));

        	if (AI_chooseLeastRepresentedUnit(panicDefenderTypes, (bGetBetterUnits ? 40 : 60) - iWarSuccessRating/3))
        	{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose panic defender", getName().GetCString());
        		return;
        	}
        }
    }

	/* original bts code. We dont' need this. It just ends up putting aqueducts everywhere.
	if (AI_chooseBuilding(BUILDINGFOCUS_FOOD, 60, 10, (bLandWar ? 30 : -1)))
	{
		if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose BUILDINGFOCUS_FOOD 3", getName().GetCString());
		return;
	} */
	
	//oppurunistic wonder build
	if (!bDanger && (!hasActiveWorldWonder() || (kPlayer.getNumCities() > 3)))
	{
		// For civ at war, don't build wonders if losing
		if (!bTotalWar && (!bLandWar || iWarSuccessRating > -30))
		{	
			int iWonderTime = GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getWonderConstructRand(), "Wonder Construction Rand");
			iWonderTime /= 5;
			iWonderTime += 8;
			if (AI_chooseBuilding(BUILDINGFOCUS_WORLDWONDER, iWonderTime))
			{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S uses oppurtunistic wonder build 2", getName().GetCString());
				return;
			}
		}
	}

	// K-Mod, short-circuit 2 - a strong chance to build some high value buildings.
	{
		int iOdds = std::max(0, (bLandWar || (bAssault && pWaterArea) ? 80 : 130) * iBestBuildingValue / (iBestBuildingValue + 20 + iBuildUnitProb) - 25);
		if (AI_chooseBuilding(0, INT_MAX, 0, iOdds))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses building value short-circuit 2 (odds: %d)", getName().GetCString(), iOdds);
   			return;
		}
	}

	// note: this is the only worker test that allows us to reach the full number of needed workers.
	if (!(bLandWar && iWarSuccessRating < -30) && !bDanger)
	{
		if (iExistingWorkers < iNeededWorkers)
		{
			if (3*AI_getWorkersHave()/2 < AI_getWorkersNeeded() // local
				|| GC.getGame().getSorenRandNum(80, "choose worker 6") > iBestBuildingValue + (iBuildUnitProb + 50)/2)
			{
				if (!bChooseWorker && AI_chooseUnit(UNITAI_WORKER))
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose worker 6", getName().GetCString());
					return;
				}
				bChooseWorker = true;
			}
		}
	}
	// K-Mod end

	if( !bDanger )
	{
		if (iBestSpreadUnitValue > ((iSpreadUnitThreshold * (bLandWar ? 80 : 60)) / 100))
		{
			if (AI_chooseUnit(eBestSpreadUnit, UNITAI_MISSIONARY))
			{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose missionary 2", getName().GetCString());
				return;
			}
			FAssertMsg(false, "AI_bestSpreadUnit should provide a valid unit when it returns true");
		}
	}

	if ((getDomainFreeExperience(DOMAIN_LAND) == 0) && (getYieldRate(YIELD_PRODUCTION) > 4))
	{
    	if (AI_chooseBuilding(BUILDINGFOCUS_EXPERIENCE, (kPlayer.getCurrentEra() > 1) ? 0 : 7, 33))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses special BUILDINGFOCUS_EXPERIENCE 1", getName().GetCString());
			return;
		}
	}

	int iCarriers = kPlayer.AI_totalUnitAIs(UNITAI_CARRIER_SEA);
	
	// Revamped logic for production for invasions
	if (iUnitSpending < iMaxUnitSpending + 25) // was + 10 (new unit spending metric)
	{
		bool bBuildAssault = bAssault;
		CvArea* pAssaultWaterArea = NULL;
		if (NULL != pWaterArea)
		{
			// Coastal city extra logic

			pAssaultWaterArea = pWaterArea;

			// If on offensive and can't reach enemy cities from here, act like using AREAAI_ASSAULT
			if( (pAssaultWaterArea != NULL) && !bBuildAssault )
			{
				if( (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0) )
				{
					if( (pArea->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE) )
					{
						// BBAI TODO: faster to switch to checking path for some selection group?
						if( !(plot()->isHasPathToEnemyCity(getTeam())) )
						{
							bBuildAssault = true;
						}
					}
				}
			}
		}

		if( bBuildAssault )
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses build assault", getName().GetCString());

			UnitTypes eBestAssaultUnit = NO_UNIT; 
			if (NULL != pAssaultWaterArea)
			{
				kPlayer.AI_bestCityUnitAIValue(UNITAI_ASSAULT_SEA, this, &eBestAssaultUnit);
			}
			else
			{
				kPlayer.AI_bestCityUnitAIValue(UNITAI_ASSAULT_SEA, NULL, &eBestAssaultUnit);
			}
			
			int iBestSeaAssaultCapacity = 0;
			if (eBestAssaultUnit != NO_UNIT)
			{
				iBestSeaAssaultCapacity = GC.getUnitInfo(eBestAssaultUnit).getCargoSpace();
			}

			int iAreaAttackCityUnits = kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_ATTACK_CITY);
			
			int iUnitsToTransport = iAreaAttackCityUnits;
			iUnitsToTransport += kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_ATTACK);
			iUnitsToTransport += kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_COUNTER)/2;

			int iLocalTransports = kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_ASSAULT_SEA);
			int iTransportsAtSea = 0;
			if (NULL != pAssaultWaterArea)
			{
				iTransportsAtSea = kPlayer.AI_totalAreaUnitAIs(pAssaultWaterArea, UNITAI_ASSAULT_SEA);
			}
			else
			{
				iTransportsAtSea = kPlayer.AI_totalUnitAIs(UNITAI_ASSAULT_SEA)/2;
			}

			//The way of calculating numbers is a bit fuzzy since the ships
			//can make return trips. When massing for a war it'll train enough
			//ships to move it's entire army. Once the war is underway it'll stop
			//training so many ships on the assumption that those out at sea
			//will return...

			int iTransports = iLocalTransports + (bPrimaryArea ? iTransportsAtSea/2 : iTransportsAtSea/4);
			int iTransportCapacity = iBestSeaAssaultCapacity*(iTransports);

			if (NULL != pAssaultWaterArea)
			{
				int iEscorts = kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_ESCORT_SEA);
				iEscorts += kPlayer.AI_totalAreaUnitAIs(pAssaultWaterArea, UNITAI_ESCORT_SEA);

				int iTransportViability = kPlayer.AI_calculateUnitAIViability(UNITAI_ASSAULT_SEA, DOMAIN_SEA);

				//int iDesiredEscorts = ((1 + 2 * iTransports) / 3);
				int iDesiredEscorts = iTransports; // K-Mod
				if( iTransportViability > 95 )
				{
					// Transports are stronger than escorts (usually Galleons and Caravels)
					iDesiredEscorts /= 2; // was /3
				}
				
				/*if (iEscorts < iDesiredEscorts)
				{
					if (AI_chooseUnit(UNITAI_ESCORT_SEA, (iEscorts < iDesiredEscorts/3) ? -1 : 50)) */
				// K-Mod
				if (iEscorts < iDesiredEscorts && iDesiredEscorts > 0)
				{
					int iOdds = 100;
					iOdds *= iDesiredEscorts;
					iOdds /= iDesiredEscorts + 2*iEscorts;
					if (AI_chooseUnit(UNITAI_ESCORT_SEA, iOdds))
				// K-Mod end
					{
						AI_chooseBuilding(BUILDINGFOCUS_DOMAINSEA, 12);
						return;
					}
				}
			
				UnitTypes eBestAttackSeaUnit = NO_UNIT;  
				kPlayer.AI_bestCityUnitAIValue(UNITAI_ATTACK_SEA, this, &eBestAttackSeaUnit);
				if (eBestAttackSeaUnit != NO_UNIT)
				{
					int iDivisor = 2;
					if (GC.getUnitInfo(eBestAttackSeaUnit).getBombardRate() == 0)
					{
						iDivisor = 5;
					}

					int iAttackSea = kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_ATTACK_SEA);
					iAttackSea += kPlayer.AI_totalAreaUnitAIs(pAssaultWaterArea, UNITAI_ATTACK_SEA);
						
					if ((iAttackSea < ((1 + 2 * iTransports) / iDivisor)))
					{
						if (AI_chooseUnit(UNITAI_ATTACK_SEA, (iUnitSpending < iMaxUnitSpending) ? 50 : 20))
						{
							AI_chooseBuilding(BUILDINGFOCUS_DOMAINSEA, 12);
							return;
						}
					}
				}
				
				if (iUnitsToTransport > iTransportCapacity)
				{
					if ((iUnitSpending < iMaxUnitSpending) || (iUnitsToTransport > 2*iTransportCapacity))
					{
						if (AI_chooseUnit(UNITAI_ASSAULT_SEA))
						{
							AI_chooseBuilding(BUILDINGFOCUS_DOMAINSEA, 8);
							return;
						}
					}
				}
			}

			if (iUnitSpending < iMaxUnitSpending)
			{
				if (NULL != pAssaultWaterArea)
				{
					if (!bFinancialTrouble && iCarriers < (kPlayer.AI_totalUnitAIs(UNITAI_ASSAULT_SEA) / 4))
					{
						// Reduce chances of starting if city has low production
						if (AI_chooseUnit(UNITAI_CARRIER_SEA, (iProductionRank <= ((kPlayer.getNumCities() / 3) + 1)) ? -1 : 30))
						{
							AI_chooseBuilding(BUILDINGFOCUS_DOMAINSEA, 16);
							return;
						}
					}
				}
			}

			// Consider building more land units to invade with
			int iTrainInvaderChance = iBuildUnitProb + 10;

			iTrainInvaderChance += (bAggressiveAI ? 15 : 0);
			iTrainInvaderChance += (bTotalWar ? 10 : 0); // K-Mod
			iTrainInvaderChance /= (bAssaultAssist ? 2 : 1);
			iTrainInvaderChance /= (bImportantCity ? 2 : 1);
			iTrainInvaderChance /= (bGetBetterUnits ? 2 : 1);

			// K-Mod
			iTrainInvaderChance *= (100 + iBestBuildingValue);
			iTrainInvaderChance /= (20 + 3 * iBestBuildingValue);
			// K-Mod end

			iUnitsToTransport *= 9;
			iUnitsToTransport /= 10;

			if( (iUnitsToTransport > iTransportCapacity) && (iUnitsToTransport > (bAssaultAssist ? 2 : 4)*iBestSeaAssaultCapacity) )
			{
				// Already have enough
				iTrainInvaderChance /= 2;
			}
			else if( iUnitsToTransport < (iLocalTransports*iBestSeaAssaultCapacity) )
			{
				iTrainInvaderChance += 15;
			}

			if( getPopulation() < 4 )
			{
				// Let small cities build themselves up first
				iTrainInvaderChance /= (5 - getPopulation());
			}

			UnitTypeWeightArray invaderTypes;
			invaderTypes.push_back(std::make_pair(UNITAI_ATTACK_CITY, 100));
			invaderTypes.push_back(std::make_pair(UNITAI_COUNTER, 50));
			invaderTypes.push_back(std::make_pair(UNITAI_ATTACK, 40));
			if( kPlayer.AI_isDoStrategy(AI_STRATEGY_AIR_BLITZ) )
			{
				invaderTypes.push_back(std::make_pair(UNITAI_PARADROP, 20));
			}

			if (AI_chooseLeastRepresentedUnit(invaderTypes, iTrainInvaderChance))
			{
				if( !bImportantCity && (iUnitsToTransport >= (iLocalTransports*iBestSeaAssaultCapacity)) )
				{
					// Have time to build barracks first
					AI_chooseBuilding(BUILDINGFOCUS_EXPERIENCE, 20);
				}
				return;
			}

			if (iUnitSpending < (iMaxUnitSpending))
			{
				int iMissileCarriers = kPlayer.AI_totalUnitAIs(UNITAI_MISSILE_CARRIER_SEA);
			
				if (!bFinancialTrouble && iMissileCarriers > 0 && !bImportantCity)
				{
					if( (iProductionRank <= ((kPlayer.getNumCities() / 2) + 1)) )
					{
						UnitTypes eBestMissileCarrierUnit = NO_UNIT;  
						kPlayer.AI_bestCityUnitAIValue(UNITAI_MISSILE_CARRIER_SEA, NULL, &eBestMissileCarrierUnit);
						if (eBestMissileCarrierUnit != NO_UNIT)
						{
							FAssert(GC.getUnitInfo(eBestMissileCarrierUnit).getDomainCargo() == DOMAIN_AIR);
							
							int iMissileCarrierAirNeeded = iMissileCarriers * GC.getUnitInfo(eBestMissileCarrierUnit).getCargoSpace();
							
							if ((kPlayer.AI_totalUnitAIs(UNITAI_MISSILE_AIR) < iMissileCarrierAirNeeded) || 
								(bPrimaryArea && (kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_MISSILE_CARRIER_SEA) * GC.getUnitInfo(eBestMissileCarrierUnit).getCargoSpace() < kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_MISSILE_AIR))))
							{
								// Don't always build missiles, more likely if really low
								if (AI_chooseUnit(UNITAI_MISSILE_AIR, (kPlayer.AI_totalUnitAIs(UNITAI_MISSILE_AIR) < iMissileCarrierAirNeeded/2) ? 50 : 20))
								{
									return;
								}
							}
						}
					}
				}
			}
		}
	}
	
	UnitTypeWeightArray airUnitTypes;

    int iAircraftNeed = 0;
    int iAircraftHave = 0;
    UnitTypes eBestAttackAircraft = NO_UNIT;
    UnitTypes eBestMissile = NO_UNIT;
    
	if (iUnitSpending < (iMaxUnitSpending + 12) && (!bImportantCity || bDefenseWar) ) // K-Mod. was +4, now +12 for the new unit spending metric
	{
		if( bLandWar || bAssault || (iFreeAirExperience > 0) || (GC.getGame().getSorenRandNum(3, "AI train air") == 0) )
		{
			int iBestAirValue = kPlayer.AI_bestCityUnitAIValue(UNITAI_ATTACK_AIR, this, &eBestAttackAircraft);
			int iBestMissileValue = kPlayer.AI_bestCityUnitAIValue(UNITAI_MISSILE_AIR, this, &eBestMissile);
			if ((iBestAirValue + iBestMissileValue) > 0)
			{
				iAircraftHave = kPlayer.AI_totalUnitAIs(UNITAI_ATTACK_AIR) + kPlayer.AI_totalUnitAIs(UNITAI_DEFENSE_AIR) + kPlayer.AI_totalUnitAIs(UNITAI_MISSILE_AIR);
				if (NO_UNIT != eBestAttackAircraft)
				{
					iAircraftNeed = (2 + kPlayer.getNumCities() * (3 * GC.getUnitInfo(eBestAttackAircraft).getAirCombat())) / (2 * std::max(1, GC.getGame().getBestLandUnitCombat()));
					int iBestDefenseValue = kPlayer.AI_bestCityUnitAIValue(UNITAI_DEFENSE_AIR, this);
					if ((iBestDefenseValue > 0) && (iBestAirValue > iBestDefenseValue))
					{
						iAircraftNeed *= 3;
						iAircraftNeed /= 2;
					}
				}
				if (iBestMissileValue > 0)
				{
					iAircraftNeed = std::max(iAircraftNeed, 1 + kPlayer.getNumCities() / 2);
				}
				
				bool bAirBlitz = kPlayer.AI_isDoStrategy(AI_STRATEGY_AIR_BLITZ);
				bool bLandBlitz = kPlayer.AI_isDoStrategy(AI_STRATEGY_LAND_BLITZ);
				if (bAirBlitz)
				{
					iAircraftNeed *= 3;
					iAircraftNeed /= 2;
				}
				else if (bLandBlitz)
				{
					iAircraftNeed /= 2;
					iAircraftNeed += 1;
				}
				
				airUnitTypes.push_back(std::make_pair(UNITAI_ATTACK_AIR, bAirBlitz ? 125 : 80));
				airUnitTypes.push_back(std::make_pair(UNITAI_DEFENSE_AIR, bLandBlitz ? 100 : 100));
				if (iBestMissileValue > 0)
				{
					airUnitTypes.push_back(std::make_pair(UNITAI_MISSILE_AIR, bAssault ? 60 : 40));
				}
				
				airUnitTypes.push_back(std::make_pair(UNITAI_ICBM, 20));
				
				if (iAircraftHave * 2 < iAircraftNeed)
				{
					if (AI_chooseLeastRepresentedUnit(airUnitTypes))
					{
						return;
					}
				}
				// Additional check for air defenses
				int iFightersHave = kPlayer.AI_totalUnitAIs(UNITAI_DEFENSE_AIR);

				if( 3*iFightersHave < iAircraftNeed )
				{
					if (AI_chooseUnit(UNITAI_DEFENSE_AIR))
					{
						return;
					}
				}
			}
		}
	}

	// Check for whether to produce planes to fill carriers
	if ( (bLandWar || bAssault) && iUnitSpending < (iMaxUnitSpending))
	{			
		if (iCarriers > 0 && !bImportantCity)
		{
			UnitTypes eBestCarrierUnit = NO_UNIT;  
			kPlayer.AI_bestCityUnitAIValue(UNITAI_CARRIER_SEA, NULL, &eBestCarrierUnit);
			if (eBestCarrierUnit != NO_UNIT)
			{
				FAssert(GC.getUnitInfo(eBestCarrierUnit).getDomainCargo() == DOMAIN_AIR);
				
				int iCarrierAirNeeded = iCarriers * GC.getUnitInfo(eBestCarrierUnit).getCargoSpace();

				// Reduce chances if city gives no air experience
				if (kPlayer.AI_totalUnitAIs(UNITAI_CARRIER_AIR) < iCarrierAirNeeded)
				{
					if (AI_chooseUnit(UNITAI_CARRIER_AIR, (iFreeAirExperience > 0) ? -1 : 35))
					{
						return;
					}
				}
			}
		}
	}
	
	if (!bAlwaysPeace && !(bLandWar || bAssault) && (kPlayer.AI_isDoStrategy(AI_STRATEGY_OWABWNW) || (GC.getGame().getSorenRandNum(12, "AI consider Nuke") == 0)))
	{
		if( !bFinancialTrouble )
		{
			int iTotalNukes = kPlayer.AI_totalUnitAIs(UNITAI_ICBM);
			int iNukesWanted = 1 + 2 * std::min(kPlayer.getNumCities(), GC.getGame().getNumCities() - kPlayer.getNumCities());
			if ((iTotalNukes < iNukesWanted) && (GC.getGame().getSorenRandNum(100, "AI train nuke MWAHAHAH") < (90 - (80 * iTotalNukes) / iNukesWanted)))
			{
				//if ((pWaterArea != NULL))
				if ((pWaterArea != NULL) && GC.getGame().getSorenRandNum(3, "AI train boat instead") == 0) // K-Mod
				{
					if (AI_chooseUnit(UNITAI_MISSILE_CARRIER_SEA, 50))
					{
						return;	
					}
				}

				if (AI_chooseUnit(UNITAI_ICBM))
				{
					return;
				}
			}
		}
	}   

	// Assault case now completely handled above
	if (!bAssault && (!bImportantCity || bDefenseWar) && (iUnitSpending < iMaxUnitSpending))
	{
		if (!bFinancialTrouble && (bLandWar || (kPlayer.AI_isDoStrategy(AI_STRATEGY_DAGGER) && !bGetBetterUnits)))
		{
			//int iTrainInvaderChance = iBuildUnitProb + 10;
			int iTrainInvaderChance = iBuildUnitProb + (bTotalWar ? 16 : 8); // K-Mod

			if (bAggressiveAI)
			{
				iTrainInvaderChance += 15;
			}

			if( bGetBetterUnits )
			{
				iTrainInvaderChance /= 2;
			}
			else if ((pArea->getAreaAIType(getTeam()) == AREAAI_MASSING) || (pArea->getAreaAIType(getTeam()) == AREAAI_ASSAULT_MASSING))
			{
				iTrainInvaderChance = (100 - ((100 - iTrainInvaderChance) / (bCrushStrategy ? 6 : 3)));
			}        	

			if (AI_chooseBuilding(BUILDINGFOCUS_EXPERIENCE, 20, 0, bDefenseWar ? 10 : 30))
			{
				return;
			}

			UnitTypeWeightArray invaderTypes;
			invaderTypes.push_back(std::make_pair(UNITAI_ATTACK_CITY, 100));
			invaderTypes.push_back(std::make_pair(UNITAI_COUNTER, 50));
			invaderTypes.push_back(std::make_pair(UNITAI_ATTACK, 40));
			invaderTypes.push_back(std::make_pair(UNITAI_PARADROP, (kPlayer.AI_isDoStrategy(AI_STRATEGY_AIR_BLITZ) ? 30 : 20) / (bAssault ? 2 : 1)));
			//if (!bAssault)
			if (!bAssault && !bCrushStrategy) // K-Mod
			{
				if (kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_PILLAGE) <= ((iNumCitiesInArea + 1) / 2))
				{
					invaderTypes.push_back(std::make_pair(UNITAI_PILLAGE, 30));
				}
			}

			// K-Mod - get more seige units for crush
			if (bCrushStrategy && GC.getGameINLINE().getSorenRandNum(100, "City AI extra crush bombard") < iTrainInvaderChance)
			{
				UnitTypes eCityAttackUnit = NO_UNIT;
				kPlayer.AI_bestCityUnitAIValue(UNITAI_ATTACK_CITY, this, &eCityAttackUnit);
				if (eCityAttackUnit != NO_UNIT && GC.getUnitInfo(eCityAttackUnit).getBombardRate() > 0)
				{
					if (AI_chooseUnit(eCityAttackUnit, UNITAI_ATTACK_CITY))
					{
						if( gCityLogLevel >= 2 ) logBBAI("      City %S uses extra crush bombard", getName().GetCString());
						return;
					}
				}
			}
			// K-Mod end

			if (AI_chooseLeastRepresentedUnit(invaderTypes, iTrainInvaderChance))
			{
				return;
			}
		}
	}

	if ((pWaterArea != NULL) && !bDefenseWar && !bAssault)
	{
		if( !bFinancialTrouble )
		{
			// Force civs with foreign colonies to build a few assault transports to defend the colonies
			if( kPlayer.AI_totalUnitAIs(UNITAI_ASSAULT_SEA) < (kPlayer.getNumCities() - iNumCapitalAreaCities)/3 )
			{
				if (AI_chooseUnit(UNITAI_ASSAULT_SEA))
				{
					return;
				}
			}

			if (kPlayer.AI_calculateUnitAIViability(UNITAI_SETTLER_SEA, DOMAIN_SEA) < 61)
			{
				// Force civs to build escorts for settler_sea units
				if( kPlayer.AI_totalUnitAIs(UNITAI_SETTLER_SEA) > kPlayer.AI_getNumAIUnits(UNITAI_RESERVE_SEA) )
				{
					if (AI_chooseUnit(UNITAI_RESERVE_SEA))
					{
						return;
					}
				}
			}
		}
	}
	
	//Arr.  Don't build pirates in financial trouble, as they'll be disbanded with high probability
	if ((pWaterArea != NULL) && !bLandWar && !bAssault && !bFinancialTrouble && !bUnitExempt)
	{
		int iPirateCount = kPlayer.AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_PIRATE_SEA);
		int iNeededPirates = (1 + (pWaterArea->getNumTiles() / std::max(1, 200 - iBuildUnitProb)));
		iNeededPirates *= (20 + iWaterPercent);
		iNeededPirates /= 100;
		
		if (kPlayer.isNoForeignTrade())
		{
			iNeededPirates *= 3;
			iNeededPirates /= 2;
		}
		if (kPlayer.AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_PIRATE_SEA) < iNeededPirates)
		{
			if (kPlayer.AI_calculateUnitAIViability(UNITAI_PIRATE_SEA, DOMAIN_SEA) > 49)
			{
				if (AI_chooseUnit(UNITAI_PIRATE_SEA, iWaterPercent / (1 + iPirateCount)))
				{
					return;
				}
			}
		}
	}
	
	if (!bLandWar && !bFinancialTrouble)
	{
		if ((pWaterArea != NULL) && (iWaterPercent > 40))
		{
			if (kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_SPY) > 0)
			{
				if (kPlayer.AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_SPY_SEA) == 0)
				{
					if (AI_chooseUnit(UNITAI_SPY_SEA))
					{
						return;
					}
				}
			}
		}
	}
	
	if (iBestSpreadUnitValue > ((iSpreadUnitThreshold * 40) / 100))
	{
		if (AI_chooseUnit(eBestSpreadUnit, UNITAI_MISSIONARY))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose missionary 3", getName().GetCString());
			return;
		}
		FAssertMsg(false, "AI_bestSpreadUnit should provide a valid unit when it returns true");
	}
		
	if (!bUnitExempt && iTotalFloatingDefenders < iNeededFloatingDefenders && (!bFinancialTrouble || bLandWar))
	{
		if (AI_chooseLeastRepresentedUnit(floatingDefenderTypes, 50))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose floating defender 2", getName().GetCString());
			return;
		}
	}

	if (bLandWar && !bDanger)
	{
		if (iNumSettlers < iMaxSettlers)
		{
			if (!bFinancialTrouble)
			{
				if (iAreaBestFoundValue > iMinFoundValue)
				{
					if (AI_chooseUnit(UNITAI_SETTLE))
					{
						if( gCityLogLevel >= 2 ) logBBAI("      City %S uses build settler 2", getName().GetCString());
						return;
					}
				}
			}
		}
	}
	
	//if ((iProductionRank <= ((kPlayer.getNumCities() > 8) ? 3 : 2))
	// Ideally we'd look at relative production, not just rank.
	if (iProductionRank <= kPlayer.getNumCities()/9 + 2	&& getPopulation() > 3)
	{
		int iWonderRand = 8 + GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getWonderConstructRand(), "Wonder Construction Rand");
		
		// increase chance of going for an early wonder
		if (GC.getGameINLINE().getElapsedGameTurns() < (100 * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getConstructPercent() / 100) && iNumCitiesInArea > 1)
		{
			iWonderRand *= 35;
			iWonderRand /= 100;
		}
		else if (iNumCitiesInArea >= 3)
		{
			iWonderRand *= 30;
			iWonderRand /= 100;
		}
		else
		{
			iWonderRand *= 25;
			iWonderRand /= 100;
		}
		
		if (bAggressiveAI)
		{
			iWonderRand *= 2;
			iWonderRand /= 3;
		}

		if (bLandWar && bTotalWar)
		{
			iWonderRand *= 2;
			iWonderRand /= 3;
		}
		
		int iWonderRoll = GC.getGameINLINE().getSorenRandNum(100, "Wonder Build Rand");
		
		if (iProductionRank == 1)
		{
			iWonderRoll /= 2;
		}
		
		if (iWonderRoll < iWonderRand)
		{
			int iWonderMaxTurns = 20 + ((iWonderRand - iWonderRoll) * 2);
			if (bLandWar)
			{
				iWonderMaxTurns /= 2;
			}
			
			if (AI_chooseBuilding(BUILDINGFOCUS_WORLDWONDER, iWonderMaxTurns))
			{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S uses oppurtunistic wonder build 3", getName().GetCString());
				return;
			}
		}
	}
	
	if (iUnitSpending < iMaxUnitSpending + 12 && !bFinancialTrouble) // was +4 (new metric)
	{
		if ((iAircraftHave * 2 >= iAircraftNeed) && (iAircraftHave < iAircraftNeed))
		{
			int iOdds = 33;

			if( iFreeAirExperience > 0 || (iProductionRank <= (1 + kPlayer.getNumCities()/2)) )
			{
				iOdds = -1;
			}

			if (AI_chooseLeastRepresentedUnit(airUnitTypes, iOdds))
			{
				return;
			}
		}
	}

	if (!bLandWar)
	{
		if (pWaterArea != NULL && bFinancialTrouble)
		{
			if (kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_MISSIONARY) > 0)
			{
				if (kPlayer.AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_MISSIONARY_SEA) == 0)
				{
					if (AI_chooseUnit(UNITAI_MISSIONARY_SEA))
					{
						return;
					}
				}
			}
		}
	}


	if (getCommerceRateTimes100(COMMERCE_CULTURE) == 0)
	{
		if (AI_chooseBuilding(BUILDINGFOCUS_CULTURE, 30))
		{
			return;
		}
	}

	if (!bAlwaysPeace )
	{
	    if (!bDanger)
	    {
			if (AI_chooseBuilding(BUILDINGFOCUS_EXPERIENCE, 20, 0, 3*getPopulation()))
            {
                return;
            }
	    }

		if (AI_chooseBuilding(BUILDINGFOCUS_DEFENSE, 20, 0, bDanger ? -1 : 3*getPopulation()))
		{
			return;
		}
		
		if (bDanger)
	    {
            if (AI_chooseBuilding(BUILDINGFOCUS_EXPERIENCE, 20, 0, 2*getPopulation()))
            {
                return;
            }
	    }
	}

	// Short-circuit 3 - a last chance to catch important buildings
	{
		int iOdds = std::max(0, (bLandWar ? 160 : 220) * iBestBuildingValue / (iBestBuildingValue + 20) - 100);
		if (AI_chooseBuilding(0, INT_MAX, 0, iOdds))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses building value short-circuit 3 (odds: %d)", getName().GetCString(), iOdds);
   			return;
		}
	}

	if (!bLandWar)
	{
		if (pWaterArea != NULL)
		{
			if (bPrimaryArea)
			{
				if (kPlayer.AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_EXPLORE_SEA) < std::min(1, kPlayer.AI_neededExplorers(pWaterArea)))
				{
					if (AI_chooseUnit(UNITAI_EXPLORE_SEA))
					{
						return;
					}
				}
			}
		}
	}

	if (!bUnitExempt && plot()->plotCheck(PUF_isUnitAIType, UNITAI_CITY_COUNTER, -1, getOwnerINLINE()) == NULL)
	{
		if (AI_chooseUnit(UNITAI_CITY_COUNTER))
		{
			return;
		}
	}

	// we do a similar check lower, in the landwar case. (umm. no you don't. I'm changing this.)
	//if (!bLandWar && bFinancialTrouble)
	if (bFinancialTrouble)
	{
		if (AI_chooseBuilding(BUILDINGFOCUS_GOLD))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose financial trouble gold", getName().GetCString());
			return;
		}
	}

	bool bChooseUnit = false;
	if (iUnitSpending < iMaxUnitSpending + 15) // was +5 (new metric)
	{
		// K-Mod
		iBuildUnitProb *= (250 + iBestBuildingValue);
		iBuildUnitProb /= (100 + 3 * iBestBuildingValue);
		// K-Mod end

		if ((bLandWar) ||
			  ((kPlayer.getNumCities() <= 3) && (GC.getGameINLINE().getElapsedGameTurns() < 60)) ||
			  (!bUnitExempt && GC.getGameINLINE().getSorenRandNum(100, "AI Build Unit Production") < iBuildUnitProb) ||
				(isHuman() && (getGameTurnFounded() == GC.getGameINLINE().getGameTurn())))
		{
			if (AI_chooseUnit())
			{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose unit by probability", getName().GetCString());
				return;
			}

			bChooseUnit = true;
		}
	}

	// Only cities with reasonable production
	if ((iProductionRank <= ((kPlayer.getNumCities() > 8) ? 3 : 2))
		&& (getPopulation() > 3))
	{
		if (AI_chooseProject())
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose project 2", getName().GetCString());
			return;
		}
	}

	if (AI_chooseBuilding())
	{
		if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose building by probability", getName().GetCString());
		return;
	}
	
	if (!bChooseUnit && !bFinancialTrouble && kPlayer.AI_isDoStrategy(AI_STRATEGY_FINAL_WAR))
	{
		if (AI_chooseUnit())
		{
			if (gCityLogLevel >= 2) logBBAI("      City %S uses choose unit by default", getName().GetCString());
			return;
		}
	}

	if (AI_chooseProcess())
	{
		if (gCityLogLevel >= 2) logBBAI("      City %S uses choose process by default", getName().GetCString());
		return;
	}
}

UnitTypes CvCityAI::AI_bestUnit(bool bAsync, AdvisorTypes eIgnoreAdvisor, UnitAITypes* peBestUnitAI)
{
	CvArea* pWaterArea;
	int aiUnitAIVal[NUM_UNITAI_TYPES];
	UnitTypes eUnit;
	UnitTypes eBestUnit;
	bool bWarPlan;
	bool bDefense;
	bool bLandWar;
	bool bAssault;
	bool bPrimaryArea;
	bool bAreaAlone;
	bool bFinancialTrouble;
	bool bWarPossible;
	bool bDanger;
	int iHasMetCount;
	int iMilitaryWeight;
	int iCoastalCities = 0;
	int iBestValue;
	int iI;

	if (peBestUnitAI != NULL)
	{
		*peBestUnitAI = NO_UNITAI;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      11/30/08                                jdog5000      */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
/* original bts code
	pWaterArea = waterArea();
*/
	pWaterArea = waterArea(true);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	bWarPlan = (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0);
	bDefense = (area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE);
	bLandWar = (bDefense || (area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) || (area()->getAreaAIType(getTeam()) == AREAAI_MASSING));
	bAssault = (area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT);
	bPrimaryArea = GET_PLAYER(getOwnerINLINE()).AI_isPrimaryArea(area());
	bAreaAlone = GET_PLAYER(getOwnerINLINE()).AI_isAreaAlone(area());
	bFinancialTrouble = GET_PLAYER(getOwnerINLINE()).AI_isFinancialTrouble();
	bWarPossible = GET_TEAM(getTeam()).AI_isWarPossible();
	bDanger = AI_isDanger();

	iHasMetCount = GET_TEAM(getTeam()).getHasMetCivCount(true);
	iMilitaryWeight = GET_PLAYER(getOwnerINLINE()).AI_militaryWeight(area());
	int iNumCitiesInArea = area()->getCitiesPerPlayer(getOwnerINLINE());

	if (pWaterArea != NULL)
	{
		iCoastalCities = GET_PLAYER(getOwnerINLINE()).countNumCoastalCitiesByArea(pWaterArea);
	}

	for (iI = 0; iI < NUM_UNITAI_TYPES; iI++)
	{
		aiUnitAIVal[iI] = 0;
	}

	if (!bFinancialTrouble && ((bPrimaryArea) ? (GET_PLAYER(getOwnerINLINE()).findBestFoundValue() > 0) : (area()->getBestFoundValue(getOwnerINLINE()) > 0)))
	{
		aiUnitAIVal[UNITAI_SETTLE]++;
	}

	aiUnitAIVal[UNITAI_WORKER] += GET_PLAYER(getOwnerINLINE()).AI_neededWorkers(area());

	aiUnitAIVal[UNITAI_ATTACK] += ((iMilitaryWeight / ((bWarPlan || bLandWar || bAssault) ? 7 : 12)) + ((bPrimaryArea && bWarPossible) ? 2 : 0) + 1);

	aiUnitAIVal[UNITAI_CITY_DEFENSE] += (iNumCitiesInArea + 1);
	aiUnitAIVal[UNITAI_CITY_COUNTER] += ((5 * (iNumCitiesInArea + 1)) / 8);
	aiUnitAIVal[UNITAI_CITY_SPECIAL] += ((iNumCitiesInArea + 1) / 2);

	if (bWarPossible)
	{
		aiUnitAIVal[UNITAI_ATTACK_CITY] += ((iMilitaryWeight / ((bWarPlan || bLandWar || bAssault) ? 10 : 17)) + ((bPrimaryArea) ? 1 : 0));
		aiUnitAIVal[UNITAI_COUNTER] += ((iMilitaryWeight / ((bWarPlan || bLandWar || bAssault) ? 13 : 22)) + ((bPrimaryArea) ? 1 : 0));
		aiUnitAIVal[UNITAI_PARADROP] += ((iMilitaryWeight / ((bWarPlan || bLandWar || bAssault) ? 5 : 8)) + ((bPrimaryArea) ? 1 : 0));

		aiUnitAIVal[UNITAI_DEFENSE_AIR] += (GET_PLAYER(getOwnerINLINE()).getNumCities() + 1);
		aiUnitAIVal[UNITAI_CARRIER_AIR] += GET_PLAYER(getOwnerINLINE()).AI_countCargoSpace(UNITAI_CARRIER_SEA);
		aiUnitAIVal[UNITAI_MISSILE_AIR] += GET_PLAYER(getOwnerINLINE()).AI_countCargoSpace(UNITAI_MISSILE_CARRIER_SEA);

		if (bPrimaryArea)
		{
			//aiUnitAIVal[UNITAI_ICBM] += std::max((GET_PLAYER(getOwnerINLINE()).getTotalPopulation() / 25), ((GC.getGameINLINE().countCivPlayersAlive() + GC.getGameINLINE().countTotalNukeUnits()) / (GC.getGameINLINE().countCivPlayersAlive() + 1)));
			// K-Mod
			aiUnitAIVal[UNITAI_ICBM] += std::max((GET_PLAYER(getOwnerINLINE()).getTotalPopulation() / 25), ((GC.getGameINLINE().countFreeTeamsAlive() + GC.getGameINLINE().countTotalNukeUnits() + GC.getGameINLINE().getNukesExploded()) / (GC.getGameINLINE().countFreeTeamsAlive() + 1)));
		}
	}

	if (isBarbarian())
	{
		aiUnitAIVal[UNITAI_ATTACK] *= 2;
	}
	else
	{
		if (!bLandWar)
		{
			aiUnitAIVal[UNITAI_EXPLORE] += GET_PLAYER(getOwnerINLINE()).AI_neededExplorers(area());
		}

		if (pWaterArea != NULL)
		{
			aiUnitAIVal[UNITAI_WORKER_SEA] += AI_neededSeaWorkers();

			if ((GET_PLAYER(getOwnerINLINE()).getNumCities() > 3) || (area()->getNumUnownedTiles() < 10))
			{
				if (bPrimaryArea)
				{
					aiUnitAIVal[UNITAI_EXPLORE_SEA] += GET_PLAYER(getOwnerINLINE()).AI_neededExplorers(pWaterArea);
				}

				if (bPrimaryArea && (GET_PLAYER(getOwnerINLINE()).findBestFoundValue() > 0) && (pWaterArea->getNumTiles() > 300))
				{
					aiUnitAIVal[UNITAI_SETTLER_SEA]++;
				}

				if (bPrimaryArea && (GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(area(), UNITAI_MISSIONARY) > 0) && (pWaterArea->getNumTiles() > 400))
				{
					aiUnitAIVal[UNITAI_MISSIONARY_SEA]++;
				}

				if (bPrimaryArea && (GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(area(), UNITAI_SPY) > 0) && (pWaterArea->getNumTiles() > 500))
				{
					aiUnitAIVal[UNITAI_SPY_SEA]++;
				}

				aiUnitAIVal[UNITAI_PIRATE_SEA] += pWaterArea->getNumTiles() / 600;

				if (bWarPossible)
				{
					aiUnitAIVal[UNITAI_ATTACK_SEA] += std::min((pWaterArea->getNumTiles() / 150), ((((iCoastalCities * 2) + (iMilitaryWeight / 9)) / ((bAssault) ? 4 : 6)) + ((bPrimaryArea) ? 1 : 0)));
					aiUnitAIVal[UNITAI_RESERVE_SEA] += std::min((pWaterArea->getNumTiles() / 200), ((((iCoastalCities * 2) + (iMilitaryWeight / 7)) / 5) + ((bPrimaryArea) ? 1 : 0)));
					aiUnitAIVal[UNITAI_ESCORT_SEA] += (GET_PLAYER(getOwnerINLINE()).AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_ASSAULT_SEA) + (GET_PLAYER(getOwnerINLINE()).AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_CARRIER_SEA) * 2));
					aiUnitAIVal[UNITAI_ASSAULT_SEA] += std::min((pWaterArea->getNumTiles() / 250), ((((iCoastalCities * 2) + (iMilitaryWeight / 6)) / ((bAssault) ? 5 : 8)) + ((bPrimaryArea) ? 1 : 0)));
					aiUnitAIVal[UNITAI_CARRIER_SEA] += std::min((pWaterArea->getNumTiles() / 350), ((((iCoastalCities * 2) + (iMilitaryWeight / 8)) / 7) + ((bPrimaryArea) ? 1 : 0)));
					aiUnitAIVal[UNITAI_MISSILE_CARRIER_SEA] += std::min((pWaterArea->getNumTiles() / 350), ((((iCoastalCities * 2) + (iMilitaryWeight / 8)) / 7) + ((bPrimaryArea) ? 1 : 0)));
				}
			}
		}

		if ((iHasMetCount > 0) && bWarPossible)
		{
			if (bLandWar || bAssault || !bFinancialTrouble || (GET_PLAYER(getOwnerINLINE()).calculateUnitCost() == 0))
			{
				aiUnitAIVal[UNITAI_ATTACK] += ((iMilitaryWeight / ((bLandWar || bAssault) ? 9 : 16)) + ((bPrimaryArea && !bAreaAlone) ? 1 : 0));
				aiUnitAIVal[UNITAI_ATTACK_CITY] += ((iMilitaryWeight / ((bLandWar || bAssault) ? 6 : 15)) + ((bPrimaryArea && !bAreaAlone) ? 1 : 0)); // K-Mod, used to be (bLandWar || bAssault) ? 7
				aiUnitAIVal[UNITAI_COLLATERAL] += ((iMilitaryWeight / ((bDefense) ? 8 : 14)) + ((bPrimaryArea && !bAreaAlone) ? 1 : 0));
				aiUnitAIVal[UNITAI_PILLAGE] += ((iMilitaryWeight / ((bLandWar || bAssault) ? 10 : 19)) + ((bPrimaryArea && !bAreaAlone) ? 1 : 0));
				aiUnitAIVal[UNITAI_RESERVE] += ((iMilitaryWeight / ((bLandWar) ? 12 : 17)) + ((bPrimaryArea && !bAreaAlone) ? 1 : 0));
				aiUnitAIVal[UNITAI_COUNTER] += ((iMilitaryWeight / ((bLandWar || bAssault) ? 9 : 16)) + ((bPrimaryArea && !bAreaAlone) ? 1 : 0));
				aiUnitAIVal[UNITAI_PARADROP] += ((iMilitaryWeight / ((bLandWar || bAssault) ? 4 : 8)) + ((bPrimaryArea && !bAreaAlone) ? 1 : 0));

				//aiUnitAIVal[UNITAI_ATTACK_AIR] += (GET_PLAYER(getOwnerINLINE()).getNumCities() + 1);
				// K-Mod (extra air attack and defence). Note: iMilitaryWeight is (pArea->getPopulationPerPlayer(getID()) + pArea->getCitiesPerPlayer(getID())
				aiUnitAIVal[UNITAI_ATTACK_AIR] += (bLandWar ? 2 : 1) * GET_PLAYER(getOwnerINLINE()).getNumCities() + 1;
				aiUnitAIVal[UNITAI_DEFENSE_AIR] += (bDefense ? 1 : 0) * GET_PLAYER(getOwnerINLINE()).getNumCities() + 1; // it would be nice if this was based on enemy air power...

				if (pWaterArea != NULL)
				{
					if ((GET_PLAYER(getOwnerINLINE()).getNumCities() > 3) || (area()->getNumUnownedTiles() < 10))
					{
						aiUnitAIVal[UNITAI_ATTACK_SEA] += std::min((pWaterArea->getNumTiles() / 100), ((((iCoastalCities * 2) + (iMilitaryWeight / 10)) / ((bAssault) ? 5 : 7)) + ((bPrimaryArea) ? 1 : 0)));
						aiUnitAIVal[UNITAI_RESERVE_SEA] += std::min((pWaterArea->getNumTiles() / 150), ((((iCoastalCities * 2) + (iMilitaryWeight / 11)) / 8) + ((bPrimaryArea) ? 1 : 0)));
					}
				}
			}
			// K-Mod
			if (bLandWar && !bDefense && GET_TEAM(getTeam()).getWarPlanCount(WARPLAN_TOTAL, true))
			{
				// if we're winning, then focus on capturing cities.
				int iSuccessRatio = GET_TEAM(getTeam()).AI_getWarSuccessRating();
				if (iSuccessRatio > 0)
				{
					aiUnitAIVal[UNITAI_ATTACK] += iSuccessRatio * iMilitaryWeight / 1200;
					aiUnitAIVal[UNITAI_ATTACK_CITY] += iSuccessRatio * iMilitaryWeight / 600;
					aiUnitAIVal[UNITAI_COUNTER] += iSuccessRatio * iMilitaryWeight / 1200;
					aiUnitAIVal[UNITAI_PARADROP] += iSuccessRatio * iMilitaryWeight / 600;
				}
			}
			// K-Mod end
		}
	}

	// XXX this should account for air and heli units too...
	// K-Mod. Human players don't choose the AI type of the units they build. Therefore we shouldn't use the unit AI counts to decide what to build next.
	if (isHuman())
	{
		aiUnitAIVal[UNITAI_SETTLE] = 0;
		aiUnitAIVal[UNITAI_WORKER] = 0;
	}
	else
	// K-Mod end
	{
		for (iI = 0; iI < NUM_UNITAI_TYPES; iI++)
		{
			if (GET_PLAYER(getOwnerINLINE()).AI_unitAIDomainType((UnitAITypes)iI) == DOMAIN_SEA)
			{
				if (pWaterArea != NULL)
				{
					aiUnitAIVal[iI] -= GET_PLAYER(getOwnerINLINE()).AI_totalWaterAreaUnitAIs(pWaterArea, ((UnitAITypes)iI));
				}
			}
			else if ((GET_PLAYER(getOwnerINLINE()).AI_unitAIDomainType((UnitAITypes)iI) == DOMAIN_AIR) || (iI == UNITAI_ICBM))
			{
				aiUnitAIVal[iI] -= GET_PLAYER(getOwnerINLINE()).AI_totalUnitAIs((UnitAITypes)iI);
			}
			else
			{
				aiUnitAIVal[iI] -= GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(area(), ((UnitAITypes)iI));
			}
		}
	}

	aiUnitAIVal[UNITAI_SETTLE] *= ((bDanger) ? 8 : 12); // was ? 8 : 20
	aiUnitAIVal[UNITAI_WORKER] *= ((bDanger) ? 2 : 7);
	aiUnitAIVal[UNITAI_ATTACK] *= 3;
	aiUnitAIVal[UNITAI_ATTACK_CITY] *= 5; // K-Mod, up from *4
	aiUnitAIVal[UNITAI_COLLATERAL] *= 5;
	aiUnitAIVal[UNITAI_PILLAGE] *= 3;
	aiUnitAIVal[UNITAI_RESERVE] *= 3;
	aiUnitAIVal[UNITAI_COUNTER] *= 3;
	aiUnitAIVal[UNITAI_COUNTER] *= 2;
	aiUnitAIVal[UNITAI_CITY_DEFENSE] *= 2;
	aiUnitAIVal[UNITAI_CITY_COUNTER] *= 2;
	aiUnitAIVal[UNITAI_CITY_SPECIAL] *= 2;
	aiUnitAIVal[UNITAI_EXPLORE] *= ((bDanger) ? 6 : 15);
	aiUnitAIVal[UNITAI_ICBM] *= 18;
	aiUnitAIVal[UNITAI_WORKER_SEA] *= ((bDanger) ? 3 : 10);
	aiUnitAIVal[UNITAI_ATTACK_SEA] *= 5;
	aiUnitAIVal[UNITAI_RESERVE_SEA] *= 4;
	aiUnitAIVal[UNITAI_ESCORT_SEA] *= 20;
	aiUnitAIVal[UNITAI_EXPLORE_SEA] *= 18;
	aiUnitAIVal[UNITAI_ASSAULT_SEA] *= 14;
	aiUnitAIVal[UNITAI_SETTLER_SEA] *= 16;
	aiUnitAIVal[UNITAI_MISSIONARY_SEA] *= 12;
	aiUnitAIVal[UNITAI_SPY_SEA] *= 10;
	aiUnitAIVal[UNITAI_CARRIER_SEA] *= 8;
	aiUnitAIVal[UNITAI_MISSILE_CARRIER_SEA] *= 8;
	aiUnitAIVal[UNITAI_PIRATE_SEA] *= 5;
	aiUnitAIVal[UNITAI_ATTACK_AIR] *= 6;
	aiUnitAIVal[UNITAI_DEFENSE_AIR] *= 3;
	aiUnitAIVal[UNITAI_CARRIER_AIR] *= 15;
	aiUnitAIVal[UNITAI_MISSILE_AIR] *= 15;

	// K-Mod
	if (GET_PLAYER(getOwner()).AI_isDoStrategy(AI_STRATEGY_CRUSH))
	{
		aiUnitAIVal[UNITAI_ATTACK_CITY] *= 2;
		aiUnitAIVal[UNITAI_ATTACK] *= 3;
		aiUnitAIVal[UNITAI_ATTACK] /= 2;
		aiUnitAIVal[UNITAI_ATTACK_AIR] *= 2;
	}
	if (GET_TEAM(getTeam()).AI_getRivalAirPower() <= 8 * GET_PLAYER(getOwner()).AI_totalAreaUnitAIs(area(), UNITAI_DEFENSE_AIR))
	{
		// unfortunately, I don't have an easy way to get the approximate power of our air defence units.
		// So I'm just going to assume the power of each unit is around 12 - the power of a fighter plane.
		aiUnitAIVal[UNITAI_DEFENSE_AIR] /= 4;
	}
	// K-Mod end

	for (iI = 0; iI < NUM_UNITAI_TYPES; iI++)
	{
		aiUnitAIVal[iI] *= std::max(0, (GC.getLeaderHeadInfo(getPersonalityType()).getUnitAIWeightModifier(iI) + 100));
		aiUnitAIVal[iI] /= 100;
	}

	iBestValue = 0;
	eBestUnit = NO_UNIT;

	for (iI = 0; iI < NUM_UNITAI_TYPES; iI++)
	{
		if (aiUnitAIVal[iI] > 0)
		{
			if (bAsync)
			{
				aiUnitAIVal[iI] += GC.getASyncRand().get(iMilitaryWeight, "AI Best UnitAI ASYNC");
			}
			else
			{
				aiUnitAIVal[iI] += GC.getGameINLINE().getSorenRandNum(iMilitaryWeight, "AI Best UnitAI");
			}

			if (aiUnitAIVal[iI] > iBestValue)
			{
				eUnit = AI_bestUnitAI(((UnitAITypes)iI), bAsync, eIgnoreAdvisor);

				if (eUnit != NO_UNIT)
				{
					iBestValue = aiUnitAIVal[iI];
					eBestUnit = eUnit;
					if (peBestUnitAI != NULL)
					{
						*peBestUnitAI = ((UnitAITypes)iI);
					}
				}
			}
		}
	}

	return eBestUnit;
}


UnitTypes CvCityAI::AI_bestUnitAI(UnitAITypes eUnitAI, bool bAsync, AdvisorTypes eIgnoreAdvisor)
{
	UnitTypes eLoopUnit;
	UnitTypes eBestUnit;
	int iValue;
	int iBestValue;
	int iOriginalValue;
	int iBestOriginalValue;
	int iI, iJ, iK;
	

	FAssertMsg(eUnitAI != NO_UNITAI, "UnitAI is not assigned a valid value");

	bool bGrowMore = false;

	if (foodDifference() > 0)
	{
		// BBAI NOTE: This is where small city worker and settler production is blocked
		if (GET_PLAYER(getOwnerINLINE()).getNumCities() <= 2)
		{
			/* original bts code
			bGrowMore = ((getPopulation() < 3) && (AI_countGoodTiles(true, false, 100) >= getPopulation())); */
			// K-Mod. We need to allow the starting city to build a worker at size 1.
			bGrowMore = (eUnitAI != UNITAI_WORKER || GET_PLAYER(getOwner()).AI_totalAreaUnitAIs(area(), UNITAI_WORKER) > 0)
				&& getPopulation() < 3 && AI_countGoodTiles(true, false, 100) >= getPopulation();
			// K-Mod end
		}
		else
		{
			bGrowMore = ((getPopulation() < 3) || (AI_countGoodTiles(true, false, 100) >= getPopulation()));
		}

		if (!bGrowMore && (getPopulation() < 6) && (AI_countGoodTiles(true, false, 80) >= getPopulation()))
		{
			if ((getFood() - (getFoodKept() / 2)) >= (growthThreshold() / 2))
			{
				if ((angryPopulation(1) == 0) && (healthRate(false, 1) == 0))
				{
					bGrowMore = true;
				}
			}
		}
		// K-Mod
		else if (bGrowMore)
		{
			if (angryPopulation(1) > 0)
				bGrowMore = false;
		}
		// K-Mod end
	}
	iBestOriginalValue = 0;

	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if (eLoopUnit != NO_UNIT)
		{
			if ((eIgnoreAdvisor == NO_ADVISOR) || (GC.getUnitInfo(eLoopUnit).getAdvisorType() != eIgnoreAdvisor))
			{
				if (!isHuman() || (GC.getUnitInfo(eLoopUnit).getDefaultUnitAIType() == eUnitAI))
				{
				    
					if (!(bGrowMore && isFoodProduction(eLoopUnit)))
					{
						if (canTrain(eLoopUnit))
						{
							iOriginalValue = GET_PLAYER(getOwnerINLINE()).AI_unitValue(eLoopUnit, eUnitAI, area());

							if (iOriginalValue > iBestOriginalValue)
							{
								iBestOriginalValue = iOriginalValue;
							}
						}
					}
				}
			}
		}
	}

	iBestValue = 0;
	eBestUnit = NO_UNIT;

	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if (eLoopUnit != NO_UNIT)
		{
			if ((eIgnoreAdvisor == NO_ADVISOR) || (GC.getUnitInfo(eLoopUnit).getAdvisorType() != eIgnoreAdvisor))
			{
				if (!isHuman() || (GC.getUnitInfo(eLoopUnit).getDefaultUnitAIType() == eUnitAI))
				{
				    
					if (!(bGrowMore && isFoodProduction(eLoopUnit)))
					{
						if (canTrain(eLoopUnit))
						{
							iValue = GET_PLAYER(getOwnerINLINE()).AI_unitValue(eLoopUnit, eUnitAI, area());

							if ((iValue > ((iBestOriginalValue * 2) / 3)) && ((eUnitAI != UNITAI_EXPLORE) || (iValue >= iBestOriginalValue)))
							{
								iValue *= (getProductionExperience(eLoopUnit) + 10);
								iValue /= 10;

                                //free promotions. slow?
                                //only 1 promotion per source is counted (ie protective isn't counted twice)
                                int iPromotionValue = 0;
                                //buildings
                                for (iJ = 0; iJ < GC.getNumPromotionInfos(); iJ++)
                                {
                                    if (isFreePromotion((PromotionTypes)iJ) && !GC.getUnitInfo(eLoopUnit).getFreePromotions((PromotionTypes)iJ))
                                    {
                                        if ((GC.getUnitInfo(eLoopUnit).getUnitCombatType() != NO_UNITCOMBAT) && GC.getPromotionInfo((PromotionTypes)iJ).getUnitCombat(GC.getUnitInfo(eLoopUnit).getUnitCombatType()))
                                        {
                                            iPromotionValue += 15;
                                            break;
                                        }
                                    }
                                }

                                //special to the unit
                                for (iJ = 0; iJ < GC.getNumPromotionInfos(); iJ++)
                                {
                                    if (GC.getUnitInfo(eLoopUnit).getFreePromotions(iJ))
                                    {
                                        iPromotionValue += 15;
                                        break;
                                    }
                                }

                                //traits
                                for (iJ = 0; iJ < GC.getNumTraitInfos(); iJ++)
                                {
                                    if (hasTrait((TraitTypes)iJ))
                                    {
                                        for (iK = 0; iK < GC.getNumPromotionInfos(); iK++)
                                        {
                                            if (GC.getTraitInfo((TraitTypes) iJ).isFreePromotion(iK))
                                            {
                                                if ((GC.getUnitInfo(eLoopUnit).getUnitCombatType() != NO_UNITCOMBAT) && GC.getTraitInfo((TraitTypes) iJ).isFreePromotionUnitCombat(GC.getUnitInfo(eLoopUnit).getUnitCombatType()))
                                                {
                                                    iPromotionValue += 15;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }

                                iValue *= (iPromotionValue + 100);
                                iValue /= 100;

								if (bAsync)
								{
									iValue *= (GC.getASyncRand().get(50, "AI Best Unit ASYNC") + 100);
									iValue /= 100;
								}
								else
								{
									iValue *= (GC.getGameINLINE().getSorenRandNum(50, "AI Best Unit") + 100);
									iValue /= 100;
								}


								int iBestHappy = 0;
								for (int iHurry = 0; iHurry < GC.getNumHurryInfos(); ++iHurry)
								{
									if (canHurryUnit((HurryTypes)iHurry, eLoopUnit, true))
									{
										int iHappy = AI_getHappyFromHurry((HurryTypes)iHurry, eLoopUnit, true);
										if (iHappy > iBestHappy)
										{
											iBestHappy = iHappy;
										}
									}
								}

								if (0 == iBestHappy)
								{
									iValue += getUnitProduction(eLoopUnit);
								}

								/* original bts code
								iValue *= (GET_PLAYER(getOwnerINLINE()).getNumCities() * 2);
								iValue /= (GET_PLAYER(getOwnerINLINE()).getUnitClassCountPlusMaking((UnitClassTypes)iI) + GET_PLAYER(getOwnerINLINE()).getNumCities() + 1); */
								// K-Mod
								{
									int iUnits = GET_PLAYER(getOwnerINLINE()).getUnitClassCountPlusMaking((UnitClassTypes)iI);
									int iCities = GET_PLAYER(getOwnerINLINE()).getNumCities();

									iValue *= 6 + iUnits + 4*iCities;
									iValue /= 3 + 3*iUnits + 2*iCities;
									// this is a factor between 1/3 and 2. It's equal to 1 roughly when iUnits == iCities.
								}
								// K-Mod end

								FAssert((MAX_INT / 1000) > iValue);
								iValue *= 1000;
						
								bool bIsSuicide = GC.getUnitInfo(eLoopUnit).isSuicide();
								
								if (bIsSuicide)
								{
									//much of this is compensated
									iValue /= 3;
								}

								if (0 == iBestHappy)
								{
									//iValue /= std::max(1, (getProductionTurnsLeft(eLoopUnit, 0) + (GC.getUnitInfo(eLoopUnit).isSuicide() ? 1 : 4)));
									// K-Mod. The number of turns is usually not so important. What's important is how much it is going to cost us to build this unit.
									int iProductionNeeded = getProductionNeeded(eLoopUnit);
									int iProductionModifier = getBaseYieldRateModifier(YIELD_PRODUCTION, getProductionModifier(eLoopUnit));
									FAssert(iProductionNeeded > 0 && iProductionModifier > 0);

									iValue *= iProductionModifier;
									iValue /= iProductionNeeded + getBaseYieldRate(YIELD_PRODUCTION)*iProductionModifier/100; // a bit of dilution.
									// maybe we should have a special bonus for building it in 1 turn?  ... maybe not.
									// K-Mod end
								}
								else
								{
									iValue *= (2 + 3 * iBestHappy);
									iValue /= 100;
								}

								iValue = std::max(1, iValue);

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									eBestUnit = eLoopUnit;
								}
							}
						}
					}
				}
			}
		}
	}

	return eBestUnit;
}


BuildingTypes CvCityAI::AI_bestBuilding(int iFocusFlags, int iMaxTurns, bool bAsync, AdvisorTypes eIgnoreAdvisor)
{
	return AI_bestBuildingThreshold(iFocusFlags, iMaxTurns, /*iMinThreshold*/ 0, bAsync, eIgnoreAdvisor);
}

BuildingTypes CvCityAI::AI_bestBuildingThreshold(int iFocusFlags, int iMaxTurns, int iMinThreshold, bool bAsync, AdvisorTypes eIgnoreAdvisor)
{
	/*BuildingTypes eLoopBuilding;
	BuildingTypes eBestBuilding;
	bool bAreaAlone;
	int iProductionRank;
	int iTurnsLeft;
	int iValue;
	int iTempValue;
	int iBestValue;
	int iI, iJ; */ // K-Mod. I've moved the declarations to where they are actually used - to reduce the likelihood of mistakes

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod (and I've replaced all other GET_PLAYER calls in this function)

	bool bAreaAlone = kOwner.AI_isAreaAlone(area());
	int iProductionRank = findYieldRateRank(YIELD_PRODUCTION);

	int iBestValue = 0;
	BuildingTypes eBestBuilding = NO_BUILDING;

	if (iFocusFlags & BUILDINGFOCUS_CAPITAL)
	{
		int iBestTurnsLeft = iMaxTurns > 0 ? iMaxTurns : MAX_INT;
		for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
		{
			BuildingTypes eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)));

			if (NO_BUILDING != eLoopBuilding)
			{
				if (GC.getBuildingInfo(eLoopBuilding).isCapital())
				{
					if (canConstruct(eLoopBuilding))
					{
						int iTurnsLeft = getProductionTurnsLeft(eLoopBuilding, 0);

						if (iTurnsLeft <= iBestTurnsLeft)
						{
							eBestBuilding = eLoopBuilding;
							iBestTurnsLeft = iTurnsLeft;
						}
					}
				}
			}
		}

		return eBestBuilding;
	}

	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		if (!(kOwner.isBuildingClassMaxedOut(((BuildingClassTypes)iI), GC.getBuildingClassInfo((BuildingClassTypes)iI).getExtraPlayerInstances())))
		{
			BuildingTypes eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)));

			if ((eLoopBuilding != NO_BUILDING) && (getNumBuilding(eLoopBuilding) < GC.getCITY_MAX_NUM_BUILDINGS())
			&&  (!isProductionAutomated() || !(isWorldWonderClass((BuildingClassTypes)iI) || isNationalWonderClass((BuildingClassTypes)iI))))
			{
				const CvBuildingInfo& kBuilding = GC.getBuildingInfo(eLoopBuilding); // K-Mod

				//don't build wonders?
				// BBAI TODO: Temp testing, remove once centralized building is working
				bool bWonderOk = false;
				//if( isHuman() || getOwner()%2 == 1 )
				//{
					bWonderOk = ((iFocusFlags == 0) || (iFocusFlags & BUILDINGFOCUS_WONDEROK) || (iFocusFlags & BUILDINGFOCUS_WORLDWONDER));
				//}

				if (bWonderOk || !isLimitedWonderClass((BuildingClassTypes)iI))
				{
					if ((eIgnoreAdvisor == NO_ADVISOR) || (kBuilding.getAdvisorType() != eIgnoreAdvisor))
					{
						if (canConstruct(eLoopBuilding))
						{
							//iValue = AI_buildingValueThreshold(eLoopBuilding, iFocusFlags, iMinThreshold);
							int iValue = AI_buildingValue(eLoopBuilding, iFocusFlags, iMinThreshold, bAsync); // K-Mod

							/* original bts code
							if (kBuilding.getFreeBuildingClass() != NO_BUILDINGCLASS)
							{
								BuildingTypes eFreeBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(kBuilding.getFreeBuildingClass());
								if (NO_BUILDING != eFreeBuilding)
								{
									//iValue += (AI_buildingValue(eFreeBuilding, iFocusFlags) * (kOwner.getNumCities() - kOwner.getBuildingClassCountPlusMaking((BuildingClassTypes)kBuilding.getFreeBuildingClass())));
									iValue += (AI_buildingValue(eFreeBuilding, iFocusFlags, 0, bAsync) * (kOwner.getNumCities() - kOwner.getBuildingClassCountPlusMaking((BuildingClassTypes)kBuilding.getFreeBuildingClass())));
								}
							} */ // Moved into AI_buildingValue

							// K-Mod
							TechTypes eObsoleteTech = (TechTypes)kBuilding.getObsoleteTech();
							TechTypes eSpObsoleteTech = kBuilding.getSpecialBuildingType() == NO_SPECIALBUILDING
								? NO_TECH
								: (TechTypes)GC.getSpecialBuildingInfo((SpecialBuildingTypes)(kBuilding.getSpecialBuildingType())).getObsoleteTech();

							if ((eObsoleteTech != NO_TECH && kOwner.getCurrentResearch() == eObsoleteTech) || (eSpObsoleteTech != NO_TECH && kOwner.getCurrentResearch() == eSpObsoleteTech))
								iValue /= 2;
							// K-Moe end

							if (isProductionAutomated())
							{
								for (int iJ = 0; iJ < GC.getNumBuildingClassInfos(); iJ++)
								{
									if (kBuilding.getPrereqNumOfBuildingClass(iJ) > 0)
									{
										iValue = 0;
										break;
									}
								}
							}

							if (iValue > 0)
							{
								int iTurnsLeft = getProductionTurnsLeft(eLoopBuilding, 0);

								// K-Mod
								// Block construction of limited buildings in bad places
								// (the value check is just for efficiency. 1250 takes into account the possible +25 random boost)
								if (iFocusFlags == 0 && iValue * 1250 / std::max(1, iTurnsLeft + 3) >= iBestValue)
								{
									int iLimit = limitedWonderClassLimit((BuildingClassTypes)iI);
									if (iLimit == -1)
									{
										// We're not out of the woods yet. Check for prereq buildings.
										for (int iJ = 0; iJ < GC.getNumBuildingClassInfos(); iJ++)
										{
											if (kBuilding.getPrereqNumOfBuildingClass(iJ) > 0)
											{
												// I wish this was easier to calculate...
												int iBuilt = kOwner.getBuildingClassCount((BuildingClassTypes)iI);
												int iBuilding = kOwner.getBuildingClassMaking((BuildingClassTypes)iI);
												int iPrereqEach = kOwner.getBuildingClassPrereqBuilding(eLoopBuilding, (BuildingClassTypes)iJ, -iBuilt);
												int iPrereqBuilt = kOwner.getBuildingClassCount((BuildingClassTypes)iJ);
												FAssert(iPrereqEach > 0);
												iLimit = iPrereqBuilt / iPrereqEach - iBuilt - iBuilding;
												FAssert(iLimit > 0);
												break;
											}
										}
									}
									if (iLimit != -1)
									{
										const int iMaxNumWonders = (GC.getGameINLINE().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman()) ? GC.getDefineINT("MAX_NATIONAL_WONDERS_PER_CITY_FOR_OCC") : GC.getDefineINT("MAX_NATIONAL_WONDERS_PER_CITY");
										int iRelativeValue = iValue;

										if (isNationalWonderClass((BuildingClassTypes)iI) && iMaxNumWonders != -1)
										{
											iRelativeValue *= iMaxNumWonders + 1 - getNumNationalWonders();
											iRelativeValue /= iMaxNumWonders + 1;
										}

										int iLoop;
										for (CvCity* pLoopCity = kOwner.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kOwner.nextCity(&iLoop))
										{
											if (pLoopCity->canConstruct(eLoopBuilding))
											{
												int iLoopValue = pLoopCity->AI_buildingValue(eLoopBuilding, 0, 0, bAsync);
												if (isNationalWonderClass((BuildingClassTypes)iI) && iMaxNumWonders != -1)
												{
													iLoopValue *= iMaxNumWonders + 1 - pLoopCity->getNumNationalWonders();
													iLoopValue /= iMaxNumWonders + 1;
												}
												if (80 * iLoopValue > 100 * iRelativeValue)
												{
													if (--iLimit <= 0)
													{
														iValue = 0;
														break;
													}
												}
											}
										}
										// Subtract some points from wonder value, just to stop us from wasting it
										if (isNationalWonderClass((BuildingClassTypes)iI))
											iValue -= 40;
									}
								}
								// K-Mod end

								if (isWorldWonderClass((BuildingClassTypes)iI))
								{
									if (iProductionRank <= std::min(3, ((kOwner.getNumCities() + 2) / 3)))
									{
										int iTempValue;
										if (bAsync)
										{
											iTempValue = GC.getASyncRand().get(GC.getLeaderHeadInfo(getPersonalityType()).getWonderConstructRand(), "Wonder Construction Rand ASYNC");
										}
										else
										{
											iTempValue = GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getWonderConstructRand(), "Wonder Construction Rand");
										}
										
										if (bAreaAlone)
										{
											iTempValue *= 2;
										}
										iValue += iTempValue;
									}
								}

								if (bAsync)
								{
									iValue *= (GC.getASyncRand().get(25, "AI Best Building ASYNC") + 100);
									iValue /= 100;
								}
								else
								{
									iValue *= (GC.getGameINLINE().getSorenRandNum(25, "AI Best Building") + 100);
									iValue /= 100;
								}

								iValue += getBuildingProduction(eLoopBuilding);


								bool bValid = ((iMaxTurns <= 0) ? true : false);
								if (!bValid)
								{
									bValid = (iTurnsLeft <= GC.getGameINLINE().AI_turnsPercent(iMaxTurns, GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getConstructPercent()));
								}
								if (!bValid)
								{
									for (int iHurry = 0; iHurry < GC.getNumHurryInfos(); ++iHurry)
									{
										if (canHurryBuilding((HurryTypes)iHurry, eLoopBuilding, true))
										{
											if (AI_getHappyFromHurry((HurryTypes)iHurry, eLoopBuilding, true) > 0)
											{
												bValid = true;
												break;
											}
										}
									}
								}
								
								if (bValid)
								{
									FAssert((MAX_INT / 1000) > iValue);
									iValue *= 1000;
									iValue /= std::max(1, (iTurnsLeft + 3));

									iValue = std::max(1, iValue);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										eBestBuilding = eLoopBuilding;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return eBestBuilding;
}


/* original BtS code. (I don't see the point of this function being separate to the "threshold" version)
int CvCityAI::AI_buildingValue(BuildingTypes eBuilding, int iFocusFlags) const
{
	return AI_buildingValueThreshold(eBuilding, iFocusFlags, 0);
} */

// XXX should some of these count cities, buildings, etc. based on teams (because wonders are shared...)
// XXX in general, this function needs to be more sensitive to what makes this city unique (more likely to build airports if there already is a harbor...)
// This function has been heavily edited for K-Mod
int CvCityAI::AI_buildingValue(BuildingTypes eBuilding, int iFocusFlags, int iThreshold, bool bConstCache, bool bAllowRecursion) const
{
	PROFILE_FUNC();

	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	BuildingClassTypes eBuildingClass = (BuildingClassTypes) kBuilding.getBuildingClassType();
	int iLimitedWonderLimit = limitedWonderClassLimit(eBuildingClass);
	bool bIsLimitedWonder = (iLimitedWonderLimit >= 0);
	// K-Mod. This new value, iPriorityFactor, is used to boost the value of productivity buildings without overvaluing productivity.
	// The point is to get the AI to build productiviy buildings quickly, but not if they come with large negative side effects.
	// I may use it for other adjustments in the future.
	int iPriorityFactor = 100;
	// K-Mod end

	// K-Mod, constructionValue cache
	if (iFocusFlags == 0 && iThreshold == 0 && m_aiConstructionValue[eBuildingClass] != -1)
	{
		return m_aiConstructionValue[eBuildingClass];
	}

	ReligionTypes eStateReligion = kOwner.getStateReligion();

	bool bAreaAlone = kOwner.AI_isAreaAlone(area());
	int iHasMetCount = GET_TEAM(getTeam()).getHasMetCivCount(true);

	int iFoodDifference = foodDifference(false);

	// Reduce reaction to temporary happy/health problems
	// K-Mod
	int iHappinessLevel = happyLevel() - unhappyLevel() + getEspionageHappinessCounter()/2 - getMilitaryHappiness()/2;
	int iHealthLevel = goodHealth() - badHealth() + getEspionageHealthCounter()/2;
	// K-Mod end

	bool bProvidesPower = (kBuilding.isPower() || ((kBuilding.getPowerBonus() != NO_BONUS) && hasBonus((BonusTypes)(kBuilding.getPowerBonus()))) || kBuilding.isAreaCleanPower());

	int iTotalPopulation = kOwner.getTotalPopulation();
	int iNumCities = kOwner.getNumCities();
	int iNumCitiesInArea = area()->getCitiesPerPlayer(getOwnerINLINE());
	
	bool bIsHighProductionCity = (findBaseYieldRateRank(YIELD_PRODUCTION) <= std::max(3, (iNumCities / 2)));
	
	int iCultureRank = findCommerceRateRank(COMMERCE_CULTURE);
	int iCulturalVictoryNumCultureCities = GC.getGameINLINE().culturalVictoryNumCultureCities();

	bool bFinancialTrouble = GET_PLAYER(getOwnerINLINE()).AI_isFinancialTrouble();

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
	bool bCulturalVictory1 = GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1);
	bool bCulturalVictory2 = GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2);
	bool bCulturalVictory3 = GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3);

	bool bSpaceVictory1 = GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_SPACE1);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/		

	bool bCanPopRush = GET_PLAYER(getOwnerINLINE()).canPopRush();
	bool bWarPlan = GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0; // K-Mod

	bool bForeignTrade = false;
	int iNumTradeRoutes = getTradeRoutes();
	for (int iI = 0; iI < iNumTradeRoutes; ++iI)
	{
		CvCity* pTradeCity = getTradeCity(iI);
		if (NULL != pTradeCity)
		{
			if (GET_PLAYER(pTradeCity->getOwnerINLINE()).getTeam() != getTeam() || pTradeCity->area() != area())
			{
				bForeignTrade = true;
				break;
			}
		}
	}

	if (kBuilding.isCapital())
	{
		return 0;
	}

	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (kBuilding.getReligionChange(iI) > 0)
		{
			if (!(GET_TEAM(getTeam()).hasHolyCity((ReligionTypes)iI)))
			{
				return 0;
			}
		}
	}

	int iValue = 0;

	// K-Mod (moved from AI_bestBuildingThreshold)
	if (kBuilding.getFreeBuildingClass() != NO_BUILDINGCLASS && bAllowRecursion)
	{
		BuildingTypes eFreeBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(kBuilding.getFreeBuildingClass());
		if (NO_BUILDING != eFreeBuilding)
		{
			iValue += (AI_buildingValue(eFreeBuilding, iFocusFlags, 0, bConstCache, false) * (kOwner.getNumCities() - kOwner.getBuildingClassCountPlusMaking((BuildingClassTypes)kBuilding.getFreeBuildingClass())));
		}
	}
	// K-Mod end

	for (int iPass = 0; iPass < 2; iPass++)
	{
		if ((iFocusFlags == 0) || (iValue > 0) || (iPass == 0))
		{
		    
		    if ((iFocusFlags & BUILDINGFOCUS_WORLDWONDER) || (iPass > 0))
		    {
		        if (isWorldWonderClass(eBuildingClass))
		        {
                    if (findBaseYieldRateRank(YIELD_PRODUCTION) <= 3)
                    {
						iValue++;
                    }
		        }
		    }
		    
			if ((iFocusFlags & BUILDINGFOCUS_DEFENSE) || (iPass > 0))
			{
				/* original bts code
				if (!bAreaAlone)
				{
					if ((GC.getGameINLINE().getBestLandUnit() == NO_UNIT) || !(GC.getUnitInfo(GC.getGameINLINE().getBestLandUnit()).isIgnoreBuildingDefense()))
					{
						iValue += (std::max(0, std::min(((kBuilding.getDefenseModifier() + getBuildingDefense()) - getNaturalDefense() - 10), kBuilding.getDefenseModifier())) / 4);
					}
				}

				iValue += kBuilding.getBombardDefenseModifier() / 8;
				
				iValue += -kBuilding.getAirModifier() / 4;
				iValue += -kBuilding.getNukeModifier() / 4;
				
				iValue += ((kBuilding.getAllCityDefenseModifier() * iNumCities) / 5);
				
				iValue += kBuilding.getAirlift() * 25; */

				// K-Mod. I've merged and modified the code from here and the code from higher up.
				if (!bAreaAlone)
				{
					if (GC.getGameINLINE().getBestLandUnit() == NO_UNIT || !GC.getUnitInfo(GC.getGameINLINE().getBestLandUnit()).isIgnoreBuildingDefense())
					{
						// bRemove means that we're evaluating the cost of losing this building rather than adding it.
						// the definition used here is just a kludge because there currently isn't any other to tell the difference.
						// Also, this isn't the only part of the evaluation that depends on whether we are adding or removing
						// but this is a particular important case due to the way walls and castles get obsoleted...
						bool bRemove = getNumActiveBuilding(eBuilding) > 0;

						int iTemp = 0;
						// bombard reduction
						int iOldBombardMod = getBuildingBombardDefense() - (bRemove ? kBuilding.getBombardDefenseModifier() : 0);
						iTemp += std::max((bRemove ?0 :kBuilding.getDefenseModifier()) + getBuildingDefense(), getNaturalDefense()) * std::min(kBuilding.getBombardDefenseModifier(), 100-iOldBombardMod) / std::max(80, 8 * (100 - iOldBombardMod));
						// defence bonus
						iTemp += std::max(0, std::min((bRemove? 0 :kBuilding.getDefenseModifier()) + getBuildingDefense() - getNaturalDefense() - 10, kBuilding.getDefenseModifier())) / 4;

						iTemp *= (bWarPlan ? 3 : 2);
						iTemp /= 3;
						iValue += iTemp;
					}
				}
				iValue += kBuilding.getAirlift() * 10 + (iNumCitiesInArea < iNumCities && kBuilding.getAirlift() > 0 ? getPopulation()+25 : 0);

				int iAirDefense = -kBuilding.getAirModifier();
				if (iAirDefense > 0)
				{
					int iTemp = iAirDefense;
					iTemp *= std::min(200, 100 * GET_TEAM(getTeam()).AI_getRivalAirPower() / std::max(1, GET_TEAM(getTeam()).AI_getAirPower()+4*iNumCities));
					iTemp /= 200;
					iValue += iTemp;
				}

				iValue += -kBuilding.getNukeModifier() / (GC.getGameINLINE().isNukesValid() && !GC.getGameINLINE().isNoNukes() ? 4 : 40);
				// K-Mod end
			}

			if ((iFocusFlags & BUILDINGFOCUS_ESPIONAGE) || (iPass > 0))
			{
				iValue += kBuilding.getEspionageDefenseModifier() / 8;
			}

			if (((iFocusFlags & BUILDINGFOCUS_HAPPY) || (iPass > 0)) && !isNoUnhappiness())
			{
				int iBestHappy = 0;
				for (int iI = 0; iI < GC.getNumHurryInfos(); iI++)
				{
					if (canHurryBuilding((HurryTypes)iI, eBuilding, true))
					{
						int iHappyFromHurry = AI_getHappyFromHurry((HurryTypes)iI, eBuilding, true);
						if (iHappyFromHurry > iBestHappy)
						{
							iBestHappy = iHappyFromHurry;
						}
					}
				}
				iValue += iBestHappy * 10;

				if (kBuilding.isNoUnhappiness())
				{
					//iValue += ((iAngryPopulation * 10) + getPopulation());
					iValue += ((std::max(0, -iHappinessLevel) * 10) + getPopulation()); // K-Mod
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/24/10                              jdog5000        */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
				int iGood, iBad = 0;
				int iBuildingActualHappiness = getAdditionalHappinessByBuilding(eBuilding,iGood,iBad);

				/* original code
				if( iBuildingActualHappiness < 0 )
				{
					// Building causes net decrease in city happiness
					iValue -= (-iBuildingActualHappiness + iAngryPopulation) * 6
						+ (-iBuildingActualHappiness) * iHappyModifier;

					// BBAI TODO: Check for potential shrink in population
					
				}
				else if( iBuildingActualHappiness > 0 )
				{
					// Building causes net increase in city happiness
					iValue += (std::min(iBuildingActualHappiness, iAngryPopulation) * 10) 
						+ (std::max(0, iBuildingActualHappiness - iAngryPopulation) * iHappyModifier);
				}*/
				// K-Mod
				int iCitValue = 6 + kOwner.getCurrentEra(); // (estimating citizen value to be 6 + era commerce per turn)
				int iAngerDelta = std::max(0, -(iHappinessLevel+iBuildingActualHappiness)) - std::max(0, -iHappinessLevel);
				// High value for any immediate change in anger.
				iValue -= iAngerDelta * 4 * iCitValue;
				// some extra value if we are still growing (this is a positive change bias)
				if (iAngerDelta < 0 && iFoodDifference > 1)
				{
					iValue -= 10 * iAngerDelta;
				}
				// finally, a little bit of value for happiness which gives us some padding
				iValue += 16 * iCitValue * std::max(0, iBuildingActualHappiness)/(4 + std::max(0, iHappinessLevel+iBuildingActualHappiness) + std::max(0, iHappinessLevel));

				// I'll now define the "iHappinessModifer" that some of the other happy effects use.
				int iHappyModifier = (iHappinessLevel <= iHealthLevel && iHappinessLevel <= 4) ? iCitValue : iCitValue/2;
				if (iHappinessLevel >= 10)
				{
					iHappyModifier = 1;
				}
				// K-Mod end

				iValue += (-kBuilding.getHurryAngerModifier() * getHurryPercentAnger()) / 100;

				for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
				{
					iValue += (kBuilding.getCommerceHappiness(iI) * iHappyModifier) / 4;
				}

				int iWarWearinessModifer = kBuilding.getWarWearinessModifier();
				if (iWarWearinessModifer != 0)
				{
					iValue += (-iWarWearinessModifer * iHappyModifier) / 16;
				}

				/*iValue += (kBuilding.getAreaHappiness() * (iNumCitiesInArea - 1) * 8);
				iValue += (kBuilding.getGlobalHappiness() * iNumCities * 8);*/
				// K-Mod - just a tweak.. nothing fancy.
				{
					int iTargetCities = GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities();
					iValue += kBuilding.getAreaHappiness() * (iNumCitiesInArea + iTargetCities/3) * 8;
					iValue += kBuilding.getGlobalHappiness() * (iNumCities + iTargetCities/2) * 8;
				}
				// K-Mod end

				int iWarWearinessPercentAnger = kOwner.getWarWearinessPercentAnger();
				int iGlobalWarWearinessModifer = kBuilding.getGlobalWarWearinessModifier();
				if (iGlobalWarWearinessModifer != 0)
				{
					iValue += (-(((iGlobalWarWearinessModifer * iWarWearinessPercentAnger / 100) / GC.getPERCENT_ANGER_DIVISOR())) * iNumCities);
					iValue += (-iGlobalWarWearinessModifer * iHappyModifier) / 16;
				}

				for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
				{
					if (kBuilding.getBuildingHappinessChanges(iI) != 0)
					{
						iValue += (kBuilding.getBuildingHappinessChanges(iI) * kOwner.getBuildingClassCount((BuildingClassTypes)iI) * 8);
					}
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			}

			//if (((iFocusFlags & BUILDINGFOCUS_HEALTHY) || (iPass > 0)) && !isNoUnhealthyPopulation())
			if ((iFocusFlags & BUILDINGFOCUS_HEALTHY) || iPass > 0) // K-Mod
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/24/10                          jdog5000 & Fuyu     */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
				int iGood, iBad = 0;
				int iBuildingActualHealth = getAdditionalHealthByBuilding(eBuilding,iGood,iBad);
/*
** K-Mod
** rewrote the evaluation of building health.
*/
				/* original
				if( iBuildingActualHealth < 0 )
				{
					// Building causes net decrease in city health
					iValue -= (-iBuildingActualHealth + iBadHealth) * 6
						+ (-iBuildingActualHealth) * iHealthModifier;

					// BBAI TODO: Check for potential shrink in population
					
				}
				else if( iBuildingActualHealth > 0 )
				{
					// Building causes net increase in city health
					iValue += (std::min(iBuildingActualHealth, iBadHealth) * 10)
						+ (std::max(0, iBuildingActualHealth - iBadHealth) * iHealthModifier);
				}*/

				int iCitValue = 6 + kOwner.getCurrentEra(); // (estimating citizen value to be 6 + era commerce per turn)
				int iWasteDelta = std::max(0, -(iHealthLevel+iBuildingActualHealth)) - std::max(0, -iHealthLevel);
				// High value for change in our food deficit.
				iValue -= 2 * iCitValue * (std::max(0, -(iFoodDifference - iWasteDelta)) - std::max(0, -iFoodDifference));
				// medium value for change in waste
				iValue -= iCitValue * iWasteDelta;
				// some extra value if the change will help us grow (this is a positive change bias)
				if (iWasteDelta < 0 && iHappinessLevel > 0)
				{
					iValue -= iCitValue * iWasteDelta;
				}
				// finally, a little bit of value for health which gives us some padding
				iValue += 10 * iCitValue * std::max(0, iBuildingActualHealth)/(6 + std::max(0, iHealthLevel+iBuildingActualHealth) + std::max(0, iHealthLevel));

				// If the GW threshold has been reached,
				// add some additional value for pollution reduction
				// Note. health benefits have already been evaluated
				if (iBad < 0 && GC.getGameINLINE().getGlobalWarmingIndex() > 0)
				{
					int iCleanValue = -2*iBad;

					iCleanValue *= (100 + 5*kOwner.getGwPercentAnger());
					iCleanValue /= 100;

					FAssert(iCleanValue >= 0);

					iValue += iCleanValue;
				}
/*
** K-Mod end
*/

				iValue += (kBuilding.getAreaHealth() * (iNumCitiesInArea-1) * 4);
				iValue += (kBuilding.getGlobalHealth() * iNumCities * 4);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			}

			if (iFocusFlags & BUILDINGFOCUS_EXPERIENCE || iPass > 0)
			{
				/* original bts code
				iValue += (kBuilding.getFreeExperience() * ((iHasMetCount > 0) ? 12 : 6));

				for (int iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
				{
					if (canTrain((UnitCombatTypes)iI))
					{
						iValue += (kBuilding.getUnitCombatFreeExperience(iI) * ((iHasMetCount > 0) ? 6 : 3));
					}
				}

				for (int iI = 0; iI < NUM_DOMAIN_TYPES; iI++)
				{
					int iDomainExpValue = 0;
					if (iI == DOMAIN_SEA)
					{
						iDomainExpValue = 7;
					} 
					else if (iI == DOMAIN_LAND)
					{
						iDomainExpValue = 12;
					}
					else
					{
						iDomainExpValue = 6;
					}
					iValue += (kBuilding.getDomainFreeExperience(iI) * ((iHasMetCount > 0) ? iDomainExpValue : iDomainExpValue / 2));
				} */
				// K-Mod.
				// note. currently this new code matches the old code exactly,
				// except that the value is reduced in cities that we don't expect to be building troops in.
				int iWeight = 12;
				iWeight /= iHasMetCount > 0 ? 1 : 2;
				iWeight /= bWarPlan || bIsHighProductionCity ? 1 : 3;

				iValue += kBuilding.getFreeExperience() * iWeight;

				for (int iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
				{
					if (canTrain((UnitCombatTypes)iI))
					{
						iValue += kBuilding.getUnitCombatFreeExperience(iI) * iWeight / 2;
					}
				}

				for (int iI = 0; iI < NUM_DOMAIN_TYPES; iI++)
				{
					int iDomainExpValue = 0;
					switch (iI)
					{
					case DOMAIN_LAND:
						// full value
						iValue += kBuilding.getDomainFreeExperience(iI) * iWeight;
						break;
					case DOMAIN_SEA:
						// special case. Don't use 'iWeight', because for sea bIsHighProductionCity may be too strict.
						iValue += kBuilding.getDomainFreeExperience(iI) * 7;
						break;
					default:
						// half value.
						iValue += kBuilding.getDomainFreeExperience(iI) * iWeight/2;
						break;
					}
				}
				// K-Mod end
			}

			// since this duplicates BUILDINGFOCUS_EXPERIENCE checks, do not repeat on pass 1
			if ((iFocusFlags & BUILDINGFOCUS_DOMAINSEA))
			{
				iValue += (kBuilding.getFreeExperience() * ((iHasMetCount > 0) ? 16 : 8));

				for (int iUnitIndex = 0; iUnitIndex < GC.getNumUnitClassInfos(); iUnitIndex++)
				{
					UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iUnitIndex);

					if (NO_UNIT != eUnit)
					{
						CvUnitInfo& kUnitInfo = GC.getUnitInfo(eUnit);
						int iCombatType = kUnitInfo.getUnitCombatType();
						if (kUnitInfo.getDomainType() == DOMAIN_SEA && canTrain(eUnit) && iCombatType != NO_UNITCOMBAT)
						{
							iValue += (kBuilding.getUnitCombatFreeExperience(iCombatType) * ((iHasMetCount > 0) ? 6 : 3));
						}
					}
				}

				iValue += (kBuilding.getDomainFreeExperience(DOMAIN_SEA) * ((iHasMetCount > 0) ? 16 : 8));

				iValue += (kBuilding.getDomainProductionModifier(DOMAIN_SEA) / 4);
			}

			if ((iFocusFlags & BUILDINGFOCUS_MAINTENANCE) || (iFocusFlags & BUILDINGFOCUS_GOLD) || (iPass > 0))
			{

				/* original bts code
				int iBaseMaintenance = getMaintenanceTimes100();
				int iExistingUpkeep = (iBaseMaintenance * std::max(0, 100 + getMaintenanceModifier())) / 100;
				int iNewUpkeep = (iBaseMaintenance * std::max(0, 100 + getMaintenanceModifier() + kBuilding.getMaintenanceModifier())) / 100;
				*/
				// K-Mod, bugfix. getMaintenanceTimes100 is the total, with modifiers already applied.
				// (note. ideally we'd use "calculateBaseMaintenanceTimes100", and that would avoid problem caused by "we love the X day".
				// but doing it this way is slightly faster.)
				int iExistingUpkeep = getMaintenanceTimes100();
				int iBaseMaintenance = 100 * iExistingUpkeep / std::max(1, 100 + getMaintenanceModifier());
				int iNewUpkeep = (iBaseMaintenance * std::max(0, 100 + getMaintenanceModifier() + kBuilding.getMaintenanceModifier())) / 100;
				// K-Mod end
				
				int iTempValue = (iExistingUpkeep - iNewUpkeep) / 16;
				
				if (bFinancialTrouble)
				{
					iTempValue *= 2;
				}

				iValue += iTempValue;
			}

			if ((iFocusFlags & BUILDINGFOCUS_SPECIALIST) || (iPass > 0))
			{
				int iSpecialistsValue = 0;
				int iCurrentSpecialistsRunnable = 0;
				SpecialistTypes eDefaultSpecialist = (SpecialistTypes)GC.getDefineINT("DEFAULT_SPECIALIST");
				for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
				{
					if (iI != eDefaultSpecialist)
					{
						bool bUnlimited = (GET_PLAYER(getOwnerINLINE()).isSpecialistValid((SpecialistTypes)iI));
						int iRunnable = (getMaxSpecialistCount((SpecialistTypes)iI) > 0);
						
						if (bUnlimited || (iRunnable > 0))
						{
							if (bUnlimited)
							{
								iCurrentSpecialistsRunnable += 5;
							}
							else
							{
								iCurrentSpecialistsRunnable += iRunnable;
							}
						}
						
						
						if (kBuilding.getSpecialistCount(iI) > 0)
						{
							if ((!bUnlimited) && (iRunnable < 5))
							{
								int iTempValue = AI_specialistValue(((SpecialistTypes)iI), false, false);

								iTempValue *= (20 + (40 * kBuilding.getSpecialistCount(iI)));
								iTempValue /= 100;

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       01/09/10                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
								if (iFoodDifference < 2)
								{
									iValue /= 4;
								}
								if (iRunnable > 0)
								{
									iValue /= 1 + iRunnable;
								}
*/
								if (iFoodDifference < 2)
								{
									iTempValue /= 4;
								}
								if (iRunnable > 0)
								{
									iTempValue /= 1 + iRunnable;
								}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

								iSpecialistsValue += std::max(12, (iTempValue / 100));
							}
						}
					}
				}
				
				if (iSpecialistsValue > 0)
				{
					iValue += iSpecialistsValue / std::max(2, iCurrentSpecialistsRunnable);
				}
			}
			
			if ((iFocusFlags & (BUILDINGFOCUS_GOLD | BUILDINGFOCUS_RESEARCH)) || iPass > 0)
			{
				// trade routes
				/* original bts code
				int iTempValue = ((kBuilding.getTradeRoutes() * ((8 * std::max(0, (totalTradeModifier() + 100))) / 100))
								* (getPopulation() / 5 + 1));
				int iGlobalTradeValue = (((6 * iTotalPopulation) / 5) / iNumCities);

				iTempValue += (kBuilding.getCoastalTradeRoutes() * kOwner.countNumCoastalCities() * iGlobalTradeValue);
				iTempValue += (kBuilding.getGlobalTradeRoutes() * iNumCities * iGlobalTradeValue);

				iTempValue += ((kBuilding.getTradeRouteModifier() * getTradeYield(YIELD_COMMERCE)) / (bForeignTrade ? 12 : 25));
				if (bForeignTrade)
				{
					iTempValue += ((kBuilding.getForeignTradeRouteModifier() * getTradeYield(YIELD_COMMERCE)) / 12);
				} */
				// K-Mod
				int iTotalTradeModifier = totalTradeModifier();
				int iTempValue = kBuilding.getTradeRoutes() * (getTradeRoutes() > 0 ? 6*getTradeYield(YIELD_COMMERCE) / getTradeRoutes() : 6 * (getPopulation() / 5 + 1) * iTotalTradeModifier / 100);
				int iGlobalTradeValue = (6 * iTotalPopulation / (5 * iNumCities) + 1) * kOwner.AI_averageYieldMultiplier(YIELD_COMMERCE) / 100;

				iTempValue += 6 * kBuilding.getTradeRouteModifier() * getTradeYield(YIELD_COMMERCE) / std::max(1, iTotalTradeModifier);
				if (bForeignTrade)
				{
					iTempValue += 4 * kBuilding.getForeignTradeRouteModifier() * getTradeYield(YIELD_COMMERCE) / std::max(1, iTotalTradeModifier+getForeignTradeRouteModifier());
				}

				iTempValue *= AI_yieldMultiplier(YIELD_COMMERCE);
				iTempValue /= 100;

				iTempValue += (kBuilding.getCoastalTradeRoutes() * kOwner.countNumCoastalCities() * iGlobalTradeValue);
				iTempValue += (kBuilding.getGlobalTradeRoutes() * iNumCities * iGlobalTradeValue);
				// K-Mod end
					
				if (bFinancialTrouble)
				{
					iTempValue *= 2;
				}
					
				if (kOwner.isNoForeignTrade())
				{
					iTempValue /= 2; // was /3
				}


				iValue += iTempValue;
			}

			if (iPass > 0)
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/24/10                       jdog5000 & Afforess    */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
				if (kBuilding.isAreaCleanPower() && !(area()->isCleanPower(getTeam())))
				{
					int iLoop;
					for( CvCity* pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop) )
					{
						if( pLoopCity->area() == area() )
						{
							if( pLoopCity->isDirtyPower() )
							{
								iValue += 12;
							}
							else if( !(pLoopCity->isPower()) )
							{
								iValue += 8;
							}
						}
					}
				}

				if (kBuilding.getDomesticGreatGeneralRateModifier() != 0)
				{
					iValue += (kBuilding.getDomesticGreatGeneralRateModifier() / 10);
				}

				if (kBuilding.isAreaBorderObstacle() && !(area()->isBorderObstacle(getTeam())))
				{
					if( !GC.getGameINLINE().isOption(GAMEOPTION_NO_BARBARIANS) )
					{
						iValue += (iNumCitiesInArea);

						if(GC.getGameINLINE().isOption(GAMEOPTION_RAGING_BARBARIANS))
						{
							iValue += (iNumCitiesInArea);
						}
					}
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				if (kBuilding.isGovernmentCenter())
				{
					FAssert(!(kBuilding.isCapital()));
					/* original bts code
					iValue += ((calculateDistanceMaintenance() - 3) * iNumCitiesInArea); */
					// K-mod. More bonus for colonies, because it reduces that extra maintenance too.
					int iTempValue = (calculateDistanceMaintenance() - 3) * iNumCitiesInArea;
					const CvCity* pCapitalCity = kOwner.getCapitalCity();
					if (pCapitalCity == NULL || pCapitalCity->area() != area())
					{
						iTempValue *= 2;
					}
					iValue += iTempValue;
					// K-Mod end
				}

				if (kBuilding.isMapCentering())
				{
					iValue++;
				}

				if (kBuilding.getFreeBonus() != NO_BONUS)
				{
					iValue += (kOwner.AI_bonusVal((BonusTypes)(kBuilding.getFreeBonus()), 1) *
						         ((kOwner.getNumTradeableBonuses((BonusTypes)(kBuilding.getFreeBonus())) == 0) ? 2 : 1) *
						         (iNumCities + kBuilding.getNumFreeBonuses()));
				}

				if (kBuilding.getNoBonus() != NO_BONUS)
				{
					iValue -= kOwner.AI_bonusVal((BonusTypes)kBuilding.getNoBonus());
				}

				if (kBuilding.getFreePromotion() != NO_PROMOTION)
				{
					/* original bts code
					iValue += ((iHasMetCount > 0) ? 100 : 40); // XXX some sort of promotion value??? */
					// K-Mod.
					// Ideally, we'd use AI_promotionValue to work out what the promotion is worth
					// but unfortunately, that function requires a target unit, and I can't think of a good
					// way to choose a suitable unit for evaluation.
					// So.. I'm just going to do a really basic kludge to stop the Dun from being worth more than Red Cross
					const CvPromotionInfo& kInfo = GC.getPromotionInfo((PromotionTypes)kBuilding.getFreePromotion());
					bool bAdvanced = kInfo.getPrereqPromotion() != NO_PROMOTION ||
						kInfo.getPrereqOrPromotion1() != NO_PROMOTION || kInfo.getPrereqOrPromotion2() != NO_PROMOTION || kInfo.getPrereqOrPromotion3() != NO_PROMOTION;
					int iTemp = (bAdvanced ? 200 : 40);
					int iProduction = getYieldRate(YIELD_PRODUCTION);
					iTemp *= 2*iProduction;
					iTemp /= 30 + iProduction;
					iTemp *= getFreeExperience() + 1;
					iTemp /= getFreeExperience() + 2;
					iValue += iTemp;
					// cf. iValue += (kBuilding.getFreeExperience() * ((iHasMetCount > 0) ? 12 : 6));
					// K-Mod end
				}

				if (kBuilding.getCivicOption() != NO_CIVICOPTION)
				{
					for (int iI = 0; iI < GC.getNumCivicInfos(); iI++)
					{
						if (GC.getCivicInfo((CivicTypes)iI).getCivicOptionType() == kBuilding.getCivicOption())
						{
							if (!(kOwner.canDoCivics((CivicTypes)iI)))
							{
								iValue += (kOwner.AI_civicValue((CivicTypes)iI) / 10);
							}
						}
					}
				}
				
				int iGreatPeopleRateModifier = kBuilding.getGreatPeopleRateModifier();
				if (iGreatPeopleRateModifier > 0)
				{
					int iGreatPeopleRate = getBaseGreatPeopleRate();
					const int kTargetGPRate = 10;

					// either not a wonder, or a wonder and our GP rate is at least the target rate
					if (!bIsLimitedWonder || iGreatPeopleRate >= kTargetGPRate)
					{
						iValue += ((iGreatPeopleRateModifier * iGreatPeopleRate) / 16);
					}
					// otherwise, this is a limited wonder (aka National Epic), we _really_ do not want to build this here
					// subtract from the value (if this wonder has a lot of other stuff, we still might build it)
					else
					{
						iValue -= ((iGreatPeopleRateModifier * (kTargetGPRate - iGreatPeopleRate)) / 12);
					}
				}

				iValue += ((kBuilding.getGlobalGreatPeopleRateModifier() * iNumCities) / 8);

				iValue += (-(kBuilding.getAnarchyModifier()) / 4);

				iValue += (-(kBuilding.getGlobalHurryModifier()) * 2);

				iValue += (kBuilding.getGlobalFreeExperience() * iNumCities * ((iHasMetCount > 0) ? 6 : 3));

				/* original bts code. (moved to where the rest of foodKept is valued)
				if (bCanPopRush)
				{
					iValue += kBuilding.getFoodKept() / 2;
				} */

				/* original bts code. (This stuff is already counted in the defense section.)
				iValue += kBuilding.getAirlift() * (getPopulation()*3 + 10);
				
				int iAirDefense = -kBuilding.getAirModifier();
				if (iAirDefense > 0)
				{
					if (((kOwner.AI_totalUnitAIs(UNITAI_DEFENSE_AIR) > 0) && (kOwner.AI_totalUnitAIs(UNITAI_ATTACK_AIR) > 0)) || (kOwner.AI_totalUnitAIs(UNITAI_MISSILE_AIR) > 0))
					{
						iValue += iAirDefense / ((iHasMetCount > 0) ? 2 : 4);
					}
				}

				iValue += kBuilding.getAirUnitCapacity() * (getPopulation() * 2 + 10);

				iValue += (-(kBuilding.getNukeModifier()) / ((iHasMetCount > 0) ? 10 : 20)); */

				iValue += std::max(0, kBuilding.getAirUnitCapacity() - plot()->airUnitSpaceAvailable(getTeam())/2) * (getPopulation() + 12); // K-Mod

				iValue += (kBuilding.getFreeSpecialist() * 16);
				iValue += (kBuilding.getAreaFreeSpecialist() * iNumCitiesInArea * 12);
				iValue += (kBuilding.getGlobalFreeSpecialist() * iNumCities * 12);

				iValue += ((kBuilding.getWorkerSpeedModifier() * kOwner.AI_getNumAIUnits(UNITAI_WORKER)) / 10);

				int iMilitaryProductionModifier = kBuilding.getMilitaryProductionModifier();
				if (iHasMetCount > 0 && iMilitaryProductionModifier > 0)
				{
					// either not a wonder, or a wonder and we are a high production city
					if (!bIsLimitedWonder || bIsHighProductionCity)
					{
						iValue += (iMilitaryProductionModifier / 4);

						// if a wonder, then pick one of the best cities
						if (bIsLimitedWonder)
						{
							// if one of the top 3 production cities, give a big boost
							if (findBaseYieldRateRank(YIELD_PRODUCTION) <= (2 + iLimitedWonderLimit))
							{
								iValue += (2 * iMilitaryProductionModifier) / (2 + findBaseYieldRateRank(YIELD_PRODUCTION));
							}
						}
						// otherwise, any of the top half of cities will do
						else if (bIsHighProductionCity)
						{
							iValue += iMilitaryProductionModifier / 4;
						}
						iValue += ((iMilitaryProductionModifier * (getFreeExperience() + getSpecialistFreeExperience())) / 10);
					}
					// otherwise, this is a limited wonder (aka Heroic Epic), we _really_ do not want to build this here
					// subtract from the value (if this wonder has a lot of other stuff, we still might build it)
					else
					{
						iValue -= (iMilitaryProductionModifier * findBaseYieldRateRank(YIELD_PRODUCTION)) / 5;
					}
				}

				iValue += (kBuilding.getSpaceProductionModifier() / 5);
				iValue += ((kBuilding.getGlobalSpaceProductionModifier() * iNumCities) / 20);
				

				if (kBuilding.getGreatPeopleUnitClass() != NO_UNITCLASS)
				{
					iValue++; // XXX improve this for diversity...
				}
				
				// prefer to build great people buildings in places that already have some GP points
				//iValue += (kBuilding.getGreatPeopleRateChange() * 10) * (1 + (getBaseGreatPeopleRate() / 2));
				// K-Mod... here's some code like the specialist value function. Kind of long, but much better.
				{
					int iTempValue = 100 * kBuilding.getGreatPeopleRateChange() * 2 * 4; // everything seems to be x4 around here
					int iCityRate = getGreatPeopleRate();
					int iHighestRate = 0;
					int iLoop;
					for( CvCity* pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop) )
					{
						int x = pLoopCity->getGreatPeopleRate();
						if (x > iHighestRate)
							iHighestRate = x;
					}
					if (iHighestRate > iCityRate)
					{
						iTempValue *= 100;
						iTempValue /= (2*100*(iHighestRate+3))/(iCityRate+3) - 100;
					}
					
					iTempValue *= getTotalGreatPeopleRateModifier();
					iTempValue /= 100;

					iValue += iTempValue / 100;
				}
				// K-Mod end

				if (!bAreaAlone)
				{
					iValue += (kBuilding.getHealRateChange() / 2);
				}

				iValue += (kBuilding.getGlobalPopulationChange() * iNumCities * 4);

				iValue += (kBuilding.getFreeTechs() * 80);

				iValue += kBuilding.getEnemyWarWearinessModifier() / 2;

				for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
				{
					if (kBuilding.getFreeSpecialistCount(iI) > 0)
					{
						//iValue += ((AI_specialistValue(((SpecialistTypes)iI), false, false) * kBuilding.getFreeSpecialistCount(iI)) / 50);
						// K-Mod
						iValue += AI_permanentSpecialistValue((SpecialistTypes)iI) * kBuilding.getFreeSpecialistCount(iI) / 100;
					}
				}

				for (int iI = 0; iI < GC.getNumImprovementInfos(); iI++)
				{
					if (kBuilding.getImprovementFreeSpecialist(iI) > 0)
					{
						iValue += kBuilding.getImprovementFreeSpecialist(iI) * countNumImprovedPlots((ImprovementTypes)iI, true) * 50;
					}
				}

				for (int iI = 0; iI < NUM_DOMAIN_TYPES; iI++)
				{
					iValue += (kBuilding.getDomainProductionModifier(iI) / 5);
					
					if (bIsHighProductionCity)
					{
						iValue += (kBuilding.getDomainProductionModifier(iI) / 5);
					}
				}

				for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
				{
					if (GC.getUnitInfo((UnitTypes)iI).getPrereqBuilding() == eBuilding)
					{
						// BBAI TODO: Smarter monastary construction, better support for mods

						if (kOwner.AI_totalAreaUnitAIs(area(), ((UnitAITypes)(GC.getUnitInfo((UnitTypes)iI).getDefaultUnitAIType()))) == 0)
						{
							iValue += iNumCitiesInArea;
						}

						iValue++;

						ReligionTypes eReligion = (ReligionTypes)(GC.getUnitInfo((UnitTypes)iI).getPrereqReligion());
						if (eReligion != NO_RELIGION)
						{
							//encouragement to get some minimal ability to train special units
							if (bCulturalVictory1 || isHolyCity(eReligion) || isCapital())
							{
								iValue += (2 + iNumCitiesInArea);
							}
							
							if (bCulturalVictory2 && GC.getUnitInfo((UnitTypes)iI).getReligionSpreads(eReligion))
							{
								//this gives a very large extra value if the religion is (nearly) unique
								//to no extra value for a fully spread religion.
								//I'm torn between huge boost and enough to bias towards the best monastery type.
								int iReligionCount = GET_PLAYER(getOwnerINLINE()).getHasReligionCount(eReligion);
								iValue += (100 * (iNumCities - iReligionCount)) / (iNumCities * (iReligionCount + 1));
							}
						}
					}
				}
				
				// is this building needed to build other buildings?

				// K-Mod
				// (I've deleted the original code for this section.)
				if (bAllowRecursion)
				{
					int iCountMaking = -1; // we'll count it when it is first needed.

					for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
					{
						BuildingTypes eLoopBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings((BuildingClassTypes)iI); // K-Mod
						if (eLoopBuilding == NO_BUILDING)
							continue;

						if (GC.getBuildingInfo(eLoopBuilding).getPrereqNumOfBuildingClass(eBuildingClass) <= 0)
							continue;

						if (iCountMaking < 0)
							iCountMaking = kOwner.getBuildingClassMaking(eBuildingClass);

						int iPrereqBuildings = kOwner.getBuildingClassPrereqBuilding(eLoopBuilding, eBuildingClass, iCountMaking);

						// if we need some of us to build iI building, and we dont need more than we have cities
						if (iPrereqBuildings > 0 && iPrereqBuildings <= iNumCities)
						{
							// do we need more than what we are currently building?
							iPrereqBuildings -= kOwner.getBuildingClassCountPlusMaking(eBuildingClass);
							if (iPrereqBuildings > 0)
							{
								// K-Mod, lets do it the long and slow way...
								CvCity* pLoopCity;
								int iLoop;
								int iHighestValue = 0;
								int iCanBuildPrereq = 0;
								for (pLoopCity = kOwner.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kOwner.nextCity(&iLoop))
								{
									if (canConstruct(eBuilding) && getProductionBuilding() != eBuilding)
										iCanBuildPrereq++;
								}
								if (iCanBuildPrereq >= iPrereqBuildings)
								{
									for (pLoopCity = kOwner.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kOwner.nextCity(&iLoop))
									{
										if (canConstruct(eLoopBuilding, false, true) && getProductionBuilding() != eLoopBuilding)
											iHighestValue = std::max(pLoopCity->AI_buildingValue(eLoopBuilding, 0, 0, bConstCache, false), iHighestValue);
									}

									int iTempValue = iHighestValue;
									iTempValue *= iCanBuildPrereq + 3*iPrereqBuildings;
									iTempValue /= (iPrereqBuildings+1)*(3*iCanBuildPrereq + iPrereqBuildings);
									// That's between 1/(iPrereqBuildings+1) and 1/3*(iPrereqBuildings+1), depending on # needed and # buildable
									iValue += iTempValue;
								}
							}
						}
					}
				}
				// K-Mod end (prereqs)
				
				for (int iI = 0; iI < GC.getNumVoteSourceInfos(); ++iI)
				{
					if (kBuilding.getVoteSourceType() == iI)
					{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/24/10                              jdog5000        */
/*                                                                                              */
/* City AI, Victory Strategy AI                                                                 */
/************************************************************************************************/					
						int iTempValue = 0;
						if (kBuilding.isStateReligion())
						{
							int iShareReligionCount = 0;
							int iPlayerCount = 0;
							for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
							{
								CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
								if ((iPlayer != getOwner()) && kLoopPlayer.isAlive())
								{
									iPlayerCount++;
									if (GET_PLAYER(getOwnerINLINE()).getStateReligion() == kLoopPlayer.getStateReligion())
									{
										iShareReligionCount++;										
									}
								}
							}
							iTempValue += (200 * (1 + iShareReligionCount)) / (1 + iPlayerCount);
						}
						else
						{
							iTempValue += 100;
						}

						iValue += (iTempValue * (GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY1) ? 5 : 1));
					}

					// Value religion buildings based on AP gains
					if (GC.getGameINLINE().isDiploVote((VoteSourceTypes)iI))
					{
						if (GET_PLAYER(getOwnerINLINE()).isLoyalMember((VoteSourceTypes)iI))
						{
							ReligionTypes eReligion = GC.getGameINLINE().getVoteSourceReligion((VoteSourceTypes)iI);

							if (NO_RELIGION != eReligion && isHasReligion(eReligion))
							{
								if (kBuilding.getReligionType() == eReligion)
								{
									for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
									{
										int iChange = GC.getVoteSourceInfo((VoteSourceTypes)iI).getReligionYield(iYield);
										int iTempValue = iChange * 4; // was 6
										// K-Mod
										iTempValue *= AI_yieldMultiplier((YieldTypes)iYield);
										iTempValue /= 100;
										if (iI == YIELD_PRODUCTION)
										{
											// priority += 2.8% per 1% in production increase. roughly. More when at war.
											iPriorityFactor += std::min(100, (bWarPlan ? 320 : 280)*iTempValue/std::max(1, 4*getYieldRate(YIELD_PRODUCTION)));
										}
										// K-Mod end
										iTempValue *= kOwner.AI_yieldWeight((YieldTypes)iYield, this);
										iTempValue /= 100;

										iValue += iTempValue;
									}

									for (int iCommerce = 0; iCommerce < NUM_COMMERCE_TYPES; ++iCommerce)
									{
										int iChange = GC.getVoteSourceInfo((VoteSourceTypes)iI).getReligionCommerce(iCommerce);
										int iTempValue = iChange * 4;

										// K-Mod
										iTempValue *= getTotalCommerceRateModifier((CommerceTypes)iCommerce);
										iTempValue /= 100;
										// K-Mod end

										// +99 mirrors code below, I think because commerce weight can be pretty small. (It's so it rounds up, not down. - K-Mod)
										iTempValue *= kOwner.AI_commerceWeight((CommerceTypes)iCommerce, this);
										iTempValue = (iTempValue + 99) / 100;

										iValue += iTempValue;
									}
								}
							}
						}
					}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				}

			}

			if (iPass > 0)
			{

				// K-Mod, I've moved this from inside the yield types loop; and I've increased the value to compensate.
				if (iFoodDifference > 0)
				{
					//iValue += kBuilding.getFoodKept() / 2;
					iValue += std::max(0, 2*(std::max(4, AI_getTargetPopulation()) - getPopulation())+(bCanPopRush ?3 :1)) * kBuilding.getFoodKept() / 4;
				}

				for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
				{
					// K-Mod - I've shuffled some parts of this code around.
					int iTempValue = 0;

					/*iTempValue += ((kBuilding.getYieldModifier(iI) * getBaseYieldRate((YieldTypes)iI)) / 10);
					iTempValue += ((kBuilding.getPowerYieldModifier(iI) * getBaseYieldRate((YieldTypes)iI)) / ((bProvidesPower || isPower()) ? 12 : 15));*/
					// K-Mod
					iTempValue += kBuilding.getYieldModifier(iI) * getBaseYieldRate((YieldTypes)iI) / 20;
					iTempValue += kBuilding.getPowerYieldModifier(iI) * getBaseYieldRate((YieldTypes)iI) / (bProvidesPower || isPower() ? 21 : 32);

					if (bProvidesPower && !isPower())
					{
						iTempValue += ((getPowerYieldRateModifier((YieldTypes)iI) * getBaseYieldRate((YieldTypes)iI)) / 21); // originally 12
					}

					for (int iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
					{
						if (hasBonus((BonusTypes)iJ))
						{
							iTempValue += ((kBuilding.getBonusYieldModifier(iJ, iI) * getBaseYieldRate((YieldTypes)iI)) / 21); // originally 12
						}
					}

					// if this is a limited wonder, and we are not one of the top 4 in this category, subtract the value
					// we do _not_ want to build this here (unless the value was small anyway)
					//if (bIsLimitedWonder && (findBaseYieldRateRank((YieldTypes)iI) > (3 + iLimitedWonderLimit)))
					// K-Mod: lets say top 1/3 instead. There are now other mechanisms to stop us from wasting buildings.
					if (bIsLimitedWonder && findBaseYieldRateRank((YieldTypes)iI) > iNumCities/3 + 1 + iLimitedWonderLimit)
					{
						iTempValue *= -1;
					}

					// (K-Mod)...and now the things that should not depend on whether or not we have a good yield rank
					int iRawYieldValue = 0;
					iRawYieldValue += ((kBuilding.getTradeRouteModifier() * getTradeYield((YieldTypes)iI)) / 21); // originally 12 (and 'iValue')
					//if (bForeignTrade)
					if (bForeignTrade && !kOwner.isNoForeignTrade()) // K-Mod
					{
						iRawYieldValue += ((kBuilding.getForeignTradeRouteModifier() * getTradeYield((YieldTypes)iI)) / 30); // originally 12 (and 'iValue')
					}

					/* original bts code (We're inside a yield types loop. This would be triple counted here!)
					if (iFoodDifference > 0)
					{
						iValue += kBuilding.getFoodKept() / 2;
					} */

					if (kBuilding.getSeaPlotYieldChange(iI) > 0)
					{
					    iRawYieldValue += kBuilding.getSeaPlotYieldChange(iI) * AI_buildingSpecialYieldChangeValue(eBuilding, (YieldTypes)iI);
					}
					if (kBuilding.getRiverPlotYieldChange(iI) > 0)
					{
						iRawYieldValue += (kBuilding.getRiverPlotYieldChange(iI) * countNumRiverPlots() * 3); // was 4
					}
					iRawYieldValue += (kBuilding.getYieldChange(iI) * 4); // was 6

					iRawYieldValue *= AI_yieldMultiplier((YieldTypes)iI);
					iRawYieldValue /= 100;
					iTempValue += iRawYieldValue;

					for (int iJ = 0; iJ < GC.getNumSpecialistInfos(); iJ++)
					{
						iTempValue += ((kBuilding.getSpecialistYieldChange(iJ, iI) * kOwner.getTotalPopulation()) / 5);
					}
					iTempValue += (kBuilding.getGlobalSeaPlotYieldChange(iI) * kOwner.countNumCoastalCities() * 8);
					iTempValue += ((kBuilding.getAreaYieldModifier(iI) * iNumCitiesInArea) / 3);
					iTempValue += ((kBuilding.getGlobalYieldModifier(iI) * iNumCities) / 3);
					
					if (iTempValue != 0)
					{
						/* original bts code (this is rolled into AI_yieldWeight)
						if (bFinancialTrouble && iI == YIELD_COMMERCE)
						{
							iTempValue *= 2;
						}*/

						// K-Mod
						if (iI == YIELD_PRODUCTION)
						{
							// priority += 2.8% per 1% in production increase. roughly. More when at war.
							iPriorityFactor += std::min(100, (bWarPlan ? 320 : 280)*iTempValue/std::max(1, 5*getYieldRate(YIELD_PRODUCTION)));
						}
						// K-Mod end

						iTempValue *= kOwner.AI_yieldWeight((YieldTypes)iI, this);
						iTempValue /= 100;
						
						// (limited wonder condition use to be here. I've moved it. - Karadoc)

						iValue += iTempValue;
					}
				}
			}
			else
			{
				if (iFocusFlags & BUILDINGFOCUS_FOOD)
				{

					iValue += kBuilding.getFoodKept();

					if (kBuilding.getSeaPlotYieldChange(YIELD_FOOD) > 0)
					{
					    int iTempValue = kBuilding.getSeaPlotYieldChange(YIELD_FOOD) * AI_buildingSpecialYieldChangeValue(eBuilding, YIELD_FOOD);
					    if ((iTempValue < 8) && (getPopulation() > 3))
					    {
					        // don't bother
					    }
					    else
					    {
                            iValue += ((iTempValue * 4) / std::max(2, iFoodDifference));
					    }
					}

					if (kBuilding.getRiverPlotYieldChange(YIELD_FOOD) > 0)
					{
						iValue += (kBuilding.getRiverPlotYieldChange(YIELD_FOOD) * countNumRiverPlots() * 4);
					}
				}

				if (iFocusFlags & BUILDINGFOCUS_PRODUCTION)
				{
					int iTempValue = ((kBuilding.getYieldModifier(YIELD_PRODUCTION) * getBaseYieldRate(YIELD_PRODUCTION)) / 20);
					iTempValue += ((kBuilding.getPowerYieldModifier(YIELD_PRODUCTION) * getBaseYieldRate(YIELD_PRODUCTION)) / ((bProvidesPower || isPower()) ? 24 : 30));
					if (kBuilding.getSeaPlotYieldChange(YIELD_PRODUCTION) > 0)
					{
						int iNumWaterPlots = countNumWaterPlots();
						if (!bIsLimitedWonder || (iNumWaterPlots > NUM_CITY_PLOTS / 2))
						{
					    	iTempValue += kBuilding.getSeaPlotYieldChange(YIELD_PRODUCTION) * iNumWaterPlots;
						}
					}
					if (kBuilding.getRiverPlotYieldChange(YIELD_PRODUCTION) > 0)
					{
						iTempValue += (kBuilding.getRiverPlotYieldChange(YIELD_PRODUCTION) * countNumRiverPlots() * 4);
					}
					if (bProvidesPower && !isPower())
					{
						//iTempValue += ((getPowerYieldRateModifier(YIELD_PRODUCTION) * getBaseYieldRate(YIELD_PRODUCTION)) / 12);
						iTempValue += ((getPowerYieldRateModifier(YIELD_PRODUCTION) * getBaseYieldRate(YIELD_PRODUCTION)) / 24); // K-Mod, consistency
					}
					
					// if this is a limited wonder, and we are not one of the top 4 in this category, subtract the value
					// we do _not_ want to build this here (unless the value was small anyway)
					if (bIsLimitedWonder && (findBaseYieldRateRank(YIELD_PRODUCTION) > (3 + iLimitedWonderLimit)))
					{
						iTempValue *= -1;
					}

					iValue += iTempValue;
				}

				if (iFocusFlags & BUILDINGFOCUS_GOLD)
				{
					int iTempValue = ((kBuilding.getYieldModifier(YIELD_COMMERCE) * getBaseYieldRate(YIELD_COMMERCE)));
					iTempValue *= kOwner.getCommercePercent(COMMERCE_GOLD);
					
					if (bFinancialTrouble)
					{
						iTempValue *= 2;
					}
					
					iTempValue /= 3000;
					
					/*if (MAX_INT == aiCommerceRank[COMMERCE_GOLD])
					{
						aiCommerceRank[COMMERCE_GOLD] = findCommerceRateRank(COMMERCE_GOLD);
					}*/

					// if this is a limited wonder, and we are not one of the top 4 in this category, subtract the value
					// we do _not_ want to build this here (unless the value was small anyway)
					if (bIsLimitedWonder && (findCommerceRateRank(COMMERCE_GOLD) > (3 + iLimitedWonderLimit)))
					{
						iTempValue *= -1;
					}

					iValue += iTempValue;
				}
			}

			if (iPass > 0)
			{
				for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
				{
					int iTempValue = 0;

					iTempValue += (kBuilding.getCommerceChange(iI) * 4);
					iTempValue += (kBuilding.getObsoleteSafeCommerceChange(iI) * 4);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/13/10                              jdog5000        */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
					if( kBuilding.getReligionType() != NO_RELIGION && kBuilding.getReligionType() == kOwner.getStateReligion() )
					{
						iTempValue += kOwner.getStateReligionBuildingCommerce((CommerceTypes)iI) * 3;
					}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					iTempValue += ((kBuilding.getSpecialistExtraCommerce(iI) * kOwner.getTotalPopulation()) / 3); // Moved from below (K-Mod)
					//iTempValue *= 100 + kBuilding.getCommerceModifier(iI);
					iTempValue *= getTotalCommerceRateModifier((CommerceTypes)iI) + kBuilding.getCommerceModifier(iI); // K-Mod. Note that getTotalCommerceRateModifier() includes the +100.
					iTempValue /= 100;

					if (kBuilding.getCommerceChangeDoubleTime(iI) > 0)
					{
						if ((kBuilding.getCommerceChange(iI) > 0) || (kBuilding.getObsoleteSafeCommerceChange(iI) > 0))
						{
							//iTempValue += (1000 / kBuilding.getCommerceChangeDoubleTime(iI));
							iTempValue += iTempValue * 250 / kBuilding.getCommerceChangeDoubleTime(iI); // K-Mod (still very rough...)
						}
					}

					if ((CommerceTypes)iI == COMMERCE_CULTURE)
					{
					    if (bCulturalVictory1)
					    {
					        iTempValue *= 2;					        
					    }

						// K-Mod
						if (iTempValue > 0 &&
							getCultureLevel() <= (CultureLevelTypes)1 && getCommerceRateTimes100(COMMERCE_CULTURE) < 100)
						{
							iTempValue += 16;
							iPriorityFactor += 30;
						}
						// K-Mod end
					}

					// K-mod. the getCommerceChangeDoubleTime bit use to be here. I moved it up to be before the culture value boost.

					// add value for a commerce modifier
					int iCommerceModifier = kBuilding.getCommerceModifier(iI);
					int iBaseCommerceRate = getBaseCommerceRate((CommerceTypes) iI);
					// K-Mod. inflate the base commerce rate, to account for the fact that commerce multipliers give us flexibility.
					{
						int x = std::max(5, kOwner.getCommercePercent((CommerceTypes)iI));
						if (iI == COMMERCE_CULTURE)
						{
							x += x <= 45 && bCulturalVictory1 ? 10 : 0;
							x += x <= 45 && bCulturalVictory2 ? 10 : 0;
							x += x <= 45 && bCulturalVictory3 ? 10 : 0;
						}
						iBaseCommerceRate += getYieldRate(YIELD_COMMERCE) * x * (100 - x) / 10000;
					}
					// K-Mod end
					int iCommerceMultiplierValue = iCommerceModifier * iBaseCommerceRate;
					if (iI == COMMERCE_CULTURE && iCommerceModifier != 0)
					{
						// K-Mod: bug fix, and improvement. (the old code was missing /= 100, and it was too conditional)
						// (disabled indefinitely. culture pressure is counted in other ways; so I want to test without this boost.)
						//iCommerceMultiplierValue *= culturePressureFactor() + 100;
						//iCommerceMultiplierValue /= 200;
						// K-Mod end

						// K-Mod. the value of culture is now boosted primarily inside AI_commerceWeight
						// so I've decreased the value boost in the following block.
						if (bCulturalVictory1)
						{
							// if this is one of our top culture cities, then we want to build this here first!
							if (iCultureRank <= iCulturalVictoryNumCultureCities)
							{
								iCommerceMultiplierValue /= 12; // was 8

								// if we at culture level 3, then these need to get built asap
								if (bCulturalVictory3)
								{
									int iLoop;
									int iHighestRate = 0;
									for (CvCity* pLoopCity = kOwner.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kOwner.nextCity(&iLoop))
									{
										iHighestRate = std::max(iHighestRate, pLoopCity->getCommerceRate(COMMERCE_CULTURE));
									}
									FAssert(iHighestRate >= getCommerceRate(COMMERCE_CULTURE));

									// its most important to build in the lowest rate city, but important everywhere
									iCommerceMultiplierValue += (iHighestRate - getCommerceRate(COMMERCE_CULTURE)) * iCommerceModifier / 12; // was 8
								}
							}
							else
							{
								//int iCountBuilt = kOwner.getBuildingClassCountPlusMaking(eBuildingClass);
								int iCountBuilt = kOwner.getBuildingClassCount(eBuildingClass); // K-Mod (to match the number used by getBuildingClassPrereqBuilding)

								// do we have enough buildings to build extras?
								bool bHaveEnough = true;

								// if its limited and the limit is less than the number we need in culture cities, do not build here
								if (bIsLimitedWonder && (iLimitedWonderLimit <= iCulturalVictoryNumCultureCities))
								{
									bHaveEnough = false;
								}

								for (int iJ = 0; bHaveEnough && iJ < GC.getNumBuildingClassInfos(); iJ++)
								{
									// K-Mod: bug fix.
									/* original bts code
									// count excess the number of prereq buildings which do not have this building built for yet
									int iPrereqBuildings = kOwner.getBuildingClassPrereqBuilding(eBuilding, (BuildingClassTypes) iJ, -iCountBuilt);
									
									// do we not have enough built (do not count ones in progress)
									if (iPrereqBuildings > 0 && kOwner.getBuildingClassCount((BuildingClassTypes) iJ) <  iPrereqBuildings)
									{
										bHaveEnough = false;
									}*/

									// Whatever that code was meant to do, I'm pretty sure it was wrong.
									int iPrereqBuildings = kOwner.getBuildingClassPrereqBuilding(eBuilding, (BuildingClassTypes) iJ, iCulturalVictoryNumCultureCities - iCountBuilt);
									if (kOwner.getBuildingClassCount((BuildingClassTypes) iJ) < iPrereqBuildings)
										bHaveEnough = false;
									// K-Mod end
								}

								// if we have enough and our rank is close to the top, then possibly build here too
								if (bHaveEnough && (iCultureRank - iCulturalVictoryNumCultureCities) <= 3)
								{
									iCommerceMultiplierValue /= 16; // was 12
								}
								// otherwise, we really do not want to build this here
								else
								{
									iCommerceMultiplierValue /= 30;
								}
							}
						}
						else
						{
							iCommerceMultiplierValue /= 20; // was 15

							// increase priority if we need culture oppressed city
							// K-Mod: moved this to outside of the current "if".
							// It should still apply even when going for a cultural victory!
							//iCommerceMultiplierValue *= (100 - calculateCulturePercent(getOwnerINLINE()));
						}
					}
					else
					{
						iCommerceMultiplierValue /= 20; // was 15
					}
					iTempValue += iCommerceMultiplierValue;

					// K-Mod. help get the ball rolling on espionage.
					if (iI == COMMERCE_ESPIONAGE && iTempValue > 0 && !GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE))
					{
						// priority += 1% per 1% increase in total espionage, more for big espionage strategy
						iPriorityFactor += std::min(100, (kOwner.AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE)?150 : 100) * iTempValue/std::max(1, 4*kOwner.getCommerceRate(COMMERCE_ESPIONAGE)));
					}

					// ... and increase the priority of research buildings if aiming for a space victory.
					if ((CommerceTypes)iI == COMMERCE_RESEARCH && bSpaceVictory1)
					{
						iPriorityFactor += std::min(25, iTempValue/2);
					}
					// K-Mod end

					iTempValue += ((kBuilding.getGlobalCommerceModifier(iI) * iNumCities) / 4);
					// iTempValue += ((kBuilding.getSpecialistExtraCommerce(iI) * kOwner.getTotalPopulation()) / 3); // moved up (K-Mod)

					if (eStateReligion != NO_RELIGION)
					{
						iTempValue += (kBuilding.getStateReligionCommerce(iI) * kOwner.getHasReligionCount(eStateReligion) * 3);
					}

					if (kBuilding.getGlobalReligionCommerce() != NO_RELIGION)
					{
						/*iTempValue += (GC.getReligionInfo((ReligionTypes)(kBuilding.getGlobalReligionCommerce())).getGlobalReligionCommerce(iI) * GC.getGameINLINE().countReligionLevels((ReligionTypes)(kBuilding.getGlobalReligionCommerce())) * 2);
						if (eStateReligion == (ReligionTypes)(kBuilding.getGlobalReligionCommerce()))
						{
						    iTempValue += 10;
						}*/
						// K-Mod
						int iExpectedSpread = GC.getGameINLINE().countReligionLevels((ReligionTypes)kBuilding.getGlobalReligionCommerce());
						iExpectedSpread += (GC.getNumEraInfos() - kOwner.getCurrentEra() + (eStateReligion == (ReligionTypes)(kBuilding.getGlobalReligionCommerce())? 2 : 0)) * GC.getWorldInfo(GC.getMap().getWorldSize()).getDefaultPlayers();
						iTempValue += GC.getReligionInfo((ReligionTypes)kBuilding.getGlobalReligionCommerce()).getGlobalReligionCommerce(iI) * iExpectedSpread * 4;
					}

					// K-Mod: I've moved the corporation stuff that use to be here to outside this loop so that it isn't quadriple counted

					if (kBuilding.isCommerceFlexible(iI))
					{
						if (!(kOwner.isCommerceFlexible((CommerceTypes)iI)))
						{
							iTempValue += 40;
						}
					}

					if (kBuilding.isCommerceChangeOriginalOwner(iI))
					{
						if ((kBuilding.getCommerceChange(iI) > 0) || (kBuilding.getObsoleteSafeCommerceChange(iI) > 0))
						{
							iTempValue++;
						}
					}
					
					if (iTempValue != 0)
					{
						if (bFinancialTrouble && iI == COMMERCE_GOLD)
						{
							iTempValue *= 2;
						}

						iTempValue *= kOwner.AI_commerceWeight(((CommerceTypes)iI), this);
						iTempValue = (iTempValue + 99) / 100;

						// if this is a limited wonder, and we are not one of the top 4 in this category, subtract the value
						// we do _not_ want to build this here (unless the value was small anyway)
						/*if (MAX_INT == aiCommerceRank[iI])
						{
							aiCommerceRank[iI] = findCommerceRateRank((CommerceTypes) iI);
						}*/
						/* original bts code
						if (bIsLimitedWonder && ((aiCommerceRank[iI] > (3 + iLimitedWonderLimit)))
							|| (bCulturalVictory1 && (iI == COMMERCE_CULTURE) && (aiCommerceRank[iI] == 1)))
						{
							iTempValue *= -1;

							// for culture, just set it to zero, not negative, just about every wonder gives culture
							if (iI == COMMERCE_CULTURE)
							{
								iTempValue = 0;
							}
						} */
						// K-Mod, let us build culture wonders to defend againt culture pressure
						// also, lets say top 1/3 instead. There are now other mechanisms to stop us from wasting buildings.
						if (bIsLimitedWonder)
						{
							if (iI == COMMERCE_CULTURE)
							{
								 if (bCulturalVictory1 &&
									 ((bCulturalVictory2 && findCommerceRateRank((CommerceTypes)iI) == 1) ||
									 findCommerceRateRank((CommerceTypes)iI) > iNumCities/3 + 1 + iLimitedWonderLimit))
								 {
									 iTempValue = 0;
								 }
							}
							
							else if (findCommerceRateRank((CommerceTypes)iI) > iNumCities/3 + 1 + iLimitedWonderLimit)
							{
								iTempValue *= -1;
							}
						}
						// K-Mod end

						iValue += iTempValue;
					}
				}

				// corp evaluation moved here, and rewriten for K-Mod
				{
					CorporationTypes eCorporation = (CorporationTypes)kBuilding.getFoundsCorporation();
					int iCorpValue = 0;
					int iExpectedSpread = kOwner.AI_isDoVictoryStrategyLevel4() ? 45 : 70 - (bWarPlan ? 10 : 0);
					// note: expected spread starts as percent (for precision), but is later converted to # of cities.
					if (kOwner.isNoCorporations())
						iExpectedSpread = 0;
					if (NO_CORPORATION != eCorporation && iExpectedSpread > 0)
					{
						//iCorpValue = kOwner.AI_corporationValue(eCorporation, this);
						// K-Mod: consider the corporation for the whole civ, not just this city.
						iCorpValue = kOwner.AI_corporationValue(eCorporation);

						for (int iCorp = 0; iCorp < GC.getNumCorporationInfos(); iCorp++)
						{
							if (iCorp != eCorporation)
							{
								if (kOwner.hasHeadquarters((CorporationTypes)iCorp))
								{
									if (GC.getGame().isCompetingCorporation(eCorporation, (CorporationTypes)iCorp))
									{
										// This new corp is no good to us if our competing corp is already better.
										// note: evaluation of the competing corp for this particular city is ok.
										if (kOwner.AI_corporationValue((CorporationTypes)iCorp, this) > iCorpValue)
										{
											iExpectedSpread = 0;
											break;
										}
										// expect to spread the corp to fewer cities.
										iExpectedSpread /= 2;
									}
								}
							}
						}
						// convert spread from percent to # of cities
						iExpectedSpread = iExpectedSpread * iNumCities / 100;

						// scale corp value by the expected spread
						iCorpValue *= iExpectedSpread;

						// Rescale from 100x commerce down to 4x commerce. (AI_corporationValue returns roughly 100x commerce)
						iCorpValue *= 4;
						iCorpValue /= 100;
					}

					if (kBuilding.getGlobalCorporationCommerce() != NO_CORPORATION)
					{
						iExpectedSpread += GC.getGameINLINE().countCorporationLevels((CorporationTypes)(kBuilding.getGlobalCorporationCommerce()));

						if (iExpectedSpread > 0)
						{
							for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
							{
								int iHqValue = 4 * GC.getCorporationInfo((CorporationTypes)(kBuilding.getGlobalCorporationCommerce())).getHeadquarterCommerce(iI) * iExpectedSpread;
								if (iHqValue != 0)
								{
									iHqValue *= getTotalCommerceRateModifier((CommerceTypes)iI);
									iHqValue *= kOwner.AI_commerceWeight((CommerceTypes)iI, this);
									iHqValue /= 10000;
								}
								// use rank as a tie-breaker... with number of national wonders thrown in to,
								// (I'm trying to boost the chance that the AI will put wallstreet with its corp HQs.)
								if (iHqValue > 0)
								{
									iHqValue *= 3*iNumCities - findCommerceRateRank((CommerceTypes)iI) - getNumNationalWonders()/2;
									iHqValue /= 2*iNumCities;
								}
								iCorpValue += iHqValue;
							}
						}
					}

					if (iCorpValue > 0)
					{
						iValue += iCorpValue;
					}
				}
				// K-Mod end (corp)

				for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
				{
					if (kBuilding.getReligionChange(iI) > 0)
					{
						if (GET_TEAM(getTeam()).hasHolyCity((ReligionTypes)iI))
						{
							iValue += (kBuilding.getReligionChange(iI) * ((eStateReligion == iI) ? 10 : 1));
						}
					}
				}

				if (NO_VOTESOURCE != kBuilding.getVoteSourceType())
				{
					iValue += 100;
				}
			}
			else
			{
				if (iFocusFlags & BUILDINGFOCUS_GOLD)
				{
					int iTempValue = ((kBuilding.getCommerceModifier(COMMERCE_GOLD) * getBaseCommerceRate(COMMERCE_GOLD)) / 40);
					
					if (iTempValue != 0)
					{
						if (bFinancialTrouble)
						{
							iTempValue *= 2;
						}
						
						/*if (MAX_INT == aiCommerceRank[COMMERCE_GOLD])
						{
							aiCommerceRank[COMMERCE_GOLD] = findCommerceRateRank(COMMERCE_GOLD);
						}*/

						// if this is a limited wonder, and we are not one of the top 4 in this category, subtract the value
						// we do _not_ want to build this here (unless the value was small anyway)
						if (bIsLimitedWonder && (findCommerceRateRank(COMMERCE_GOLD) > (3 + iLimitedWonderLimit)))
						{
							iTempValue *= -1;
						}
						iValue += iTempValue;
					}
					
					iValue += (kBuilding.getCommerceChange(COMMERCE_GOLD) * 4);
					iValue += (kBuilding.getObsoleteSafeCommerceChange(COMMERCE_GOLD) * 4);
				}

				if (iFocusFlags & BUILDINGFOCUS_RESEARCH)
				{
					int iTempValue = ((kBuilding.getCommerceModifier(COMMERCE_RESEARCH) * getBaseCommerceRate(COMMERCE_RESEARCH)) / 40);
					
					if (iTempValue != 0)
					{
						/*if (MAX_INT == aiCommerceRank[COMMERCE_RESEARCH])
						{
							aiCommerceRank[COMMERCE_RESEARCH] = findCommerceRateRank(COMMERCE_RESEARCH);
						}*/

						// if this is a limited wonder, and we are not one of the top 4 in this category, subtract the value
						// we do _not_ want to build this here (unless the value was small anyway)
						if (bIsLimitedWonder && (findCommerceRateRank(COMMERCE_RESEARCH) > (3 + iLimitedWonderLimit)))
						{
							iTempValue *= -1;
						}

						iValue += iTempValue;
					}
					iValue += (kBuilding.getCommerceChange(COMMERCE_RESEARCH) * 4);
					iValue += (kBuilding.getObsoleteSafeCommerceChange(COMMERCE_RESEARCH) * 4);
				}

				if (iFocusFlags & BUILDINGFOCUS_CULTURE)
				{
					int iTempValue = (kBuilding.getCommerceChange(COMMERCE_CULTURE) * 3);
					iTempValue += (kBuilding.getObsoleteSafeCommerceChange(COMMERCE_CULTURE) * 3);
					if (GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE))
					{
						iTempValue += (kBuilding.getCommerceChange(COMMERCE_ESPIONAGE) * 3);
						iTempValue += (kBuilding.getObsoleteSafeCommerceChange(COMMERCE_ESPIONAGE) * 3);
					}

					if ((getCommerceRate(COMMERCE_CULTURE) == 0) && (AI_calculateTargetCulturePerTurn() == 1))
					{
						if (iTempValue >= 3)
						{
							iTempValue += 7;
						}
					}

					// K-Mod, this stuff was moved from below
					iTempValue += ((kBuilding.getCommerceModifier(COMMERCE_CULTURE) * getBaseCommerceRate(COMMERCE_CULTURE)) / 15);
					if (GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE))
					{
						iTempValue += ((kBuilding.getCommerceModifier(COMMERCE_ESPIONAGE) * getBaseCommerceRate(COMMERCE_ESPIONAGE)) / 15);
					}
					// K-Mod end

					if (iTempValue != 0)
					{
						/*if (MAX_INT == aiCommerceRank[COMMERCE_CULTURE])
						{
							aiCommerceRank[COMMERCE_CULTURE] = findCommerceRateRank(COMMERCE_CULTURE);
						}*/

						// if this is a limited wonder, and we are not one of the top 4 in this category, 
						// do not count the culture value
						// we probably do not want to build this here (but we might)
						/* original bts code
						if (bIsLimitedWonder && (findCommerceRateRank(COMMERCE_CULTURE) > (3 + iLimitedWonderLimit)))
						{
							iTempValue  = 0;
						}*/
						// K-Mod. The original code doesn't take prereq buildings into account, and it was in the wrong place.
						// To be honest, I think this "building focus" flag system is pretty bad; but I'm fixing it anyway.
						if (findCommerceRateRank(COMMERCE_CULTURE) > iCulturalVictoryNumCultureCities)
						{
							bool bAvoid = false;
							if (bIsLimitedWonder && findCommerceRateRank(COMMERCE_CULTURE) - iLimitedWonderLimit >= iCulturalVictoryNumCultureCities)
								bAvoid = true;
							for (int iJ = 0; !bAvoid && iJ < GC.getNumBuildingClassInfos(); iJ++)
							{
								int iPrereqBuildings = kOwner.getBuildingClassPrereqBuilding(eBuilding, (BuildingClassTypes) iJ, iCulturalVictoryNumCultureCities - kOwner.getBuildingClassCount(eBuildingClass));
								if (kOwner.getBuildingClassCount((BuildingClassTypes) iJ) < iPrereqBuildings)
									bAvoid = true;
							}
							if (bAvoid)
								iTempValue = 0;
						}
						// K-Mod end
						iValue += iTempValue;
					}

					/* original bts code. (I've moved this stuff up to be before the limited wonder checks.
					iValue += ((kBuilding.getCommerceModifier(COMMERCE_CULTURE) * getBaseCommerceRate(COMMERCE_CULTURE)) / 15);
					if (GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE))
					{
						iValue += ((kBuilding.getCommerceModifier(COMMERCE_ESPIONAGE) * getBaseCommerceRate(COMMERCE_ESPIONAGE)) / 15);
					}*/
				}
				
                if (iFocusFlags & BUILDINGFOCUS_BIGCULTURE)
				{
					int iTempValue = (kBuilding.getCommerceModifier(COMMERCE_CULTURE) / 5);
					if (iTempValue != 0)
					{
						/*if (MAX_INT == aiCommerceRank[COMMERCE_CULTURE])
						{
							aiCommerceRank[COMMERCE_CULTURE] = findCommerceRateRank(COMMERCE_CULTURE);
						}*/

						// if this is a limited wonder, and we are not one of the top 4 in this category, 
						// do not count the culture value
						// we probably do not want to build this here (but we might)
						if (bIsLimitedWonder && (findCommerceRateRank(COMMERCE_CULTURE) > (3 + iLimitedWonderLimit)))
						{
							iTempValue  = 0;
						}

						iValue += iTempValue;
					}
				}
				
				//if (iFocusFlags & BUILDINGFOCUS_ESPIONAGE || (GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE) && (iFocusFlags & BUILDINGFOCUS_CULTURE)))
				// K-Mod: the "no espionage" stuff is already taken into account in the culture section.
				if (iFocusFlags & BUILDINGFOCUS_ESPIONAGE && !GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE))
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/09/10                                jdog5000      */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
// K-Mod, changed this section.
					int iTempValue = ((kBuilding.getCommerceModifier(COMMERCE_ESPIONAGE) * getBaseCommerceRate(COMMERCE_ESPIONAGE)) / 50);
					
					if (iTempValue != 0)
					{
						// if this is a limited wonder, and we are not one of the top 4 in this category, subtract the value
						// we do _not_ want to build this here (unless the value was small anyway)
						if (bIsLimitedWonder && (findCommerceRateRank(COMMERCE_ESPIONAGE) > (3 + iLimitedWonderLimit)))
						{
							iTempValue *= -1;
						}

						iValue += iTempValue;
					}
					iTempValue = (kBuilding.getCommerceChange(COMMERCE_ESPIONAGE) * 4);
					iTempValue += (kBuilding.getObsoleteSafeCommerceChange(COMMERCE_ESPIONAGE) * 4);
					iTempValue *= 100 + getTotalCommerceRateModifier(COMMERCE_ESPIONAGE) + kBuilding.getCommerceModifier(COMMERCE_ESPIONAGE);
					iValue += iTempValue / 100;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				}
			}
			
			if ((iThreshold > 0) && (iPass == 0))
			{
				if (iValue < iThreshold)
				{
					iValue = 0;
				}
			}

			if (iPass > 0 && !isHuman())
			{
				iValue += kBuilding.getAIWeight();
				if (iValue > 0)
				{
					for (int iI = 0; iI < GC.getNumFlavorTypes(); iI++)
					{
						iValue += (kOwner.AI_getFlavorValue((FlavorTypes)iI) * kBuilding.getFlavorValue(iI));
					}
				}
			}
		}
	}

//	
//	// obsolete checks
//	bool bCanResearchObsoleteTech = false;
//	int iErasUntilObsolete = MAX_INT;
//	if (kBuilding.getObsoleteTech() != NO_TECH)
//	{
//		TechTypes eObsoleteTech = (TechTypes) kBuilding.getObsoleteTech();
//		FAssertMsg(eObsoleteTech == NO_TECH || !(GET_TEAM(getTeam()).isHasTech(eObsoleteTech)), "Team expected to not have the tech that obsoletes eBuilding");
//		iErasUntilObsolete = GC.getTechInfo(eObsoleteTech).getEra() - kOwner.getCurrentEra();
//		bCanResearchObsoleteTech = kOwner.canResearch(eObsoleteTech);
//	}
//
//	if (kBuilding.getSpecialBuildingType() != NO_SPECIALBUILDING)
//	{
//		TechTypes eSpecialBldgObsoleteTech = (TechTypes) GC.getSpecialBuildingInfo((SpecialBuildingTypes)(kBuilding.getSpecialBuildingType())).getObsoleteTech();
//		FAssertMsg(eSpecialBldgObsoleteTech == NO_TECH || !(GET_TEAM(getTeam()).isHasTech(eSpecialBldgObsoleteTech)), "Team expected to not have the tech that obsoletes eBuilding");
//		if (eSpecialBldgObsoleteTech != NO_TECH)
//		{
//			int iSpecialBldgErasUntilObsolete = GC.getTechInfo(eSpecialBldgObsoleteTech).getEra() - kOwner.getCurrentEra();
//
//			if (iSpecialBldgErasUntilObsolete < iErasUntilObsolete)
//			{
//				iErasUntilObsolete = iSpecialBldgErasUntilObsolete;
//			}
//
//			if (!bCanResearchObsoleteTech)
//			{
//				bCanResearchObsoleteTech = kOwner.canResearch(eSpecialBldgObsoleteTech);
//			}
//		}
//	}
//	
//	// tech path method commented out, it would be more accurate if we want to be so.
//	//int iObsoleteTechPathLength = 0;
//	//if (kBuilding.getObsoleteTech() != NO_TECH)
//	//{
//	//	iObsoleteTechPathLength = findPathLength(kBuilding.getObsoleteTech(), false);
//	//}
//
//	//if (kBuilding.getSpecialBuildingType() != NO_SPECIALBUILDING)
//	//{
//	//	TechTypes eSpecialBldgObsoleteTech = GC.getSpecialBuildingInfo((SpecialBuildingTypes)(kBuilding.getSpecialBuildingType())).getObsoleteTech();
//	//	int iSpecialBldgObsoleteTechPathLength = findPathLength(eSpecialBldgObsoleteTech, false);
//
//	//	if (iSpecialBldgObsoleteTechPathLength < iObsoleteTechPathLength)
//	//	{
//	//		iObsoleteTechPathLength = iSpecialBldgObsoleteTechPathLength;
//	//	}
//	//}
//	
//	// if we can research obsolete tech, then this almost no value
//	if (bCanResearchObsoleteTech)
//	{
//		iValue = std::min(16, (iValue + 31) / 32);
//	}
//	// if we are going obsolete in the next era, the current era, or even previous eras...
//	else if (iErasUntilObsolete < 2)
//	{
//		// do not care about obsolete if we are going cultural and there is a culture benefit post obsolesence
//		if (!bCulturalVictory1 || (kBuilding.getObsoleteSafeCommerceChange(COMMERCE_CULTURE) <= 0))
//		{
//			iValue++;
//			iValue /= 2;
//		}
//	}

//	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
//	{
//		if (!GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getBuildingInfo((BuildingTypes)iI).getObsoleteTech())))
//		{
//			if (GC.getBuildingInfo((BuildingTypes)iI).isBuildingClassNeededInCity(kBuilding.getBuildingClassType()))
//			{
//				iValue += AI_buildingValue((BuildingTypes)iI, iFocusFlags);
//			}
//
//			int iNumPrereqs = kOwner.getBuildingClassPrereqBuilding((BuildingTypes)iI, (BuildingClassTypes)kBuilding.getBuildingClassType()) - kOwner.getBuildingClassCount((BuildingClassTypes)kBuilding.getBuildingClassType());
//			if (iNumPrereqs > 0)
//			{
//				iValue += AI_buildingValue((BuildingTypes)iI, iFocusFlags) / iNumPrereqs;
//			}
//		}
//	}

	/* original bts code
	if (!canConstruct(eBuilding))
	{
		//This building is being constructed in some special way, 
		//reduce the value for small cities.
		if (getPopulation() < 6)
		{
			iValue /= (8 - getPopulation());
		}
		
	} */ // disabled by K-Mod. I don't understand the point of this, and it messes up the building value for things that are in-progress or already built.

	iValue = std::max(0, iValue);

	// K-Mod
	// priority factor
	iValue *= iPriorityFactor;
	iValue /= 100;

	// constructionValue cache
	if (!bConstCache && iFocusFlags == 0 && iThreshold == 0)
	{
		// please don't hate me for using const_cast.
		// The const-correctness was my idea in the first place, but the original code doesn't make it easy for me.
		const_cast<std::vector<int>&>(m_aiConstructionValue)[eBuildingClass] = iValue;
	}
	// K-Mod end

	return iValue;
}


ProjectTypes CvCityAI::AI_bestProject()
{
	ProjectTypes eBestProject;
	int iProductionRank;
	int iTurnsLeft;
	int iValue;
	int iBestValue;
	int iI;

	iProductionRank = findYieldRateRank(YIELD_PRODUCTION);

	iBestValue = 0;
	eBestProject = NO_PROJECT;

	for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
	{
		if (canCreate((ProjectTypes)iI))
		{
			iValue = AI_projectValue((ProjectTypes)iI);

			if ((GC.getProjectInfo((ProjectTypes)iI).getEveryoneSpecialUnit() != NO_SPECIALUNIT) ||
				  (GC.getProjectInfo((ProjectTypes)iI).getEveryoneSpecialBuilding() != NO_SPECIALBUILDING) ||
				  GC.getProjectInfo((ProjectTypes)iI).isAllowsNukes())
			{
				if (GC.getGameINLINE().getSorenRandNum(100, "Project Everyone") == 0)
				{
					iValue++;
				}
			}

			if (iValue > 0)
			{
				iValue += getProjectProduction((ProjectTypes)iI);

				iTurnsLeft = getProductionTurnsLeft(((ProjectTypes)iI), 0);

				if ((iTurnsLeft <= GC.getGameINLINE().AI_turnsPercent(10, GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getCreatePercent())) || !(GET_TEAM(getTeam()).isHuman()))
				{
					if ((iTurnsLeft <= GC.getGameINLINE().AI_turnsPercent(20, GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getCreatePercent())) || (iProductionRank <= std::max(3, (GET_PLAYER(getOwnerINLINE()).getNumCities() / 2))))
					{
						if (iProductionRank == 1)
						{
							iValue += iTurnsLeft;
						}
						else
						{
							FAssert((MAX_INT / 1000) > iValue);
							iValue *= 1000;
							iValue /= std::max(1, (iTurnsLeft + 10));
						}

						iValue = std::max(1, iValue);

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							eBestProject = ((ProjectTypes)iI);
						}
					}
				}
			}
		}
	}

	return eBestProject;
}


int CvCityAI::AI_projectValue(ProjectTypes eProject)
{
	int iValue;
	int iI;

	iValue = 0;

	if (GC.getProjectInfo(eProject).getNukeInterception() > 0)
	{
		if (GC.getGameINLINE().canTrainNukes())
		{
			iValue += (GC.getProjectInfo(eProject).getNukeInterception() / 10);
		}
	}

	if (GC.getProjectInfo(eProject).getTechShare() > 0)
	{
		if (GC.getProjectInfo(eProject).getTechShare() < GET_TEAM(getTeam()).getHasMetCivCount(true))
		{
			iValue += (20 / GC.getProjectInfo(eProject).getTechShare());
		}
	}

	for (iI = 0; iI < GC.getNumVictoryInfos(); iI++)
	{
		if (GC.getGameINLINE().isVictoryValid((VictoryTypes)iI))
		{
			iValue += (std::max(0, (GC.getProjectInfo(eProject).getVictoryThreshold(iI) - GET_TEAM(getTeam()).getProjectCount(eProject))) * 20);
		}
	}

	for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
	{
		iValue += (std::max(0, (GC.getProjectInfo((ProjectTypes)iI).getProjectsNeeded(eProject) - GET_TEAM(getTeam()).getProjectCount(eProject))) * 10);
	}

	return iValue;
}


ProcessTypes CvCityAI::AI_bestProcess()
{
	return AI_bestProcess(NO_COMMERCE);
}

ProcessTypes CvCityAI::AI_bestProcess(CommerceTypes eCommerceType)
{
	ProcessTypes eBestProcess;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	eBestProcess = NO_PROCESS;

	for (iI = 0; iI < GC.getNumProcessInfos(); iI++)
	{
		if (canMaintain((ProcessTypes)iI))
		{
			iValue = AI_processValue((ProcessTypes)iI, eCommerceType);

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestProcess = ((ProcessTypes)iI);
			}
		}
	}

	return eBestProcess;
}


int CvCityAI::AI_processValue(ProcessTypes eProcess)
{
	return AI_processValue(eProcess, NO_COMMERCE);
}

int CvCityAI::AI_processValue(ProcessTypes eProcess, CommerceTypes eCommerceType)
{
	int iValue;
	int iTempValue;
	int iI;
	bool bValid = (eCommerceType == NO_COMMERCE);
	
	iValue = 0;

	if (GET_PLAYER(getOwnerINLINE()).AI_isFinancialTrouble())
	{
		iValue += GC.getProcessInfo(eProcess).getProductionToCommerceModifier(COMMERCE_GOLD);
	}

	// if we own less than 50%, or we need to pop borders
	if ((plot()->calculateCulturePercent(getOwnerINLINE()) < 50) || (getCultureLevel() <= (CultureLevelTypes) 1))
	{
		iValue += GC.getProcessInfo(eProcess).getProductionToCommerceModifier(COMMERCE_CULTURE);
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/30/09                                jdog5000      */
/*                                                                                              */
/* Cultural Victory AI                                                                          */
/************************************************************************************************/
	if ( GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) )
	{
		// Final city for cultural victory will build culture to speed up victory
		if( findCommerceRateRank(COMMERCE_CULTURE) == GC.getGameINLINE().culturalVictoryNumCultureCities() )
		{
			iValue += 2*GC.getProcessInfo(eProcess).getProductionToCommerceModifier(COMMERCE_CULTURE);
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		iTempValue = GC.getProcessInfo(eProcess).getProductionToCommerceModifier(iI);
		if (!bValid && ((CommerceTypes)iI == eCommerceType) && (iTempValue > 0))
		{
			bValid = true;
		    iTempValue *= 2;
		}

		iTempValue *= GET_PLAYER(getOwnerINLINE()).AI_commerceWeight(((CommerceTypes)iI), this);
		
		iTempValue /= 100;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      07/08/09                                jdog5000      */
/*                                                                                              */
/* Gold AI                                                                                      */
/************************************************************************************************/
		iTempValue *= GET_PLAYER(getOwnerINLINE()).AI_averageCommerceExchange((CommerceTypes)iI);

		iTempValue /= 60;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

		iValue += iTempValue;
	}

	return (bValid ? iValue : 0);
}


int CvCityAI::AI_neededSeaWorkers()
{
	CvArea* pWaterArea;
	int iNeededSeaWorkers = 0;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/01/09                                jdog5000      */
/*                                                                                              */
/* Worker AI                                                                                    */
/************************************************************************************************/
	pWaterArea = waterArea(true);
	
	if (pWaterArea == NULL)
	{
		return 0;
	}

	iNeededSeaWorkers += GET_PLAYER(getOwnerINLINE()).countUnimprovedBonuses(pWaterArea, plot());

	// Check if second water area city can reach was any unimproved bonuses
	pWaterArea = secondWaterArea();
	if (pWaterArea != NULL)
	{
		iNeededSeaWorkers += GET_PLAYER(getOwnerINLINE()).countUnimprovedBonuses(pWaterArea, plot());
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return iNeededSeaWorkers;
}


bool CvCityAI::AI_isDefended(int iExtra)
{
	PROFILE_FUNC();

	return ((plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isCityAIType) + iExtra) >= AI_neededDefenders()); // XXX check for other team's units?
}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						10/17/08		jdog5000		*/
/* 																			*/
/* 	Air AI																	*/
/********************************************************************************/
/* original BTS code
bool CvCityAI::AI_isAirDefended(int iExtra)
{
	PROFILE_FUNC();
	
	return ((plot()->plotCount(PUF_canAirDefend, -1, -1, getOwnerINLINE(), NO_TEAM, PUF_isDomainType, DOMAIN_AIR) + iExtra) >= AI_neededAirDefenders()); // XXX check for other team's units?
}
*/
// Function now answers question of whether city has enough ready air defense, no longer just counts fighters
bool CvCityAI::AI_isAirDefended(bool bCountLand, int iExtra)
{
	PROFILE_FUNC();
	
	int iAirDefenders = iExtra;
	int iAirIntercept = 0;
	int iLandIntercept = 0;

	CvUnit* pLoopUnit;
	CLLNode<IDInfo>* pUnitNode = plot()->headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = plot()->nextUnitNode(pUnitNode);

		if ((pLoopUnit->getOwnerINLINE() == getOwnerINLINE()))
		{
			if ( pLoopUnit->canAirDefend() )
			{
				if( pLoopUnit->getDomainType() == DOMAIN_AIR )
				{
					// can find units which are already air patrolling using group activity
					if( pLoopUnit->getGroup()->getActivityType() == ACTIVITY_INTERCEPT )
					{
						iAirIntercept += pLoopUnit->currInterceptionProbability();
					}
					else
					{
						// Count air units which can air patrol
						if( pLoopUnit->getDamage() == 0 && !pLoopUnit->hasMoved() )
						{
							if( pLoopUnit->AI_getUnitAIType() == UNITAI_DEFENSE_AIR )
							{
								iAirIntercept += pLoopUnit->currInterceptionProbability();
							}
							else
							{
								iAirIntercept += pLoopUnit->currInterceptionProbability()/3;
							}
						}

					}
				}
				else if( pLoopUnit->getDomainType() == DOMAIN_LAND )
				{
					iLandIntercept += pLoopUnit->currInterceptionProbability();
				}
			}
		}
	}

	iAirDefenders += (iAirIntercept/100);

	if( bCountLand )
	{
		iAirDefenders += (iLandIntercept/100);
	}

	int iNeededAirDefenders = AI_neededAirDefenders();
	bool bHaveEnough = (iAirDefenders >= iNeededAirDefenders);

	return bHaveEnough;
}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/


/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/25/10                                jdog5000      */
/*                                                                                              */
/* War strategy AI, Barbarian AI                                                                */
/************************************************************************************************/
int CvCityAI::AI_neededDefenders()
{
	PROFILE_FUNC();
	int iDefenders;
	bool bOffenseWar = ((area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) || (area()->getAreaAIType(getTeam()) == AREAAI_MASSING));
	bool bDefenseWar = ((area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE));
	
	if (!(GET_TEAM(getTeam()).AI_isWarPossible()))
	{
		return 1;
	}

	if (isBarbarian())
	{
		iDefenders = GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getBarbarianInitialDefenders();
		iDefenders += ((getPopulation() + 2) / 7);
		return iDefenders;
	}

	iDefenders = 1;
	
	if (hasActiveWorldWonder() || isCapital() || isHolyCity())
	{
		iDefenders++;

		if( GET_PLAYER(getOwner()).AI_isDoStrategy(AI_STRATEGY_ALERT1) || GET_PLAYER(getOwner()).AI_isDoStrategy(AI_STRATEGY_TURTLE) )
		{
			iDefenders++;
		}
	}
	
	if (!GET_PLAYER(getOwner()).AI_isDoStrategy(AI_STRATEGY_CRUSH))
	{
		iDefenders += AI_neededFloatingDefenders();
	}
	else
	{
		iDefenders += (AI_neededFloatingDefenders() + 2) / 4;
	}
	
	if (bDefenseWar || GET_PLAYER(getOwner()).AI_isDoStrategy(AI_STRATEGY_ALERT2))
	{
		if (!(plot()->isHills()))
		{
			iDefenders++;
		}
	}
	
	if ((GC.getGame().getGameTurn() - getGameTurnAcquired()) < 10)
	{
/* original code
		if (bOffenseWar)
		{
			if (!hasActiveWorldWonder() && !isHolyCity())
			{
				iDefenders /= 2;
				iDefenders = std::max(1, iDefenders);
			}
		}		
	}
	
	if (GC.getGame().getGameTurn() - getGameTurnAcquired() < 10)
	{
		iDefenders = std::max(2, iDefenders);
		if (AI_isDanger())
		{
			iDefenders ++;
		}
		if (bDefenseWar)
		{
			iDefenders ++;
		}
*/
		iDefenders = std::max(2, iDefenders);

		if (bOffenseWar && getTotalDefense(true) > 0)
		{
			if (!hasActiveWorldWonder() && !isHolyCity())
			{
				iDefenders /= 2;
			}
		}		
		
		if (AI_isDanger())
		{
			iDefenders++;
		}
		if (bDefenseWar)
		{
			iDefenders++;
		}
	}
	
	if (GET_PLAYER(getOwnerINLINE()).AI_isDoStrategy(AI_STRATEGY_LAST_STAND))
	{
		iDefenders += 10;
	}

	if( GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) )
	{
		if( findCommerceRateRank(COMMERCE_CULTURE) <= GC.getGameINLINE().culturalVictoryNumCultureCities() )
		{
			iDefenders += 4;

			if( bDefenseWar )
			{
				iDefenders += 2;
			}
		}
	}

	if( GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_SPACE3) )
	{
		if( isCapital() || isProductionProject())
		{
			iDefenders += 4;

			if( bDefenseWar )
			{
				iDefenders += 3;
			}
		}

		if( isCapital() && GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_SPACE4) )
		{
			iDefenders += 6;
		}
	}
	
	iDefenders = std::max(iDefenders, AI_minDefenders());

	return iDefenders;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

int CvCityAI::AI_minDefenders()
{
	int iDefenders = 1;
	int iEra = GET_PLAYER(getOwnerINLINE()).getCurrentEra();
	if (iEra > 0)
	{
		iDefenders++;
	}
	if (((iEra - GC.getGame().getStartEra() / 2) >= GC.getNumEraInfos() / 2) && isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
	{
		iDefenders++;
	}
	
	return iDefenders;
}
	
int CvCityAI::AI_neededFloatingDefenders()
{
	if (m_iNeededFloatingDefendersCacheTurn != GC.getGame().getGameTurn())
	{
		AI_updateNeededFloatingDefenders();
	}
	return m_iNeededFloatingDefenders;	
}

void CvCityAI::AI_updateNeededFloatingDefenders()
{
	int iFloatingDefenders = GET_PLAYER(getOwnerINLINE()).AI_getTotalFloatingDefendersNeeded(area());
		
	int iTotalThreat = std::max(1, GET_PLAYER(getOwnerINLINE()).AI_getTotalAreaCityThreat(area()));
	
	iFloatingDefenders -= area()->getCitiesPerPlayer(getOwnerINLINE());
	
	iFloatingDefenders *= AI_cityThreat();
	iFloatingDefenders += (iTotalThreat / 2);
	iFloatingDefenders /= iTotalThreat;
	
	m_iNeededFloatingDefenders = iFloatingDefenders;
	m_iNeededFloatingDefendersCacheTurn = GC.getGame().getGameTurn();
}

int CvCityAI::AI_neededAirDefenders()
{
	int iDefenders;

	if (!(GET_TEAM(getTeam()).AI_isWarPossible()))
	{
		return 0;
	}

	iDefenders = 0;

	int iRange = 5;

	int iOtherTeam = 0;
	int iEnemyTeam = 0;
	for (int iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (int iDY = -(iRange); iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

			if ((pLoopPlot != NULL) && pLoopPlot->isOwned() && (pLoopPlot->getTeam() != getTeam()))
			{
				iOtherTeam++;
				if (GET_TEAM(getTeam()).AI_getWarPlan(pLoopPlot->getTeam()) != NO_WARPLAN)
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/01/09                                jdog5000      */
/*                                                                                              */
/* Air AI                                                                                       */
/************************************************************************************************/
					// If enemy has no bombers, don't need to defend as much
					if( GET_PLAYER(pLoopPlot->getOwner()).AI_totalUnitAIs(UNITAI_ATTACK_AIR) == 0 )
					{
						continue;
					}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					iEnemyTeam += 2;
					if (pLoopPlot->isCity())
					{
						iEnemyTeam += 6;
					}
				}
			}
		}
	}
	
	iDefenders += (iOtherTeam + iEnemyTeam + 2) / 8;

	iDefenders = std::min((iEnemyTeam > 0) ? 4 : 2, iDefenders);
	
	if (iDefenders == 0)
	{
		if (isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
		{
			iDefenders++;
		}
	}
	return iDefenders;
}


bool CvCityAI::AI_isDanger()
{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* City AI, Efficiency                                                                          */
/************************************************************************************************/
	//return GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2, false);
	return GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2, false);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/	
}


int CvCityAI::AI_getEmphasizeAvoidGrowthCount() const
{
	return m_iEmphasizeAvoidGrowthCount;
}


bool CvCityAI::AI_isEmphasizeAvoidGrowth() const
{
	return (AI_getEmphasizeAvoidGrowthCount() > 0);
}


int CvCityAI::AI_getEmphasizeGreatPeopleCount() const
{
	return m_iEmphasizeGreatPeopleCount;
}


bool CvCityAI::AI_isEmphasizeGreatPeople() const
{
	return (AI_getEmphasizeGreatPeopleCount() > 0);
}


bool CvCityAI::AI_isAssignWorkDirty() const
{
	return m_bAssignWorkDirty;
}


void CvCityAI::AI_setAssignWorkDirty(bool bNewValue)
{
	m_bAssignWorkDirty = bNewValue;
}


bool CvCityAI::AI_isChooseProductionDirty() const
{
	return m_bChooseProductionDirty;
}


void CvCityAI::AI_setChooseProductionDirty(bool bNewValue)
{
	m_bChooseProductionDirty = bNewValue;
}


CvCity* CvCityAI::AI_getRouteToCity() const
{
	return getCity(m_routeToCity);
}


void CvCityAI::AI_updateRouteToCity()
{
	CvCity* pLoopCity;
	CvCity* pBestCity;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;

	gDLL->getFAStarIFace()->ForceReset(&GC.getRouteFinder());

	iBestValue = MAX_INT;
	pBestCity = NULL;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam())
		{
			for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				if (pLoopCity != this)
				{
					if (pLoopCity->area() == area())
					{
						if (!(gDLL->getFAStarIFace()->GeneratePath(&GC.getRouteFinder(), getX_INLINE(), getY_INLINE(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), false, getOwnerINLINE(), true)))
						{
							iValue = plotDistance(getX_INLINE(), getY_INLINE(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE());

							if (iValue < iBestValue)
							{
								iBestValue = iValue;
								pBestCity = pLoopCity;
							}
						}
					}
				}
			}
		}
	}

	if (pBestCity != NULL)
	{
		m_routeToCity = pBestCity->getIDInfo();
	}
	else
	{
		m_routeToCity.reset();
	}
}


int CvCityAI::AI_getEmphasizeYieldCount(YieldTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiEmphasizeYieldCount[eIndex];
}


bool CvCityAI::AI_isEmphasizeYield(YieldTypes eIndex) const
{
	return (AI_getEmphasizeYieldCount(eIndex) > 0);
}


int CvCityAI::AI_getEmphasizeCommerceCount(CommerceTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return (m_aiEmphasizeCommerceCount[eIndex] > 0);
}


bool CvCityAI::AI_isEmphasizeCommerce(CommerceTypes eIndex) const
{
	return (AI_getEmphasizeCommerceCount(eIndex) > 0);
}


bool CvCityAI::AI_isEmphasize(EmphasizeTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumEmphasizeInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(m_pbEmphasize != NULL, "m_pbEmphasize is not expected to be equal with NULL");
	return m_pbEmphasize[eIndex];
}


void CvCityAI::AI_setEmphasize(EmphasizeTypes eIndex, bool bNewValue)
{
	int iI;

	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumEmphasizeInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (AI_isEmphasize(eIndex) != bNewValue)
	{
		m_pbEmphasize[eIndex] = bNewValue;

		if (GC.getEmphasizeInfo(eIndex).isAvoidGrowth())
		{
			m_iEmphasizeAvoidGrowthCount += ((AI_isEmphasize(eIndex)) ? 1 : -1);
			FAssert(AI_getEmphasizeAvoidGrowthCount() >= 0);
		}

		if (GC.getEmphasizeInfo(eIndex).isGreatPeople())
		{
			m_iEmphasizeGreatPeopleCount += ((AI_isEmphasize(eIndex)) ? 1 : -1);
			FAssert(AI_getEmphasizeGreatPeopleCount() >= 0);
		}

		for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			if (GC.getEmphasizeInfo(eIndex).getYieldChange(iI))
			{
				m_aiEmphasizeYieldCount[iI] += ((AI_isEmphasize(eIndex)) ? 1 : -1);
				FAssert(AI_getEmphasizeYieldCount((YieldTypes)iI) >= 0);
			}
		}

		for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			if (GC.getEmphasizeInfo(eIndex).getCommerceChange(iI))
			{
				m_aiEmphasizeCommerceCount[iI] += ((AI_isEmphasize(eIndex)) ? 1 : -1);
				FAssert(AI_getEmphasizeCommerceCount((CommerceTypes)iI) >= 0);
			}
		}

		AI_assignWorkingPlots();

		if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
		}
	}
}

void CvCityAI::AI_forceEmphasizeCulture(bool bNewValue)
{
	if (m_bForceEmphasizeCulture != bNewValue)
	{
		m_bForceEmphasizeCulture = bNewValue;

		m_aiEmphasizeCommerceCount[COMMERCE_CULTURE] += (bNewValue ? 1 : -1);
		FAssert(m_aiEmphasizeCommerceCount[COMMERCE_CULTURE] >= 0);
	}
}


int CvCityAI::AI_getBestBuildValue(int iIndex)
{
	FAssertMsg(iIndex >= 0, "iIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(iIndex < NUM_CITY_PLOTS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiBestBuildValue[iIndex];
}


int CvCityAI::AI_totalBestBuildValue(CvArea* pArea)
{
	CvPlot* pLoopPlot;
	int iTotalValue;
	int iI;

	iTotalValue = 0;

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		if (iI != CITY_HOME_PLOT)
		{
			pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->area() == pArea)
				{
					if ((pLoopPlot->getImprovementType() == NO_IMPROVEMENT) || !(GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_SAFE_AUTOMATION) && !(pLoopPlot->getImprovementType() == (GC.getDefineINT("RUINS_IMPROVEMENT")))))
					{
						iTotalValue += AI_getBestBuildValue(iI);
					}
				}
			}
		}
	}

	return iTotalValue;
}

int CvCityAI::AI_clearFeatureValue(int iIndex)
{
	CvPlot* pPlot = plotCity(getX_INLINE(), getY_INLINE(), iIndex);
	FAssert(pPlot != NULL);
	
	FeatureTypes eFeature = pPlot->getFeatureType();
	FAssert(eFeature != NO_FEATURE);
	
	CvFeatureInfo& kFeatureInfo = GC.getFeatureInfo(eFeature);
	
	/* original bts code
	int iValue = 0;
	iValue += kFeatureInfo.getYieldChange(YIELD_FOOD) * 100;
	iValue += kFeatureInfo.getYieldChange(YIELD_PRODUCTION) * 60;
	iValue += kFeatureInfo.getYieldChange(YIELD_COMMERCE) * 40;
	
	if (iValue > 0 && pPlot->isBeingWorked())
	{
		iValue *= 3;
		iValue /= 2;
	}
	if (iValue != 0)
	{
		BonusTypes eBonus = pPlot->getBonusType(getTeam());
		if (eBonus != NO_BONUS)
		{
			iValue *= 3;
			if (pPlot->getImprovementType() != NO_IMPROVEMENT)
			{
				if (GC.getImprovementInfo(pPlot->getImprovementType()).isImprovementBonusTrade(eBonus))
				{
					iValue *= 4;
				}
			}
		}
	} */
	// K-Mod. All that yield change stuff is taken into account by the improvement evaluation function anyway.
	// ... except the bit about keeping good features on top of bonuses
	int iValue = 0;
	BonusTypes eBonus = pPlot->getNonObsoleteBonusType(getTeam());
	if (eBonus != NO_BONUS)
	{
		iValue += kFeatureInfo.getYieldChange(YIELD_FOOD) * 100;
		iValue += kFeatureInfo.getYieldChange(YIELD_PRODUCTION) * 60;
		iValue += kFeatureInfo.getYieldChange(YIELD_COMMERCE) * 40;
		iValue *= 2;
		// that should be enough incentive to keep good features
	}
	// K-Mod end
	
	int iHealthValue = 0;
	if (kFeatureInfo.getHealthPercent() != 0)
	{
		int iHealth = goodHealth() - badHealth();
		
		/* original bts code
		iHealthValue += (6 * kFeatureInfo.getHealthPercent()) / std::max(3, 1 + iHealth);
		if (iHealthValue > 0 && !pPlot->isBeingWorked())
		{
			iHealthValue *= 3;
			iHealthValue /= 2;
		} */
		// K-Mod -
		iHealthValue += (iHealth < 0 ? 100 : 200/(3+iHealth)) + 100 * pPlot->getPlayerCityRadiusCount(getOwnerINLINE());
		iHealthValue *= kFeatureInfo.getHealthPercent();
		iHealthValue /= 100;
		// note: health is not any more valuable when we aren't working it.
		// That kind of thing should be handled by the chop code.
		// K-Mod end
	}
	iValue += iHealthValue;

	// K-Mod
	if (GC.getGame().getGwEventTally() >= 0) // if GW Threshold has been reached
	{
		iValue += kFeatureInfo.getWarmingDefense() * (150 + 5 * GET_PLAYER(getOwner()).getGwPercentAnger()) / 100;
	}
	// K-Mod end
	
	if (iValue > 0)
	{
		if (pPlot->getImprovementType() != NO_IMPROVEMENT)
		{
			if (GC.getImprovementInfo(pPlot->getImprovementType()).isRequiresFeature())
			{
				iValue += 500;
			}
		}
		
		if (GET_PLAYER(getOwnerINLINE()).getAdvancedStartPoints() >= 0)
		{
			iValue += 400;
		}
	}
	
	return -iValue;
}

// K-Mod, based on ideas from BBAI (this code is currently duplicated in AI_getYieldMultipliers and AI_countWorkedPoorTiles)
int CvCityAI::AI_getGoodTileCount() const
{
	int iGoodTileCount = 0;
	int aiFinalYields[NUM_YIELD_TYPES];

	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		if (iI != CITY_HOME_PLOT)
		{
			CvPlot* pLoopPlot = getCityIndexPlot(iI);

			if (NULL != pLoopPlot && pLoopPlot->getWorkingCity() == this)
			{
				int iWorkerCount = (kPlayer.AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD));

				bool bUseBaseValue = true;
				//If the tile has a BestBuild and is being improved, then use the BestBuild
				//determine if the tile is being improved.
				if (iWorkerCount > 0)
				{
					BuildTypes eBuild = NO_BUILD;
					if (m_aeBestBuild[iI] != NO_BUILD && m_aiBestBuildValue[iI] > 0)
					{
						eBuild = m_aeBestBuild[iI];
					}

					if( eBuild != NO_BUILD )
					{
						ImprovementTypes eImprovement = (ImprovementTypes)GC.getBuildInfo(eBuild).getImprovement();
						if (eImprovement != NO_IMPROVEMENT)
						{
							bool bIgnoreFeature = false;
							if (pLoopPlot->getFeatureType() != NO_FEATURE)
							{
								if (GC.getBuildInfo(eBuild).isFeatureRemove(pLoopPlot->getFeatureType()))
								{
									bIgnoreFeature = true;
								}
							}

							bUseBaseValue = false;
							for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
							{
								aiFinalYields[iJ] = (pLoopPlot->calculateNatureYield(((YieldTypes)iJ), getTeam(), bIgnoreFeature) + pLoopPlot->calculateImprovementYieldChange(eImprovement, ((YieldTypes)iJ), getOwnerINLINE(), false));
							}
						}
					}
				}

				//Otherwise use the base value.
				if (bUseBaseValue)
				{
					for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
					{
						//by default we'll use the current value
						aiFinalYields[iJ] = pLoopPlot->getYield((YieldTypes)iJ);
						if (pLoopPlot->getFeatureType() != NO_FEATURE)
						{
							aiFinalYields[iJ] += std::max(0, -GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange((YieldTypes)iJ));
						}
					}
				}

				if (aiFinalYields[YIELD_FOOD]*10 + aiFinalYields[YIELD_PRODUCTION]*6 + aiFinalYields[YIELD_COMMERCE]*4 > 27 ||
					(!pLoopPlot->isWater() && aiFinalYields[YIELD_FOOD]*10 + aiFinalYields[YIELD_PRODUCTION]*6 + aiFinalYields[YIELD_COMMERCE]*4 > 21))
				{
					iGoodTileCount++;
				}
			}
		}
	}
	return iGoodTileCount;
}

int CvCityAI::AI_countWorkedPoorTiles() const
{
	int iWorkedPoorTileCount = 0;
	int aiFinalYields[NUM_YIELD_TYPES];

	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		if (iI != CITY_HOME_PLOT)
		{
			CvPlot* pLoopPlot = getCityIndexPlot(iI);

			if (NULL != pLoopPlot && pLoopPlot->getWorkingCity() == this)
			{
				int iWorkerCount = (kPlayer.AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD));

				bool bUseBaseValue = true;
				//If the tile has a BestBuild and is being improved, then use the BestBuild
				//determine if the tile is being improved.
				if (iWorkerCount > 0)
				{
					BuildTypes eBuild = NO_BUILD;
					if (m_aeBestBuild[iI] != NO_BUILD && m_aiBestBuildValue[iI] > 0)
					{
						eBuild = m_aeBestBuild[iI];
					}

					if( eBuild != NO_BUILD )
					{
						ImprovementTypes eImprovement = (ImprovementTypes)GC.getBuildInfo(eBuild).getImprovement();
						if (eImprovement != NO_IMPROVEMENT)
						{
							bool bIgnoreFeature = false;
							if (pLoopPlot->getFeatureType() != NO_FEATURE)
							{
								if (GC.getBuildInfo(eBuild).isFeatureRemove(pLoopPlot->getFeatureType()))
								{
									bIgnoreFeature = true;
								}
							}

							bUseBaseValue = false;
							for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
							{
								aiFinalYields[iJ] = (pLoopPlot->calculateNatureYield(((YieldTypes)iJ), getTeam(), bIgnoreFeature) + pLoopPlot->calculateImprovementYieldChange(eImprovement, ((YieldTypes)iJ), getOwnerINLINE(), false));
							}
						}
					}
				}

				//Otherwise use the base value.
				if (bUseBaseValue)
				{
					for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
					{
						//by default we'll use the current value
						aiFinalYields[iJ] = pLoopPlot->getYield((YieldTypes)iJ);
						if (pLoopPlot->getFeatureType() != NO_FEATURE)
						{
							aiFinalYields[iJ] += std::max(0, -GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange((YieldTypes)iJ));
						}
					}
				}
				
				if (pLoopPlot->isBeingWorked() &&
					aiFinalYields[YIELD_FOOD]*10 + aiFinalYields[YIELD_PRODUCTION]*6 + aiFinalYields[YIELD_COMMERCE]*4 <= 27 &&
					(pLoopPlot->isWater() || aiFinalYields[YIELD_FOOD]*10 + aiFinalYields[YIELD_PRODUCTION]*6 + aiFinalYields[YIELD_COMMERCE]*4 <= 21))
				{
					iWorkedPoorTileCount++;
				}
			}
		}
	}
	return iWorkedPoorTileCount;
}

// K-Mod, based on BBAI ideas
int CvCityAI::AI_getTargetPopulation() const
{
	int iHealth = goodHealth() - badHealth() + getEspionageHealthCounter();
	int iTargetSize = AI_getGoodTileCount();

	iTargetSize = std::min(iTargetSize, 2 + getPopulation() + iHealth/2);

	if (iTargetSize < getPopulation())
	{
		iTargetSize = std::max(iTargetSize, getPopulation() - AI_countWorkedPoorTiles() + std::min(0, iHealth/2));
	}

	iTargetSize = std::min(iTargetSize, 1 + getPopulation()+(happyLevel()-unhappyLevel()+getEspionageHappinessCounter()));

	return iTargetSize;
}

// K-Mod note: This function was once used for Debug only, but I've made it the one and only way to get the yield multipliers.
// This way we don't have to put up with duplicated code.
void CvCityAI::AI_getYieldMultipliers( int &iFoodMultiplier, int &iProductionMultiplier, int &iCommerceMultiplier, int &iDesiredFoodChange ) const
{
	PROFILE_FUNC();
	
	int aiFinalYields[NUM_YIELD_TYPES];

	int iBonusFoodSurplus = 0;
	int iBonusFoodDeficit = 0;
	int iFeatureFoodSurplus = 0;
	int iHillFoodDeficit = 0;
	//int iFoodTotal = GC.getYieldInfo(YIELD_FOOD).getMinCity();
	//int iProductionTotal = GC.getYieldInfo(YIELD_PRODUCTION).getMinCity();
	// K-Mod. rather than starting at zero, and adding up productino from the tiles we are working,
	// I'm just going to use the actual value, which include yield from specialists, buildings, corporations etc.
	int iFoodTotal = getBaseYieldRate(YIELD_FOOD);
	int iProductionTotal = getBaseYieldRate(YIELD_PRODUCTION);
	// K-Mod end
	iFoodMultiplier = 100;
	iCommerceMultiplier = 100;
	iProductionMultiplier = 100;
	
	//int iWorkedFood = 0;
	int iWorkableFood = 0;
	int iWorkableFoodPlotCount = 0;
	
	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());

	int iGoodTileCount = 0;
	
	int iSpecialistCount = getSpecialistPopulation() - totalFreeSpecialists();

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		if (iI != CITY_HOME_PLOT)
		{
			CvPlot* pLoopPlot = getCityIndexPlot(iI);

			if (NULL != pLoopPlot && pLoopPlot->getWorkingCity() == this)
			{
				int iWorkerCount = (kPlayer.AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD));

				bool bUseBaseValue = true;
				//If the tile has a BestBuild and is being improved, then use the BestBuild
				//determine if the tile is being improved.
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/25/09                                jdog5000      */
/*                                                                                              */
/* Worker AI, City AI                                                                           */
/************************************************************************************************/
				if (iWorkerCount > 0)
				{
					BuildTypes eBuild = NO_BUILD;
					if (m_aeBestBuild[iI] != NO_BUILD && m_aiBestBuildValue[iI] > 0)
					{
						eBuild = m_aeBestBuild[iI];
					}
					else
					{
						// This check is necessary to stop oscillation which can result
						// when best build changes food situation for city, changing the best build.

						// K-Mod. I don't believe you. I think that if the rest of the code is right, this check will be unnecessary.
						/* lets find out
						CvUnit* pLoopUnit;
						CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();

						while (pUnitNode != NULL)
						{
							pLoopUnit = ::getUnit(pUnitNode->m_data);
							pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

							if (pLoopUnit->getBuildType() != NO_BUILD)
							{
								if( eBuild == NO_BUILD || pLoopPlot->getBuildTurnsLeft(eBuild,0,0) > pLoopPlot->getBuildTurnsLeft(pLoopUnit->getBuildType(),0,0) )
								{
									eBuild = pLoopUnit->getBuildType();
								}
							}
						}*/
					}

					if( eBuild != NO_BUILD )
					{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
						ImprovementTypes eImprovement = (ImprovementTypes)GC.getBuildInfo(eBuild).getImprovement();
						if (eImprovement != NO_IMPROVEMENT)
						{
							bool bIgnoreFeature = false;
							if (pLoopPlot->getFeatureType() != NO_FEATURE)
							{
								if (GC.getBuildInfo(eBuild).isFeatureRemove(pLoopPlot->getFeatureType()))
								{
									bIgnoreFeature = true;
								}
							}

							/*
							iHappyAdjust += GC.getImprovementInfo(eImprovement).getHappiness();
							if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT)
							{
								iHappyAdjust -= GC.getImprovementInfo(pLoopPlot->getImprovementType()).getHappiness();
							}
							*/
							
							bUseBaseValue = false;
							for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
							{
								aiFinalYields[iJ] = (pLoopPlot->calculateNatureYield(((YieldTypes)iJ), getTeam(), bIgnoreFeature) + pLoopPlot->calculateImprovementYieldChange(eImprovement, ((YieldTypes)iJ), getOwnerINLINE(), false));
							}
						}
					}
				}

				//Otherwise use the base value.
				if (bUseBaseValue)
				{
					for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
					{
						//by default we'll use the current value
						aiFinalYields[iJ] = pLoopPlot->getYield((YieldTypes)iJ);
						if (pLoopPlot->getFeatureType() != NO_FEATURE)
						{
							aiFinalYields[iJ] += std::max(0, -GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange((YieldTypes)iJ));
						}
					}
				}
				
				/* original code
				if (pLoopPlot->isBeingWorked())
				{
					iWorkedFood += aiFinalYields[YIELD_FOOD];					
				}
				else
				{
					if (aiFinalYields[YIELD_FOOD] >= GC.getFOOD_CONSUMPTION_PER_POPULATION())
					{
						iWorkableFood += aiFinalYields[YIELD_FOOD];
						iWorkableFoodPlotCount++;
					}
				} */
				
				/* original code
				if (pLoopPlot->isBeingWorked() || (((aiFinalYields[YIELD_FOOD]*10) + (aiFinalYields[YIELD_PRODUCTION]*6) + (aiFinalYields[YIELD_COMMERCE]*4)) > 21))
				{
					iGoodTileCount++;
					if (pLoopPlot->isBeingWorked())
					{
						iFoodTotal += aiFinalYields[YIELD_FOOD];
					}
					else
					{
						iFoodTotal += aiFinalYields[YIELD_FOOD] / 2;
					}
                    if (aiFinalYields[YIELD_PRODUCTION] > 1)
                    {	
                    	iProductionTotal += aiFinalYields[YIELD_PRODUCTION];
                    }
				}*/
				// K-Mod experimental changes.
				if (!pLoopPlot->isBeingWorked() && aiFinalYields[YIELD_FOOD] > GC.getFOOD_CONSUMPTION_PER_POPULATION())
				{
					// note: worked plots are already counted.
					iFoodTotal += aiFinalYields[YIELD_FOOD];
				}
				if (aiFinalYields[YIELD_FOOD]*10 + aiFinalYields[YIELD_PRODUCTION]*6 + aiFinalYields[YIELD_COMMERCE]*4 > 27 ||
					(!pLoopPlot->isWater() && aiFinalYields[YIELD_FOOD]*10 + aiFinalYields[YIELD_PRODUCTION]*6 + aiFinalYields[YIELD_COMMERCE]*4 > 21))
				{
					iGoodTileCount++;
					if (!pLoopPlot->isBeingWorked())
					{
						iProductionTotal += aiFinalYields[YIELD_PRODUCTION];
						iWorkableFood += aiFinalYields[YIELD_FOOD];
						iWorkableFoodPlotCount++;
						// note. this can count plots that are low on food.
					}
				}
				// K-Mod end

				if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
				{
                    int iNetFood = (aiFinalYields[YIELD_FOOD] - GC.getFOOD_CONSUMPTION_PER_POPULATION());
                    iBonusFoodSurplus += std::max(0, iNetFood);
                    iBonusFoodDeficit += std::max(0, -iNetFood);
				}

				if ((pLoopPlot->getFeatureType()) != NO_FEATURE)
				{
					iFeatureFoodSurplus += std::max(0, pLoopPlot->calculateNatureYield(YIELD_FOOD, getTeam()) - GC.getFOOD_CONSUMPTION_PER_POPULATION());
				}

				if ((pLoopPlot->isHills()))
				{
					iHillFoodDeficit += std::max(0, GC.getFOOD_CONSUMPTION_PER_POPULATION() - pLoopPlot->calculateNatureYield(YIELD_FOOD, getTeam()));
				}
			}
		}
	}
	// K-Mod, based on ideas from BBAI, and original bts code
	int iHealth = goodHealth() - badHealth() + getEspionageHealthCounter();
	int iTargetSize = iGoodTileCount;

	iTargetSize = std::min(iTargetSize, 2 + getPopulation() + iHealth/2);

	if (iTargetSize < getPopulation())
	{
		iTargetSize = std::max(iTargetSize, getPopulation() - AI_countWorkedPoorTiles() + std::min(0, iHealth/2));
	}

	iTargetSize = std::min(iTargetSize, 1 + getPopulation()+(happyLevel()-unhappyLevel()+getEspionageHappinessCounter()));

	int iFutureFoodAdjustment = 0;
	if (iWorkableFoodPlotCount > 0)
	{
		// calculate approximately how much extra food we could work if we used our specialists and our extra population.
		iFutureFoodAdjustment = (std::min(iSpecialistCount + std::max(0, iTargetSize - getPopulation()), iWorkableFoodPlotCount) * iWorkableFood) / iWorkableFoodPlotCount;
	}
	iFoodTotal += iFutureFoodAdjustment;
	// K-Mod end

	int iBonusFoodDiff = ((iBonusFoodSurplus + iFeatureFoodSurplus) - (iBonusFoodDeficit + iHillFoodDeficit / 2));


	/* original bts code. (Do we really want fat cities in advanced start games?)
	if (GET_PLAYER(getOwnerINLINE()).getAdvancedStartPoints() >= 0)
	{
		iTargetSize += 2 + GET_PLAYER(getOwnerINLINE()).getCurrentEra() / 2;
	}
	
	if (kPlayer.getAdvancedStartPoints() >= 0)
	{
		iTargetSize += kPlayer.getCurrentEra() / 2;
	} */

	/* original bts code
	if (iBonusFoodDiff < 2)
	{
		iFoodMultiplier += 10 * (2 - iBonusFoodDiff);
	}
	
	int iExtraFoodForGrowth = (std::max(0, iTargetSize - getPopulation()) + 3) / 4;
	if (getPopulation() < iTargetSize)
	{
		iExtraFoodForGrowth ++;
	}*/
	// K-Mod
	int iExtraFoodForGrowth = 0;
	if (iTargetSize > getPopulation())
	{
		iExtraFoodForGrowth = (iTargetSize - getPopulation())/2 + 1;
	}
	// K-Mod end

	int iFoodDifference = iFoodTotal - ((iTargetSize * GC.getFOOD_CONSUMPTION_PER_POPULATION()) + iExtraFoodForGrowth);

	iDesiredFoodChange = -iFoodDifference + std::max(0, -iHealth);
	/* original bts code (K-Mod, after my other changes, we don't need this.)
	if (iTargetSize > getPopulation())
	{
		if (iDesiredFoodChange > 3)
		{
			iDesiredFoodChange = (iDesiredFoodChange + 3) / 2;
		}
	} */

	// K-Mod. One of the adjustments from the original code, but with an extra condition:
	if (iBonusFoodDiff < 2 && iDesiredFoodChange > 0)
	{
		iFoodMultiplier += 10 * (2 - iBonusFoodDiff);
	}
	// K-Mod end

	if (iFoodDifference < 0)
	{
		iFoodMultiplier +=  -iFoodDifference * 4;
	}
	
	if (iFoodDifference > 4)
	{
		iFoodMultiplier -= 8 + 4 * iFoodDifference;
	}

	if (iProductionTotal < 10)
	{
	    iProductionMultiplier += (80 - 8 * iProductionTotal);
	}
	//int iProductionTarget = 1 + (std::min(getPopulation(), (iTargetSize * 3) / 5));
	int iProductionTarget = 1 + std::max(getPopulation(), iTargetSize * 3 / 5); // K-Mod

	if (iProductionTotal < iProductionTarget)
	{
	    iProductionMultiplier += 8 * (iProductionTarget - iProductionTotal);
	}

	// K-mod
	if (kPlayer.AI_getFlavorValue(FLAVOR_PRODUCTION) > 0 || kPlayer.AI_getFlavorValue(FLAVOR_MILITARY) >= 10)
	{
		iProductionMultiplier *= 113 + 2 * kPlayer.AI_getFlavorValue(FLAVOR_PRODUCTION) + kPlayer.AI_getFlavorValue(FLAVOR_MILITARY);
		iProductionMultiplier /= 100;
	}
	// K-Mod end

	if ((iBonusFoodSurplus + iFeatureFoodSurplus > 5) && ((iBonusFoodDeficit + iHillFoodDeficit) > 5))
	{
		if ((iBonusFoodDeficit + iHillFoodDeficit) > 8)
		{
			//probably a good candidate for a wonder pump
			iProductionMultiplier += 40;
			iCommerceMultiplier += (kPlayer.AI_isFinancialTrouble()) ? 0 : -40;
		}
	}
	
	
	int iNetCommerce = 1 + kPlayer.getCommerceRate(COMMERCE_GOLD) + kPlayer.getCommerceRate(COMMERCE_RESEARCH) + std::max(0, kPlayer.getGoldPerTurn());
/* original BTS code
	int iNetExpenses = kPlayer.calculateInflatedCosts() + std::min(0, kPlayer.getGoldPerTurn());
*/
	// Unofficial Patch
	int iNetExpenses = kPlayer.calculateInflatedCosts() + std::max(0, -kPlayer.getGoldPerTurn());

	int iRatio = (100 * iNetExpenses) / std::max(1, iNetCommerce);
	
	if (iRatio > 40)
	{
		//iCommerceMultiplier += (33 * (iRatio - 40)) / 60;
		// K-Mod, just a bit steeper
		iCommerceMultiplier += (50 * (iRatio - 40)) / 60;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/06/09                                jdog5000      */
/*                                                                                              */
/* Worker AI                                                                                    */
/************************************************************************************************/
	// AI no longer uses emphasis except for short term boosts.
	if( isHuman() )
	{
		if (AI_isEmphasizeYield(YIELD_FOOD))
		{
			iFoodMultiplier *= 130;
			iFoodMultiplier /= 100;
		}
		if (AI_isEmphasizeYield(YIELD_PRODUCTION))
		{
			iProductionMultiplier *= 140;
			iProductionMultiplier /= 100;
		}
		if (AI_isEmphasizeYield(YIELD_COMMERCE))
		{
			iCommerceMultiplier *= 140;
			iCommerceMultiplier /= 100;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	int iProductionAdvantage = 100 * AI_yieldMultiplier(YIELD_PRODUCTION);
	iProductionAdvantage /= kPlayer.AI_averageYieldMultiplier(YIELD_PRODUCTION);
	iProductionAdvantage *= kPlayer.AI_averageYieldMultiplier(YIELD_COMMERCE);
	iProductionAdvantage /= AI_yieldMultiplier(YIELD_COMMERCE);

	//now we normalize the effect by # of cities

	int iNumCities = kPlayer.getNumCities();
	FAssert(iNumCities > 0);//superstisious?

	//in short in an OCC the relative multipliers should *never* make a difference
	//so this indeed equals "100" for the iNumCities == 0 case.
	iProductionAdvantage = ((iProductionAdvantage * (iNumCities - 1) + 200) / (iNumCities + 1));

	iProductionMultiplier *= iProductionAdvantage;
	iProductionMultiplier /= 100;

	iCommerceMultiplier *= 100;
	iCommerceMultiplier /= iProductionAdvantage;

	int iGreatPeopleAdvantage = 100 * getTotalGreatPeopleRateModifier();
	iGreatPeopleAdvantage /= kPlayer.AI_averageGreatPeopleMultiplier();
	iGreatPeopleAdvantage = ((iGreatPeopleAdvantage * (iNumCities - 1) + 200) / (iNumCities + 1));
	iGreatPeopleAdvantage += 200; //gpp multipliers are larger than others so lets not go overboard
	iGreatPeopleAdvantage /= 3;

	//With great people we want to slightly increase food priority at the expense of commerce
	//this gracefully handles both wonder and specialist based GPP...
	iCommerceMultiplier *= 100;
	iCommerceMultiplier /= iGreatPeopleAdvantage;
	iFoodMultiplier *= iGreatPeopleAdvantage;
	iFoodMultiplier /= 100;

	if (kPlayer.AI_isDoStrategy(AI_STRATEGY_PRODUCTION))
	{
		/*iProductionMultiplier += 10;
		iCommerceMultiplier -= 10; */
		// K-Mod. Strategy_production is now more rare and specific than before
		// so when the AI wants to use it, I think we can afford to emphasise a bit more strongly...
		iProductionMultiplier += 25;
		iCommerceMultiplier -= 20;
		// K-Mod end
	}

	if (iFoodMultiplier < 100)
	{
		iFoodMultiplier = 10000 / (200 - iFoodMultiplier);
	}
	if (iProductionMultiplier < 100)
	{
		iProductionMultiplier = 10000 / (200 - iProductionMultiplier);
	}
	if (iCommerceMultiplier < 100)
	{
		iCommerceMultiplier = 10000 / (200 - iCommerceMultiplier);
	}

	/* original bts code (this seems like a good way to screw up your tile improvements at the drop of a hat...)
	if (angryPopulation(1) > 0)
	{
		iFoodMultiplier /= 2;
	} */
}


int CvCityAI::AI_getImprovementValue(CvPlot* pPlot, ImprovementTypes eImprovement, int iFoodPriority, int iProductionPriority, int iCommercePriority, int iFoodChange, int iClearFeatureValue, bool bEmphasizeIrrigation, BuildTypes* peBestBuild) const
{
	// first check if the improvement is valid on this plot
	// this also allows us work out whether or not the improvement will remove the plot feature...
	int iBestTempBuildValue = 0;
	BuildTypes eBestTempBuild = NO_BUILD;

	bool bIgnoreFeature = false;
	bool bValid = false;
	BonusTypes eBonus = pPlot->getBonusType(getTeam());
	BonusTypes eNonObsoleteBonus = pPlot->getNonObsoleteBonusType(getTeam());

	if (eImprovement == pPlot->getImprovementType())
	{
		bValid = true;
	}
	else
	{
		int iValue;
		for (int iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
		{
			BuildTypes eBuild = ((BuildTypes)iJ);

			if (GC.getBuildInfo(eBuild).getImprovement() == eImprovement)
			{
				if (GET_PLAYER(getOwnerINLINE()).canBuild(pPlot, eBuild, false))
				{
					iValue = 10000;

					iValue /= (GC.getBuildInfo(eBuild).getTime() + 1);

					// XXX feature production???

					if (iValue > iBestTempBuildValue)
					{
						iBestTempBuildValue = iValue;
						eBestTempBuild = eBuild;
					}
				}
			}
		}

		if (eBestTempBuild != NO_BUILD)
		{
			bValid = true;

			if (pPlot->getFeatureType() != NO_FEATURE)
			{
				if (GC.getBuildInfo(eBestTempBuild).isFeatureRemove(pPlot->getFeatureType()))
				{
					bIgnoreFeature = true;

					if (GC.getFeatureInfo(pPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) > 0)
					{
						if (eNonObsoleteBonus == NO_BONUS)
						{
							if (GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_LEAVE_FORESTS))
							{
								bValid = false;
							}
							else if (healthRate() < 0 && GC.getFeatureInfo(pPlot->getFeatureType()).getHealthPercent() > 0)
							{
								bValid = false;
							}
							else if (GET_PLAYER(getOwnerINLINE()).getFeatureHappiness(pPlot->getFeatureType()) > 0)
							{
								bValid = false;
							}
						}
					}
				}
			}
		}
	}

	if (!bValid)
	{
		if (peBestBuild != NULL)
			*peBestBuild = NO_BUILD;
		return 0;
	}

	// Now get the valid of the improvement itself.

	ImprovementTypes eFinalImprovement = finalImprovementUpgrade(eImprovement);

	if (eFinalImprovement == NO_IMPROVEMENT)
	{
		eFinalImprovement = eImprovement;
	}

	int iValue = 0;
	int aiDiffYields[NUM_YIELD_TYPES];
	int aiFinalYields[NUM_YIELD_TYPES];

	if (eBonus != NO_BONUS)
	{
		if (eNonObsoleteBonus != NO_BONUS)
		{
			if (GC.getImprovementInfo(eFinalImprovement).isImprovementBonusTrade(eNonObsoleteBonus))
			{
				// K-Mod
				iValue += (GET_PLAYER(getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus, 1) * 50);
				iValue += 100;
				// K-Mod end
			}
			else
			{
				// K-Mod, bug fix. (original code deleted now.)
				// Presumablly the original author wanted to subtract 1000 if eBestBuild would take away the bonus; not ... the nonsense they actually wrote.
				ImprovementTypes eCurrentImprovement = pPlot->getImprovementType();
				if (eCurrentImprovement != NO_IMPROVEMENT && GC.getImprovementInfo(eCurrentImprovement).isImprovementBonusTrade(eNonObsoleteBonus))
				{
					// By the way, AI_bonusVal is typically 10 for the first bonus, and 2 for subsequent.
					iValue -= (GET_PLAYER(getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus, -1) * 50);
					iValue -= 100;
				}
			}
		}
	}
	else
	{
		for (int iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
		{
			if (GC.getImprovementInfo(eFinalImprovement).getImprovementBonusDiscoverRand(iJ) > 0)
			{
				iValue++;
			}
		}
	}

	if (iValue >= 0)
	{
		iValue *= 2;
		for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
		{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/06/09                                jdog5000      */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
/* original BTS code
			aiFinalYields[iJ] = 2*(pPlot->calculateNatureYield(((YieldTypes)iJ), getTeam(), bIgnoreFeature));
			aiFinalYields[iJ] += (pPlot->calculateImprovementYieldChange(eFinalImprovement, ((YieldTypes)iJ), getOwnerINLINE(), false));
			aiFinalYields[iJ] += (pPlot->calculateImprovementYieldChange(eImprovement, ((YieldTypes)iJ), getOwnerINLINE(), false));
			if (bIgnoreFeature && pPlot->getFeatureType() != NO_FEATURE)
			{
				aiFinalYields[iJ] -= 2 * GC.getFeatureInfo(pPlot->getFeatureType()).getYieldChange((YieldTypes)iJ);							
			}
			aiDiffYields[iJ] = (aiFinalYields[iJ] - (2 * pPlot->getYield(((YieldTypes)iJ))));
*/
			//
			aiFinalYields[iJ] = 2*(pPlot->calculateNatureYield(((YieldTypes)iJ), getTeam(), bIgnoreFeature));
			aiFinalYields[iJ] += (pPlot->calculateImprovementYieldChange(eFinalImprovement, ((YieldTypes)iJ), getOwnerINLINE(), false, true));
			aiFinalYields[iJ] += (pPlot->calculateImprovementYieldChange(eImprovement, ((YieldTypes)iJ), getOwnerINLINE(), false, true));
			/* if (bIgnoreFeature && pPlot->getFeatureType() != NO_FEATURE)
			{
				aiFinalYields[iJ] -= 2 * GC.getFeatureInfo(pPlot->getFeatureType()).getYieldChange((YieldTypes)iJ);							
			} */ // disabled by K-Mod. (this is already taken into account with bIgnoreFeature in calculateNatureYield)

			int iCurYield = 2*(pPlot->calculateNatureYield(((YieldTypes)iJ), getTeam(), false));

			ImprovementTypes eCurImprovement = pPlot->getImprovementType();
			if( eCurImprovement != NO_IMPROVEMENT )
			{
				ImprovementTypes eCurFinalImprovement = finalImprovementUpgrade(eCurImprovement);
				if (eCurFinalImprovement == NO_IMPROVEMENT)
				{
					eCurFinalImprovement = eCurImprovement;
				}
				iCurYield += (pPlot->calculateImprovementYieldChange(eCurFinalImprovement, ((YieldTypes)iJ), getOwnerINLINE(), false, true));
				iCurYield += (pPlot->calculateImprovementYieldChange(eCurImprovement, ((YieldTypes)iJ), getOwnerINLINE(), false, true));
			}

			aiDiffYields[iJ] = (aiFinalYields[iJ] - iCurYield);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		}

		// K-Mod
		/* remember this from AI_updateBestBuild?
			if (iFoodDifference < 0)
			{
				iFoodMultiplier +=  -iFoodDifference * 4;
			}
			
			if (iFoodDifference > 4)
			{
				iFoodMultiplier -= 8 + 4 * iFoodDifference;
			}
		that roughly means that
		if iFoodChange > 0 && iFoodChange - aiDiffYields[0] <= 0, or
		if iFoodChange < -4 && iFoodChange - aiDiffYields[0] >= -4
		then the food multiplier will change.
		We should try to preempt that change to prevent bestbuild from oscillating. */
		int iCorrectedFoodPriority = iFoodPriority;
		if (iFoodChange > 0 && iFoodChange - aiDiffYields[YIELD_FOOD] <= 0)
		{
			iCorrectedFoodPriority -= iFoodChange * 4; // undo the food priority increase.
		}
		if (iFoodChange < -4 && iFoodChange - aiDiffYields[YIELD_FOOD] >= -4)
		{
			iCorrectedFoodPriority += 8 + 4 * -iFoodChange; // undo the food priority decrease
		}
		// This corrected priority isn't perfect, but I think it will be better than nothing.
		// K-Mod end

		iValue += (aiDiffYields[YIELD_FOOD] * ((100 * iCorrectedFoodPriority) / 100));
		iValue += (aiDiffYields[YIELD_PRODUCTION] * ((60 * iProductionPriority) / 100));
		iValue += (aiDiffYields[YIELD_COMMERCE] * ((40 * iCommercePriority) / 100));

		iValue /= 2;

		for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
		{
			aiFinalYields[iJ] /= 2;
			aiDiffYields[iJ] /= 2;
		}

		if (iValue > 0)
		{
			// this is mainly to make it improve better tiles first
			//flood plain > grassland > plain > tundra
			iValue += (aiFinalYields[YIELD_FOOD] * 10);
			iValue += (aiFinalYields[YIELD_PRODUCTION] * 6);
			iValue += (aiFinalYields[YIELD_COMMERCE] * 4);

			if (aiFinalYields[YIELD_FOOD] >= GC.getFOOD_CONSUMPTION_PER_POPULATION())
			{
				//this is a food yielding tile
				if (iCorrectedFoodPriority > 100)
				{
					iValue *= 100 + iCorrectedFoodPriority;
					iValue /= 200;							
				}
				if (iFoodChange > 0)
				{
					iValue += (10 * (1 + aiDiffYields[YIELD_FOOD]) * (1 + aiFinalYields[YIELD_FOOD] - GC.getFOOD_CONSUMPTION_PER_POPULATION()) * iFoodChange * iCorrectedFoodPriority) / 100;
				}
				if (iCommercePriority > 100)
				{
					iValue *= 100 + (((iCommercePriority - 100) * aiDiffYields[YIELD_COMMERCE]) / 2);
					iValue /= 100;
				}
			}
			else if (aiFinalYields[YIELD_FOOD] < GC.getFOOD_CONSUMPTION_PER_POPULATION())
			{
				if ((aiDiffYields[YIELD_PRODUCTION] > 0) && (aiFinalYields[YIELD_FOOD]+aiFinalYields[YIELD_PRODUCTION] > 3))
				{
					if (iCorrectedFoodPriority < 100 || GET_PLAYER(getOwnerINLINE()).getCurrentEra() < 2)
					{
						//value booster for mines on hills
						iValue *= (100 + 25 * aiDiffYields[YIELD_PRODUCTION]);
						iValue /= 100;
					}
				}
				if (iFoodChange < 0)
				{
					iValue *= 4 - iFoodChange;
					iValue /= 3 + aiFinalYields[YIELD_FOOD];
				}
			}

			if ((iCorrectedFoodPriority < 100) && (iProductionPriority > 100))
			{
				iValue *= (200 + ((iProductionPriority - 100)*aiFinalYields[YIELD_PRODUCTION]));
				iValue /= 200;
			}
			if (eNonObsoleteBonus == NO_BONUS)
			{
				if (iFoodChange > 0)
				{
					//We want more food.
					iValue *= 2 + std::max(0, aiDiffYields[YIELD_FOOD]);
					iValue /= 2 * (1 + std::max(0, -aiDiffYields[YIELD_FOOD]));
				}
//				else if (iFoodChange < 0)
//				{
//					//We want to soak up food.
//					iValue *= 8;
//					iValue /= 8 + std::max(0, aiDiffYields[YIELD_FOOD]);
//				}
			}
		}

		if (bEmphasizeIrrigation && GC.getImprovementInfo(eFinalImprovement).isCarriesIrrigation())
		{
			iValue += 500;
		}

		if (getImprovementFreeSpecialists(eFinalImprovement) > 0)
		{
			iValue += 2000;
		}

		int iHappiness = GC.getImprovementInfo(eFinalImprovement).getHappiness();
		if ((iHappiness != 0) && !(GET_PLAYER(getOwnerINLINE()).getAdvancedStartPoints() >= 0))
		{
			//int iHappyLevel = iHappyAdjust + (happyLevel() - unhappyLevel(0));
			int iHappyLevel = happyLevel() - unhappyLevel(0); // iHappyAdjust isn't currently being used.
			if (eImprovement == pPlot->getImprovementType())
			{
				iHappyLevel -= iHappiness;
			}
			int iHealthLevel = (goodHealth() - badHealth(false, 0));

			int iHappyValue = 0;
			if (iHappyLevel <= 0)
			{
				iHappyValue += 400;
			}
			bool bCanGrow = true;// (getYieldRate(YIELD_FOOD) > foodConsumption());

			/* original bts code
			if (iHappyLevel <= iHealthLevel)
			{
				iHappyValue += 200 * std::max(0, (bCanGrow ? std::min(6, 2 + iHealthLevel - iHappyLevel) : 0) - iHappyLevel);
			}
			else */ // commented out by K-Mod
			{
				iHappyValue += 200 * std::max(0, (bCanGrow ? 1 : 0) - iHappyLevel);
			}
			if (!pPlot->isBeingWorked())
			{
				iHappyValue *= 4;
				iHappyValue /= 3;
			}
			//iHappyValue += std::max(0, (pPlot->getCityRadiusCount() - 1)) * ((iHappyValue > 0) ? iHappyLevel / 2 : 200);
			// K-Mod
			iHappyValue *= (pPlot->getPlayerCityRadiusCount(getOwner()) + 1);
			iHappyValue /= 2;
			//
			iValue += iHappyValue * iHappiness;
		}

		if (!isHuman())
		{
			iValue *= std::max(0, (GC.getLeaderHeadInfo(getPersonalityType()).getImprovementWeightModifier(eFinalImprovement) + 200));
			iValue /= 200;
		}

		if (pPlot->getImprovementType() == NO_IMPROVEMENT)
		{
			//if (pPlot->isBeingWorked())
			if (pPlot->isBeingWorked()  // K-Mod. (don't boost the value if it means removing a good feature.)
				&& (iClearFeatureValue >= 0 || eBestTempBuild == NO_BUILD || !GC.getBuildInfo(eBestTempBuild).isFeatureRemove(pPlot->getFeatureType())))
			{
				iValue *= 5;
				iValue /= 4;
			}

			if (eBestTempBuild != NO_BUILD)
			{
				if (pPlot->getFeatureType() != NO_FEATURE)
				{
					if (GC.getBuildInfo(eBestTempBuild).isFeatureRemove(pPlot->getFeatureType()))
					{
						CvCity* pCity;
						iValue += pPlot->getFeatureProduction(eBestTempBuild, getTeam(), &pCity) * 2;
						FAssert(pCity == this);

						iValue += iClearFeatureValue;
					}
				}
			}
		}
		else
		{
			// cottage/villages (don't want to chop them up if turns have been invested)
			ImprovementTypes eImprovementDowngrade = (ImprovementTypes)GC.getImprovementInfo(pPlot->getImprovementType()).getImprovementPillage();
			while (eImprovementDowngrade != NO_IMPROVEMENT)
			{
				CvImprovementInfo& kImprovementDowngrade = GC.getImprovementInfo(eImprovementDowngrade);
				iValue -= kImprovementDowngrade.getUpgradeTime() * 8;
				eImprovementDowngrade = (ImprovementTypes)kImprovementDowngrade.getImprovementPillage();
			}

			if (GC.getImprovementInfo(pPlot->getImprovementType()).getImprovementUpgrade() != NO_IMPROVEMENT)
			{
				iValue -= (GC.getImprovementInfo(pPlot->getImprovementType()).getUpgradeTime() * 8 * (pPlot->getUpgradeProgress())) / std::max(1, GC.getGameINLINE().getImprovementUpgradeTime(pPlot->getImprovementType()));
			}

			if (eNonObsoleteBonus == NO_BONUS)
			{
				if (isWorkingPlot(pPlot))
				{
					if (((iCorrectedFoodPriority < 100) && (aiFinalYields[YIELD_FOOD] >= GC.getFOOD_CONSUMPTION_PER_POPULATION())) || (GC.getImprovementInfo(pPlot->getImprovementType()).getImprovementPillage() != NO_IMPROVEMENT))
					{
						iValue -= 70;
						iValue *= 2;
						iValue /= 3;
					}
				}
			}

			if (GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_SAFE_AUTOMATION))
			{
				iValue /= 4;	//Greatly prefer builds which are legal.
			}
		}
	}
	if (peBestBuild != NULL)
		*peBestBuild = eBestTempBuild;

	return iValue;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	
BuildTypes CvCityAI::AI_getBestBuild(int iIndex) const
{
	FAssertMsg(iIndex >= 0, "iIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(iIndex < NUM_CITY_PLOTS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aeBestBuild[iIndex];
}


int CvCityAI::AI_countBestBuilds(CvArea* pArea) const
{
	CvPlot* pLoopPlot;
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		if (iI != CITY_HOME_PLOT)
		{
			pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->area() == pArea)
				{
					if (AI_getBestBuild(iI) != NO_BUILD)
					{
						iCount++;
					}
				}
			}
		}
	}

	return iCount;
}

// Note: this function has been somewhat mangled by K-Mod
void CvCityAI::AI_updateBestBuild()
{
	int iFoodMultiplier, iProductionMultiplier, iCommerceMultiplier, iDesiredFoodChange;

	AI_getYieldMultipliers(iFoodMultiplier, iProductionMultiplier, iCommerceMultiplier, iDesiredFoodChange);

	/* I've disabled these for now
	int iHappyAdjust = 0;
	int iHealthAdjust = 0; */

	bool bChop = false;

    if (!bChop)
	{
		ProjectTypes eProductionProject = getProductionProject();
		bChop = (eProductionProject != NO_PROJECT && AI_projectValue(eProductionProject) > 0);
	}
	if (!bChop)
	{
		BuildingTypes eProductionBuilding = getProductionBuilding();
		bChop = (eProductionBuilding != NO_BUILDING && isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eProductionBuilding).getBuildingClassType())));
	}
	if (!bChop)
	{
		UnitTypes eProductionUnit = getProductionUnit();
		bChop = (eProductionUnit != NO_UNIT && GC.getUnitInfo(eProductionUnit).isFoodProduction());
	}
	if (!bChop)
	{
		bChop = ((area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) || (area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE) || (area()->getAreaAIType(getTeam()) == AREAAI_MASSING));
	}

	/*if (getProductionBuilding() != NO_BUILDING)
	{
		iHappyAdjust += getBuildingHappiness(getProductionBuilding());
		iHealthAdjust += getBuildingHealth(getProductionBuilding());
	}*/

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		if (iI != CITY_HOME_PLOT)
		{
			CvPlot* pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);

			if (NULL != pLoopPlot && pLoopPlot->getWorkingCity() == this)
			{
				int iLastBestBuildValue = m_aiBestBuildValue[iI];
				BuildTypes eLastBestBuildType = m_aeBestBuild[iI];

				//AI_bestPlotBuild(pLoopPlot, &(m_aiBestBuildValue[iI]), &(m_aeBestBuild[iI]), iFoodMultiplier, iProductionMultiplier, iCommerceMultiplier, bChop, iHappyAdjust, iHealthAdjust, iDesiredFoodChange);
				AI_bestPlotBuild(pLoopPlot, &(m_aiBestBuildValue[iI]), &(m_aeBestBuild[iI]), iFoodMultiplier, iProductionMultiplier, iCommerceMultiplier, bChop, 0, 0, iDesiredFoodChange);
				//int iWorkerCount = GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD); // K-Mod, originally this was all workers at the city.
				/* m_aiBestBuildValue[iI] *= 4;
				m_aiBestBuildValue[iI] += 3 + iWorkerCount; // (round up)
				m_aiBestBuildValue[iI] /= (4 + iWorkerCount); */ // disabled by K-Mod. I don't think this really helps us.

				if (m_aiBestBuildValue[iI] > 0)
				{
					FAssert(m_aeBestBuild[iI] != NO_BUILD);
				}
				if (m_aeBestBuild[iI] != NO_BUILD)
				{
					FAssert(m_aiBestBuildValue[iI] > 0);
				}

				// K-Mod, make some adjustments to our yield weights based on our new bestbuild
				if (m_aeBestBuild[iI] != NO_BUILD && m_aeBestBuild[iI] != eLastBestBuildType)
				{
					// its a new improvement, so lets adjust our values.
					// This is rough, but better than nothing. (at least, I hope so...)
					int iOldFood = (eLastBestBuildType == NO_BUILD)? pLoopPlot->getYield(YIELD_FOOD) : pLoopPlot->getYieldWithBuild(eLastBestBuildType, YIELD_FOOD, true);
					int iOldProd = (eLastBestBuildType == NO_BUILD)? pLoopPlot->getYield(YIELD_PRODUCTION) : pLoopPlot->getYieldWithBuild(eLastBestBuildType, YIELD_PRODUCTION, true);

					int iDelta = pLoopPlot->getYieldWithBuild(m_aeBestBuild[iI], YIELD_FOOD, true) - iOldFood;
					if (iDelta != 0)
					{
						/* original K-Mod code...
						if (iFoodDifference < 0)
						{
							iFoodMultiplier -= std::min(iDelta, -iFoodDifference) * 4;
							// cf. iFoodMultiplier +=  -iFoodDifference * 4;
						}
						
						if (iFoodDifference > 4)
						{
							iFoodMultiplier -= std::max(iDelta, 4 - iFoodDifference) * 4;
							if (iFoodDifference + iDelta <= 4)
								iFoodMultiplier += 8;
							// cf. iFoodMultiplier -= 8 + 4 * iFoodDifference;
						}
						iFoodDifference += iDelta; */
						iDesiredFoodChange -= iDelta;
						iFoodMultiplier -= 4 * iDelta;
					}

					iDelta = pLoopPlot->getYieldWithBuild(m_aeBestBuild[iI], YIELD_PRODUCTION, true) - iOldProd;
					if (iDelta != 0)
					{
						/* original K-Mod code... 
						if (iProductionTotal < 10)
						{
							iProductionMultiplier -= 8 * std::min(iDelta, 10-iProductionTotal);
							// cf. iProductionMultiplier += (80 - 8 * iProductionTotal);
						}
						
						if (iProductionTotal < iProductionTarget)
						{
							iProductionMultiplier -= 8 * std::min(iDelta, iProductionTarget - iProductionTotal);
							// cf. iProductionMultiplier += 8 * (iProductionTarget - iProductionTotal);
						}
						iProductionTotal += iDelta;*/
						iProductionMultiplier -= 8 * iDelta;
					}
					// Happiness modifers.. maybe I'll do this later, after testing etc.

					// since best-build has changed, cancel all current build missions on this plot
					if (eLastBestBuildType != NO_BUILD)
					{
						CvPlayer& kOwner = GET_PLAYER(getOwnerINLINE());
						int iLoop;
						for(CvSelectionGroup* pLoopSelectionGroup = kOwner.firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = kOwner.nextSelectionGroup(&iLoop))
						{
							if (pLoopSelectionGroup->AI_getMissionAIPlot() == pLoopPlot && pLoopSelectionGroup->AI_getMissionAIType() == MISSIONAI_BUILD)
							{
								FAssert(pLoopSelectionGroup->getHeadUnitAI() == UNITAI_WORKER || pLoopSelectionGroup->getHeadUnitAI() == UNITAI_WORKER_SEA);
								pLoopSelectionGroup->clearMissionQueue();
							}
						}
					}
				}
				// K-Mod end

				// K-Mod
				//if (iWorkerCount > 0)
				/*{
					CvUnit* pLoopUnit;
					CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();

					while (pUnitNode != NULL)
					{
						pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

						if (pLoopUnit->getOwnerINLINE() == getOwnerINLINE() && pLoopUnit->getGroup()->AI_isControlled() &&
							pLoopUnit->getBuildType() != NO_BUILD && pLoopUnit->getBuildType() != m_aeBestBuild[iI])
						{
							pLoopUnit->getGroup()->clearMissionQueue();
						}
					}
				}*/
				// K-Mod end
			}
		}
	}


	{	//new experimental yieldValue calcuation
		short aiYields[NUM_YIELD_TYPES];
		int iBestPlot = -1;
		int iBestPlotValue = -1;
		int iValue;
		
		int iBestUnworkedPlotValue = 0;
		
		int aiValues[NUM_CITY_PLOTS];
		
		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			if (iI != CITY_HOME_PLOT)
			{
				CvPlot* pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);

				if (NULL != pLoopPlot && pLoopPlot->getWorkingCity() == this)
				{
					if (m_aeBestBuild[iI] != NO_BUILD)
					{
						for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
						{
							aiYields[iJ] = pLoopPlot->getYieldWithBuild(m_aeBestBuild[iI], (YieldTypes)iJ, true);
						}
						
						iValue = AI_yieldValue(aiYields, NULL, false, false, false, false, true, true);
						aiValues[iI] = iValue;
						if ((iValue > 0) && (pLoopPlot->getRouteType() != NO_ROUTE))
						{
							iValue++;
						}
						//FAssert(iValue > 0);
						
						iValue = std::max(0, iValue);
						
						m_aiBestBuildValue[iI] *= iValue + 100;
						m_aiBestBuildValue[iI] /= 100;
						
						if (iValue > iBestPlotValue)
						{
							iBestPlot = iI;
							iBestPlotValue = iValue;							
						}
					}
					if (!pLoopPlot->isBeingWorked())
					{
						for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
						{
							aiYields[iJ] = pLoopPlot->getYield((YieldTypes)iJ);
						}
						
						iValue = AI_yieldValue(aiYields, NULL, false, false, false, false, true, true);					
						
						iBestUnworkedPlotValue = std::max(iBestUnworkedPlotValue, iValue);
					}
				}
			}
		}
		if (iBestPlot != -1)
		{
			m_aiBestBuildValue[iBestPlot] *= 2;
		}

		//Prune plots which are sub-par.
		if (iBestUnworkedPlotValue > 0)
		{
			for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
			{
				if (iI != CITY_HOME_PLOT)
				{
					CvPlot* pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);

					if (NULL != pLoopPlot && pLoopPlot->getWorkingCity() == this)
					{
						if (m_aeBestBuild[iI] != NO_BUILD) 
						{
							if (!pLoopPlot->isBeingWorked() && (pLoopPlot->getImprovementType() == NO_IMPROVEMENT))
							{
								if (GC.getBuildInfo(m_aeBestBuild[iI]).getImprovement() != NO_IMPROVEMENT)
								{
									if ((aiValues[iI] <= iBestUnworkedPlotValue) && (aiValues[iI] < 500))
									{
										m_aiBestBuildValue[iI] = 1;
									}
								}
							}
							else if ((pLoopPlot->getImprovementType() != NO_IMPROVEMENT) && (GC.getBuildInfo(m_aeBestBuild[iI]).getImprovement() != NO_IMPROVEMENT))
							{
								for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
								{
									aiYields[iJ] = pLoopPlot->getYield((YieldTypes)iJ);
								}
								
								iValue = AI_yieldValue(aiYields, NULL, false, false, false, false, true, true);
								if (iValue > aiValues[iI])
								{
									m_aiBestBuildValue[iI] = 1;
								}
							}
						}
					}
				}
			}
		}
	}
}

// Protected Functions...

void CvCityAI::AI_doDraft(bool bForce)
{
	PROFILE_FUNC();

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	FAssert(!isHuman());
	if (isBarbarian())
	{
		return;
	}

	if (canConscript())
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      07/12/09                                jdog5000      */
/*                                                                                              */
/* City AI, War Strategy AI                                                                     */
/************************************************************************************************/
		// if (GC.getGameINLINE().AI_combatValue(getConscriptUnit()) > 33) // disabled by K-Mod
		{
			if (bForce)
			{
				conscript();
				return;
			}
			bool bLandWar = ((area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) || (area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE) || (area()->getAreaAIType(getTeam()) == AREAAI_MASSING));
			bool bDanger = (!AI_isDefended() && AI_isDanger());
			int iUnitCostPerMil = kOwner.AI_unitCostPerMil(); // K-Mod

			// Don't go broke from drafting
			//if( !bDanger && kOwner.AI_isFinancialTrouble() )
			if (!bDanger && iUnitCostPerMil > kOwner.AI_maxUnitCostPerMil(area(), 50)) // K-Mod. (cf. conditions for scraping units in AI_doTurnUnitPost)
			{
				return;
			}

			// K-Mod. See if our drafted unit is particularly good value.
			// (cf. my calculation in CvPlayerAI::AI_civicValue)
			UnitTypes eConscriptUnit = getConscriptUnit();
			int iConscriptPop = std::max(1, GC.getUnitInfo(eConscriptUnit).getProductionCost() / GC.getDefineINT("CONSCRIPT_POPULATION_PER_COST"));

			// call it "good value" if we get at least 1.4 times the normal hammers-per-conscript-pop.
			// (with standard settings, this only happens for riflemen)
			bool bGoodValue = 10 * GC.getUnitInfo(eConscriptUnit).getProductionCost() / (iConscriptPop * GC.getDefineINT("CONSCRIPT_POPULATION_PER_COST")) >= 14;

			// one more thing... it's not "good value" if we already have too many troops.
			if (!bLandWar && bGoodValue)
			{
				bGoodValue = iUnitCostPerMil <= kOwner.AI_maxUnitCostPerMil(area(), 20) + kOwner.AI_getFlavorValue(FLAVOR_MILITARY);
			}
			// K-Mod end - although, I've put a bunch of 'bGoodValue' conditions in all through the rest of this function.

			// Don't shrink cities too much
			//int iConscriptPop = getConscriptPopulation();
			if (!bGoodValue && !bDanger && (3 * (getPopulation() - iConscriptPop) < getHighestPopulation() * 2) )
			{
				return;
			}

			// Large cities want a little spare happiness
			int iHappyDiff = GC.getDefineINT("CONSCRIPT_POP_ANGER") - iConscriptPop + (bGoodValue ? 0 : getPopulation()/10);

			if ((bGoodValue || bLandWar) && angryPopulation(iHappyDiff) == 0)
			{
				bool bWait = true;

				if( bWait && kOwner.AI_isDoStrategy(AI_STRATEGY_TURTLE) )
				{
					// Full out defensive
					/* original bts code
					if( bDanger || (getPopulation() >= std::max(5, getHighestPopulation() - 1)) )
					{
						bWait = false;
					}
					else if( AI_countWorkedPoorTiles() >= 1 )
					{
						bWait = false;
					}*/

					// K-Mod: full out defensive indeed. We've already checked for happiness, and we're desperate for units.
					// Just beware of happiness sources that might expire - such as military happiness.
					if (getConscriptAngerTimer() == 0 || AI_countWorkedPoorTiles() > 0)
						bWait = false;
				}

				if( bWait && bDanger )
				{
					// If city might be captured, don't hold back
					/* BBAI code
					int iOurDefense = GET_TEAM(getTeam()).AI_getOurPlotStrength(plot(),0,true,false,true);
					int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_getEnemyPlotStrength(plot(),2,false,false);

					if( (iOurDefense == 0) || (3*iEnemyOffense > 2*iOurDefense) ) */
					// K-Mod
					int iOurDefense = kOwner.AI_localDefenceStrength(plot(), getTeam(), DOMAIN_LAND, 0);
					int iEnemyOffense = kOwner.AI_localAttackStrength(plot(), NO_TEAM, DOMAIN_LAND, 2);
					if (iOurDefense < iEnemyOffense)
					// K-Mod end
					{
						bWait = false;
					}
				}

				if( bWait )
				{
					// Non-critical, only burn population if population is not worth much
					//if ((getConscriptAngerTimer() == 0) && (AI_countWorkedPoorTiles() > 1))
					if ((getConscriptAngerTimer() == 0 || isNoUnhappiness()) // K-Mod
						&& (bGoodValue || AI_countWorkedPoorTiles() > 0 || foodDifference()+getFood() < 0 || (foodDifference() < 0 && healthRate() <= -4)))
					{
						//if( (getPopulation() >= std::max(5, getHighestPopulation() - 1)) )
						// We're working poor tiles. What more do you want?
						{
							bWait = false;
						}
					}
				}

				if( !bWait && gCityLogLevel >= 2 )
				{
					logBBAI("      City %S (size %d, highest %d) chooses to conscript with danger: %d, land war: %d, poor tiles: %d%s", getName().GetCString(), getPopulation(), getHighestPopulation(), bDanger, bLandWar, AI_countWorkedPoorTiles(), bGoodValue ? ", good value" : "");
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				if (!bWait)
				{
					conscript();
				}
			}
		}
	}
}

// This function has been heavily edited for K-Mod
void CvCityAI::AI_doHurry(bool bForce)
{
	PROFILE_FUNC();

	FAssert(!isHuman() || isProductionAutomated());

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	
	if (kOwner.isBarbarian())
	{
		return;
	}

	if (getProduction() == 0 && !bForce)
	{
		return;
	}

	UnitTypes eProductionUnit = getProductionUnit();
	UnitAITypes eProductionUnitAI = getProductionUnitAI();
	BuildingTypes eProductionBuilding = getProductionBuilding();

	for (int iI = 0; iI < GC.getNumHurryInfos(); iI++)
	{
		if (!canHurry((HurryTypes)iI))
			continue;

		if (bForce)
		{
			hurry((HurryTypes)iI);
			break;
		}

		// gold hurry information
		int iHurryGold = hurryGold((HurryTypes)iI);
		int iGoldTarget = 0;
		int iGoldCost = 0;
		if (iHurryGold > 0)
		{
			if (iHurryGold > kOwner.getGold() - kOwner.AI_goldTarget(true))
				continue; // we don't have enough gold for this hurry type.
			iGoldTarget = kOwner.AI_goldTarget();
			iGoldCost = iHurryGold;
			if (kOwner.getGold() - iHurryGold >= iGoldTarget)
			{
				iGoldCost *= 100;
				iGoldCost /= 100 + 50 * (kOwner.getGold() - iHurryGold) / std::max(iGoldTarget, iHurryGold);
			}
			if (kOwner.isHuman())
				iGoldCost = iGoldCost * 3 / 2;
		}
		//

		// population hurry information
		int iHurryAngerLength = hurryAngerLength((HurryTypes)iI);
		int iHurryPopulation = hurryPopulation((HurryTypes)iI);

		int iHappyDiff = 0;
		int iHappy = 0;
		int iPopCost = 0;
		//

		if (iHurryPopulation > 0)
		{
			if (!isNoUnhappiness())
			{
				iHappyDiff = iHurryPopulation - GC.getDefineINT("HURRY_POP_ANGER");

				if (iHurryAngerLength > 0 && getHurryAngerTimer() > 1)
					iHappyDiff -= ROUND_DIVIDE((kOwner.AI_getFlavorValue(FLAVOR_GROWTH) > 0 ? 4 : 3) * getHurryAngerTimer(), iHurryAngerLength);
			}

			iHappy = happyLevel() - unhappyLevel();

			if (iHappyDiff > 0 && iGoldCost == 0)
			{
				if (iHappy < 0 && iHurryPopulation < -4*iHappy) // don't kill 4 citizens to remove 1 angry citizen.
				{
					if (gCityLogLevel >= 2)
						logBBAI("      City %S whips to reduce unhappiness", getName().GetCString() );
					hurry((HurryTypes)iI);
					return;
				}
			}
			else if (iHappy + iHappyDiff < 0)
			{
				continue; // not enough happiness to afford this hurry
			}

			if (iHappy + iHappyDiff >= 1 && iGoldCost == 0 && foodDifference() < -iHurryPopulation)
			{
				if (gCityLogLevel >= 2)
					logBBAI("      City %S whips to reduce food loss", getName().GetCString() );
				hurry((HurryTypes)iI);
				return;
			}

			iPopCost = AI_citizenSacrificeCost(iHurryPopulation, iHappy, GC.getDefineINT("HURRY_POP_ANGER"), iHurryAngerLength);
			iPopCost += std::max(0, 6 * -iHappyDiff) * iHurryAngerLength;

			if (kOwner.isHuman())
				iPopCost = iPopCost * 3 / 2;

			// subtract overflow from the cost; but only if we can be confident the city isn't being over-whipped. (iHappyDiff has been adjusted above based on the anger-timer)
			if (iHappyDiff > 0 || isNoUnhappiness() || (iHappy > 1 && (getHurryAngerTimer() <= 1 || iHurryAngerLength == 0)))
			{
				int iOverflow = (hurryProduction((HurryTypes)iI) - productionLeft()); // raw overflow
				iOverflow *= kOwner.AI_getFlavorValue(FLAVOR_PRODUCTION) > 0 ? 6 : 4; // value factor
				iOverflow *= getBaseYieldRateModifier(YIELD_PRODUCTION); // limit which multiplier apply to the overflow
				iOverflow /= std::max(1, getBaseYieldRateModifier(YIELD_PRODUCTION, getProductionModifier()));
				iPopCost -= iOverflow;
			}

			// convert units from 4x commerce to 1x commerce
			iPopCost /= 4;
		}

		int iTotalCost = iPopCost + iGoldCost;

		if (eProductionUnit != NO_UNIT)
		{
			const CvUnitInfo& kUnitInfo = GC.getUnitInfo(eProductionUnit);

			int iValue = 0;
			if (kOwner.AI_isFinancialTrouble())
				iTotalCost = std::max(0, iTotalCost); // overflow is not good when it is being used to build units that we don't want.
			else
			{
				iValue = productionLeft();
				switch (eProductionUnitAI)
				{
				case UNITAI_WORKER:
				case UNITAI_SETTLE:
				case UNITAI_WORKER_SEA:
					iValue *= kUnitInfo.isFoodProduction() ? 5 : 4;
					iValue += (eProductionUnitAI == UNITAI_SETTLE && area()->getNumAIUnits(getOwnerINLINE(), eProductionUnitAI) == 0
						? 24 : 14) * (getProductionTurnsLeft(eProductionUnit, 1)-1); // cf. value of commerce/turn
					break;
				case UNITAI_SETTLER_SEA:
				case UNITAI_EXPLORE_SEA:
					iValue *= kOwner.AI_getNumAIUnits(eProductionUnitAI) == 0 ? 5 : 4;
					break;
				default:
					if (kUnitInfo.getUnitCombatType() == NO_UNITCOMBAT)
						iValue *= 4;
					else
					{
						if (AI_isDanger())
						{
							iValue *= 6;
						}
						else if (kUnitInfo.getDomainType() == DOMAIN_SEA &&
							(area()->getAreaAIType(kOwner.getTeam()) == AREAAI_ASSAULT ||
							area()->getAreaAIType(kOwner.getTeam()) == AREAAI_ASSAULT_ASSIST ||
							area()->getAreaAIType(kOwner.getTeam()) == AREAAI_ASSAULT_MASSING))
						{
							iValue *= 5;
						}
						else
						{
							if (area()->getAreaAIType(kOwner.getTeam()) == AREAAI_DEFENSIVE)
							{
								int iSuccessRating = GET_TEAM(kOwner.getTeam()).AI_getWarSuccessRating();
								iValue *= iSuccessRating < -5 ? (iSuccessRating < -50 ? 6 : 5) : 4;
							}
							else if (kOwner.AI_isDoStrategy(AI_STRATEGY_CRUSH) &&
								(area()->getAreaAIType(kOwner.getTeam()) == AREAAI_OFFENSIVE || area()->getAreaAIType(kOwner.getTeam()) == AREAAI_MASSING))
							{
								int iSuccessRating = GET_TEAM(kOwner.getTeam()).AI_getWarSuccessRating();
								iValue *= iSuccessRating < 35 ? (iSuccessRating < 1 ? 6 : 5) : 4;
							}
							else
							{
								if (area()->getAreaAIType(kOwner.getTeam()) == AREAAI_NEUTRAL)
									iValue *= kOwner.AI_unitCostPerMil() >= kOwner.AI_maxUnitCostPerMil(area(), AI_buildUnitProb()) ? 0 : 3;
								else
									iValue *= 4;
							}
						}
					}
					break;
				}
				iValue /= 4 + std::max(0, -iHappyDiff);
			}
			if (iValue > iTotalCost)
			{
				if( gCityLogLevel >= 2 )
				{
					logBBAI("      City %S (%d) hurries %S. %d pop (%d) + %d gold (%d) to save %d turns. (value %d) (hd %d)",
						getName().GetCString(), getPopulation(), GC.getUnitInfo(eProductionUnit).getDescription(0), iHurryPopulation, iPopCost, iHurryGold, iGoldCost, getProductionTurnsLeft(eProductionUnit, 1), iValue, iHappyDiff);
				}

				hurry((HurryTypes)iI);
				return;
			}
		}
		else if (eProductionBuilding != NO_BUILDING)
		{
			const CvBuildingInfo& kBuildingInfo = GC.getBuildingInfo(eProductionBuilding);

			int iValue = AI_buildingValue(eProductionBuilding) * (getProductionTurnsLeft(eProductionBuilding, 1) - 1);
			iValue /= std::max(4, 4 - iHappyDiff) + (iHurryPopulation+1)/2;

			if (iValue > iTotalCost)
			{
				if( gCityLogLevel >= 2 )
				{
					logBBAI("      City %S (%d) hurries %S. %d pop (%d) + %d gold (%d) to save %d turns with %d building value (%d) (hd %d)",
						getName().GetCString(), getPopulation(), kBuildingInfo.getDescription(0), iHurryPopulation, iPopCost, iHurryGold, iGoldCost,
						getProductionTurnsLeft(eProductionBuilding, 1), AI_buildingValue(eProductionBuilding), iValue, iHappyDiff);
				}

				hurry((HurryTypes)iI);
				return;
			}
		}
	}
}


// Improved use of emphasize by Blake, to go with his whipping strategy - thank you!
void CvCityAI::AI_doEmphasize()
{
	PROFILE_FUNC();

	FAssert(!isHuman());

	bool bFirstTech;
	bool bEmphasize;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
	bool bCultureVictory = GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	//Note from Blake:
	//Emphasis proved to be too clumsy to manage AI economies,
	//as such it's been nearly completely phased out by 
	//the AI_specialYieldMultiplier system which allows arbitary
	//value-boosts and works very well.
	//Ideally the AI should never use emphasis.
	int iI;
	
	if (GET_PLAYER(getOwnerINLINE()).getCurrentResearch() != NO_TECH)
	{
		bFirstTech = GET_PLAYER(getOwnerINLINE()).AI_isFirstTech(GET_PLAYER(getOwnerINLINE()).getCurrentResearch());
	}
	else
	{
		bFirstTech = false;
	}

	int iPopulationRank = findPopulationRank();

	for (iI = 0; iI < GC.getNumEmphasizeInfos(); iI++)
	{
		bEmphasize = false;

		if (GC.getEmphasizeInfo((EmphasizeTypes)iI).getYieldChange(YIELD_FOOD) > 0)
		{

		}

		if (GC.getEmphasizeInfo((EmphasizeTypes)iI).getYieldChange(YIELD_PRODUCTION) > 0)
		{

		}
		
		if (AI_specialYieldMultiplier(YIELD_PRODUCTION) < 50)
		{
			if (GC.getEmphasizeInfo((EmphasizeTypes)iI).getYieldChange(YIELD_COMMERCE) > 0)
			{
				if (bFirstTech)
				{
					bEmphasize = true;
				}
			}

			if (GC.getEmphasizeInfo((EmphasizeTypes)iI).getCommerceChange(COMMERCE_RESEARCH) > 0)
			{
				if (bFirstTech && !bCultureVictory)
				{
					if (iPopulationRank < ((GET_PLAYER(getOwnerINLINE()).getNumCities() / 4) + 1))
					{
						bEmphasize = true;
					}
				}
			}
			
			if (GC.getEmphasizeInfo((EmphasizeTypes)iI).isGreatPeople())
			{
				int iHighFoodTotal = 0;
				int iHighFoodPlotCount = 0;
				int iHighHammerPlotCount = 0;
				int iHighHammerTotal = 0;
				int iGoodFoodSink = 0;
				int iFoodPerPop = GC.getFOOD_CONSUMPTION_PER_POPULATION();
				for (int iPlot = 0; iPlot < NUM_CITY_PLOTS; iPlot++)
				{
					CvPlot* pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iPlot);
					if (pLoopPlot != NULL && pLoopPlot->getWorkingCity() == this)
					{
						int iFood = pLoopPlot->getYield(YIELD_FOOD);
						if (iFood > iFoodPerPop)
						{
							iHighFoodTotal += iFood;
							iHighFoodPlotCount++;
						}
						int iHammers = pLoopPlot->getYield(YIELD_PRODUCTION);
						if ((iHammers >= 3) && ((iHammers + iFood) >= 4))
						{
							iHighHammerPlotCount++;
							iHighHammerTotal += iHammers;
						}
						int iCommerce = pLoopPlot->getYield(YIELD_COMMERCE);
						if ((iCommerce * 2 + iHammers * 3) > 9)
						{
							iGoodFoodSink += std::max(0, iFoodPerPop - iFood);
						}
					}
				}
				
				if ((iHighFoodTotal + iHighFoodPlotCount - iGoodFoodSink) >= foodConsumption(true))
				{
					if ((iHighHammerPlotCount < 2) && (iHighHammerTotal < (getPopulation())))
					{
						if (AI_countGoodTiles(true, false, 100, true) < getPopulation())
						{
							bEmphasize = true;
						}
					}
				}
			}
		}

		AI_setEmphasize(((EmphasizeTypes)iI), bEmphasize);
	}
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/09/10                                jdog5000      */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
bool CvCityAI::AI_chooseUnit(UnitAITypes eUnitAI, int iOdds)
{
	UnitTypes eBestUnit;

	if (eUnitAI != NO_UNITAI)
	{
		eBestUnit = AI_bestUnitAI(eUnitAI);
	}
	else
	{
		eBestUnit = AI_bestUnit(false, NO_ADVISOR, &eUnitAI);
	}

	if (eBestUnit != NO_UNIT)
	{
		/* original BBAI code
		if( iOdds < 0 ||
			getUnitProduction(eBestUnit) > 0 ||
			GC.getGameINLINE().getSorenRandNum(100, "City AI choose unit") < iOdds ) */
		// K-Mod. boost the odds based on our completion percentage.
		if (iOdds < 0 ||
			GC.getGameINLINE().getSorenRandNum(100, "City AI choose unit")
			< iOdds + 100 * getUnitProduction(eBestUnit) / std::max(1, getProductionNeeded(eBestUnit)))
		// K-Mod end
		{
			pushOrder(ORDER_TRAIN, eBestUnit, eUnitAI, false, false, false);
			return true;
		}
	}

	return false;
}

bool CvCityAI::AI_chooseUnit(UnitTypes eUnit, UnitAITypes eUnitAI)
{
	if (eUnit != NO_UNIT)
	{
		pushOrder(ORDER_TRAIN, eUnit, eUnitAI, false, false, false);
		return true;
	}
	return false;	
}


bool CvCityAI::AI_chooseDefender()
{
	if (plot()->plotCheck(PUF_isUnitAIType, UNITAI_CITY_SPECIAL, -1, getOwnerINLINE()) == NULL)
	{
		if (AI_chooseUnit(UNITAI_CITY_SPECIAL))
		{
			return true;
		}
	}

	if (plot()->plotCheck(PUF_isUnitAIType, UNITAI_CITY_COUNTER, -1, getOwnerINLINE()) == NULL)
	{
		if (AI_chooseUnit(UNITAI_CITY_COUNTER))
		{
			return true;
		}
	}

	if (AI_chooseUnit(UNITAI_CITY_DEFENSE))
	{
		return true;
	}

	return false;
}

bool CvCityAI::AI_chooseLeastRepresentedUnit(UnitTypeWeightArray &allowedTypes, int iOdds)
{
	int iValue;

	UnitTypeWeightArray::iterator it;
	
	/* original bts code
 	std::multimap<int, UnitAITypes, std::greater<int> > bestTypes;
 	std::multimap<int, UnitAITypes, std::greater<int> >::iterator best_it; */
	std::vector<std::pair<int, UnitAITypes> > bestTypes; // K-Mod
 
	for (it = allowedTypes.begin(); it != allowedTypes.end(); it++)
	{
		iValue = it->second;
		iValue *= 750 + GC.getGameINLINE().getSorenRandNum(250, "AI choose least represented unit");
		iValue /= 1 + GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(area(), it->first);
		//bestTypes.insert(std::make_pair(iValue, it->first));
		bestTypes.push_back(std::make_pair(iValue, it->first)); // K-Mod
	}
 
	// K-Mod
	std::sort(bestTypes.begin(), bestTypes.end(), std::greater<std::pair<int, UnitAITypes> >());
	std::vector<std::pair<int, UnitAITypes> >::iterator best_it;
	// K-Mod end
 	for (best_it = bestTypes.begin(); best_it != bestTypes.end(); best_it++)
 	{
		if (AI_chooseUnit(best_it->second, iOdds))
		{
			return true;
		}
 	}
	return false;
}

bool CvCityAI::AI_bestSpreadUnit(bool bMissionary, bool bExecutive, int iBaseChance, UnitTypes* eBestSpreadUnit, int* iBestSpreadUnitValue)
{
	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
	CvTeamAI& kTeam = GET_TEAM(getTeam());
	CvGame& kGame = GC.getGame();
	
	FAssert(eBestSpreadUnit != NULL && iBestSpreadUnitValue != NULL);

	int iBestValue = 0;

	if (bMissionary)
	{
		for (int iReligion = 0; iReligion < GC.getNumReligionInfos(); iReligion++)
		{
			ReligionTypes eReligion = (ReligionTypes)iReligion;
			if (isHasReligion(eReligion))
			{
				int iHasCount = kPlayer.getHasReligionCount(eReligion);
				FAssert(iHasCount > 0);
				int iRoll = (iHasCount > 4) ? iBaseChance : (((100 - iBaseChance) / iHasCount) + iBaseChance);
				if (kPlayer.AI_isDoStrategy(AI_STRATEGY_MISSIONARY))
				{
					iRoll *= (kPlayer.getStateReligion() == eReligion) ? 170 : 65;
					iRoll /= 100;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
				if (kPlayer.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				{
					iRoll += 25;
				}
				else if (!kTeam.hasHolyCity(eReligion) && !(kPlayer.getStateReligion() == eReligion))
				{
					iRoll /= 2;
					if (kPlayer.isNoNonStateReligionSpread())
					{
						iRoll /= 2;
					}
				}

				if (iRoll > kGame.getSorenRandNum(100, "AI choose missionary"))
				{
					int iReligionValue = kPlayer.AI_missionaryValue(area(), eReligion);
					if (iReligionValue > 0)
					{
						for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
						{
							UnitTypes eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

							if (eLoopUnit != NO_UNIT)
							{
								CvUnitInfo& kUnitInfo = GC.getUnitInfo(eLoopUnit);
								if (kUnitInfo.getReligionSpreads(eReligion) > 0)
								{
									if (canTrain(eLoopUnit))
									{
										int iValue = iReligionValue;
										iValue /= kUnitInfo.getProductionCost();
										
										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											*eBestSpreadUnit = eLoopUnit;
											*iBestSpreadUnitValue = iReligionValue;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	if (bExecutive)
	{
		for (int iCorporation = 0; iCorporation < GC.getNumCorporationInfos(); iCorporation++)
		{
			CorporationTypes eCorporation = (CorporationTypes)iCorporation;
			if (isActiveCorporation(eCorporation))
			{
				int iHasCount = kPlayer.getHasCorporationCount(eCorporation);
				FAssert(iHasCount > 0);
				int iRoll = (iHasCount > 4) ? iBaseChance : (((100 - iBaseChance) / iHasCount) + iBaseChance);
				if (!kTeam.hasHeadquarters(eCorporation))
				{
					iRoll /= 3; // was 8
				}
				
				if (iRoll > kGame.getSorenRandNum(100, "AI choose executive"))
				{
					int iCorporationValue = kPlayer.AI_executiveValue(area(), eCorporation);
					if (iCorporationValue > 0)
					{
						for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
						{
							UnitTypes eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

							if (eLoopUnit != NO_UNIT)
							{
								CvUnitInfo& kUnitInfo = GC.getUnitInfo(eLoopUnit);
								if (kUnitInfo.getCorporationSpreads(eCorporation) > 0)
								{
									if (canTrain(eLoopUnit))
									{
										int iValue = iCorporationValue;
										iValue /= kUnitInfo.getProductionCost();

										int iLoop;
										int iTotalCount = 0;
										int iPlotCount = 0;
										for (CvUnit* pLoopUnit = kPlayer.firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = kPlayer.nextUnit(&iLoop))
										{
											if ((pLoopUnit->AI_getUnitAIType() == UNITAI_MISSIONARY) && (pLoopUnit->getUnitInfo().getCorporationSpreads(eCorporation) > 0))
											{
												iTotalCount++;
												if (pLoopUnit->plot() == plot())
												{
													iPlotCount++;
												}
											}
										}
										iCorporationValue /= std::max(1, (iTotalCount / 4) + iPlotCount);

										int iCost = std::max(0, GC.getCorporationInfo(eCorporation).getSpreadCost() * (100 + GET_PLAYER(getOwnerINLINE()).calculateInflationRate()));
										iCost /= 100;

										if (kPlayer.getGold() >= iCost)
										{
											iCost *= GC.getDefineINT("CORPORATION_FOREIGN_SPREAD_COST_PERCENT");
											iCost /= 100;
											if (kPlayer.getGold() < iCost && iTotalCount > 1)
											{
												iCorporationValue /= 2;
											}
										}
										else if (iTotalCount > 1)
										{
											iCorporationValue /= 5;
										}
										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											*eBestSpreadUnit = eLoopUnit;
											*iBestSpreadUnitValue = iCorporationValue;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	return (*eBestSpreadUnit != NULL);
}

bool CvCityAI::AI_chooseBuilding(int iFocusFlags, int iMaxTurns, int iMinThreshold, int iOdds)
{
	BuildingTypes eBestBuilding;

	eBestBuilding = AI_bestBuildingThreshold(iFocusFlags, iMaxTurns, iMinThreshold);

	if (eBestBuilding != NO_BUILDING)
	{
		if( iOdds < 0 || 
			getBuildingProduction(eBestBuilding) > 0 ||
			GC.getGameINLINE().getSorenRandNum(100,"City AI choose building") < iOdds )
		{
			pushOrder(ORDER_CONSTRUCT, eBestBuilding, -1, false, false, false);
			return true;
		}
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


bool CvCityAI::AI_chooseProject()
{
	ProjectTypes eBestProject;

	eBestProject = AI_bestProject();

	if (eBestProject != NO_PROJECT)
	{
		pushOrder(ORDER_CREATE, eBestProject, -1, false, false, false);
		return true;
	}

	return false;
}


bool CvCityAI::AI_chooseProcess(CommerceTypes eCommerceType)
{
	ProcessTypes eBestProcess;

	eBestProcess = AI_bestProcess(eCommerceType);

	if (eBestProcess != NO_PROCESS)
	{
		pushOrder(ORDER_MAINTAIN, eBestProcess, -1, false, false, false);
		return true;
	}

	return false;
}


// Returns true if a worker was added to a plot...
bool CvCityAI::AI_addBestCitizen(bool bWorkers, bool bSpecialists, int* piBestPlot, SpecialistTypes* peBestSpecialist)
{
	PROFILE_FUNC();

	bool bAvoidGrowth = AI_avoidGrowth();
	bool bIgnoreGrowth = AI_ignoreGrowth();
	bool bIsSpecialistForced = false;

	int iBestSpecialistValue = 0;
	SpecialistTypes eBestSpecialist = NO_SPECIALIST;
	SpecialistTypes eBestForcedSpecialist = NO_SPECIALIST;

	if (bSpecialists)
	{
		// count the total forced specialists
		int iTotalForcedSpecialists = 0;
		for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			int iForcedSpecialistCount = getForceSpecialistCount((SpecialistTypes)iI);
			if (iForcedSpecialistCount > 0)
			{
				bIsSpecialistForced = true;
				iTotalForcedSpecialists += iForcedSpecialistCount;
			}
		}
		
		// if forcing any specialists, find the best one that we can still assign
		if (bIsSpecialistForced)
		{
			int iBestForcedValue = MIN_INT;
			
			int iTotalSpecialists = 1 + getSpecialistPopulation();
			for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
			{
				if (isSpecialistValid((SpecialistTypes)iI, 1))
				{
					int iForcedSpecialistCount = getForceSpecialistCount((SpecialistTypes)iI);
					if (iForcedSpecialistCount > 0)
					{
						int iSpecialistCount = getSpecialistCount((SpecialistTypes)iI);

						// the value is based on how close we are to our goal ratio forced/total
						int iForcedValue = ((iForcedSpecialistCount * 128) / iTotalForcedSpecialists) -  ((iSpecialistCount * 128) / iTotalSpecialists);
						if (iForcedValue >= iBestForcedValue)
						{
							int iSpecialistValue = AI_specialistValue((SpecialistTypes)iI, bAvoidGrowth, false);
							
							// if forced value larger, or if equal, does this specialist have a higher value
							if (iForcedValue > iBestForcedValue || iSpecialistValue > iBestSpecialistValue)
							{					
								iBestForcedValue = iForcedValue;
								iBestSpecialistValue = iSpecialistValue;
								eBestForcedSpecialist = ((SpecialistTypes)iI);
								eBestSpecialist = eBestForcedSpecialist;
							}
						}
					}
				}
			}
		}
		
		// if we do not have a best specialist yet, then just find the one with the best value
		if (eBestSpecialist == NO_SPECIALIST)
		{
			for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
			{
				if (isSpecialistValid((SpecialistTypes)iI, 1))
				{
					int iValue = AI_specialistValue(((SpecialistTypes)iI), bAvoidGrowth, false);
					if (iValue >= iBestSpecialistValue)
					{
						iBestSpecialistValue = iValue;
						eBestSpecialist = ((SpecialistTypes)iI);
					}
				}
			}
		}
	}

	int iBestPlotValue = 0;
	int iBestPlot = -1;
	if (bWorkers)
	{
		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			if (iI != CITY_HOME_PLOT)
			{
				if (!isWorkingPlot(iI))
				{
					CvPlot* pLoopPlot = getCityIndexPlot(iI);

					if (pLoopPlot != NULL)
					{
						if (canWork(pLoopPlot))
						{
							int iValue = AI_plotValue(pLoopPlot, bAvoidGrowth, /*bRemove*/ false, /*bIgnoreFood*/ false, bIgnoreGrowth);

							if (iValue > iBestPlotValue)
							{
								iBestPlotValue = iValue;
								iBestPlot = iI;
							}
						}
					}
				}
			}
		}
	}
	
	// if we found a plot to work
	if (iBestPlot != -1)
	{
		// if the best plot value is better than the best specialist, or if we forcing and we could not assign a forced specialst
		if (iBestPlotValue > iBestSpecialistValue || (bIsSpecialistForced && eBestForcedSpecialist == NO_SPECIALIST))
		{
			// do not work the specialist
			eBestSpecialist = NO_SPECIALIST;
		}
	}
	
	if (eBestSpecialist != NO_SPECIALIST)
	{
		changeSpecialistCount(eBestSpecialist, 1);
		if (piBestPlot != NULL)
		{
			FAssert(peBestSpecialist != NULL);
			*peBestSpecialist = eBestSpecialist;
			*piBestPlot = -1;
		}
		return true;
	}
	else if (iBestPlot != -1)
	{
		setWorkingPlot(iBestPlot, true);
		if (piBestPlot != NULL)
		{
			FAssert(peBestSpecialist != NULL);
			*peBestSpecialist = NO_SPECIALIST;
			*piBestPlot = iBestPlot;
			
		}
		return true;
	}

	return false;
}


// Returns true if a worker was removed from a plot...
bool CvCityAI::AI_removeWorstCitizen(SpecialistTypes eIgnoreSpecialist)
{
	CvPlot* pLoopPlot;
	SpecialistTypes eWorstSpecialist;
	bool bAvoidGrowth;
	bool bIgnoreGrowth;
	int iWorstPlot;
	int iValue;
	int iWorstValue;
	int iI;
	
	// if we are using more specialists than the free ones we get
	if (extraFreeSpecialists() < 0)
	{
		// does generic 'citizen' specialist exist?
		if (GC.getDefineINT("DEFAULT_SPECIALIST") != NO_SPECIALIST)
		{
			// is ignore something other than generic citizen?
			if (eIgnoreSpecialist != GC.getDefineINT("DEFAULT_SPECIALIST"))
			{
				// do we have at least one more generic citizen than we are forcing?
				if (getSpecialistCount((SpecialistTypes)(GC.getDefineINT("DEFAULT_SPECIALIST"))) > getForceSpecialistCount((SpecialistTypes)(GC.getDefineINT("DEFAULT_SPECIALIST"))))
				{
					// remove the extra generic citzen
					changeSpecialistCount(((SpecialistTypes)(GC.getDefineINT("DEFAULT_SPECIALIST"))), -1);
					return true;
				}
			}
		}
	}

	bAvoidGrowth = AI_avoidGrowth();
	bIgnoreGrowth = AI_ignoreGrowth();

	iWorstValue = MAX_INT;
	eWorstSpecialist = NO_SPECIALIST;
	iWorstPlot = -1;

	// if we are using more specialists than the free ones we get
	if (extraFreeSpecialists() < 0)
	{
		for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			if (eIgnoreSpecialist != iI)
			{
				if (getSpecialistCount((SpecialistTypes)iI) > getForceSpecialistCount((SpecialistTypes)iI))
				{
					iValue = AI_specialistValue(((SpecialistTypes)iI), bAvoidGrowth, /*bRemove*/ true);

					if (iValue < iWorstValue)
					{
						iWorstValue = iValue;
						eWorstSpecialist = ((SpecialistTypes)iI);
						iWorstPlot = -1;
					}
				}
			}
		}
	}
	
	// check all the plots we working
	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		if (iI != CITY_HOME_PLOT)
		{
			if (isWorkingPlot(iI))
			{
				pLoopPlot = getCityIndexPlot(iI);

				if (pLoopPlot != NULL)
				{
					iValue = AI_plotValue(pLoopPlot, bAvoidGrowth, /*bRemove*/ true, /*bIgnoreFood*/ false, bIgnoreGrowth);

					if (iValue < iWorstValue)
					{
						iWorstValue = iValue;
						eWorstSpecialist = NO_SPECIALIST;
						iWorstPlot = iI;
					}
				}
			}
		}
	}

	if (eWorstSpecialist != NO_SPECIALIST)
	{
		changeSpecialistCount(eWorstSpecialist, -1);
		return true;
	}
	else if (iWorstPlot != -1)
	{
		setWorkingPlot(iWorstPlot, false);
		return true;
	}
	
	// if we still have not removed one, then try again, but do not ignore the one we were told to ignore
	if (extraFreeSpecialists() < 0)
	{
		for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			if (getSpecialistCount((SpecialistTypes)iI) > 0)
			{
				iValue = AI_specialistValue(((SpecialistTypes)iI), bAvoidGrowth, /*bRemove*/ true);

				if (iValue < iWorstValue)
				{
					iWorstValue = iValue;
					eWorstSpecialist = ((SpecialistTypes)iI);
					iWorstPlot = -1;
				}
			}
		}
	}

	if (eWorstSpecialist != NO_SPECIALIST)
	{
		changeSpecialistCount(eWorstSpecialist, -1);
		return true;
	}

	return false;
}

// This function has been completely rewritten for K-Mod - original code deleted
void CvCityAI::AI_juggleCitizens()
{
	bool bAvoidGrowth = AI_avoidGrowth();
	bool bIgnoreGrowth = AI_ignoreGrowth();

	int iTotalFreeSpecialists = totalFreeSpecialists();

	// count the total forced specialists
	int iTotalForcedSpecialists = 0;
	bool bForcedSpecialists = false;
	for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		int iForcedSpecialistCount = getForceSpecialistCount((SpecialistTypes)iI);
		if (iForcedSpecialistCount > 0)
		{
			bForcedSpecialists = true;
			iTotalForcedSpecialists += iForcedSpecialistCount;
		}
	}

	bool bDone = false;
	int iCycles = 0;

	int iLatestPlot = -1;
	SpecialistTypes eLatestSpecialist = NO_SPECIALIST;

	do
	{
		int iWorkedPlot = -1;
		int iWorkedPlotValue = INT_MAX; // lowest value worked plot
		SpecialistTypes eWorkedSpecialist = NO_SPECIALIST;
		int iWorkedSpecValue = INT_MAX; // lowest value worked specialist

		int iUnworkedPlot = -1;
		int iUnworkedPlotValue = 0; // highest value unworked plot
		SpecialistTypes eUnworkedSpecialist = NO_SPECIALIST;
		int iUnworkedSpecValue = 0; // highest value unworked specialist
		int iUnworkedSpecForce = INT_MIN; // (force value can be negative)

		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			if (iI != CITY_HOME_PLOT)
			{
				CvPlot* pLoopPlot = getCityIndexPlot(iI);

				if (pLoopPlot != NULL)
				{
					if (isWorkingPlot(iI))
					{
						int iValue = AI_plotValue(pLoopPlot, bAvoidGrowth, true, false, bIgnoreGrowth);
						if (iValue < iWorkedPlotValue)
						{
							iWorkedPlot = iI;
							iWorkedPlotValue = iValue;
						}
					}
					else if (canWork(pLoopPlot))
					{
						int iValue = AI_plotValue(pLoopPlot, bAvoidGrowth, false, false, bIgnoreGrowth);
						if (iValue > iUnworkedPlotValue)
						{
							iUnworkedPlot = iI;
							iUnworkedPlotValue = iValue;
						}
					}
				}
			}
		}

		FAssert(!bForcedSpecialists || iTotalForcedSpecialists > 0); // otherwise we will divide by zero soon...

		for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			if (getSpecialistCount((SpecialistTypes)iI) > getForceSpecialistCount((SpecialistTypes)iI))
			{
				int iValue = AI_specialistValue(((SpecialistTypes)iI), bAvoidGrowth, true);
				if (iValue <= iWorkedSpecValue)
				{
					eWorkedSpecialist = (SpecialistTypes)iI;
					iWorkedSpecValue = iValue;
				}
			}
			if (isSpecialistValid((SpecialistTypes)iI, 1))
			{
				int iValue = AI_specialistValue(((SpecialistTypes)iI), bAvoidGrowth, false);
				int iForceValue = bForcedSpecialists ? getForceSpecialistCount((SpecialistTypes)iI) * 128 / iTotalForcedSpecialists - getSpecialistCount((SpecialistTypes)iI) * 128 / (getSpecialistPopulation()+1) : 0;
				if (iForceValue > iUnworkedSpecForce || (iForceValue >= iUnworkedSpecForce && iValue > iUnworkedSpecValue))
				{
					eUnworkedSpecialist = (SpecialistTypes)iI;
					iUnworkedSpecValue = iValue;
					iUnworkedSpecForce = iForceValue;
				}
			}
		}
		// if values are equal - prefer to work the specialist job. (perhaps a different city can use the plot)

		// don't let us reassign our free specialists onto plots
		if (iWorkedSpecValue < iWorkedPlotValue && iUnworkedPlotValue > iUnworkedSpecValue &&
			getSpecialistPopulation() <= iTotalFreeSpecialists)
		{
			iWorkedSpecValue = INT_MAX; // block spec removal
		}
		//
		if (std::max(iUnworkedPlotValue, iUnworkedSpecValue) > std::min(iWorkedPlotValue, iWorkedSpecValue))
		{
			// check to see if we're trying to remove the same job we most recently assigned
			if (iWorkedPlotValue <= iWorkedSpecValue ? iWorkedPlot == iLatestPlot : eWorkedSpecialist == eLatestSpecialist)
			{
				// ... that suggests we should break now to avoid getting into an endless loop.

				//FAssertMsg(false, "circuit break"); // (for testing)
				PROFILE("juggle citizen circuit break"); // testing... (I want to know how often this happens)

				// Note: what tends to happen is that when the city evaluates stopping work on a food tile,
				// it assumes that the citizen will go on to work some other 2-food plot.
				// That's usually a fair assumption, but it can sometimes cause loops.
				// The upshot is that we should try to break on the high-food tile.
				bDone = true;
				int iCurrentFood = iWorkedPlotValue <= iWorkedSpecValue
					? getCityIndexPlot(iWorkedPlot)->getYield(YIELD_FOOD)
					: GET_PLAYER(getOwnerINLINE()).specialistYield(eWorkedSpecialist, YIELD_FOOD);
				int iNextFood = iUnworkedPlotValue > iUnworkedSpecValue
					? getCityIndexPlot(iUnworkedPlot)->getYield(YIELD_FOOD)
					: GET_PLAYER(getOwnerINLINE()).specialistYield(eUnworkedSpecialist, YIELD_FOOD);
				if (iCurrentFood >= iNextFood)
					break; // ie. don't swap to the new job.
				// otherwise, take the new job and then we're done. (bDone == true)
			}

			// remove lowest value job
			if (iWorkedPlotValue <= iWorkedSpecValue)
			{
				FAssert(iWorkedPlot != -1);
				setWorkingPlot(iWorkedPlot, false);
			}
			else
			{
				FAssert(eWorkedSpecialist != NO_SPECIALIST);
				changeSpecialistCount(eWorkedSpecialist, -1);
			}

			// assign highest value job
			// remember which job we are assigning, to use in the above check on the next cycle.
			if (iUnworkedPlotValue > iUnworkedSpecValue)
			{
				FAssert(iUnworkedPlot != -1);
				setWorkingPlot(iUnworkedPlot, true);

				iLatestPlot = iUnworkedPlot;
				eLatestSpecialist  = NO_SPECIALIST;
			}
			else
			{
				FAssert(eUnworkedSpecialist != NO_SPECIALIST);
				changeSpecialistCount(eUnworkedSpecialist, 1);

				iLatestPlot = -1;
				eLatestSpecialist = eUnworkedSpecialist;
			}
		}
		else
			bDone = true;

		if (iCycles > getPopulation() + iTotalFreeSpecialists)
		{
			// This isn't a serious problem. I just want to know how offen it happens.
			//FAssertMsg(false, "juggle citizens failed to find a stable solution.");
			PROFILE("juggle citizen failure");
			break;
		}
		iCycles++;
	} while (!bDone);
}

// K-Mod. Estimate the cost of recovery after losing iQuantity number of citizens in this city.
// (units of 4x commerce.)
int CvCityAI::AI_citizenSacrificeCost(int iCitLoss, int iHappyLevel, int iNewAnger, int iAngerTimer)
{
	PROFILE_FUNC();

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	iCitLoss = std::min(getPopulation()-1, iCitLoss);
	if (iCitLoss < 1)
		return 0;

	if (isNoUnhappiness())
	{
		iNewAnger = 0;
		iAngerTimer = 0;
	}

	short iYields[NUM_YIELD_TYPES] = {};
	const short iYieldWeights[NUM_YIELD_TYPES] =
	{
		9, // food
		7, // production
		4, // commerce
	};
	std::vector<int> job_scores;
	int iTotalScore = 0;

	for (int i = 0; i < -iHappyLevel; i++)
		job_scores.push_back(0);

	if ((int)job_scores.size() < iCitLoss)
	{
		FAssert(CITY_HOME_PLOT == 0); // this is why the following loop starts at 1
		for (int i = 1; i < NUM_CITY_PLOTS; i++)
		{
			if (isWorkingPlot(i))
			{
				CvPlot* pLoopPlot = getCityIndexPlot(i);
				FAssert(pLoopPlot && pLoopPlot->getWorkingCity() == this);

				// ideally, the plots would be scored using AI_plotValue, but I'm worried that would be too slow.
				// so here's a really rough estimate of the plot value
				int iPlotScore = 0;
				for (int j = 0; j < NUM_YIELD_TYPES; j++)
				{
					int y = pLoopPlot->getYield((YieldTypes)j);
					iYields[j] += y;
					iPlotScore += y * iYieldWeights[j];
				}
				if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT && GC.getImprovementInfo(pLoopPlot->getImprovementType()).getImprovementUpgrade() != NO_IMPROVEMENT)
				{
					iYields[YIELD_COMMERCE] += 2;
					iPlotScore += 2 * iYieldWeights[YIELD_COMMERCE];
				}
				iTotalScore += iPlotScore;
				job_scores.push_back(iPlotScore);
			}
		}
	}
	if ((int)job_scores.size() < iCitLoss)
	{
		for (int i = 0; i < GC.getNumSpecialistInfos(); i++)
		{
			if (getSpecialistCount((SpecialistTypes)i) > 0)
			{
				// very rough..
				int iSpecScore = 0;
				for (int j = 0; j < NUM_YIELD_TYPES; j++)
				{
					int y = kOwner.specialistYield((SpecialistTypes)i, (YieldTypes)j)*3/2;
					iYields[j] += y;
					iSpecScore += y * iYieldWeights[j];
				}
				for (int j = 0 ; j < NUM_COMMERCE_TYPES; j++)
				{
					int c = kOwner.specialistCommerce((SpecialistTypes)i, (CommerceTypes)j)*3/2;
					iYields[YIELD_COMMERCE] += c;
					iSpecScore += c * iYieldWeights[YIELD_COMMERCE];
				}
				iTotalScore += iSpecScore * getSpecialistCount((SpecialistTypes)i);
				job_scores.resize(job_scores.size() + getSpecialistCount((SpecialistTypes)i), iSpecScore);
			}
		}
	}

	if ((int)job_scores.size() < iCitLoss)
	{
		FAssertMsg(false, "Not enough job data to calculate citizen loss cost.");
		int iBogusData = (1+GC.getFOOD_CONSUMPTION_PER_POPULATION()) * iYieldWeights[YIELD_FOOD];
		job_scores.resize(iCitLoss, iBogusData);
		iTotalScore += iBogusData;
	}

	FAssert((int)job_scores.size() >= iCitLoss);
	std::partial_sort(job_scores.begin(), job_scores.begin()+iCitLoss, job_scores.end());
	int iAverageScore = ROUND_DIVIDE(iTotalScore, job_scores.size());

	int iWastedFood = -healthRate();
	int iTotalFood = getYieldRate(YIELD_FOOD) - iWastedFood;

	// calculate the total cost-per-turn from missing iCitLoss citizens.
	int iBaseCostPerTurn = 0;
	for (int j = 0; j < NUM_YIELD_TYPES; j++)
	{
		if (iYields[j] > 0)
			iBaseCostPerTurn += 4 * iYields[j] * AI_yieldMultiplier((YieldTypes)j) * kOwner.AI_yieldWeight((YieldTypes)j) / 100;
	}
	// reduce value of food used for production - otherwise our citizens might be overvalued due to working extra food.
	if (isFoodProduction())
	{
		int iExtraFood = iTotalFood - getPopulation() * GC.getFOOD_CONSUMPTION_PER_POPULATION();
		iExtraFood = std::min(iExtraFood, iYields[YIELD_FOOD]*AI_yieldMultiplier(YIELD_FOOD)/100);
		iBaseCostPerTurn -= 4 * iExtraFood * (kOwner.AI_yieldWeight(YIELD_FOOD)-kOwner.AI_yieldWeight(YIELD_PRODUCTION)); // note that we're keeping a factor of 100.
	}
	//

	int iCost = 0; // accumulator for the final return value
	int iScoreLoss = 0;
	for (int i = 0; i < iCitLoss; i++)
	{
		// since the score estimate is very rough, I'm going to flatten it out a bit by combining it with the average score
		job_scores[i] = (2*job_scores[i] + iAverageScore + 2)/3;
		iScoreLoss += job_scores[i];
	}

	for (int i = iCitLoss; i > 0; i--)
	{
		int iFoodLoss = kOwner.getGrowthThreshold(getPopulation() - i) * (105 - getMaxFoodKeptPercent()) / 100;
		int iFoodRate = iTotalFood - (iScoreLoss * AI_yieldMultiplier(YIELD_FOOD) * iYields[YIELD_FOOD])/std::max(1, iTotalScore * 100);
		iFoodRate -= (getPopulation() - i) * GC.getFOOD_CONSUMPTION_PER_POPULATION();
		iFoodRate += std::min(iWastedFood, i);
		iFoodRate += iHappyLevel <= 0 ? i : 0; // if we're at the happiness cap, assume we've got some spare food that we're not currently working.

		int iRecoveryTurns = iFoodRate > 0 ? (iFoodLoss+iFoodRate-1) / iFoodRate : iFoodLoss;

		int iCostPerTurn = iBaseCostPerTurn;
		iCostPerTurn *= iScoreLoss;
		iCostPerTurn /= std::max(1, iTotalScore * 100);

		// extra food from reducing the population:
		iCostPerTurn -= 4 * (std::min(iWastedFood, i) + i*GC.getFOOD_CONSUMPTION_PER_POPULATION()) * kOwner.AI_yieldWeight(YIELD_FOOD)/100;
		iCostPerTurn += 2*i; // just a little bit of extra cost, to show that we care...

		FAssert(iRecoveryTurns > 0); // iCostPerTurn <= 0 is possible, but it should be rare

		// recovery isn't complete if the citizen is still angry
		iAngerTimer -= iRecoveryTurns;
		if (iAngerTimer > 0 && iHappyLevel + i - iNewAnger < 0)
		{
			iCost += iAngerTimer * GC.getFOOD_CONSUMPTION_PER_POPULATION() * 4 * kOwner.AI_yieldWeight(YIELD_FOOD) / 100;
			iRecoveryTurns += iAngerTimer;
		}

		iCost += iRecoveryTurns * iCostPerTurn;

		iScoreLoss -= job_scores[i-1];
	}
	// if there is any anger left over after regrowing, we might as well count some cost for that too.
	if (iAngerTimer > 0 && iHappyLevel - 1 - iNewAnger < 0)
	{
		int iGrowthTime = std::max(1, iTotalFood - getPopulation()*GC.getFOOD_CONSUMPTION_PER_POPULATION()); // just the food rate here
		iGrowthTime = (kOwner.getGrowthThreshold(getPopulation()) - getFood() + iGrowthTime - 1)/iGrowthTime; // now it's the time.

		iAngerTimer -= iGrowthTime;
		if (iAngerTimer > 0)
		{
			int iCostPerTurn = iBaseCostPerTurn;

			iCostPerTurn *= iAverageScore * (iNewAnger + 1 - iHappyLevel);
			iCostPerTurn /= std::max(1, iTotalScore * 100);

			iCostPerTurn += (iNewAnger + 1 - iHappyLevel) * GC.getFOOD_CONSUMPTION_PER_POPULATION() * 4 * kOwner.AI_yieldWeight(YIELD_FOOD) / 100;

			FAssert(iCostPerTurn > 0 && iAngerTimer > 0);

			iCost += iCostPerTurn * iAngerTimer;
		}
	}
	if (kOwner.AI_getFlavorValue(FLAVOR_GROWTH) > 0)
	{
		iCost *= 120 + kOwner.AI_getFlavorValue(FLAVOR_GROWTH);
		iCost /= 100;
	}

	return iCost;
}
// K-Mod end

bool CvCityAI::AI_potentialPlot(short* piYields) const
{
	int iNetFood = piYields[YIELD_FOOD] - GC.getFOOD_CONSUMPTION_PER_POPULATION();

	if (iNetFood < 0)
	{
 		if (piYields[YIELD_FOOD] == 0)
		{
			if (piYields[YIELD_PRODUCTION] + piYields[YIELD_COMMERCE] < 2)
			{
				return false;
			}
		}
		else
		{
			if (piYields[YIELD_PRODUCTION] + piYields[YIELD_COMMERCE] == 0)
			{
				return false;
			}
		}
	}

	return true;
}


bool CvCityAI::AI_foodAvailable(int iExtra) const
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	bool abPlotAvailable[NUM_CITY_PLOTS];
	int iFoodCount;
	int iPopulation;
	int iBestPlot;
	int iValue;
	int iBestValue;
	int iI;

	iFoodCount = 0;

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		abPlotAvailable[iI] = false;
	}

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		pLoopPlot = getCityIndexPlot(iI);

		if (pLoopPlot != NULL)
		{
			if (iI == CITY_HOME_PLOT)
			{
				iFoodCount += pLoopPlot->getYield(YIELD_FOOD);
			}
			else if ((pLoopPlot->getWorkingCity() == this) && AI_potentialPlot(pLoopPlot->getYield()))
			{
				abPlotAvailable[iI] = true;
			}
		}
	}

	iPopulation = (getPopulation() + iExtra);

	while (iPopulation > 0)
	{
		iBestValue = 0;
		iBestPlot = CITY_HOME_PLOT;

		for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			if (abPlotAvailable[iI])
			{
				iValue = getCityIndexPlot(iI)->getYield(YIELD_FOOD);

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					iBestPlot = iI;
				}
			}
		}

		if (iBestPlot != CITY_HOME_PLOT)
		{
			iFoodCount += iBestValue;
			abPlotAvailable[iBestPlot] = false;
		}
		else
		{
			break;
		}

		iPopulation--;
	}

	for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		iFoodCount += (GC.getSpecialistInfo((SpecialistTypes)iI).getYieldChange(YIELD_FOOD) * getFreeSpecialistCount((SpecialistTypes)iI));
	}

	if (iFoodCount < foodConsumption(false, iExtra))
	{
		return false;
	}

	return true;
}


// K-Mod: I've rewritten large chunks of this function. I've deleted much of the original code, rather than commenting it; just to keep things a bit tidier and clearer.
// Note. I've changed the scale to match the building evaluation code: ~4x commerce.
int CvCityAI::AI_yieldValue(short* piYields, short* piCommerceYields, bool bAvoidGrowth, bool bRemove, bool bIgnoreFood, bool bIgnoreGrowth, bool bIgnoreStarvation, bool bWorkerOptimization) const
{
	PROFILE_FUNC();
	const int iBaseProductionValue = 7;
	const int iBaseCommerceValue = 4;

	bool bEmphasizeFood = AI_isEmphasizeYield(YIELD_FOOD);
	bool bFoodIsProduction = isFoodProduction();
	bool bCanPopRush = GET_PLAYER(getOwnerINLINE()).canPopRush();

	int iBaseProductionModifier = getBaseYieldRateModifier(YIELD_PRODUCTION);
	int iExtraProductionModifier = getProductionModifier();
	int iProductionTimes100 = piYields[YIELD_PRODUCTION] * (iBaseProductionModifier + iExtraProductionModifier);
	int iCommerceYieldTimes100 = piYields[YIELD_COMMERCE] * getBaseYieldRateModifier(YIELD_COMMERCE);
	int iFoodYieldTimes100 = piYields[YIELD_FOOD] * getBaseYieldRateModifier(YIELD_FOOD);
	int iFoodYield = iFoodYieldTimes100/100;

	int iValue = 0; // total value

	// Commerce
	int iCommerceValue = 0;
	ProcessTypes eProcess = getProductionProcess();

	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		int iCommerceTimes100 = iCommerceYieldTimes100 * GET_PLAYER(getOwnerINLINE()).getCommercePercent((CommerceTypes)iI) / 100;
		if (piCommerceYields != NULL)
		{
			iCommerceTimes100 += piCommerceYields[iI] * 100;
		}

		iCommerceTimes100 *= getTotalCommerceRateModifier((CommerceTypes)iI);
		iCommerceTimes100 /= 100;

		if (eProcess != NO_PROCESS)
			iCommerceTimes100 += GC.getProcessInfo(getProductionProcess()).getProductionToCommerceModifier(iI) * iProductionTimes100 / 100;

		if (iCommerceTimes100 != 0)
		{
			int iCommerceWeight = GET_PLAYER(getOwnerINLINE()).AI_commerceWeight((CommerceTypes)iI, this);
			if (AI_isEmphasizeCommerce((CommerceTypes)iI))
			{
				iCommerceWeight *= 2;
			}
			if (iI == COMMERCE_CULTURE && getCultureLevel() <= (CultureLevelTypes)1)
			{
				// bring on the artists
				if (getCommerceRateTimes100(COMMERCE_CULTURE) - (bRemove ? iCommerceTimes100 : 0) < 100)
				{
					iCommerceValue += 20;
				}
				iCommerceWeight = std::max(iCommerceWeight, 200);
			}

			iCommerceValue += iCommerceWeight * iCommerceTimes100 * iBaseCommerceValue * GET_PLAYER(getOwnerINLINE()).AI_averageCommerceExchange((CommerceTypes)iI) / 1000000;
		}
	}
	//

	// Production
	int iProductionValue = 0;
	if (eProcess == NO_PROCESS)
	{
		iProductionTimes100 += bFoodIsProduction ? iFoodYieldTimes100 : 0;

		iProductionValue += iProductionTimes100 * iBaseProductionValue / 100;
		// If city has more than enough food, but very little production, add large value to production
		// Particularly helps coastal cities with plains forests
		// (based on BBAI code)
		if (!bWorkerOptimization && iProductionTimes100 > 0 && !bFoodIsProduction && isProduction())
		{
			if (!isHuman() || AI_isEmphasizeYield(YIELD_PRODUCTION)
				|| (!bEmphasizeFood && !AI_isEmphasizeYield(YIELD_COMMERCE) && !AI_isEmphasizeGreatPeople()))
			{
				if (foodDifference(false) >= GC.getFOOD_CONSUMPTION_PER_POPULATION())
				{
					/*if (getYieldRate(YIELD_PRODUCTION) - (bRemove ? iProductionTimes100/100 : 0)  < 1 + getPopulation()/3)
					{
						iValue += 60 + iBaseProductionValue * iProductionTimes100 / 100;
					}*/
					int iCurrentProduction = getYieldRate(YIELD_PRODUCTION) - (bRemove ? iProductionTimes100/100 : 0);
					if (iCurrentProduction < 1 + getPopulation()/3)
					{
						iValue += 5 * iBaseProductionValue * std::min(iProductionTimes100, (1 + getPopulation()/3 - iCurrentProduction)*100) / 100;
					}
				}
			}
		}
		if (!isProduction() && !isHuman())
		{
			iProductionValue /= 2;
		}
	}
	//

	int iSlaveryValue = 0;

	int iFoodGrowthValue = 0;
	int iFoodGPPValue = 0;

	if (!bIgnoreFood && iFoodYield > 0)
	{
		// tiny food factor, to ensure that even when we don't want to grow, 
		// we still prefer more food if everything else is equal
		iValue += bFoodIsProduction ? 0 : (iFoodYieldTimes100+50)/100;

		int iConsumtionPerPop = GC.getFOOD_CONSUMPTION_PER_POPULATION();
		int iFoodPerTurn = getYieldRate(YIELD_FOOD) - foodConsumption() - (bRemove? iFoodYield : 0);
		int iFoodLevel = getFood();
		int iFoodToGrow = growthThreshold();
		int iHealthLevel = goodHealth() - badHealth();
		int iHappinessLevel = (isNoUnhappiness() ? std::max(3, iHealthLevel + 5) : happyLevel() - unhappyLevel(0));
		int iPopulation = getPopulation();
		//int iExtraPopulationThatCanWork = std::min(iPopulation - range(-iHappinessLevel, 0, iPopulation) + std::min(0, extraFreeSpecialists()) , NUM_CITY_PLOTS) - getWorkingPopulation() + (bRemove ? 1 : 0);
		int iExtraPopulationThatCanWork = std::min(NUM_CITY_PLOTS-1 - getWorkingPopulation(), std::max(0, extraPopulation()+(bRemove?1:0)));
		bool bReassign = extraPopulation() <= 0;

		int iAdjustedFoodDifference = getYieldRate(YIELD_FOOD) - (bRemove? iFoodYield : 0) + std::min(0, iHealthLevel) - (iPopulation + std::min(0, iHappinessLevel)) * iConsumtionPerPop;

		// if we not human, allow us to starve to half full if avoiding growth
		if (!bIgnoreStarvation)
		{
			int iStarvingAllowance = 0;
			if (bAvoidGrowth && !isHuman())
			{
				iStarvingAllowance = std::max(0, (iFoodLevel - std::max(1, ((9 * iFoodToGrow) / 10))));
			}

			if (iStarvingAllowance < 1 && iFoodLevel > iFoodToGrow * 75 / 100 && (iHappinessLevel < 1 || iHealthLevel < 1))
			{
				iStarvingAllowance = 1;
			}

			// if still starving
			if (iFoodPerTurn + (bRemove ? std::min(iFoodYield, iConsumtionPerPop) : 0) + iStarvingAllowance < 0)
			{
				// if working plots all like this one will save us from starving
				if (((bReassign?1:0)+iExtraPopulationThatCanWork+std::max(0, getSpecialistPopulation() - totalFreeSpecialists())) * iFoodYield >= -iFoodPerTurn)
				{
					// if this is high food, then we want to pick it first, this will allow us to pick some great non-food later
					/*int iHighFoodThreshold = std::min(getBestYieldAvailable(YIELD_FOOD), iConsumtionPerPop + 1);				
					if (iFoodPerTurn <= (AI_isEmphasizeGreatPeople() ? 0 : -iHighFoodThreshold) && iFoodYield >= iHighFoodThreshold)
					{
						// value all the food that will contribute to not starving
						iValue += 2048 * std::min(iFoodYield, -iFoodPerTurn);
					}
					else */
					{
						// give a huge boost to this plot, but not based on how much food it has
						// ie, if working a bunch of 1f 7h plots will stop us from starving, then do not force working unimproved 2f plot
						iValue += 2048;
					}
				}
				//else
				{
					// value food high, but not forced
					iValue += 36 * std::min(iFoodYield, -iFoodPerTurn+(bReassign ? iConsumtionPerPop : 0));
				}
			}
		}

		// if food isn't production, then adjust for growth
		if (bWorkerOptimization || !bFoodIsProduction)
		{
			int iPopToGrow = 0;
			if (!bAvoidGrowth)
			{
				int iFutureHappy = 0; // K-Mod. Happiness boost we expect before we grow. (originally, the boost was added directly to iHappyLevel)

				// only do relative checks on food if we want to grow AND we not emph food
				// the emp food case will just give a big boost to all food under all circumstances
				if (bWorkerOptimization || (!bIgnoreGrowth))// && !bEmphasizeFood))
				{
					// we have less than 10 extra happy, do some checks to see if we can increase it
					//if (iHappinessLevel < 10)
					if (iHappinessLevel < 5) // K-Mod. make it 5. We shouldn't really be worried about growing 10 pop all at once...
					{
						// if we have anger becase no military, do not count it, on the assumption that it will 
						// be remedied soon, and that we still want to grow
						if (getMilitaryHappinessUnits() == 0)
						{
							if (GET_PLAYER(getOwnerINLINE()).getNumCities() > 2)
							{
								iHappinessLevel += ((GC.getDefineINT("NO_MILITARY_PERCENT_ANGER") * (iPopulation + 1)) / GC.getPERCENT_ANGER_DIVISOR());
							}
						}

						const int kMaxHappyIncrease = 2;

						// if happy is large enough so that it will be over zero after we do the checks
						if (iHappinessLevel + kMaxHappyIncrease > 0)
						{
							int iNewFoodPerTurn = iFoodPerTurn + iFoodYield;
							int iApproxTurnsToGrow = (iNewFoodPerTurn > 0) ? ((iFoodToGrow - iFoodLevel + iNewFoodPerTurn-1) / iNewFoodPerTurn) : MAX_INT;

							// do we have hurry anger?
							int iHurryAngerTimer = getHurryAngerTimer();
							if (iHurryAngerTimer > 0)
							{
								int iTurnsUntilAngerIsReduced = iHurryAngerTimer % flatHurryAngerLength();
								
								// angry population is bad but if we'll recover by the time we grow...
								if (iTurnsUntilAngerIsReduced <= iApproxTurnsToGrow)
								{
									iFutureHappy++;
								}
							}

							// do we have conscript anger?
							int iConscriptAngerTimer = getConscriptAngerTimer();
							if (iConscriptAngerTimer > 0)
							{
								int iTurnsUntilAngerIsReduced = iConscriptAngerTimer % flatConscriptAngerLength();
								
								// angry population is bad but if we'll recover by the time we grow...
								if (iTurnsUntilAngerIsReduced <= iApproxTurnsToGrow)
								{
									iFutureHappy++;
								}
							}

							// do we have defy resolution anger?
							int iDefyResolutionAngerTimer = getDefyResolutionAngerTimer();
							if (iDefyResolutionAngerTimer > 0)
							{
								int iTurnsUntilAngerIsReduced = iDefyResolutionAngerTimer % flatDefyResolutionAngerLength();

								// angry population is bad but if we'll recover by the time we grow...
								if (iTurnsUntilAngerIsReduced <= iApproxTurnsToGrow)
								{
									iFutureHappy++;
								}
							}
						}
					}

					if (bEmphasizeFood)
					{
						//If we are emphasize food, pay less heed to caps.
						iHealthLevel += 5;
						iFutureHappy += 2;
					}

					// approximate the food that can be gained by working other plots (and refund the food removed at the start)
					iFoodPerTurn += iExtraPopulationThatCanWork * std::min(iConsumtionPerPop, iFoodYield);
					iAdjustedFoodDifference += iExtraPopulationThatCanWork * std::min(iConsumtionPerPop, iFoodYield);

					bool bBarFull = (iFoodLevel + iFoodPerTurn /*+ aiYields[YIELD_FOOD]*/ > ((90 * iFoodToGrow) / 100));

					int iPopToGrow = std::max(0, iHappinessLevel+iFutureHappy);
					int iGoodTiles = AI_countGoodTiles(iHealthLevel > 0, true, 50, true);
					iGoodTiles += AI_countGoodSpecialists(iHealthLevel > 0);
					//iGoodTiles += bBarFull ? 0 : 1;

					if (!bEmphasizeFood)
					{
						//iPopToGrow = std::min(iPopToGrow, iGoodTiles + ((bRemove) ? 1 : 0));
						iPopToGrow = std::min(iPopToGrow, iGoodTiles + 1); // testing
					}

					// if we have growth pontential, fill food bar to 85%
					bool bFillingBar = false;
					if (iPopToGrow == 0 && iHappinessLevel >= 0 && iGoodTiles >= 0 && iHealthLevel >= 0)
					{
						if (!bBarFull)
						{
							if (AI_specialYieldMultiplier(YIELD_PRODUCTION) < 50)
							{
								bFillingBar = true;
							}
						}
					}

					/*if (getPopulation() < 3)
					{
						iPopToGrow = std::max(iPopToGrow, 3 - getPopulation());
						iPopToGrow += 2;
					}*/

					// if we want to grow
					// don't count growth value for tiles with nothing else to contribute.
					if ((iPopToGrow > 0 || bFillingBar) && (iFoodYield - iConsumtionPerPop > 0 || iProductionValue > 0 || iCommerceValue > 0))
					{
						// will multiply this by factors
						iFoodGrowthValue = iFoodYield;
						/*if (iHealthLevel < (bFillingBar ? 0 : 1))
						{
							iFoodGrowthValue--;
						}*/ // moved lower.

						// (range 1-25) - we want to grow more if we have a lot of growth to do
						// factor goes up like this: 0:1, 1:8, 2:9, 3:10, 4:11, 5:13, 6:14, 7:15, 8:16, 9:17, ... 17:25
						int iFactorPopToGrow;

						/* original bts code
						if (iPopToGrow < 1 || bFillingBar)
							iFactorPopToGrow = 20 - (10 * (iFoodLevel + iFoodPerTurn + aiYields[YIELD_FOOD])) / iFoodToGrow;
						else if (iPopToGrow < 7)
							iFactorPopToGrow = 17 + 3 * iPopToGrow;
						else
							iFactorPopToGrow = 41; */
						// K-Mod, reduced the scale to match the building evaluation code.
						if (bFillingBar)
							iFactorPopToGrow = 11 * iFoodToGrow / std::max(1, iFoodToGrow + 2*iFoodLevel + iFoodPerTurn);
						else if (iPopToGrow < 6)
							iFactorPopToGrow = 11 + 2 * iPopToGrow;
						else
							iFactorPopToGrow = 23;

						if (iHealthLevel < (bFillingBar ? 0 : 1))
						{
							iFactorPopToGrow = iFactorPopToGrow * iFoodYield / (iFoodYield + 1);
						}
						// K-Mod end

						iFoodGrowthValue *= iFactorPopToGrow;

						//If we already grow somewhat fast, devalue further food
						//Remember growth acceleration is not dependent on food eaten per 
						//pop, 4f twice as fast as 2f twice as fast as 1f...
						/*int iHighGrowthThreshold = 2 + std::max(std::max(0, 5 - getPopulation()), (iPopToGrow + 1) / 2);
						if (bEmphasizeFood)
						{
							iHighGrowthThreshold *= 2;
						}
						
						if (iFoodPerTurn > iHighGrowthThreshold)
						{
							iFoodGrowthValue *= 25 + (75 * iHighGrowthThreshold) / iFoodPerTurn;
							iFoodGrowthValue /= 100;
						}*/
					}
				}

				//very high food override
				/* original bts code
				if ((isHuman()) && ((iPopToGrow > 0) || bCanPopRush))
				{
					//very high food override
					int iTempValue = std::max(0, 30 * aiYields[YIELD_FOOD] - 15 * iConsumtionPerPop);
					iTempValue *= std::max(0, 3 * iConsumtionPerPop - iAdjustedFoodDifference);
					iTempValue /= 3 * iConsumtionPerPop;
					if (iHappinessLevel < 0)
					{
						iTempValue *= 2;
						iTempValue /= 1 + 2 * -iHappinessLevel;
					}
					iFoodGrowthValue += iTempValue;
				} */ // Disabled by K-Mod. I don't see the point of this.
				//Slavery Override
				//if (bCanPopRush && (iHappinessLevel > 0))
				if (bCanPopRush && getHurryAngerTimer() <= std::max(6-getPopulation(), 0)
					&& iHappinessLevel >= (getPopulation()%2 == 0 ? 1 : 0)) // K-Mod
				{
					//iSlaveryValue = 30 * 14 * std::max(0, aiYields[YIELD_FOOD] - ((iHealthLevel < 0) ? 1 : 0));
					// K-Mod. Rescaled values. "30" represents GC.getHurryInfo(eHurry).getProductionPerPopulation()
					iSlaveryValue = 30 * iBaseProductionValue * std::max(0, iFoodYield - ((iHealthLevel < 0) ? 1 : 0)); // K-Mod
					// K-Mod end
					iSlaveryValue /= std::max(10, (growthThreshold() * (100 - getMaxFoodKeptPercent())));

					iSlaveryValue *= 100;
					iSlaveryValue /= getHurryCostModifier(true);

					iSlaveryValue *= iConsumtionPerPop * 2;
					iSlaveryValue /= iConsumtionPerPop * 2 + std::max(0, iAdjustedFoodDifference);
				}

				//Great People Override
				if ((iExtraPopulationThatCanWork > 1) && AI_isEmphasizeGreatPeople())
				{
					int iAdjust = iConsumtionPerPop;
					if (iFoodPerTurn <= 0)
					{
						iAdjust -= 1;
					}
					//iFoodGPPValue += std::max(0, aiYields[YIELD_FOOD] - iAdjust) * std::max(0, (12 + 5 * std::min(0, iHappinessLevel)));
					// K-Mod, rescaling
					iFoodGPPValue += std::max(0, iFoodYield - iAdjust) * std::max(0, 3 * iBaseCommerceValue * (2 + std::min(0, iHappinessLevel+iFutureHappy))/2);
					// K-Mod end
				}
			}
		}
	}


	int iMaxFoodValue = 3 * (eProcess == NO_PROCESS ? iBaseProductionValue : iBaseCommerceValue) - 1;
	int iFoodValue = std::min(iFoodGrowthValue, iMaxFoodValue * iFoodYield);

	//Slavery translation
	if ((iSlaveryValue > 0) && (iSlaveryValue > iFoodValue))
	{
		//treat the food component as production
		iFoodValue = 0;
	}
	else
	{
		//treat it as just food
		iSlaveryValue = 0;
	}
	
	iFoodValue += iFoodGPPValue;

	//Lets have some fun with the multipliers, this basically bluntens the impact of
	//massive bonuses.....
	
	//normalize the production... this allows the system to account for rounding
	//and such while preventing an "out to lunch smoking weed" scenario with
	//unusually high transient production modifiers.
	//Other yields don't have transient bonuses in quite the same way.

	// Rounding can be a problem, particularly for small commerce amounts.  Added safe guards to make
	// sure commerce is counted, even if just a tiny amount.
	if (AI_isEmphasizeYield(YIELD_PRODUCTION))
	{
		iProductionValue *= 140; // was 130
		iProductionValue /= 100;
		
		if (isFoodProduction())
		{
			iFoodValue *= 140; // was 130
			iFoodValue /= 100;
		}
	}
	else if (iProductionValue > 0 && AI_isEmphasizeYield(YIELD_COMMERCE))
	{
		iProductionValue *= 75;
		iProductionValue /= 100;
		iProductionValue = std::max(1,iProductionValue);
	}

	if (AI_isEmphasizeYield(YIELD_FOOD))
	{
		if (!isFoodProduction())
		{
			iFoodValue *= 130;
			iFoodValue /= 100;
			iSlaveryValue *= 130;
			iSlaveryValue /= 100;
		}
	}
	else if (iFoodValue > 0 && (AI_isEmphasizeYield(YIELD_PRODUCTION) || AI_isEmphasizeYield(YIELD_COMMERCE)))
	{
		iFoodValue *= 75; // was 75 for production, and 80 for commerce. (now same for both.)
		iFoodValue /= 100;
		iFoodValue = std::max(1, iFoodValue);
	}

	if (AI_isEmphasizeYield(YIELD_COMMERCE))
	{
		iCommerceValue *= 140; // was 130
		iCommerceValue /= 100;
	}
	else if (iCommerceValue > 0 && AI_isEmphasizeYield(YIELD_PRODUCTION))
	{
		iCommerceValue *= 75; // was 60
		iCommerceValue /= 100;
		iCommerceValue = std::max(1, iCommerceValue);
	}
		
	if( iProductionValue > 0 )
	{
		if (isFoodProduction())
		{
			iProductionValue *= 100 + (bWorkerOptimization ? 0 : AI_specialYieldMultiplier(YIELD_PRODUCTION));
			iProductionValue /= 100;
		}
		else
		{
			iProductionValue *= iBaseProductionModifier;
			iProductionValue /= (iBaseProductionModifier + iExtraProductionModifier);

			iProductionValue += iSlaveryValue;
			iProductionValue *= (100 + (bWorkerOptimization ? 0 : AI_specialYieldMultiplier(YIELD_PRODUCTION)));

			iProductionValue /= GET_PLAYER(getOwnerINLINE()).AI_averageYieldMultiplier(YIELD_PRODUCTION);
		}

		iValue += std::max(1,iProductionValue);
	}

	if( iCommerceValue > 0 )
	{
		iCommerceValue *= (100 + (bWorkerOptimization ? 0 : AI_specialYieldMultiplier(YIELD_COMMERCE)));
		iCommerceValue /= GET_PLAYER(getOwnerINLINE()).AI_averageYieldMultiplier(YIELD_COMMERCE);
		iValue += std::max(1, iCommerceValue);
	}
//
	if( iFoodValue > 0 )
	{
		iFoodValue *= 100;
		iFoodValue /= GET_PLAYER(getOwnerINLINE()).AI_averageYieldMultiplier(YIELD_FOOD);
		iValue += std::max(1, iFoodValue);
	}

	return iValue;
}

int CvCityAI::AI_plotValue(CvPlot* pPlot, bool bAvoidGrowth, bool bRemove, bool bIgnoreFood, bool bIgnoreGrowth, bool bIgnoreStarvation) const
{
	PROFILE_FUNC();

	short aiYields[NUM_YIELD_TYPES];
	ImprovementTypes eCurrentImprovement;
	ImprovementTypes eFinalImprovement;
	int iYieldDiff;
	int iValue;
	int iI;
	int iTotalDiff;

	iValue = 0;
	iTotalDiff = 0;

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		aiYields[iI] = pPlot->getYield((YieldTypes)iI);
	}

	eCurrentImprovement = pPlot->getImprovementType();
	eFinalImprovement = NO_IMPROVEMENT;

	if (eCurrentImprovement != NO_IMPROVEMENT)
	{
		eFinalImprovement = finalImprovementUpgrade(eCurrentImprovement);
	}

	int iYieldValue = (AI_yieldValue(aiYields, NULL, bAvoidGrowth, bRemove, bIgnoreFood, bIgnoreGrowth, bIgnoreStarvation) * 100);

	//if (eFinalImprovement != NO_IMPROVEMENT)
	if (eFinalImprovement != NO_IMPROVEMENT && eFinalImprovement != eCurrentImprovement) // K-Mod
	{
		for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			iYieldDiff = (pPlot->calculateImprovementYieldChange(eFinalImprovement, ((YieldTypes)iI), getOwnerINLINE()) - pPlot->calculateImprovementYieldChange(eCurrentImprovement, ((YieldTypes)iI), getOwnerINLINE()));
			aiYields[iI] += iYieldDiff;
		}
		int iFinalYieldValue = (AI_yieldValue(aiYields, NULL, bAvoidGrowth, bRemove, bIgnoreFood, bIgnoreGrowth, bIgnoreStarvation) * 100);
		
		if (iFinalYieldValue > iYieldValue)
		{
			iYieldValue = (40 * iYieldValue + 60 * iFinalYieldValue) / 100;
		}
		else
		{
			iYieldValue = (60 * iYieldValue + 40 * iFinalYieldValue) / 100;
		}
	}
	// unless we are emph food (and also food not production)
	if (!(AI_isEmphasizeYield(YIELD_FOOD) && !isFoodProduction()))
		// if this plot is super bad (less than 2 food and less than combined 2 prod/commerce
		if (!AI_potentialPlot(aiYields))
			// undervalue it even more!
			iYieldValue /= 16;
	iValue += iYieldValue;

	if (eCurrentImprovement != NO_IMPROVEMENT)
	{
		if (pPlot->getBonusType(getTeam()) == NO_BONUS) // XXX double-check CvGame::doFeature that the checks are the same...
		{
			for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
			{
				if (GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getBonusInfo((BonusTypes) iI).getTechReveal())))
				{
					if (GC.getImprovementInfo(eCurrentImprovement).getImprovementBonusDiscoverRand(iI) > 0)
					{
						//iValue += 35;
						iValue += 20; // K-Mod (rescaling to match new yield values)
					}
				}
			}
		}
	}

	if ((eCurrentImprovement != NO_IMPROVEMENT) && (GC.getImprovementInfo(pPlot->getImprovementType()).getImprovementUpgrade() != NO_IMPROVEMENT))
	{
		/* original bts code
		iValue += 200;
		iValue -= pPlot->getUpgradeTimeLeft(eCurrentImprovement, NO_PLAYER); */
		// K-Mod. Note: this is still just an ugly kludge, just rescaled to match the new yield value.
		// (the goal of this is to prefer plots that are close to upgrading, but not over immediate yield differences.)
		iValue += 100 * pPlot->getUpgradeProgress() / std::max(1, GC.getGameINLINE().getImprovementUpgradeTime(eCurrentImprovement));
		// K-Mod
	}

	return iValue;
}


int CvCityAI::AI_experienceWeight()
{
	//return ((getProductionExperience() + getDomainFreeExperience(DOMAIN_SEA)) * 2);
	// K-Mod
	return 2 * getProductionExperience() + getDomainFreeExperience(DOMAIN_LAND) + getDomainFreeExperience(DOMAIN_SEA);
	// K-Mod end
}


// BBAI / K-Mod
int CvCityAI::AI_buildUnitProb()
{
	int iProb;
	iProb = (GC.getLeaderHeadInfo(getPersonalityType()).getBuildUnitProb() + AI_experienceWeight());

	if (!isBarbarian() && GET_PLAYER(getOwnerINLINE()).AI_isFinancialTrouble())
	{
		iProb /= 2;
	}
	
	if( GET_TEAM(getTeam()).getHasMetCivCount(false) == 0 )
	{
		iProb /= 2;
	}

	if (GET_PLAYER(getOwnerINLINE()).getCurrentEra() < GC.getGameINLINE().getCurrentEra())
	{
		iProb *= std::max(40, 100 - 20 * (GC.getGameINLINE().getCurrentEra() - GET_PLAYER(getOwnerINLINE()).getCurrentEra()));
		iProb /= 100;
	}

	iProb *= (100 + 2*getMilitaryProductionModifier());
	iProb /= 100;
	//return iProb;
	return std::min(100, iProb); // experimental (K-Mod)
}

void CvCityAI::AI_bestPlotBuild(CvPlot* pPlot, int* piBestValue, BuildTypes* peBestBuild, int iFoodPriority, int iProductionPriority, int iCommercePriority, bool bChop, int iHappyAdjust, int iHealthAdjust, int iFoodChange)
{
	PROFILE_FUNC();

	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod

	BuildTypes eBestBuild;

	bool bEmphasizeIrrigation = false;
	int iBestValue;

	if (piBestValue != NULL)
	{
		*piBestValue = 0;
	}
	if (peBestBuild != NULL)
	{
		*peBestBuild = NO_BUILD;
	}

	if (pPlot->getWorkingCity() != this)
	{
		return;
	}

	//When improving new plots only, count emphasis twice
	//helps to avoid too much tearing up of old improvements.
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/06/09                                jdog5000      */
/*                                                                                              */
/* Worker AI                                                                                    */
/************************************************************************************************/
	// AI no longer uses emphasis really, except for short term boosts to commerce.
	// Inappropriate to base improvements on short term goals.
	if( isHuman() )
	{
		if (pPlot->getImprovementType() == NO_IMPROVEMENT)
		{
			if (AI_isEmphasizeYield(YIELD_FOOD))
			{
				iFoodPriority *= 130;
				iFoodPriority /= 100;
			}
			if (AI_isEmphasizeYield(YIELD_PRODUCTION))
			{
				iProductionPriority *= 180;
				iProductionPriority /= 100;
			}
			if (AI_isEmphasizeYield(YIELD_COMMERCE))
			{
				iCommercePriority *= 180;
				iCommercePriority /= 100;
			}
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	FAssertMsg(pPlot->getOwnerINLINE() == getOwnerINLINE(), "pPlot must be owned by this city's owner");

	BonusTypes eBonus = pPlot->getBonusType(getTeam());
	BonusTypes eNonObsoleteBonus = pPlot->getNonObsoleteBonusType(getTeam());

	bool bHasBonusImprovement = false;

	if (eNonObsoleteBonus != NO_BONUS)
	{
		if (pPlot->getImprovementType() != NO_IMPROVEMENT)
		{
			if (GC.getImprovementInfo(pPlot->getImprovementType()).isImprovementBonusTrade(eNonObsoleteBonus))
			{
				bHasBonusImprovement = true;
			}
		}
	}

	iBestValue = 0;
	eBestBuild = NO_BUILD;

	int iClearFeatureValue = 0;
	// K-Mod. I've removed the yield change evaluation from AI_clearFeatureValue (to avoid double counting it elsewhere)
	// but for the parts of this function, we want to count that value...
	int iClearValue_wYield = 0; // So I've made this new number for that purpose.

	if (pPlot->getFeatureType() != NO_FEATURE)
	{
		iClearFeatureValue = AI_clearFeatureValue(getCityPlotIndex(pPlot));

		const CvFeatureInfo& kFeatureInfo = GC.getFeatureInfo(pPlot->getFeatureType());
		iClearValue_wYield = iClearFeatureValue;
		iClearValue_wYield -= kFeatureInfo.getYieldChange(YIELD_FOOD) * 100 * iFoodPriority / 100;
		iClearValue_wYield -= kFeatureInfo.getYieldChange(YIELD_PRODUCTION) * 60 * iProductionPriority / 100;
		iClearValue_wYield -= kFeatureInfo.getYieldChange(YIELD_COMMERCE) * 40 * iCommercePriority / 100;
	}

	if (!bHasBonusImprovement)
	{
		bEmphasizeIrrigation = false;

		CvPlot* pAdjacentPlot;
		CvPlot* pAdjacentPlot2;
		BonusTypes eTempBonus;

		//It looks unwieldly but the code has to be rigid to avoid "worker ADD"
		//where they keep connecting then disconnecting a crops resource or building
		//multiple farms to connect a single crop resource.
		//isFreshWater is used to ensure invalid plots are pruned early, the inner loop
		//really doesn't run that often.

		//using logic along the lines of "Will irrigating me make crops wet"
		//wont really work... it does have to "am i the tile the crops want to be irrigated"

		//I optimized through the use of "isIrrigated" which is just checking a bool...
		//once everything is nicely irrigated, this code should be really fast...
		if ((pPlot->isIrrigated()) || (pPlot->isFreshWater() && pPlot->canHavePotentialIrrigation()))
		{
			for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
			{
				pAdjacentPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), ((DirectionTypes)iI));

				if ((pAdjacentPlot != NULL) && (pAdjacentPlot->getOwner() == getOwner()) && (pAdjacentPlot->isCityRadius()))
				{
					if (!pAdjacentPlot->isFreshWater())
					{
						//check for a city? cities can conduct irrigation and that effect is quite
						//useful... so I think irrigate cities.
						if (pAdjacentPlot->isPotentialIrrigation())
						{
							CvPlot* eBestIrrigationPlot = NULL;


							for (int iJ = 0; iJ < NUM_DIRECTION_TYPES; iJ++)
							{
								pAdjacentPlot2 = plotDirection(pAdjacentPlot->getX_INLINE(), pAdjacentPlot->getY_INLINE(), ((DirectionTypes)iJ));
								if ((pAdjacentPlot2 != NULL) && (pAdjacentPlot2->getOwner() == getOwner()))
								{
									eTempBonus = pAdjacentPlot2->getNonObsoleteBonusType(getTeam());
									if (pAdjacentPlot->isIrrigated())
									{
										//the irrigation has to be coming from somewhere
										if (pAdjacentPlot2->isIrrigated())
										{
											//if we find a tile which is already carrying irrigation
											//then lets not replace that one...
											eBestIrrigationPlot = pAdjacentPlot2;

											if ((pAdjacentPlot2->isCity()) || (eTempBonus != NO_BONUS) || (!pAdjacentPlot2->isCityRadius()))
											{
												if (pAdjacentPlot2->isFreshWater())
												{
													//these are all ideal for irrigation chains so stop looking.
													break;
												}
											}
										}

									}
									else
									{
										if (pAdjacentPlot2->getNonObsoleteBonusType(getTeam()) == NO_BONUS)
										{
											if (pAdjacentPlot2->canHavePotentialIrrigation() && pAdjacentPlot2->isIrrigationAvailable())
											{
												//could use more sophisticated logic
												//however this would rely on things like smart irrigation chaining
												//of out-of-city plots
												eBestIrrigationPlot = pAdjacentPlot2;
												break;
											}
										}
									}
								}
							}

							if (pPlot == eBestIrrigationPlot)
							{
								bEmphasizeIrrigation = true;
								break;
							}
						}
					}
				}
			}
		}
	}

	for (int iI = 0; iI < GC.getNumImprovementInfos(); iI++)
	{
		ImprovementTypes eImprovement = ((ImprovementTypes)iI);
		BuildTypes eBestTempBuild;
		int iValue = AI_getImprovementValue(pPlot, eImprovement, iFoodPriority, iProductionPriority, iCommercePriority, iFoodChange, iClearFeatureValue, bEmphasizeIrrigation, &eBestTempBuild);
		if (iValue > iBestValue)
		{
			iBestValue = iValue;
			eBestBuild = eBestTempBuild;
		}
	}

	if (iClearValue_wYield > 0)
	{
		FAssert(pPlot->getFeatureType() != NO_FEATURE);

		{
			/*if ((GC.getFeatureInfo(pPlot->getFeatureType()).getHealthPercent() < 0) ||
				((GC.getFeatureInfo(pPlot->getFeatureType()).getYieldChange(YIELD_FOOD) + GC.getFeatureInfo(pPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) + GC.getFeatureInfo(pPlot->getFeatureType()).getYieldChange(YIELD_COMMERCE)) < 0))*/
			// Disabled by K-Mod. That stuff is already taken into account by iClearValue_wYield.
			{
				for (int iI = 0; iI < GC.getNumBuildInfos(); iI++)
				{
					BuildTypes eBuild = ((BuildTypes)iI);

					if (GC.getBuildInfo(eBuild).getImprovement() == NO_IMPROVEMENT)
					{
						if (GC.getBuildInfo(eBuild).isFeatureRemove(pPlot->getFeatureType()))
						{
							if (kOwner.canBuild(pPlot, eBuild))
							{
								int iValue = iClearValue_wYield;
								CvCity* pCity;
								iValue += (pPlot->getFeatureProduction(eBuild, getTeam(), &pCity) * 5); // was * 10

								iValue *= 400;
								iValue /= std::max(1, (GC.getBuildInfo(eBuild).getFeatureTime(pPlot->getFeatureType()) + 100));

								if ((iValue > iBestValue) || ((iValue > 0) && (eBestBuild == NO_BUILD)))
								{
									iBestValue = iValue;
									eBestBuild = eBuild;
								}
							}
						}
					}
				}
			}
		}
	}

	//Chop - maybe integrate this better with the other feature-clear code tho the logic
	//is kinda different
	// K-Mod. Don't chop the feature if we need it for our best improvement!
	if (bChop)
	{
		if (eBestBuild == NO_BUILD || (GC.getBuildInfo(eBestBuild).getImprovement() != NO_IMPROVEMENT
			&& GC.getImprovementInfo((ImprovementTypes)GC.getBuildInfo(eBestBuild).getImprovement()).isRequiresFeature()))
		{
			bChop = false;
		}
	}
	// K-Mod end
	if (bChop && (eBonus == NO_BONUS) && (pPlot->getFeatureType() != NO_FEATURE) &&
		(pPlot->getImprovementType() == NO_IMPROVEMENT) && !(kOwner.isOption(PLAYEROPTION_LEAVE_FORESTS)))
	{
		for (int iI = 0; iI < GC.getNumBuildInfos(); iI++)
		{
			BuildTypes eBuild = ((BuildTypes)iI);
			if (GC.getBuildInfo(eBuild).getImprovement() == NO_IMPROVEMENT)
			{
				if (GC.getBuildInfo(eBuild).isFeatureRemove(pPlot->getFeatureType()))
				{
					if (kOwner.canBuild(pPlot, eBuild))
					{
						CvCity* pCity;
						int iValue = (pPlot->getFeatureProduction(eBuild, getTeam(), &pCity)) * 10;
						FAssert(pCity == this);

						if (iValue > 0)
						{
							iValue += iClearValue_wYield;
							// K-Mod
							if (!pPlot->isBeingWorked() && iClearFeatureValue < 0)
							{
								iValue += iClearFeatureValue/2; // extra points for passive feature bonuses such as health
							}
							// K-Mod end

							if (iValue > 0)
							{
								if (kOwner.AI_isDoStrategy(AI_STRATEGY_DAGGER))
								{
									iValue += 20;
									iValue *= 2;
								}
								// K-Mod, flavour production, military, and growth.
								if (kOwner.AI_getFlavorValue(FLAVOR_PRODUCTION) > 0 || kOwner.AI_getFlavorValue(FLAVOR_MILITARY) + kOwner.AI_getFlavorValue(FLAVOR_GROWTH) > 2)
								{
									iValue *= 110 + 3 * kOwner.AI_getFlavorValue(FLAVOR_PRODUCTION) + 2 * kOwner.AI_getFlavorValue(FLAVOR_MILITARY) + 2 * kOwner.AI_getFlavorValue(FLAVOR_GROWTH);
									iValue /= 100;
								}
								// K-Mod end
								iValue *= 500;
								iValue /= std::max(1, (GC.getBuildInfo(eBuild).getFeatureTime(pPlot->getFeatureType()) + 100));

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									eBestBuild = eBuild;
								}
							}
						}
					}
				}
			}
		}
	}


	for (int iI = 0; iI < GC.getNumRouteInfos(); iI++)
	{
		RouteTypes eRoute = (RouteTypes)iI;
		RouteTypes eOldRoute = pPlot->getRouteType();

		if (eRoute != eOldRoute)
		{
			int iTempValue = 0;
			if (pPlot->getImprovementType() != NO_IMPROVEMENT)
			{
				if ((eOldRoute == NO_ROUTE) || (GC.getRouteInfo(eRoute).getValue() > GC.getRouteInfo(eOldRoute).getValue()))
				{
					iTempValue += ((GC.getImprovementInfo(pPlot->getImprovementType()).getRouteYieldChanges(eRoute, YIELD_FOOD)) * 100);
					iTempValue += ((GC.getImprovementInfo(pPlot->getImprovementType()).getRouteYieldChanges(eRoute, YIELD_PRODUCTION)) * 60);
					iTempValue += ((GC.getImprovementInfo(pPlot->getImprovementType()).getRouteYieldChanges(eRoute, YIELD_COMMERCE)) * 40);
				}

				if (pPlot->isBeingWorked())
				{
					iTempValue *= 2;
				}
				//road up bonuses if sort of bored.
				//if ((eOldRoute == NO_ROUTE) && (eBonus != NO_BONUS))
				if (!pPlot->isWater() && eOldRoute == NO_ROUTE && eBonus != NO_BONUS) // K-Mod
				{
					iTempValue += (pPlot->isConnectedToCapital() ? 10 : 30);
				}
			}

			if (iTempValue > 0)
			{
				for (int iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
				{
					BuildTypes eBuild = ((BuildTypes)iJ);
					if (GC.getBuildInfo(eBuild).getRoute() == eRoute)
					{
						if (kOwner.canBuild(pPlot, eBuild, false))
						{
							//the value multiplier is based on the default time...
							int iValue = iTempValue * 5 * 300;
							iValue /= GC.getBuildInfo(eBuild).getTime();

							if ((iValue > iBestValue) || ((iValue > 0) && (eBestBuild == NO_BUILD)))
							{
								iBestValue = iValue;
								eBestBuild = eBuild;
							}
						}
					}
				}
			}
		}
	}

	if (eBestBuild != NO_BUILD)
	{
		FAssertMsg(iBestValue > 0, "iBestValue is expected to be greater than 0");

		/*
		//Now modify the priority for this build.
		if (GET_PLAYER(getOwnerINLINE()).AI_isFinancialTrouble())
		{
			if (GC.getBuildInfo(eBestBuild).getImprovement() != NO_IMPROVEMENT)
			{
				iBestValue += (iBestValue * std::max(0, aiBestDiffYields[YIELD_COMMERCE])) / 4;
				iBestValue = std::max(1, iBestValue);
			}

		}
		*/

		if (piBestValue != NULL)
		{
			*piBestValue = iBestValue;
		}
		if (peBestBuild != NULL)
		{
			*peBestBuild = eBestBuild;
		}
	}
}


int CvCityAI::AI_getHappyFromHurry(HurryTypes eHurry) const
{
	return AI_getHappyFromHurry(hurryPopulation(eHurry));
}

int CvCityAI::AI_getHappyFromHurry(int iHurryPopulation) const
{
	PROFILE_FUNC();

	int iHappyDiff = iHurryPopulation - GC.getDefineINT("HURRY_POP_ANGER");
	if (iHappyDiff > 0)
	{
		if (getHurryAngerTimer() <= 1)
		{
			if (2 * angryPopulation(1) - healthRate(false, 1) > 1)
			{
				return iHappyDiff;
			}
		}
	}

	return 0;
}

int CvCityAI::AI_getHappyFromHurry(HurryTypes eHurry, UnitTypes eUnit, bool bIgnoreNew) const
{
	return AI_getHappyFromHurry(getHurryPopulation(eHurry, getHurryCost(true, eUnit, bIgnoreNew)));
}

int CvCityAI::AI_getHappyFromHurry(HurryTypes eHurry, BuildingTypes eBuilding, bool bIgnoreNew) const
{
	return AI_getHappyFromHurry(getHurryPopulation(eHurry, getHurryCost(true, eBuilding, bIgnoreNew)));
}


int CvCityAI::AI_cityValue() const
{
	
	AreaAITypes eAreaAI = area()->getAreaAIType(getTeam());
    if ((eAreaAI == AREAAI_OFFENSIVE) || (eAreaAI == AREAAI_MASSING) || (eAreaAI == AREAAI_DEFENSIVE))
    {
        return 0;
    }

	// K-Mod: disorder causes the commerce rates to go to zero... so that would  mess up our calculations
	if (isDisorder())
		return 0;
	// K-Mod end
    
	int iValue = 0;
	
	/* original bts code
	iValue += getCommerceRateTimes100(COMMERCE_GOLD);
	iValue += getCommerceRateTimes100(COMMERCE_RESEARCH);
	iValue += 100 * getYieldRate(YIELD_PRODUCTION);
	iValue -= 3 * calculateColonyMaintenanceTimes100(); */
	// K-Mod. The original code fails for civs using high espionage or high culture.
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	iValue += getCommerceRateTimes100(COMMERCE_RESEARCH) * kOwner.AI_commerceWeight(COMMERCE_RESEARCH);
	iValue += getCommerceRateTimes100(COMMERCE_ESPIONAGE) * kOwner.AI_commerceWeight(COMMERCE_ESPIONAGE);
	iValue /= 100;
	// Culture isn't counted, because its value is local. Unfortunately this will mean that civs running 100% culture
	// (ie CULTURE4) will give up their non-core cities. It could be argued that this would be good strategy,
	// but the problem is that CULTURE4 doesn't always run its full course. ... so I'm going to make a small ad hoc adjustment...
	iValue += 100 * getYieldRate(YIELD_PRODUCTION);
	iValue *= kOwner.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4)? 2 : 1;
	// Gold value is not weighted, and does not get the cultural victory boost, because gold is directly comparable to maintenance.
	iValue += getCommerceRateTimes100(COMMERCE_GOLD);

	int iCosts = calculateColonyMaintenanceTimes100() + 2*getMaintenanceTimes100()/3;
	int iTargetPop = std::max(5, AI_getTargetPopulation()); // target pop is not a good measure for small cities w/ unimproved tiles.
	if (getPopulation() > 0 && getPopulation() < iTargetPop)
	{
		iValue *= iTargetPop + 3;
		iValue /= getPopulation() + 3;
		iCosts *= getPopulation() + 6;
		iCosts /= iTargetPop + 6;
	}
	iValue -= iCosts;
	// K-Mod end

	return iValue;
}

bool CvCityAI::AI_doPanic()
{
	bool bLandWar = ((area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) || (area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE) || (area()->getAreaAIType(getTeam()) == AREAAI_MASSING));
	
	if (bLandWar)
	{
		int iOurDefense = GET_PLAYER(getOwnerINLINE()).AI_localDefenceStrength(plot(), getTeam());
		int iEnemyOffense = GET_PLAYER(getOwnerINLINE()).AI_localAttackStrength(plot(), NO_TEAM);
		int iRatio = (100 * iEnemyOffense) / (std::max(1, iOurDefense));

		if (iRatio > 100)
		{
			UnitTypes eProductionUnit = getProductionUnit();

			if (eProductionUnit != NO_UNIT)
			{
				if (getProduction() > 0)
				{
					if (GC.getUnitInfo(eProductionUnit).getCombat() > 0)
					{
						AI_doHurry(true);
						return true;
					}
				}
			}
			else
			{
				if ((GC.getGame().getSorenRandNum(2, "AI choose panic unit") == 0) && AI_chooseUnit(UNITAI_CITY_COUNTER))
				{
					AI_doHurry((iRatio > 140));	
				}
				else if (AI_chooseUnit(UNITAI_CITY_DEFENSE))
				{
					AI_doHurry((iRatio > 140));
				}
				else if (AI_chooseUnit(UNITAI_ATTACK))
				{
					AI_doHurry((iRatio > 140));
				}
			}
		}
	}
	return false;
}

int CvCityAI::AI_calculateCulturePressure(bool bGreatWork) const
{
	PROFILE_FUNC();
    CvPlot* pLoopPlot;
    BonusTypes eNonObsoleteBonus;
    int iValue;
    int iTempValue;
    int iI;

    iValue = 0;
    for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
    {
        pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);
		if (pLoopPlot != NULL)
		{
		    if (pLoopPlot->getOwnerINLINE() == NO_PLAYER)
		    {
		        iValue++;
		    }
		    else
		    {
                iTempValue = pLoopPlot->calculateCulturePercent(getOwnerINLINE());
                if (iTempValue == 100)
                {
                    //do nothing
                }
                else if ((iTempValue == 0) || (iTempValue > 75))
                {
                    iValue++;
                }
                else
                {
                    iTempValue = (100 - iTempValue);
                    FAssert(iTempValue > 0);
                    FAssert(iTempValue <= 100);

                    if (iI != CITY_HOME_PLOT)
                    {
                        iTempValue *= 4;
                        iTempValue /= NUM_CITY_PLOTS;
                    }

                    eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(getTeam());

                    if (eNonObsoleteBonus != NO_BONUS)
                    {
                        iTempValue += (GET_PLAYER(getOwnerINLINE()).AI_bonusVal(eNonObsoleteBonus) * ((GET_PLAYER(getOwnerINLINE()).getNumTradeableBonuses(eNonObsoleteBonus) == 0) ? 4 : 2));
                    }
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       03/20/10                          denev & jdog5000    */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
					if ((iTempValue > 80) && (pLoopPlot->getOwnerINLINE() == getID()))
*/
					if ((iTempValue > 80) && (pLoopPlot->getOwnerINLINE() == getOwnerINLINE()))
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
                    {
                        //captured territory special case
                        iTempValue *= (100 - iTempValue);
                        iTempValue /= 100;
                    }

                    if (pLoopPlot->getTeam() == getTeam())
                    {
                        iTempValue /= (bGreatWork ? 10 : 2);
                    }
                    else
                    {
                        iTempValue *= 2;
                        if (bGreatWork)
                        {
                            if (GET_PLAYER(getOwnerINLINE()).AI_getAttitude(pLoopPlot->getOwnerINLINE()) == ATTITUDE_FRIENDLY)
                            {
                                iValue /= 10;
                            }
                        }
                    }

                    iValue += iTempValue;
                }
            }
		}
    }

    return iValue;
}

// K-Mod: I've completely butchered this function. ie. deleted the bulk of it, and rewritten it.
void CvCityAI::AI_buildGovernorChooseProduction()
{
	PROFILE_FUNC();

	AI_setChooseProductionDirty(false);
	clearOrderQueue();

	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	CvArea* pWaterArea = waterArea();
	bool bWasFoodProduction = isFoodProduction();
	bool bDanger = AI_isDanger();

	BuildingTypes eBestBuilding = AI_bestBuildingThreshold(); // go go value cache!
	int iBestBuildingValue = (eBestBuilding == NO_BUILDING) ? 0 : AI_buildingValue(eBestBuilding);

    // pop borders
	if (getCultureLevel() <= (CultureLevelTypes)1 && getCommerceRate(COMMERCE_CULTURE) == 0)
	{
		if (eBestBuilding != NO_BUILDING && AI_countGoodTiles(true, false) > 0)
		{
			const CvBuildingInfo& kBestBuilding = GC.getBuildingInfo(eBestBuilding);
			if (kBestBuilding.getCommerceChange(COMMERCE_CULTURE) + kBestBuilding.getObsoleteSafeCommerceChange(COMMERCE_CULTURE) > 0
				&& (GC.getNumCultureLevelInfos() < 2 || getProductionTurnsLeft(eBestBuilding, 0) <= GC.getGame().getCultureThreshold((CultureLevelTypes)2)))
			{
				pushOrder(ORDER_CONSTRUCT, eBestBuilding, -1, false, false, false);
				return;
			}
		}
		if (AI_countGoodTiles(true, true, 60) == 0 && AI_chooseProcess(COMMERCE_CULTURE))
		{
			return;
		}
	}
	
	//workboat
	if (pWaterArea != NULL)
	{
		if (kOwner.AI_totalWaterAreaUnitAIs(pWaterArea, UNITAI_WORKER_SEA) == 0)
		{
			if (AI_neededSeaWorkers() > 0)
			{
				bool bLocalResource = AI_countNumImprovableBonuses(true, NO_TECH, false, true) > 0;
				if (bLocalResource || iBestBuildingValue < 20)
				{
					int iOdds = 100;
					iOdds *= 50 + iBestBuildingValue;
					iOdds /= 50 + (bLocalResource ? 3 : 5) * iBestBuildingValue;
					if (AI_chooseUnit(UNITAI_WORKER_SEA, iOdds))
					{
						return;
					}
				}
			}
		}
	}
	
	if (kOwner.AI_isPrimaryArea(area())) // if its not a primary area, let the player ship units in.
	{
		//worker
		if (!bDanger)
		{
			int iExistingWorkers = kOwner.AI_totalAreaUnitAIs(area(), UNITAI_WORKER);
			int iNeededWorkers = kOwner.AI_neededWorkers(area());

			if (iExistingWorkers < (iNeededWorkers + 2)/3) // I don't want to build more workers than the player actually wants.
			{
				int iOdds = 100;
				iOdds *= iNeededWorkers - iExistingWorkers;
				iOdds /= std::max(1, iNeededWorkers);
				iOdds *= 50 + iBestBuildingValue;
				iOdds /= 50 + 5 * iBestBuildingValue;

				if (AI_chooseUnit(UNITAI_WORKER, iOdds))
				{
					return;
				}
			}
		}

		// adjust iBestBuildValue for the remaining comparisons. (military and spies.)
		if (GC.getNumEraInfos() > 1)
		{
			FAssert(kOwner.getCurrentEra() < GC.getNumEraInfos());
			iBestBuildingValue *= 2*(GC.getNumEraInfos()-1) - kOwner.getCurrentEra();
			iBestBuildingValue /= GC.getNumEraInfos()-1;
		}

		//military
		bool bWar = GET_TEAM(getTeam()).getAtWarCount(true) > 0;
		if (bDanger || iBestBuildingValue < (bWar ? 35 : 20))
		{
			int iOdds = (bWar ? 100 : 50) - kOwner.AI_unitCostPerMil()/2;
			iOdds += AI_experienceWeight();
			iOdds *= 100 + 2*getMilitaryProductionModifier();
			iOdds /= 100;

			iOdds *= 50 + iBestBuildingValue;
			iOdds /= 50 + 10 * iBestBuildingValue;

			if (GC.getGameINLINE().getSorenRandNum(100, "City governor choose military") < iOdds)
			{
				UnitAITypes eUnitAI;
				UnitTypes eBestUnit = AI_bestUnit(false, NO_ADVISOR, &eUnitAI);
				if (eBestUnit != NO_UNIT)
				{
					const CvUnitInfo& kUnit = GC.getUnitInfo(eBestUnit);

					if (kUnit.getUnitCombatType() != NO_UNITCOMBAT)
					{
						BuildingTypes eExperienceBuilding = AI_bestBuildingThreshold(BUILDINGFOCUS_EXPERIENCE);

						if (eExperienceBuilding != NO_BUILDING)
						{
							const CvBuildingInfo& kBuilding = GC.getBuildingInfo(eExperienceBuilding);
							if (kBuilding.getFreeExperience() > 0 ||
								kBuilding.getUnitCombatFreeExperience(kUnit.getUnitCombatType()) > 0 ||
								kBuilding.getDomainFreeExperience(kUnit.getDomainType()) > 0)
							{
								// This building helps the unit we want
								// ...so do the building first.
								pushOrder(ORDER_CONSTRUCT, eBestBuilding, -1, false, false, false);
								return;
							}
						}
					}
					// otherwise, we're ready to build the unit
					pushOrder(ORDER_TRAIN, eBestUnit, eUnitAI, false, false, false);
					return;
				}
			}
		}

		//spies
		if (!kOwner.AI_isAreaAlone(area()))
		{
			int iNumSpies = kOwner.AI_totalAreaUnitAIs(area(), UNITAI_SPY) + kOwner.AI_getNumTrainAIUnits(UNITAI_SPY);
			int iNeededSpies = 2 + area()->getCitiesPerPlayer(kOwner.getID()) / 5;
			iNeededSpies += kOwner.getCommercePercent(COMMERCE_ESPIONAGE)/20;

			if (iNumSpies < iNeededSpies)
			{
				int iOdds = 35 - iBestBuildingValue/3;
				if (iOdds > 0)
				{
					iOdds *= (40 + iBestBuildingValue);
					iOdds /= (20 + 3 * iBestBuildingValue);
					iOdds *= iNeededSpies;
					iOdds /= (4*iNumSpies+iNeededSpies);
					if (AI_chooseUnit(UNITAI_SPY, iOdds))
					{
						return;
					}
				}
			}
		}

		//spread
		int iSpreadUnitOdds = std::max(0, 80 - iBestBuildingValue*2);

		int iSpreadUnitThreshold = 1200 + (bWar ? 800: 0) + iBestBuildingValue * 10;
		// is it wrong to use UNITAI values for human players?
		iSpreadUnitThreshold += kOwner.AI_totalAreaUnitAIs(area(), UNITAI_MISSIONARY) * 500;

		UnitTypes eBestSpreadUnit = NO_UNIT;
		int iBestSpreadUnitValue = -1;
		if (AI_bestSpreadUnit(true, true, iSpreadUnitOdds, &eBestSpreadUnit, &iBestSpreadUnitValue))
		{
			if (iBestSpreadUnitValue > iSpreadUnitThreshold)
			{
				if (AI_chooseUnit(eBestSpreadUnit, UNITAI_MISSIONARY))
				{
					return;
				}
				FAssertMsg(false, "AI_bestSpreadUnit should provide a valid unit when it returns true");
			}
		}
	}

	//default
	if (AI_chooseBuilding())
	{
		return;
	}

	if (AI_chooseProcess())
	{
		return;
	}

    if (AI_chooseUnit())
	{
		return;
	}
}

// K-Mod. This is a chunk of code that I moved out of AI_chooseProduction. The only reason I've moved it is to reduce clutter in the other function.
void CvCityAI::AI_barbChooseProduction()
{
	const CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());

	CvArea* pArea = area();
	CvArea* pWaterArea = waterArea(true);
	bool bMaybeWaterArea = false;
	bool bWaterDanger = false;
	bool bDanger = AI_isDanger();

	if (pWaterArea != NULL)
	{
		bMaybeWaterArea = true;
		if (!GET_TEAM(getTeam()).AI_isWaterAreaRelevant(pWaterArea))
		{
			pWaterArea = NULL;
		}

		bWaterDanger = kPlayer.AI_getWaterDanger(plot(), 4) > 0;
	}

	int iNumCitiesInArea = pArea->getCitiesPerPlayer(getOwnerINLINE());

	int iBuildUnitProb = AI_buildUnitProb();

	int iExistingWorkers = kPlayer.AI_totalAreaUnitAIs(pArea, UNITAI_WORKER);
	int iNeededWorkers = kPlayer.AI_neededWorkers(pArea);
	// Sea worker need independent of whether water area is militarily relevant
	int iNeededSeaWorkers = (bMaybeWaterArea) ? AI_neededSeaWorkers() : 0;
	int iExistingSeaWorkers = (waterArea(true) != NULL) ? kPlayer.AI_totalWaterAreaUnitAIs(waterArea(true), UNITAI_WORKER_SEA) : 0;
	int iWaterPercent = AI_calculateWaterWorldPercent();

	if (!AI_isDefended(plot()->plotCount(PUF_isUnitAIType, UNITAI_ATTACK, -1, getOwnerINLINE()))) // XXX check for other team's units?
	{
		if (AI_chooseDefender())
		{
			return;
		}

		if (AI_chooseUnit(UNITAI_ATTACK))
		{
			return;
		}
	}

	if (!bDanger && (2*iExistingWorkers < iNeededWorkers) && (AI_getWorkersNeeded() > 0) && (AI_getWorkersHave() == 0))
	{
		if( getPopulation() > 1 || (GC.getGameINLINE().getGameTurn() - getGameTurnAcquired() > (15 * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent())/100) )
		{
			if (AI_chooseUnit(UNITAI_WORKER))
			{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S uses barb choose worker 1", getName().GetCString());
				return;
			}
		}
	}

	if (!bDanger && !bWaterDanger && (iNeededSeaWorkers > 0))
	{
		if (AI_chooseUnit(UNITAI_WORKER_SEA))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses barb choose worker sea 1", getName().GetCString());
			return;
		}
	}

	int iFreeLandExperience = getSpecialistFreeExperience() + getDomainFreeExperience(DOMAIN_LAND);
	iBuildUnitProb += (3 * iFreeLandExperience);

	bool bRepelColonists = false;
	if( area()->getNumCities() > area()->getCitiesPerPlayer(BARBARIAN_PLAYER) + 2 )
	{
		if( area()->getCitiesPerPlayer(BARBARIAN_PLAYER) > area()->getNumCities()/3 )
		{
			// New world scenario with invading colonists ... fight back!
			bRepelColonists = true;
			iBuildUnitProb += 8*(area()->getNumCities() - area()->getCitiesPerPlayer(BARBARIAN_PLAYER));
		}
	}

	if (!bDanger && GC.getGameINLINE().getSorenRandNum(100, "AI Build Unit Production") > iBuildUnitProb)
	{

		int iBarbarianFlags = 0;
		if( getPopulation() < 4 ) iBarbarianFlags |= BUILDINGFOCUS_FOOD;
		iBarbarianFlags |= BUILDINGFOCUS_PRODUCTION;
		iBarbarianFlags |= BUILDINGFOCUS_EXPERIENCE;
		if( getPopulation() > 3 ) iBarbarianFlags |= BUILDINGFOCUS_DEFENSE;

		if (AI_chooseBuilding(iBarbarianFlags, 15))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses barb AI_chooseBuilding with flags and iBuildUnitProb = %d", getName().GetCString(), iBuildUnitProb);
			return;
		}

		if( GC.getGameINLINE().getSorenRandNum(100, "AI Build Unit Production") > iBuildUnitProb)
		{
			if (AI_chooseBuilding())
			{
				if( gCityLogLevel >= 2 ) logBBAI("      City %S uses barb AI_chooseBuilding without flags and iBuildUnitProb = %d", getName().GetCString(), iBuildUnitProb);
				return;
			}
		}
	}

	if (plot()->plotCount(PUF_isUnitAIType, UNITAI_ASSAULT_SEA, -1, getOwnerINLINE()) > 0)
	{
		if (AI_chooseUnit(UNITAI_ATTACK_CITY))
		{
			if( gCityLogLevel >= 2 ) logBBAI("      City %S uses barb choose attack city for transports", getName().GetCString());
			return;
		}
	}

	if (!bDanger && (pWaterArea != NULL) && (iWaterPercent > 30))
	{
		if (GC.getGameINLINE().getSorenRandNum(3, "AI Coast Raiders!") == 0)
		{
			if (kPlayer.AI_totalUnitAIs(UNITAI_ASSAULT_SEA) <= (1 + kPlayer.getNumCities() / 2))
			{
				if (AI_chooseUnit(UNITAI_ASSAULT_SEA))
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses barb choose transport", getName().GetCString());
					return;
				}
			}
		}
		if (GC.getGameINLINE().getSorenRandNum(110, "AI arrrr!") < (iWaterPercent + 10))
		{
			if (kPlayer.AI_totalUnitAIs(UNITAI_PIRATE_SEA) <= kPlayer.getNumCities())
			{
				if (AI_chooseUnit(UNITAI_PIRATE_SEA))
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses barb choose pirate", getName().GetCString());
					return;
				}
			}

			if (kPlayer.AI_totalAreaUnitAIs(pWaterArea, UNITAI_ATTACK_SEA) < iNumCitiesInArea)
			{
				if (AI_chooseUnit(UNITAI_ATTACK_SEA))
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses barb choose attack sea", getName().GetCString());
					return;
				}
			}
		}
	}

	if (GC.getGameINLINE().getSorenRandNum(2, "Barb worker") == 0)
	{
		if (!bDanger && (iExistingWorkers < iNeededWorkers) && (AI_getWorkersNeeded() > 0) && (AI_getWorkersHave() == 0))
		{
			if( getPopulation() > 1 )
			{
				if (AI_chooseUnit(UNITAI_WORKER))
				{
					if( gCityLogLevel >= 2 ) logBBAI("      City %S uses barb choose worker 2", getName().GetCString());
					return;
				}
			}
		}
	}

	UnitTypeWeightArray barbarianTypes;
	barbarianTypes.push_back(std::make_pair(UNITAI_ATTACK, 125));
	barbarianTypes.push_back(std::make_pair(UNITAI_ATTACK_CITY, (bRepelColonists ? 100 : 50)));
	barbarianTypes.push_back(std::make_pair(UNITAI_COUNTER, 100));
	barbarianTypes.push_back(std::make_pair(UNITAI_CITY_DEFENSE, 50));

	if (AI_chooseLeastRepresentedUnit(barbarianTypes))
	{
		return;
	}

	if (AI_chooseUnit())
	{
		return;
	}

	return;
}
// K-Mod end

int CvCityAI::AI_calculateWaterWorldPercent()
{
	int iI;
	int iWaterPercent = 0;
	int iTeamCityCount = 0;
	int iOtherCityCount = 0;
	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (iI == getTeam() || GET_TEAM((TeamTypes)iI).isVassal(getTeam())
				|| GET_TEAM(getTeam()).isVassal((TeamTypes)iI))
			{
				iTeamCityCount += GET_TEAM((TeamTypes)iI).countNumCitiesByArea(area());
			}
			else
			{
				iOtherCityCount += GET_TEAM((TeamTypes)iI).countNumCitiesByArea(area());
			}
		}
	}

	if (iOtherCityCount == 0)
	{
		iWaterPercent = 100;
	}
	else
	{
		iWaterPercent = 100 - ((iTeamCityCount + iOtherCityCount) * 100) / std::max(1, (GC.getGame().getNumCities()));
	}

	iWaterPercent *= 50;
	iWaterPercent /= 100;

	iWaterPercent += (50 * (2 + iTeamCityCount)) / (2 + iTeamCityCount + iOtherCityCount);

	iWaterPercent = std::max(1, iWaterPercent);


	return iWaterPercent;
}

//Please note, takes the yield multiplied by 100
int CvCityAI::AI_getYieldMagicValue(const int* piYieldsTimes100, bool bHealthy) const
{
	FAssert(piYieldsTimes100 != NULL);

	int iPopEats = GC.getFOOD_CONSUMPTION_PER_POPULATION();
	iPopEats += (bHealthy ? 0 : 1);
	iPopEats *= 100;

	int iValue = ((piYieldsTimes100[YIELD_FOOD] * 100 + piYieldsTimes100[YIELD_PRODUCTION]*55 + piYieldsTimes100[YIELD_COMMERCE]*40) - iPopEats * 102);
	iValue /= 100;
	return iValue;
}

//The magic value is basically "Look at this plot, is it worth working"
//-50 or lower means the plot is worthless in a "workers kill yourself" kind of way.
//-50 to -1 means the plot isn't worth growing to work - might be okay with emphasize though.
//Between 0 and 50 means it is marginal.
//50-100 means it's okay.
//Above 100 means it's definitely decent - seriously question ever not working it.
//This function deliberately doesn't use emphasize settings.
int CvCityAI::AI_getPlotMagicValue(CvPlot* pPlot, bool bHealthy, bool bWorkerOptimization) const
{
	int aiYields[NUM_YIELD_TYPES];
	ImprovementTypes eCurrentImprovement;
	ImprovementTypes eFinalImprovement;
	int iI;
	int iYieldDiff;

	FAssert(pPlot != NULL);

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		if ((bWorkerOptimization) && (pPlot->getWorkingCity() == this) && (AI_getBestBuild(getCityPlotIndex(pPlot)) != NO_BUILD))
		{
			aiYields[iI] = pPlot->getYieldWithBuild(AI_getBestBuild(getCityPlotIndex(pPlot)), (YieldTypes)iI, true);
		}
		else
		{
			aiYields[iI] = pPlot->getYield((YieldTypes)iI) * 100;
		}
	}

	eCurrentImprovement = pPlot->getImprovementType();

	if (eCurrentImprovement != NO_IMPROVEMENT)
	{
		eFinalImprovement = finalImprovementUpgrade(eCurrentImprovement);

		if ((eFinalImprovement != NO_IMPROVEMENT) && (eFinalImprovement != eCurrentImprovement))
		{
			for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
			{
				iYieldDiff = 100 * pPlot->calculateImprovementYieldChange(eFinalImprovement, ((YieldTypes)iI), getOwnerINLINE());
				iYieldDiff -= 100 * pPlot->calculateImprovementYieldChange(eCurrentImprovement, ((YieldTypes)iI), getOwnerINLINE());
				aiYields[iI] += iYieldDiff / 2;
			}
		}
	}

	return AI_getYieldMagicValue(aiYields, bHealthy);
}

//useful for deciding whether or not to grow... or whether the city needs terrain
//improvement.
//if healthy is false it assumes bad health conditions.
int CvCityAI::AI_countGoodTiles(bool bHealthy, bool bUnworkedOnly, int iThreshold, bool bWorkerOptimization) const
{
	CvPlot* pLoopPlot;
	int iI;
	int iCount;

	iCount = 0;
	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		pLoopPlot = plotCity(getX_INLINE(),getY_INLINE(), iI);
		if ((iI != CITY_HOME_PLOT) && (pLoopPlot != NULL))
		{
			if (pLoopPlot->getWorkingCity() == this)
			{
				if (!bUnworkedOnly || !(pLoopPlot->isBeingWorked()))
				{
					if (AI_getPlotMagicValue(pLoopPlot, bHealthy) > iThreshold)
					{
						iCount++;
					}
				}
			}
		}
	}
	return iCount;
}

int CvCityAI::AI_calculateTargetCulturePerTurn() const
{
	/*
	int iTarget = 0;
	
	bool bAnyGoodPlotUnowned = false;
	bool bAnyGoodPlotHighPressure = false;	
	
	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = plotCity(getX_INLINE(),getY_INLINE(),iI);
        
        if (pLoopPlot != NULL)
        {
			if ((pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
				|| (pLoopPlot->getYield(YIELD_FOOD) > GC.getFOOD_CONSUMPTION_PER_POPULATION()))
			{
				if (!pLoopPlot->isOwned())
				{
					bAnyGoodPlotUnowned = true;        			
				}
				else if (pLoopPlot->getOwnerINLINE() != getOwnerINLINE())
				{
					bAnyGoodPlotHighPressure = true;
				}
			}
        }
	}
	if (bAnyGoodPlotUnowned)
	{
		iTarget = 1;
	}
	if (bAnyGoodPlotHighPressure)
	{
		iTarget += getCommerceRate(COMMERCE_CULTURE) + 1;
	}
	return iTarget;
	*/
	return 1;
}
	
int CvCityAI::AI_countGoodSpecialists(bool bHealthy) const
{
	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
	int iCount = 0;
	for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		SpecialistTypes eSpecialist = (SpecialistTypes)iI;
		
		int iValue = 0;
		
		iValue += 100 * kPlayer.specialistYield(eSpecialist, YIELD_FOOD);
		iValue += 65 * kPlayer.specialistYield(eSpecialist, YIELD_PRODUCTION);
		iValue += 40 * kPlayer.specialistYield(eSpecialist, YIELD_COMMERCE);
		
		iValue += 40 * kPlayer.specialistCommerce(eSpecialist, COMMERCE_RESEARCH);
		iValue += 40 * kPlayer.specialistCommerce(eSpecialist, COMMERCE_GOLD);
		iValue += 20 * kPlayer.specialistCommerce(eSpecialist, COMMERCE_ESPIONAGE);
		iValue += 15 * kPlayer.specialistCommerce(eSpecialist, COMMERCE_CULTURE);
		iValue += 25 * GC.getSpecialistInfo(eSpecialist).getGreatPeopleRateChange();
		
		if (iValue >= (bHealthy ? 200 : 300))
		{
			// K-Mod
			if (isSpecialistValid(eSpecialist))
				return getPopulation(); // unlimited
			// K-Mod end
			iCount += getMaxSpecialistCount(eSpecialist);
		}
	}
	//iCount -= getFreeSpecialist();
	iCount -= totalFreeSpecialists(); // K-Mod

	//return iCount;
	return std::max(0, iCount); // K-Mod
}
//0 is normal
//higher than zero means special.
int CvCityAI::AI_getCityImportance(bool bEconomy, bool bMilitary)
{
    int iValue = 0;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
	if (GET_PLAYER(getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
    {
        int iCultureRateRank = findCommerceRateRank(COMMERCE_CULTURE);
        int iCulturalVictoryNumCultureCities = GC.getGameINLINE().culturalVictoryNumCultureCities();
        
        if (iCultureRateRank <= iCulturalVictoryNumCultureCities)
        {
            iValue += 100;
        
            if ((getCultureLevel() < (GC.getNumCultureLevelInfos() - 1)))
            {
                iValue += !bMilitary ? 100 : 0;
            }
            else
            {
                iValue += bMilitary ? 100 : 0;
            }
        }
    }
    
    return iValue;
}

// K-Mod
int CvCityAI::AI_calculateMilitaryOutput() const
{
	int iValue = 0;

	iValue = getBaseYieldRate(YIELD_PRODUCTION);
	//UnitTypes eDefaultUnit = getConscriptUnit();

	iValue *= 100 + getMilitaryProductionModifier() + 10 * getProductionExperience();
	iValue /= 100;

	return iValue;
}

void CvCityAI::AI_stealPlots()
{
    PROFILE_FUNC();
    CvPlot* pLoopPlot;
    CvCityAI* pWorkingCity;
    int iI;
    int iOtherImportance;
    
    int iImportance = AI_getCityImportance(true, false);

    for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
    {
        pLoopPlot = plotCity(getX_INLINE(),getY_INLINE(),iI);
        
        if (pLoopPlot != NULL)
        {
            if (iImportance > 0)
            {
                if (pLoopPlot->getOwnerINLINE() == getOwnerINLINE())
                {
                    pWorkingCity = static_cast<CvCityAI*>(pLoopPlot->getWorkingCity());
                    if ((pWorkingCity != this) && (pWorkingCity != NULL))
                    {
                        FAssert(pWorkingCity->getOwnerINLINE() == getOwnerINLINE());
                        iOtherImportance = pWorkingCity->AI_getCityImportance(true, false);
                        if (iImportance > iOtherImportance)
                        {
                            pLoopPlot->setWorkingCityOverride(this);
                        }
                    }
                }
            }
                
            if (pLoopPlot->getWorkingCityOverride() == this)
            {
                if (pLoopPlot->getOwnerINLINE() != getOwnerINLINE())
                {
                    pLoopPlot->setWorkingCityOverride(NULL);                    
                }
            }
        }
    }
}




// +1/+3/+5 plot based on base food yield (1/2/3)
// +4 if being worked.
// +4 if a bonus.
// Unworked ocean ranks very lowly. Unworked lake ranks at 3. Worked lake at 7.
// Worked bonus in ocean ranks at like 11
int CvCityAI::AI_buildingSpecialYieldChangeValue(BuildingTypes eBuilding, YieldTypes eYield) const
{
    int iI;
    CvPlot* pLoopPlot;
    int iValue = 0;
    CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
    int iWorkedCount = 0;
    
    int iYieldChange = kBuilding.getSeaPlotYieldChange(eYield);
    if (iYieldChange > 0)
    {
        int iWaterCount = 0;
        for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
        {
            if (iI != CITY_HOME_PLOT)
            {
                pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);
                if ((pLoopPlot != NULL) && (pLoopPlot->getWorkingCity() == this))
                {
                    if (pLoopPlot->isWater())
                    {
                        iWaterCount++;
                        int iFood = pLoopPlot->getYield(YIELD_FOOD);
                        iFood += (eYield == YIELD_FOOD) ? iYieldChange : 0;
                        
                        iValue += std::max(0, iFood * 2 - 1);
                        if (pLoopPlot->isBeingWorked())
                        {
                        	iValue += 4;
                        	iWorkedCount++;
                        }
                        iValue += ((pLoopPlot->getBonusType(getTeam()) != NO_BONUS) ? 8 : 0);
                    }
                }
            }
        }
    }
    if (iWorkedCount == 0)
    {
		SpecialistTypes eDefaultSpecialist = (SpecialistTypes)GC.getDefineINT("DEFAULT_SPECIALIST");
		if ((getPopulation() > 2) && ((eDefaultSpecialist == NO_SPECIALIST) || (getSpecialistCount(eDefaultSpecialist) == 0)))
		{
			iValue /= 2;		
		}
    }

    return iValue;
}


int CvCityAI::AI_yieldMultiplier(YieldTypes eYield) const
{
	PROFILE_FUNC();
	
	int iMultiplier = getBaseYieldRateModifier(eYield);
	
	if (eYield == YIELD_PRODUCTION)
	{
		iMultiplier += (getMilitaryProductionModifier() / 2);
	}
	
	if (eYield == YIELD_COMMERCE)
	{
		/* original code
		iMultiplier += (getCommerceRateModifier(COMMERCE_RESEARCH) * 60) / 100;
		iMultiplier += (getCommerceRateModifier(COMMERCE_GOLD) * 35) / 100;
		iMultiplier += (getCommerceRateModifier(COMMERCE_CULTURE) * 15) / 100; */
		// K-Mod
		// this breakdown seems wrong... lets try it a different way.
		const CvPlayer& kPlayer = GET_PLAYER(getOwnerINLINE());
		int iExtra = 0;
		for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			iExtra += getTotalCommerceRateModifier((CommerceTypes)iI) * kPlayer.getCommercePercent((CommerceTypes)iI);
		}
		// base commerce modifier compounds with individual commerce modifiers.
		iMultiplier *= iExtra;
		iMultiplier /= 10000;
		// K-Mod end
	}
	
	return iMultiplier;	
}
//this should be called before doing governor stuff.
//This is the function which replaces emphasis
//Could stand for a Commerce Variety to be added
//especially now that there is Espionage
void CvCityAI::AI_updateSpecialYieldMultiplier()
{
	PROFILE_FUNC();
	
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_aiSpecialYieldMultiplier[iI] = 0;
	}
	
	UnitTypes eProductionUnit = getProductionUnit();
	if (eProductionUnit != NO_UNIT)
	{
		if (GC.getUnitInfo(eProductionUnit).getDefaultUnitAIType() == UNITAI_WORKER_SEA)
		{
			m_aiSpecialYieldMultiplier[YIELD_PRODUCTION] += 50;
			m_aiSpecialYieldMultiplier[YIELD_COMMERCE] -= 50;				
		}
		if ((GC.getUnitInfo(eProductionUnit).getDefaultUnitAIType() == UNITAI_WORKER) ||
			(GC.getUnitInfo(eProductionUnit).getDefaultUnitAIType() == UNITAI_SETTLE))
		
		{
			m_aiSpecialYieldMultiplier[YIELD_COMMERCE] -= 50;			
		}
	}

	BuildingTypes eProductionBuilding = getProductionBuilding();
	if (eProductionBuilding != NO_BUILDING)
	{
		if (isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eProductionBuilding).getBuildingClassType()))
			|| isProductionProject())
		{
			m_aiSpecialYieldMultiplier[YIELD_PRODUCTION] += 50;
			m_aiSpecialYieldMultiplier[YIELD_COMMERCE] -= 25;					
		}
		m_aiSpecialYieldMultiplier[YIELD_PRODUCTION] += std::max(-25, GC.getBuildingInfo(eProductionBuilding).getFoodKept());
		
		if ((GC.getBuildingInfo(eProductionBuilding).getCommerceChange(COMMERCE_CULTURE) > 0)
			|| (GC.getBuildingInfo(eProductionBuilding).getObsoleteSafeCommerceChange(COMMERCE_CULTURE) > 0))
		{
			int iTargetCultureRate = AI_calculateTargetCulturePerTurn();
			if (iTargetCultureRate > 0)
			{
				if (getCommerceRate(COMMERCE_CULTURE) == 0)
				{
					m_aiSpecialYieldMultiplier[YIELD_PRODUCTION] += 50;
				}
				else if (getCommerceRate(COMMERCE_CULTURE) < iTargetCultureRate)
				{
					m_aiSpecialYieldMultiplier[YIELD_PRODUCTION] += 20;					
				}
			}
		}
	}
	
	// non-human production value increase
	if (!isHuman())
	{
		CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
		AreaAITypes eAreaAIType = area()->getAreaAIType(getTeam());
		
		if ((kPlayer.AI_isDoStrategy(AI_STRATEGY_DAGGER) && getPopulation() >= 4)
			|| (eAreaAIType == AREAAI_OFFENSIVE) || (eAreaAIType == AREAAI_DEFENSIVE) 
			|| (eAreaAIType == AREAAI_MASSING) || (eAreaAIType == AREAAI_ASSAULT))
		{
			m_aiSpecialYieldMultiplier[YIELD_PRODUCTION] += 10;
			if (!kPlayer.AI_isFinancialTrouble())
			{
				m_aiSpecialYieldMultiplier[YIELD_COMMERCE] -= 40;
			}
		}
		
		int iIncome = 1 + kPlayer.getCommerceRate(COMMERCE_GOLD) + kPlayer.getCommerceRate(COMMERCE_RESEARCH) + std::max(0, kPlayer.getGoldPerTurn());
		int iExpenses = 1 + kPlayer.calculateInflatedCosts() - std::min(0, kPlayer.getGoldPerTurn());
		FAssert(iIncome > 0);
		
		int iRatio = (100 * iExpenses) / iIncome;
		//Gold -> Production Reduced To
		// 40- -> 100%
		// 60 -> 83%
		// 100 -> 28%
		// 110+ -> 14%
		m_aiSpecialYieldMultiplier[YIELD_PRODUCTION] += 100;
		if (iRatio > 60)
		{
			//Greatly decrease production weight 
			m_aiSpecialYieldMultiplier[YIELD_PRODUCTION] *= std::max(10, 120 - iRatio);
			m_aiSpecialYieldMultiplier[YIELD_PRODUCTION] /= 72;
		}
		else if (iRatio > 40)
		{
			//Slightly decrease production weight.
			m_aiSpecialYieldMultiplier[YIELD_PRODUCTION] *= 160 - iRatio;
			m_aiSpecialYieldMultiplier[YIELD_PRODUCTION] /= 120;
		}
		m_aiSpecialYieldMultiplier[YIELD_PRODUCTION] -= 100;
	}
}

int CvCityAI::AI_specialYieldMultiplier(YieldTypes eYield) const
{
	return m_aiSpecialYieldMultiplier[eYield];
}


int CvCityAI::AI_countNumBonuses(BonusTypes eBonus, bool bIncludeOurs, bool bIncludeNeutral, int iOtherCultureThreshold, bool bLand, bool bWater)
{
    CvPlot* pLoopPlot;
    BonusTypes eLoopBonus;
    int iI;
    int iCount = 0;
    for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
    {
        pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);
        
        if (pLoopPlot != NULL)
        {
        	if ((pLoopPlot->area() == area()) || (bWater && pLoopPlot->isWater()))
        	{
				eLoopBonus = pLoopPlot->getBonusType(getTeam());
				if (eLoopBonus != NO_BONUS)
				{
					if ((eBonus == NO_BONUS) || (eBonus == eLoopBonus))
					{
						if (bIncludeOurs && (pLoopPlot->getOwnerINLINE() == getOwnerINLINE()) && (pLoopPlot->getWorkingCity() == this))
						{
							iCount++;                    
						}
						else if (bIncludeNeutral && (!pLoopPlot->isOwned()))
						{
							iCount++;
						}
						else if ((iOtherCultureThreshold > 0) && (pLoopPlot->isOwned() && (pLoopPlot->getOwnerINLINE() != getOwnerINLINE())))
						{
							if ((pLoopPlot->getCulture(pLoopPlot->getOwnerINLINE()) - pLoopPlot->getCulture(getOwnerINLINE())) < iOtherCultureThreshold)
							{
								iCount++;
							}                        
						}
					}
				}
        	}
        }
    }
    
    
    return iCount;    
    
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      11/14/09                                jdog5000      */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
int CvCityAI::AI_countNumImprovableBonuses( bool bIncludeNeutral, TechTypes eExtraTech, bool bLand, bool bWater )
{
	CvPlot* pLoopPlot;
    BonusTypes eLoopBonus;
    int iI;
    int iCount = 0;
    for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
    {
        pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);
        
        if (pLoopPlot != NULL)
        {
        	if ((bLand && pLoopPlot->area() == area()) || (bWater && pLoopPlot->isWater()))
        	{
				eLoopBonus = pLoopPlot->getBonusType(getTeam());
				if (eLoopBonus != NO_BONUS)
				{
					if ( ((pLoopPlot->getOwnerINLINE() == getOwnerINLINE()) && (pLoopPlot->getWorkingCity() == this)) || (bIncludeNeutral && (!pLoopPlot->isOwned())))
					{
						for (int iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
						{
							BuildTypes eBuild = ((BuildTypes)iJ);
							
							if( eBuild != NO_BUILD && pLoopPlot->canBuild(eBuild, getOwnerINLINE()) )
							{
								ImprovementTypes eImp = (ImprovementTypes)GC.getBuildInfo(eBuild).getImprovement();

								if( eImp != NO_IMPROVEMENT && GC.getImprovementInfo(eImp).isImprovementBonusTrade(eLoopBonus) )
								{
									if( GET_PLAYER(getOwnerINLINE()).canBuild(pLoopPlot, eBuild) )
									{
										iCount++;
										break;
									}
									else if( (eExtraTech != NO_TECH) )
									{
										if (GC.getBuildInfo(eBuild).getTechPrereq() == eExtraTech)
										{
											iCount++;
											break;
										}
									}
								}
							}
						}
					}
				}
        	}
        }
    }
    
    
    return iCount;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

int CvCityAI::AI_playerCloseness(PlayerTypes eIndex, int iMaxDistance)
{
	FAssert(GET_PLAYER(eIndex).isAlive());

	if ((m_iCachePlayerClosenessTurn != GC.getGame().getGameTurn())
		|| (m_iCachePlayerClosenessDistance != iMaxDistance))
	{
		AI_cachePlayerCloseness(iMaxDistance);
	}
	
	return m_aiPlayerCloseness[eIndex];	
}

void CvCityAI::AI_cachePlayerCloseness(int iMaxDistance)
{
	PROFILE_FUNC();
	CvCity* pLoopCity;
	int iI;
	int iLoop;
	int iValue;
	int iTempValue;
	int iBestValue;

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						5/16/10				jdog5000		*/
/* 																				*/
/* 	General AI, closeness changes												*/
/********************************************************************************/	
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive() && 
			((GET_TEAM(getTeam()).isHasMet(GET_PLAYER((PlayerTypes)iI).getTeam()))))
		{
			iValue = 0;
			iBestValue = 0;
			for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				if( pLoopCity == this )
				{
					continue;
				}

				int iDistance = stepDistance(getX_INLINE(), getY_INLINE(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE());
				
				if (area() != pLoopCity->area() )
				{
					iDistance += 1;
					iDistance /= 2;
				}
				if (iDistance <= iMaxDistance)
				{
					if ( getArea() == pLoopCity->getArea() )
					{
						int iPathDistance = GC.getMap().calculatePathDistance(plot(), pLoopCity->plot());
						if (iPathDistance > 0)
						{
							iDistance = iPathDistance;
						}
					}
					if (iDistance <= iMaxDistance)
					{
						// Weight by population of both cities, not just pop of other city
						//iTempValue = 20 + 2*pLoopCity->getPopulation();
						iTempValue = 20 + pLoopCity->getPopulation() + getPopulation();

						iTempValue *= (1 + (iMaxDistance - iDistance));
						iTempValue /= (1 + iMaxDistance);
						
						//reduce for small islands.
						int iAreaCityCount = pLoopCity->area()->getNumCities();
						iTempValue *= std::min(iAreaCityCount, 5);
						iTempValue /= 5;
						if (iAreaCityCount < 3)
						{
							iTempValue /= 2;
						}
						
						if (pLoopCity->isBarbarian())
						{
							iTempValue /= 4;
						}
						
						iValue += iTempValue;					
						iBestValue = std::max(iBestValue, iTempValue);
					}
				}
			}
			m_aiPlayerCloseness[iI] = (iBestValue + iValue / 4);
		}
	}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END							        */
/********************************************************************************/
	
	m_iCachePlayerClosenessTurn = GC.getGame().getGameTurn();	
	m_iCachePlayerClosenessDistance = iMaxDistance;
}

// K-Mod
// return true if there is an adjacent plot not owned by us.
bool CvCityAI::AI_isFrontlineCity() const
{
	for (int i = 0; i < NUM_DIRECTION_TYPES; i++)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), (DirectionTypes)i);

		if (pAdjacentPlot && !pAdjacentPlot->isWater() && pAdjacentPlot->getTeam() != getTeam())
			return true;
	}

	return false;
}
// K-Mod end

int CvCityAI::AI_cityThreat(bool bDangerPercent)
{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/04/10                                jdog5000      */
/*                                                                                              */
/* War tactics AI                                                                               */
/************************************************************************************************/
	PROFILE_FUNC();
	int iValue = 0;
	const CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // K-Mod
	bool bCrushStrategy = kOwner.AI_isDoStrategy(AI_STRATEGY_CRUSH);

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI); // K-Mod
		//if ((iI != getOwner()) && GET_PLAYER((PlayerTypes)iI).isAlive())
		if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() != getTeam()) // K-Mod
		{
			int iTempValue = AI_playerCloseness((PlayerTypes)iI, DEFAULT_PLAYER_CLOSENESS);

			if (iTempValue > 0)
			{
				// K-Mod
				for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					CvPlot* pLoopPlot = getCityIndexPlot(iJ);
					if (pLoopPlot && pLoopPlot->getOwnerINLINE() == iI)
					{
						iTempValue += (stepDistance(getX_INLINE(), getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()) <= 1) ? 2 : 1;
					}
				}
				// K-Mod end

				if (bCrushStrategy && GET_TEAM(getTeam()).AI_getWarPlan(kLoopPlayer.getTeam()) != NO_WARPLAN)
				{
					iTempValue *= 400;
				}
				else if (atWar(getTeam(), kLoopPlayer.getTeam()))
				{
					iTempValue *= 300;
				}
				// Beef up border security before starting war, but not too much
				else if ( GET_TEAM(getTeam()).AI_getWarPlan(kLoopPlayer.getTeam()) != NO_WARPLAN )
				{
					iTempValue *= 180;
				}
				// Extra trust of/for Vassals, regardless of relations
				else if ( GET_TEAM(kLoopPlayer.getTeam()).isVassal(getTeam()) ||
							GET_TEAM(getTeam()).isVassal(kLoopPlayer.getTeam()))
				{
					iTempValue *= 30;
				}
				else
				{
					switch (kOwner.AI_getAttitude((PlayerTypes)iI))
					{
					case ATTITUDE_FURIOUS:
						iTempValue *= 180;
						break;

					case ATTITUDE_ANNOYED:
						iTempValue *= 130;
						break;

					case ATTITUDE_CAUTIOUS:
						iTempValue *= 100;
						break;

					case ATTITUDE_PLEASED:
						iTempValue *= 50;
						break;

					case ATTITUDE_FRIENDLY:
						iTempValue *= 20;
						break;

					default:
						FAssert(false);
						break;
					}

					if (bCrushStrategy)
					{
						iTempValue /= 2;
					}
				}

				// Beef up border security next to powerful rival. (K-Mod moved this from the block above)
				if( kLoopPlayer.getPower() > kOwner.getPower() )
				{
					//iTempValue *= std::min( 400, (100 * GET_PLAYER((PlayerTypes)iI).getPower())/std::max(1, GET_PLAYER(getOwnerINLINE()).getPower()) );
					// K-Mod. Use exclude population power.
					int iLoopPower = kLoopPlayer.getPower() - getPopulationPower(kLoopPlayer.getTotalPopulation());
					int iOurPower = kOwner.getPower() - getPopulationPower(kOwner.getTotalPopulation());
					iTempValue *= range(100 * iLoopPower/std::max(1, iOurPower), 100, 400); // K-Mod
					iTempValue /= 100;
				}

				iTempValue /= 100;
				iValue += iTempValue;
			}
		}
	}
	
	if (isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
	{
		int iCurrentEra = kOwner.getCurrentEra();
		iValue += std::max(0, ((10 * iCurrentEra) / 3) - 6); //there are better ways to do this
	}
	
	iValue += getNumActiveWorldWonders() * 5;

	if (kOwner.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3))
	{
		iValue += 5;
		iValue += getCommerceRateModifier(COMMERCE_CULTURE) / 20;
		if (getCultureLevel() >= (GC.getNumCultureLevelInfos() - 2))
		{
			iValue += 20;
			if (getCultureLevel() >= (GC.getNumCultureLevelInfos() - 1))
			{
				iValue += 30;
			}
		}
	}
	
	iValue += 3 * kOwner.AI_getPlotDanger(plot(), 3, false); // was 2 *
	
	return iValue;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
}

//Workers have/needed is not intended to be a strict
//target but rather an indication.
//if needed is at least 1 that means a worker
//will be doing something useful
int CvCityAI::AI_getWorkersHave()
{
	return m_iWorkersHave;	
}

int CvCityAI::AI_getWorkersNeeded()
{
	return m_iWorkersNeeded;
}

void CvCityAI::AI_changeWorkersHave(int iChange)
{
	m_iWorkersHave += iChange;
	//FAssert(m_iWorkersHave >= 0);
	m_iWorkersHave = std::max(0, m_iWorkersHave);
}
	
//This needs to be serialized for human workers.
void CvCityAI::AI_updateWorkersNeededHere()
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	
	short aiYields[NUM_YIELD_TYPES];
	
	int iWorkersNeeded = 0;
	int iWorkersHave = 0;
	int iUnimprovedWorkedPlotCount = 0;
	int iUnimprovedUnworkedPlotCount = 0;
	int iWorkedUnimprovableCount = 0;
	int iImprovedUnworkedPlotCount = 0;
	
	int iSpecialCount = 0;
	
	int iWorstWorkedPlotValue = MAX_INT;
	int iBestUnworkedPlotValue = 0;
	
	iWorkersHave = 0;
	
	if (getProductionUnit() != NO_UNIT)
	{
		if (getProductionUnitAI() == UNITAI_WORKER)
		{
			if (getProductionTurnsLeft() <= 2)
			{
				iWorkersHave++;
			}
		}
	}
	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		pLoopPlot = getCityIndexPlot(iI);

		if (NULL != pLoopPlot && pLoopPlot->getWorkingCity() == this)
		{
			//if (pLoopPlot->getArea() == getArea())
			{
				//How slow is this? It could be almost NUM_CITY_PLOT times faster
				//by iterating groups and seeing if the plot target lands in this city
				//but since this is only called once/turn i'm not sure it matters.
				iWorkersHave += (GET_PLAYER(getOwnerINLINE()).AI_plotTargetMissionAIs(pLoopPlot, MISSIONAI_BUILD));

				iWorkersHave += pLoopPlot->plotCount(PUF_isUnitAIType, UNITAI_WORKER, -1, getOwner(), getTeam(), PUF_isNoMission, -1, -1);
				if (iI != CITY_HOME_PLOT)
				{
					if (pLoopPlot->getImprovementType() == NO_IMPROVEMENT)
					{
						if (pLoopPlot->isBeingWorked())
						{
							//if (AI_getBestBuild(iI) != NO_BUILD)
							if (AI_getBestBuild(iI) != NO_BUILD && pLoopPlot->getArea() == getArea()) // K-Mod
							{
								iUnimprovedWorkedPlotCount++;
							}
							else
							{
								iWorkedUnimprovableCount++;
							}
						}
						else
						{
							//if (AI_getBestBuild(iI) != NO_BUILD)
							if (AI_getBestBuild(iI) != NO_BUILD && pLoopPlot->getArea() == getArea()) // K-Mod
							{
								iUnimprovedUnworkedPlotCount++;
							}
						}
					}
					else
					{
						if (!pLoopPlot->isBeingWorked())
						{
							iImprovedUnworkedPlotCount++;
						}
					}

					for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
					{
						aiYields[iJ] = pLoopPlot->getYield((YieldTypes)iJ);
					}

					if (pLoopPlot->isBeingWorked())
					{
						int iPlotValue = AI_yieldValue(aiYields, NULL, false, false, false, false, true, true);
						iWorstWorkedPlotValue = std::min(iWorstWorkedPlotValue, iPlotValue);					
					}
					else
					{
						int iPlotValue = AI_yieldValue(aiYields, NULL, false, false, false, false, true, true);
						iBestUnworkedPlotValue = std::max(iBestUnworkedPlotValue, iPlotValue);
					}
				}
			}
		}
	}
	//specialists?

	//iUnimprovedWorkedPlotCount += std::min(iUnimprovedUnworkedPlotCount, iWorkedUnimprovableCount) / 2;
	// K-Mod
	{
		int iPopDelta = AI_getTargetPopulation() - getPopulation();
		int iFutureWork = std::max(0, iWorkedUnimprovableCount + range((iPopDelta+1)/2, 0, 3) - iImprovedUnworkedPlotCount);
		iUnimprovedWorkedPlotCount += (std::min(iUnimprovedUnworkedPlotCount, iFutureWork)+1) / 2;
	}
	// K-Mod end

	iWorkersNeeded += 2 * iUnimprovedWorkedPlotCount;

	int iBestPotentialPlotValue = -1;
	if (iWorstWorkedPlotValue != MAX_INT)
	{
		//Add an additional citizen to account for future growth.
		int iBestPlot = -1;
		SpecialistTypes eBestSpecialist = NO_SPECIALIST;
		
		if (angryPopulation() == 0)
		{
			AI_addBestCitizen(true, true, &iBestPlot, &eBestSpecialist);
		}
		
		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			if (iI != CITY_HOME_PLOT)
			{
				pLoopPlot = getCityIndexPlot(iI);

				if (NULL != pLoopPlot && pLoopPlot->getWorkingCity() == this && pLoopPlot->getArea() == getArea())
				{
					if (AI_getBestBuild(iI) != NO_BUILD)
					{
						for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
						{
							aiYields[iJ] = pLoopPlot->getYieldWithBuild(m_aeBestBuild[iI], (YieldTypes)iJ, true);
						}

						int iPlotValue = AI_yieldValue(aiYields, NULL, false, false, false, false, true, true);
						ImprovementTypes eImprovement = (ImprovementTypes)GC.getBuildInfo(AI_getBestBuild(iI)).getImprovement();
						if (eImprovement != NO_IMPROVEMENT)
						{
							if ((getImprovementFreeSpecialists(eImprovement) > 0) || (GC.getImprovementInfo(eImprovement).getHappiness() > 0))
							{
								iSpecialCount ++;
							}
						}
						iBestPotentialPlotValue = std::max(iBestPotentialPlotValue, iPlotValue);
					}
				}
			}
		}

		if (iBestPlot != -1)
		{
			setWorkingPlot(iBestPlot, false);
		}
		if (eBestSpecialist != NO_SPECIALIST)
		{
			changeSpecialistCount(eBestSpecialist, -1);
		}

		if (iBestPotentialPlotValue > iWorstWorkedPlotValue)
		{
			iWorkersNeeded += 2;
		}
	}

	iWorkersNeeded += (std::max(0, iUnimprovedWorkedPlotCount - 1) * (GET_PLAYER(getOwnerINLINE()).getCurrentEra())) / 3;

	if (GET_PLAYER(getOwnerINLINE()).AI_isFinancialTrouble())
	{
		iWorkersNeeded *= 3;
		iWorkersNeeded /= 2;
	}

	if (iWorkersNeeded > 0)
	{
		iWorkersNeeded++;
		iWorkersNeeded = std::max(1, iWorkersNeeded / 3);
	}

	int iSpecialistExtra = std::min((getSpecialistPopulation() - totalFreeSpecialists()), iUnimprovedUnworkedPlotCount);
	iSpecialistExtra -= iImprovedUnworkedPlotCount;
	
	iWorkersNeeded += std::max(0, 1 + iSpecialistExtra) / 2;
	
	if (iWorstWorkedPlotValue <= iBestUnworkedPlotValue && iBestUnworkedPlotValue >= iBestPotentialPlotValue)
	{
		iWorkersNeeded /= 2;
	}
	if (angryPopulation(1) > 0)
	{
		iWorkersNeeded++;
		iWorkersNeeded /= 2;
	}
	
	iWorkersNeeded += (iSpecialCount + 1) / 2;
	
	iWorkersNeeded = std::max((iUnimprovedWorkedPlotCount + 1) / 2, iWorkersNeeded);
	
	m_iWorkersNeeded = iWorkersNeeded;
	m_iWorkersHave = iWorkersHave;
}

BuildingTypes CvCityAI::AI_bestAdvancedStartBuilding(int iPass)
{
	int iFocusFlags = 0;
	if (iPass >= 0)
	{
		iFocusFlags |= BUILDINGFOCUS_FOOD;
	}
	if (iPass >= 1)
	{
		iFocusFlags |= BUILDINGFOCUS_PRODUCTION;
	}
	if (iPass >= 2)
	{
		iFocusFlags |= BUILDINGFOCUS_EXPERIENCE;
	}
	if (iPass >= 3)
	{
		iFocusFlags |= (BUILDINGFOCUS_HAPPY | BUILDINGFOCUS_HEALTHY);
	}
	if (iPass >= 4)
	{
		iFocusFlags |= (BUILDINGFOCUS_GOLD | BUILDINGFOCUS_RESEARCH | BUILDINGFOCUS_MAINTENANCE);
		if (!GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE))
		{
			iFocusFlags |= BUILDINGFOCUS_ESPIONAGE;
		}
	}
			
	return AI_bestBuildingThreshold(iFocusFlags, 0, std::max(0, 20 - iPass * 5));
}

//
//
//
void CvCityAI::read(FDataStreamBase* pStream)
{
	CvCity::read(pStream);

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iEmphasizeAvoidGrowthCount);
	pStream->Read(&m_iEmphasizeGreatPeopleCount);
	pStream->Read(&m_bAssignWorkDirty);
	pStream->Read(&m_bChooseProductionDirty);

	pStream->Read((int*)&m_routeToCity.eOwner);
	pStream->Read(&m_routeToCity.iID);

	pStream->Read(NUM_YIELD_TYPES, m_aiEmphasizeYieldCount);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiEmphasizeCommerceCount);
	pStream->Read(&m_bForceEmphasizeCulture);
	pStream->Read(NUM_CITY_PLOTS, m_aiBestBuildValue);
	pStream->Read(NUM_CITY_PLOTS, (int*)m_aeBestBuild);
	pStream->Read(GC.getNumEmphasizeInfos(), m_pbEmphasize);
	pStream->Read(NUM_YIELD_TYPES, m_aiSpecialYieldMultiplier);
	pStream->Read(&m_iCachePlayerClosenessTurn);
	pStream->Read(&m_iCachePlayerClosenessDistance);
	pStream->Read(MAX_PLAYERS, m_aiPlayerCloseness);
	pStream->Read(&m_iNeededFloatingDefenders);
	pStream->Read(&m_iNeededFloatingDefendersCacheTurn);
	pStream->Read(&m_iWorkersNeeded);
	pStream->Read(&m_iWorkersHave);
	// K-Mod
	if (uiFlag >= 1)
	{
		FAssert(m_aiConstructionValue.size() == GC.getNumBuildingClassInfos());
		pStream->Read(GC.getNumBuildingClassInfos(), &m_aiConstructionValue[0]);
	}
	// K-Mod end
}

//
//
//
void CvCityAI::write(FDataStreamBase* pStream)
{
	CvCity::write(pStream);

	uint uiFlag=1;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iEmphasizeAvoidGrowthCount);
	pStream->Write(m_iEmphasizeGreatPeopleCount);
	pStream->Write(m_bAssignWorkDirty);
	pStream->Write(m_bChooseProductionDirty);

	pStream->Write(m_routeToCity.eOwner);
	pStream->Write(m_routeToCity.iID);

	pStream->Write(NUM_YIELD_TYPES, m_aiEmphasizeYieldCount);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiEmphasizeCommerceCount);
	pStream->Write(m_bForceEmphasizeCulture);
	pStream->Write(NUM_CITY_PLOTS, m_aiBestBuildValue);
	pStream->Write(NUM_CITY_PLOTS, (int*)m_aeBestBuild);
	pStream->Write(GC.getNumEmphasizeInfos(), m_pbEmphasize);
	pStream->Write(NUM_YIELD_TYPES, m_aiSpecialYieldMultiplier);
	pStream->Write(m_iCachePlayerClosenessTurn);
	pStream->Write(m_iCachePlayerClosenessDistance);
	pStream->Write(MAX_PLAYERS, m_aiPlayerCloseness);
	pStream->Write(m_iNeededFloatingDefenders);
	pStream->Write(m_iNeededFloatingDefendersCacheTurn);
	pStream->Write(m_iWorkersNeeded);
	pStream->Write(m_iWorkersHave);
	// K-Mod (note: cache needs to be saved, otherwise players who join mid-turn might go out of sync when the cache is used)
	pStream->Write(GC.getNumBuildingClassInfos(), &m_aiConstructionValue[0]); // uiFlag >= 1
	// K-Mod end
}

// K-Mod
void CvCityAI::AI_ClearConstructionValueCache()
{
	m_aiConstructionValue.assign(GC.getNumBuildingClassInfos(), -1);
}

/***
**** K-Mod, 13/sep/10, Karadoc
**** Some AI code which I might use later.
****
***/
/*
void CvCityAI::GetTotalCitizenYields(short* piYields, int iCitizens)
{
	// assume we have iCitizens number of citiziens... how much yield are we getting?
}

int CvCityAI::GetPowerImprovement(int eUnit)
{
	// how much would it help our military to have access to "eUnit"?
}
*/