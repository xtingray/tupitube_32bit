# File generated by kdevelop's qmake manager. 
# ------------------------------------------- 
# Subdir relative project main directory: ./src/plugins/tools/common.pro
# Target is a library:  

INSTALLS += headers target 
target.path = /lib/ 

headers.target = .
headers.commands = cp *.h $(INSTALL_ROOT)/include/plugincommon
headers.path = /include/plugincommon

HEADERS += buttonspanel.h \
           tweenmanager.h \
           stepsviewer.h \
           spinboxdelegate.h \
           target.h
SOURCES += buttonspanel.cpp \
           tweenmanager.cpp \
           stepsviewer.cpp \
           spinboxdelegate.cpp \
           target.cpp

CONFIG += dll warn_on
TEMPLATE = lib
TARGET = plugincommon 

FRAMEWORK_DIR = "../../../framework"
include($$FRAMEWORK_DIR/framework.pri)
include(./tools_config.pri)

include(../../../../tupiglobal.pri)