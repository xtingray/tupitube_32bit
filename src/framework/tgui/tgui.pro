# File generated by kdevelop's qmake manager. 
# ------------------------------------------- 
# Subdir relative project main directory: ./src/framework/tgui
# Target is a library: tupifwgui 

!include(../tupconfig.pri) {
    error("Run ./configure first!")
}

INSTALLS += target
target.path = /lib/

contains(DEFINES, ADD_HEADERS) {
    INSTALLS += headers 
    headers.files += *.h
    headers.path = /include/tupigui
}

macx {
    CONFIG += plugin warn_on
}

HEADERS += taction.h \
           tactionmanager.h \
           tanimwidget.h \
           tapplication.h \
           tbuttonbar.h \
           tcellview.h \
           tcirclebutton.h \
           tcirclebuttonbar.h \
           tclicklineedit.h \
           tcollapsiblewidget.h \
           tcolorbutton.h \
           tcommandhistory.h \
           tconfigurationdialog.h \
           tcontrolnode.h \
           tdoublecombobox.h \
           tdualcolorbutton.h \
           teditspinbox.h \
           tflatbutton.h \
           tfontchooser.h \
           tformfactory.h \
           tformvalidator.h \
           ticon.h \
           tideality.h \
           timagebutton.h \
           titemselector.h \
           tmainwindow.h \
           tmainwindowabstractsettings.h \
           tmainwindowfactory.h \
           tmoviegenerator.h \
           tmoviegeneratorinterface.h \
           tnodegroup.h \
           tseparator.h \
           toptionaldialog.h \
           tosd.h \
           tpathhelper.h \
           tpushbutton.h \
           tradiobuttongroup.h \
           trulerbase.h \
           tstackedmainwindow.h \
           tstylecombobox.h \
           tabbedmainwindow.h \
           tabdialog.h \
           ttabwidget.h \
           themedocument.h \
           thememanager.h \
           tipdialog.h \
           ttoolbox.h \
           toolview.h \
           treelistwidget.h \
           treewidgetsearchline.h \
           tvhbox.h \
           tviewbutton.h \
           twaitstyle.h \
           twidgetlistview.h \
           twizard.h \
           # texportwizard.h \
           tworkspacemainwindow.h \
           txyspinbox.h \
           tcolorarrow.xpm \
           tcolorreset.xpm

SOURCES += taction.cpp \
           tactionmanager.cpp \
           tanimwidget.cpp \
           tapplication.cpp \
           tbuttonbar.cpp \
           tcellview.cpp \
           tcirclebutton.cpp \
           tcirclebuttonbar.cpp \
           tclicklineedit.cpp \
           tcolorbutton.cpp \
           tcollapsiblewidget.cpp \
           tcommandhistory.cpp \
           tconfigurationdialog.cpp \
           tcontrolnode.cpp \
           tdoublecombobox.cpp \
           tdualcolorbutton.cpp \
           teditspinbox.cpp \
           tflatbutton.cpp \
           tfontchooser.cpp \
           tformfactory.cpp \
           tformvalidator.cpp \
           ticon.cpp \ 
           timagebutton.cpp \
           titemselector.cpp \
           tmainwindow.cpp \
           tmainwindowfactory.cpp \
           tmoviegenerator.cpp \
           tnodegroup.cpp \
           tseparator.cpp \
           toptionaldialog.cpp \
           tosd.cpp \
           tpathhelper.cpp \
           tpushbutton.cpp \
           tradiobuttongroup.cpp \
           trulerbase.cpp \
           tstackedmainwindow.cpp \
           tstylecombobox.cpp \
           tabbedmainwindow.cpp \
           tabdialog.cpp \
           ttabwidget.cpp \
           themedocument.cpp \
           thememanager.cpp \
           tipdialog.cpp \
           ttoolbox.cpp \
           toolview.cpp \
           treelistwidget.cpp \
           treewidgetsearchline.cpp \
           tvhbox.cpp \
           tviewbutton.cpp \
           twaitstyle.cpp \
           twidgetlistview.cpp \
           twizard.cpp \
           # texportwizard.cpp \
           tworkspacemainwindow.cpp \
           txyspinbox.cpp

*:!macx{
    CONFIG += warn_on dll
}

TEMPLATE = lib
TARGET = tupifwgui
QT += xml opengl

INCLUDEPATH += ../tcore ../ ../../libbase

RESOURCES += tgui_images.qrc

linux-g{
    TARGETDEPS += ../tcore/libtupifwcore.so
}

# include(../../../tupiglobal.pri)

LIBS += -L../tcore -ltupifwcore

macx {
    LIBS += -lavcodec -lavformat -lavutil 
}
