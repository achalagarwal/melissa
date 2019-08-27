from time import sleep
from threading import Thread

import ipywidgets as widgets
from IPython.display import display

from .main import MelissaMonitoring
from .internal.plots import CoreUsageDashPlot, JobStatusesDashPlot
from .utils import HiddenPrints


class MelissaDash:

    def __init__(self, study_options, melissa_stats, user_functions):
        self.monitoring = MelissaMonitoring(study_options, melissa_stats, user_functions)
        self.__monitoringThread = None

        self.__tabbedPlots = [CoreUsageDashPlot(self.monitoring), JobStatusesDashPlot(self.monitoring)]
        self.__plots = None
        self.__menu = None
        self.__studyInfo = None
        self.__failedParamInfo = None
        self.__footerOutput = None
        self.app = None
        self.__init()

    def __init(self):
        self.__initPlots()
        self.__initMenu()
        self.__initStudyInfo()
        #self.__initFailedParamInfo()
        self.__initFooter()
        self.__initApp()

    def __initPlots(self):
        tabNames = [x.name for x in self.__tabbedPlots]
        children = [x.widget for x in self.__tabbedPlots]
        tab = widgets.Tab()
        tab.children = children
        for index, title in enumerate(tabNames):
            tab.set_title(index,title)

        self.__plots = tab

    def __initMenu(self):
        menu = widgets.ToggleButtons(
            options=['Plots', 'Failed parameters', 'Study information'],
            value = 'Plots',
            description='',
            disabled=False,
            button_style='',
        )
        menu.observe(self.__changeView, 'value')
        serverStatus = self.monitoring._createServerStatusWidget()
        self.__menu = widgets.VBox([menu, serverStatus])

        # FIXME: Fix swapping between views
    def __changeView(self, change):
        newView = change['new']
        if newView == 'Plots':
            self.app.center = self.__plots
            #self.__displayPlots()
        elif newView == 'Failed parameters':
            pass
        elif newView == 'Study information':
            self.app.center = self.__studyInfo

    def __initStudyInfo(self):
        self.__studyInfo = widgets.Tab()
        tabNames = ['Remaining job time', 'CPU job usage', 'Server status & failed parameters']
        children = [self.monitoring._createRemainingJobsTimeWidget(), self.monitoring._createJobsCPUCountWidget(), self.monitoring._createFailedParametersWidget()]
        self.__studyInfo.children = children
        for index, title in enumerate(tabNames):
            self.__studyInfo.set_title(index,title)

    def __initFailedParamInfo(self):
        raise NotImplementedError

    def __initFooter(self):
        self.__footerOutput = widgets.Output()

    def __initApp(self):
        header = widgets.HTML("<h1>Melissa Dashboard</h1><hr/>", layout=widgets.Layout(height='auto'))
        header.style.text_align='center'

        self.app = widgets.AppLayout(header=header,
                                    left_sidebar=self.__menu,
                                    center=self.__plots,
                                    right_sidebar=None,
                                    footer=self.__footerOutput)

    def __displayApp(self):
        display(self.app)
        self.__displayPlots()
    
    def __displayPlots(self):
        for plot in self.__tabbedPlots:
            with plot.widget:
                display(plot.figure)

    def start(self, refreshRate:float = 1):
        with HiddenPrints():
            self.__displayApp()
            self.monitoring.startStudyInThread()
            self.__monitoringThread = Thread(target=self.__runMonitoringThread, args=(refreshRate,))
            self.__monitoringThread.start()
        
    # TODO: Check for possible threading problems
    # Notebook kernel busy after finished study
    def __runMonitoringThread(self, refreshRate):
        with HiddenPrints():
            self.monitoring.showServerStatus()
            self.monitoring.waitForInitialization()
            while self.monitoring.isStudyRunning():
                for plot in self.__tabbedPlots:
                    plot.plot()
                self.monitoring.showServerStatus()
                self.monitoring.showFailedParameters()
                self.monitoring.showRemainingJobsTime()
                self.monitoring.showJobsCPUCount()
                sleep(refreshRate)