//=================================================================================================
/*
    Copyright (C) 2015-2017 Sky kit authors united with omega. <http://omega.gg/about>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of the SkCore module of Sky kit.

    - GNU General Public License Usage:
    This file may be used under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation and appearing in the LICENSE.md file included in the packaging
    of this file. Please review the following information to ensure the GNU General Public License
    requirements will be met: https://www.gnu.org/licenses/gpl.html.
*/
//=================================================================================================

#ifndef WDECLARATIVECOREAPPLICATION_H
#define WDECLARATIVECOREAPPLICATION_H

// Qt includes
#ifdef QT_4
#include <QDeclarativeListProperty>
#else
#include <QQmlListProperty>
#endif

// Sk includes
#include <Sk>

#ifndef SK_NO_COREAPPLICATION

// Forward declarations
#ifdef QT_4
class QDeclarativeItem;
#else
class QQuickItem;
#endif
class WDeclarativeCoreApplicationPrivate;

class SK_CORE_EXPORT WDeclarativeCoreApplication : public QObject, public WPrivatable
{
    Q_OBJECT

#ifdef QT_4
    Q_PROPERTY(QDeclarativeListProperty<QDeclarativeItem> children READ children)
#else
    Q_PROPERTY(QQmlListProperty<QQuickItem> children READ children)
#endif

    Q_CLASSINFO("DefaultProperty", "children")

public:
    WDeclarativeCoreApplication(QObject * object = 0);
protected:
    WDeclarativeCoreApplication(WDeclarativeCoreApplicationPrivate * p, QObject * object = 0);

private: // Declarative
#ifdef QT_4
    static void childrenAppend(QDeclarativeListProperty<QDeclarativeItem> * property,
                               QDeclarativeItem * item);

    static void childrenClear(QDeclarativeListProperty<QDeclarativeItem> * property);

    static int childrenCount(QDeclarativeListProperty<QDeclarativeItem> * property);

    static QDeclarativeItem * childrenAt(QDeclarativeListProperty<QDeclarativeItem> * property,
                                         int index);
#else
    static void childrenAppend(QQmlListProperty<QQuickItem> * property, QQuickItem * item);
    static void childrenClear (QQmlListProperty<QQuickItem> * property);

    static int childrenCount(QQmlListProperty<QQuickItem> * property);

    static QQuickItem * childrenAt(QQmlListProperty<QQuickItem> * property, int index);
#endif

public: // Properties
#ifdef QT_4
    QDeclarativeListProperty<QDeclarativeItem> children();
#else
    QQmlListProperty<QQuickItem> children();
#endif

private:
    W_DECLARE_PRIVATE(WDeclarativeCoreApplication)
};

#endif // SK_NO_DECLARATIVECOREAPPLICATION
#endif // WDECLARATIVECOREAPPLICATION_H
