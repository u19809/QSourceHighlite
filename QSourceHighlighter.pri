QT += gui

HEADERS += $$PWD/qsourcehighlighter.h \
           $$PWD/languages/language_others.json.autosave \
           $$PWD/languagedata.h

SOURCES += $$PWD/qsourcehighlighter.cpp \
    $$PWD/languagedata.cpp

RESOURCES += $$PWD/qsourcehighlighterlanguages.qrc

DISTFILES += $$files( $$PWD/languages/*.json)

INCLUDEPATH += $$PWD
