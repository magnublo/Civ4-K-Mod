// RI art: Realism Invictus graphics (see CvRIArt.h)

#include "CvGameCoreDLL.h"
#include "CvRIArt.h"
#include "CvXMLLoadUtility.h"
#include "CvDLLXMLIFaceBase.h"
#include "CvDLLInterfaceIFaceBase.h"
#include "CvArtFileMgr.h"
#include "CvGameAI.h"
#include "CvPlayerAI.h"
#include "CvInitCore.h"

CvRIArt CvRIArt::m_kInstance;

static const char* const s_aszTierTags[CvRIArt::NUM_TIERS] =
{
	"EarlyArtDefineTag", "ClassicalArtDefineTag", "MiddleArtDefineTag", "RennArtDefineTag",
	"IndustrialArtDefineTag", "LateArtDefineTag", "FutureArtDefineTag"
};

CvRIArt::CvRIArt() : m_bEnabled(true), m_bAvailable(false), m_iNumUnits(0), m_iNumBuildings(0)
{
}

void CvRIArt::clear()
{
	m_bAvailable = false;
	m_aUnitArt.clear();
	m_aiUnitArt.clear();
	m_aszBuildingArt.clear();
	m_aiBuildingArt.clear();
	m_aeArtStyle.clear();
}

// the civilizations named in the current element's <Civilizations>: none means every civilization (-1)
static std::vector<int> readCivilizations(CvXMLLoadUtility* pXML)
{
	std::vector<int> aiCivs;
	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "Civilizations"))
	{
		CvString* paszCivs = NULL;
		int iNumCivs = 0;
		pXML->SetStringList(&paszCivs, &iNumCivs);
		for (int i = 0; i < iNumCivs; i++)
		{
			int iCiv = pXML->FindInInfoClass(paszCivs[i]);
			if (iCiv >= 0)
			{
				aiCivs.push_back(iCiv);
			}
		}
		SAFE_DELETE_ARRAY(paszCivs);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	if (aiCivs.empty())
	{
		aiCivs.push_back(-1);
	}
	return aiCivs;
}

// The exe's LoadXml shows an error dialog for a missing file, so look for the file first
bool CvRIArt::loadXml(CvXMLLoadUtility* pXML, FXml* pFXml, const char* szFilename)
{
	std::vector<CvString> aszFiles;
	gDLL->enumerateFiles(aszFiles, szFilename);
	return !aszFiles.empty() && pXML->LoadCivXml(pFXml, szFilename);
}

void CvRIArt::read(CvXMLLoadUtility* pXML)
{
	clear();
	if (!loadXml(pXML, pXML->GetXML(), "xml\\Art\\RI_CIV4ArtInfos.xml"))
	{
		return; // not installed: K-Mod's art only
	}
	FXml* pFXml = pXML->GetXML();
	CvXMLLoadUtility& kXML = *pXML;

	// art defines that exist (a missing one would show the "ERROR" art)
	std::set<std::string> unitArt;
	for (int i = 0; i < ARTFILEMGR.getNumUnitArtInfos(); i++)
	{
		unitArt.insert(ARTFILEMGR.getUnitArtInfo(i).getTag());
	}
	std::set<std::string> buildingArt;
	for (int i = 0; i < ARTFILEMGR.getNumBuildingArtInfos(); i++)
	{
		buildingArt.insert(ARTFILEMGR.getBuildingArtInfo(i).getTag());
	}

	m_iNumUnits = GC.getNumUnitInfos();
	m_iNumBuildings = GC.getNumBuildingInfos();
	m_aiUnitArt.assign((GC.getNumCivilizationInfos() + 1) * m_iNumUnits, -1);
	m_aiBuildingArt.assign((GC.getNumCivilizationInfos() + 1) * m_iNumBuildings, -1);
	m_aeArtStyle.assign(GC.getNumCivilizationInfos(), NO_ARTSTYLE);
	CvString szTextVal;

	if (gDLL->getXMLIFace()->LocateNode(pFXml, "Civ4RIArtInfos/RIUnitArtInfos/RIUnitArtInfo"))
	{
		do
		{
			kXML.GetChildXmlValByName(szTextVal, "UnitType");
			int iUnit = kXML.FindInInfoClass(szTextVal);
			UnitArt kArt;
			kXML.GetChildXmlValByName(&kArt.fMaxSpeed, "fMaxSpeed");
			kXML.GetChildXmlValByName(&kArt.fPadTime, "fPadTime");
			bool bValid = true;
			if (gDLL->getXMLIFace()->SetToChildByTagName(pFXml, "UnitMeshGroups"))
			{
				int iNumGroups = gDLL->getXMLIFace()->NumOfChildrenByTagName(pFXml, "UnitMeshGroup");
				if (iNumGroups > 0 && gDLL->getXMLIFace()->SetToChildByTagName(pFXml, "UnitMeshGroup"))
				{
					for (int k = 0; k < iNumGroups; k++)
					{
						UnitMeshGroup kGroup;
						kXML.GetChildXmlValByName(&kGroup.iRequired, "iRequired");
						bool bAny = false;
						for (int iTier = 0; iTier < NUM_TIERS; iTier++)
						{
							kXML.GetChildXmlValByName(kGroup.aszTags[iTier], s_aszTierTags[iTier]);
							if (!kGroup.aszTags[iTier].empty() && unitArt.find(kGroup.aszTags[iTier]) == unitArt.end())
							{
								FAssertMsg(false, CvString::format("RI art: unit art %s not defined", kGroup.aszTags[iTier].c_str()).c_str());
								kGroup.aszTags[iTier].clear();
							}
							bAny = bAny || !kGroup.aszTags[iTier].empty();
						}
						bValid = bValid && bAny;
						kArt.aGroups.push_back(kGroup);
						if (!gDLL->getXMLIFace()->NextSibling(pFXml))
						{
							break;
						}
					}
					gDLL->getXMLIFace()->SetToParent(pFXml);
				}
				gDLL->getXMLIFace()->SetToParent(pFXml);
			}
			std::vector<int> aiCivs = readCivilizations(pXML);
			if (iUnit >= 0 && bValid && !kArt.aGroups.empty())
			{
				m_aUnitArt.push_back(kArt);
				for (size_t i = 0; i < aiCivs.size(); i++)
				{
					m_aiUnitArt[(aiCivs[i] + 1) * m_iNumUnits + iUnit] = (int)m_aUnitArt.size() - 1;
				}
			}
		} while (gDLL->getXMLIFace()->NextSibling(pFXml) && kXML.SkipToNextVal());
	}

	if (gDLL->getXMLIFace()->LocateNode(pFXml, "Civ4RIArtInfos/RIBuildingArtInfos/RIBuildingArtInfo"))
	{
		do
		{
			kXML.GetChildXmlValByName(szTextVal, "BuildingType");
			int iBuilding = kXML.FindInInfoClass(szTextVal);
			CvString szArt;
			kXML.GetChildXmlValByName(szArt, "ArtDefineTag");
			std::vector<int> aiCivs = readCivilizations(pXML);
			FAssertMsg(buildingArt.find(szArt) != buildingArt.end(), CvString::format("RI art: building art %s not defined", szArt.c_str()).c_str());
			if (iBuilding >= 0 && buildingArt.find(szArt) != buildingArt.end())
			{
				m_aszBuildingArt.push_back(szArt);
				for (size_t i = 0; i < aiCivs.size(); i++)
				{
					m_aiBuildingArt[(aiCivs[i] + 1) * m_iNumBuildings + iBuilding] = (int)m_aszBuildingArt.size() - 1;
				}
			}
		} while (gDLL->getXMLIFace()->NextSibling(pFXml) && kXML.SkipToNextVal());
	}

	if (gDLL->getXMLIFace()->LocateNode(pFXml, "Civ4RIArtInfos/RICivilizationArtInfos/RICivilizationArtInfo"))
	{
		do
		{
			kXML.GetChildXmlValByName(szTextVal, "CivilizationType");
			int iCiv = kXML.FindInInfoClass(szTextVal);
			kXML.GetChildXmlValByName(szTextVal, "ArtStyleType");
			int iStyle = GC.getTypesEnum(szTextVal);
			if (iCiv >= 0 && iStyle >= 0 && iStyle < GC.getNumArtStyleTypes())
			{
				m_aeArtStyle[iCiv] = (ArtStyleTypes)iStyle;
			}
		} while (gDLL->getXMLIFace()->NextSibling(pFXml) && kXML.SkipToNextVal());
	}

	m_bAvailable = !m_aUnitArt.empty() || !m_aszBuildingArt.empty();
}

void CvRIArt::setEnabled(bool bEnabled)
{
	bool bWasEnabled = isEnabled();
	m_bEnabled = bEnabled;
	if (isEnabled() != bWasEnabled)
	{
		refreshGraphics();
	}
}

// Redraw every unit and city, as CvPlayer::changeCiv does
void CvRIArt::refreshGraphics()
{
	if (!GC.IsGraphicsInitialized() || !GC.getGameINLINE().isFinalInitialized())
	{
		return;
	}
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if (!kPlayer.isAlive())
		{
			continue;
		}
		int iLoop;
		for (CvCity* pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
		{
			pLoopCity->setLayoutDirty(true);
		}
		for (CvUnit* pLoopUnit = kPlayer.firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = kPlayer.nextUnit(&iLoop))
		{
			if (!pLoopUnit->isFighting()) // a battle being shown keeps its figures
			{
				pLoopUnit->reloadEntity();
			}
		}
	}
	gDLL->getInterfaceIFace()->makeInterfaceDirty();
}

const CvRIArt::UnitArt* CvRIArt::getUnitArt(UnitTypes eUnit, CivilizationTypes eCivilization) const
{
	if (!isEnabled() || eUnit < 0 || eUnit >= m_iNumUnits)
	{
		return NULL;
	}
	int iIndex = -1;
	if (eCivilization >= 0 && eCivilization < (int)m_aeArtStyle.size())
	{
		iIndex = m_aiUnitArt[(eCivilization + 1) * m_iNumUnits + eUnit];
	}
	if (iIndex < 0)
	{
		iIndex = m_aiUnitArt[eUnit];
	}
	return iIndex >= 0 ? &m_aUnitArt[iIndex] : NULL;
}

// As in RI: the art of the latest art era up to the game era; failing that, the earliest there is
const CvArtInfoUnit* CvRIArt::getUnitArtInfo(const UnitArt& kArt, int iMeshGroup, EraTypes eEra) const
{
	const UnitMeshGroup& kGroup = kArt.aGroups[range(iMeshGroup, 0, (int)kArt.aGroups.size() - 1)];
	int iTier = range((int)eEra, 0, NUM_TIERS - 1);
	for (int i = iTier; i >= 0; i--)
	{
		if (!kGroup.aszTags[i].empty())
		{
			return ARTFILEMGR.getUnitArtInfo(kGroup.aszTags[i]);
		}
	}
	for (int i = iTier + 1; i < NUM_TIERS; i++)
	{
		if (!kGroup.aszTags[i].empty())
		{
			return ARTFILEMGR.getUnitArtInfo(kGroup.aszTags[i]);
		}
	}
	return NULL;
}

const CvArtInfoBuilding* CvRIArt::getBuildingArtInfo(BuildingTypes eBuilding, CivilizationTypes eCivilization) const
{
	if (!isEnabled() || eBuilding < 0 || eBuilding >= m_iNumBuildings)
	{
		return NULL;
	}
	int iIndex = -1;
	if (eCivilization >= 0 && eCivilization < (int)m_aeArtStyle.size())
	{
		iIndex = m_aiBuildingArt[(eCivilization + 1) * m_iNumBuildings + eBuilding];
	}
	if (iIndex < 0)
	{
		iIndex = m_aiBuildingArt[eBuilding];
	}
	return iIndex >= 0 ? ARTFILEMGR.getBuildingArtInfo(m_aszBuildingArt[iIndex]) : NULL;
}

bool CvRIArt::isRIArtStyle(ArtStyleTypes eArtStyle)
{
	return eArtStyle >= 0 && eArtStyle < GC.getNumArtStyleTypes() &&
		strncmp(GC.getArtStyleTypes(eArtStyle).GetCString(), "RI_", 3) == 0;
}

ArtStyleTypes CvRIArt::getArtStyle(CivilizationTypes eCivilization) const
{
	if (!isEnabled() || eCivilization < 0 || eCivilization >= (int)m_aeArtStyle.size())
	{
		return NO_ARTSTYLE;
	}
	return m_aeArtStyle[eCivilization];
}
