#!/usr/bin/env python3

import sys
import json

from PyQt5.QtWidgets import QMainWindow, QApplication, QTreeView, QAction
from PyQt5.QtGui import QStandardItemModel, QStandardItem, QKeySequence

class GoalDepTreeModel (QStandardItemModel):
    def __init__ (self, jsonFile):
        super (GoalDepTreeModel, self).__init__()

        with open (jsonFile) as f:
            treeData = json.load(f)
            self.setTreeData (self.invisibleRootItem(), treeData)


    def setTreeData (self, parent, treeData):
        if type (treeData) == list:
            for elem in treeData:
                self.setTreeData (parent, elem)
        elif type (treeData) == dict:
            for elem in treeData:
                newParent = QStandardItem (elem)
                self.setTreeData (newParent, treeData[elem])
                parent.appendRow (newParent)
        else:
            parent.appendRow (QStandardItem (treeData))

    def goalCount (self):
        return self.invisibleRootItem().rowCount()

class GoalDepTreeView (QTreeView):
    def __init__ (self, jsonFile):
        super (GoalDepTreeView, self).__init__()
        treeModel = GoalDepTreeModel (jsonFile)
        self.setModel (treeModel)
        self.setHeaderHidden (True)
        self.show()

class OurMainWindow (QMainWindow):
    def __init__(self):
        super(OurMainWindow, self).__init__()
        self.setWindowTitle ('Goal dependency tree viewer')

        self.menu = self.menuBar()
        self.menu.setNativeMenuBar(False)
        self.fileMenu = self.menu.addMenu ('File')

        exitAction = QAction("Exit", self)
        exitAction.setShortcut(QKeySequence.Quit)
        exitAction.triggered.connect(self.close)

        self.fileMenu.addAction(exitAction)

        self.treeView = GoalDepTreeView (sys.argv[1])
        self.setCentralWidget (self.treeView)
        self.statusBar().showMessage ('Showing {} goal(s)'.format (self.treeView.model().goalCount()))
        self.show()


if __name__ == '__main__':
    app = QApplication(sys.argv)
    mainWindow = OurMainWindow()
    sys.exit (app.exec_())
