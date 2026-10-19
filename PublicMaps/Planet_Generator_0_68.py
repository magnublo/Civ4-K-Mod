#
#	FILE:	 Planet_Generator.py
#	AUTHORS:
#	Script code: Nerijus Arlauskas (Nercury)
#	Mac support: AlanH
#
#	Version: 0.68
#-----------------------------------------------------------------------------
#	Copyright (c) 2006 Nercury
#-----------------------------------------------------------------------------

"""
Change log:

v. 0.68 K-Mod-Magnus (2026, written with Claude for the K-Mod-Magnus mod; the plot types part is mostly new)
- Percentage of ocean is exact now and leaves the islands out: "70%" means the continents cover exactly 30% of the
  map, and the islands around them and the lonely islands come on top, as if they were ocean. The land used to be
  lowered for the continent separation and by another 5%, and land bridges and filled lakes came on top of it.
- Continents grow until the land is there: one continent that could not grow any more used to stop all of them.
- Continent separation holds: grow candidates were checked when found, not when used, so continents could touch.
- Each continent is one landmass: "Continent scattering" now puts islands near its coast instead of flinging parts
  of the continent away (those parts often were continents of their own). Archipelago continents are as before.
- Lonely islands are placed by how far open water is from all land, so "Use every empty area in ocean" fills every
  gap (big islands first, smaller ones in the gaps left), at the chosen size. A number of islands goes into smaller
  gaps too when the open sea runs out, if an island of the chosen size still fits. More choices: 1 to 8, 10, 12, 15,
  20, 25 or 30 islands, or 10% of the empty area.
- Nobody starts on an island: civs start on the continents (the game's own choice could put one on a big island).
- New option Coastline: Rugged, Very rugged or Maximum coast reshape the continents for more coast (fjords and
  peninsulas, no new lakes), keeping each continent in one piece, its size and the separation.
- New option Minimum island size (replaces One tile islands allowed).
- New option Rival civs on your continent: how many other civs start on your continent.
- New option Every resource at least twice: whatever the climate, every resource is on the map at least twice.
- Fixed: a crash in continent growth (part bookkeeping), peaks filling one-plot lakes, and plots near the map's
  x seam and edges in two helpers.

v. 0.68
- Coordinate system changed to circular for continent growth calculation (faster, not so blocky shapes)
- Continents should no longer break in several big parts (except with "archipelago continents")
- Bug fix: the lonely islands are all devoid of hills or mountains

v. 0.67
- Further improvements for mod map size support: now can understand completely customized sizes

v. 0.66
- Mod map size support

v. 0.65
- Many feature generation tweaks
- More forests or jungle (jungle depends on temperature)
- More flood plains on deserts
- More oases, better oasis placement
- Flood plain "Standard" generation options reduced to one
- Desert maps should be more interesting now
- Forests and jungle can start growing not only near water, but also near peaks which do not have many features around

v. 0.64
- Random float number bug fix

v. 0.63
- Bug fix: generation failure with high continent scattering and distribution around center

v. 0.62
- Resource normalization fix (it was disabled all the time!)

v. 0.61
- 4,5 island option bug fix

v. 0.60
- Continent scattering improved, all options thoroughly tested to produce good-looking maps.
- TileBuilder code improved, to avoid continent jams.
- Generation bug fixed. Hopefully, it will no longer stops at start (!).
- Random (Standard:Rare/Common) flood option fixed.
- New option: Map size. It is now possible to specify custom map size, up to 256x160. Because of this change, following options are modified:
- Continent separation options expanded up to 10 tiles.
- Distance from poles no longer depends on continent separation, and is always constant (3).
- From now on, area for continent separation is excluded from ocean area when calculating ocean percentage. It means that selecting higher continent separation now will produce less land. This helps against continent jams.
- Lonely island selection options improved, now it is possible to specify exact amount of islands (1-5), but they will be placed only if there is enough empty area in ocean.
- Default lonely island option changed to 3 islands.
- Default option of removing one-tile islands changed to "yes".
- Humidity options tweaked, now "normal" is a little bit more wet.
- Land near pole ice default value is now "sometimes" (i think that looks better)
- Fixed typo in options

v. 0.53
- Desert tile changing into plains (if jungle nearby) chance reduced from 100% to 60%.
- Rewritten continent scattering algorithm. Changed name from "Continent Islands" to "Continent Scattering", because, well, it's what it does.
- Island separation now is the same as continent separation.

v. 0.51
- Continent islands option fix.

v. 0.50
- New options added: Land near pole ice, Continent islands, Lonely islands, Lonely island size, One tile islands allowed.

v. 0.47
- Smoother transition from jungle to desert/tundra.

v. 0.46
- Both Mac and Windows support in one file. Thanks AlanH for writing code for mac!

v. 0.45
- Oasis.

v. 0.44
- Fixed continent distribution bug for high continent count and huge map.
- Added more ways how continents can be distributed: you can choose old rule with square on equator, two or three rows, circle. Even more, i added option Smart Selection, which will choose the best distribution method for selected continent count and map size.
- Flood plains now have default "Standard" distribution, which makes them appear only on desert, some on tundra and only near fresh water.
- Redefined what "natural" continent means. Now it is much more concentrated landmass with possible peninsulas or inland seas, not snake starting in the middle and ending near poles :)
- New landmass generation templates added - Natural, Crab-like, Smooth sides, old natural renamed to Traveling Natural (because it travels a lot from starting square).
- New "Smart Random for each continent" shape selection which almost ensures that you will have playable and unpredictable maps with different continent and map size selections
- Added a lot of comments to script code
Things to try out:
- New Crab-like continent shape for Pangaea map

v. 0.43
- Tested with Warlords: works
- [B]Emergency fix[/B]: Grid size function was rewritten incorrectly, making map height higher than width ;)

v. 0.42
- Last user choices are now remembered
- Tested with Total Realism 2.0, Fall From Heavens 2.014, SevoMod 3.1
- Modified hill intersection function (produces more hills close together)
- Grid size function is now written correctly (compatible with "very huge" map size mode (if there is such in some mod))

v. 0.41
- Some adjustments for humidity
- Fixed some mistakes in options
"""

from CvPythonExtensions import *
import CvUtil
from CvUtil import *
import CvMapGeneratorUtil
from CvMapGeneratorUtil import *
import math
import sys
import gc
import pickle

VERSION = "0_60"

#------------------------------------------------------
"""Dropdown options"""
#------------------------------------------------------
# (title), (script name), (use auto random generation)
selection_titles = [
					["Map size override:","map_size",False], #do not use auto random generation from array
					["Continents:","continents",True],
					["Continent separation:","continent_separation",True],
					["Continent distribution:","continent_distribution",True],
					["Continent shape:","continent_shape",True],
					["Coastline:","coastline",False], #K-Mod-Magnus
					["Continent scattering:","continent_scattering",False], #do not use auto random generation from array
					["Relative continent size:","continent_size",True],
					["Rival civs on your continent:","home_rivals",False], #K-Mod-Magnus
					["Percentage of ocean:","ocean",True],
					["Land near pole ice:","land_near_ice",True],
					["Lonely islands:","lonely_islands",True],
					["Lonely island size:","lonely_island_size",True],
					["Minimum island size:","min_island_size",False], #K-Mod-Magnus: was "One tile islands allowed" (No = 2 plots)
					["Mountains (peaks):","peaks",False], #do not use auto random generation from array
					["Hills:","hills",False], #do not use auto random generation from array
					["Climate variation:","variation",True],
					["Humidity:","humidity",True],
					["Temperature:","temperature",True],
					["Flood plains:","flood_plains",True],
					["Resources:","bonus_generation",True],
					["Every resource at least twice:","every_resource",False], #K-Mod-Magnus
					]
# default choices, or first-time choices (later only last-time values should be remembered)
selection_defaults = [
					0, #Map size override
					2, #Continents
					4, #Continent separation
					0, #Continent distribution
					2, #Continent shape
					0, #Coastline
					2, #Continent scattering
					1, #Relative continent size
					0, #Rival civs on your continent
					8, #Percentage of ocean
					0, #Land near pole ice
					4, #Lonely islands
					1, #Lonely island size
					1, #Minimum island size
					7, #Mountains (peaks)
					6, #Hills
					3, #Climate variation
					3, #Humidity
					6, #Temperature
					1, #Flood plains
					0, #Resources
					0, #Every resource at least twice
					]
# (selection_description), (value or random value list), (is included in "random" selection)
selection_names_and_values = [
					[ #Map size override:
					["No override (use selected map size)",[0,0],False], #0,0 == bypass
					["40 x 24",[40,24],False],
					["48 x 28",[48,28],False],
					["52 x 32",[52,32],False],
					["64 x 40",[64,40],False],
					["84 x 52",[84,52],False],
					["108 x 68",[108,68],False],
					["128 x 80",[128,80],False],
					["140 x 88",[140,88],False],
					["152 x 96",[152,96],False],
					["160 x 100",[160,100],False],
					["172 x 108",[172,108],False],
					["180 x 112",[180,112],False],
					["192 x 120",[192,120],False],
					["200 x 120",[200,120],False],
					["208 x 132",[208,132],False],
					["220 x 136",[220,136],False],
					["240 x 152",[240,152],False],
					["256 x 160",[256,160],False],
					],
					[ #Continents:
					["First Time (FT) Random","random",False],
					["FT Random (1 - 3 continents)",[1,2,3],False],
					["FT Random (1 - 5 continents)",[1,2,3,4,5],False],
					["FT Random (1 - 7 continents)",[1,2,3,4,5,6,7],False],
					["FT Random (2 - 5 continents, default)",[2,3,4,5],False],
					["FT Random (2 - 7 continents)",[2,3,4,5,6,7],False],
					["FT Random (2 - 13 continents)",[2,3,4,5,6,7,8,9,10,11,12,13],False],
					["FT Random (2 - 15 continents)",[2,3,4,5,6,7,8,9,10,11,12,13,14,15],False],
					["FT Random (3 - 7 continents)",[3,4,5,6,7],False],
					["FT Random (3 - 13 continents)",[3,4,5,6,7,8,9,10,11,12,13],False],
					["FT Random (3 - 15 continents)",[3,4,5,6,7,8,9,10,11,12,13,14,15],False],
					["FT Random (3 - 18 continents)",[3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18],False],
					["One continent",1,True], # True means this will be added to "random" selection
					["2 continents",2,True],
					["3 continents",3,True],
					["4 continents",4,True],
					["5 continents",5,True],
					["6 continents",6,True],
					["7 continents",7,True],
					["8 continents",8,True],
					["9 continents",9,True],
					["10 continents",10,True],
					["11 continents",11,True],
					["12 continents",12,True],
					["13 continents",13,True],
					["14 continents",14,False],
					["15 continents",15,False],
					["16 continents",16,False],
					["17 continents",17,False],
					["18 continents",18,False],
					],
					[ #Continent separation:
					["FT Random (include all)","random",False],
					["FT Random (2 - 4)",[2,3,4],False],
					["Min 1 water tile (for small maps)",1,True],
					["Min 2 water tiles",2,True],
					["Min 3 water tiles (recommended)",3,True],
					["Min 4 water tiles (late civ contact)",4,True],
					["Min 5 water tiles (for very huge maps)",5,True], #actually, forcing this to small map will make it huge
					["Min 6 water tiles",6,True],
					["Min 7 water tiles",7,True],
					["Min 8 water tiles",8,True],
					["Min 9 water tiles",9,True],
					["Min 10 water tiles",10,True],
					],
					[ #Continent distribution:
					["Smart Selection (default)","smart-random",False],
					["Must have square on equator","equator",True],
					["Around center","center",True],
					["Two Rows","2rows",True],
					["Three Rows","3rows",True],
					],
					[ #Continent shape:
					["Random","random",False],
					["Smart Random for each continent","continent-random",True], #makes each continent to use different grow method
					["Unpredictable (default)","unpredictable",True],
					["Snaky - very thin","snaky-thin",False], #this generates some very narrow passages
					["Snaky","snaky",True],
					["Short, wide snaky","snaky-natural",True], #tries to keep more round shape (compared to snaky)
					["Drifting landmass","traveling-natural",True], #tries to keep more round shape (compared to snaky)
					["Landmass","natural",True],
					["Landmass (rough edges)","crab",True],
					["Round","smooth",False],
					],
					[ #Coastline: (K-Mod-Magnus) reshapes the continents for more coast; each continent stays one landmass
					["As grown (default)",0,True],
					["Rugged (fjords, more coast)",1,True],
					["Very rugged",2,True],
					["Maximum coast",3,True],
					],
					[ #Continent scattering:
					["Random (without archipelago)","random",False],
					# Min, Max, ChangeChance, MaxDif: TileBuilder scattering, now only used by "Archipelago continents".
					# K-Mod-Magnus: the other choices put islands near each continent instead (scattering used to fling parts of
					# the continent away): share of the continent's land, smallest and biggest island, farthest distance from its coast.
					["Solid, no islands around",		[0,	0,	0,	0,	0.0,	0,	0,	0],True],# 0,0,0,0 == bypass
					["Rare small islands (default)",	[1,	3,	0.5,1,	0.02,	1,	3,	3],True],
					["Common islands",					[3,	6,	0.4,2,	0.08,	2,	6,	4],True],
					["Rare islands - common islands",	[1,	5,	0.3,2,	0.05,	1,	5,	4],True],
					["Rare islands - many islands",		[1,	8,	0.3,3,	0.07,	1,	8,	5],True],
					["Common islands - many islands",	[3,	10,	0.4,2,	0.12,	2,	10,	4],True],
					["Coast varies alot",				[1,	12,	0.3,5,	0.10,	1,	12,	6],True],
					["Archipelago continents",			[8,	15,	0.5,2,	0.0,	0,	0,	0],False],
					],
					[ #Continent size:
					["Random selection","random",False],
					["Equal size (default)","equal",True],
					["By player count","players",True],
					["May vary","unpredictable",True],
					],
					[ #Rival civs on your continent: (K-Mod-Magnus) how many other civs start on your continent
					["Any (default)",-1,False],
					["None, a continent of your own",0,False],
					["1 rival",1,False],
					["2 rivals",2,False],
					["3 rivals",3,False],
					["4 rivals",4,False],
					["5 rivals",5,False],
					["6 rivals",6,False],
					["7 rivals",7,False],
					["8 rivals",8,False],
					["9 rivals",9,False],
					["10 rivals",10,False],
					["12 rivals",12,False],
					["15 rivals",15,False],
					["20 rivals",20,False],
					["All other civs",999,False],
					],
					[ #Percentage of ocean:
					["Random (include all)","random",False],
					["Random (60% - 70%)",[60,62,64,66,68,70],False],
					["Random (50% - 60%)",[50,55,60],False],
					["Random (40% - 70%)",[40,45,50,55,60,65,70],False],
					["Minimize ocean size",0,True],
					["30%",30,True], #K-Mod-Magnus: exact percentages now, of the map without the islands (these were "More than ..."); if the land does not fit, as much as fits
					["40%",40,True],
					["50%",50,True],
					["60% (normal)",60,True],
					["70%",70,True],
					["80%",80,True],
					["90%",90,True],
					],
					[ #Land near ice:
					["Sometimes (default)",True,False],
					["Never",False,False],
					],
					[ #Lonely islands: (K-Mod-Magnus: in order, and more of them; the new ones are not in "Random")
					["Random","random",False],
					["No islands",0.0,True],
					["One island somewhere (if possible)","lonely1",True],
					["2 islands (if possible)","lonely2",True],
					["3 islands (if possible, default)","lonely3",True],
					["4 islands (if possible)","lonely4",True],
					["5 islands (if possible)","lonely5",True],
					["6 islands (if possible)","lonely6",False],
					["7 islands (if possible)","lonely7",False],
					["8 islands (if possible)","lonely8",False],
					["10 islands (if possible)","lonely10",False],
					["12 islands (if possible)","lonely12",False],
					["15 islands (if possible)","lonely15",False],
					["20 islands (if possible)","lonely20",False],
					["25 islands (if possible)","lonely25",False],
					["30 islands (if possible)","lonely30",False],
					["10% of empty area",0.1,False],
					["25% of empty area",0.25,True],
					["50% of empty area",0.5,True],
					["75% of empty area",0.75,True],
					["Use every empty area in ocean",1.0,False],
					],
					[ #Lonely island size:
					["Small","small",True],
					["Medium (default)","medium",True],
					["Big","big",True],
					],
					[ #Minimum island size: (K-Mod-Magnus) smaller islands grow to it, or go if there is no room
					["1 plot (one-plot islands allowed)",1,True],
					["2 plots (default)",2,True],
					["3 plots",3,True],
					["4 plots",4,True],
					["5 plots",5,True],
					["6 plots",6,True],
					["8 plots",8,True],
					["10 plots",10,True],
					["12 plots",12,True],
					["15 plots",15,True],
					["20 plots",20,True],
					["25 plots",25,True],
					["30 plots",30,True],
					],
					[ #Mountains (peaks):
					["Random (without extreme)","random",False],
					["None",[0,0,0],True], # [peakStartProbability,peakGrowProbability,maxPeaksInRow]
					["Few peaks, Groups up to 3",[0.04,0.5,3],True],
					["Few peaks, Groups up to 5",[0.03,0.6,5],True],
					["Few peaks, Groups up to 10",[0.03,0.7,10],True],
					["Some peaks, No groups",[0.08,0,0],True],
					["Some peaks, Groups up to 2",[0.07,0.5,2],True],
					["Some peaks, Groups up to 5 (default)",[0.067,0.6,5],True],
					["Some peaks, Groups up to 7",[0.064,0.7,7],True],
					["Some peaks, Groups up to 11",[0.061,0.8,11],True],
					["Many peaks, No groups",[0.24,0,0],True],
					["Many peaks, Groups up to 3",[0.2,0.5,3],True],
					["Many peaks, Groups up to 5",[0.19,0.7,5],True],
					["Many peaks, Groups up to 10",[0.175,0.8,10],True],
					["Many peaks, Groups up to 14",[0.16,0.85,14],True],
					["Groups up to 100, for fun or pain",[0.1,1,100],False],
					["Extreme peaks, for fun or pain",[0.4,0.95,35],False],
					],
					[ #Hills:
					["Random","random",False],
					["None",[0,0,0],True], # [hillStartProbability,hillGrowProbability,maxHillsInRow]
					["Few hills, No groups",[0.1,0,0],True],
					["Few hills, Groups up to 2",[0.1,0.5,2],True],
					["Few hills, Groups up to 5",[0.1,0.55,5],True],
					["Normal hills, No groups",[0.21,0,0],True],
					["Normal hills, Groups up to 2 (default)",[0.18,0.5,2],True],
					["Normal hills, Groups up to 5",[0.18,0.55,5],True],
					["Many hills, No groups",[0.4,0,0],True],
					["Many hills, Groups up to 2",[0.35,0.5,2],True],
					["Many hills, Groups up to 5",[0.3,0.55,5],True],
					["Many hills, Groups up to 9",[0.25,0.7,9],True],
					],
					[ #Climate variation:
					["FT Random","random",False],
					["Very soft",0.8,True],
					["Soft",0.6,True],
					["Normal",0.4,True],
					["Hard",0.3,True],
					["Drastic",0.2,True],
					],
					[ #Humidity:
					["Random","random",False],
					["Very small (least plants)",0.025,True],
					["Small",0.2,True],
					["Normal",0.5,True],
					["Wet",0.61,True],
					["Rainy",0.78,True],
					["Very rainy (many plants)",0.95,True],
					],
					[ #Temperature:
					["FT Random (include all)","random",False],
					["FT Random (normal)",[15,20,25,30,35,40,45,50,55,60],False],
					["Extremely cold",-9,True],
					["Very cold (tundra in the middle)",10,True],
					["Cold (forests in the middle)",20,True],
					["Cool (light jungle, no deserts)",30,True],
					["Normal",40,True],
					["Warm (more deserts or jungle)",55,True],
					["Hot",70,True],
					["Extremely hot",85,True],
					],
					[ #Flood plains: bellow zero - standart distribution method, ranging from -1 = no floods to 0.001 flood hell, above zero - floods can spread, 0 = lowest spread level
					["Random (include all)","random",False],
					["Standard placement (default)",-0.01,True],
					["Any terrain - Rare, near fresh water",0,True],
					["Any terrain - Common, also near oceans",1,False],
					["Any terrain - Many (spreads more)",2,False],
					["Any terrain - Extreme",3,False],
					],
					[ #Resources (bonus)
					["Standard generator","standard",False],
					#["Planet generator - Rare",0.5,False],
					#["Planet generator - Normal",1.0,False],
					#["Planet generator - Full",1.5,False],
					#["Planet generator - Many",3.0,False],
					],
					[ #Every resource at least twice: (K-Mod-Magnus) a drop-down: the map options have no check boxes
					["No (default)",False,False],
					["Yes, whatever the climate",True,False],
					],
					]

continentCount = 1 #initial value
continentSeparation = 3 #initial value
climateVariation = 0.4
climateHumidity = 0.5
climateTemperature = 40
allowLandNearIce = False

world = None

# K-Mod-Magnus: for the start areas (findStartingArea)
continentAnchors = [] # a land plot of each continent
homeContinent = -1 # the continent planned for the human player and his rivals
homeRivals = -1 # "Rival civs on your continent" (-1: any)
startAreaPlan = None # player -> area, made at the first findStartingArea of a game
continentPlayers = None # players each continent was made for ("By player count")
landPlan = None # the LandPlan of the last map, and what the options came to (for checking maps outside the game)
debugChecks = False # set by tests outside the game: check the continents after every step
landSettings = None
continentAreaIDs = None # the game's areas of continent land, made at the first findStartingArea of a game

landTypes = [PlotTypes.PLOT_LAND,PlotTypes.PLOT_HILLS,PlotTypes.PLOT_PEAK]

generationStarted = False

def getSelValue(name):
	"""Returns None if not found"""
	cgc = CyGlobalContext()
	cymap = CyMap()
	dice = cgc.getGame().getMapRand()
	optionIndex = 0
	found = False
	for optionIndex in range(len(selection_titles)):
		title = selection_titles[optionIndex][1]
		if title == name:
			found = True
			break
	if found:
		#print "---------- chosen option:",optionIndex
		index = int(cymap.getCustomMapOption(optionIndex))
		#print "---------- selected value index:",index
		if index < len(selection_names_and_values[optionIndex]):
			#print "---------- selected value name:",selection_names_and_values[optionIndex][index][0]
			val = selection_names_and_values[optionIndex][index][1]
			#print "---------- selected actual value:",selection_names_and_values[optionIndex][index][1]
			#print "---------- val type",type(val)
			if val == "random":
				colInd = []
				for i in range(len(selection_names_and_values[optionIndex])):
					if selection_names_and_values[optionIndex][i][2]:
						colInd.append(i)
				#print colInd
				drop = dice.get(len(colInd),"Python - EarthGen: Random coice for menu selection")
				#print "drop:",colInd[drop]
				val = selection_names_and_values[optionIndex][colInd[drop]][1]
				#print "---------- selected random value:",val
			if selection_titles[optionIndex][2] and isinstance(val,list): #if use auto random seletion and value is array
				drop = dice.get(len(val),"Python - EarthGen: Random choice for menu selection")
				#print "Random menu option, drop:",drop
				return val[drop]
			else:
				return val
		else:
			val = selection_names_and_values[optionIndex][selection_defaults[optionIndex]][1]
	else:
		return None
#------------------------------------------------------
"""MAP SETUP FUNCTIONS"""
#------------------------------------------------------

def getDescription():
	"""
	A map's	Description	is displayed in	the	main menu when players go to begin a game.
	For	no description return an empty string.
	"""
	return unicode("Map generator, capable of creating very unpredictable continent shapes, while keeping continent size in control and balance.")

worldTypes = []
chosenWorldType = -1

def getGridSize(argsList):
	"""Given an	argument of	[worldSize], where worldSize is	of type	WorldSizeTypes.
	Can	be overridden to return	a (width, height) tuple	representing the number	of terrain cells
	the	world should have. (a terrain cell is a	chunk of 4x4 plots)"""
	#[worldSize] = argsList
	global generationStarted

	saveMapOptionDefaults()

	generationStarted = True
	if (argsList[0] == -1): # (-1,) is passed to function on loads ; thanks smart map ;)
		return (-1,-1)
	global continentCount
	global continentSeparation
	continentCount = getSelValue("continents")
	if continentCount is None:
		continentCount = 1
	continentSeparation = getSelValue("continent_separation")
	if continentSeparation is None:
		continentSeparation = 3

	cgc = CyGlobalContext()

	global worldTypes
	global chosenWorldType

	worldTypes = []
	chosenWorldType = -1
	for i in range(cgc.getNumWorldInfos()):
		worldInfo = cgc.getWorldInfo(i)
		worldTypes.append(worldInfo)

	map = cgc.getMap()
	worldSize = map.getWorldSize()

	ovW, ovH = getSelValue("map_size")

	if ovW > 0:
		iW = ovW
		iH = ovH
	else:
		rasta = False
		for index, worldType in enumerate(worldTypes):
			#print "World Type", worldType.getType()
			if cgc.getInfoTypeForString(worldType.getType()) == worldSize:
				iW = worldType.getGridWidth() * 4
				iH = worldType.getGridHeight() * 4
				rasta = True
				chosenWorldType = index
				break
		if not rasta:
			iW = 40
			iH = 24

	fW = iW / 4
	fH = iH / 4

	distribution = getContinentDistribution()
	minWidthDivision = 1
	if distribution == "2rows":
		minWidthDivision = 2
	elif distribution == "3rows":
		minWidthDivision = 3

	#check to see if there is enough place for continents to start growing
	#if not, increase grid width
	if distribution == "center":
		perimeter = (continentSeparation + 5) * continentCount
		radius = perimeter / (2 * 3.141592)
		width_multiplier = 1.5
		required_width = width_multiplier * radius * 4
		if required_width > fW * 4:
			fnW = required_width / 4
			fH = int(fH * fnW / float(fW) * 4) / 4
			fW = fnW
	else:
		if ((continentSeparation * 2) + 1) * continentCount / minWidthDivision > fW * 4:
			fnW = (((continentSeparation * 2) + 1) * continentCount / minWidthDivision + continentCount) / 4
			fH = int(fH * fnW / float(fW) * 4) / 4
			fW = fnW
	return (int(fW), int(fH)) # K-Mod-Magnus: "Around center" made them fractions
	# return (10, 6) # tiny
	#return (32, 20) # huge

# array wrapper with binary search optimization
# usage:
# - creating array:
# a = bArray()
# - adding data:
# a[2] = "data1"
# b[5] = "data2"
# - retrieving data:
# if a[2] != None:
#    print a[2]
# - length:
# print len(a)
# - removing item:
# a.pop(itemIndex)
# - removing item by key
# a.remove(5)
# - taking random item from array and removing it at the same time:
# print a.random(dice)
# - taking random item from array and not removing it:
# print a.randomItem(dice)
# - reading array data:
# for data in a:
#    print data
class bArray:
	def __init__(self):
		self._data = []

	def __setitem__(self, power, newValue):
		index, value = self._search(power,0,len(self._data)-1)
		if value == None:
			self._data.insert(index, [power, newValue])
		else:
			self._data[index][1] = newValue

	def __getitem__(self, power):
		index, value = self._search(power,0,len(self._data)-1)
		return value

	def __len__(self):
		return len(self._data)

	def pop(self, index = None):
		if index == None:
			return self._data.pop()
		else:
			return self._data.pop(index)

	def remove(self, power):
		index, value = self._search(power,0,len(self._data)-1)
		if value != None:
			self._data.pop(index)
			return True
		else:
			return False

	def random(self, dice):
		if len(self._data) == 0:
			return None
		else:
			item = dice.get(len(self._data),"Python - EarthGen: Random bArray item with removing.")
			value = self._data[item][1]
			self._data.pop(item)
			return value

	def randomItem(self, dice):
		if len(self._data) == 0:
			return None
		else:
			item = dice.get(len(self._data),"Python - EarthGen: Random bArray item without removing.")
			value = self._data[item][1]
			return value

	def _search(self,value,fromI,toI):
		left = fromI
		right = toI
		while left <= right:
			mid = (right-left) / 2 + left
			if value > self._data[mid][0]:
				left = mid + 1
			elif value < self._data[mid][0]:
				right = mid-1
			else:
				return mid, self._data[mid][1]
		return left, None

	def __iter__(self):
		self.index = -1
		return self

	def next(self):
		if self.index == len(self._data) - 1:
			raise StopIteration
		self.index += 1
		return self._data[self.index][1]

class probabilityArray:
	def __init__(self):
		self._data = []
		self._sum = 0.0
		self._maxpower = bArray()
		self._maxpowerindex = -1

	def __setitem__(self, power, newValue):
		if power > 0:
			self._data.append([power, newValue])
			self._sum += float(power)
			if power > self._maxpower:
				self._maxpower = power
				self._maxpowerindex = len(self._data) - 1

	def __len__(self):
		return len(self._data)

	def random(self, dice):
		if len(self._data) > 0:
			item = getRand(dice) * self._sum
			counted = 0
			foundindex = -1
			for index in range(len(self._data)):
				counted += self._data[index][0]
				if counted >= item:
					foundindex = index
					break
			if foundindex >= 0:
				power, value = self._data[foundindex]
				self._data.pop(foundindex)
				self._sum -= power
				return value
			else:
				if self._maxpowerindex != -1:
					power, value = self._data[self._maxpowerindex]
					self._data.pop(self._maxpowerindex)
					self._sum -= power
					self._maxpower = 0.0
					for index in range(len(self._data)):
						newpower, newvalue = self._data[index]
						if newpower > self._maxpower:
							self._maxpower = newpower
							self._maxpowerindex = index
					return value
		return None

	def randomItem(self, dice):
		if len(self._data) > 0:
			item = getRand(dice) * self._sum
			counted = 0
			foundindex = -1
			for index in range(len(self._data)):
				counted += self._data[index][0]
				if counted > item:
					foundindex = index
					break
			if foundindex >= 0:
				power, value = self._data[foundindex]
				return value
			else:
				if self._maxpowerindex != -1:
					return self._data[self._maxpowerindex][1]
		return None

# multi dimensional array wrapper
# usage:
# - creating array:
# a = mdArray([5,9],0) #creates 5x9 array with data:0 in every cell
# b = mdArray([4,2,6],"nodata") #creates 4x2x6 array with data:"nodata" in every cell
# - changing data:
# a[0,1] = 5 #change 0x1 cell to 5
# b[3,1,5] = "some data" #change 3x1x5 cell to "some data"
# - also:
# print a[0,1]
# print len(a)
# - changing data and retrieve offset:
# offset = a.getOffset([1,1])
# a._data[offset] = 6
class mdArray:
	"multi dimensional array handler"
	def __init__(self, dimensions, initial = None):
		dimlen = len(dimensions)
		self._dimensions = [0] * dimlen
		self._factors = [0] * dimlen
		product = 1
		i = dimlen - 1
		while i >= 0:
			self._dimensions[i] = dimensions[i]
			self._factors[i] = product
			product *= self._dimensions[i]
			i -= 1
		self._data = [initial] * product

	def getOffset(self, indices):
		offset = 0
		for i, dim in enumerate(self._dimensions):
			offset += self._factors[i] * indices[i]
		return offset

	def getPos(self, offset):
		indices = []
		product = offset
		for i, dim in enumerate(self._dimensions):
			value = product / self._factors[i]
			indices.append(value)
			product %= self._factors[i]
		return indices

	def __getitem__(self, indices):
		return self._data[self.getOffset(indices)]

	def __setitem__(self, indices, value):
		self._data[self.getOffset(indices)] = value

	def get(self):
		return self._data

	def __len__():
		return len(self._data)

#for saving grid data, wraps in w
class worldArray(mdArray):
	def __init__(self, dimensions, initial = None):
		if len(dimensions) != 2: #only allow 2 dimensions
			raise IndexError
		dimensions_copy = [dimensions[1],dimensions[0]] #reverse dimensions (required to pass plot types corectly)
		dimlen = len(dimensions_copy)
		self._dimensions = [0] * dimlen
		self._factors = [0] * dimlen
		product = 1
		i = dimlen - 1
		while i >= 0:
			self._dimensions[i] = dimensions_copy[i]
			self._factors[i] = product
			product *= self._dimensions[i]
			i -= 1
		self._data = [initial] * product

	def getOffset(self, indices):
		indices_copy = [indices[1],indices[0]] #reverse indices (required to match dimensions)

		# make sure array wraps if out of bounds
		while indices_copy[0] < 0:
			indices_copy[0] += self._dimensions[0]
		while indices_copy[0] > self._dimensions[0] - 1:
			indices_copy[0] -= self._dimensions[0]

		while indices_copy[1] < 0:
			indices_copy[1] += self._dimensions[1]
		while indices_copy[1] > self._dimensions[1] - 1:
			indices_copy[1] -= self._dimensions[1]

		offset = 0
		for i, dim in enumerate(self._dimensions):
			offset += self._factors[i] * indices_copy[i]
		return offset

	def getPos(self, offset):
		indices = []
		product = offset
		for i, dim in enumerate(self._dimensions):
			value = product / self._factors[i]
			indices.insert(0,value)
			product %= self._factors[i]
		return indices

	def getHeight(self):
		return self._dimensions[0]

	def getWidth(self):
		return self._dimensions[1]

class ContinentPart:
	def __init__(self,id,builder):
		self.builder = builder
		self.tiles = bArray()
		self.id = id
		self.links = bArray()

	def deepMaxTiles(self,used = bArray()):
		if used[self.id] == None:
			used[self.id] = True
			maxTiles = len(self.tiles)
			for link in self.links:
				id, o1, o2 = link
				part = self.builder.continentParts[id]
				partMax = part.deepMaxTiles(used)
				if partMax > maxTiles:
					maxTiles = partMax
			return maxTiles
		else:
			return 0

# snaky continent generation algorythm
class TileBuilder:
	def __init__(self,world,tileType,builderTiles,activeTiles,growCount,continentSeparation,targetSize):
		self.cgc = CyGlobalContext()
		self.dice = self.cgc.getGame().getMapRand()
		self.map = self.cgc.getMap()
		self.world = world
		self.tileType = [tileType] # what tile type we are building
		if isinstance(builderTiles,list):
			self.builderTiles = builderTiles
		else:
			self.builderTiles = [builderTiles] # what tiles spawn new tiles
		if isinstance(activeTiles,list):
			self.activeTiles = activeTiles
		else:
			self.activeTiles = [activeTiles] # how many tiles are active for becomming "builder" tiles in case builders extinct
		if isinstance(growCount,list):
			self.growCount = growCount
		else:
			self.growCount = [growCount] # how much tiles one builder tile tries to spawn per life
		self.continentSeparation = continentSeparation # how far same type of tiles (but not active) must be
		self.poleSeparation = 3
		self.baseTerrain = [] #add plot types to this array to make tile builder build only on them
		self.targetSize = targetSize
		self.allowReachPoles = True
		self.coastScatterMin = 1 # minimum continent scattering
		self.coastScatterMax = 2 # maximum scattering
		self.coastScatterChangeChance = 0.1 # scattering change chance per one tile grow
		self.coastScatterChangeDif = 1 # scatering maximum change per change
		self.initialScatter = -1 #initial scatter value; if == -1, will be chosen randomly
		self.refreshCountRequired = 6
		self.continentPartControlEnabled = True
		self.continentParts = bArray()
		self.continentPartsIdGen = -1
		self.continentPartAllTiles = bArray()
		# K-Mod-Magnus: with a LandPlan, new plots must keep the separation from other continents and islands and
		# must not merge with them (the plan checks every plot before it becomes land)
		self.plan = None
		self.group = 0
		self.mass = 0
		self.otherBits = 0
		self.contiguous = False # with a plan: only grow next to its own land (all but archipelago continents)
		self.searches = 0 # slow candidate searches so far

	def usePlan(self, plan, group, mass):
		self.plan = plan
		self.group = group
		self.mass = mass
		self.otherBits = ~(1 << group)

	def start(self, x, y):
		self.progress = 0
		self.refreshCount = 0
		self.nextRefresh = self.targetSize / float(self.refreshCountRequired + 1) * (self.refreshCount + 1)
		offset = self.world.getOffset([x,y])
		if self.plan is not None:
			self.plan.setLand(offset, self.mass, self.group)
		else:
			self.world[x,y] = self.tileType[0]
		if self.continentPartControlEnabled:
			self.continentPartsIdGen += 1
			part = ContinentPart(self.continentPartsIdGen,self)
			self.continentParts[part.id] = part
			part.tiles[offset] = offset
			self.continentPartAllTiles[offset] = part.id
		if self.initialScatter == -1:
			if self.coastScatterMin == 0:
				initial_scatt = 0
			else:
				initial_scatt = self.coastScatterMin
		else:
			initial_scatt = self.initialScatter
		if getRand(self.dice) > 0.5:
			initial_scattdir = 1
		else:
			initial_scattdir = -1
		self.builders = bArray() # for storing sorted builder tiles for fast access
		self.builders[offset] = [x, y, initial_scatt, initial_scattdir]
		self.active = bArray() # for storing sorted active tiles for fast access
		self.allTiles = bArray()
		self.active[offset] = [x, y, self.getPrValue(self.activeTiles) / self.getPrValue(self.growCount), initial_scatt, initial_scattdir]
		self.allTiles[offset] = [x, y, self.getPrValue(self.activeTiles) / self.getPrValue(self.growCount), initial_scatt, initial_scattdir]
		self._growInd = 0
		self.builderList = [] # for storing sequence in which builder tiles were created
		self.builderList.append(offset)
		self.possibleTiles = bArray() # for storing possible grow tile list

	def getPrValue(self, prArray):
		if len(prArray) == 1:
			return int(prArray[0])
		elif self.progress >= self.targetSize:
			return int(prArray[len(prArray) - 1])
		else:
			scale = self.progress / float(self.targetSize)
			onepiece = 1 / float(len(prArray))
			piecesum = onepiece
			index = 0
			while scale > piecesum:
				piecesum += onepiece
				index += 1
			if index >= len(prArray) - 1:
				return int(prArray[len(prArray) - 1])
			else:
				piecesum -= onepiece
				lscale = (scale - piecesum) / float(onepiece)
				return int(prArray[index] + (prArray[index + 1] - prArray[index]) * lscale)

	def grow(self):
		world = self.world
		result = True

		builderTiles = self.getPrValue(self.builderTiles)
		activeTiles = self.getPrValue(self.activeTiles)
		growCount = self.getPrValue(self.growCount)

		# increase index (because only one tile per function)
		self._growInd += 1
		# pradzioj rasti langelius aplink
		if self._growInd == 1 and len(self.possibleTiles) < 8: # if there is shortage of possible tiles, remake possible tile list
			self.possibleTiles = self._getPossibleGrowTiles()

		#check if special refresh is needed
		if self.progress > self.nextRefresh:
			#print " ------------------ SPECIAL REFRESH -------------------- "
			self.refreshCount += 1
			self.nextRefresh = self.targetSize / float(self.refreshCountRequired + 1) * (self.refreshCount + 1)
			for index in range(int(activeTiles / 15)):
				tile = self.allTiles.randomItem(self.dice)
				if tile != None:
					x, y, d, scatt, scattdir = tile
					if not self._intersect(x,y):
						self._makeActive(x,y)
				else:
					break

		self._findMoreTiles(builderTiles, activeTiles)

		# grow one tile
		tile = self._takeValidTile()
		if tile == None and self.plan is not None:
			# K-Mod-Magnus: the candidates left were no longer valid (other land grew near them); look once more
			self.possibleTiles = self._getPossibleGrowTiles()
			self._findMoreTiles(builderTiles, activeTiles)
			tile = self._takeValidTile()
		if tile != None:
			x, y, scatt, scattdir, origoffset = tile
			offset = world.getOffset([x, y])
			self.active[offset] = [x, y, activeTiles / growCount, scatt, scattdir]
			self.allTiles[offset] = [x, y, activeTiles / growCount, scatt, scattdir]
			if self.plan is not None:
				self.plan.setLand(offset, self.mass, self.group)
			else:
				world[x, y] = self.tileType[0]
			#zx, zy = self.world.getPos(offset)
			#print "Offset %d translated to x:%d y:%d while original is x:%d y:%d"%(offset,zx,zy,x,y)
			if self.continentPartControlEnabled:
				tiles = getTilesAround(x,y)
				partsAround = bArray()
				for tile in tiles:
					tx, ty = tile
					toffset =  world.getOffset([tx, ty])
					partId = self.continentPartAllTiles[toffset]
					if partId != None:
						partsAround[partId] = partId
				if len(partsAround) == 0:
					self.continentPartsIdGen += 1 #gaunam nauja id
					part = ContinentPart(self.continentPartsIdGen,self) #sukuriam nauja part
					part.tiles[offset] = offset #idedam tile i sarasa
					self.continentParts[part.id] = part #idedam part i sarasa
					#print "New lonely tile: x:%d y:%d id:%d"%(x,y,part.id)
					self.continentPartAllTiles[offset] = part.id #idedam part id i tile sarasa
					currentPartId = self.continentPartAllTiles[origoffset] #gaunam dabartinio part id
					currentPart = None
					if currentPartId != None:
						currentPart = self.continentParts[currentPartId] #gaunam dabartinio part obj
					if currentPart != None: # K-Mod-Magnus: the part may have been merged away (crashed before)
						part.links[currentPart.id] = [currentPart.id, offset, origoffset] #idedam linka
						#print "Add link from id:%d to id:%d orig:%d new:%d"%(part.id,currentPart.id,offset,origoffset)
						currentPart.links[part.id] = [part.id, origoffset, offset] #idedam linka
					#print "Add link from id:%d to id:%d orig:%d new:%d"%(currentPart.id,part.id,origoffset,offset)
				else:
					part = self.continentParts[partsAround._data[0][1]]
					#print "Join tile x:%d y:%d with %d continent parts, main part id: %d"%(x,y,len(partsAround),part.id)
					for i in range(len(partsAround) - 1):
						nextPart = self.continentParts[partsAround._data[i+1][1]]
						#print "Join with id: %d"%(nextPart.id)
						for partTileOffset in nextPart.tiles:
							part.tiles[partTileOffset] = partTileOffset
							self.continentPartAllTiles[partTileOffset] = part.id
						nextPart.tiles = None
						for link in nextPart.links:
							otherId, thisOffset, otherOffset = link
							if otherId == part.id:
								part.links.remove(nextPart.id)
							else:
								part.links[otherId] = link
								#print "Add link from id:%d to id:%d orig:%d new:%d"%(part.id,otherId,thisOffset,otherOffset)
								otherPart = self.continentParts[otherId]
								otherPart.links.remove(nextPart.id)
								#print "Remove link from id:%d to id:%d"%(otherPart.id,nextPart.id)
								otherPart.links[part.id] = [part.id, otherOffset, thisOffset]
								#print "Add link from id:%d to id:%d orig:%d new:%d"%(otherPart.id,part.id,otherOffset,thisOffset)
						nextPart.links = None
						nextPart.builder = None
						self.continentParts.remove(nextPart.id)
						#print "Remove part: %d"%(nextPart.id)
					part.tiles[offset] = offset
					self.continentPartAllTiles[offset] = part.id
			#add tile as possible builder
			scale = self.progress / float(self.targetSize)
			if scale > 1:
				scale = 1
			elif scale <=0:
				scale = 0.01
			if scattdir < 0:
				bChance = self.coastScatterChangeChance + self.coastScatterChangeChance / (1 + scale * 5)
			else:
				bChance = self.coastScatterChangeChance - self.coastScatterChangeChance / (1 + scale * 5)
			if getRand(self.dice) < bChance:
				scattchange = scattdir * (1 + int(self.coastScatterChangeDif * getRand(self.dice)))
				if scatt + scattchange > self.coastScatterMax:
					scatt = self.coastScatterMax
					scattdir = -scattdir
				elif scatt + scattchange < self.coastScatterMin:
					scatt = self.coastScatterMin
					scattdir = -scattdir
				else:
					scatt += scattchange
			self.builders[offset] = [x, y, scatt, scattdir]
			self.builderList.append(offset)
		else:
			return False

		# readjusting builders
		while len(self.builders) > builderTiles:
			offset = self.builderList.pop(0)
			self.builders.remove(offset)

		if self._growInd == growCount:
			self._growInd = 0
			#decrease active tiles
			i = len(self.active)
			while i > 0:
				i -= 1
				if self.active._data[i][1][2] < 1:
					self.active.pop(i)
				else:
					self.active._data[i][1][2] -= 1

		self.progress += 1

		return result

	def _findMoreTiles(self, builderTiles, activeTiles):
		"""Refills self.possibleTiles when it is empty (moved out of grow() unchanged, for K-Mod-Magnus)."""
		world = self.world
		# try to find more tiles if there is no more
		#fast method first:
		if len(self.possibleTiles) == 0:
			self.builders = bArray()
			c = 0
			for j in range(builderTiles * 2):
				tile = self.active.randomItem(self.dice)
				if tile != None:
					x, y, d, scatt, scattdir = tile
					c = self._checkPosGrow(self.possibleTiles,x,y,scatt,scattdir)
					if (c > 0):
						if len(self.builders) < builderTiles:
							offset = world.getOffset([x,y])
							self.builders[offset] = [x, y, scatt, scattdir]
							self.builderList.append(offset)
						else:
							break
				else:
					break

		# K-Mod-Magnus: with a plan, a continent that keeps needing the slow searches below is nearly stuck; it stops
		# here and the plan grows it at its sides (balanceLand), which is much faster (the searches look at every
		# plot of the continent each time, and took minutes on big maps)
		if len(self.possibleTiles) == 0 and self.plan is not None:
			self.searches += 1
			if self.searches > 6:
				return

		#extreme method (slow as hell):
		if len(self.possibleTiles) == 0:
			#print " ------------------ USING EXTREME METHOD -------------------- "
			newActive = bArray()
			for index in range(len(self.active)):
				tile = self.active.random(self.dice)
				if tile != None:
					x, y, d, scatt, scattdir = tile
					offset = world.getOffset([x,y])
					newActive[offset] = tile
					c = self._checkPosGrow(self.possibleTiles,x,y,scatt,scattdir)
					if (c > 0):
						if len(self.builders) < builderTiles:
							self.builders[offset] = [x, y, scatt, scattdir]
							self.builderList.append(offset)
			self.active = newActive

		if len(self.possibleTiles) == 0:
			c = 0
			#print " --------------- USING EMERGENCY TILE SEARCH ----------------- "
			#clear active tiles, we will make new
			self.active = bArray()
			for index in range(len(self.allTiles)):
				tile = self.allTiles.randomItem(self.dice)
				if tile != None:
					x, y, d, scatt, scattdir = tile
					if self._makeActive(x,y):
						c += self._checkPosGrow(self.possibleTiles,x,y,scatt,scattdir)
						if len(self.active) >= activeTiles:
							break
				else:
					break
			#add tiles to builders
			if (c > 0):
				while len(self.builders) < builderTiles:
					tile = self.possibleTiles.random(self.dice)
					if (tile != None):
						if len(self.builders) < builderTiles:
							x, y, scatt, scattdir, origoffset = tile
							offset = world.getOffset([x,y])
							self.builders[offset] = [x, y, scatt, scattdir]
							self.builderList.append(offset)
						else:
							break
					else:
						break

	def _takeValidTile(self):
		"""Takes a random candidate; with a LandPlan, only one that may still become land (K-Mod-Magnus: candidates
		are found long before they are used, and used to be placed even when other land had grown next to them)."""
		while len(self.possibleTiles) > 0:
			tile = self.possibleTiles.random(self.dice)
			if self.plan is None:
				return tile
			o = self.world.getOffset([tile[0], tile[1]])
			if self.plan.canAdd(o, self.group, self.mass) and (not self.contiguous or self.plan.touches(o, self.mass)):
				return tile
		return None

	def joinParts(self, minSize):
		if self.continentPartControlEnabled:
			bUsed = bArray()
			for part in self.continentParts:
				if bUsed[part.id] == None:
					if len(part.tiles) >= minSize:
						for link in part.links:
							otherId, thisOffset, otherOffset = link
							if bUsed[otherId] == None:
								otherPart = self.continentParts[otherId]
								usedTiles = bArray()
								usedTiles[part.id] = True
								if otherPart.deepMaxTiles(usedTiles) >= minSize:
									ax, ay = self.world.getPos(thisOffset)
									#print "This offset %d translated to x:%d y:%d"%(thisOffset,ax,ay)
									bx, by = self.world.getPos(otherOffset)
									#print "Other offset %d translated to x:%d y:%d"%(otherOffset,bx,by)
									if ax > bx:
										if ax - bx > self.world.getWidth() + bx - ax:
											bx += self.world.getWidth()
									else:
										if bx - ax > self.world.getWidth() + ax - bx:
											ax += self.world.getWidth()
									dx = bx - ax
									dy = by - ay
									if abs(dx) > abs(dy):
										d = abs(dx)
									else:
										d = abs(dy)
									p = 0
									if dx < 0:
										dx -= 1
									elif dx > 0:
										dx += 1
									if dy < 0:
										dy -= 1
									elif dy > 0:
										dy += 1
									while p < d - 1:
										p += 1
										progress = p / float(d + 1)
										x = int(round(dx * progress,0)) + ax
										y = int(round(dy * progress,0)) + ay
										self.world[x,y] = PlotTypes.PLOT_LAND
								else:
									bUsed[otherId] = None
					bUsed[part.id] = True

	def _makeActive(self, x, y):
		dice = self.dice

		cs = self.continentSeparation
		r = range(cs * 2)

		for bx in r:
			for by in r:
				ax = x - cs + bx
				ay = y - cs + by
				if self.world[ax, ay] in self.tileType:
					offset = self.world.getOffset([ax, ay])
					tile = self.allTiles[offset]
					if tile != None:
						self.active[offset] = tile
					else:
						return False
		return True

	def _intersect(self, x, y):
		dice = self.dice

		# K-Mod-Magnus: other continents and islands, within the separation in any direction (the ring below only
		# looks at plots exactly that far away)
		if self.plan is not None and self.plan.claims[self.world.getOffset([x, y])] & self.otherBits:
			return True

		cs = self.continentSeparation
		r = range(cs * 2)

		for i in r:
			ax = x - cs + i
			ay = y + cs
			offset = self.world.getOffset([ax, ay])
			if (not self.active[offset]) and self.world[ax, ay] in self.tileType:
				return True
		for i in r:
			ax = x + cs
			ay = y + cs - i
			offset = self.world.getOffset([ax, ay])
			if (not self.active[offset]) and self.world[ax, ay] in self.tileType:
				return True
		for i in r:
			ax = x + cs - i
			ay = y - cs
			offset = self.world.getOffset([ax, ay])
			if (not self.active[offset]) and self.world[ax, ay] in self.tileType:
				return True
		for i in r:
			ax = x - cs
			ay = y - cs + i
			offset = self.world.getOffset([ax, ay])
			if (not self.active[offset]) and self.world[ax, ay] in self.tileType:
				return True
		return False

	# dont ask me to explain anything here, just keep reading and enjoy :D
	def _checkPosGrowMatrix(self,matrix,list,x,y,scatt,scattdir):
		world = self.world
		c = 0
		h = len(matrix)
		w = len(matrix[0])
		dh = (h - 1) / 2
		dw = (w - 1) / 2

		ps = self.poleSeparation

		if self.allowReachPoles:
			ps = 1
		for ax in range(w):
			for ay in range(h):
				cell = matrix[ay][ax]
				if cell > 0:
					px = x + (ax - dw)
					py = y + (ay - dh)
					if (len(self.baseTerrain) == 0):
						wtt = world[px,py]
						if py > y:
							if py < world._dimensions[0]-ps-1 and not wtt in self.tileType and not self._intersect(px,py):
								list[world.getOffset([px,py])] = [px,py,scatt,scattdir,world.getOffset([x,y])]
								c += 1
						elif py < y:
							if py > ps and not wtt in self.tileType and not self._intersect(px,py):
								list[world.getOffset([px,py])] = [px,py,scatt,scattdir,world.getOffset([x,y])]
								c += 1
						else:
							if not wtt in self.tileType and not self._intersect(px,py):
								list[world.getOffset([px,py])] = [px,py,scatt,scattdir,world.getOffset([x,y])]
								c += 1
					else:
						wtt = world[px,py]
						if py > y:
							if py < world._dimensions[0]-ps-1 and wtt in self.baseTerrain and not self._intersect(px,py):
								list[world.getOffset([px,py])] = [px,py,scatt,scattdir,world.getOffset([x,y])]
								c += 1
						elif py < y:
							if py > ps and wtt in self.baseTerrain and not self._intersect(px,py):
								list[world.getOffset([px,py])] = [px,py,scatt,scattdir,world.getOffset([x,y])]
								c += 1
						else:
							if wtt in self.baseTerrain and not self._intersect(px,py):
								list[world.getOffset([px,py])] = [px,py,scatt,scattdir,world.getOffset([x,y])]
								c += 1
		return c

	# dont ask me to explain anything here, just keep reading and enjoy :D
	def _checkPosGrowList(self,matrix,list,x,y,scatt,scattdir):
		world = self.world
		c = 0

		ps = self.poleSeparation

		if self.allowReachPoles:
			ps = 1
		for cell in matrix:
			px, py = cell
			if (len(self.baseTerrain) == 0):
				wtt = world[px,py]
				if py > y:
					if py < world._dimensions[0]-ps-1 and not wtt in self.tileType and not self._intersect(px,py):
						list[world.getOffset([px,py])] = [px,py,scatt,scattdir,world.getOffset([x,y])]
						c += 1
				elif py < y:
					if py > ps and not wtt in self.tileType and not self._intersect(px,py):
						list[world.getOffset([px,py])] = [px,py,scatt,scattdir,world.getOffset([x,y])]
						c += 1
				else:
					if not wtt in self.tileType and not self._intersect(px,py):
						list[world.getOffset([px,py])] = [px,py,scatt,scattdir,world.getOffset([x,y])]
						c += 1
			else:
				wtt = world[px,py]
				if py > y:
					if py < world._dimensions[0]-ps-1 and wtt in self.baseTerrain and not self._intersect(px,py):
						list[world.getOffset([px,py])] = [px,py,scatt,scattdir,world.getOffset([x,y])]
						c += 1
				elif py < y:
					if py > ps and wtt in self.baseTerrain and not self._intersect(px,py):
						list[world.getOffset([px,py])] = [px,py,scatt,scattdir,world.getOffset([x,y])]
						c += 1
				else:
					if wtt in self.baseTerrain and not self._intersect(px,py):
						list[world.getOffset([px,py])] = [px,py,scatt,scattdir,world.getOffset([x,y])]
						c += 1
		return c

	def _checkPosGrow(self,list,x,y,scatt,scattdir):
		world = self.world
		dice = self.dice
		c = 0
		iH = world.getHeight()
		#matrix of what tiles to check around builder tiles. 1 - available for building, 0 - not
		if scatt == 0:
			this_scatt = 1
		else:
			this_scatt = scatt

		if (this_scatt == 1):
			flipUp = False
			flipDown = False
			if y > iH / 2: #get chance to flip up
				scale = (iH - y) / (iH / 2.0) # 0 - equator, 1 - pole
				if getRand(dice) < scale:
					flipUp = True
			elif y < iH / 2:
				scale = 1 - y / (iH / 2.0) # 0 - equator, 1 - pole
				if getRand(dice) < scale:
					flipDown = True
			if flipUp:
				default_matrix = [
					[1,1,1],
					[1,0,1],
					[1,0,1],
				]
			elif flipDown:
				default_matrix = [
					[1,0,1],
					[1,0,1],
					[1,1,1],
				]
			else:
				default_matrix = [
					[1,1,1],
					[1,0,1],
					[1,1,1],
				]
			c += self._checkPosGrowMatrix(default_matrix, list, x, y, scatt, scattdir)
		else:
			#get chance to flip
			half = iH / 2
			if y < half / 3 * 2:
				scale = 1 - y / float(half / 3 * 2 - 1)
			elif y >= half / 3 * 4:
				scale = - (y - half / 3 * 4) / float(half / 3 * 2 - 1)
			else:
				scale = 0
			matrix_list = bArray()
			point_count = int(8 + (this_scatt - 1) * 2.5)
			for index in range(point_count):
				shift = 0.8
				t = getRand(dice) * 3.141592 * 2
				r = getRand(dice) * (this_scatt + 1) + 1
				rx = int(r * math.cos(t))
				ry = r * math.sin(t)
				if not scale == 0:
					if scale > 0:
						if ry < 0:
							ry = int(ry - scale * shift * ry)
						else:
							ry = int(ry)
					else:
						if ry > 0:
							ry = int(ry + scale * shift * ry)
						else:
							ry = int(ry)
				else:
					ry = int(ry)
				if (not (rx == 0 and ry == 0)):
					matrix_list[world.getOffset([x + rx,y + ry])] = [x + rx, y + ry]
			c += self._checkPosGrowList(matrix_list, list, x, y, scatt, scattdir)
		return c

	def _getPossibleGrowTiles(self):
		world = self.world
		result = bArray()
		rem_ind = []
		i = 0
		for item in self.builders:
			x, y, scatt, scattdir = item
			c = self._checkPosGrow(result,x,y,scatt,scattdir)
			if c == 0:
				rem_ind.append(i)
			i += 1
		# remove unecessary builder tiles
		x = len(rem_ind)
		while x > 0:
			x -= 1
			self.builders.pop(x)
		return result

#------------------------------------------------------
"""LAND PLANNING (K-Mod-Magnus)"""
#------------------------------------------------------
# Written for K-Mod-Magnus (2026) so that the land options do what they say. LandPlan knows the continent ("group")
# and the landmass ("mass") of every land plot while the plot types are made. With it the continents get exactly
# the land the ocean percentage leaves (islands are left out of it), continents keep the chosen separation from
# each other and from islands, every continent stays one landmass (scattering now puts islands near the coast
# instead of flinging parts of the continent away), lonely islands fill the empty ocean as asked, and the
# Coastline option reshapes the continents for more coast plots (the DLL makes every water plot next to land
# coast, the rest ocean).

RING_DIRS = [[0,1],[1,1],[1,0],[1,-1],[0,-1],[-1,-1],[-1,0],[-1,1]] # N, NE, E, SE, S, SW, W, NW

def _ringConnected(mask):
	"""True if the plots marked in mask (bit i = RING_DIRS[i] around a plot) are connected to each other without
	the plot in the middle (8-neighbour connectivity, as Civ4 makes its areas)."""
	pos = []
	for i in range(8):
		if mask & (1 << i):
			pos.append(i)
	if len(pos) == 0:
		return False
	adj = []
	for i in range(8):
		adj.append([(i + 1) % 8, (i + 7) % 8])
	for a, b in [[0,2],[2,4],[4,6],[6,0]]:
		adj[a].append(b)
		adj[b].append(a)
	seen = {pos[0]: True}
	stack = [pos[0]]
	while len(stack) > 0:
		i = stack.pop()
		for j in adj[i]:
			if (mask & (1 << j)) and not seen.has_key(j):
				seen[j] = True
				stack.append(j)
	return len(seen) == len(pos)

# SIMPLE_RING[mask]: removing a plot whose same-landmass neighbours are mask keeps the landmass in one piece
SIMPLE_RING = []
for _mask in range(256):
	SIMPLE_RING.append(_ringConnected(_mask))

class RandSet:
	"Set of plot offsets with random picks in constant time."
	def __init__(self):
		self.items = []
		self.pos = {}

	def __len__(self):
		return len(self.items)

	def add(self, o):
		if not self.pos.has_key(o):
			self.pos[o] = len(self.items)
			self.items.append(o)

	def remove(self, o):
		i = self.pos.pop(o, None)
		if i is not None:
			last = self.items.pop()
			if i < len(self.items):
				self.items[i] = last
				self.pos[last] = i

	def pick(self, dice):
		return self.items[dice.get(len(self.items), "Python - Planet Generator: random plot")]

class LandPlan:
	def __init__(self, world, separation, allowLandNearIce, dice):
		self.world = world
		self.data = world._data # plot types, offset = y * width + x
		W = world.getWidth()
		H = world.getHeight()
		self.W = W
		self.H = H
		self.N = W * H
		if separation < 1:
			separation = 1
		self.sep = separation
		self.dice = dice
		# the rows TileBuilder may grow into (it keeps one row more than poleSeparation free)
		if allowLandNearIce:
			ps = 1
		else:
			ps = 3
		self.minY = ps + 1
		self.maxY = H - ps - 2
		N = self.N
		self.owner = [-1] * N # group of each land plot: continents 0.., lonely islands after them
		self.mass = [-1] * N # landmass of each land plot: a continent's body has the continent's number, islands 1000..
		self.claims = [0] * N # bit g: group g has land within the separation
		self.landNb = [0] * N # land plots among the 8 neighbours
		self.land = 0
		self.contLand = 0 # land plots of the continents (landmasses below 1000): the islands do not count
		self.massTiles = {}
		self.massCoast = {} # land plots of the mass with water next to them
		self.massGroup = {}
		self.nextIsland = 1000
		self.nextGroup = 0
		self.growCache = {} # landmass -> candidates from its last full scan (used up before scanning again)
		nb8 = []
		ring = []
		for o in range(N):
			x = o % W
			y = o // W
			r = []
			n = []
			for d in RING_DIRS:
				yy = y + d[1]
				if yy < 0 or yy >= H:
					r.append(-1)
				else:
					oo = yy * W + (x + d[0]) % W
					r.append(oo)
					n.append(oo)
			ring.append(r)
			nb8.append(n)
		self.nb8 = nb8
		self.ring = ring

	def newGroup(self):
		g = self.nextGroup
		self.nextGroup += 1
		return g

	def newIslandMass(self):
		m = self.nextIsland
		self.nextIsland += 1
		return m

	def isWater(self, o):
		return self.data[o] == PlotTypes.PLOT_OCEAN

	def rowOK(self, o):
		y = o // self.W
		return y >= self.minY and y <= self.maxY

	def canAdd(self, o, g, m):
		"""Can water plot o become land of landmass m (group g)? Not in the pole margin, not within the
		separation of another group's land, and not next to another landmass (that would merge them)."""
		if self.data[o] != PlotTypes.PLOT_OCEAN:
			return False
		y = o // self.W
		if y < self.minY or y > self.maxY:
			return False
		if self.claims[o] & ~(1 << g):
			return False
		mass = self.mass
		for n in self.nb8[o]:
			k = mass[n]
			if k != -1 and k != m:
				return False
		return True

	def touches(self, o, m):
		"Is plot o next to land of landmass m?"
		mass = self.mass
		for n in self.nb8[o]:
			if mass[n] == m:
				return True
		return False

	def setLand(self, o, m, g):
		self.data[o] = PlotTypes.PLOT_LAND
		self.owner[o] = g
		self.mass[o] = m
		if not self.massTiles.has_key(m):
			self.massTiles[m] = RandSet()
			self.massCoast[m] = RandSet()
			self.massGroup[m] = g
		self.massTiles[m].add(o)
		self.land += 1
		if m < 1000:
			self.contLand += 1
		landNb = self.landNb
		mass = self.mass
		for n in self.nb8[o]:
			landNb[n] += 1
		self._updateCoast(o)
		for n in self.nb8[o]:
			if mass[n] != -1:
				self._updateCoast(n)
		self._claim(o, 1 << g)

	def setWater(self, o):
		m = self.mass[o]
		self.data[o] = PlotTypes.PLOT_OCEAN
		self.owner[o] = -1
		self.mass[o] = -1
		self.massTiles[m].remove(o)
		self.massCoast[m].remove(o)
		self.land -= 1
		if m < 1000:
			self.contLand -= 1
		landNb = self.landNb
		mass = self.mass
		for n in self.nb8[o]:
			landNb[n] -= 1
			if mass[n] != -1:
				self._updateCoast(n)
		# claims are not taken back: they only ever keep other land further away than needed

	def _updateCoast(self, o):
		if self.landNb[o] < len(self.nb8[o]):
			self.massCoast[self.mass[o]].add(o)
		else:
			self.massCoast[self.mass[o]].remove(o)

	def _claim(self, o, bit):
		W = self.W
		s = self.sep
		x = o % W
		y = o // W
		y0 = y - s
		if y0 < 0:
			y0 = 0
		y1 = y + s
		if y1 > self.H - 1:
			y1 = self.H - 1
		claims = self.claims
		xs = []
		for dx in range(-s, s + 1):
			xs.append((x + dx) % W)
		for yy in range(y0, y1 + 1):
			row = yy * W
			for xx in xs:
				claims[row + xx] |= bit

	def coastAddDelta(self, o):
		"change in coast plots if water plot o becomes land"
		d = 0
		if self.landNb[o] > 0:
			d = -1
		data = self.data
		landNb = self.landNb
		for n in self.nb8[o]:
			if landNb[n] == 0 and data[n] == PlotTypes.PLOT_OCEAN:
				d += 1
		return d

	def coastRemoveDelta(self, o):
		"change in coast plots if land plot o becomes water"
		d = 0
		if self.landNb[o] > 0:
			d = 1
		data = self.data
		landNb = self.landNb
		for n in self.nb8[o]:
			if landNb[n] == 1 and data[n] == PlotTypes.PLOT_OCEAN:
				d -= 1
		return d

	def coastCount(self):
		c = 0
		data = self.data
		landNb = self.landNb
		for o in xrange(self.N):
			if landNb[o] > 0 and data[o] == PlotTypes.PLOT_OCEAN:
				c += 1
		return c

	def isSimple(self, o):
		"Can land plot o go without splitting its landmass?"
		m = self.mass[o]
		mask = 0
		bit = 1
		mass = self.mass
		for n in self.ring[o]:
			if n >= 0 and mass[n] == m:
				mask |= bit
			bit <<= 1
		return SIMPLE_RING[mask]

	def keepsOrthogonalLinks(self, o):
		"Would every land neighbour of o (same landmass) still touch its landmass at a side, not only a corner?"
		m = self.mass[o]
		ring = self.ring
		mass = self.mass
		r = ring[o]
		for i in [0, 2, 4, 6]:
			n = r[i]
			if n >= 0 and mass[n] == m:
				rn = ring[n]
				links = 0
				for j in [0, 2, 4, 6]:
					k = rn[j]
					if k >= 0 and k != o and mass[k] == m:
						links += 1
				if links == 0:
					return False
		return True

	def distance(self, a, b):
		"plots between a and b in the Chebyshev metric (x wraps)"
		W = self.W
		dx = abs(a % W - b % W)
		if dx > W - dx:
			dx = W - dx
		dy = abs(a // W - b // W)
		if dx > dy:
			return dx
		return dy

	def distanceMap(self, sources, limit):
		"""Distance (Chebyshev, x wraps) of every plot from the nearest source plot; limit + 1 beyond limit."""
		dist = [limit + 1] * self.N
		frontier = []
		for o in sources:
			dist[o] = 0
			frontier.append(o)
		d = 0
		nb8 = self.nb8
		while len(frontier) > 0 and d < limit:
			d += 1
			nxt = []
			for o in frontier:
				for n in nb8[o]:
					if dist[n] > d:
						dist[n] = d
						nxt.append(n)
			frontier = nxt
		return dist

	def findSeed(self, o, g, m):
		"the plot nearest to o (rings around it) that landmass m of group g may start on, or -1"
		if self.canAdd(o, g, m):
			return o
		W = self.W
		x0 = o % W
		y0 = o // W
		for r in range(1, W):
			for dy in range(-r, r + 1):
				y = y0 + dy
				if y < 0 or y >= self.H:
					continue
				for dx in range(-r, r + 1):
					if abs(dx) == r or abs(dy) == r:
						oo = y * W + (x0 + dx) % W
						if self.canAdd(oo, g, m):
							return oo
		return -1

	def _growCandidate(self, m, g, mode, tries):
		"a water plot at a side of landmass m that may become land, chosen by mode; -1 if none"
		coast = self.massCoast[m]
		if len(coast) == 0:
			return -1
		dice = self.dice
		ring = self.ring
		best = -1
		bestValue = -9999
		for t in range(tries):
			o = coast.pick(dice)
			n = ring[o][2 * dice.get(4, "Python - Planet Generator: grow direction")]
			if n < 0 or not self.canAdd(n, g, m):
				continue
			if mode == "coast" and not self.keepsWaterLinked(n):
				continue
			v = self._growValue(n, mode)
			if v > bestValue:
				bestValue = v
				best = n
		if best < 0:
			# the samples missed: candidates left from the last full scan, then a new full scan (every side of the
			# landmass; then the corners too, closing off a lake if nothing else is left)
			cache = self.growCache.get(m)
			if cache is not None:
				while len(cache) > 0:
					n = cache.pop()
					if self.canAdd(n, g, m) and self.touches(n, m):
						return n
			cache = self._scanCandidates(m, g, [0, 2, 4, 6], mode == "coast")
			if len(cache) == 0:
				cache = self._scanCandidates(m, g, range(8), False)
			self.growCache[m] = cache
			while len(cache) > 0:
				n = cache.pop()
				if self.canAdd(n, g, m) and self.touches(n, m):
					return n
		return best

	def _scanCandidates(self, m, g, dirs, linked):
		"every plot at the given sides of landmass m that may become land, in random order"
		ring = self.ring
		seen = {}
		cands = []
		for o in self.massCoast[m].items:
			for i in dirs:
				n = ring[o][i]
				if n >= 0 and not seen.has_key(n) and self.canAdd(n, g, m) and (not linked or self.keepsWaterLinked(n)):
					seen[n] = True
					cands.append(n)
		dice = self.dice
		for i in range(len(cands) - 1, 0, -1):
			j = dice.get(i + 1, "Python - Planet Generator: grow order")
			cands[i], cands[j] = cands[j], cands[i]
		return cands

	def _growValue(self, n, mode):
		r = self.dice.get(4, "Python - Planet Generator: grow choice")
		if mode == "coast":
			return self.coastAddDelta(n) * 4 + r
		if mode == "compact":
			return self.landNb[n] * 4 + r
		if mode == "organic":
			return self.landNb[n] + self.dice.get(6, "Python - Planet Generator: grow choice")
		return self.dice.get(100, "Python - Planet Generator: grow choice")

	def growMass(self, m, g, count, mode):
		"""Adds up to count plots at the sides of landmass m: mode "compact" fills bays first, "organic" a little,
		"coast" makes the most new coast, anything else grows at random. Returns the plots added."""
		added = 0
		while added < count:
			n = self._growCandidate(m, g, mode, 16)
			if n < 0:
				break
			self.setLand(n, m, g)
			added += 1
		return added

	def _shrinkCandidate(self, m, mode, tries):
		coast = self.massCoast[m]
		if len(coast) == 0 or len(self.massTiles[m]) < 2:
			return -1
		dice = self.dice
		best = -1
		bestValue = -9999
		for t in range(tries):
			o = coast.pick(dice)
			if not self.isSimple(o) or not self.keepsOrthogonalLinks(o):
				continue
			v = self._shrinkValue(o, mode)
			if v > bestValue:
				bestValue = v
				best = o
		if best < 0:
			for o in coast.items:
				if self.isSimple(o) and self.keepsOrthogonalLinks(o):
					v = self._shrinkValue(o, mode)
					if v > bestValue:
						bestValue = v
						best = o
		return best

	def _shrinkValue(self, o, mode):
		r = self.dice.get(4, "Python - Planet Generator: shrink choice")
		if mode == "coast":
			return self.coastRemoveDelta(o) * 4 + r
		if mode == "compact":
			return -self.landNb[o] * 4 + r # the most exposed plots first: smooths the coast
		return self.dice.get(100, "Python - Planet Generator: shrink choice")

	def shrinkMass(self, m, count, mode):
		"Removes up to count plots from the coast of landmass m without splitting it. Returns the plots removed."
		removed = 0
		while removed < count:
			o = self._shrinkCandidate(m, mode, 16)
			if o < 0:
				break
			self.setWater(o)
			removed += 1
		return removed

	def seaMap(self):
		"water connected to the polar rows (always water): the open sea, as opposed to lakes"
		sea = [False] * self.N
		data = self.data
		stack = []
		for o in range(self.W):
			for oo in [o, (self.H - 1) * self.W + o]:
				if data[oo] == PlotTypes.PLOT_OCEAN and not sea[oo]:
					sea[oo] = True
					stack.append(oo)
		nb8 = self.nb8
		while len(stack) > 0:
			o = stack.pop()
			for n in nb8[o]:
				if not sea[n] and data[n] == PlotTypes.PLOT_OCEAN:
					sea[n] = True
					stack.append(n)
		return sea

	def keepsWaterLinked(self, o):
		"Would the water around plot o stay connected if o became land (no new lake cut off)?"
		data = self.data
		mask = 0
		bit = 1
		for n in self.ring[o]:
			if n >= 0 and data[n] == PlotTypes.PLOT_OCEAN:
				mask |= bit
			bit <<= 1
		return mask == 0 or SIMPLE_RING[mask]

	def roughen(self, m, g, iterations, reach):
		"""Coastline option: moves plots of landmass m from where they make little coast to where they make more,
		keeping the landmass in one piece and its number of plots. Plots only go where the open sea reaches and
		only come where they cut no water off, so the coast gets fjords and peninsulas rather than lakes. With
		reach > 0 nothing changes further than that from the coast as it was, so the inland stays."""
		dice = self.dice
		coast = self.massCoast[m]
		ring = self.ring
		nb8 = self.nb8
		sea = self.seaMap()
		inland = None
		offshore = None
		if reach > 0:
			water = []
			for o in xrange(self.N):
				if self.data[o] == PlotTypes.PLOT_OCEAN:
					water.append(o)
			inland = self.distanceMap(water, reach) # land: plots to the coast
			offshore = self.distanceMap(self.massTiles[m].items, reach) # water: plots from this landmass
		gained = 0
		for it in xrange(iterations):
			if len(coast) < 4:
				break
			a = -1
			da = -9999
			for t in range(6):
				o = coast.pick(dice)
				nearSea = False
				for n in nb8[o]:
					if sea[n]:
						nearSea = True
						break
				if inland is not None and inland[o] > reach:
					continue
				if nearSea and self.isSimple(o) and self.keepsOrthogonalLinks(o):
					d = self.coastRemoveDelta(o)
					if d > da:
						da = d
						a = o
			if a < 0:
				continue
			b = -1
			db = -9999
			for t in range(6):
				o = coast.pick(dice)
				n = ring[o][2 * dice.get(4, "Python - Planet Generator: coastline direction")]
				if n < 0 or self.distance(n, a) <= 2:
					continue
				if offshore is not None and offshore[n] > reach:
					continue
				if self.canAdd(n, g, m) and self.keepsWaterLinked(n):
					d = self.coastAddDelta(n)
					if d > db:
						db = d
						b = n
			if b < 0:
				continue
			gain = da + db
			if gain > 0 or (gain == 0 and dice.get(8, "Python - Planet Generator: coastline drift") == 0):
				self.setWater(a)
				sea[a] = True
				self.setLand(b, m, g)
				sea[b] = False
				gained += gain
		return gained

	def pieces(self, m):
		"the 8-connected pieces of landmass m, biggest first"
		mass = self.mass
		nb8 = self.nb8
		seen = {}
		result = []
		for o in self.massTiles[m].items:
			if seen.has_key(o):
				continue
			piece = [o]
			seen[o] = True
			i = 0
			while i < len(piece):
				for n in nb8[piece[i]]:
					if mass[n] == m and not seen.has_key(n):
						seen[n] = True
						piece.append(n)
				i += 1
			result.append(piece)
		result.sort(lambda a, b: len(b) - len(a))
		return result

	def check(self, step, count):
		"debugging: every continent in one piece"
		for m in range(count):
			if self.massTiles.has_key(m) and len(self.massTiles[m]) > 0:
				ps = self.pieces(m)
				if len(ps) > 1:
					print "Planet Generator check after %s: continent %d in %d pieces %s" % (step, m, len(ps), [len(x) for x in ps][:5])

	def removeMass(self, m):
		removed = 0
		for o in list(self.massTiles[m].items):
			self.setWater(o)
			removed += 1
		return removed

	def fitIslands(self, minSize, mode):
		"""Minimum island size: islands below it grow to it, or go if there is no room. Returns the plots removed."""
		removed = 0
		for m in self.massTiles.keys():
			n = len(self.massTiles[m])
			if m >= 1000 and n > 0 and n < minSize:
				n += self.growMass(m, self.massGroup[m], minSize - n, mode)
				if n < minSize:
					removed += self.removeMass(m)
		return removed

	def removeSmallPieces(self, minSize):
		"""Removes islands below minSize, and the pieces of an archipelago continent below it (all but its biggest
		piece). Returns the plots removed."""
		removed = 0
		mass = self.mass
		nb8 = self.nb8
		for m in self.massTiles.keys():
			tiles = self.massTiles[m].items
			if len(tiles) == 0:
				continue
			if m >= 1000:
				if len(tiles) < minSize:
					removed += self.removeMass(m)
				continue
			# the pieces of a continent (more than one only for archipelago continents)
			seen = {}
			pieces = []
			for o in tiles:
				if seen.has_key(o):
					continue
				piece = [o]
				seen[o] = True
				i = 0
				while i < len(piece):
					for n in nb8[piece[i]]:
						if mass[n] == m and not seen.has_key(n):
							seen[n] = True
							piece.append(n)
					i += 1
				pieces.append(piece)
			if len(pieces) > 1:
				biggest = 0
				for i in range(len(pieces)):
					if len(pieces[i]) > len(pieces[biggest]):
						biggest = i
				for i in range(len(pieces)):
					if i != biggest and len(pieces[i]) < minSize:
						for o in pieces[i]:
							self.setWater(o)
							removed += 1
		return removed

	def islandAt(self, seed, size, g, mode):
		"""A new island of up to size plots from seed (group g: a continent's own islands, or a lonely island's
		own group). Returns the plots placed."""
		m = self.newIslandMass()
		if not self.canAdd(seed, g, m):
			return 0
		self.setLand(seed, m, g)
		if size > 1:
			return 1 + self.growMass(m, g, size - 1, mode)
		return 1

	def room(self, o, limit):
		"""Plots a new landmass from water plot o could grow over (8-connected, outside the separation of all land),
		counted up to limit"""
		claims = self.claims
		data = self.data
		if claims[o] != 0 or data[o] != PlotTypes.PLOT_OCEAN or not self.rowOK(o):
			return 0
		seen = {o: True}
		todo = [o]
		i = 0
		while i < len(todo) and len(todo) < limit:
			for n in self.nb8[todo[i]]:
				if not seen.has_key(n) and claims[n] == 0 and data[n] == PlotTypes.PLOT_OCEAN and self.rowOK(n):
					seen[n] = True
					todo.append(n)
			i += 1
		return len(todo)

	def placeIslands(self, spots, sizes, smallest, mode):
		"""Lonely islands in spots, each its own group, as big as sizes (at least smallest) where there is room. They
		grow a plot each in turn, so that neighbours share the water between them. Returns the plots placed."""
		growing = []
		placed = 0
		for i in range(len(spots)):
			g = self.newGroup()
			m = self.newIslandMass()
			if self.canAdd(spots[i], g, m):
				self.setLand(spots[i], m, g)
				placed += 1
				if max(smallest, sizes[i]) > 1:
					growing.append([m, g, max(smallest, sizes[i]) - 1])
		while len(growing) > 0:
			still = []
			for seed in growing:
				if self.growMass(seed[0], seed[1], 1, mode) > 0:
					placed += 1
					seed[2] -= 1
					if seed[2] > 0:
						still.append(seed)
			growing = still
		return placed

	def coastalIslands(self, g, share, sizeMin, sizeMax, maxDistance, mode, allowOneTile):
		"""Continent scattering: islands near continent g, about share of its land, 1 to maxDistance - 1 water
		plots off its coast. Returns the plots placed."""
		body = self.massTiles.get(g)
		if body is None or share <= 0 or maxDistance < 2:
			return 0
		budget = int(len(body) * share + 0.5)
		if budget <= 0:
			return 0
		if not allowOneTile and sizeMin < 2:
			sizeMin = 2
		if sizeMax < sizeMin:
			sizeMax = sizeMin
		dist = self.distanceMap(body.items, maxDistance)
		cands = []
		for o in xrange(self.N):
			if dist[o] >= 2 and dist[o] <= maxDistance and self.data[o] == PlotTypes.PLOT_OCEAN and self.rowOK(o):
				cands.append(o)
		placed = 0
		dice = self.dice
		while placed < budget and len(cands) > 0:
			i = dice.get(len(cands), "Python - Planet Generator: coastal island")
			o = cands[i]
			cands[i] = cands[-1]
			cands.pop()
			size = sizeMin + dice.get(sizeMax - sizeMin + 1, "Python - Planet Generator: coastal island size")
			if size > budget - placed:
				size = budget - placed
				if size < sizeMin:
					break
			placed += self.islandAt(o, size, g, mode)
		return placed

	def lonelySpots(self, radius):
		"""Centres for lonely islands: open water at least separation + 1 + radius plots from all land (the most
		open first), packed so that islands of that radius keep the separation between each other too."""
		W = self.W
		data = self.data
		sources = []
		for o in xrange(self.N):
			if data[o] != PlotTypes.PLOT_OCEAN:
				sources.append(o)
		need = self.sep + 1 + radius
		dist = self.distanceMap(sources, need + 50)
		dice = self.dice
		cands = []
		# centres at least 4 rows from the map's edges, as the original ocean search (no lonely islands in the ice)
		low = max(self.minY, 4) + radius
		high = min(self.maxY, self.H - 5) - radius
		for o in xrange(self.N):
			if dist[o] >= need:
				y = o // W
				if y >= low and y <= high:
					# most open first, give or take two plots (strictly by distance they came out as a grid)
					cands.append((-dist[o] - dice.get(3, "Python - Planet Generator: lonely island spot"), dice.get(10000, "Python - Planet Generator: lonely island spot"), o))
		cands.sort()
		reach = 2 * radius + self.sep # centres must be further apart than this
		blocked = [False] * self.N
		spots = []
		for c in cands:
			o = c[2]
			if blocked[o]:
				continue
			spots.append(o)
			x = o % W
			y = o // W
			y0 = y - reach
			if y0 < 0:
				y0 = 0
			y1 = y + reach
			if y1 > self.H - 1:
				y1 = self.H - 1
			for yy in range(y0, y1 + 1):
				row = yy * W
				for dx in range(-reach, reach + 1):
					blocked[row + (x + dx) % W] = True
		return spots

def landTargetPlots(iW, iH, oceanPercentage):
	"land plots for the chosen percentage of ocean (of the whole map, poles included)"
	return int(round(iW * iH * (100 - oceanPercentage) / 100.0))

def islandSizeRange(lonelyIslandSize):
	"(smallest, biggest, radius): the sizes lonely islands were always drawn from"
	if lonelyIslandSize == "small":
		return 2, 5, 1
	elif lonelyIslandSize == "medium":
		return 16, 51, 3
	return 20, 83, 4

def continentPlayerCounts(players, count, rivals, dice):
	"""Players per continent for "By player count" sizes, and the continent of the human player (-1: not
	planned). With rivals >= 0 that continent gets the human and that many rivals, the others share the rest."""
	if rivals < 0 or count < 2:
		contData = []
		for p in range(players):
			contData.append(1)
		while len(contData) > count:
			index = int(getRand(dice) * len(contData))
			if index == 0:
				index2 = len(contData) - 1
			else:
				index2 = index - 1
			contData[index2] += contData[index]
			contData.pop(index)
		return contData, -1
	k = rivals
	if k > players - 1:
		k = players - 1
	rest = players - 1 - k
	others = []
	for i in range(count - 1):
		others.append(rest // (count - 1))
	for i in range(rest % (count - 1)):
		others[i] += 1
	shuffled = []
	while len(others) > 0:
		shuffled.append(others.pop(dice.get(len(others), "Python - Planet Generator: continent players")))
	home = dice.get(count, "Python - Planet Generator: home continent")
	shuffled.insert(home, k + 1)
	return shuffled, home

def growContinents(contList, targets, steps, total):
	"""Grows the continents to their targets, each in turn (those meant for more players more per turn, as
	before). K-Mod-Magnus: a continent that cannot grow any more leaves its share to the others; one stuck
	continent used to stop all of them, which left far too little land."""
	n = len(contList)
	grown = [1] * n # the start plot
	stuck = [False] * n
	quota = list(targets)
	while True:
		progress = False
		for i in range(n):
			if stuck[i]:
				continue
			for k in range(steps[i]):
				if grown[i] >= quota[i]:
					break
				if contList[i].grow():
					grown[i] += 1
					progress = True
				else:
					stuck[i] = True
					break
		if not progress:
			missing = total - sum(grown)
			free = []
			for i in range(n):
				if not stuck[i]:
					free.append(i)
			if missing <= 0 or len(free) == 0:
				break
			for j in range(len(free)):
				quota[free[j]] += missing // len(free)
				if j < missing % len(free):
					quota[free[j]] += 1
	for i in range(n):
		contList[i].stuck = stuck[i]
	return grown

def balanceLand(plan, contList, steps, target, growMode, shrinkMode, useBuilders):
	"""K-Mod-Magnus: adds or removes land at the coasts of the continents until they have exactly target land plots
	(as far as there is room; the islands do not count). Continents grow their own way first (TileBuilder), then at
	their sides."""
	full = {}
	while plan.contLand < target:
		progress = False
		for i in range(len(contList)):
			continent = contList[i]
			m = continent.mass
			if full.has_key(m):
				continue
			for k in range(steps[i]):
				if plan.contLand >= target:
					break
				ok = False
				if useBuilders and not continent.stuck:
					ok = continent.grow()
					if not ok:
						continent.stuck = True
				if not ok:
					ok = plan.growMass(m, continent.group, 1, growMode) > 0
				if ok:
					progress = True
				else:
					full[m] = True
					break
		if not progress:
			break
	while plan.contLand > target:
		progress = False
		order = []
		for continent in contList:
			order.append((-len(plan.massTiles[continent.mass]), continent.mass))
		order.sort()
		for item in order:
			if plan.contLand <= target:
				break
			if plan.shrinkMass(item[1], 1, shrinkMode) > 0:
				progress = True
		if not progress:
			break

def generateRandomMap():
	"""Can be overridden if you want to manually control the sequence in which plot types, heights, and terrain are generated.

	WARNING: If you override this function, then none of the following functions will be automatically called for you:
		generateTerrain()
		generatePlotTypes()

	However, unless you have some specific reason to change	the	sequence of	map	generation, you should probably just override
	one or more of the functions listed above.
	"""
	#print "--------- Generate random map"
	CyPythonMgr().allowDefaultImpl()

def initReplacements():
	# 0 - nothing
	# 1 - water
	# 2 - land
	rep = []
	orig = [
		[0,2,0],
		[2,1,2],
		[0,2,0],
		]
	final = [
		[2],
		]
	rep.append([orig,final])
	orig = [
		[0,2,2,0],
		[2,1,1,2],
		[0,2,2,0],
		]
	final = [
		[2,2],
		]
	rep.append([orig,final])
	orig = [
		[0,2,0],
		[2,1,2],
		[2,1,2],
		[0,2,0],
		]
	final = [
		[2],
		[2],
		]
	rep.append([orig,final])
	# unify islands
	orig = [
		[1,1,1,1,1],
		[1,1,1,1,1],
		[1,2,1,2,1],
		[1,1,1,1,1],
		]
	final = [
	[
		[0,2,0],
		[2,0,2],
	],
	[
		[0,2,2],
		[2,0,2],
	],
	[
		[2,2,0],
		[2,0,2],
	],
	[
		[0,2,0],
		[2,2,2],
	],
		]
	rep.append([orig,final])
	orig = [
		[1,1,1,1,1],
		[1,2,1,2,1],
		[1,1,1,1,1],
		[1,1,1,1,1],
		]
	final = [
	[
		[2,0,2],
		[0,2,0],
	],
	[
		[2,0,2],
		[0,2,2],
	],
	[
		[2,0,2],
		[2,2,0],
	],
	[
		[2,2,2],
		[0,2,0],
	],
		]
	rep.append([orig,final])
	orig = [
		[1,1,1,1,1],
		[1,2,1,1,1],
		[1,1,1,2,1],
		[1,1,1,1,1],
		]
	final = [
	[
		[2,0,0],
		[0,2,2],
	],
	[
		[2,2,0],
		[0,0,2],
	],
		]
	rep.append([orig,final])
	orig = [
		[1,1,1,1,1],
		[1,1,1,2,1],
		[1,2,1,1,1],
		[1,1,1,1,1],
		]
	final = [
	[
		[0,2,2],
		[2,0,0],
	],
	[
		[0,0,2],
		[2,2,0],
	],
		]
	rep.append([orig,final])
	orig = [
		[1,1,1,1],
		[1,2,1,1],
		[1,1,1,1],
		[1,2,1,1],
		[1,1,1,1],
		]
	final = [
	[
		[2,0],
		[0,2],
		[2,0],
	],
	[
		[2,2],
		[0,2],
		[2,0],
	],
	[
		[2,2],
		[0,2],
		[2,2],
	],
	[
		[2,2],
		[2,2],
		[2,0],
	],
	[
		[2,0],
		[2,2],
		[2,2],
	],
		]
	rep.append([orig,final])
	orig = [
		[1,1,1,1],
		[1,1,2,1],
		[1,1,1,1],
		[1,1,2,1],
		[1,1,1,1],
		]
	final = [
	[
		[0,2],
		[2,0],
		[0,2],
	],
	[
		[2,2],
		[2,2],
		[0,2],
	],
	[
		[0,2],
		[2,2],
		[2,2],
	],
	[
		[0,2],
		[2,2],
		[2,2],
	],
	[
		[0,2],
		[2,0],
		[2,2],
	],
	[
		[2,2],
		[2,0],
		[0,2],
	],
		]
	rep.append([orig,final])
	orig = [
		[1,1,1,1],
		[1,2,1,1],
		[1,1,1,1],
		[1,1,2,1],
		[1,1,1,1],
		]
	final = [
	[
		[2,0],
		[2,0],
		[0,2],
	],
	[
		[2,0],
		[0,2],
		[0,2],
	],
	[
		[2,2],
		[0,2],
		[0,2],
	],
	[
		[2,2],
		[2,2],
		[0,2],
	],
	[
		[2,0],
		[2,2],
		[2,2],
	],
		]
	rep.append([orig,final])
	orig = [
		[1,1,1,1],
		[1,1,2,1],
		[1,1,1,1],
		[1,2,1,1],
		[1,1,1,1],
		]
	final = [
	[
		[0,2],
		[2,0],
		[2,0],
	],
	[
		[0,2],
		[0,2],
		[2,0],
	],
	[
		[0,2],
		[2,2],
		[2,0],
	],
	[
		[0,2],
		[2,2],
		[2,2],
	],
	[
		[2,2],
		[2,2],
		[2,0],
	],
		]
	rep.append([orig,final])
	return rep

def landR(i):
	if i == 0:
		return None
	elif i == 1:
		return PlotTypes.PLOT_OCEAN
	elif i == 2:
		return PlotTypes.PLOT_LAND

# called by Firaxis - expects to get array of plot types
def generatePlotTypes():
	collectGarbage()

	global generationStarted
	global selection_titles

	global world # our world array

	global continentCount
	global continentSeparation

	global allowLandNearIce
	global poleSeparation

	global homeContinent
	global homeRivals
	global startAreaPlan
	global continentPlayers
	global continentAreaIDs

	poleSeparation = 3

	cgc = CyGlobalContext()
	map = cgc.getMap()

	#if generationStarted:
	#	print "=============== receives latitudes and map size =============== "
	#else:
	#	print "=============== no lattude or map size change, regeneration =============== "

	#print "generate plot types"
	dice = cgc.getGame().getMapRand() #dice is Firaxis' random number generator
	players = cgc.getGame().countCivPlayersEverAlive()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	#print "----------------- generating world",
	oceanPercentage = getSelValue("ocean")
	if oceanPercentage is None or oceanPercentage > 100: #ensure we don't make something bad if wrong settings exist
		oceanPercentage = 70
	continentShape = getSelValue("continent_shape") #string value
	if continentShape is None: #if something bad with settings, jump to normal
		continentShape = "natural"
	continentSize = getSelValue("continent_size")
	if continentSize is None:
		continentSize = "players"
	continentScatter = getSelValue("continent_scattering")
	if continentScatter is None:
		continentScatter = [0,0,0,0]
	lonelyIslandSize = getSelValue("lonely_island_size")
	if lonelyIslandSize is None:
		lonelyIslandSize = "medium"
	lonelyIslands = getSelValue("lonely_islands")
	if lonelyIslands is None:
		lonelyIslands = "one"
	minIslandSize = getSelValue("min_island_size") #K-Mod-Magnus (was "One tile islands allowed": No = 2, Yes = 1)
	if minIslandSize is None:
		minIslandSize = 2
	allowOneTileIslands = minIslandSize <= 1
	allowLandNearIce = getSelValue("land_near_ice")
	if allowLandNearIce is None:
		allowLandNearIce = False
	coastline = getSelValue("coastline") #K-Mod-Magnus
	if coastline is None:
		coastline = 0
	homeRivals = getSelValue("home_rivals") #K-Mod-Magnus
	if homeRivals is None:
		homeRivals = -1
	if continentScatter[0] <= 7 and len(continentScatter) < 8: # settings from before K-Mod-Magnus
		continentScatter = list(continentScatter) + [0.0, 0, 0, 0]
	startAreaPlan = None
	continentAreaIDs = None
	# there is list of templates which are used for various continent shape types
	# there are three parameters in this array (separated by commas): [builder tiles, active tiles, grow count]

	#first type of natural shape:
	#this happens to be like snaky
	tbsSnakyNatural = [2, iW * iH / 8, 2] # iW * iH / 8 = MapWidth * MapHeight / 8 = active tiles

	#second type of natural shape:
	#more thick than first natural
	tbsTravelingNatural = [4, iW * iH / 10, 4] # iW * iH / 10 = MapWidth * MapHeight / 10 = active tiles

	#new2 - spreads equally to all directions, posibility of peninsulas and inland seas
	tbsNatural = [[7,6,6], [iW * iH / 12, iW * iH / 6], [4,2,4,2,2]]

	#new1 - spreads equaly to all directions, can huge peninsulas happen
	tbsSmooth = [15, iW * iH / 3, 4]

	#new3 - spreads equally to all directions, has huge landmass in the middle and many peninsulas, low probability of inside lakes
	sn = iW * iH / 3
	if sn < 40:
		sn = 40
		tbsCrabLike = [[20,4], sn, 2]
	else:
		tbsCrabLike = [[20,4], sn, 2]

	#snaky template:
	#when generating snaky continent, we must ensure active tile count is not too low, so we check for that
	sn = iW * iH / 60
	if sn < 40:
		sn = 40
	tbsSnaky = [3, sn, 2]

	#snaky-thin template:
	#this is extreemly thin
	sn = iW * iH / 50
	if sn < 40:
		sn = 40
	tbsSnakyThin = [2, sn, 1]

	# if random for each continent is chosen
	if continentShape == "continent-random":
		shapeRandom = True
		if iW < 64: # < standart
			if continentCount <= 2:
				randomList = [tbsSnaky,tbsSnakyNatural,tbsTravelingNatural]
			elif continentCount <= 5:
				randomList = [tbsSnaky,tbsSnakyNatural,tbsTravelingNatural,tbsNatural]
			else:
				randomList = [tbsSnaky,tbsSnakyNatural,tbsTravelingNatural,tbsNatural,tbsCrabLike]
		elif iW < 128: # < huge
			if continentCount <= 2:
				randomList = [tbsSnakyNatural,tbsTravelingNatural,tbsNatural,tbsCrabLike]
			elif continentCount <= 5:
				randomList = [tbsSnaky,tbsSnakyNatural,tbsTravelingNatural,tbsNatural,tbsCrabLike]
			else:
				randomList = [tbsSnaky,tbsSnakyNatural,tbsTravelingNatural,tbsCrabLike]
		else: # >= huge
			if continentCount == 1:
				randomList = [tbsTravelingNatural,tbsSmooth,tbsNatural,tbsCrabLike]
			elif continentCount == 2:
				randomList = [tbsTravelingNatural,tbsSmooth,tbsNatural,tbsCrabLike]
			elif continentCount <= 5:
				randomList = [tbsSnakyNatural,tbsTravelingNatural,tbsNatural,tbsCrabLike]
			else:
				randomList = [tbsSnaky,tbsSnakyNatural,tbsTravelingNatural,tbsNatural,tbsCrabLike]
	else: #otherwise we select template
		shapeRandom = False
		if continentShape == "snaky-natural":
			shapeData = tbsSnakyNatural
		elif continentShape == "traveling-natural":
			shapeData = tbsTravelingNatural
		elif continentShape == "unpredictable":
			# if unpredictable, we generate random shifting template, different for every continent
			shapeRandom = True
			randomList = []
			for i in range(continentCount):
				randomList.append([[2 + getRand(dice) * 3, 2 + getRand(dice) * 3, 2 + getRand(dice) * 3], [iW * iH / (5 + getRand(dice) * 10), iW * iH / (5 + getRand(dice) * 10), iW * iH / (5 + getRand(dice) * 5)], [2 + getRand(dice) * 2, 2 + getRand(dice) * 2]])
		elif continentShape == "snaky":
			shapeData = tbsSnaky
		elif continentShape == "snaky-thin":
			shapeData = tbsSnakyThin
		elif continentShape == "smooth":
			shapeData = tbsSmooth
		elif continentShape == "natural":
			shapeData = tbsNatural
		elif continentShape == "crab":
			shapeData = tbsCrabLike

	#calculate actual land tiles we must produce
	# K-Mod-Magnus: the continents cover exactly what the chosen percentage of ocean leaves of the whole map; the
	# islands come on top, as if they were ocean. The land used to be lowered for the separation and by another 5%,
	# while land bridges and filled lakes came on top of it.
	targetLand = landTargetPlots(iW, iH, oceanPercentage)
	print "Planet Generator: continent land plots wanted:", targetLand, "of", iW * iH

	#world is two-dimensional array which wraps in X automagically, so you may not worry about edges
	world = worldArray([iW,iH],PlotTypes.PLOT_OCEAN)
	plan = LandPlan(world, continentSeparation, allowLandNearIce, dice)

	# lonely islands: the number asked for, or a share of all the empty spots
	islandMin, islandMax, islandRadius = islandSizeRange(lonelyIslandSize)
	lonelyNumber = 0
	lonelyShare = 0.0
	if isinstance(lonelyIslands, str):
		if lonelyIslands.startswith("lonely"):
			lonelyNumber = int(lonelyIslands[6:])
	elif lonelyIslands > 0:
		lonelyNumber = -1 # a share of all the empty spots
		lonelyShare = float(lonelyIslands)

	archipelago = continentScatter[0] > 7
	coastShare = 0.0
	if not archipelago and len(continentScatter) >= 8:
		coastShare = continentScatter[4]
	# the islands do not count (they come on top), so all of the land is for the continents
	bodyTotal = targetLand
	if bodyTotal < continentCount:
		bodyTotal = continentCount

	# players per continent ("By player count" sizes) and the continent the human player is to start on
	homeContinent = -1
	contData = None
	if continentSize == "players" and continentCount > 1 and (players > continentCount or homeRivals >= 0):
		contData, homeContinent = continentPlayerCounts(players, continentCount, homeRivals, dice)
	continentPlayers = contData
	weights = []
	for i in range(continentCount):
		if contData is not None:
			w = float(contData[i])
			if w < 0.5:
				w = 0.5 # a continent nobody starts on still gets some land
		elif continentSize == "unpredictable":
			w = 0.4 + getRand(dice) * 1.2
		else:
			w = 1.0
		weights.append(w)
	weightSum = 0.0
	for w in weights:
		weightSum += w
	minWeight = min(weights)

	#now we must somehow distribute start positions for each continent, depending on number of players.
	#actual player distribution is handled by Firaxis code (or findStartingArea below), which looks how big each continent is.
	contPositions = getStartPositions(world, contData)

	contList = [] #contList will contain TileBuilders for each continent; continent i is group i and landmass i of the plan
	targets = []
	steps = []
	for i in range(continentCount):
		if shapeRandom: #if we have random shape for each continent, we get it now
			index = int(getRand(dice) * len(randomList)) #random index
			shapeData = randomList[index]
		group = plan.newGroup()
		target = int(bodyTotal * weights[i] / weightSum)
		if target < 1:
			target = 1
		#initialize tile builder
		continent = TileBuilder(world, PlotTypes.PLOT_LAND, shapeData[0], shapeData[1], shapeData[2], continentSeparation, target)
		if archipelago:
			continent.coastScatterMin = continentScatter[0]
			continent.coastScatterMax = continentScatter[1]
			continent.coastScatterChangeChance = continentScatter[2]
			continent.coastScatterChangeDif = continentScatter[3]
		else:
			# the continent grows in one piece; the islands around it come afterwards
			continent.coastScatterMin = 0
			continent.coastScatterMax = 0
			continent.coastScatterChangeChance = 0
			continent.coastScatterChangeDif = 0
		continent.allowReachPoles = allowLandNearIce
		continent.continentPartControlEnabled = False
		continent.usePlan(plan, group, group)
		continent.contiguous = not archipelago
		continent.stuck = False
		#place first tile (x, y)
		startX, startY = contPositions[i]
		seed = plan.findSeed(world.getOffset([startX, startY]), group, group)
		if seed < 0:
			print "Planet Generator: no room for continent", i
			continue
		continent.start(seed % iW, seed // iW)
		contList.append(continent)
		targets.append(target)
		steps.append(int(weights[i] / minWeight + 0.5))

	#now what is left - growing :)
	growContinents(contList, targets, steps, bodyTotal)
	size = plan.land
	print "Planet Generator: continents", size, "plots (wanted", bodyTotal, ")"
	if debugChecks and not archipelago:
		plan.check("growth", len(contList))

	# now we want to remove one-square and two-square lakes. more templates (which describe how to remove tiles) can be added in initReplacements function,
	#  and code below should handle them just fine. i am not adding comments to it because i forgot how it works :D
	# K-Mod-Magnus: only where all the land around belongs to one continent, and only where that continent may grow
	rep = initReplacements()

	for x in range(iW):
		for y in range(iH):
			# handle replacements
			for r in rep:
				orig, final_v = r
				h = len(orig)
				w = len(orig[0])
				dh = (h - 1) / 2
				dw = (w - 1) / 2
				Tinka = True
				for ax in range(w):
					for ay in range(h):
						cell = orig[ay][ax]
						if landR(cell) != None:
							if landR(cell) != world[x-dw+ax,y-dh+ay]:
								Tinka = False
								break
				if Tinka:
					group = -1
					mass = -1
					for ax in range(w):
						for ay in range(h):
							if orig[ay][ax] == 2:
								o = world.getOffset([x-dw+ax,y-dh+ay])
								if group == -1:
									group = plan.owner[o]
									mass = plan.mass[o]
								elif plan.owner[o] != group:
									Tinka = False
					if group < 0:
						Tinka = False
				if Tinka:
					if isinstance(final_v[0][0],list): # if array is deeper than coordinates
						index = int(getRand(dice) * len(final_v))
						final = final_v[index]
					else:
						final = final_v
					h = len(final)
					w = len(final[0])
					dh = (h - 1) / 2
					dw = (w - 1) / 2
					for ax in range(w):
						for ay in range(h):
							cell = final[ay][ax]
							if landR(cell) == PlotTypes.PLOT_LAND:
								o = world.getOffset([x-dw+ax,y-dh+ay])
								if plan.isWater(o) and plan.rowOK(o) and not (plan.claims[o] & ~(1 << group)):
									plan.setLand(o, mass, group)

	if debugChecks and not archipelago:
		plan.check("lakes", len(contList))
	# K-Mod-Magnus: now exactly the land wanted on the continents (the islands come later and do not count). Pieces
	# of archipelago continents below the minimum island size go, and the rest grows at its sides to make up for them.
	balanceLand(plan, contList, steps, targetLand, "random", "compact", True)
	if minIslandSize > 1 and plan.removeSmallPieces(minIslandSize) > 0:
		balanceLand(plan, contList, steps, targetLand, "random", "compact", False)
	print "Planet Generator: continents balanced to", plan.contLand, "plots"
	if debugChecks and not archipelago:
		plan.check("balance", len(contList))
	# Coastline (K-Mod-Magnus): reshape the continents for more coast
	if coastline > 0 and not archipelago:
		iterations = [0, 0.5, 2, 10][coastline] # moves tried, per plot of the continent
		reach = [0, 1, 3, 0][coastline] # how far from the coast as grown it may change (0: anywhere)
		before = plan.coastCount()
		for continent in contList:
			plan.roughen(continent.mass, continent.group, int(iterations * len(plan.massTiles[continent.mass])), reach)
		print "Planet Generator: coastline", coastline, "- coast plots", before, "->", plan.coastCount()
	if coastline >= 2:
		islandMode = "coast"
	else:
		islandMode = "organic"

	if debugChecks and not archipelago:
		plan.check("coastline", len(contList))
	# islands stay smaller than continents: at most half the smallest continent (or the minimum island size)
	smallestBody = 0
	for continent in contList:
		n = len(plan.massTiles[continent.mass])
		if smallestBody == 0 or n < smallestBody:
			smallestBody = n
	islandCap = max(minIslandSize, smallestBody // 2)

	# lonely islands (K-Mod-Magnus: found with a distance map, so every empty part of the ocean counts; they keep the
	# separation from all other land and are as big as chosen, as they do not count toward the percentage of ocean.
	# They come before the islands near the coasts, which would take the open sea they need.)
	if lonelyNumber != 0:
		smallest = max(1, minIslandSize)
		# the islands of the chosen size, in the most open spots (or a share of them, or the number asked for)
		spots = plan.lonelySpots(islandRadius)
		if lonelyNumber > 0:
			spots = spots[:lonelyNumber]
		elif lonelyShare < 1.0:
			want = max(1, int(len(spots) * lonelyShare + 0.5)) # at least one, if there is room for it
			chosen = []
			while len(chosen) < want and len(spots) > 0:
				chosen.append(spots.pop(dice.get(len(spots), "Python - Planet Generator: lonely island")))
			spots = chosen
		sizes = []
		for o in spots:
			sizes.append(min(islandCap, max(smallest, islandMin + dice.get(islandMax - islandMin + 1, "Python - Planet Generator: lonely island size"))))
		placed = plan.placeIslands(spots, sizes, smallest, islandMode)
		count = len(spots)
		# too few open spots for the number asked for: smaller gaps, where an island of the chosen size still fits
		need = min(islandCap, max(smallest, islandMin))
		r = islandRadius - 1
		while count < lonelyNumber and r >= 0:
			spots = []
			for o in plan.lonelySpots(r):
				if count + len(spots) >= lonelyNumber:
					break
				if plan.room(o, need) >= need:
					spots.append(o)
			sizes = []
			for o in spots:
				sizes.append(min(islandCap, max(smallest, islandMin + dice.get(islandMax - islandMin + 1, "Python - Planet Generator: lonely island size"))))
			placed += plan.placeIslands(spots, sizes, smallest, islandMode)
			count += len(spots)
			r -= 1
		if lonelyShare >= 1.0:
			# "every empty area": smaller islands in the gaps that are left, the biggest gaps first
			r = islandRadius - 1
			while r >= 1:
				spots = plan.lonelySpots(r)
				size = min(islandCap, max(smallest, (2 * r + 1) * (2 * r + 1) * 2 // 3))
				sizes = []
				for o in spots:
					sizes.append(size)
				placed += plan.placeIslands(spots, sizes, smallest, islandMode)
				count += len(spots)
				r -= 1
		print "Planet Generator: lonely islands", count, "with", placed, "plots"

	if debugChecks and not archipelago:
		plan.check("lonely", len(contList))
	# continent scattering (K-Mod-Magnus): islands near each continent's coast
	if coastShare > 0:
		for continent in contList:
			plan.coastalIslands(continent.group, coastShare, max(continentScatter[5], minIslandSize), max(minIslandSize, min(continentScatter[6], islandCap)), continentScatter[7], islandMode, allowOneTileIslands)

	if debugChecks and not archipelago:
		plan.check("scattering", len(contList))
	# islands below the minimum size grow to it, or go if there is no room (K-Mod-Magnus; was: remove 1 - tile islands)
	if minIslandSize > 1:
		plan.fitIslands(minIslandSize, islandMode)
		plan.removeSmallPieces(minIslandSize)

	if debugChecks and not archipelago:
		plan.check("islands", len(contList))
	global landPlan
	global landSettings
	landPlan = plan
	landSettings = {"ocean": oceanPercentage, "separation": continentSeparation, "continents": len(contList),
		"minIslandSize": minIslandSize, "archipelago": archipelago, "coastline": coastline, "targetLand": targetLand,
		"continentLand": plan.contLand, "islandLand": plan.land - plan.contLand}

	global continentAnchors
	continentAnchors = []
	for i in range(continentCount):
		anchor = -1
		if plan.massTiles.has_key(i) and len(plan.massTiles[i]) > 0:
			anchor = plan.massTiles[i].items[0]
		continentAnchors.append(anchor)
	islands = 0
	for m in plan.massTiles.keys():
		if m >= 1000 and len(plan.massTiles[m]) > 0:
			islands += 1
	# (no percent sign in the text: the game's log runs printed text through printf)
	print "Planet Generator: %d continents with %d land plots (wanted %d: %.1f percent water without the islands), %d islands with %d plots, %.1f percent water, %d coast plots" % (len(contList), plan.contLand, targetLand, 100.0 * (iW * iH - plan.contLand) / (iW * iH), islands, plan.land - plan.contLand, 100.0 * (iW * iH - plan.land) / (iW * iH), plan.coastCount())

	peakOptions = getSelValue("peaks")
	if peakOptions is None:
		peakOptions = [0,0,0]

	peakStartProbability = peakOptions[0]
	peakGrowProbability = peakOptions[1]
	maxPeaksInRow = peakOptions[2]

	hillOptions = getSelValue("hills")
	if hillOptions is None:
		hillOptions = [0,0,0]

	hillStartProbability = hillOptions[0]
	hillGrowProbability = hillOptions[1]
	maxHillsInRow = hillOptions[2]

	#peak storage:
	peakArray = bArray()

	#hill storage:
	hillArray = bArray()

	for y in range(iH):
		for x in range(iW):
			offset = world.getOffset([x,y])
			if peakStartProbability > 0:
				#collect peaks
				#check if land area is 9x9 empty
				if world[x,y] == PlotTypes.PLOT_LAND and world[x-1,y] == PlotTypes.PLOT_LAND and world[x+1,y] == PlotTypes.PLOT_LAND and world[x,y-1] == PlotTypes.PLOT_LAND and world[x,y+1] == PlotTypes.PLOT_LAND and (not world[x+1,y+1] == PlotTypes.PLOT_PEAK) and (not world[x-1,y+1] == PlotTypes.PLOT_PEAK) and (not world[x-1,y-1] == PlotTypes.PLOT_PEAK) and (not world[x+1,y-1] == PlotTypes.PLOT_PEAK):
					if getRand(dice) < peakStartProbability:
						world._data[offset] = PlotTypes.PLOT_PEAK
						if peakGrowProbability > 0: #if we want peeks to grow
							peakArray[offset] = [x,y,1]
			if hillStartProbability > 0:
				#collect hills
				#check if land area is 11x11 empty
				if world[x,y] == PlotTypes.PLOT_LAND and not hillIntersect(world,x,y,x,y,1):
					if getRand(dice) < hillStartProbability:
						world._data[offset] = PlotTypes.PLOT_HILLS
						if hillGrowProbability > 0: #if we want peeks to grow
							hillArray[offset] = [x,y,1]

	if peakStartProbability > 0: #if we want peeks
		# now grow peaks. this code is actually using simplified method of tileBuilder. BTW, i tried to use tileBuilder to grow
		#  peaks and hills and that worked fine, but in this way it is much more optimized and without unecessary things
		while len(peakArray) > 0:
			x, y, count = peakArray.random(dice) #get peak and remove it from array

			if world[x,y] != PlotTypes.PLOT_OCEAN and count < maxPeaksInRow: # if we are in limit
				offset1 = world.getOffset([x,y])

				#print "++ Grow peak ++"
				#print "offset: %d"%(offset1)

				# for every peak we find all possible branches
				possibleTiles = bArray() # store them here
				c = 0
				wtt = world[x-1,y]
				if not wtt == PlotTypes.PLOT_PEAK and not wtt == PlotTypes.PLOT_OCEAN and not peakIntersect(world,x-1,y,x,y):
					possibleTiles[world.getOffset([x-1,y])] = [x-1,y]
					c += 1
				wtt = world[x-1,y-1]
				if not wtt == PlotTypes.PLOT_PEAK and not wtt == PlotTypes.PLOT_OCEAN and not peakIntersect(world,x-1,y-1,x,y):
					possibleTiles[world.getOffset([x-1,y-1])] = [x-1,y-1]
					c += 1
				wtt = world[x,y-1]
				if not wtt == PlotTypes.PLOT_PEAK and not wtt == PlotTypes.PLOT_OCEAN and not peakIntersect(world,x,y-1,x,y):
					possibleTiles[world.getOffset([x,y-1])] = [x,y-1]
					c += 1
				wtt = world[x+1,y-1]
				if not wtt == PlotTypes.PLOT_PEAK and not wtt == PlotTypes.PLOT_OCEAN and not peakIntersect(world,x+1,y-1,x,y):
					possibleTiles[world.getOffset([x+1,y-1])] = [x+1,y-1]
					c += 1
				wtt = world[x+1,y]
				if not wtt == PlotTypes.PLOT_PEAK and not wtt == PlotTypes.PLOT_OCEAN and not peakIntersect(world,x+1,y,x,y):
					possibleTiles[world.getOffset([x+1,y])] = [x+1,y]
					c += 1
				wtt = world[x+1,y+1]
				if not wtt == PlotTypes.PLOT_PEAK and not wtt == PlotTypes.PLOT_OCEAN and not peakIntersect(world,x+1,y+1,x,y):
					possibleTiles[world.getOffset([x+1,y+1])] = [x+1,y+1]
					c += 1
				wtt = world[x,y+1]
				if not wtt == PlotTypes.PLOT_PEAK and not wtt == PlotTypes.PLOT_OCEAN and not peakIntersect(world,x,y+1,x,y):
					possibleTiles[world.getOffset([x,y+1])] = [x,y+1]
					c += 1
				wtt = world[x-1,y+1]
				if not wtt == PlotTypes.PLOT_PEAK and not wtt == PlotTypes.PLOT_OCEAN and not peakIntersect(world,x-1,y+1,x,y):
					possibleTiles[world.getOffset([x-1,y+1])] = [x-1,y+1]
					c += 1

				tile = possibleTiles.random(dice) #take one tile from possible tiles

				if tile is not None and getRand(dice) < peakGrowProbability:
					ax, ay = tile
					world[ax,ay] = PlotTypes.PLOT_PEAK # add it
					peakArray[world.getOffset([ax,ay])] = [ax,ay,count + 1] # add it to peak array to make it continue group grow

	#now grow hills, the same thing as peaks, only this time we check for peaks when growing which obviosly already exist :)
	if hillStartProbability > 0: #if we want peeks
		while len(hillArray) > 0:
			x, y, count = hillArray.random(dice) #get peak and remove it from array
			if world[x,y] != PlotTypes.PLOT_OCEAN and count < maxHillsInRow: # if we are in limit
				offset1 = world.getOffset([x,y])
				# for every hill we find all possible branches
				possibleTiles = bArray() # store them here
				c = 0
				separation = 1
				wtt = world[x-1,y]
				if wtt == PlotTypes.PLOT_LAND and not hillIntersect(world,x-1,y,x,y,separation):
					possibleTiles[world.getOffset([x-1,y])] = [x-1,y]
					c += 1
				wtt = world[x-1,y-1]
				if wtt == PlotTypes.PLOT_LAND and not hillIntersect(world,x-1,y-1,x,y,separation):
					possibleTiles[world.getOffset([x-1,y-1])] = [x-1,y-1]
					c += 1
				wtt = world[x,y-1]
				if wtt == PlotTypes.PLOT_LAND and not hillIntersect(world,x,y-1,x,y,separation):
					possibleTiles[world.getOffset([x,y-1])] = [x,y-1]
					c += 1
				wtt = world[x+1,y-1]
				if wtt == PlotTypes.PLOT_LAND and not hillIntersect(world,x+1,y-1,x,y,separation):
					possibleTiles[world.getOffset([x+1,y-1])] = [x+1,y-1]
					c += 1
				wtt = world[x+1,y]
				if wtt == PlotTypes.PLOT_LAND and not hillIntersect(world,x+1,y,x,y,separation):
					possibleTiles[world.getOffset([x+1,y])] = [x+1,y]
					c += 1
				wtt = world[x+1,y+1]
				if wtt == PlotTypes.PLOT_LAND and not hillIntersect(world,x+1,y+1,x,y,separation):
					possibleTiles[world.getOffset([x+1,y+1])] = [x+1,y+1]
					c += 1
				wtt = world[x,y+1]
				if wtt == PlotTypes.PLOT_LAND and not hillIntersect(world,x,y+1,x,y,separation):
					possibleTiles[world.getOffset([x,y+1])] = [x,y+1]
					c += 1
				wtt = world[x-1,y+1]
				if wtt == PlotTypes.PLOT_LAND and not hillIntersect(world,x-1,y+1,x,y,separation):
					possibleTiles[world.getOffset([x-1,y+1])] = [x-1,y+1]
					c += 1

				tile = possibleTiles.random(dice) #take one tile from possible tiles

				if tile is not None and getRand(dice) < hillGrowProbability:
					ax, ay = tile
					world[ax,ay] = PlotTypes.PLOT_HILLS # add it
					hillArray[world.getOffset([ax,ay])] = [ax,ay,count + 1] # add it to peak array to make it continue group grow

	# return our world in one-dimensional array to Firaxis to do the rest :)
	return world.get()

def getStartPositions(world, contData):
	global continentCount
	global continentSeparation

	cgc = CyGlobalContext()
	map = cgc.getMap()

	distribution = getContinentDistribution()

	dice = cgc.getGame().getMapRand() #dice is Firaxis' random number generator
	players = cgc.getGame().countCivPlayersEverAlive()
	iW = map.getGridWidth()
	iH = map.getGridHeight()

	if contData == None:
		contData = []
		for p in range(continentCount):
			contData.append(1) # create dummy array

	positionArray = []

	if distribution == "equator": #old style distribution
		p = -1
		part = float(iW) / float(continentCount * 2) #figure out x position of continent start tile
		for i in range(len(contData)):
			p += 2
			positionArray.append([int(part * p), iH / 2])
	elif distribution == "2rows":
		p = -1
		direction = 1
		part = float(iW) / float(continentCount * 2) #figure out x position of continent start tile
		for i in range(len(contData)):
			p += 2
			positionArray.append([int(part * p), int(iH / 2 * (1 + direction * (0.2 + getRand(dice) * 0.3)))])
			direction *= -1
	elif distribution == "3rows":
		p = -1
		offset = 0
		offsetchange = 1
		part = float(iW) / float(continentCount * 2) #figure out x position of continent start tile
		for i in range(len(contData)):
			p += 2
			positionArray.append([int(part * p), int(iH / 2 * (1 + offset * (0.4 + getRand(dice) * 0.2)))])
			offset += offsetchange
			if offset > 0 or offset < 0:
				offsetchange *= -1
	elif distribution == "center":
		perimeter = (continentSeparation + 5) * continentCount
		radius = perimeter / (2 * 3.141592)
		inc_angle = (360 / continentCount) * 3.1415 / 180
		angle = 0
		height_multiplier = 0.9
		width_multiplier = 1.5
		for i in range(continentCount):
			if angle >= 360 * 3.1415 / 180:
				angle -= 360 * 3.1415 / 180
			if angle == 0:
				positionArray.append([ int(iW / 2 - radius * width_multiplier), int(iH / 2) ])
			elif angle > 0 and angle < 3.1415 / 2:
				positionArray.append([ int(iW / 2 - radius * width_multiplier * math.cos(angle)), int(iH / 2 - radius * height_multiplier * math.sin(angle)) ])
			elif angle >= 3.1415 / 2 and angle < 3.1415:
				positionArray.append([ int(iW / 2 + radius * width_multiplier * math.sin(angle - 3.1415 / 2)), int(iH / 2 - radius * height_multiplier * math.cos(angle - 3.1415 / 2)) ])
			elif angle >= 3.1415 and angle < 3.1415 / 2 * 3:
				positionArray.append([ int(iW / 2 + radius * width_multiplier * math.cos(angle - 3.1415)), int(iH / 2 + radius * height_multiplier * math.sin(angle - 3.1415)) ])
			else:
				positionArray.append([ int(iW / 2 - radius * width_multiplier * math.sin(angle - 3.1415 / 2 * 3)), int(iH / 2 + radius * height_multiplier * math.cos(angle - 3.1415 / 2 * 3)) ])

			angle += inc_angle

	return positionArray

def getContinentDistribution():
	global continentCount
	global continentSeparation

	global chosenWorldType
	global worldTypes

	distributionOption = getSelValue("continent_distribution")
	if distributionOption == None:
		distributionOption = "smart-random"

	cgc = CyGlobalContext()
	map = cgc.getMap()

	if chosenWorldType == -1:
		worldInfo = worldTypes[0]
	else:
		worldInfo = worldTypes[chosenWorldType]

	mapArea = worldInfo.getGridHeight() * worldInfo.getGridWidth() * 16

	if distributionOption == "smart-random":
		if mapArea <= 40 * 24: #duel
			if continentSeparation < 4:
				if continentCount < 15:
					distribution = "2rows"
				else:
					distribution = "3rows"
			else:
				if continentCount < 12:
					distribution = "2rows"
				else:
					distribution = "3rows"
		elif mapArea <= 48 * 28: #tiny
			if continentSeparation < 4:
				if continentCount < 13:
					distribution = "2rows"
				else:
					distribution = "3rows"
			else:
				if continentCount < 12:
					distribution = "2rows"
				else:
					distribution = "3rows"
		elif mapArea <= 52 * 32: #small
			if continentSeparation < 4:
				if continentCount < 14:
					distribution = "2rows"
				else:
					distribution = "3rows"
			else:
				if continentCount < 13:
					distribution = "2rows"
				else:
					distribution = "3rows"
		elif mapArea <= 64 * 40: #standard
			if continentSeparation < 4:
				if continentCount < 15:
					distribution = "2rows"
				else:
					distribution = "3rows"
			else:
				if continentCount < 14:
					distribution = "2rows"
				else:
					distribution = "3rows"
		elif mapArea <= 84 * 52: #large
			if continentSeparation < 4:
				if continentCount < 16:
					distribution = "2rows"
				else:
					distribution = "3rows"
			else:
				if continentCount < 15:
					distribution = "2rows"
				else:
					distribution = "3rows"
		elif mapArea <= 128 * 80: #huge
			if continentSeparation < 4:
				if continentCount < 16:
					distribution = "2rows"
				else:
					distribution = "3rows"
			else:
				if continentCount < 3:
					distribution = "center"
				elif continentCount < 15:
					distribution = "2rows"
				else:
					distribution = "3rows"
		else: #else
			if continentSeparation < 4:
				if continentCount < 17:
					distribution = "2rows"
				else:
					distribution = "3rows"
			else:
				if continentCount < 15:
					distribution = "2rows"
				else:
					distribution = "3rows"
	else:
		distribution = distributionOption

	return distribution

#function to determine if tile x,y intersects with something, ignoring ox,oy tile
def peakIntersect(world, x, y, ox, oy):
	ax = x
	ay = y + 1
	if ax != ox and ay != oy:
		offset = world.getOffset([ax, ay])
		if world._data[offset] == PlotTypes.PLOT_PEAK or world._data[offset] == PlotTypes.PLOT_OCEAN:
			return True
	ax = x + 1
	ay = y
	if ax != ox and ay != oy:
		offset = world.getOffset([ax, ay])
		if world._data[offset] == PlotTypes.PLOT_PEAK or world._data[offset] == PlotTypes.PLOT_OCEAN:
			return True
	ax = x
	ay = y - 1
	if ax != ox and ay != oy:
		offset = world.getOffset([ax, ay])
		if world._data[offset] == PlotTypes.PLOT_PEAK or world._data[offset] == PlotTypes.PLOT_OCEAN:
			return True
	ax = x - 1
	ay = y
	if ax != ox and ay != oy:
		offset = world.getOffset([ax, ay])
		if world._data[offset] == PlotTypes.PLOT_PEAK or world._data[offset] == PlotTypes.PLOT_OCEAN:
			return True

	ax = x - 1
	ay = y - 1
	if ax != ox and ay != oy:
		offset = world.getOffset([ax, ay])
		if world._data[offset] == PlotTypes.PLOT_PEAK:
			return True
	ax = x - 1
	ay = y + 1
	if ax != ox and ay != oy:
		offset = world.getOffset([ax, ay])
		if world._data[offset] == PlotTypes.PLOT_PEAK:
			return True
	ax = x + 1
	ay = y + 1
	if ax != ox and ay != oy:
		offset = world.getOffset([ax, ay])
		if world._data[offset] == PlotTypes.PLOT_PEAK:
			return True
	ax = x + 1
	ay = y - 1
	if ax != ox and ay != oy:
		offset = world.getOffset([ax, ay])
		if world._data[offset] == PlotTypes.PLOT_PEAK:
			return True

	return False

#function to determine if tile x,y intersects with something in distance <separation>, ignoring ox,oy tile
def hillIntersect(world, x, y, ox, oy, separation):
	ax = x - 1
	ay = y
	if ax != ox and ay != oy:
		offset = world.getOffset([ax, ay])
		if world[ax, ay] == PlotTypes.PLOT_HILLS:
			return True
	ax = x + 1
	ay = y
	if ax != ox and ay != oy:
		offset = world.getOffset([ax, ay])
		if world[ax, ay] == PlotTypes.PLOT_HILLS:
			return True
	ax = x
	ay = y - 1
	if ax != ox and ay != oy:
		offset = world.getOffset([ax, ay])
		if world[ax, ay] == PlotTypes.PLOT_HILLS:
			return True
	ax = x
	ay = y + 1
	if ax != ox and ay != oy:
		offset = world.getOffset([ax, ay])
		if world[ax, ay] == PlotTypes.PLOT_HILLS:
			return True
	return False

#------------------------------------------------------
"""GAME	ELEMENT	PLACEMENT FUNCTIONS"""
#------------------------------------------------------

def beforeInit():
	"Called before the map is initialized"

def beforeGeneration():
	"Called before the map generation starts"

# default implementation
def addRivers():
	"Should add all desired rivers to the map."
	CyPythonMgr().allowDefaultImpl()

# default implementation
def getRiverStartCardinalDirection(argsList):
	pPlot = argsList[0]
	"Returns the cardinal direction	of the first river segment."
	CyPythonMgr().allowDefaultImpl()

# default implementation
def getRiverAltitude(argsList):
	pPlot = argsList[0]
	"Returns the altitude level of pPlot (rivers will flow to the lowest point)."
	CyPythonMgr().allowDefaultImpl()

# default implementation
def addLakes():
	"Can override to control where lakes are added on the map"
	CyPythonMgr().allowDefaultImpl()

#Step 4: add features to your terrain
#this is responsible for things like oasis, ice floes on the ocean, floodplains
def addFeatures():
	collectGarbage()

	global world

	global climateVariation
	global climateTemperature

	global climateHumidity

	global allowLandNearIce
	global poleSeparation

	climateHumidity = getSelValue("humidity")
	if climateHumidity is None:
		climateHumidity = 0.5
	floodPlains = getSelValue("flood_plains")
	if floodPlains is None:
		floodPlains = -1

	iceOnWater = 0.5

	global continentSeparation

	cgc = CyGlobalContext()

	terrainDesert = cgc.getInfoTypeForString("TERRAIN_DESERT")
	terrainPlains = cgc.getInfoTypeForString("TERRAIN_PLAINS")
	terrainGrass = cgc.getInfoTypeForString("TERRAIN_GRASS")
	terrainTundra = cgc.getInfoTypeForString("TERRAIN_TUNDRA")
	terrainSnow = cgc.getInfoTypeForString("TERRAIN_SNOW")

	featNone = cgc.getInfoTypeForString("NO_FEATURE")
	featIce = cgc.getInfoTypeForString("FEATURE_ICE")
	featJungle = cgc.getInfoTypeForString("FEATURE_JUNGLE")
	featOasis = cgc.getInfoTypeForString("FEATURE_OASIS")
	featFlood = cgc.getInfoTypeForString("FEATURE_FLOOD_PLAINS")
	featForest = cgc.getInfoTypeForString("FEATURE_FOREST")

	map = cgc.getMap()
	dice = cgc.getGame().getMapRand()
	players = cgc.getGame().countCivPlayersEverAlive()
	iW = map.getGridWidth()
	iH = map.getGridHeight()

	# Terrain Type generation
	seedList = bArray()
	for y in range(iH):
		randomTerrain = probabilityArray()

		tileTemperature = getTileTemperature(y,iH)
		#print "tile temperature",tileTemperature
		if tileTemperature > 30:
			randomTerrain[7 * (1.5 - climateHumidity)] = terrainDesert
		if tileTemperature > 15 and tileTemperature < 39:
			randomTerrain[7] = terrainPlains
		if tileTemperature > 4 and tileTemperature < 30:
			randomTerrain[7 * (climateHumidity + 0.5)] = terrainGrass
		if tileTemperature > -2 and tileTemperature < 25:
			randomTerrain[7] = terrainPlains
		if tileTemperature > -10 and tileTemperature < 10:
			randomTerrain[7] = terrainTundra
		if tileTemperature < -30:
			randomTerrain[15 * (climateHumidity / 2 + 0.75)] = terrainSnow
		elif tileTemperature < 0:
			randomTerrain[7 * (climateHumidity / 2 + 0.75)] = terrainSnow

		for x in range(iW):
			plot = map.sPlot(x,y)
			if plot.isWater():
				if allowLandNearIce:
					if y < poleSeparation:
						prob = (poleSeparation - y) / float(poleSeparation)
						if getRand(dice) < prob:
							plot.setFeatureType(featIce,-1)
					elif y > iH - poleSeparation - 1:
						prob = (poleSeparation - (iH - y - 1)) / float(poleSeparation)
						if getRand(dice) < prob:
							plot.setFeatureType(featIce,-1)
				else:
					if y < poleSeparation and not plot.isAdjacentToLand():
						prob = (poleSeparation - y) / float(poleSeparation)
						if getRand(dice) < prob:
							plot.setFeatureType(featIce,-1)
					elif y > iH - poleSeparation - 1 and not plot.isAdjacentToLand():
						prob = (poleSeparation - (iH - y - 1)) / float(poleSeparation)
						if getRand(dice) < prob:
							plot.setFeatureType(featIce,-1)

				if plot.isAdjacentToLand() and y > poleSeparation and y < iH - poleSeparation - 1:
					if tileTemperature < -40:
						if getRand(dice) < iceOnWater * 2:
							plot.setFeatureType(featIce,-1)
					elif tileTemperature < -25:
						if getRand(dice) < iceOnWater:
							plot.setFeatureType(featIce,-1)
					elif tileTemperature < -10:
						if getRand(dice) < iceOnWater / 2:
							plot.setFeatureType(featIce,-1)
					elif tileTemperature < -5:
						if getRand(dice) < iceOnWater / 3:
							plot.setFeatureType(featIce,-1)
					elif tileTemperature < 0:
						if getRand(dice) < iceOnWater / 4:
							plot.setFeatureType(featIce,-1)
			else:
				if plot.isPeak() or plot.isFreshWater() or plot.isCoastalLand():
					seedList[world.getOffset([x,y])] = [x,y,1,None] # x,y,distance from river,FeatureType
				plot.setTerrainType(randomTerrain.randomItem(dice),False,False)

	# Feature generation
	while len(seedList) > 0:
		x, y, distanceFromRiver, feature = seedList.random(dice)
		plot = map.sPlot(x,y)
		terrain = plot.getTerrainType()
		tileTemperature = getTileTemperature(y,iH)

		if not plot.isPeak():
			if feature == None and plot.getFeatureType() == featNone:
				randomFeat = probabilityArray()

				randomFeat[distanceFromRiver * 14 * (1 - climateHumidity)] = None # how many tiles have no forest and no jungle
				if tileTemperature > 28:
					randomFeat[(distanceFromRiver / 3 + (tileTemperature - 22) / 2) * (1 - (climateHumidity * 0.5) - 0.5)] = None

				if plot.isFreshWater() and not plot.isRiverSide():
					if terrain == terrainDesert:
						if getRand(dice) > 0.6:
							if getRand(dice) > 0.6:
								plot.setTerrainType(terrainGrass,False,False)
							else:
								plot.setTerrainType(terrainPlains,False,False)
					elif terrain == terrainPlains:
						if getRand(dice) > 0.7:
							plot.setTerrainType(terrainGrass,False,False)
					terrain = plot.getTerrainType()

				if plot.isCoastalLand() and not plot.isFreshWater():
					if terrain == terrainDesert:
						if getRand(dice) > 0.3:
								plot.setTerrainType(terrainPlains,False,False)
					elif terrain == terrainPlains:
						if tileTemperature > 5:
							if getRand(dice) > 0.3:
									plot.setTerrainType(terrainGrass,False,False)
					elif terrain == terrainTundra:
						if tileTemperature > 5:
							if getRand(dice) > 0.6:
									plot.setTerrainType(terrainGrass,False,False)
					terrain = plot.getTerrainType()

				# Features based on initial terrain
				if terrain == terrainDesert:
					randomFeat[1] = featJungle #more jungle on sand
				elif terrain == terrainGrass:
					if tileTemperature > 20:
						randomFeat[1] = featJungle
					randomFeat[3] = featForest #most forest on grass
				elif terrain == terrainPlains:
					if tileTemperature > 25:
						randomFeat[2] = featJungle
					randomFeat[3] = featForest
				elif terrain == terrainTundra:
					randomFeat[3] = featForest #more forest in tundra
				elif terrain == terrainSnow:
					randomFeat[3] = featForest #some forest on snow
				if terrain != terrainTundra and terrain != terrainSnow: #no jungle on these
					if tileTemperature > 30:
						randomFeat[1] = featJungle
						if climateHumidity > 0.8:
							randomFeat[1] = featJungle

				# Floods:
				if floodPlains < 0: #standard generation
					if (not plot.isHills()) and plot.isRiverSide(): #no floods on hills
						if getRand(dice) > -1 * floodPlains:
							if terrain == terrainDesert:
								if tileTemperature > 28:
									randomFeat[60 + (tileTemperature - 22) * (1 - climateHumidity)] = featFlood
								else:
									randomFeat[60 + 5 * (1 - climateHumidity)] = featFlood
				else:
					if not plot.isHills(): #no floods on hills
						randomFeat[floodPlains] = featFlood
						if plot.isFreshWater(): #make more floods near lakes and rivers
							if terrain == terrainTundra or terrain == terrainSnow:
								if tileTemperature > -10:
									randomFeat[2 * (1 - climateHumidity)] = featFlood
								elif tileTemperature > -30:
									randomFeat[1 * (1 - climateHumidity)] = featFlood
							elif terrain == terrainDesert:
								if tileTemperature < 40:
									randomFeat[2 * (1 - climateHumidity)] = featFlood
								elif tileTemperature < 55:
									randomFeat[1 * (1 - climateHumidity)] = featFlood
							else:
								randomFeat[4 * (1 - climateHumidity)] = featFlood
						# We increase amount of flood on poor terrain, to make it more playable
						if terrain == terrainDesert:
							randomFeat[3 * (1 - climateHumidity)] = featFlood
						elif terrain == terrainSnow:
							randomFeat[3 * (1 - climateHumidity)] = featFlood
						elif terrain == terrainTundra:
							randomFeat[1 * (1 - climateHumidity)] = featFlood

				# Features based on temperature
				if tileTemperature > 40:
					randomFeat[2] = featJungle
				elif tileTemperature > -20:
					randomFeat[2] = featForest
				else:
					randomFeat[1] = featForest

				feat = randomFeat.randomItem(dice)
				if feat != None:
					plot.setFeatureType(feat,-1)
					feature = feat

		if plot.isPeak():
				# make this seed spread to other tiles
				tileList = getTilesAroundDistance(x,y,2)
				posList = probabilityArray()
				score = 80
				for tile in tileList:
					tx,ty = tile
					tilePlot = map.sPlot(tx,ty)
					tileTerrain = tilePlot.getTerrainType()
					if tilePlot.getFeatureType() != featNone:
						score -= 20
					if tilePlot.getTerrainType() != terrainDesert:
						score -= 5
					if tilePlot.isFreshWater():
						score -= 20
					if tilePlot.isWater():
						score -= 5
					if tilePlot.isPeak():
						score += 7
					if score < 0:
						break
				if score > 0:
					print "Peak with score %d was found."%(score)
					tileList = getTilesAroundDistance(x,y,1)
					for tile in tileList:
						tx,ty = tile
						tilePlot = map.sPlot(tx,ty)
						if (not tilePlot.isPeak()) and (not tilePlot.isWater()):
							if getRand(dice) < (0.1 + climateHumidity * 0.6):
								if tileTemperature > 40:
									seedList[world.getOffset([tx,ty])] = [tx,ty,2,featJungle]
								elif tileTemperature > 35:
									if getRand(dice) > 0.6:
										seedList[world.getOffset([tx,ty])] = [tx,ty,2,featJungle]
									else:
										seedList[world.getOffset([tx,ty])] = [tx,ty,2,featForest]
								elif tileTemperature > 28:
									if getRand(dice) > 0.8:
										seedList[world.getOffset([tx,ty])] = [tx,ty,2,featJungle]
									else:
										seedList[world.getOffset([tx,ty])] = [tx,ty,2,featForest]
								elif tileTemperature > 24:
									if getRand(dice) > 0.95:
										seedList[world.getOffset([tx,ty])] = [tx,ty,2,featJungle]
									else:
										seedList[world.getOffset([tx,ty])] = [tx,ty,2,featForest]
								else:
									seedList[world.getOffset([tx,ty])] = [tx,ty,2,featForest]
		else:
			if feature != None:
				if plot.getFeatureType() == featNone:
					plot.setFeatureType(feature,-1)

				#feature, when spreads, modifies terrain type
				if feature == featJungle:
					if plot.isFreshWater():
						plot.setTerrainType(terrainGrass,False,False)
					elif terrain == terrainDesert:
						if getRand(dice) > 0.6:
							plot.setTerrainType(terrainPlains,False,False)
						else:
							plot.setTerrainType(terrainGrass,False,False)
					elif terrain == terrainPlains:
						if getRand(dice) > 0.1:
							plot.setTerrainType(terrainGrass,False,False)
					elif terrain == terrainTundra:
						plot.setFeatureType(featForest,-1)
					elif terrain == terrainSnow: #that happened once, looks weird :)
						plot.setFeatureType(featForest,-1)
				elif feature == featForest:
					if terrain == terrainDesert:
						if getRand(dice) > 0.3:
							plot.setTerrainType(terrainPlains,False,False)
						else:
							plot.setTerrainType(terrainGrass,False,False)
					elif terrain == terrainPlains:
						if getRand(dice) > 0.4:
							plot.setTerrainType(terrainGrass,False,False)
				elif feature == featFlood:
					if (not plot.isFreshWater()) and floodPlains >= 0:
						if terrain == terrainSnow:
							plot.setTerrainType(terrainTundra,False,False)

				# make this seed spread to other tiles
				tileList = getTilesAroundDistance(x,y,1)
				posList = probabilityArray()
				for tile in tileList:
					tx,ty = tile
					tilePlot = map.sPlot(tx,ty)
					tileTerrain = tilePlot.getTerrainType()
					if not tilePlot.isWater() and not tilePlot.isPeak():
						if feature == featFlood:
							if floodPlains > 0: #if this is not standart mode
								if tilePlot.getFeatureType() == featNone and tilePlot.isFlatlands():
									if tilePlot.isFreshWater():
										posList[2] = [tx,ty,distanceFromRiver + 3,feature]
									else:
										posList[1] = [tx,ty,distanceFromRiver + 3,feature]
						elif feature == featJungle:
							if tilePlot.getFeatureType() == featNone:
								if tilePlot.isFlatlands():
									posList[1] = [tx,ty,distanceFromRiver + 1,feature]
								if not isRiverCrossing(tilePlot,plot):
									if tileTerrain != terrainSnow and tileTerrain != terrainTundra:
										if tileTerrain == terrainDesert:
											posList[1] = [tx,ty,distanceFromRiver + 1,feature]
										else:
											posList[2] = [tx,ty,distanceFromRiver + 1,feature]
							if tileTerrain == terrainDesert:
								if not (tilePlot.getFeatureType() == featOasis or tilePlot.getFeatureType() == featFlood):
									if (getRand(dice) < 0.6):
										if tilePlot.isFreshWater():
											if climateHumidity > 0.5:
												if (getRand(dice) > 0.4):
													tilePlot.setTerrainType(terrainPlains,False,False)
												else:
													tilePlot.setTerrainType(plot.getTerrainType(),False,False)
											else:
												if (getRand(dice) > 0.2):
													tilePlot.setTerrainType(terrainPlains,False,False)
												else:
													tilePlot.setTerrainType(plot.getTerrainType(),False,False)
										else:
											if climateHumidity > 0.5:
												if (getRand(dice) > 0.3):
													tilePlot.setTerrainType(terrainPlains,False,False)
												else:
													tilePlot.setTerrainType(plot.getTerrainType(),False,False)
											else:
												if (getRand(dice) > 0.3):
													tilePlot.setTerrainType(terrainPlains,False,False)
							elif tileTerrain == terrainTundra:
								if plot.getTerrainType() == terrainGrass:
									tilePlot.setTerrainType(terrainPlains,False,False)
								else:
									tilePlot.setTerrainType(plot.getTerrainType(),False,False)
							elif tileTerrain == terrainSnow:
								if (getRand(dice) > 0.5):
									tilePlot.setTerrainType(terrainPlains,False,False)
								else:
									tilePlot.setTerrainType(terrainTundra,False,False)
						elif feature == featForest:
							if not (tilePlot.getFeatureType() == featOasis or tilePlot.getFeatureType() == featFlood):
								if tilePlot.getFeatureType() == featNone:
									if tilePlot.isFlatlands():
										posList[1] = [tx,ty,distanceFromRiver + 1,feature]
									if not isRiverCrossing(tilePlot,plot):
										if tileTerrain != terrainDesert:
											if tileTerrain == terrainPlains:
												posList[1] = [tx,ty,distanceFromRiver + 1,feature]
											else:
												posList[2] = [tx,ty,distanceFromRiver + 1,feature]
								if tileTerrain == terrainDesert:
									if (getRand(dice) < 0.3):
										if tilePlot.isFreshWater():
											if (getRand(dice) > 0.5):
												tilePlot.setTerrainType(terrainPlains,False,False)
											else:
												tilePlot.setTerrainType(plot.getTerrainType(),False,False)
										else:
											if (getRand(dice) > 0.3):
												tilePlot.setTerrainType(terrainPlains,False,False)
											else:
												tilePlot.setTerrainType(plot.getTerrainType(),False,False)

				posList[len(posList) / 2] = None # add some none probability
				item = posList.random(dice)
				if item != None:
					seedList[world.getOffset([item[0],item[1]])] = item

	# find best places for oasis
	possibleOasisList = bArray()
	for y in range(iH):
		for x in range(iW):
			plot = map.sPlot(x,y)
			if (not plot.isWater()) and (not plot.isFreshWater()) and (not plot.isHills()) and (not plot.isCoastalLand()) and (not plot.isPeak()):
				score = 0
				if plot.getTerrainType() == terrainDesert:
					score += 10
					tileList = getTilesAround(x,y)
					for tile in tileList:
						tx,ty = tile
						tilePlot = map.sPlot(tx,ty)
						tileTerrain = tilePlot.getTerrainType()
						if tilePlot.isFreshWater():
							score -= 20
						if tilePlot.isRiverSide():
							score -= 20
						if tilePlot.isPeak():
							score -= 2
						if tilePlot.getFeatureType() == featNone:
							score += 1
						elif tilePlot.getFeatureType() == featJungle:
							score -= 5
						elif tilePlot.getFeatureType() == featForest:
							score -= 3
						elif tilePlot.getFeatureType() == featFlood:
							score -= 20
						if tileTerrain == terrainDesert:
							score += 1
						elif tileTerrain == terrainPlains:
							score -= 2
						elif tileTerrain == terrainGrass:
							score -= 6
						elif tileTerrain == terrainTundra:
							score -= 20
						elif tileTerrain == terrainSnow:
							score -= 20
						if not tilePlot.hasYield():
							score += 1

				if score > 0:
					#plot.setFeatureType(featOasis,-1)
					possibleOasisList[world.getOffset([x,y])] = [x, y]

	missedItems = 0
	for index in range(int(len(possibleOasisList) / 3)):
		x, y = possibleOasisList.random(dice)
		canPlace = True
		tileList = getTilesAround(x,y)
		for tile in tileList:
			tx,ty = tile
			plot = map.sPlot(tx,ty)
			if plot.getFeatureType() == featOasis:
				canPlace = False
				missedItems += 1
				break
		plot = map.sPlot(x,y)
		if canPlace:
			plot.setFeatureType(featOasis,-1)
		if missedItems > 20:
			break

def isRiverCrossing(plot1,plot2):
	cgc = CyGlobalContext()

	dirNorth = cgc.getInfoTypeForString("DIRECTION_NORTH")
	dirNorthEast = cgc.getInfoTypeForString("DIRECTION_NORTHEAST")
	dirEast = cgc.getInfoTypeForString("DIRECTION_EAST")
	dirSouthEast = cgc.getInfoTypeForString("DIRECTION_SOUTHEAST")
	dirSouth = cgc.getInfoTypeForString("DIRECTION_SOUTH")
	dirSouthWest = cgc.getInfoTypeForString("DIRECTION_SOUTHWEST")
	dirWest = cgc.getInfoTypeForString("DIRECTION_WEST")
	dirNorthWest = cgc.getInfoTypeForString("DIRECTION_NORTHWEST")

	map = cgc.getMap()
	iW = map.getGridWidth()
	iH = map.getGridHeight()

	# quite nasty lines of code to determine plot direction

	x1 = plot1.getX()
	y1 = plot1.getY()
	x2 = plot2.getX()
	y2 = plot2.getY()

	if x2 >= iW:
		x2 = 0
	elif x2 < 0:
		x2 = iW - 1
	if y2 >= iH:
		y2 = 0
	elif y2 < 0:
		y2 = iH - 1

	dx = x2 - x1
	dy = y2 - y1

	direction = -1
	if dx < 0 and dy == 0:
		direction = dirWest
	elif dx < 0 and dy > 0:
		direction = dirNorthWest
	elif dx == 0 and dy > 0:
		direction = dirNorth
	elif dx > 0 and dy > 0:
		direction = dirNorthEast
	elif dx > 0 and dy == 0:
		direction = dirEast
	elif dx > 0 and dy < 0:
		direction = dirSouthEast
	elif dx == 0 and dy < 0:
		direction = dirSouth
	elif dx < 0 and dy < 0:
		direction = dirSouthWest

	if direction == -1:
		return False
	else:
		return plot1.isRiverCrossing(direction)

def getTilesAround(x,y):
	cgc = CyGlobalContext()
	map = cgc.getMap()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	l = []
	for ax, ay in [[x-1,y],[x-1,y+1],[x,y+1],[x+1,y+1],[x+1,y],[x+1,y-1],[x,y-1],[x-1,y-1]]:
		# K-Mod-Magnus: popping the off-map ones by index removed the wrong ones (and failed at the map's edge)
		if ay >= 0 and ay < iH:
			l.append([ax % iW, ay])
	return l

def getTilesAroundDistance(x,y,distance,plotTypes = None):
	cgc = CyGlobalContext()
	map = cgc.getMap()
	iW = map.getGridWidth()
	iH = map.getGridHeight()
	l = []
	if plotTypes == None:
		for by in range(distance * 2 + 1):
			for bx in range(distance * 2 + 1):
				ax = x - distance + bx
				ay = y - distance + by
				if ay >= 0 and ay < iH:
					if (not (ax == x and ay == y)):
						ax = ax % iW # K-Mod-Magnus: was iW - 1 or 0 whatever the distance
						l.append([ax,ay])
	else:
		for by in range(distance * 2 + 1):
			for bx in range(distance * 2 + 1):
				ax = x - distance + bx
				ay = y - distance + by
				if ay >= 0 and ay < iH:
					if (not (ax == x and ay == y)):
						ax = ax % iW # K-Mod-Magnus: was iW - 1 or 0 whatever the distance
						if map.plot(ax, ay).getPlotType() in plotTypes:
							l.append([ax,ay])
	return l

def initClimateSettings():
	global climateVariation
	global climateTemperature
	climateVariation = getSelValue("variation")
	if climateVariation is None:
		climateVariation = 0.4
	climateTemperature = getSelValue("temperature")
	if climateTemperature is None:
		climateTemperature = 40

def getTileTemperature(y,h):
	global climateTemperature
	global climateVariation
	lowestTemperature = ((climateTemperature + 50) * climateVariation) - 50
	th = h - 1
	half = th / 2.0
	if y > half:
		return lowestTemperature + ((climateTemperature - lowestTemperature) * (half - (y - half)) / half)
	else:
		return lowestTemperature + ((climateTemperature - lowestTemperature) * y / half)

#Step 5: add bonuses to your map
#this method puts down the tile bonuses, such as iron, whale, rice, oil, etc.
def addBonuses():
	"Can override to control where bonuses are added on	the	map"
	print "-------- Add bonuses ----------"
	global bonusGeneration

	bonusGeneration = getSelValue("bonus_generation")
	if bonusGeneration is None: #ensure we don't make something bad if wrong settings exist
		bonusGeneration = "standard"

	if bonusGeneration == "standard":
		CyPythonMgr().allowDefaultImpl()
	#CyPythonMgr().allowDefaultImpl()

#Step 5B: override addBonusType so that it can pass through
def addBonusType(argsList):
	"Can override to control where a type of bonus is added	on the map"
	"""WARNING:	If addBonuses()	is overriden then this function	will not automatically be called."""
	[iBonusType] = argsList
	CyPythonMgr().allowDefaultImpl()

def canPlaceBonusAt(argsList):
	"Can override to determine if a	bonus can be placed	at the specified plot"
	"""WARNING:	If addBonuses()	is overriden then this function	will not automatically be called."""
	pPlot =	argsList[0]
	CyPythonMgr().allowDefaultImpl()

#Step 6: place goody huts
def addGoodies():
	"Can override to control where goodies are added on	the	map"
	global generationStarted
	generationStarted = False
	# K-Mod-Magnus: the resources are on the map by now (addBonuses), the starting plots not yet
	if getSelValue("every_resource"):
		ensureEveryResource(2)
	CyPythonMgr().allowDefaultImpl()

def canPlaceGoodyAt(argsList):
	pPlot = argsList[0]
	"Can override to determine if a	goody can be placed	at the specified plot"
	CyPythonMgr().allowDefaultImpl()

def afterGeneration():
	"Called	after the map generation is	finished"

#------------------------------------------------------
"""STARTING	PLOT FUNCTIONS"""
#------------------------------------------------------

def minStartingDistanceModifier():
	"Returns a percent modification	for	the	minimum	distance between players (return 50	would mean +50%)"
	return 0

def assignStartingPlots():
	"Can assign	as many	starting locations as desired"
	CyPythonMgr().allowDefaultImpl()

def makeStartAreaPlan():
	"""K-Mod-Magnus, "Rival civs on your continent": player -> area, so that the human player (with his team) and
	that many rivals start on one continent and everybody else on the others, shared out by their land (or by the
	players they were made for). {} when the option is off or there is only one continent."""
	if homeRivals < 0:
		return {}
	cgc = CyGlobalContext()
	cymap = CyMap()
	dice = cgc.getGame().getMapRand()
	players = []
	human = -1
	for p in range(cgc.getMAX_CIV_PLAYERS()):
		player = cgc.getPlayer(p)
		if player.isAlive():
			players.append(p)
			if human < 0 and player.isHuman():
				human = p
	if human < 0 or len(players) < 2:
		return {}
	conts = [] # [plots, area, continent], as the game sees them now
	seen = {}
	for i in range(len(continentAnchors)):
		o = continentAnchors[i]
		if o < 0:
			continue
		plot = cymap.plotByIndex(o)
		if plot.isWater():
			continue
		a = plot.getArea()
		if seen.has_key(a):
			continue
		seen[a] = True
		conts.append([cymap.getArea(a).getNumTiles(), a, i])
	if len(conts) < 2:
		return {}
	team = cgc.getPlayer(human).getTeam()
	mates = []
	rivals = []
	for p in players:
		if p != human:
			if cgc.getPlayer(p).getTeam() == team:
				mates.append(p)
			else:
				rivals.append(p)
	k = homeRivals
	if k > len(rivals):
		k = len(rivals)
	home = None
	for c in conts:
		if c[2] == homeContinent:
			home = c
	total = 0
	for c in conts:
		total += c[0]
	if home is None:
		# the continent whose share of the land fits the players on it best
		share = (k + 1 + len(mates)) / float(len(players))
		best = 2.0
		for c in conts:
			d = abs(c[0] / float(total) - share)
			if d < best:
				best = d
				home = c
	plan = {human: home[1]}
	for p in mates:
		plan[p] = home[1]
	shuffled = []
	while len(rivals) > 0:
		shuffled.append(rivals.pop(dice.get(len(rivals), "Python - Planet Generator: start continent")))
	for p in shuffled[:k]:
		plan[p] = home[1]
	rest = shuffled[k:]
	others = []
	for c in conts:
		if c is not home:
			others.append(c)
	if len(rest) > 0:
		# shares: the players the continents were made for ("By player count"), otherwise their land
		weights = []
		for c in others:
			w = float(c[0])
			if continentPlayers is not None and c[2] < len(continentPlayers):
				w = float(continentPlayers[c[2]]) + 0.01
			weights.append(w)
		wsum = 0.0
		for w in weights:
			wsum += w
		quota = []
		given = 0
		for i in range(len(others)):
			q = len(rest) * weights[i] / wsum
			quota.append([int(q), q - int(q)])
			given += int(q)
		while given < len(rest):
			best = 0
			for i in range(len(quota)):
				if quota[i][1] > quota[best][1]:
					best = i
			quota[best][0] += 1
			quota[best][1] = -1.0
			given += 1
		j = 0
		for i in range(len(others)):
			for n in range(quota[i][0]):
				plan[rest[j]] = others[i][1]
				j += 1
	print "Planet Generator: start areas", plan
	return plan

def ensureEveryResource(minCount):
	"""K-Mod-Magnus, "Every resource at least twice": adds missing resources where they can be (at any latitude),
	spread out and on other landmasses than the ones they have. Where the climate left no such plot, one is made
	to fit (terrain, forest or jungle, hills) as the resource needs."""
	cgc = CyGlobalContext()
	cymap = CyMap()
	dice = cgc.getGame().getMapRand()
	iW = cymap.getGridWidth()
	numPlots = cymap.numPlots()
	plots = []
	for i in range(numPlots):
		plots.append(cymap.plotByIndex(i))
	where = {}
	for i in range(numPlots):
		b = plots[i].getBonusType(-1)
		if b >= 0:
			if not where.has_key(b):
				where[b] = []
			where[b].append(i)
	terrains = []
	for name in ["TERRAIN_GRASS", "TERRAIN_PLAINS", "TERRAIN_DESERT", "TERRAIN_TUNDRA", "TERRAIN_SNOW", "TERRAIN_COAST", "TERRAIN_OCEAN"]:
		t = cgc.getInfoTypeForString(name)
		if t >= 0:
			terrains.append(t)
	# latitude each terrain or feature suits, for making a plot fit with as little change as possible
	suits = {}
	for name, lat in [["TERRAIN_GRASS", 30], ["TERRAIN_PLAINS", 40], ["TERRAIN_DESERT", 20], ["TERRAIN_TUNDRA", 65], ["TERRAIN_SNOW", 80], ["FEATURE_JUNGLE", 10], ["FEATURE_FOREST", 50]]:
		suits[cgc.getInfoTypeForString(name)] = lat
	features = []
	for name in ["FEATURE_FOREST", "FEATURE_JUNGLE"]:
		f = cgc.getInfoTypeForString(name)
		if f >= 0:
			features.append(f)

	def dist(a, b):
		dx = abs(a % iW - b % iW)
		if dx > iW - dx:
			dx = iW - dx
		dy = abs(a // iW - b // iW)
		if dx > dy:
			return dx
		return dy

	def spread(i, have):
		"spread score: other landmasses than the resource's, then far from it"
		if len(have) == 0:
			return 0
		v = 1000
		near = 99
		area = plots[i].getArea()
		for j in have:
			if plots[j].getArea() == area:
				v = 0
			d = dist(i, j)
			if d < near:
				near = d
		return v + min(near, 30)

	for b in range(cgc.getNumBonusInfos()):
		info = cgc.getBonusInfo(b)
		if info.getConstAppearance() <= 0:
			continue # not placed on maps (e.g. from buildings)
		have = []
		if where.has_key(b):
			have = where[b]
		missing = minCount - len(have)
		if missing <= 0:
			continue
		added = 0
		forced = 0
		# 1. plots that can take it as they are, the latitude aside
		cands = []
		for i in range(numPlots):
			if plots[i].canHaveBonus(b, True):
				cands.append(i)
		while missing > 0 and len(cands) > 0:
			best = -1
			bestValue = -1
			for t in range(min(300, len(cands))):
				i = cands[dice.get(len(cands), "Python - Planet Generator: resource plot")]
				v = spread(i, have) * 10 + dice.get(10, "Python - Planet Generator: resource plot")
				if v > bestValue:
					bestValue = v
					best = i
			plots[best].setBonusType(b)
			have = have + [best]
			cands.remove(best)
			missing -= 1
			added += 1
		# 2. make plots fit: the terrains (and forest or jungle on them) it can be on
		if missing > 0:
			options = []
			for t in terrains:
				if info.isTerrain(t):
					options.append([t, -1])
			for f in features:
				if info.isFeature(f):
					for t in terrains:
						if info.isFeatureTerrain(t) and cgc.getFeatureInfo(f).isTerrain(t):
							options.append([t, f])
			tries = 0
			while missing > 0 and len(options) > 0 and tries < 400:
				tries += 1
				t, f = options[dice.get(len(options), "Python - Planet Generator: resource terrain")]
				water = cgc.getTerrainInfo(t).isWater()
				best = -1
				bestValue = -99999
				for n in range(60):
					i = dice.get(numPlots, "Python - Planet Generator: resource plot")
					plot = plots[i]
					if plot.getBonusType(-1) >= 0 or plot.isPeak() or plot.isWater() != water or plot.isStartingPlot():
						continue
					if not water and ((plot.isHills() and not info.isHills()) or (plot.isFlatlands() and not info.isFlatlands())):
						continue
					if info.isNoRiverSide() and plot.isRiverSide():
						continue
					v = spread(i, have) * 10
					if plot.getTerrainType() == t:
						v += 50
					if suits.has_key(t):
						v -= abs(plot.getLatitude() - suits[t])
					if f >= 0 and suits.has_key(f):
						v -= abs(plot.getLatitude() - suits[f])
					v += dice.get(10, "Python - Planet Generator: resource plot")
					if v > bestValue:
						bestValue = v
						best = i
				if best < 0:
					continue
				plot = plots[best]
				oldTerrain = plot.getTerrainType()
				oldFeature = plot.getFeatureType()
				oldVariety = plot.getFeatureVariety()
				plot.setTerrainType(t, True, True)
				plot.setFeatureType(f, -1)
				if plot.canHaveBonus(b, True):
					plot.setBonusType(b)
					have = have + [best]
					missing -= 1
					forced += 1
				else:
					plot.setTerrainType(oldTerrain, True, True)
					plot.setFeatureType(oldFeature, oldVariety)
			if missing > 0 and info.isHills() and not info.isFlatlands():
				# it needs hills and none would do: make some
				for n in range(400):
					if missing <= 0:
						break
					i = dice.get(numPlots, "Python - Planet Generator: resource plot")
					plot = plots[i]
					if plot.isWater() or plot.isPeak() or plot.isHills() or plot.getBonusType(-1) >= 0 or plot.isStartingPlot():
						continue
					for t, f in options:
						if cgc.getTerrainInfo(t).isWater():
							continue
						oldTerrain = plot.getTerrainType()
						oldFeature = plot.getFeatureType()
						oldVariety = plot.getFeatureVariety()
						plot.setPlotType(PlotTypes.PLOT_HILLS, True, True)
						plot.setTerrainType(t, True, True)
						plot.setFeatureType(f, -1)
						if plot.canHaveBonus(b, True):
							plot.setBonusType(b)
							have = have + [i]
							missing -= 1
							forced += 1
							break
						plot.setPlotType(PlotTypes.PLOT_LAND, True, True)
						plot.setTerrainType(oldTerrain, True, True)
						plot.setFeatureType(oldFeature, oldVariety)
		if added + forced > 0:
			print "Planet Generator: %s %d more (%d where it could be, %d on a plot made to fit)%s" % (info.getType(), added + forced, added, forced, (missing > 0 and ", still missing %d" % missing) or "")

def continentAreas():
	"""K-Mod-Magnus: the game's land areas of the continents (an archipelago continent has several), not of the
	islands"""
	cymap = CyMap()
	result = []
	if landPlan is None or landPlan.N != cymap.numPlots():
		return result
	seen = {}
	mass = landPlan.mass
	for o in xrange(landPlan.N):
		m = mass[o]
		if m >= 0 and m < 1000:
			plot = cymap.plotByIndex(o)
			if not plot.isWater():
				a = plot.getArea()
				if not seen.has_key(a):
					seen[a] = True
					result.append(a)
	return result

def bestContinentArea():
	"""K-Mod-Magnus: the area CvPlayer::findStartingArea would choose (the same value), among the continents only:
	it looks at every land area, so a big island could get a civ. -1 if the continents are not known."""
	global continentAreaIDs
	if continentAreaIDs is None:
		continentAreaIDs = continentAreas()
	cymap = CyMap()
	cityPlots = CyGlobalContext().getNUM_CITY_PLOTS()
	best = -1
	bestValue = 0
	for a in continentAreaIDs:
		area = cymap.getArea(a)
		if area.isNone():
			continue
		onArea = area.getNumStartingPlots() + 1 # with this player
		tiles = area.getNumTiles()
		value = (area.calculateTotalBestNatureYield() + area.countCoastalLand() * 2 + area.getNumRiverEdges() + tiles + 1) // onArea
		value = value * min(cityPlots + 1, tiles + 1) // (cityPlots + 1)
		if onArea <= 2:
			value = value * 4 // 3
		if value > bestValue:
			bestValue = value
			best = a
	return best

def findStartingArea(argsList):
	"""Can override	to return the ID of	the	area in	which the given	player should start. Return	an areaID to
	force the player to	start in a certain area, or	-1 to specify that the player should start in the best
	location, irrespective of which	area it	is in. If not overridden, the default implementation will try
	to maximize	the	size of	the	player's part of his area

	WARNING: If	findStartingPlot() is overriden	then this function will	not	automatically be called."""
	playerID = argsList[0]
	# K-Mod-Magnus: "Rival civs on your continent", otherwise the area the game would choose among the continents
	# (nobody starts on an island). The DLL updates the found values and then asks for the area; the best plot in it
	# is found by the default findStartingPlot.
	global startAreaPlan
	if startAreaPlan is None:
		startAreaPlan = makeStartAreaPlan()
	if startAreaPlan.has_key(playerID):
		return startAreaPlan[playerID]
	area = bestContinentArea()
	if area < 0:
		CyPythonMgr().allowDefaultImpl()
		return
	return area

def findStartingPlot(argsList):
	"Can override to return	the	plot index at which	the	given player should	start"
	#print "-------- Find starting plot ----------"
	playerID = argsList[0]
	CyPythonMgr().allowDefaultImpl()

def normalizeStartingPlotLocations():
	"Can override to change	how	starting plots are distributed.	(note, this	function is	called after all starting locs are distributed...)"
	print "-------- Normalize starting plot locations ----------"
	if startAreaPlan: # K-Mod-Magnus: the start continents were planned; swapping starts (team games) would undo that
		return
	CyPythonMgr().allowDefaultImpl()

def normalizeAddRiver():
	"A normalize start function	(called	after starting plots are set) -	this call adds a river."
	print "-------- Normalize add river ----------"
	if bonusGeneration == "standard":
		CyPythonMgr().allowDefaultImpl()

def normalizeRemovePeaks():
	"A normalize start function	(called	after starting plots are set) -	this call removes peaks."
	print "-------- Normalize remove peaks ----------"
	if bonusGeneration == "standard":
		CyPythonMgr().allowDefaultImpl()

def normalizeAddLakes():
	"A normalize start function	(called	after starting plots are set) -	this call adds a lake."
	print "-------- Normalize add lakes ----------"
	if bonusGeneration == "standard":
		CyPythonMgr().allowDefaultImpl()

def normalizeRemoveBadFeatures():
	"A normalize start function	(called	after starting plots are set) -	this call removes bad features."
	print "-------- Normalize remove gbad features ----------"
	CyPythonMgr().allowDefaultImpl()

def normalizeRemoveBadTerrain():
	"A normalize start function	(called	after starting plots are set) -	this call removes bad terrain."
	print "-------- Normalize remove good terrain ----------"
	if bonusGeneration == "standard":
		CyPythonMgr().allowDefaultImpl()

def normalizeAddFoodBonuses():
	"A normalize start function	(called	after starting plots are set) -	this call adds food	bonuses."
	print "-------- Normalize add food ----------"
	if bonusGeneration == "standard":
		CyPythonMgr().allowDefaultImpl()

def normalizeAddGoodTerrain():
	"A normalize start function	(called	after starting plots are set) -	this call adds good	terrain."
	print "-------- Normalize add good terrain ----------"
	if bonusGeneration == "standard":
		CyPythonMgr().allowDefaultImpl()

# base class for fair bonus, resource, etc distribution handling
class FairBuilder:
	def __init__(self,world,usedTiles):
		self.cgc = CyGlobalContext()
		self.dice = self.cgc.getGame().getMapRand()
		self.map = self.cgc.getMap()
		self.world = world
		self.edgeTiles = bArray()
		self.usedTiles = usedTiles
		self.validPlotTypes = [PlotTypes.PLOT_LAND, PlotTypes.PLOT_HILLS]

	def start(self,x,y):
		#print "--- Start grow: x=%d y=%d"%(x,y)
		distance = 0
		offset = self.world.getOffset([x,y])
		if self._grow(offset,x,y,distance):
			self.usedTiles[offset] = [offset, x, y, distance]
			self.edgeTiles[offset] = [offset, x, y, distance, self._getPossibleTiles(offset, x, y, distance)]
			return True
		else:
			return False

	def grow(self):
		tile = None
		gtile = None
		while tile == None:
			gtile = None
			tile = self.edgeTiles.randomItem(self.dice)
			if tile != None:
				offset, x, y, distance, validTiles = tile
				#print "--- Random edge tile try: x=%d y=%d"%(x,y)
				gtile = None
				while gtile == None:
					gtile = validTiles.random(self.dice)
					if gtile == None:
						#print "--- Random possible tile try failed"
						break
					else:
						aoffset, ax, ay, adistance = gtile
						#print "--- Random possible tile try: x=%d y=%d distance=%d"%(ax,ay,adistance)
						if self.usedTiles[aoffset] == None:
							#print "ok"
							break
						else:
							#print "remove"
							validTiles.remove(aoffset)
							gtile = None
				if gtile != None:
					break
				else:
					self.edgeTiles.remove(offset)
					tile = None
			else:
				#print "--- Random edge tile try failed"
				break

		if tile != None and gtile != None:
			offset, x, y, distance, validTiles = tile
			aoffset, ax, ay, adistance = gtile
			if self._grow(aoffset, ax, ay, adistance):
				#print "--- Tile Grow To: x=%d y=%d"%(ax,ay)
				self.usedTiles[aoffset] = [aoffset, ax, ay, adistance]
				possibleTiles = self._getPossibleTiles(aoffset, ax, ay, adistance)
				#print "--- Possible tile count: %d"%(len(possibleTiles))
				if len(possibleTiles) > 0:
					self.edgeTiles[aoffset] = [aoffset, ax, ay, adistance, possibleTiles]
					#print "--- Add edge tile: x=%d y=%d"%(ax,ay)
				tilesAround = getTilesAround(ax,ay)
				for rmtile in tilesAround:
					rmx, rmy = rmtile
					rmoffset = self.world.getOffset([rmx,rmy])
					edgeTile = self.edgeTiles[rmoffset]
					if edgeTile != None:
						if self._isEdgeTileExpired(edgeTile,aoffset):
							#print "--- Remove edge tile: x=%d y=%d"%(rmx,rmy)
							self.edgeTiles.remove(rmoffset)
			else:
				return False
		else:
			#print "Fair builder failed"
			return False
		return True

	def _isEdgeTileExpired(self,edgeTile,newOffset):
		offset, x, y, distance, validTiles = edgeTile
		if validTiles[newOffset] != None:
			validTiles.remove(newOffset)
		if len(validTiles) == 0:
			return True
		return False

	def _getPossibleTiles(self,offset,x,y,distance):
		tilesAround = getTilesAround(x,y)
		validTiles = bArray()
		for atile in tilesAround:
			ax, ay = atile
			aoffset = self.world.getOffset([ax,ay])
			if self.usedTiles[aoffset] == None:
				if self._canGrow(offset, x, y, aoffset, ax, ay, distance + 1):
					validTiles[aoffset] = [aoffset, ax, ay, distance + 1]
		return validTiles

	def _canGrow(self,originalOffset,ox,oy,offset,x,y,distance):
		plot = self.map.plotByIndex(offset)
		plotType = plot.getPlotType()
		if plotType in self.validPlotTypes:
			return True
		return False

	def _grow(self,offset,x,y,distance):
		return True

class BonusBuilder(FairBuilder):
	def __init__(self,world,usedTiles,usedCoastTiles):
		FairBuilder.__init__(self,world,usedTiles)
		self.usedCoastTiles = usedCoastTiles

		# several arrays to store tile lists
		self.allBuilderTiles = bArray()
		self.coastTiles = bArray()
		self.landTilesWithoutBonuses = bArray()
		self.coastTilesWithoutBonuses = bArray()

		# player area distance
		self.maxDistance = 0
		# coast distance from land
		self.coastDistance = 2

		# walkable plot types
		self.validPlotTypes = [PlotTypes.PLOT_LAND, PlotTypes.PLOT_HILLS]

	def _grow(self,offset,x,y,distance):
		if distance > self.maxDistance:
			self.maxDistance = distance
		plot = self.map.plotByIndex(offset)
		self.allBuilderTiles[offset] = [offset, x, y, distance]
		self.landTilesWithoutBonuses[offset] = [offset, x, y, distance]
		tiles = getTilesAroundDistance(x,y,self.coastDistance,[PlotTypes.PLOT_OCEAN])
		for tile in tiles:
			ax, ay = tile
			aoffset = self.world.getOffset([ax,ay])
			if self.usedCoastTiles[aoffset] == None:
				self.usedCoastTiles[aoffset] = [aoffset]
				self.coastTiles[aoffset] = [aoffset, ax, ay, distance + 1]
				self.coastTilesWithoutBonuses[aoffset] = [aoffset, ax, ay, distance + 1]
		#print "--- Grow OK: x=%d y=%d"%(x,y)
		return True

def normalizeAddExtras():
	"A normalize start function (called after starting plots are set) - this call adds extra features and bonuses."
	print "-------- Normalize extras ----------"
	collectGarbage()

	global bonusGeneration

	if bonusGeneration == "standard":
		CyPythonMgr().allowDefaultImpl()
	else:
		cgc = CyGlobalContext()

		terrainGrass = cgc.getInfoTypeForString("TERRAIN_GRASS")
		terrainPlains = cgc.getInfoTypeForString("TERRAIN_PLAINS")
		terrainDesert = cgc.getInfoTypeForString("TERRAIN_DESERT")
		terrainTundra = cgc.getInfoTypeForString("TERRAIN_TUNDRA")
		terrainSnow = cgc.getInfoTypeForString("TERRAIN_SNOW")
		terrainCoast = cgc.getInfoTypeForString("TERRAIN_COAST")
		terrainOcean = cgc.getInfoTypeForString("TERRAIN_OCEAN")

		terrainWater = [terrainCoast, terrainOcean]
		terrainLand = [terrainGrass, terrainPlains, terrainDesert, terrainTundra, terrainSnow]

		global continentCount

		map = cgc.getMap()

		dice = cgc.getGame().getMapRand()

		iW = map.getGridWidth()
		iH = map.getGridHeight()

		# world = worldArray([iW,iH],PlotTypes.PLOT_OCEAN)
		# in our case we use existing world, but only for index and wraping calculations
		global world

		bonusTypes = enumeratePlaceableBonusTypes()
		bonusTypesOcean = []
		bonusTypesLand = []
		for index in range(len(bonusTypes)):
			bonusInt = bonusTypes._data[index][0]
			bonusInfo = bonusTypes._data[index][1][1]
			bWater = False
			for terrain in terrainWater:
				if bonusInfo.isTerrain(terrain):
					bWater = True
					break
			if bWater:
				bonusTypesOcean.append([bonusInt,bonusInfo])
			bLand = False
			for terrain in terrainLand:
				if bonusInfo.isTerrain(terrain):
					bLand = True
					break
			if bLand:
				bonusTypesLand.append([bonusInt,bonusInfo])
		bonusTypesOcean.sort(lambda x, y: cmp(x[1].getPlacementOrder(), y[1].getPlacementOrder()))
		bonusTypesLand.sort(lambda x, y: cmp(x[1].getPlacementOrder(), y[1].getPlacementOrder()))
		print "bonusTypesOcean:",bonusTypesOcean
		print "bonusTypesLand:",bonusTypesLand

		# we use BonusBuilder class to get all land and coast tiles player can reach without sailing
		# it is done using randomly calculated aproximate traveling paths, but produces generally accurate result
		# also, we get distance to each tile, so we can use this information to distribute bonus well

		bonusBuilders = []
		usedTiles = bArray()
		usedCoastTiles = bArray()

		for pIndex in range(cgc.getMAX_CIV_PLAYERS()):
			player = cgc.getPlayer(pIndex)
			if player.isAlive():
				plot = player.getStartingPlot()
				px = plot.getX()
				py = plot.getY()
				builder = BonusBuilder(world, usedTiles, usedCoastTiles)
				bonusBuilders.append(builder)
				builder.start(px,py)

		grows = True
		maxSize = iW * iH
		size = 0
		while grows and size < maxSize:
			for builder in bonusBuilders:
				if builder.grow():
					size += 1
				else:
					grows = False
		if size >= maxSize:
			print "WARNING: size limit exceeded!"

		for index in range(cgc.getNumBonusInfos()):
			bonusInfo = cgc.getBonusInfo(index)

		featIce = cgc.getInfoTypeForString("FEATURE_ICE")
		featOasis = cgc.getInfoTypeForString("FEATURE_OASIS")

		for item in bonusTypesLand:
			bonusInt, bonusInfo = item
			usedTiles = bArray()
			playerResCount = []
			for index in range(len(bonusBuilders)):
				playerResCount.append([index, 0])

		for index in range(len(bonusBuilders)):
			builder = bonusBuilders[index]
			landTiles = builder.allBuilderTiles
			coastTiles = builder.coastTiles
			landTilesWB = builder.landTilesWithoutBonuses
			coastTilesWB = builder.coastTilesWithoutBonuses
			maxDistance = bonusBuilders[index].maxDistance
			if maxDistance > 0:
				for tile in landTiles:
					offset, x, y, distance = tile
					relDistance = distance / float(maxDistance)
					plot = map.plotByIndex(offset)
					#plot.setTerrainType(index,False,False)
					plot.setBonusType(bonusTypes._data[index][0])
				for tile in coastTiles:
					offset, x, y, distance = tile
					relDistance = distance / float(maxDistance)
					plot = map.plotByIndex(offset)
					#plot.setFeatureType(featIce,-1)
					plot.setBonusType(bonusTypes._data[index+5][0])

def cmpByPlacementOrder(x, y):
	return x[1].getPlacementOrder() - y[0].getPlacementOrder()

def enumeratePlaceableBonusTypes():
	cgc = CyGlobalContext()
	arr = bArray()
	for i in range(cgc.getNumBonusInfos()):
		bonusInfo = cgc.getBonusInfo(i)
		if bonusInfo.getTilesPer() > 0 or bonusInfo.getPercentPerPlayer() > 0:
			arr[i] = [i, bonusInfo]
	return arr

def startHumansOnSameTile():
	"Returns true if all human units should	start on the same tile"
	return False

#----------------------------------------------
"""Helpers"""
#----------------------------------------------
def getRand(dice):
	"""return random number from 0 to 0.999"""
	return (dice.get(999,"Python - EarthGen: Random dice.")) / 1000.0
# taken from SmartMap
import os
import os.path

if (sys.platform == 'darwin'):
	def civFilePath():
		try:
			civ4Dir = os.path.basename(os.getcwd())
			if "Warlords" in civ4Dir:
				civ4Dir = "Civilization IV Warlords"
			myDocuments = os.path.join(os.environ['HOME'], "Documents")
			finalFolder =  os.path.join(myDocuments, civ4Dir)
			return finalFolder
		except:
			return ""
else:
	import _winreg
	def regRead(registry, path, field):
		try:
			pathKey = _winreg.OpenKey(registry, path)
			fieldValue = _winreg.QueryValueEx(pathKey, field)
			return fieldValue[0]
		finally:
			pathKey.Close()
	def civFilePath():
		try:
			userFolder = regRead(_winreg.HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders","Personal")
			#print "    userFolder",userFolder
			civFolder = os.path.basename(regRead(_winreg.HKEY_LOCAL_MACHINE,"Software\\Firaxis Games\\Sid Meier's Civilization 4","INSTALLDIR"))
			#print "    civFolder",civFolder
			finalFolder = os.path.join(os.path.join(userFolder, "My Games"), civFolder)
			#print "    finalFolder",finalFolder
			return finalFolder
		except:
			return ""
#----------------------------------------------
"""Other functions"""
#----------------------------------------------
def isAdvancedMap():
	"""
	Advanced maps only show	up in the map script pulldown on the advanced menu.
	Return 0 if	you	want your map to show up in	the	simple singleplayer	menu
	"""
	return 0
def getModPath():
	"""
	Returns	the	path of	the	mod	this map is	associated with
	"""
	return ""
def isClimateMap():
	"""Uses the Climate options"""
	return 0

def isSeaLevelMap():
	"""Uses the Sea Level options"""
	return 0

def getNumCustomMapOptions():
	loadMapOptionDefaults()
	return len(selection_names_and_values)

def getCustomMapOptionName(argsList):
	#print "getCustomMapOptionName",argsList
	index = argsList[0]
	return unicode(selection_titles[index][0])

def getNumCustomMapOptionValues(argsList):
	#print "getNumCustomMapOptionValues",argsList
	index = argsList[0]
	return len(selection_names_and_values[index])

def getClosestMapSizeName(width, height):
	cgc = CyGlobalContext()
	area = width * height / 16
	for i in range(cgc.getNumWorldInfos()):
		worldInfo = cgc.getWorldInfo(i)
		if worldInfo.getGridHeight() * worldInfo.getGridWidth() == area:
			return " (" + worldInfo.getText() + ")"
	return ""

def getCustomMapOptionDescAt(argsList):
	#print "getCustomMapOptionDescAt",argsList
	iOption = argsList[0]
	iSelection = argsList[1]
	if selection_titles[iOption][1] == "map_size":
		w, h = selection_names_and_values[iOption][iSelection][1]
		translated_text = unicode(selection_names_and_values[iOption][iSelection][0] + getClosestMapSizeName(w, h))
	else:
		translated_text = unicode(selection_names_and_values[iOption][iSelection][0])
	return translated_text

def getCustomMapOptionDefault(argsList):
	index =	argsList[0]
	return selection_defaults[index]

def saveMapOptionDefaults():
	cymap = CyMap()
	fileName = os.path.join(civFilePath(),"planet_generator_" + VERSION + ".cfg")
	try:
		settings = open(fileName, 'w')
		smoptions = []
		index = -1
		for item in selection_titles:
			smoption = item[1]
			index += 1
			selectionIndex = int(cymap.getCustomMapOption(index))
			valueName = selection_names_and_values[index][selectionIndex][0]
			smoptions.append((smoption,valueName))

		#print "    try to store settings to",fileName
		print smoptions
		try:
			pickle.dump(smoptions, settings)
		except Exception, inst:
			print "    Pickling Error",inst,"trying to save map settings to",fileName
		settings.close()
		print "    store successful"
	except IOError:
		print "    Couldn't create ",fileName
	except EOFError:
		print "    EOF writing ",fileName
	except:
		print "    unexpected problem writing ",fileName

def loadMapOptionDefaults():
	global selection_defaults
	fileName = os.path.join(civFilePath(),"planet_generator_" + VERSION + ".cfg")
	try:
		settings = open(fileName, 'r')
		optionsWithDefault = pickle.load(settings)
		#print optionsWithDefault
		for optionIndex in range(len(optionsWithDefault)):
			option, valueName = optionsWithDefault[optionIndex]
			# K-Mod-Magnus: settings saved by the original script
			if valueName.startswith("More than "):
				valueName = valueName[len("More than "):]
			if option == "allow_one_tile_islands":
				option = "min_island_size"
				if valueName == "Yes":
					valueName = "1 plot (one-plot islands allowed)"
				else:
					valueName = "2 plots (default)"
			for selOptionIndex in range(len(selection_titles)):
				selectionTitleCaption, selectionTitleName, autoRandom = selection_titles[selOptionIndex]
				if option == selectionTitleName:
					for selectionIndex in range(len(selection_names_and_values[selOptionIndex])):
						selectionName, selectionValue, allowedRandomPick = selection_names_and_values[selOptionIndex][selectionIndex]
						if selectionName == valueName:
							selection_defaults[selOptionIndex] = selectionIndex
		#print "    loaded option from cfg:",option,value
		settings.close()
	except IOError:
		print "    Couldn't find ",fileName
	except EOFError:
		print "    Bad contents in ",fileName
	except:
		print "    unexpected problem reading",fileName

def isRandomCustomMapOption(argsList):
	return False # we use our own defaults

#----------------------------------------------
"""MAP SIZE	AND	WRAPPING FUNCTIONS"""
#----------------------------------------------

# note:	you	must override both getWrap...()	functions or neither
def getWrapX():
	"Can be	overridden to change whether the map wraps around in the X direction. Default is true"
	return True
def getWrapY():
	"Can be	overridden to change whether the map wraps around in the Y direction. Default is false"
	return False

def getTopLatitude():
	"Can be	overridden to change whether the the top latitude. Default is 90"
	initClimateSettings()
	cgc = CyGlobalContext()
	map = cgc.getMap()
	iH = map.getGridHeight()
	topLatitudeTemperature = getTileTemperature(0,iH)
	result = 30 - topLatitudeTemperature * 2
	return int(result)

def getBottomLatitude():
	"Can be	overridden to change whether the the bottom	latitude. Default is -90"
	cgc = CyGlobalContext()
	map = cgc.getMap()
	iH = map.getGridHeight()
	topLatitudeTemperature = getTileTemperature(0,iH)
	result = - (30 - topLatitudeTemperature * 2)
	return int(result)

def isBonusIgnoreLatitude():
	"Can be	overridden to determine	whether	bonus generation ignores latitude. Default is false"
	CyPythonMgr().allowDefaultImpl()

def collectGarbage():
	print "Collect python garbage"
	try:
		gc.collect()
	except:
		print "Unexpected error during garbage collection"