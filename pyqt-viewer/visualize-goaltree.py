#!/usr/bin/env python3

import sys
import json

from PyQt5.QtWidgets import QMainWindow, QApplication, QTreeView
from PyQt5.QtGui import QStandardItemModel, QStandardItem

class OurTreeModel (QStandardItemModel):
    def __init__ (self):
        super (OurTreeModel, self).__init__()

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

class OurTreeView (QTreeView):
    def __init__ (self, jsonFile):
        super (OurTreeView, self).__init__()
        treeModel = OurTreeModel ()

        with open (jsonFile) as f:
            treeData = json.load(f)
            treeModel.setTreeData (treeModel.invisibleRootItem(), treeData)

        self.setModel (treeModel)
        self.show()

class OurMainWindow (QMainWindow):
    def __init__(self):
        super(OurMainWindow, self).__init__()
        self.treeView = OurTreeView (sys.argv[1])
        self.setCentralWidget (self.treeView)
        self.show()


if __name__ == '__main__':
    app = QApplication(sys.argv)
    mainWindow = OurMainWindow()
    sys.exit (app.exec_())
