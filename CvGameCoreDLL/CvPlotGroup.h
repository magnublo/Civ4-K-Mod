#pragma once

// plotGroup.h

#ifndef CIV4_PLOT_GROUP_H
#define CIV4_PLOT_GROUP_H

//#include "CvStructs.h"
#include "LinkedList.h"

class CvPlot;
class CvPlotGroup
{

public:

	CvPlotGroup();
	virtual ~CvPlotGroup();

	void init(int iID, PlayerTypes eOwner, CvPlot* pPlot);
	void uninit();
	void reset(int iID = 0, PlayerTypes eOwner = NO_PLAYER, bool bConstructorCall=false);

	void addPlot(CvPlot* pPlot);
	void removePlot(CvPlot* pPlot);
	void recalculatePlots();														

	int getID() const;
	void setID(int iID);

	PlayerTypes getOwner() const;
#ifdef _USRDLL
	inline PlayerTypes getOwnerINLINE() const
	{
		return m_eOwner;
	}
#endif
	int getNumBonuses(BonusTypes eBonus) const;
	bool hasBonus(BonusTypes eBonus);										
	void changeNumBonuses(BonusTypes eBonus, int iChange);

	void insertAtEndPlots(XYCoords xy);			
	CLLNode<XYCoords>* deletePlotsNode(CLLNode<XYCoords>* pNode);
	CLLNode<XYCoords>* nextPlotsNode(CLLNode<XYCoords>* pNode);
	int getLengthPlots();
	CLLNode<XYCoords>* headPlotsNode();

	// for serialization
	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);

protected:
	int countConnectedPlots(const CvPlot& kStart) const; // perf
public:
	// perf: the plots (by index) that a blockade just took out of each team's trade network, or NULL; while set,
	// recalculatePlots knows that a group containing one of them has to be rebuilt (see there)
	static void setLostNetworkPlots(const std::vector<int>* paiLostPlotsByTeam);
protected:

	int m_iID;

	PlayerTypes m_eOwner;

	int* m_paiNumBonuses;

	CLinkList<XYCoords> m_plots;
};

#endif
