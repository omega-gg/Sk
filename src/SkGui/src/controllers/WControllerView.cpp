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

#include "WControllerView.h"

#ifndef SK_NO_CONTROLLERVIEW

// Qt includes
#include <QApplication>
#include <QDesktopWidget>
#ifdef QT_4
#include <QGraphicsObject>
#else
#include <QQuickItem>
#include <QQuickItemGrabResult>
#endif
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QDir>

// Private includes
#include "WControllerView_p.h"

W_INIT_CONTROLLER(WControllerView)

#ifdef QT_LATEST

//=================================================================================================
// WControllerViewLoader
//=================================================================================================

class SK_GUI_EXPORT WControllerViewLoader : public QObject
{
    Q_OBJECT

public:
    WControllerViewLoader();

private slots:
    void onReady();

public: // Properties
    bool ready;
};

//-------------------------------------------------------------------------------------------------
// Ctor / dtor
//-------------------------------------------------------------------------------------------------

WControllerViewLoader::WControllerViewLoader() : QObject()
{
    ready = false;
}

//-------------------------------------------------------------------------------------------------
// Private slots
//-------------------------------------------------------------------------------------------------

void WControllerViewLoader::onReady()
{
    ready = true;
}

#endif

//=================================================================================================
// WControllerViewPrivate
//=================================================================================================

WControllerViewPrivate::WControllerViewPrivate(WControllerView * p) : WControllerPrivate(p) {}

/* virtual */ WControllerViewPrivate::~WControllerViewPrivate()
{
    W_CLEAR_CONTROLLER(WControllerView);
}

//-------------------------------------------------------------------------------------------------

void WControllerViewPrivate::init()
{
    opengl = true;

    loadMode = WControllerView::LoadAlways;

    scaleDelay = 220;
}

//-------------------------------------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------------------------------------

void WControllerViewPrivate::registerView(WView * view)
{
    views.append(view);
}

void WControllerViewPrivate::unregisterView(WView * view)
{
    views.removeOne(view);
}

#ifdef QT_4

//-------------------------------------------------------------------------------------------------
// Private static functions
//-------------------------------------------------------------------------------------------------

/* static */ void WControllerViewPrivate::paintRecursive(QPainter        * painter,
                                                         QGraphicsObject * item)
{
    QList<QGraphicsObject *> childs;

    foreach (QGraphicsItem * item, item->childItems())
    {
        QGraphicsObject * child = item->toGraphicsObject();

        if (child)
        {
            if (child->zValue() < 0)
            {
                paintChild(painter, child);
            }
            else childs.append(child);
        }
    }

    QStyleOptionGraphicsItem style;

    style.rect = item->boundingRect().toRect();

    style.exposedRect = style.rect;

    item->paint(painter, &style);

    foreach (QGraphicsObject * child, childs)
    {
        paintChild(painter, child);
    }
}

/* static */ void WControllerViewPrivate::paintChild(QPainter * painter, QGraphicsObject * item)
{
    if (item->isVisible() == false) return;

    painter->save();

    painter->translate(item->x(), item->y());

    qreal rotation = item->rotation();

    if (rotation)
    {
        QPointF origin = item->transformOriginPoint();

        painter->translate(origin);

        painter->rotate(rotation);

        painter->translate(-origin);
    }

    if (item->flags() & QGraphicsItem::ItemClipsChildrenToShape)
    {
        painter->setClipRect(item->boundingRect(), Qt::IntersectClip);
    }

    painter->setOpacity(item->effectiveOpacity());

    paintRecursive(painter, item);

    painter->restore();
}

#endif

//=================================================================================================
// WControllerView
//=================================================================================================
// Private

WControllerView::WControllerView() : WController(new WControllerViewPrivate(this)) {}

//-------------------------------------------------------------------------------------------------
// Initialize
//-------------------------------------------------------------------------------------------------

/* virtual */ void WControllerView::init()
{
    Q_D(WControllerView); d->init();
}

//-------------------------------------------------------------------------------------------------
// Static functions
//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE static */ int WControllerView::screenNumber(const QPoint & pos)
{
    return qApp->desktop()->screenNumber(pos);
}

#ifdef QT_4
/* Q_INVOKABLE static */ int WControllerView::screenNumber(const QWidget * widget)
{
    return qApp->desktop()->screenNumber(widget);
}
#endif

//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE static */ const QRect WControllerView::availableGeometry(int screen)
{
    return qApp->desktop()->availableGeometry(screen);
}

/* Q_INVOKABLE static */ const QRect WControllerView::availableGeometry(const QPoint & pos)
{
    return qApp->desktop()->availableGeometry(pos);
}

#ifdef QT_4
/* Q_INVOKABLE static */ const QRect WControllerView::availableGeometry(const QWidget * widget)
{
    return qApp->desktop()->availableGeometry(widget);
}
#endif

//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE static */ const QRect WControllerView::screenGeometry(int screen)
{
    return qApp->desktop()->screenGeometry(screen);
}

/* Q_INVOKABLE static */ const QRect WControllerView::screenGeometry(const QPoint & pos)
{
    return qApp->desktop()->screenGeometry(pos);
}

#ifdef QT_4
/* Q_INVOKABLE static */ const QRect WControllerView::screenGeometry(const QWidget * widget)
{
    return qApp->desktop()->screenGeometry(widget);
}
#endif

//-------------------------------------------------------------------------------------------------

#ifdef QT_4
/* Q_INVOKABLE static */ QPixmap WControllerView::takeItemShot(QGraphicsObject * item,
                                                               const QColor    & background)
#else
/* Q_INVOKABLE static */ QPixmap WControllerView::takeItemShot(QQuickItem   * item,
                                                               const QColor & background)
#endif
{
    Q_ASSERT(item);

    QSize size = item->boundingRect().size().toSize();

    if (size.isValid() == false) return QPixmap();

    QPixmap pixmap(size);

    pixmap.fill(background);

#ifdef QT_LATEST
    QSharedPointer<QQuickItemGrabResult> grab = item->grabToImage();

    WControllerViewLoader loader;

    connect(grab.data(), SIGNAL(ready()), &loader, SLOT(onReady()));

    while (loader.ready == false)
    {
        QCoreApplication::processEvents();
    }
#endif

    QPainter painter(&pixmap);

#ifdef QT_4
    WControllerViewPrivate::paintRecursive(&painter, item);
#else
    painter.drawImage(item->boundingRect().toRect(), grab->image());
#endif

    painter.end();

    return pixmap;
}

#ifdef QT_4
/* Q_INVOKABLE static */ bool WControllerView::saveItemShot(const QString   & fileName,
                                                            QGraphicsObject * item,
                                                            const QColor    & background)
#else
/* Q_INVOKABLE static */ bool WControllerView::saveItemShot(const QString & fileName,
                                                            QQuickItem    * item,
                                                            const QColor  & background)
#endif
{
    QImage image = takeItemShot(item, background).toImage();

    return image.save(fileName, "png");
}

//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE static */ QImage WControllerView::desaturate(const QImage & image)
{
    QImage result(image.width(), image.height(), image.format());

    for (int y = 0; y < image.height(); y++)
    {
        const QRgb * lineA = (QRgb *) image.scanLine(y);

        QRgb * lineB = (QRgb *) result.scanLine(y);

        for (int x = 0; x < image.width(); x++)
        {
            const QRgb & color = *lineA;

            int average = (qRed(color) + qGreen(color) + qBlue(color)) / 3;

            *lineB = qRgba(average, average, average, qAlpha(color));

            lineA++;
            lineB++;
        }
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE static */ bool WControllerView::compressShot(const QString & fileName, int quality)
{
    QFileInfo info(fileName);

    if (info.exists() && info.suffix().toLower() == "png")
    {
        qDebug("Compressing: %s", fileName.C_STR);

        QImage image(fileName);

        if (image.save(fileName, "png", quality) == false)
        {
            qWarning("WControllerView::compressShot: Failed to save image.");
        }

        return true;
    }
    else return false;
}

/* Q_INVOKABLE static */ bool WControllerView::compressShots(const QString & path, int quality)
{
    QDir dir(path);

    if (dir.exists() == false) return false;

    QFileInfoList list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files);

    foreach (QFileInfo info, list)
    {
        if (info.suffix().toLower() == "png")
        {
            const QString & path = info.filePath();

            qDebug("Compressing: %s", path.C_STR);

            QImage image(path);

            if (image.save(path, "png", quality) == false)
            {
                qWarning("WControllerView::compressShots: Failed to save image.");
            }
        }
    }

    return true;
}

//-------------------------------------------------------------------------------------------------
// Properties
//-------------------------------------------------------------------------------------------------

bool WControllerView::opengl() const
{
    Q_D(const WControllerView); return d->opengl;
}

void WControllerView::setOpengl(bool enabled)
{
    Q_D(WControllerView);

    if (d->opengl == enabled) return;

    d->opengl = enabled;

    emit openglChanged();
}

//-------------------------------------------------------------------------------------------------

WControllerView::LoadMode WControllerView::loadMode() const
{
    Q_D(const WControllerView); return d->loadMode;
}

void WControllerView::setLoadMode(LoadMode mode)
{
    Q_D(WControllerView);

    if (d->loadMode == mode) return;

    d->loadMode = mode;

    emit loadModeChanged();
}

//-------------------------------------------------------------------------------------------------

int WControllerView::scaleDelay() const
{
    Q_D(const WControllerView); return d->scaleDelay;
}

void WControllerView::setScaleDelay(int delay)
{
    Q_D(WControllerView);

    if (d->scaleDelay == delay) return;

    d->scaleDelay = delay;

    emit scaleDelayChanged();
}

#endif // SK_NO_CONTROLLERVIEW

#ifdef QT_LATEST
#include "WControllerView.moc"
#endif
