QT += gui

HEADERS += $$PWD/qsourcehighliter.h \
           $$PWD/languages/language_others.json.autosave \
           $$PWD/qsourcehighliterthemes.h \
           $$PWD/languagedata.h

SOURCES += $$PWD/qsourcehighliter.cpp \
    $$PWD/languagedata.cpp \
    $$PWD/qsourcehighliterthemes.cpp

RESOURCES += $$PWD/qsourcehighliterlanguages.qrc

DISTFILES += $$files( $$PWD/languages/*.json)

INCLUDEPATH += $$PWD
