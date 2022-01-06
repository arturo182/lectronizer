QT       += core gui network widgets
CONFIG   += c++20

SOURCES += \
    clickylineedit.cpp \
    filterbuttondelegate.cpp \
    filtertreewidget.cpp \
    main.cpp \
    mainwindow.cpp \
    orderdetailswidget.cpp \
    ordersortfiltermodel.cpp \
    settingsdialog.cpp \
    structs.cpp \
    utils.cpp

HEADERS += \
    clickylineedit.h \
    filterbuttondelegate.h \
    filtertreewidget.h \
    mainwindow.h \
    orderdetailswidget.h \
    ordersortfiltermodel.h \
    settingsdialog.h \
    shareddata.h \
    structs.h \
    utils.h

FORMS += \
    mainwindow.ui \
    orderdetailswidget.ui \
    settingsdialog.ui

RESOURCES += \
    res.qrc

# Window icon
RC_ICONS = res/cart_chip.ico

# Version
VERSION = 0.1
VERSION_PARTS = $$split(VERSION, ".")

DEFINES += \
    VER_MINOR="$$member(VERSION_PARTS, 1)" \
    VER_MAJOR="$$member(VERSION_PARTS, 0)"

# Git revision, used by the Github workflow
defined(VER_SHA1, var) {
DEFINES += \
    VER_SHA1=\\\"$$VER_SHA1\\\"
}
