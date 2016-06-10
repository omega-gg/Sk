SK = $$_PRO_FILE_PWD_/../..

greaterThan(QT_MAJOR_VERSION, 4) {
    SK_BIN = latest
} else {
    SK_BIN = bin
}

CONFIG(debug, debug|release) {
    TARGET = SkTorrentD
} else {
    TARGET = SkTorrent
}

DESTDIR = $$SK/lib

TEMPLATE = lib

QT += network

CONFIG       += plugin
win32:CONFIG += dll

DEFINES += SK_TORRENT_LIBRARY

greaterThan(QT_MAJOR_VERSION, 4): DEFINES += QT_LATEST

include(src/controllers/controllers.pri)
include(src/media/media.pri)
include(src/torrent/torrent.pri)

INCLUDEPATH += \
            $$SK/include/SkCore \
            $$SK/include/SkCore/private \
            $$SK/include/SkGui \
            $$SK/include/SkGui/private \
            $$SK/include/SkTorrent \
            $$SK/include/SkTorrent/private \
            $$SK/include/ \

CONFIG(debug, debug|release) {
    LIBS += \
         -L$$SK/lib -lSkCoreD \
         -L$$SK/lib -lSkGuiD \

} else {
    LIBS += \
         -L$$SK/lib -lSkCore \
         -L$$SK/lib -lSkGui \

}

LIBS += -static -L$$_PRO_FILE_PWD_ -ltorrent \
        -static -L$$_PRO_FILE_PWD_ -lboost_system \
        -static -L$$_PRO_FILE_PWD_ -lboost_random \
        -static -L$$_PRO_FILE_PWD_ -lboost_chrono \

win32:LIBS += -lmswsock -lws2_32

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

win32:equals(QMAKE_COPY, "copy /y") {
    SK ~= s,/,\\,g

    QMAKE_POST_LINK += $${QMAKE_COPY} $$SK\\lib\\$${TARGET}.dll $$SK\\$$SK_BIN
}

win32:equals(QMAKE_COPY, "cp") {
    QMAKE_POST_LINK += $${QMAKE_COPY} $$SK/lib/$${TARGET}.dll $$SK/$$SK_BIN
}

macx:  QMAKE_POST_LINK += $${QMAKE_COPY} $$SK/lib/lib$${TARGET}.dylib $$SK/$$SK_BIN
linux: QMAKE_POST_LINK += $${QMAKE_COPY} $$SK/lib/lib$${TARGET}.so    $$SK/$$SK_BIN
