//=================================================================================================
/*
    Copyright (C) 2015-2017 Sky kit authors united with omega. <http://omega.gg/about>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of the SkTorrent module of Sky kit.

    - GNU General Public License Usage:
    This file may be used under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation and appearing in the LICENSE.md file included in the packaging
    of this file. Please review the following information to ensure the GNU General Public License
    requirements will be met: https://www.gnu.org/licenses/gpl.html.
*/
//=================================================================================================

#ifndef WCONTROLLERTORRENT_H
#define WCONTROLLERTORRENT_H

// Qt includes
#include <QEvent>
#include <QVariant>
#include <QUrl>
#include <QStringList>
#include <QBitArray>

// Sk includes
#include <WController>

#ifndef SK_NO_CONTROLLERTORRENT

// Forward declarations
class WControllerTorrentPrivate;
class WTorrentEngine;
class WTorrentReply;

// Defines
#define wControllerTorrent WControllerTorrent::instance()

//-------------------------------------------------------------------------------------------------
// WTorrent
//-------------------------------------------------------------------------------------------------

class SK_TORRENT_EXPORT WTorrent : public QObject
{
    Q_OBJECT

    Q_ENUMS(Mode)
    Q_ENUMS(EventType)

public: // Enums
    enum Mode { Default, Sequential, Stream };

    enum EventType
    {
        EventAdd = QEvent::User,
        EventProgress,
        EventBuffer,
        EventFinished,
        EventError
    };

private:
    WTorrent(const QUrl & url, Mode mode, QObject * parent);

protected: // Events
    /* virtual */ bool event(QEvent * event);

public: // Properties
    QUrl url  () const;
    int  index() const;

    Mode mode() const;

    bool isDefault   () const;
    bool isSequential() const;
    bool isStream    () const;

    bool isLoaded() const;

    QStringList paths() const;

    qint64 size() const;

    qint64 progress() const;
    qint64 buffer  () const;

    int download() const;
    int upload  () const;

    int seeds() const;
    int peers() const;

    bool hasError() const;

    QString error() const;

private: // Variables
    QList<WTorrentReply *> _replies;

    QUrl _url;
    int  _index;

    Mode _mode;

    bool _loaded;

    QStringList _paths;

    qint64 _size;

    qint64 _progress;
    qint64 _buffer;

    int _download;
    int _upload;

    int _seeds;
    int _peers;

    QString _error;

private:
    friend class WControllerTorrent;
    friend class WControllerTorrentPrivate;
};

//-------------------------------------------------------------------------------------------------
// WTorrentReply
//-------------------------------------------------------------------------------------------------

class SK_TORRENT_EXPORT WTorrentReply : public QObject
{
    Q_OBJECT

private:
    WTorrentReply(QObject * parent);
public:
    /* virtual */ ~WTorrentReply();

signals:
    void added (WTorrentReply * reply);
    void loaded(WTorrentReply * reply);

    void progress(qint64 bytesReceived, qint64 bytesTotal);
    void buffer  (qint64 bytesReceived);

public: // Properties
    WTorrent * torrent() const;

private: // Variables
    WTorrent * _torrent;

private:
    friend class WControllerTorrent;
    friend class WControllerTorrentPrivate;
    friend class WTorrent;
};

//-------------------------------------------------------------------------------------------------
// WTorrentEvent
//-------------------------------------------------------------------------------------------------

class WTorrentEvent : public QEvent
{
public:
    WTorrentEvent(WTorrent::EventType type) : QEvent(static_cast<QEvent::Type> (type)) {}
};

//-------------------------------------------------------------------------------------------------
// WTorrentEventValue
//-------------------------------------------------------------------------------------------------

class WTorrentEventValue : public WTorrentEvent
{
public:
    WTorrentEventValue(WTorrent::EventType type, const QVariant & value) : WTorrentEvent(type)
    {
        this->value = value;
    }

public: // Variables
    QVariant value;
};

//-------------------------------------------------------------------------------------------------
// WTorrentEventAdd
//-------------------------------------------------------------------------------------------------

class WTorrentEventAdd : public WTorrentEvent
{
public:
    WTorrentEventAdd(const QStringList & paths, qint64 size) : WTorrentEvent(WTorrent::EventAdd)
    {
        this->paths = paths;
        this->size  = size;
    }

public: // Variables
    QStringList paths;
    qint64      size;
};

//-------------------------------------------------------------------------------------------------
// WTorrentEventProgress
//-------------------------------------------------------------------------------------------------

class WTorrentEventProgress : public WTorrentEvent
{
public:
    WTorrentEventProgress(qint64 progress, int download, int upload, int seeds, int peers)
        : WTorrentEvent(WTorrent::EventProgress)
    {
        this->progress = progress;

        this->download = download;
        this->upload   = upload;

        this->seeds = seeds;
        this->peers = peers;
    }

public: // Variables
    qint64 progress;

    int download;
    int upload;

    int seeds;
    int peers;
};

//-------------------------------------------------------------------------------------------------
// WControllerTorrent
//-------------------------------------------------------------------------------------------------

class SK_TORRENT_EXPORT WControllerTorrent : public WController
{
    Q_OBJECT

    Q_ENUMS(Type)

    Q_PROPERTY(WTorrentEngine * engine READ engine CONSTANT)

    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)

public: // Enums
    enum Type
    {
        Null,
        String,
        Integer,
        List,
        Dictionary
    };

private:
    WControllerTorrent();

public: // Interface
    Q_INVOKABLE WTorrentReply * getTorrent(const QUrl     & url,
                                           QObject        * parent = NULL,
                                           WTorrent::Mode   mode   = WTorrent::Default);

    Q_INVOKABLE void clearSource(const QUrl & url);

    Q_INVOKABLE void clearTorrents();

    Q_INVOKABLE int  registerPort  ();
    Q_INVOKABLE void unregisterPort(int port);

public: // Initialize
    /* virtual */ void initController(const QString & path,
                                      qint64          sizeMax = 1048576 * 1000); // 1 gigabyte

public: // Static functions
    //---------------------------------------------------------------------------------------------
    // Bencode

    Q_INVOKABLE static Type extractType(const QString & text, int at = 0);

    Q_INVOKABLE static QString extractString (const QString & text, int at = 0);
    Q_INVOKABLE static int     extractInteger(const QString & text, int at = 0);
    Q_INVOKABLE static QString extractList   (const QString & text, int at = 0);

    Q_INVOKABLE static int indexAfter(const QString & text,
                                      const QString & string, int at = 0);

    Q_INVOKABLE static QString stringAfter(const QString & text,
                                           const QString & string, int at = 0);

    Q_INVOKABLE static int integerAfter(const QString & text,
                                        const QString & string, int at = 0);

    Q_INVOKABLE static QString listAfter(const QString & text,
                                         const QString & string, int at = 0);

    Q_INVOKABLE static int skipString (const QString & text, int at = 0);
    Q_INVOKABLE static int skipInteger(const QString & text, int at = 0);
    Q_INVOKABLE static int skipList   (const QString & text, int at = 0);

    Q_INVOKABLE static QStringList splitList(const QString & text);

    Q_INVOKABLE static Type getType(const QChar & character);

signals:
    void portChanged();

public: // Properties
    WTorrentEngine * engine() const;

    int  port() const;
    void setPort(int port);

private:
    W_DECLARE_PRIVATE   (WControllerTorrent)
    W_DECLARE_CONTROLLER(WControllerTorrent)

    Q_PRIVATE_SLOT(d_func(), void onLoaded(WRemoteData *))

    friend class WTorrentReply;
};

#include <private/WControllerTorrent_p>

#endif // SK_NO_CONTROLLERTORRENT
#endif // WCONTROLLERTORRENT_H
