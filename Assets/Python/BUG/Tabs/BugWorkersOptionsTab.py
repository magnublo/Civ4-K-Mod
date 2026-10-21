## BugWorkersOptionsTab (K-Mod-Magnus, written by Claude)
##
## The "Workers" tab of the BUG options: which improvements your automated workers build, and which they replace
## where they find them. The options come from Contrib/AutomatedWorkers.py, one per improvement in the XML.

import BugOptionsTab
import AutomatedWorkers

class BugWorkersOptionsTab(BugOptionsTab.BugOptionsTab):
	"BUG Workers Options Screen Tab"

	def __init__(self, screen):
		BugOptionsTab.BugOptionsTab.__init__(self, "Workers", "Workers")

	def create(self, screen):
		tab = self.createTab(screen)
		panel = self.createMainPanel(screen)
		column = self.addOneColumnLayout(screen, panel)

		self.addLabel(screen, column, "AutomatedWorkersNote", "For the workers you automate. The AI's workers and the orders you give a worker yourself are not affected.")
		screen.attachHSeparator(column, column + "Sep1")

		left, right = self.addTwoColumnLayout(screen, column, "AutomatedWorkers", True)
		self.addLabel(screen, left, "AutomatedWorkersBuild", "They build:")
		for id in AutomatedWorkers.getBuildOptionIDs():
			self.addCheckbox(screen, left, id)
		self.addLabel(screen, right, "AutomatedWorkersReplace", "They replace, with the best other improvement:")
		for id in AutomatedWorkers.getReplaceOptionIDs():
			self.addCheckbox(screen, right, id)
