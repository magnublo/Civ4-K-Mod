## PreChopSigns (K-Mod-Magnus, written by Claude)
##
## Puts a "prechopped" sign on a plot when a worker's pre-chop stops there: the DLL reports it as the event
## "preChopped" (CvSelectionGroup::groupBuild) once the next worker-turn would remove the forest or jungle. The sign
## belongs to the worker's owner, like one the player placed, and the game saves it like those. It goes when no chop
## is waiting on the plot any more: at once when a build removes the feature (the chop finished, or a farm built over
## the forest), and at the start of the next turn when the feature went another way (an event, global warming,
## fallout). Only signs that read "prechopped" are removed. A plot that already has one of the player's signs keeps
## it and gets no second sign (CyEngine.addSign adds none there). Signs are only shown, so none of this changes the game.

from CvPythonExtensions import *

gc = CyGlobalContext()

def getCaption():
	return CyTranslator().getText("TXT_KEY_SIGN_PRECHOPPED", ())

def isChopWaiting(pPlot):
	"""True if a build that removes the plot's feature has work done on it."""
	eFeature = pPlot.getFeatureType()
	if eFeature == FeatureTypes.NO_FEATURE:
		return False
	for eBuild in range(gc.getNumBuildInfos()):
		if pPlot.getBuildProgress(eBuild) > 0 and gc.getBuildInfo(eBuild).isFeatureRemove(eFeature):
			return True
	return False

def getSigns(pAtPlot = None):
	"""(plot, player) of each player's sign that reads "prechopped", only those on pAtPlot if it is given."""
	engine = CyEngine()
	szCaption = getCaption()
	signs = []
	for i in range(engine.getNumSigns()):
		pSign = engine.getSignByIndex(i)
		pPlot = pSign.getPlot()
		if pAtPlot is not None and (pPlot.getX() != pAtPlot.getX() or pPlot.getY() != pAtPlot.getY()):
			continue
		# getCaption gives unicode, as getText does
		if pSign.getPlayerType() != PlayerTypes.NO_PLAYER and pSign.getCaption() == szCaption:
			signs.append((pPlot, pSign.getPlayerType()))
	return signs

def onPreChopped(argsList):
	iX, iY, ePlayer, eBuild = argsList[0][0]
	pPlot = CyMap().plot(iX, iY)
	if pPlot.getBuildProgress(eBuild) > 0:
		CyEngine().addSign(pPlot, ePlayer, getCaption().encode('latin_1'))

def onPlotFeatureRemoved(argsList):
	# sent before the feature goes; the arguments are (plot, city, feature), not as CvEventManager names them
	pPlot = argsList[0]
	engine = CyEngine()
	if engine.getNumSigns() == 0:
		return
	for (pSignPlot, ePlayer) in getSigns(pPlot):
		engine.removeSign(pSignPlot, ePlayer)

def onBeginGameTurn(argsList):
	engine = CyEngine()
	for (pPlot, ePlayer) in getSigns():
		if not isChopWaiting(pPlot):
			engine.removeSign(pPlot, ePlayer)
