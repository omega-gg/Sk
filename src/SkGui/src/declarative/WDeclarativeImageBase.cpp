//=================================================================================================
/*
    Copyright (C) 2015-2017 Sky kit authors united with omega. <http://omega.gg/about>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of the SkGui module of Sky kit.

    - GNU General Public License Usage:
    This file may be used under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation and appearing in the LICENSE.md file included in the packaging
    of this file. Please review the following information to ensure the GNU General Public License
    requirements will be met: https://www.gnu.org/licenses/gpl.html.
*/
//=================================================================================================

#include "WDeclarativeImageBase.h"

#ifndef SK_NO_DECLARATIVEIMAGEBASE

#ifdef QT_4
// Qt includes
#include <QImageReader>
#endif

// Sk includes
#include <WControllerView>
#include <WControllerFile>
#ifdef QT_LATEST
#include <WView>
#endif
#include <WCache>
#include <WImageFilter>

#ifdef QT_LATEST
// Private includes
#include <private/qquickwindow_p.h>
#include <private/qsgadaptationlayer_p.h>
#endif

//-------------------------------------------------------------------------------------------------
// Private
//-------------------------------------------------------------------------------------------------

WDeclarativeImageBasePrivate::WDeclarativeImageBasePrivate(WDeclarativeImageBase * p)
    : WDeclarativeItemPrivate(p) {}

#ifdef QT_LATEST

/* virtual */ WDeclarativeImageBasePrivate::~WDeclarativeImageBasePrivate()
{
    if (texture) texture->deleteLater();
}

#endif

//-------------------------------------------------------------------------------------------------

void WDeclarativeImageBasePrivate::init()
{
    Q_Q(WDeclarativeImageBase);

    file = NULL;

#ifdef QT_LATEST
    context = NULL;
    texture = NULL;

    updateTexture  = false;
    updateGeometry = false;
    updateSmooth   = false;
#endif

    status = WDeclarativeImageBase::Null;

    loadMode = static_cast<WDeclarativeImageBase::LoadMode> (wControllerView->loadMode());

    asynchronous = false;

    progress = 0.0;

    filter = NULL;

    sourceDefault = true;

    explicitSize = false;

    loadLater = false;

    cache = true;

    smooth = true;

#ifdef QT_4
    q->setFlag(QGraphicsItem::ItemHasNoContents, false);
#else
    q->setFlag(QQuickItem::ItemHasContents);
#endif
}

//-------------------------------------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------------------------------------

void WDeclarativeImageBasePrivate::loadVisible()
{
    if (loadLater == false) return;

    loadLater = false;

    loadUrl();
}

//-------------------------------------------------------------------------------------------------

void WDeclarativeImageBasePrivate::loadUrl()
{
    Q_Q(WDeclarativeImageBase);

    q->pixmapClear();

    if (url.isEmpty())
    {
        if (file) clearFile();

        pix.clear(q);

        q->clearUrl(WDeclarativeImageBase::Null);

        return;
    }

    if (file)
    {
        QObject::disconnect(file, 0, q, 0);

        file->deleteLater();
    }

    file = wControllerFile->getHttp(url, q);

    if (file)
    {
        if (file->isLoading())
        {
            if (asynchronous == false || pix.pixmap().isNull())
            {
                pix.clear(q);

                applySourceDefault();
            }
            else pix.disconnect(q);

            progress = 0.0;
            status   = WDeclarativeImageBase::Loading;

            QObject::connect(file, SIGNAL(loaded(WCacheFile *)), q, SLOT(onLoaded(WCacheFile *)));

            QObject::connect(file, SIGNAL(progress       (qint64, qint64)),
                             q,    SLOT  (requestProgress(qint64, qint64)));

            emit q->progressChanged();
            emit q->statusChanged  ();
        }
        else
        {
            QUrl url = file->urlCache();

            clearFile();

            q->applyUrl(url, asynchronous);
        }
    }
    else q->applyUrl(url, asynchronous);
}

//-------------------------------------------------------------------------------------------------

void WDeclarativeImageBasePrivate::loadDefault()
{
    Q_Q(WDeclarativeImageBase);

    readDefault();

    q->pixmapChange();

#ifdef QT_LATEST
    updateTexture = true;
#endif

    q->update();
}

void WDeclarativeImageBasePrivate::readDefault()
{
    WPixmapCache::readPixmap(&(pixmapDefault),
                             WControllerFile::toLocalFile(urlDefault), sourceSize, sourceArea);

    applyFilter();
}

//-------------------------------------------------------------------------------------------------

void WDeclarativeImageBasePrivate::reload()
{
    Q_Q(WDeclarativeImageBase);

    if (q->isComponentComplete() == false) return;

    if (loadMode == WDeclarativeImageBase::LoadVisible && q->isVisible() == false)
    {
        pixmapDefault = QPixmap();

        loadLater = true;
    }
    else if (urlDefault.isValid())
    {
        if (sourceDefault == false)
        {
            pixmapDefault = QPixmap();

            if (url.isValid())
            {
                loadUrl();
            }
        }
        else loadDefault();
    }
    else if (url.isValid())
    {
        loadUrl();
    }
}

//-------------------------------------------------------------------------------------------------

void WDeclarativeImageBasePrivate::applyRequest()
{
    Q_Q(WDeclarativeImageBase);

    progress = 1.0;
    status   = WDeclarativeImageBase::Ready;

    setSourceDefault(pix.isNull());

    applyFilter();

    q->pixmapChange();

#ifdef QT_LATEST
    updateTexture = true;
#endif

    q->update();

    if (sourceSize.width() != pix.width() || sourceSize.height() != pix.height())
    {
        emit q->sourceSizeChanged();
    }

    emit q->progressChanged();
    emit q->statusChanged  ();

    emit q->loaded();
}

void WDeclarativeImageBasePrivate::applyFilter()
{
    if (filter == NULL) return;

    if (sourceDefault == false || pixmapDefault.isNull())
    {
        QPixmap pixmap = pix.pixmap();

        filter->applyFilter(&pixmap);

        QString path = pix.path();

        pix.applyPixmap(pixmap, path);
    }
    else filter->applyFilter(&pixmapDefault);
}

//-------------------------------------------------------------------------------------------------

void WDeclarativeImageBasePrivate::setSourceDefault(bool sourceDefault)
{
    if (this->sourceDefault == sourceDefault) return;

    Q_Q(WDeclarativeImageBase);

    this->sourceDefault = sourceDefault;

    emit q->defaultChanged();
}

void WDeclarativeImageBasePrivate::applySourceDefault()
{
    Q_Q(WDeclarativeImageBase);

    if (sourceDefault)
    {
        if (urlDefault.isValid() && pixmapDefault.isNull())
        {
            loadDefault();
        }
    }
    else
    {
        sourceDefault = true;

        if (urlDefault.isValid() && pixmapDefault.isNull())
        {
            readDefault();
        }

        q->pixmapChange();

#ifdef QT_LATEST
        updateTexture = true;
#endif

        q->update();

        emit q->defaultChanged();
    }
}

//-------------------------------------------------------------------------------------------------

void WDeclarativeImageBasePrivate::setExplicitSize(bool explicitSize)
{
    Q_Q(WDeclarativeImageBase);

    this->explicitSize = explicitSize;

    emit q->explicitSizeChanged();
}

//-------------------------------------------------------------------------------------------------

void WDeclarativeImageBasePrivate::clearFile()
{
    Q_Q(WDeclarativeImageBase);

    QObject::disconnect(file, 0, q, 0);

    file->deleteLater();

    file = NULL;
}

//-------------------------------------------------------------------------------------------------

#ifdef QT_LATEST

void WDeclarativeImageBasePrivate::applySmooth(QSGInternalImageNode * node)
{
    updateSmooth = false;

    if (smooth)
    {
        node->setMipmapFiltering(QSGTexture::Linear);
        node->setFiltering      (QSGTexture::Linear);
    }
    else
    {
        node->setMipmapFiltering(QSGTexture::Nearest);
        node->setFiltering      (QSGTexture::Nearest);
    }
}

#endif

//-------------------------------------------------------------------------------------------------
// Private slots
//-------------------------------------------------------------------------------------------------

void WDeclarativeImageBasePrivate::onLoaded(WCacheFile * file)
{
    Q_Q(WDeclarativeImageBase);

    this->file = NULL;

    if (file->hasError())
    {
         q->clearUrl(WDeclarativeImageBase::Error);
    }
    else q->applyUrl(file->urlCache(), true);

    file->deleteLater();
}

//-------------------------------------------------------------------------------------------------

void WDeclarativeImageBasePrivate::onFilterUpdated()
{
    Q_Q(WDeclarativeImageBase);

    if (pix.isNull()) return;

    filter->applyFilter(const_cast<QPixmap *> (&(pix.pixmap())));

    q->pixmapChange();

#ifdef QT_LATEST
    updateTexture = true;
#endif

    q->update();
}

//-------------------------------------------------------------------------------------------------
// Ctor / dtor
//-------------------------------------------------------------------------------------------------

#ifdef QT_4
/* explicit */ WDeclarativeImageBase::WDeclarativeImageBase(QDeclarativeItem * parent)
#else
/* explicit */ WDeclarativeImageBase::WDeclarativeImageBase(QQuickItem * parent)
#endif
    : WDeclarativeItem(new WDeclarativeImageBasePrivate(this), parent)
{
    Q_D(WDeclarativeImageBase); d->init();
}

//-------------------------------------------------------------------------------------------------
// Protected

#ifdef QT_4
WDeclarativeImageBase::WDeclarativeImageBase(WDeclarativeImageBasePrivate * p,
                                             QDeclarativeItem             * parent)
#else
WDeclarativeImageBase::WDeclarativeImageBase(WDeclarativeImageBasePrivate * p, QQuickItem * parent)
#endif
    : WDeclarativeItem(p, parent)
{
    Q_D(WDeclarativeImageBase); d->init();
}

//-------------------------------------------------------------------------------------------------
// Interface
//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE */ void WDeclarativeImageBase::loadSource(const QUrl & url, bool force)
{
    Q_D(WDeclarativeImageBase);

    if (force) d->url = QUrl();

    setSource(url);
}

/* Q_INVOKABLE */ void WDeclarativeImageBase::loadNow(const QUrl & url)
{
    Q_D(WDeclarativeImageBase);

    QUrl source;

    if (url.isValid())
    {
         source = url;
    }
    else source = d->url;

    LoadMode loadMode     = d->loadMode;
    bool     asynchronous = d->asynchronous;

    d->loadMode     = LoadAlways;
    d->asynchronous = false;

    if (d->status == Loading)
    {
        loadSource(source, true);
    }
    else if (d->loadLater)
    {
        d->loadLater = false;

        loadSource(source, true);
    }
    else setSource(source);

    d->loadMode     = loadMode;
    d->asynchronous = asynchronous;
}

//-------------------------------------------------------------------------------------------------

#ifdef QT_4
/* Q_INVOKABLE */ void WDeclarativeImageBase::setItemShot(QGraphicsObject * object)
#else
/* Q_INVOKABLE */ void WDeclarativeImageBase::setItemShot(QQuickItem * item)
#endif
{
#ifdef QT_4
    Q_ASSERT(object);

    QPixmap pixmap = WControllerView::takeItemShot(object, Qt::transparent);
#else
    Q_ASSERT(item);

    QPixmap pixmap = WControllerView::takeItemShot(item, Qt::transparent);
#endif

    setPixmap(pixmap);
}

//-------------------------------------------------------------------------------------------------
// QDeclarativeItem / QQuickItem reimplementation
//-------------------------------------------------------------------------------------------------

/* virtual */ void WDeclarativeImageBase::componentComplete()
{
    Q_D(WDeclarativeImageBase);

    WDeclarativeItem::componentComplete();

    if (d->url.isValid())
    {
        load();
    }
    else if (d->urlDefault.isValid())
    {
        d->loadDefault();
    }
}

//-------------------------------------------------------------------------------------------------

#ifdef QT_LATEST

/* virtual */ QSGNode * WDeclarativeImageBase::updatePaintNode(QSGNode             * oldNode,
                                                               UpdatePaintNodeData *)
{
    Q_D(WDeclarativeImageBase);

    const QPixmap & pixmap = getPixmap();

    if (pixmap.isNull() || width() < 1 || height() < 1)
    {
        if (oldNode)
        {
            delete oldNode;

            delete d->texture;

            d->texture = NULL;
        }

        return NULL;
    }

    QSGInternalImageNode * node;

    if (oldNode)
    {
        node = static_cast<QSGInternalImageNode *> (oldNode);

        if (d->updateTexture)
        {
            if (d->updateSmooth) d->applySmooth(node);

            d->updateTexture  = false;
            d->updateGeometry = false;

            if (d->texture) delete d->texture;

            d->texture = d->view->createTextureFromImage(pixmap.toImage());

            node->setTexture(d->texture);

            applyGeometry(node, pixmap);

            node->update();
        }
        else if (d->updateGeometry)
        {
            if (d->updateSmooth) d->applySmooth(node);

            d->updateGeometry = false;

            applyGeometry(node, pixmap);

            node->update();
        }
        else if (d->updateSmooth)
        {
            d->applySmooth(node);

            node->update();
        }
    }
    else
    {
        node = d->context->createInternalImageNode();

        d->applySmooth(node);

        d->updateTexture  = false;
        d->updateGeometry = false;

        d->texture = d->view->createTextureFromImage(pixmap.toImage());

        node->setTexture(d->texture);

        applyGeometry(node, pixmap);

        node->update();
    }

    return node;
}

#endif

//-------------------------------------------------------------------------------------------------
// Protected Functions
//-------------------------------------------------------------------------------------------------

const QPixmap & WDeclarativeImageBase::currentPixmap() const
{
    Q_D(const WDeclarativeImageBase);

    if (d->sourceDefault)
    {
         return d->pixmapDefault;
    }
    else return d->pix.pixmap();
}

//-------------------------------------------------------------------------------------------------
// Protected virtual functions
//-------------------------------------------------------------------------------------------------

/* virtual */ void WDeclarativeImageBase::load()
{
    Q_D(WDeclarativeImageBase);

    if (d->loadMode == LoadVisible && isVisible() == false)
    {
        d->loadLater = true;
    }
    else d->loadUrl();
}

//-------------------------------------------------------------------------------------------------

/* virtual */ void WDeclarativeImageBase::applyUrl(const QUrl & url, bool asynchronous)
{
    Q_D(WDeclarativeImageBase);

    QPixmap pixmap = d->pix.pixmap();

    if (d->explicitSize)
    {
         d->pix.load(WControllerFile::toString(url), d->sourceSize, d->sourceArea, asynchronous,
                     d->cache, this, SLOT(requestFinished()));
    }
    else d->pix.load(WControllerFile::toString(url), QSize(), QSize(), asynchronous,
                     d->cache, this, SLOT(requestFinished()));

    if (d->pix.isLoading())
    {
        if (asynchronous)
        {
            if (pixmap.isNull())
            {
                d->applySourceDefault();
            }
            else d->pix.changePixmap(pixmap);
        }
        else d->applySourceDefault();

        if (d->progress)
        {
            d->progress = 0.0;

            emit progressChanged();
        }

        if (d->status != Loading)
        {
            d->status = Loading;

            emit statusChanged();
        }
    }
    else requestFinished();
}

/* virtual */ void WDeclarativeImageBase::clearUrl(WDeclarativeImageBase::Status status)
{
    Q_D(WDeclarativeImageBase);

    d->applySourceDefault();

    d->progress = 0.0;
    d->status   = status;

    emit progressChanged();
    emit statusChanged  ();

    emit loaded();
}

//-------------------------------------------------------------------------------------------------

/* virtual */ const QPixmap & WDeclarativeImageBase::getPixmap()
{
    return currentPixmap();
}

//-------------------------------------------------------------------------------------------------

/* virtual */ void WDeclarativeImageBase::pixmapChange()
{
    const QPixmap & pixmap = currentPixmap();

    setImplicitWidth (pixmap.width ());
    setImplicitHeight(pixmap.height());
}

/* virtual */ void WDeclarativeImageBase::pixmapClear() {}

//-------------------------------------------------------------------------------------------------

#ifdef QT_LATEST

/* virtual */ void WDeclarativeImageBase::applyGeometry(QSGInternalImageNode *, const QPixmap &) {}

#endif

//-------------------------------------------------------------------------------------------------
// Protected slots
//-------------------------------------------------------------------------------------------------

/* virtual */ void WDeclarativeImageBase::requestFinished()
{
    Q_D(WDeclarativeImageBase);

    if (d->pix.isNull())
    {
        clearUrl(Error);
    }
    else d->applyRequest();
}

/* virtual */ void WDeclarativeImageBase::requestProgress(qint64 received, qint64 total)
{
    Q_D(WDeclarativeImageBase);

    if (d->status == Loading && total)
    {
        d->progress = (qreal) received / total;

        emit progressChanged();
    }
}

//-------------------------------------------------------------------------------------------------
// Protected QGraphicsItem / QQuickItem reimplementation
//-------------------------------------------------------------------------------------------------

#ifdef QT_4
/* virtual */ QVariant WDeclarativeImageBase::itemChange(GraphicsItemChange change,
                                                         const QVariant &   value)
#else
/* virtual */ void WDeclarativeImageBase::itemChange(ItemChange             change,
                                                     const ItemChangeData & value)
#endif
{
#ifdef QT_4
    if (change == ItemVisibleHasChanged)
    {
        Q_D(WDeclarativeImageBase);

#else
    Q_D(WDeclarativeImageBase);

    if (change == ItemSceneChange)
    {
        QQuickWindow * window = value.window;

        if (window)
        {
            QQuickWindowPrivate * p
                = static_cast<QQuickWindowPrivate *> (QObjectPrivate::get(window));

            d->context = p->context->sceneGraphContext();
        }
        else d->context = NULL;
    }
    else if (d->view && change == ItemVisibleHasChanged && value.boolValue)
    {
#endif
        d->loadVisible();
    }

#ifdef QT_4
    return WDeclarativeItem::itemChange(change, value);
#else
    WDeclarativeItem::itemChange(change, value);
#endif
}

//-------------------------------------------------------------------------------------------------

#ifdef QT_LATEST

/* virtual */ void WDeclarativeImageBase::geometryChanged(const QRectF & newGeometry,
                                                          const QRectF & oldGeometry)
{
    Q_D(WDeclarativeImageBase);

    WDeclarativeItem::geometryChanged(newGeometry, oldGeometry);

    d->updateGeometry = true;
}

#endif

//-------------------------------------------------------------------------------------------------
// Properties
//-------------------------------------------------------------------------------------------------

QPixmap WDeclarativeImageBase::pixmap() const
{
    Q_D(const WDeclarativeImageBase); return d->pix.pixmap();
}

void WDeclarativeImageBase::setPixmap(const QPixmap & pixmap)
{
    Q_D(WDeclarativeImageBase);

    if (d->url.isEmpty() == false)
    {
        d->url = QUrl();

        emit sourceChanged();
    }

    d->pix.setPixmap(pixmap, this);

    d->applyRequest();
}

//-------------------------------------------------------------------------------------------------

WDeclarativeImageBase::Status WDeclarativeImageBase::status() const
{
    Q_D(const WDeclarativeImageBase); return d->status;
}

//-------------------------------------------------------------------------------------------------

bool WDeclarativeImageBase::isNull() const
{
    Q_D(const WDeclarativeImageBase); return (d->status == Null);
}

bool WDeclarativeImageBase::isLoading() const
{
    Q_D(const WDeclarativeImageBase); return (d->status == Loading);
}

bool WDeclarativeImageBase::isReady() const
{
    Q_D(const WDeclarativeImageBase); return (d->status == Ready);
}

//-------------------------------------------------------------------------------------------------

bool WDeclarativeImageBase::isSourceDefault() const
{
    Q_D(const WDeclarativeImageBase); return d->sourceDefault;
}

bool WDeclarativeImageBase::isExplicitSize() const
{
    Q_D(const WDeclarativeImageBase); return d->explicitSize;
}

//-------------------------------------------------------------------------------------------------

QUrl WDeclarativeImageBase::source() const
{
    Q_D(const WDeclarativeImageBase); return d->url;
}

void WDeclarativeImageBase::setSource(const QUrl & url)
{
    Q_D(WDeclarativeImageBase);

    if (d->url.isEmpty() == url.isEmpty() && d->url == url) return;

    d->url = url;

    if (d->loadMode == LoadVisible && isVisible() == false)
    {
        if (url.isValid())
        {
             d->setSourceDefault(false);
        }
        else d->setSourceDefault(true);
    }

    if (isComponentComplete()) load();

    emit sourceChanged();
}

//-------------------------------------------------------------------------------------------------

QUrl WDeclarativeImageBase::sourceDefault() const
{
    Q_D(const WDeclarativeImageBase); return d->urlDefault;
}

void WDeclarativeImageBase::setSourceDefault(const QUrl & url)
{
    Q_D(WDeclarativeImageBase);

    if (d->urlDefault.isEmpty() == url.isEmpty() && d->urlDefault == url) return;

    d->urlDefault = url;

    if (isComponentComplete())
    {
        if (d->loadMode == LoadVisible && isVisible() == false)
        {
            d->pixmapDefault = QPixmap();

            d->loadLater = true;
        }
        else if (d->sourceDefault)
        {
            if (url.isValid())
            {
                d->readDefault();
            }
            else d->pixmapDefault = QPixmap();

            pixmapChange();

#ifdef QT_LATEST
            d->updateTexture = true;
#endif

            update();
        }
        else d->pixmapDefault = QPixmap();
    }

    emit sourceDefaultChanged();
}

//-------------------------------------------------------------------------------------------------

QSize WDeclarativeImageBase::sourceSize() const
{
    Q_D(const WDeclarativeImageBase);

    if (d->explicitSize)
    {
         return d->sourceSize;
    }
    else return QSize(implicitWidth(), implicitHeight());
}

void WDeclarativeImageBase::setSourceSize(const QSize & size)
{
    Q_D(WDeclarativeImageBase);

    if (d->sourceSize == size) return;

    d->sourceSize = size;

    d->setExplicitSize(d->sourceSize.isValid());

    d->reload();

    emit sourceSizeChanged();
}

void WDeclarativeImageBase::resetSourceSize()
{
    Q_D(WDeclarativeImageBase);

    if (d->explicitSize == false) return;

    d->sourceSize = QSize();

    d->setExplicitSize(false);

    d->reload();

    emit sourceSizeChanged();
}

//-------------------------------------------------------------------------------------------------

QSize WDeclarativeImageBase::sourceArea() const
{
    Q_D(const WDeclarativeImageBase); return d->sourceArea;
}

void WDeclarativeImageBase::setSourceArea(const QSize & size)
{
    Q_D(WDeclarativeImageBase);

    if (d->sourceArea == size) return;

    d->sourceArea = size;

    d->reload();

    emit sourceAreaChanged();
}

void WDeclarativeImageBase::resetSourceArea()
{
    Q_D(WDeclarativeImageBase);

    if (d->sourceArea.isValid() == false) return;

    d->sourceArea = QSize();

    d->reload();

    emit sourceAreaChanged();
}

//-------------------------------------------------------------------------------------------------

WDeclarativeImageBase::LoadMode WDeclarativeImageBase::loadMode() const
{
    Q_D(const WDeclarativeImageBase); return d->loadMode;
}

void WDeclarativeImageBase::setLoadMode(LoadMode mode)
{
    Q_D(WDeclarativeImageBase);

    if (d->loadMode == mode) return;

    d->loadMode = mode;

    if (mode != LoadVisible)
    {
        d->loadVisible();
    }

    emit loadModeChanged();
}

//-------------------------------------------------------------------------------------------------

bool WDeclarativeImageBase::asynchronous() const
{
    Q_D(const WDeclarativeImageBase); return d->asynchronous;
}

void WDeclarativeImageBase::setAsynchronous(bool enabled)
{
    Q_D(WDeclarativeImageBase);

    if (d->asynchronous == enabled) return;

    d->asynchronous = enabled;

    if (enabled && d->status == Loading)
    {
        load();
    }

    emit asynchronousChanged();
}

//-------------------------------------------------------------------------------------------------

bool WDeclarativeImageBase::cache() const
{
    Q_D(const WDeclarativeImageBase); return d->cache;
}

void WDeclarativeImageBase::setCache(bool enabled)
{
    Q_D(WDeclarativeImageBase);

    if (d->cache == enabled) return;

    d->cache = enabled;

    if (isComponentComplete()) load();

    emit cacheChanged();
}

//-------------------------------------------------------------------------------------------------

qreal WDeclarativeImageBase::progress() const
{
    Q_D(const WDeclarativeImageBase); return d->progress;
}

//-------------------------------------------------------------------------------------------------

WImageFilter * WDeclarativeImageBase::filter() const
{
    Q_D(const WDeclarativeImageBase); return d->filter;
}

void WDeclarativeImageBase::setFilter(WImageFilter * filter)
{
    Q_D(WDeclarativeImageBase);

    if (d->filter == filter) return;

    if (d->filter)
    {
        disconnect(d->filter, 0, this, 0);
    }

    d->filter = filter;

    if (filter)
    {
        connect(filter, SIGNAL(filterUpdated()), this, SLOT(onFilterUpdated()));

        d->applyFilter();

        pixmapChange();

#ifdef QT_LATEST
        d->updateTexture = true;
#endif

        update();
    }

    emit filterChanged();
}

//-------------------------------------------------------------------------------------------------

bool WDeclarativeImageBase::smooth() const
{
    Q_D(const WDeclarativeImageBase); return d->smooth;
}

void WDeclarativeImageBase::setSmooth(bool smooth)
{
    Q_D(WDeclarativeImageBase);

    if (d->smooth == smooth) return;

    d->smooth = smooth;

#ifdef QT_LATEST
    d->updateSmooth = true;
#endif

    update();

    emit smoothChanged();
}

//-------------------------------------------------------------------------------------------------

qreal WDeclarativeImageBase::ratioWidth()  const
{
    Q_D(const WDeclarativeImageBase);

    if (d->pix.isNull()) return 1.0;

    const QImage & image = d->pix.pixmap().toImage();

    return (qreal) image.width() / image.height();
}

qreal WDeclarativeImageBase::ratioHeight() const
{
    Q_D(const WDeclarativeImageBase);

    if (d->pix.isNull()) return 1.0;

    const QImage & image = d->pix.pixmap().toImage();

    return (qreal) image.height() / image.width();
}

#endif // SK_NO_DECLARATIVEIMAGEBASE
