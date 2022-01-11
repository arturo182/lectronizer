QT       += core gui network widgets
CONFIG   += c++20

SOURCES += \
    filterbuttondelegate.cpp \
    main.cpp \
    ordersortfiltermodel.cpp \
    structs.cpp \
    utils.cpp \
    widgets/clickylineedit.cpp \
    widgets/filtertreewidget.cpp \
    widgets/mainwindow.cpp \
    widgets/orderdetailswidget.cpp \
    widgets/settingsdialog.cpp

HEADERS += \
    filterbuttondelegate.h \
    ordersortfiltermodel.h \
    shareddata.h \
    structs.h \
    utils.h \
    widgets/clickylineedit.h \
    widgets/filtertreewidget.h \
    widgets/mainwindow.h \
    widgets/orderdetailswidget.h \
    widgets/settingsdialog.h

FORMS += \
    widgets/mainwindow.ui \
    widgets/orderdetailswidget.ui \
    widgets/settingsdialog.ui

RESOURCES += \
    res.qrc

# Window icon
RC_ICONS = res/cart_chip.ico

# MacOS icon
ICON = res/cart_chip.icns

# Version
VERSION = 0.2
VERSION_PARTS = $$split(VERSION, ".")

DEFINES += \
    VER_MINOR="$$member(VERSION_PARTS, 1)" \
    VER_MAJOR="$$member(VERSION_PARTS, 0)"

# Git revision, used by the Github workflow
defined(VER_SHA1, var) {
DEFINES += \
    VER_SHA1=\\\"$$VER_SHA1\\\"
}
