# File generated by kdevelop's qmake manager. 
# ------------------------------------------- 
# Subdir relative project main directory: ./src/framework/tplugins/tgstengine
# Target is a library:  

INSTALLS += target 
target.path = /lib/plugins/ 

CONFIG += warn_on plugin 
TEMPLATE = lib 
TARGET = tgstengine

INCLUDEPATH += ../../ ../../tcore
LIBS += -L../../tcore -ltupifwcore

contains(DEFINES, HAVE_GST10){
    HEADERS += tgstengine.h 
    SOURCES += tgstengine.cpp 
}

linux-g{
    TARGETDEPS += ../../tcore/libtupifwcore.so
}

!include(../../tupconfig.pri){
    error("Please configure first")
}