QT += core gui widgets

CONFIG += c++17

TARGET = ArWindow
TEMPLATE = app

SOURCES += main.cpp \
           mainwindow.cpp \
           settingsdialog.cpp

HEADERS += mainwindow.h \
           settingsdialog.h

FORMS += mainwindow.ui \
         settingsdialog.ui

OTHER_FILES += \
    LICENSE \
    README.md

DESTDIR = ./bin
MOC_DIR = ./moc
OBJECTS_DIR = ./obj
RCC_DIR = ./rcc
RC_ICONS = appicon.ico

# Windows平台依赖（新增psapi.lib用于GetModuleFileNameExW）
win32 {
    LIBS += -luser32 -lkernel32 -lshell32 -lpsapi
}

VERSION = 1.0.0
MAKE_TARGET_COMPANY = "Ar Studio"
QMAKE_TARGET_DESCRIPTION = $${TARGET}
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2026 Ar Studio. All Rights Reserved."

DEFINES += QT_DEPRECATED_WARNINGS
