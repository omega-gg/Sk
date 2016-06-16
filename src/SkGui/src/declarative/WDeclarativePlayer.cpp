//=================================================================================================
/*
    Copyright (C) 2015-2016 Sky kit authors united with omega. <http://omega.gg/about>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of the SkGui module of Sky kit.

    - GNU General Public License Usage:
    This file may be used under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation and appearing in the LICENSE.md file included in the packaging
    of this file. Please review the following information to ensure the GNU General Public License
    requirements will be met: https://www.gnu.org/licenses/gpl.html.
*/
//=================================================================================================

#include "WDeclarativePlayer.h"

// Qt includes
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>

// Sk includes
#include <WControllerApplication>
#include <WControllerView>
#include <WControllerPlaylist>
#include <WAbstractPlaylist>
#include <WAbstractHook>
#include <WPlaylistNet>
#include <WTabsTrack>
#include <WTabTrack>
#include <WLibraryFolder>

#ifndef SK_NO_DECLARATIVEPLAYER

//-------------------------------------------------------------------------------------------------
// Private
//-------------------------------------------------------------------------------------------------

WDeclarativePlayerPrivate::WDeclarativePlayerPrivate(WDeclarativePlayer * p)
    : WDeclarativeItemPrivate(p) {}

/* virtual */ WDeclarativePlayerPrivate::~WDeclarativePlayerPrivate()
{
    if (backend) backend->deleteBackend();
}

//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::init()
{
    Q_Q(WDeclarativePlayer);

    backend  = NULL;
    folder   = NULL;
    playlist = NULL;

    tabs = NULL;
    tab  = NULL;

    state = WAbstractBackend::StateStopped;

    speed = 1.0;

    volume = 100;

    shuffle = false;

    shuffleIndex = -1;
    shuffleLock  = false;

    repeat = WDeclarativePlayer::RepeatNone;

    quality = WAbstractBackend::QualityMedium;

    fillMode = WAbstractBackend::PreserveAspectFit;

    keepState = false;

    q->setFlag(QGraphicsItem::ItemHasNoContents, false);
}

//-------------------------------------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::setPlaylist(WPlaylistNet * playlist)
{
    if (this->playlist == playlist) return;

    Q_Q(WDeclarativePlayer);

    if (folder)
    {
        folder->setActiveId(-1);

        QObject::disconnect(folder, SIGNAL(destroyed()), q, SLOT(onFolderDestroyed()));
    }

    if (shuffle && this->playlist)
    {
        this->playlist->unregisterWatcher(q);
    }

    this->playlist = playlist;

    if (playlist)
    {
        folder = playlist->parentFolder();

        if (folder)
        {
            folder->setActiveId(playlist->id());
        }

        if (shuffle) playlist->registerWatcher(q);

        QObject::connect(folder, SIGNAL(destroyed()), q, SLOT(onFolderDestroyed()));
    }
    else folder = NULL;

    if (shuffle) resetShuffle();

    emit q->playlistChanged();
}

//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::setTab(WTabTrack * tab)
{
    if (this->tab == tab) return;

    Q_Q(WDeclarativePlayer);

    int count = q->count();

    if (this->tab)
    {
        this->tab->setPlayer(NULL);

        this->tab->setStackEnabled(false);

        QObject::disconnect(this->tab, 0, q, 0);
    }

    this->tab = tab;

    if (tab)
    {
        QObject::connect(tab, SIGNAL(countChanged()), q, SIGNAL(countChanged()));

        QObject::connect(tab, SIGNAL(playlistUpdated()), q, SIGNAL(playlistUpdated()));

        QObject::connect(tab, SIGNAL(currentBookmarkChanged()),
                         q,   SLOT(onCurrentBookmarkChanged()));

        QObject::connect(tab, SIGNAL(currentBookmarkUpdated()),
                         q,   SLOT(onCurrentBookmarkUpdated()));

        QObject::connect(tab, SIGNAL(destroyed   ()),
                         q,   SLOT(onTabDestroyed()));

        tab->setPlayer(q);

        if (q->hasStarted())
        {
             tab->setStackEnabled(true);
        }
        else tab->setStackEnabled(false);

        onCurrentBookmarkChanged();
        onCurrentBookmarkUpdated();
    }

    if (q->count() != count)
    {
        emit q->countChanged();
    }

    emit q->playlistUpdated();

    emit q->tabChanged();
}

//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::loadSource(const QUrl & source, int duration, int currentTime)
{
    Q_Q(WDeclarativePlayer);

    if (backend)
    {
        backend->loadSource(source, duration, currentTime);
    }

    if (shuffle && shuffleLock == false)
    {
        resetShuffle();

        emit q->playlistUpdated();
    }

    emit q->sourceChanged();
}

//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::updateRepeat()
{
    if (repeat == WDeclarativePlayer::RepeatOne
        ||
        (repeat == WDeclarativePlayer::RepeatAll && playlist == NULL))
    {
         backend->setRepeat(true);
    }
    else backend->setRepeat(false);
}

//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::resetShuffle()
{
    Q_Q(WDeclarativePlayer);

    shuffleHistory.clear();

    if (q->count() > 1)
    {
        int index = q->trackIndex();

        const WAbstractTrack * track = playlist->trackPointerAt(index);

        if (track)
        {
            shuffleTracks = playlist->trackPointers();

            shuffleTracks.removeOne(track);

            shuffleHistory.append(track);

            shuffleIndex = 0;

            return;
        }
    }

    shuffleTracks.clear();

    shuffleIndex = -1;
}

void WDeclarativePlayerPrivate::clearShuffle()
{
    shuffleHistory.clear();
    shuffleTracks .clear();

    shuffleIndex = -1;
}

//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::setShuffleTrack(const WAbstractTrack * track)
{
    shuffleLock = true;

    if (tab)
    {
         tab->setCurrentTrackPointer(track);
    }
    else playlist->setCurrentTrackPointer(track);

    shuffleLock = false;
}

//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::clearPlaylistAndTabs()
{
    Q_Q(WDeclarativePlayer);

    if (tabs)
    {
        QObject::disconnect(tabs, 0, q, 0);
        QObject::disconnect(tab,  0, q, 0);

        tabs = NULL;
        tab  = NULL;
    }

    if (playlist)
    {
        QObject::disconnect(playlist, 0, q, 0);

        playlist = NULL;
    }
}

//-------------------------------------------------------------------------------------------------
// Private slots
//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::onEnded()
{
    Q_Q(WDeclarativePlayer);

    if (repeat != WDeclarativePlayer::RepeatOne)
    {
        if (tab) tab->setCurrentTime(-1);

        if (q->hasNextTrack() == false)
        {
            if (playlist && repeat == WDeclarativePlayer::RepeatAll)
            {
                playlist->setCurrentIndex(0);

                backend->replay();
            }
            else q->stop();
        }
        else q->setNextTrack();
    }
    else backend->replay();
}

//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::onStateChanged()
{
    if (keepState) return;

    Q_Q(WDeclarativePlayer);

    WAbstractBackend::State state = backend->state();

    if (tab)
    {
        if (state == WAbstractBackend::StatePlaying)
        {
            tab->setStackEnabled(true);
        }
        else if (state == WAbstractBackend::StatePaused)
        {
            if (tabs->highlightedTab())
            {
                tab->setPlayer(NULL);

                backend->stop();

                return;
            }
        }
        else if (state == WAbstractBackend::StateStopped)
        {
            tab->setStackEnabled(false);

            tabs->setHighlightedTab(NULL);
        }
    }

    if (this->state != state)
    {
        this->state = state;

        emit q->stateChanged();
    }
}

void WDeclarativePlayerPrivate::onDurationChanged()
{
    if (tab) tab->setDuration(backend->duration());
}

//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::onCurrentTrackChanged()
{
    Q_Q(WDeclarativePlayer);

    const WAbstractTrack * track = playlist->currentTrackPointer();

    loadSource(track->source(), track->duration(), -1);

    emit q->currentTrackUpdated();
}

//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::onPlaylistDestroyed()
{
    Q_Q(WDeclarativePlayer);

    playlist = NULL;

    emit q->playlistChanged();
}

void WDeclarativePlayerPrivate::onFolderDestroyed()
{
    folder = NULL;
}

//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::onCurrentTabChanged()
{
    Q_Q(WDeclarativePlayer);

    WTabTrack * currentTab = tabs->currentTabTrack();

    if (q->isPlaying())
    {
        if (tabs->highlightedTab())
        {
            if (tab == currentTab)
            {
                tabs->setHighlightedTab(NULL);
            }
        }
        else if (currentTab->currentTime() == -1)
        {
            tabs->setHighlightedTab(tab);
        }
        else setTab(currentTab);
    }
    else setTab(currentTab);
}

void WDeclarativePlayerPrivate::onHighlightedTabChanged()
{
    WTabTrack * tab = tabs->highlightedTab();

    if (tab)
    {
         setTab(tab);
    }
    else setTab(tabs->currentTabTrack());
}

//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::onCurrentBookmarkChanged()
{
    const WBookmarkTrack * bookmark = tab->currentBookmark();

    if (bookmark)
    {
        loadSource(bookmark->source(), bookmark->duration(), bookmark->currentTime());
    }
    else if (backend)
    {
        backend->setSource(QUrl());
    }
}

void WDeclarativePlayerPrivate::onCurrentBookmarkUpdated()
{
    Q_Q(WDeclarativePlayer);

    const WBookmarkTrack * bookmark = tab->currentBookmark();

    if (bookmark)
    {
        WAbstractPlaylist * playlist = bookmark->playlist();

        if (playlist)
        {
             setPlaylist(playlist->toPlaylistNet());
        }
        else setPlaylist(NULL);
    }
    else setPlaylist(NULL);

    emit q->currentTrackUpdated();
}

//-------------------------------------------------------------------------------------------------

void WDeclarativePlayerPrivate::onTabsDestroyed()
{
    Q_Q(WDeclarativePlayer); q->setTabs(NULL);
}

void WDeclarativePlayerPrivate::onTabDestroyed()
{
    WAbstractTab * currentTab = tabs->currentTab();

    if (currentTab)
    {
        WTabTrack * tabTrack = currentTab->toTabTrack();

        tab = tabTrack;

        if (backend) backend->stop();

        tab = NULL;

        setTab(tabTrack);
    }
    else
    {
        tab = NULL;

        if (backend) backend->stop();
    }
}

//-------------------------------------------------------------------------------------------------
// Ctor / dtor
//-------------------------------------------------------------------------------------------------

/* explicit */ WDeclarativePlayer::WDeclarativePlayer(QDeclarativeItem * parent)
    : WDeclarativeItem(new WDeclarativePlayerPrivate(this), parent)
{
    Q_D(WDeclarativePlayer); d->init();
}

//-------------------------------------------------------------------------------------------------
// Interface
//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE */ void WDeclarativePlayer::play()
{
    Q_D(WDeclarativePlayer);

    if (d->backend == NULL) return;

    if (d->tab)
    {
        d->tab->setPlayer(this);

        if (d->backend->isStopped())
        {
            d->backend->seekTo(d->tab->currentTime());
        }
    }

    d->backend->play();
}

/* Q_INVOKABLE */ void WDeclarativePlayer::replay()
{
    Q_D(WDeclarativePlayer);

    if (d->backend == NULL) return;

    if (d->tab) d->tab->setPlayer(this);

    d->backend->replay();
}

/* Q_INVOKABLE */ void WDeclarativePlayer::pause()
{
    Q_D(WDeclarativePlayer);

    if (d->backend) d->backend->pause();
}

/* Q_INVOKABLE */ void WDeclarativePlayer::stop()
{
    Q_D(WDeclarativePlayer);

    if (d->backend == NULL) return;

    if (d->tab) d->tab->setPlayer(NULL);

    d->backend->stop();
}

/* Q_INVOKABLE */ void WDeclarativePlayer::togglePlay()
{
    if (isPlaying()) pause();
    else             play ();
}

/* Q_INVOKABLE */ void WDeclarativePlayer::seekTo(int msec)
{
    Q_D(WDeclarativePlayer);

    if (d->backend) d->backend->seekTo(msec);
}

//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE */ void WDeclarativePlayer::setPreviousTrack()
{
    Q_D(WDeclarativePlayer);

    if (d->shuffle)
    {
        if (d->shuffleIndex == -1) return;

        const WAbstractTrack * track;

        if (d->shuffleTracks.isEmpty() && d->shuffleIndex == 0
            &&
            d->repeat == WDeclarativePlayer::RepeatAll)
        {
            d->shuffleIndex = d->shuffleHistory.count() - 1;

            track = d->shuffleHistory.at(d->shuffleIndex);

            d->setShuffleTrack(track);

            return;
        }

        if (d->shuffleIndex != 0)
        {
            d->shuffleIndex--;

            track = d->shuffleHistory.at(d->shuffleIndex);
        }
        else
        {
            int count = d->shuffleTracks.count();

            if (count == 0) return;

            int index = qrand() % count;

            track = d->shuffleTracks.takeAt(index);

            d->shuffleHistory.prepend(track);
        }

        d->setShuffleTrack(track);
    }
    else if (d->tab)
    {
        if (d->repeat == WDeclarativePlayer::RepeatNone)
        {
             d->tab->setPreviousTrack(false);
        }
        else d->tab->setPreviousTrack(true);
    }
    else if (d->playlist)
    {
        if (d->repeat == WDeclarativePlayer::RepeatNone)
        {
             d->playlist->setPreviousTrack(false);
        }
        else d->playlist->setPreviousTrack(true);
    }
}

/* Q_INVOKABLE */ void WDeclarativePlayer::setNextTrack()
{
    Q_D(WDeclarativePlayer);

    if (d->shuffle)
    {
        if (d->shuffleIndex == -1) return;

        const WAbstractTrack * track;

        int last = d->shuffleHistory.count() - 1;

        if (d->shuffleTracks.isEmpty() && d->shuffleIndex == last
            &&
            d->repeat == WDeclarativePlayer::RepeatAll)
        {
            d->shuffleIndex = 0;

            track = d->shuffleHistory.at(d->shuffleIndex);

            d->setShuffleTrack(track);

            return;
        }

        if (d->shuffleIndex != last)
        {
            d->shuffleIndex++;

            track = d->shuffleHistory.at(d->shuffleIndex);
        }
        else
        {
            int count = d->shuffleTracks.count();

            if (count == 0) return;

            int index = qrand() % count;

            track = d->shuffleTracks.takeAt(index);

            d->shuffleHistory.append(track);

            d->shuffleIndex++;
        }

        d->setShuffleTrack(track);
    }
    else if (d->tab)
    {
        if (d->repeat == WDeclarativePlayer::RepeatNone)
        {
             d->tab->setNextTrack(false);
        }
        else d->tab->setNextTrack(true);
    }
    else if (d->playlist)
    {
        if (d->repeat == WDeclarativePlayer::RepeatNone)
        {
             d->playlist->setNextTrack(false);
        }
        else d->playlist->setNextTrack(true);
    }
}

//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE */ QImage WDeclarativePlayer::getFrame() const
{
    Q_D(const WDeclarativePlayer);

    if (d->backend)
    {
        d->backend->updateFrame();

        return d->backend->getFrame();
    }
    else return QImage();
}

//-------------------------------------------------------------------------------------------------
// QGraphicsItem reimplementation
//-------------------------------------------------------------------------------------------------

/* virtual */ void WDeclarativePlayer::paint(QPainter                       * painter,
                                             const QStyleOptionGraphicsItem * option, QWidget *)
{
    Q_D(WDeclarativePlayer);

    if (d->backend)
    {
        d->backend->drawFrame(painter, option);
    }
}

//-------------------------------------------------------------------------------------------------
// Protected QDeclarativeItem reimplementation
//-------------------------------------------------------------------------------------------------

/* virtual */ void WDeclarativePlayer::geometryChanged(const QRectF & newGeometry,
                                                       const QRectF & oldGeometry)
{
    Q_D(WDeclarativePlayer);

    WDeclarativeItem::geometryChanged(newGeometry, oldGeometry);

    if (d->backend)
    {
        d->backend->setSize(newGeometry.size());
    }
}

//-------------------------------------------------------------------------------------------------
// Protected WAbstractPlaylistWatcher implementation
//-------------------------------------------------------------------------------------------------

/* virtual */ void WDeclarativePlayer::beginTracksInsert(int first, int last)
{
    Q_D(WDeclarativePlayer);

    if (d->shuffle == false) return;

    for (int i = first; i <= last; i++)
    {
        const WAbstractTrack * track = d->playlist->trackPointerAt(i);

        d->shuffleTracks.append(track);
    }
}

/* virtual */ void WDeclarativePlayer::endTracksInsert()
{
    Q_D(WDeclarativePlayer);

    if (d->shuffle && d->shuffleIndex == -1)
    {
        d->resetShuffle();

        if (d->shuffleIndex == 0)
        {
            emit playlistUpdated();
        }
    }
}

//-------------------------------------------------------------------------------------------------

/* virtual */ void WDeclarativePlayer::beginTracksRemove(int first, int last)
{
    Q_D(WDeclarativePlayer);

    if (d->shuffle == false) return;

    for (int i = first; i <= last; i++)
    {
        const WAbstractTrack * track = d->playlist->trackPointerAt(i);

        if (d->shuffleIndex == i)
        {
            d->shuffleIndex = -1;
        }
        else if (d->shuffleIndex > i)
        {
            d->shuffleIndex--;
        }

        d->shuffleTracks .removeOne(track);
        d->shuffleHistory.removeOne(track);
    }
}

/* virtual */ void WDeclarativePlayer::beginTracksClear()
{
    Q_D(WDeclarativePlayer);

    if (d->shuffle == false) return;

    d->shuffleIndex = -1;

    d->shuffleTracks .clear();
    d->shuffleHistory.clear();
}

//-------------------------------------------------------------------------------------------------
// Properties
//-------------------------------------------------------------------------------------------------

WAbstractBackend * WDeclarativePlayer::backend() const
{
    Q_D(const WDeclarativePlayer); return d->backend;
}

void WDeclarativePlayer::setBackend(WAbstractBackend * backend)
{
    Q_D(WDeclarativePlayer);

    if (d->backend == backend) return;

    if (d->backend)
    {
        QObject::disconnect(d->backend, 0, this, 0);

        d->backend->deleteBackend();
    }

    d->backend = backend;

    d->backend->setParent(this);

    d->backend->setParentItem(this);

    if (backend)
    {
        backend->setSize(QSizeF(width(), height()));

        d->updateRepeat();

        backend->setSpeed(d->speed);

        backend->setVolume(d->volume);

        backend->setQuality(d->quality);

        backend->setFillMode(d->fillMode);

        QObject::connect(backend, SIGNAL(sourceChanged()),
                         this,    SIGNAL(sourceChanged()));

        QObject::connect(backend, SIGNAL(stateChanged()),
                         this,    SIGNAL(stateChanged()));

        QObject::connect(backend, SIGNAL(stateLoadChanged()),
                         this,    SIGNAL(stateLoadChanged()));

        QObject::connect(backend, SIGNAL(startedChanged()),
                         this,    SIGNAL(startedChanged()));

        QObject::connect(backend, SIGNAL(currentTimeChanged()),
                         this,    SIGNAL(currentTimeChanged()));

        QObject::connect(backend, SIGNAL(durationChanged()),
                         this,    SIGNAL(durationChanged()));

        QObject::connect(backend, SIGNAL(qualityActiveChanged()),
                         this,    SIGNAL(qualityActiveChanged()));

        QObject::connect(backend, SIGNAL(ended()), this, SLOT(onEnded()));

        QObject::connect(backend, SIGNAL(stateChanged   ()), this, SLOT(onStateChanged   ()));
        QObject::connect(backend, SIGNAL(durationChanged()), this, SLOT(onDurationChanged()));
    }

    emit backendChanged();
}

//-------------------------------------------------------------------------------------------------

WAbstractHook * WDeclarativePlayer::hook() const
{
    Q_D(const WDeclarativePlayer); return d->hook;
}

void WDeclarativePlayer::setHook(WAbstractHook * hook)
{
    Q_D(WDeclarativePlayer);

    if (d->hook == hook) return;

    if (d->hook) d->hook->deleteBackend();

    d->hook = hook;

    d->hook->setParent(this);

    d->hook->setParentItem(this);

    d->hook->setBackend(d->backend);

    emit hookChanged();
}

//-------------------------------------------------------------------------------------------------

WAbstractBackend::State WDeclarativePlayer::state() const
{
    Q_D(const WDeclarativePlayer); return d->state;
}

//-------------------------------------------------------------------------------------------------

QUrl WDeclarativePlayer::source() const
{
    Q_D(const WDeclarativePlayer);

    if (d->backend) return d->backend->source();
    else            return QUrl();
}

void WDeclarativePlayer::setSource(const QUrl & url)
{
    Q_D(WDeclarativePlayer);

    if (d->backend && d->backend->source() == url) return;

    d->clearPlaylistAndTabs();

    d->loadSource(url, -1, -1);
}

//-------------------------------------------------------------------------------------------------

WPlaylistNet * WDeclarativePlayer::playlist() const
{
    Q_D(const WDeclarativePlayer); return d->playlist;
}

void WDeclarativePlayer::setPlaylist(WPlaylistNet * playlist)
{
    Q_D(WDeclarativePlayer);

    if (d->playlist == playlist) return;

    int count = this->count();

    d->clearPlaylistAndTabs();

    if (playlist)
    {
        if (playlist->parent() == NULL) playlist->setParent(this);

        connect(playlist, SIGNAL(countChanged()), this, SIGNAL(countChanged()));

        connect(playlist, SIGNAL(playlistUpdated()), this, SIGNAL(playlistUpdated()));

        connect(playlist, SIGNAL(currentTrackUpdated()), this, SIGNAL(currentTrackUpdated()));

        connect(playlist, SIGNAL(currentTrackChanged()), this, SLOT(onCurrentTrackChanged()));

        connect(playlist, SIGNAL(destroyed()), this, SLOT(onPlaylistDestroyed()));
    }

    d->setPlaylist(playlist);

    if (this->count() != count)
    {
        emit countChanged();
    }

    emit playlistUpdated();
}

//-------------------------------------------------------------------------------------------------

bool WDeclarativePlayer::hasStarted() const
{
    Q_D(const WDeclarativePlayer);

    if (d->backend) return d->backend->hasStarted();
    else            return false;
}

//-------------------------------------------------------------------------------------------------

bool WDeclarativePlayer::isLoading() const
{
    Q_D(const WDeclarativePlayer);

    if (d->backend) return d->backend->isLoading();
    else            return false;
}

//-------------------------------------------------------------------------------------------------

bool WDeclarativePlayer::isStarting() const
{
    Q_D(const WDeclarativePlayer);

    if (d->backend) return d->backend->isStarting();
    else            return false;
}

bool WDeclarativePlayer::isResuming() const
{
    Q_D(const WDeclarativePlayer);

    if (d->backend) return d->backend->isResuming();
    else            return false;
}

bool WDeclarativePlayer::isBuffering() const
{
    Q_D(const WDeclarativePlayer);

    if (d->backend) return d->backend->isBuffering();
    else            return false;
}

//-------------------------------------------------------------------------------------------------

bool WDeclarativePlayer::isPlaying() const
{
    Q_D(const WDeclarativePlayer); return (d->state == WAbstractBackend::StatePlaying);
}

bool WDeclarativePlayer::isPaused() const
{
    Q_D(const WDeclarativePlayer); return (d->state == WAbstractBackend::StatePaused);
}

bool WDeclarativePlayer::isStopped() const
{
    Q_D(const WDeclarativePlayer); return (d->state == WAbstractBackend::StateStopped);
}

//-------------------------------------------------------------------------------------------------

int WDeclarativePlayer::currentTime() const
{
    Q_D(const WDeclarativePlayer);

    if (d->backend) return d->backend->currentTime();
    else            return -1;
}

int WDeclarativePlayer::duration() const
{
    Q_D(const WDeclarativePlayer);

    if (d->backend) return d->backend->duration();
    else            return -1;
}

//-------------------------------------------------------------------------------------------------

qreal WDeclarativePlayer::speed() const
{
    Q_D(const WDeclarativePlayer); return d->speed;
}

void WDeclarativePlayer::setSpeed(qreal speed)
{
    Q_D(WDeclarativePlayer);

    if (d->speed == speed) return;

    d->speed = speed;

    if (d->backend) d->backend->setSpeed(speed);

    emit speedChanged();
}

//-------------------------------------------------------------------------------------------------

int WDeclarativePlayer::volume() const
{
    Q_D(const WDeclarativePlayer); return d->volume;
}

void WDeclarativePlayer::setVolume(int percent)
{
    Q_D(WDeclarativePlayer);

    if (d->volume == percent) return;

    d->volume = percent;

    if (d->backend) d->backend->setVolume(percent);

    emit volumeChanged();
}

//-------------------------------------------------------------------------------------------------

bool WDeclarativePlayer::shuffle() const
{
    Q_D(const WDeclarativePlayer); return d->shuffle;
}

void WDeclarativePlayer::setShuffle(bool shuffle)
{
    Q_D(WDeclarativePlayer);

    if (d->shuffle == shuffle) return;

    d->shuffle = shuffle;

    if (shuffle) d->resetShuffle();
    else         d->clearShuffle();

    emit shuffleChanged();

    emit playlistUpdated();
}

//-------------------------------------------------------------------------------------------------

WDeclarativePlayer::Repeat WDeclarativePlayer::repeat() const
{
    Q_D(const WDeclarativePlayer); return d->repeat;
}

void WDeclarativePlayer::setRepeat(Repeat repeat)
{
    Q_D(WDeclarativePlayer);

    if (d->repeat == repeat) return;

    d->repeat = repeat;

    if (d->backend) d->updateRepeat();

    emit repeatChanged();

    emit playlistUpdated();
}

//-------------------------------------------------------------------------------------------------

WAbstractBackend::Quality WDeclarativePlayer::quality() const
{
    Q_D(const WDeclarativePlayer); return d->quality;
}

void WDeclarativePlayer::setQuality(WAbstractBackend::Quality quality)
{
    Q_D(WDeclarativePlayer);

    if (d->quality == quality) return;

    d->quality = quality;

    if (d->backend) d->backend->setQuality(quality);

    emit qualityChanged();
}

//-------------------------------------------------------------------------------------------------

WAbstractBackend::Quality WDeclarativePlayer::qualityActive() const
{
    Q_D(const WDeclarativePlayer);

    if (d->backend)
    {
         return d->backend->qualityActive();
    }
    else return WAbstractBackend::QualityInvalid;
}

//-------------------------------------------------------------------------------------------------

WAbstractBackend::FillMode WDeclarativePlayer::fillMode() const
{
    Q_D(const WDeclarativePlayer); return d->fillMode;
}

void WDeclarativePlayer::setFillMode(WAbstractBackend::FillMode fillMode)
{
    Q_D(WDeclarativePlayer);

    if (d->fillMode == fillMode) return;

    d->fillMode = fillMode;

    if (d->backend)
    {
        d->backend->setFillMode(fillMode);
    }

    update();

    emit fillModeChanged();
}

//-------------------------------------------------------------------------------------------------

int WDeclarativePlayer::count() const
{
    Q_D(const WDeclarativePlayer);

    if (d->playlist) return d->playlist->count();
    else             return -1;
}

//-------------------------------------------------------------------------------------------------

bool WDeclarativePlayer::hasPreviousTrack() const
{
    Q_D(const WDeclarativePlayer);

    if (d->repeat == WDeclarativePlayer::RepeatAll && count() > 1)
    {
        return true;
    }
    else if (d->shuffle)
    {
        if (d->shuffleIndex > 0)
        {
             return true;
        }
        else return false;
    }
    else if (d->tab)
    {
        return d->tab->hasPreviousTrack();
    }
    else if (d->playlist)
    {
        return d->playlist->hasPreviousTrack();
    }
    else return false;
}

bool WDeclarativePlayer::hasNextTrack() const
{
    Q_D(const WDeclarativePlayer);

    if (d->repeat == WDeclarativePlayer::RepeatAll && count() > 1)
    {
        return true;
    }
    else if (d->shuffle)
    {
        if (d->shuffleIndex == -1
            ||
            (d->shuffleTracks.isEmpty() && d->shuffleIndex == d->shuffleHistory.count() - 1))
        {
             return false;
        }
        else return true;
    }
    else if (d->tab)
    {
        return d->tab->hasNextTrack();
    }
    else if (d->playlist)
    {
        return d->playlist->hasNextTrack();
    }
    else return false;
}

//-------------------------------------------------------------------------------------------------

WAbstractTrack::State WDeclarativePlayer::trackState() const
{
    Q_D(const WDeclarativePlayer);

    if (d->playlist)
    {
        const WTrackNet * track
                        = static_cast<const WTrackNet *> (d->playlist->currentTrackPointer());

        if (track)
        {
             return track->state();
        }
        else return WAbstractTrack::Default;
    }
    else return WAbstractTrack::Default;
}

bool WDeclarativePlayer::trackIsDefault() const
{
    return (trackState() == WAbstractTrack::Default);
}

bool WDeclarativePlayer::trackIsLoading() const
{
    return (trackState() == WAbstractTrack::Loading);
}

bool WDeclarativePlayer::trackIsLoaded() const
{
    return (trackState() == WAbstractTrack::Loaded);
}

//-------------------------------------------------------------------------------------------------

QString WDeclarativePlayer::trackTitle() const
{
    Q_D(const WDeclarativePlayer);

    if (d->tab)
    {
        return d->tab->title();
    }
    else if (d->playlist)
    {
        return d->playlist->currentTitle();
    }
    else return QString();
}

QUrl WDeclarativePlayer::trackCover() const
{
    Q_D(const WDeclarativePlayer);

    if (d->tab)
    {
        return d->tab->cover();
    }
    else if (d->playlist)
    {
        return d->playlist->currentCover();
    }
    else return QUrl();
}

//-------------------------------------------------------------------------------------------------

int WDeclarativePlayer::trackCurrentTime() const
{
    Q_D(const WDeclarativePlayer);

    if (d->tab) return d->tab->currentTime();
    else        return -1;
}

int WDeclarativePlayer::trackDuration() const
{
    Q_D(const WDeclarativePlayer);

    if (d->tab)
    {
        return d->tab->duration();
    }
    else if (d->playlist)
    {
        return d->playlist->currentDuration();
    }
    else return -1;
}

//-------------------------------------------------------------------------------------------------

int WDeclarativePlayer::trackIndex() const
{
    Q_D(const WDeclarativePlayer);

    if (d->tab)
    {
        return d->tab->trackIndex();
    }
    else if (d->playlist)
    {
        return d->playlist->currentIndex();
    }
    else return -1;
}

//-------------------------------------------------------------------------------------------------

WTabsTrack * WDeclarativePlayer::tabs() const
{
    Q_D(const WDeclarativePlayer); return d->tabs;
}

void WDeclarativePlayer::setTabs(WTabsTrack * tabs)
{
    Q_D(WDeclarativePlayer);

    if (d->tabs == tabs) return;

    d->clearPlaylistAndTabs();

    d->tabs = tabs;

    if (d->tabs)
    {
        QObject::connect(d->tabs, SIGNAL(tabsUpdated()), this, SIGNAL(tabsUpdated()));

        QObject::connect(d->tabs, SIGNAL(currentTabChanged()),
                         this,    SLOT(onCurrentTabChanged()));

        QObject::connect(d->tabs, SIGNAL(highlightedTabChanged()),
                         this,    SLOT(onHighlightedTabChanged()));

        QObject::connect(d->tabs, SIGNAL(destroyed    ()),
                         this,    SLOT(onTabsDestroyed()));
    }

    emit tabsChanged();

    if (d->tabs) d->onCurrentTabChanged();
}

//-------------------------------------------------------------------------------------------------

WTabTrack * WDeclarativePlayer::tab() const
{
    Q_D(const WDeclarativePlayer); return d->tab;
}

int WDeclarativePlayer::tabIndex() const
{
    Q_D(const WDeclarativePlayer);

    if (d->tabs)
    {
        if (d->tabs->highlightedTab())
        {
             return d->tabs->highlightedIndex();
        }
        else return d->tabs->currentIndex();
    }
    else return -1;
}

//-------------------------------------------------------------------------------------------------

bool WDeclarativePlayer::keepState() const
{
    Q_D(const WDeclarativePlayer); return d->keepState;
}

void WDeclarativePlayer::setKeepState(bool keepState)
{
    Q_D(WDeclarativePlayer);

    if (d->keepState == keepState) return;

    d->keepState = keepState;

    if (d->backend)
    {
        if (keepState)
        {
             QObject::disconnect(d->backend, SIGNAL(startedChanged()),
                                 this,       SIGNAL(startedChanged()));
        }
        else QObject::connect(d->backend, SIGNAL(startedChanged()),
                              this,       SIGNAL(startedChanged()));
    }

    emit keepStateChanged();
}

#endif // SK_NO_DECLARATIVEPLAYER
