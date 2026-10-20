#pragma once

// RI art: Realism Invictus graphics. The unit and building art of the Realism Invictus mod, picked per civilization as
// RI does, shown instead of K-Mod's while the option is on (BUG option "Realism Invictus graphics", which calls
// gc.setRealismInvictusArt). It reaches the exe through CvUnit's art functions, CvPlayer::getUnitArtInfo and
// getUnitButton (units), CvCity::getBuildingArtInfo and CvGame::getBuildingArtInfo (buildings) and
// CvInitCore::getArtStyle (the city style: houses, walls, RI's regional versions of buildings; setArtStyle never
// stores one of RI's styles).
// The data is xml/Art/RI_CIV4ArtInfos.xml plus RI's art defines (types prefixed RI_), all written by
// tools/ri_art/port_ri_art.py; without these files (a mod folder given only this DLL) nothing is loaded and the option
// does nothing.
// Graphics only: nothing the game logic sees depends on the option, so players of a multiplayer game may set it
// differently. Battles are planned with each unit's figure count (its group size), so RI's figure groups are fitted
// to K-Mod's group sizes, and CvUnit::isRanged keeps looking at K-Mod's art.

#ifndef CvRIArt_h
#define CvRIArt_h

class CvArtInfoUnit;
class CvArtInfoBuilding;
class CvXMLLoadUtility;
class FXml;

class CvRIArt
{
public:
	enum { NUM_TIERS = 7 }; // RI's art eras: Early, Classical, Middle, Renn, Industrial, Late, Future (game eras 0-6)

	struct UnitMeshGroup
	{
		int iRequired;
		CvString aszTags[NUM_TIERS]; // art define per art era; "" = none
	};
	struct UnitArt
	{
		float fMaxSpeed;
		float fPadTime;
		std::vector<UnitMeshGroup> aGroups;
	};

	static CvRIArt& get() { return m_kInstance; }

	void read(CvXMLLoadUtility* pXML); // after the units, buildings and civilizations
	// LoadCivXml for one of the port's files: false, without the exe's error dialog, if the mod folder doesn't have it
	static bool loadXml(CvXMLLoadUtility* pXML, FXml* pFXml, const char* szFilename);

	bool isAvailable() const { return m_bAvailable; }
	bool isEnabled() const { return m_bEnabled && m_bAvailable; }
	void setEnabled(bool bEnabled); // redraws units and cities if a game is running

	// NULL / NO_ARTSTYLE unless the option is on and RI has art for this
	const UnitArt* getUnitArt(UnitTypes eUnit, CivilizationTypes eCivilization) const;
	const CvArtInfoUnit* getUnitArtInfo(const UnitArt& kArt, int iMeshGroup, EraTypes eEra) const;
	const CvArtInfoBuilding* getBuildingArtInfo(BuildingTypes eBuilding, CivilizationTypes eCivilization) const;
	ArtStyleTypes getArtStyle(CivilizationTypes eCivilization) const;
	// One of RI's city styles (GlobalTypes.xml, named RI_...)? They are only shown, never stored in the game.
	static bool isRIArtStyle(ArtStyleTypes eArtStyle);

private:
	CvRIArt();
	void clear();
	void refreshGraphics();

	static CvRIArt m_kInstance;

	bool m_bEnabled;
	bool m_bAvailable;
	int m_iNumUnits;
	int m_iNumBuildings;
	std::vector<UnitArt> m_aUnitArt;
	std::vector<int> m_aiUnitArt; // [(civilization + 1) * m_iNumUnits + unit]: index in m_aUnitArt, -1: none (row 0: any civ)
	std::vector<CvString> m_aszBuildingArt;
	std::vector<int> m_aiBuildingArt; // [(civilization + 1) * m_iNumBuildings + building]: index in m_aszBuildingArt
	std::vector<ArtStyleTypes> m_aeArtStyle; // per civilization
};

#endif
