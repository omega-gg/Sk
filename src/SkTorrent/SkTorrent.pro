SK = $$_PRO_FILE_PWD_/../..

contains(QT_MAJOR_VERSION, 4) {
    SK_BIN = bin
} else {
    SK_BIN = latest
}

CONFIG(debug, debug|release) {
    TARGET = SkTorrentD
} else {
    TARGET = SkTorrent
}

DESTDIR = $$SK/lib

TEMPLATE = lib

contains(QT_MAJOR_VERSION, 4) {
    QT += declarative network
} else {
    QT += quick network
}

CONFIG       += plugin
win32:CONFIG += dll

DEFINES += SK_TORRENT_LIBRARY

contains(QT_MAJOR_VERSION, 4) {
    DEFINES += QT_4
} else {
    DEFINES += QT_LATEST
}

QMAKE_CXXFLAGS += -std=c++11

unix:QMAKE_LFLAGS += "-Wl,-rpath,'\$$ORIGIN'"

include(src/controllers/controllers.pri)
include(src/io/io.pri)
include(src/media/media.pri)
include(src/torrent/torrent.pri)

INCLUDEPATH += $$SK/include/SkCore \
               $$SK/include/SkCore/private \
               $$SK/include/SkGui \
               $$SK/include/SkGui/private \
               $$SK/include/SkTorrent \
               $$SK/include/SkTorrent/private \
               $$SK/include \

CONFIG(debug, debug|release) {

    LIBS += -L$$SK/lib -lSkCoreD \
            -L$$SK/lib -lSkGuiD
} else {
    LIBS += -L$$SK/lib -lSkCore \
            -L$$SK/lib -lSkGui
}

win32:LIBS += -L$$SK/lib -ltorrent \
              -L$$SK/lib -lboost_system \
              -L$$SK/lib -lboost_random \
              -L$$SK/lib -lboost_chrono \
              -lmswsock -lws2_32 \

unix:LIBS += -ltorrent-rasterbar \
             -lboost_system -lboost_random -lboost_chrono \

macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12

    QMAKE_CXXFLAGS += -stdlib=libc++
}

macx {
CONFIG(debug, debug|release) {
    QMAKE_POST_LINK  = install_name_tool -change libSkCoreD.dylib \
                       @loader_path/libSkCoreD.dylib $${DESTDIR}/lib$${TARGET}.dylib ;

    QMAKE_POST_LINK += install_name_tool -change libSkGuiD.dylib \
                       @loader_path/libSkGuiD.dylib $${DESTDIR}/lib$${TARGET}.dylib ;
} else {
    QMAKE_POST_LINK  = install_name_tool -change libSkCore.dylib \
                       @loader_path/libSkCore.dylib $${DESTDIR}/lib$${TARGET}.dylib ;

    QMAKE_POST_LINK += install_name_tool -change libSkGui.dylib \
                       @loader_path/libSkGui.dylib $${DESTDIR}/lib$${TARGET}.dylib ;
}
}

#--------------------------------------------------------------------------------------------------
# Copy library to the bin directory

win32 {
equals(QMAKE_COPY, "copy /y") {
    SK ~= s,/,\\,g

    QMAKE_POST_LINK += $${QMAKE_COPY} $$SK\\lib\\$${TARGET}.dll $$SK\\$$SK_BIN
} else {
    QMAKE_POST_LINK += $${QMAKE_COPY} $$SK/lib/$${TARGET}.dll $$SK/$$SK_BIN
}
}

macx: QMAKE_POST_LINK += $${QMAKE_COPY} $$SK/lib/lib$${TARGET}.dylib $$SK/$$SK_BIN

unix:!macx {
    QMAKE_POST_LINK += $${QMAKE_COPY} $$SK/lib/lib$${TARGET}.so $$SK/$$SK_BIN
}
