## RealismInvictusArt
##
## The option "Realism Invictus graphics" (BUG options, Map tab) of K-Mod-Magnus: shows the unit and building art
## of the Realism Invictus mod instead of K-Mod's. The DLL does the work (CvGameCoreDLL/CvRIArt.h); this hands it
## the option when BUG starts and whenever it changes. Changing it redraws the units and cities on the map.

from CvPythonExtensions import *
import BugCore

gc = CyGlobalContext()
options = BugCore.game.RealismInvictusArt

def init():
	gc.setRealismInvictusArt(options.isEnabled())

def onEnabledOptionChanged(option, value):
	gc.setRealismInvictusArt(value)
