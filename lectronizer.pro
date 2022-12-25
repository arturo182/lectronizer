QT       += charts core gui network widgets sql
CONFIG   += c++20

SOURCES += \
    filterbuttondelegate.cpp \
    main.cpp \
    orderitemdelegate.cpp \
    ordermanager.cpp \
    ordersortfiltermodel.cpp \
    sqlmanager.cpp \
    structs.cpp \
    utils.cpp \
    widgets/bulkexporterdialog.cpp \
    widgets/clickylineedit.cpp \
    widgets/collapsiblewidgetheader.cpp \
    widgets/currencyfetchdialog.cpp \
    widgets/filtertreewidget.cpp \
    widgets/mainwindow.cpp \
    widgets/markshippeddialog.cpp \
    widgets/orderdetailswidget.cpp \
    widgets/packaginghelperdialog.cpp \
    widgets/settingsdialog.cpp \
    widgets/settingspages/generalsettingspage.cpp \
    widgets/settingspages/ordersettingspage.cpp \
    widgets/settingspages/packagingsettingspage.cpp \
    widgets/statisticsdialog.cpp

HEADERS += \
    enums.h \
    filterbuttondelegate.h \
    orderitemdelegate.h \
    ordermanager.h \
    ordersortfiltermodel.h \
    shareddata.h \
    sqlmanager.h \
    structs.h \
    utils.h \
    widgets/bulkexporterdialog.h \
    widgets/clickylineedit.h \
    widgets/collapsiblewidgetheader.h \
    widgets/currencyfetchdialog.h \
    widgets/filtertreewidget.h \
    widgets/mainwindow.h \
    widgets/markshippeddialog.h \
    widgets/orderdetailswidget.h \
    widgets/packaginghelperdialog.h \
    widgets/settingsdialog.h \
    widgets/settingspages/generalsettingspage.h \
    widgets/settingspages/ordersettingspage.h \
    widgets/settingspages/packagingsettingspage.h \
    widgets/statisticsdialog.h

FORMS += \
    widgets/bulkexporterdialog.ui \
    widgets/mainwindow.ui \
    widgets/markshippeddialog.ui \
    widgets/orderdetailswidget.ui \
    widgets/packaginghelperdialog.ui \
    widgets/settingsdialog.ui \
    widgets/settingspages/generalsettingspage.ui \
    widgets/settingspages/ordersettingspage.ui \
    widgets/settingspages/packagingsettingspage.ui \
    widgets/statisticsdialog.ui

RESOURCES += \
    res.qrc

# App details
QMAKE_TARGET_COMPANY = arturo182
QMAKE_TARGET_DESCRIPTION = Lectronizer

# Window icon
RC_ICONS = res/cart_chip.ico

# MacOS icon
ICON = res/cart_chip.icns

# Version
VERSION = 0.6.0
VERSION_PARTS = $$split(VERSION, ".")

DEFINES += \
    VER_PATCH="$$member(VERSION_PARTS, 2)" \
    VER_MINOR="$$member(VERSION_PARTS, 1)" \
    VER_MAJOR="$$member(VERSION_PARTS, 0)"

# Git revision, used by the Github workflow
defined(VER_SHA1, var) {
DEFINES += \
    VER_SHA1=\\\"$$VER_SHA1\\\"
}
