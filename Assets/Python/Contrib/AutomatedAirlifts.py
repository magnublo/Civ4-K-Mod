## AutomatedAirlifts (K-Mod-Magnus, written by Claude)
##
## Two checkboxes on the Game tab of the Options screen (Screens/CvOptionsScreen.py): whether the player's automated
## units take airlifts on their own ("Automated Units Use Airlifts"), and whether units on their way to their city's
## rally point or spy rally point do ("Rally Point Units Use Airlifts"). They are BUG options, settings of this computer
## kept in "Automated Airlifts.ini"; the defaults come from Config/Automated Airlifts.xml. BUG's options aren't loaded in
## the main menu, so the checkboxes are there only during a game.
##
## The DLL holds the settings per player (CvPlayer::getAirliftUse) and does the work. They reach it as a ModNetMessage,
## so every computer of a network game applies them alike: at the start of each of the player's turns (also after
## loading a game) and whenever a checkbox changes. AI players and AI autoplay are not affected.

from CvPythonExtensions import *
import BugCore
import BugOptions

gc = CyGlobalContext()

MOD_ID = "AutomatedAirlifts"
# ModNetMessage (MSG_AIRLIFT_USE, player, AIRLIFT_USE_* bits, -1, -1)
MSG_AIRLIFT_USE = 2611
# CvDefines.h
AIRLIFT_USE_AUTOMATED = 1
AIRLIFT_USE_RALLY = 2
# (option, bit), in the order of the checkboxes
OPTIONS = (("Automated", AIRLIFT_USE_AUTOMATED), ("Rally", AIRLIFT_USE_RALLY))

def getOption(szId):
	return BugOptions.getOption("%s__%s" % (MOD_ID, szId))

def getAirliftUse():
	"""The AIRLIFT_USE_* bits of the checked options."""
	iUse = 0
	for (szId, iBit) in OPTIONS:
		if getOption(szId).getValue():
			iUse |= iBit
	return iUse

def sendChoices(argsList=None):
	"""Sends the active player's settings to the DLL of every computer in the game."""
	game = CyGame()
	if not game.isFinalInitialized():
		return
	iPlayer = game.getActivePlayer()
	if iPlayer < 0 or not gc.getPlayer(iPlayer).isAlive():
		return
	CyMessageControl().sendModNetMessage(MSG_AIRLIFT_USE, iPlayer, getAirliftUse(), -1, -1)

def onOptionChanged(option, value):
	sendChoices()

def onModNetMessage(argsList):
	iData1, iData2, iData3, iData4, iData5 = argsList
	if iData1 != MSG_AIRLIFT_USE:
		return 0
	gc.getPlayer(iData2).setAirliftUse(iData3)
	return 1

def addCheckbox(tab, szVBox, szId):
	"""The checkbox of an option on the Game tab of the Options screen, at the end of the box szVBox."""
	option = getOption(szId)
	szWidgetName = "AutomatedAirliftsCheckBox_" + szId
	tab.attachCheckBox(szVBox, szWidgetName, option.getTitle(), "CvScreensInterface", "onAirliftOptionClicked", szWidgetName, option.getValue())
	tab.setToolTip(szWidgetName, option.getTooltip())

def onCheckBoxClicked(argsList):
	"""Called by the Options screen (through CvScreensInterface); BUG saves the option, and the change sends the
	settings."""
	bValue, szName = argsList
	getOption(szName[szName.find("_") + 1:]).setValue(bValue)
	return 1
