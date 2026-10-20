// plotGroup.cpp

#include "CvGameCoreDLL.h"
#include "CvPlotGroup.h"
#include "CvPlot.h"
#include "CvGlobals.h"
#include "CvPlayerAI.h"
#include "CvMap.h"
#include "CvCity.h"
#include "CvDLLFAStarIFaceBase.h"
#include "FProfiler.h"

// Public Functions...

CvPlotGroup::CvPlotGroup()
{
	m_paiNumBonuses = NULL;

	reset(0, NO_PLAYER, true);
}


CvPlotGroup::~CvPlotGroup()
{
	uninit();
}


void CvPlotGroup::init(int iID, PlayerTypes eOwner, CvPlot* pPlot)
{
	//--------------------------------
	// Init saved data
	reset(iID, eOwner);

	//--------------------------------
	// Init non-saved data

	//--------------------------------
	// Init other game data
	addPlot(pPlot);
}


void CvPlotGroup::uninit()
{
	SAFE_DELETE_ARRAY(m_paiNumBonuses);

	m_plots.clear();
}

// FUNCTION: reset()
// Initializes data members that are serialized.
void CvPlotGroup::reset(int iID, PlayerTypes eOwner, bool bConstructorCall)
{
	int iI;

	//--------------------------------
	// Uninit class
	uninit();

	m_iID = iID;
	m_eOwner = eOwner;

	if (!bConstructorCall)
	{
		FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated in CvPlotGroup::reset");
		m_paiNumBonuses = new int [GC.getNumBonusInfos()];
		for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
		{
			m_paiNumBonuses[iI] = 0;
		}
	}
}


void CvPlotGroup::addPlot(CvPlot* pPlot)
{
	XYCoords xy;

	xy.iX = pPlot->getX_INLINE();
	xy.iY = pPlot->getY_INLINE();

	insertAtEndPlots(xy);

	pPlot->setPlotGroup(getOwnerINLINE(), this);
}


void CvPlotGroup::removePlot(CvPlot* pPlot)
{
	CLLNode<XYCoords>* pPlotNode;

	pPlotNode = headPlotsNode();

	while (pPlotNode != NULL)
	{
		if (GC.getMapINLINE().plotSorenINLINE(pPlotNode->m_data.iX, pPlotNode->m_data.iY) == pPlot)
		{
			pPlot->setPlotGroup(getOwnerINLINE(), NULL);

			pPlotNode = deletePlotsNode(pPlotNode); // can delete this PlotGroup...
			break;
		}
		else
		{
			pPlotNode = nextPlotsNode(pPlotNode);
		}
	}
}


namespace
{
	const std::vector<int>* g_paiLostNetworkPlots = NULL; // perf: indexed by team
}

void CvPlotGroup::setLostNetworkPlots(const std::vector<int>* paiLostPlotsByTeam)
{
	g_paiLostNetworkPlots = paiLostPlotsByTeam;
}

void CvPlotGroup::recalculatePlots()
{
	PROFILE_FUNC();

	CLLNode<XYCoords>* pPlotNode;
	CvPlot* pPlot;
	CLinkList<XYCoords> oldPlotGroup;
	XYCoords xy;
	PlayerTypes eOwner;
	int iCount;

	eOwner = getOwnerINLINE();

	pPlotNode = headPlotsNode();

	if (pPlotNode != NULL)
	{
		pPlot = GC.getMapINLINE().plotSorenINLINE(pPlotNode->m_data.iX, pPlotNode->m_data.iY);

		/* original bts code
		iCount = 0;

		gDLL->getFAStarIFace()->SetData(&GC.getPlotGroupFinder(), &iCount);
		gDLL->getFAStarIFace()->GeneratePath(&GC.getPlotGroupFinder(), pPlot->getX_INLINE(), pPlot->getY_INLINE(), -1, -1, false, eOwner); */
		// perf: The count is certainly short of the group's length if a plot of the group other than the first is no
		// longer trade network: the search never enters such a plot. Then the group is rebuilt without counting.
		// (The plots a blockade took out of the network are named by CvUnit::updatePlunder.)
		bool bKnownShort = false;
		if (g_paiLostNetworkPlots != NULL)
		{
			TeamTypes eTeam = GET_PLAYER(eOwner).getTeam();
			const std::vector<int>& aiLost = g_paiLostNetworkPlots[eTeam];
			for (size_t i = 0; i < aiLost.size() && !bKnownShort; i++)
			{
				CvPlot* pLostPlot = GC.getMapINLINE().plotByIndexINLINE(aiLost[i]);
				if (pLostPlot != pPlot && pLostPlot->getPlotGroup(eOwner) == this && !pLostPlot->isTradeNetwork(eTeam))
					bKnownShort = true;
			}
		}

		// perf: the same count without the exe's pathfinder
		iCount = bKnownShort ? -1 : countConnectedPlots(*pPlot);
#ifdef PERF_VERIFY
		int iExeCount = 0;
		gDLL->getFAStarIFace()->SetData(&GC.getPlotGroupFinder(), &iExeCount);
		gDLL->getFAStarIFace()->GeneratePath(&GC.getPlotGroupFinder(), pPlot->getX_INLINE(), pPlot->getY_INLINE(), -1, -1, false, eOwner);
		if (bKnownShort ? iExeCount == getLengthPlots() : iExeCount != iCount)
			gDLL->logMsg("perf_verify.log", CvString::format("recalculatePlots: exe count %d, DLL count %d (player %d, plot %d,%d, %d plots)", iExeCount, iCount, eOwner, pPlot->getX_INLINE(), pPlot->getY_INLINE(), getLengthPlots()).c_str());
#endif

		if (iCount == getLengthPlots())
		{
			return;
		}
	}

	{
		PROFILE("CvPlotGroup::recalculatePlots update");

		oldPlotGroup.clear();

		pPlotNode = headPlotsNode();

		while (pPlotNode != NULL)
		{
			PROFILE("CvPlotGroup::recalculatePlots update 1");

			pPlot = GC.getMapINLINE().plotSorenINLINE(pPlotNode->m_data.iX, pPlotNode->m_data.iY);

			FAssertMsg(pPlot != NULL, "Plot is not assigned a valid value");

			xy.iX = pPlot->getX_INLINE();
			xy.iY = pPlot->getY_INLINE();

			oldPlotGroup.insertAtEnd(xy);

			pPlot->setPlotGroup(eOwner, NULL);

			pPlotNode = deletePlotsNode(pPlotNode); // will delete this PlotGroup...
		}

		pPlotNode = oldPlotGroup.head();

		while (pPlotNode != NULL)
		{
			PROFILE("CvPlotGroup::recalculatePlots update 2");

			pPlot = GC.getMapINLINE().plotSorenINLINE(pPlotNode->m_data.iX, pPlotNode->m_data.iY);

			FAssertMsg(pPlot != NULL, "Plot is not assigned a valid value");

			pPlot->updatePlotGroup(eOwner, true);

			pPlotNode = oldPlotGroup.deleteNode(pPlotNode);
		}
	}
}


// perf: the number of plots reachable from kStart through plots of this group, as counted by the exe's plot group
// finder (FAStar with plotGroupValid and countPlotGroup) that recalculatePlots used: a plot is reached from an
// adjacent reached plot if both are in the same group and it is trade network connected to it. A plain breadth-first
// search finds the same plots without the exe's pathfinder overhead, which was the most expensive part of the check.
int CvPlotGroup::countConnectedPlots(const CvPlot& kStart) const
{
	PROFILE_FUNC();

	static std::vector<unsigned> aiReached; // search in which each plot was reached
	static std::vector<int> aiQueue;
	static unsigned iSearch = 0;

	const CvMap& kMap = GC.getMapINLINE();
	if ((int)aiReached.size() != kMap.numPlotsINLINE())
	{
		aiReached.assign(kMap.numPlotsINLINE(), 0);
		iSearch = 0;
	}
	if (++iSearch == 0)
	{
		std::fill(aiReached.begin(), aiReached.end(), 0);
		iSearch = 1;
	}

	PlayerTypes eOwner = getOwnerINLINE();
	TeamTypes eTeam = GET_PLAYER(eOwner).getTeam();

	aiQueue.clear();
	aiQueue.push_back(kMap.plotNumINLINE(kStart.getX_INLINE(), kStart.getY_INLINE()));
	aiReached[aiQueue[0]] = iSearch;
	for (size_t i = 0; i < aiQueue.size(); i++)
	{
		CvPlot* pPlot = kMap.plotByIndexINLINE(aiQueue[i]);
		CvPlotGroup* pPlotGroup = pPlot->getPlotGroup(eOwner);
		for (int iDir = 0; iDir < NUM_DIRECTION_TYPES; iDir++)
		{
			CvPlot* pAdjacentPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), (DirectionTypes)iDir);
			if (pAdjacentPlot == NULL)
				continue;
			int iAdjacent = kMap.plotNumINLINE(pAdjacentPlot->getX_INLINE(), pAdjacentPlot->getY_INLINE());
			if (aiReached[iAdjacent] == iSearch)
				continue;
			// plotGroupValid(pPlot -> pAdjacentPlot)
			if (pAdjacentPlot->getPlotGroup(eOwner) == pPlotGroup && pAdjacentPlot->isTradeNetwork(eTeam) &&
				pAdjacentPlot->isTradeNetworkConnected(pPlot, eTeam))
			{
				aiReached[iAdjacent] = iSearch;
				aiQueue.push_back(iAdjacent);
			}
		}
	}
	return (int)aiQueue.size();
}


int CvPlotGroup::getID() const
{
	return m_iID;
}


void CvPlotGroup::setID(int iID)
{
	m_iID = iID;
}


PlayerTypes CvPlotGroup::getOwner() const
{
	return getOwnerINLINE();
}


int CvPlotGroup::getNumBonuses(BonusTypes eBonus) const
{
	FAssertMsg(eBonus >= 0, "eBonus is expected to be non-negative (invalid Index)");
	FAssertMsg(eBonus < GC.getNumBonusInfos(), "eBonus is expected to be within maximum bounds (invalid Index)");
	return m_paiNumBonuses[eBonus];
}


bool CvPlotGroup::hasBonus(BonusTypes eBonus)
{
	return(getNumBonuses(eBonus) > 0);
}


void CvPlotGroup::changeNumBonuses(BonusTypes eBonus, int iChange)
{
	FAssertMsg(eBonus >= 0, "eBonus is expected to be non-negative (invalid Index)");
	FAssertMsg(eBonus < GC.getNumBonusInfos(), "eBonus is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		//iOldNumBonuses = getNumBonuses(eBonus);

		m_paiNumBonuses[eBonus] = (m_paiNumBonuses[eBonus] + iChange);

		//FAssertMsg(m_paiNumBonuses[eBonus] >= 0, "m_paiNumBonuses[eBonus] is expected to be non-negative (invalid Index)"); XXX

		// K-Mod note, m_paiNumBonuses[eBonus] is often temporarily negative while plot groups are being updated.
		// It's an unfortuante side effect of the way the update is implemented. ... and so this assert is invalid.
		// (This isn't my fault. I haven't changed it. It has always been like this.)

		/* original bts code
		CLLNode<XYCoords>* pPlotNode = headPlotsNode();

		while (pPlotNode != NULL)
		{
			CvCity* pCity = GC.getMapINLINE().plotSorenINLINE(pPlotNode->m_data.iX, pPlotNode->m_data.iY)->getPlotCity();

			if (pCity != NULL)
			{
				if (pCity->getOwnerINLINE() == getOwnerINLINE())
				{
					pCity->changeNumBonuses(eBonus, iChange);
				}
			}

			pPlotNode = nextPlotsNode(pPlotNode);
		} */
		// perf: The cities to update are the owner's cities that stand on a plot of this group, as in the plot walk
		// above. A plot's group changes right before it leaves the old group's list and right after it joins the new
		// one's, so "the plot's group is this one" is "the plot is in this group's list" whenever this is called.
		// The city must also be the plot's city: a new city is in its owner's list a little before it is put on its
		// plot, and a dying one a little after it is taken off; the plot walk never reaches those. (Without that
		// check, a 200-turn autoplay game left K-Mod's game at turn 113; found on 2026-10-20.) Going through the
		// owner's cities is much faster than through every plot of the group, thousands for a big trade network.
		// Only the order of the city updates changes, and each one changes just that city (and player totals).
		CvPlayer& kOwner = GET_PLAYER(getOwnerINLINE());
#ifdef PERF_VERIFY
		std::vector<int> aiWalkCities, aiCities;
		for (CLLNode<XYCoords>* pPlotNode = headPlotsNode(); pPlotNode != NULL; pPlotNode = nextPlotsNode(pPlotNode))
		{
			CvCity* pCity = GC.getMapINLINE().plotSorenINLINE(pPlotNode->m_data.iX, pPlotNode->m_data.iY)->getPlotCity();
			if (pCity != NULL && pCity->getOwnerINLINE() == getOwnerINLINE())
				aiWalkCities.push_back(pCity->getID());
		}
#endif
		int iLoop;
		for (CvCity* pLoopCity = kOwner.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kOwner.nextCity(&iLoop))
		{
			CvPlot* pCityPlot = pLoopCity->plot();
			if (pCityPlot->getPlotGroup(getOwnerINLINE()) == this && pCityPlot->getPlotCity() == pLoopCity)
			{
#ifdef PERF_VERIFY
				aiCities.push_back(pLoopCity->getID());
#endif
				pLoopCity->changeNumBonuses(eBonus, iChange);
			}
		}
#ifdef PERF_VERIFY
		std::sort(aiWalkCities.begin(), aiWalkCities.end());
		std::sort(aiCities.begin(), aiCities.end());
		if (aiWalkCities != aiCities)
			gDLL->logMsg("perf_verify.log", CvString::format("changeNumBonuses: %d cities, the plot walk %d (player %d, bonus %d)", (int)aiCities.size(), (int)aiWalkCities.size(), getOwnerINLINE(), eBonus).c_str());
#endif
	}
}


void CvPlotGroup::insertAtEndPlots(XYCoords xy)
{
	m_plots.insertAtEnd(xy);
}


CLLNode<XYCoords>* CvPlotGroup::deletePlotsNode(CLLNode<XYCoords>* pNode)
{
	CLLNode<XYCoords>* pPlotNode;

	pPlotNode = m_plots.deleteNode(pNode);

	if (getLengthPlots() == 0)
	{
		GET_PLAYER(getOwnerINLINE()).deletePlotGroup(getID());
	}

  return pPlotNode;
}


CLLNode<XYCoords>* CvPlotGroup::nextPlotsNode(CLLNode<XYCoords>* pNode)
{
	return m_plots.next(pNode);
}


int CvPlotGroup::getLengthPlots()
{
	return m_plots.getLength();
}


CLLNode<XYCoords>* CvPlotGroup::headPlotsNode()
{
	return m_plots.head();
}


void CvPlotGroup::read(FDataStreamBase* pStream)
{
	// Init saved data
	reset();

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iID);

	pStream->Read((int*)&m_eOwner);

	FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated in CvPlotGroup::read");
	pStream->Read(GC.getNumBonusInfos(), m_paiNumBonuses);

	m_plots.Read(pStream);
}


void CvPlotGroup::write(FDataStreamBase* pStream)
{
	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iID);

	pStream->Write(m_eOwner);

	FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated in CvPlotGroup::write");
	pStream->Write(GC.getNumBonusInfos(), m_paiNumBonuses);

	m_plots.Write(pStream);
}
