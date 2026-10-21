## AutomatedWorkers (K-Mod-Magnus, written by Claude)
##
## The "Workers" tab of the BUG options (BUG/Tabs/BugWorkersOptionsTab.py): for each improvement, whether the player's
## automated workers build it, and whether they replace it where they find it. One option per improvement, made here
## from the improvements in the XML (a new improvement gets its own), kept in "Automated Workers.ini" on this computer;
## the defaults come from Config/Automated Workers.xml.
##
## The DLL holds the choices per player (CvPlayer::getAutomatedWorkers) and does the work. They reach it as a
## ModNetMessage, so every computer of a network game applies them alike: at the start of each of the player's turns
## (also after loading a game) and whenever an option changes. The AI's workers, AI autoplay and the orders the player
## gives a worker are not affected.

from CvPythonExtensions import *
import BugCore
import BugOptions
import BugUtil

gc = CyGlobalContext()

MOD_ID = "AutomatedWorkers"
# ModNetMessage (MSG_AUTOMATED_WORKERS, player, first improvement, never-build bits, replace bits) for CHUNK improvements
MSG_AUTOMATED_WORKERS = 2610
CHUNK = 30

g_buildOptions = [] # (improvement, option): the improvements that a build makes
g_replaceOptions = [] # (improvement, option): those and what they grow into (hamlet, village, town)

def init(noBuild=(), replace=()):
	"""Makes the options. noBuild and replace list the improvement types that are unticked or ticked by default."""
	mod = BugCore.game._getMod(MOD_ID)
	ini = BugOptions.getOptions().getFile(MOD_ID)
	buildable = []
	for iBuild in range(gc.getNumBuildInfos()):
		iImprovement = gc.getBuildInfo(iBuild).getImprovement()
		if iImprovement != -1 and iImprovement not in buildable:
			buildable.append(iImprovement)
	onMap = buildable[:]
	for iImprovement in buildable:
		iUpgrade = gc.getImprovementInfo(iImprovement).getImprovementUpgrade()
		while iUpgrade != -1 and iUpgrade not in onMap:
			onMap.append(iUpgrade)
			iUpgrade = gc.getImprovementInfo(iUpgrade).getImprovementUpgrade()
	byName = lambda a, b: cmp(gc.getImprovementInfo(a).getDescription(), gc.getImprovementInfo(b).getDescription())
	buildable.sort(byName)
	onMap.sort(byName)
	for iImprovement in buildable:
		bDefault = gc.getImprovementInfo(iImprovement).getType() not in noBuild
		g_buildOptions.append((iImprovement, makeOption(mod, ini, "Build", iImprovement, bDefault)))
	for iImprovement in onMap:
		bDefault = gc.getImprovementInfo(iImprovement).getType() in replace
		g_replaceOptions.append((iImprovement, makeOption(mod, ini, "Replace", iImprovement, bDefault)))

def makeOption(mod, ini, szSection, iImprovement, bDefault):
	"""A checkbox option named after the improvement, stored under [szSection] by improvement type."""
	info = gc.getImprovementInfo(iImprovement)
	szTooltip = BugUtil.getText("TXT_KEY_AUTOMATED_WORKERS_%s_HOVER" % szSection.upper(), (info.getDescription(),))
	option = BugOptions.IniOption(mod, "%s__%s_%s" % (MOD_ID, szSection, info.getType()), ini, szSection, info.getType(),
			"boolean", bDefault, None, None, info.getDescription(), szTooltip)
	BugOptions.getOptions().addOption(option)
	mod._addOption(option)
	option.addDirtyFunction(onOptionChanged)
	return option

def getBuildOptionIDs():
	return [option.getID() for (iImprovement, option) in g_buildOptions]

def getReplaceOptionIDs():
	return [option.getID() for (iImprovement, option) in g_replaceOptions]

def getMasks(iFirst):
	"""Bits of the improvements iFirst to iFirst + CHUNK - 1 that the workers never build, and of those they replace.
	Replacing one also means never building it."""
	iReplace = 0
	for (iImprovement, option) in g_replaceOptions:
		if iFirst <= iImprovement < iFirst + CHUNK and option.getValue():
			iReplace |= 1 << (iImprovement - iFirst)
	iNeverBuild = iReplace
	for (iImprovement, option) in g_buildOptions:
		if iFirst <= iImprovement < iFirst + CHUNK and not option.getValue():
			iNeverBuild |= 1 << (iImprovement - iFirst)
	return (iNeverBuild, iReplace)

def sendChoices(argsList=None):
	"""Sends the active player's choices to the DLL of every computer in the game."""
	game = CyGame()
	if not game.isFinalInitialized():
		return
	iPlayer = game.getActivePlayer()
	if iPlayer < 0 or not gc.getPlayer(iPlayer).isAlive():
		return
	for iFirst in range(0, gc.getNumImprovementInfos(), CHUNK):
		(iNeverBuild, iReplace) = getMasks(iFirst)
		CyMessageControl().sendModNetMessage(MSG_AUTOMATED_WORKERS, iPlayer, iFirst, iNeverBuild, iReplace)

def onOptionChanged(option, value):
	sendChoices()

def onModNetMessage(argsList):
	iData1, iData2, iData3, iData4, iData5 = argsList
	if iData1 != MSG_AUTOMATED_WORKERS:
		return 0
	gc.getPlayer(iData2).setAutomatedWorkers(iData3, iData4, iData5)
	return 1
