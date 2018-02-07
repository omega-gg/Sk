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

#include "WDeclarativeBorders.h"

#ifndef SK_NO_DECLARATIVEBORDERS

// Qt includes
#ifdef QT_4
#include <QPainter>
#else
#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>
#endif

// Private includes
#include <private/WDeclarativeItem_p>

#ifdef QT_LATEST

//=================================================================================================
// WDeclarativeBordersLine
//=================================================================================================

class SK_GUI_EXPORT WDeclarativeBordersLine : public QSGGeometryNode
{
public:
    WDeclarativeBordersLine(QSGFlatColorMaterial * material);

public: // Interface
    void setRect(const QRectF & rect);

private: // Variables
    QSGGeometry _geometry;
};

//-------------------------------------------------------------------------------------------------
// Ctor / dtor
//-------------------------------------------------------------------------------------------------

WDeclarativeBordersLine::WDeclarativeBordersLine(QSGFlatColorMaterial * material)
    : _geometry(QSGGeometry::defaultAttributes_Point2D(), 4)
{
    setGeometry(&_geometry);

    setMaterial(material);
}

//-------------------------------------------------------------------------------------------------
// Interface
//-------------------------------------------------------------------------------------------------

void WDeclarativeBordersLine::setRect(const QRectF & rect)
{
    QSGGeometry::updateRectGeometry(&_geometry, rect);

    markDirty(QSGNode::DirtyGeometry);
}

//=================================================================================================
// WDeclarativeBordersNode
//=================================================================================================

class SK_GUI_EXPORT WDeclarativeBordersNode : public QSGNode
{
public:
    WDeclarativeBordersNode();

public: // Properties
    WDeclarativeBordersLine left;
    WDeclarativeBordersLine right;
    WDeclarativeBordersLine top;
    WDeclarativeBordersLine bottom;

    QSGFlatColorMaterial material;
};

//-------------------------------------------------------------------------------------------------
// Ctor / dtor
//-------------------------------------------------------------------------------------------------

WDeclarativeBordersNode::WDeclarativeBordersNode() : left(&material), right (&material),
                                                     top (&material), bottom(&material)
{
    appendChildNode(&left);
    appendChildNode(&right);
    appendChildNode(&top);
    appendChildNode(&bottom);
}

#endif

//=================================================================================================
// WDeclarativeBordersPrivate
//=================================================================================================

class SK_GUI_EXPORT WDeclarativeBordersPrivate : public WDeclarativeItemPrivate
{
public:
    WDeclarativeBordersPrivate(WDeclarativeBorders * p);

    void init();

public: // Variables
    qreal left;
    qreal right;
    qreal top;
    qreal bottom;

    QColor color;

#ifdef QT_LATEST
    bool updateLeft;
    bool updateRight;
    bool updateTop;
    bool updateBottom;

    bool updateColor;
#endif

protected:
    W_DECLARE_PUBLIC(WDeclarativeBorders)
};

//-------------------------------------------------------------------------------------------------

WDeclarativeBordersPrivate::WDeclarativeBordersPrivate(WDeclarativeBorders * p)
    : WDeclarativeItemPrivate(p) {}

void WDeclarativeBordersPrivate::init()
{
    Q_Q(WDeclarativeBorders);

    left   = 0.0;
    right  = 0.0;
    top    = 0.0;
    bottom = 0.0;

#ifdef QT_LATEST
    updateLeft   = true;
    updateRight  = true;
    updateTop    = true;
    updateBottom = true;

    updateColor = true;
#endif

    q->setFlag(QGraphicsItem::ItemHasNoContents, false);
}

//=================================================================================================
// WDeclarativeBorders
//=================================================================================================

#ifdef QT_4
/* explicit */ WDeclarativeBorders::WDeclarativeBorders(QDeclarativeItem * parent)
#else
/* explicit */ WDeclarativeBorders::WDeclarativeBorders(QQuickItem * parent)
#endif
    : WDeclarativeItem(new WDeclarativeBordersPrivate(this), parent)
{
    Q_D(WDeclarativeBorders); d->init();
}

//-------------------------------------------------------------------------------------------------
// QGraphicsItem / QQuickItem reimplementation
//-------------------------------------------------------------------------------------------------

#ifdef QT_4

/* virtual */ void WDeclarativeBorders::paint(QPainter * painter,
                                              const QStyleOptionGraphicsItem *, QWidget *)
{
    Q_D(WDeclarativeBorders);

    painter->setPen(Qt::NoPen);

    painter->setBrush(d->color);

    painter->drawRect(QRectF(0,                  0, d->left,  height()));
    painter->drawRect(QRectF(width() - d->right, 0, d->right, height()));

    painter->drawRect(QRectF(0,                    0, width(), d->top));
    painter->drawRect(QRectF(0, height() - d->bottom, width(), d->bottom));
}

#else

/* virtual */ void WDeclarativeBorders::geometryChanged(const QRectF & newGeometry,
                                                        const QRectF & oldGeometry)
{
    Q_D(WDeclarativeBorders);

    WDeclarativeItem::geometryChanged(newGeometry, oldGeometry);

    if (d->left)   d->updateLeft   = true;
    if (d->right)  d->updateRight  = true;
    if (d->top)    d->updateTop    = true;
    if (d->bottom) d->updateBottom = true;
}

/* virtual */ QSGNode * WDeclarativeBorders::updatePaintNode(QSGNode             * oldNode,
                                                             UpdatePaintNodeData *)
{
    Q_D(WDeclarativeBorders);

    WDeclarativeBordersNode * node = static_cast<WDeclarativeBordersNode *> (oldNode);

    if (node == NULL) node = new WDeclarativeBordersNode;

    qreal width  = this->width ();
    qreal height = this->height();

    if (d->updateLeft)
    {
        d->updateLeft = false;

        node->left.setRect(QRectF(0, 0, d->left, height));
    }

    if (d->updateRight)
    {
        d->updateRight = false;

        node->right.setRect(QRectF(width - d->right, 0, d->right, height));
    }

    if (d->updateTop)
    {
        d->updateTop = false;

        node->top.setRect(QRectF(0, 0, width, d->top));
    }

    if (d->updateBottom)
    {
        d->updateBottom = false;

        node->bottom.setRect(QRectF(0, height - d->bottom, width, d->bottom));
    }

    if (d->updateColor)
    {
        d->updateColor = false;

        node->material.setColor(d->color);

        node->left  .markDirty(QSGNode::DirtyMaterial);
        node->right .markDirty(QSGNode::DirtyMaterial);
        node->top   .markDirty(QSGNode::DirtyMaterial);
        node->bottom.markDirty(QSGNode::DirtyMaterial);
    }

    return node;
}

#endif

//-------------------------------------------------------------------------------------------------
// properties
//-------------------------------------------------------------------------------------------------

qreal WDeclarativeBorders::left() const
{
    Q_D(const WDeclarativeBorders); return d->left;
}

void WDeclarativeBorders::setLeft(qreal left)
{
    Q_D(WDeclarativeBorders);

    if (d->left == left) return;

    d->left = left;

#ifdef QT_LATEST
    d->updateLeft = true;
#endif

    update();

    emit leftChanged();
}

//-------------------------------------------------------------------------------------------------

qreal WDeclarativeBorders::right() const
{
    Q_D(const WDeclarativeBorders); return d->right;
}

void WDeclarativeBorders::setRight(qreal right)
{
    Q_D(WDeclarativeBorders);

    if (d->right == right) return;

    d->right = right;

#ifdef QT_LATEST
    d->updateRight = true;
#endif

    update();

    emit rightChanged();
}

//-------------------------------------------------------------------------------------------------

qreal WDeclarativeBorders::top() const
{
    Q_D(const WDeclarativeBorders); return d->top;
}

void WDeclarativeBorders::setTop(qreal top)
{
    Q_D(WDeclarativeBorders);

    if (d->top == top) return;

    d->top = top;

#ifdef QT_LATEST
    d->updateTop = true;
#endif

    update();

    emit topChanged();
}

//-------------------------------------------------------------------------------------------------

qreal WDeclarativeBorders::bottom() const
{
    Q_D(const WDeclarativeBorders); return d->bottom;
}

void WDeclarativeBorders::setBottom(qreal bottom)
{
    Q_D(WDeclarativeBorders);

    if (d->bottom == bottom) return;

    d->bottom = bottom;

#ifdef QT_LATEST
    d->updateBottom = true;
#endif

    update();

    emit bottomChanged();
}

//-------------------------------------------------------------------------------------------------

QColor WDeclarativeBorders::color() const
{
    Q_D(const WDeclarativeBorders); return d->color;
}

void WDeclarativeBorders::setColor(const QColor & color)
{
    Q_D(WDeclarativeBorders);

    if (d->color == color) return;

    d->color = color;

#ifdef QT_LATEST
    d->updateColor = true;
#endif

    update();

    emit colorChanged();
}

#endif // SK_NO_DECLARATIVEBORDERS
