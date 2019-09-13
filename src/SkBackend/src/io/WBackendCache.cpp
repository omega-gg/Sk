//=================================================================================================
/*
    Copyright (C) 2015-2017 Sky kit authors united with omega. <http://omega.gg/about>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of the SkBackend module of Sky kit.

    - GNU General Public License Usage:
    This file may be used under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation and appearing in the LICENSE.md file included in the packaging
    of this file. Please review the following information to ensure the GNU General Public License
    requirements will be met: https://www.gnu.org/licenses/gpl.html.
*/
//=================================================================================================

#include "WBackendCache.h"

#ifndef SK_NO_BACKENDCACHE

// Sk includes
#include <WBackendUniversal>

//-------------------------------------------------------------------------------------------------
// Static variables

static const int BACKENDCACHE_MAX = 20;

//-------------------------------------------------------------------------------------------------
// Private
//-------------------------------------------------------------------------------------------------

class SK_BACKEND_EXPORT WBackendCachePrivate : public WPrivate
{
public:
    WBackendCachePrivate(WBackendCache * p);

    void init();

public: // Functions
    void updateScripts();

public: // Variables
    QHash<QString, WBackendUniversalScript> scripts;

    QStringList names;

    int maxCount;

protected:
    W_DECLARE_PUBLIC(WBackendCache)
};

//-------------------------------------------------------------------------------------------------

WBackendCachePrivate::WBackendCachePrivate(WBackendCache * p) : WPrivate(p) {}

void WBackendCachePrivate::init()
{
    maxCount = BACKENDCACHE_MAX;
}

//-------------------------------------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------------------------------------

void WBackendCachePrivate::updateScripts()
{
    while (names.count() > BACKENDCACHE_MAX)
    {
        scripts.remove(names.takeFirst());
    }
}

//-------------------------------------------------------------------------------------------------
// Ctor / dtor
//-------------------------------------------------------------------------------------------------

WBackendCache::WBackendCache(QObject * parent)
    : QObject(parent), WPrivatable(new WBackendCachePrivate(this))
{
    Q_D(WBackendCache); d->init();
}

//-------------------------------------------------------------------------------------------------
// Interface
//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE */ WBackendUniversalScript * WBackendCache::getScript(const QString & name,
                                                                     const QString & data)
{
    Q_D(WBackendCache);

    QHash<QString, WBackendUniversalScript>::iterator i = d->scripts.find(name);

    if (i != d->scripts.end())
    {
        qDebug("SCRIPT CACHED");

        d->names.removeOne(name);
        d->names.append   (name);

        return &(i.value());
    }

    WBackendUniversalScript script(data);

    i = d->scripts.insert(name, script);

    d->names.append(name);

    d->updateScripts();

    return &(i.value());
}

/* Q_INVOKABLE */ void WBackendCache::removeScripts(const QString & pattern)
{
    Q_D(WBackendCache);

    QMutableHashIterator<QString, WBackendUniversalScript> i(d->scripts);

    while (i.hasNext())
    {
        i.next();

        QString name = i.key();

        if (name.startsWith(pattern))
        {
            d->names.removeOne(name);

            i.remove();
        }
    }
}

//-------------------------------------------------------------------------------------------------
// Properties
//-------------------------------------------------------------------------------------------------

int WBackendCache::maxCount() const
{
    Q_D(const WBackendCache); return d->maxCount;
}

void WBackendCache::setMaxCount(int max)
{
    Q_D(WBackendCache);

    if (d->maxCount == max) return;

    d->maxCount = max;

    emit maxCountChanged();
}

#endif // SK_NO_BACKENDCACHE