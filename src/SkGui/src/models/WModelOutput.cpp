//=================================================================================================
/*
    Copyright (C) 2015-2020 Sky kit authors. <http://omega.gg/Sky>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of SkGui.

    - GNU Lesser General Public License Usage:
    This file may be used under the terms of the GNU Lesser General Public License version 3 as
    published by the Free Software Foundation and appearing in the LICENSE.md file included in the
    packaging of this file. Please review the following information to ensure the GNU Lesser
    General Public License requirements will be met: https://www.gnu.org/licenses/lgpl.html.

    - Private License Usage:
    Sky kit licensees holding valid private licenses may use this file in accordance with the
    private license agreement provided with the Software or, alternatively, in accordance with the
    terms contained in written agreement between you and Sky kit authors. For further information
    contact us at contact@omega.gg.
*/
//=================================================================================================

#include "WModelOutput.h"

#ifndef SK_NO_MODELOUTPUT

//-------------------------------------------------------------------------------------------------
// Private
//-------------------------------------------------------------------------------------------------

#include <private/Sk_p>

class SK_GUI_EXPORT WModelOutputPrivate : public WPrivate
{
public:
    WModelOutputPrivate(WModelOutput * p);

    /* virtual */ ~WModelOutputPrivate();

    void init();

public: // Variables
    WAbstractBackend * backend;

protected:
    W_DECLARE_PUBLIC(WModelOutput)
};

//-------------------------------------------------------------------------------------------------

WModelOutputPrivate::WModelOutputPrivate(WModelOutput * p) : WPrivate(p) {}

/* virtual */ WModelOutputPrivate::~WModelOutputPrivate()
{
    Q_Q(WModelOutput);

    if (backend) backend->unregisterWatcher(q);
}

//-------------------------------------------------------------------------------------------------

void WModelOutputPrivate::init()
{
    backend = NULL;

#ifdef QT_4
    Q_Q(WModelOutput);

    q->setRoleNames(q->roleNames());
#endif
}

//-------------------------------------------------------------------------------------------------
// Ctor / dtor
//-------------------------------------------------------------------------------------------------

/* explicit */ WModelOutput::WModelOutput(QObject * parent)
    : QAbstractListModel(parent), WPrivatable(new WModelOutputPrivate(this))
{
    Q_D(WModelOutput); d->init();
}

//-------------------------------------------------------------------------------------------------
// QAbstractItemModel implementation
//-------------------------------------------------------------------------------------------------

/* virtual */ int WModelOutput::rowCount(const QModelIndex &) const
{
    Q_D(const WModelOutput);

    if (d->backend)
    {
         return d->backend->countOutput();
    }
    else return 0;
}

/* virtual */ QVariant WModelOutput::data(const QModelIndex & index, int role) const
{
    Q_D(const WModelOutput);

    if (d->backend == NULL) return QVariant();

    int row = index.row();

    if (row < 0 || row >= d->backend->countOutput())
    {
        return QVariant();
    }

    const WBackendOutput * output = d->backend->outputAt(row);

    if      (role == RoleName)    return output->name;
    else if (role == RoleCurrent) return (d->backend->currentOutput() == row);
    else                          return QVariant();
}

//-------------------------------------------------------------------------------------------------
// QAbstractItemModel reimplementation
//-------------------------------------------------------------------------------------------------

/* virtual */ QHash<int, QByteArray> WModelOutput::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles.insert(WModelOutput::RoleName,    "name");
    roles.insert(WModelOutput::RoleCurrent, "current");

    return roles;
}

//-------------------------------------------------------------------------------------------------
// Protected WBackendWatcher reimplementation
//-------------------------------------------------------------------------------------------------

/* virtual */ void WModelOutput::beginOutputInsert(int first, int last)
{
    beginInsertRows(QModelIndex(), first, last);
}

/* virtual */ void WModelOutput::endOutputInsert()
{
    endInsertRows();
}

//-------------------------------------------------------------------------------------------------

/* virtual */ void WModelOutput::beginOutputRemove(int first, int last)
{
    beginRemoveRows(QModelIndex(), first, last);
}

/* virtual */ void WModelOutput::endOutputRemove()
{
    endRemoveRows();
}

//-------------------------------------------------------------------------------------------------

/* virtual */ void WModelOutput::beginOutputClear()
{
    beginResetModel();
}

/* virtual */ void WModelOutput::endOutputClear()
{
    endResetModel();
}

//-------------------------------------------------------------------------------------------------

/* virtual */ void WModelOutput::currentOutputChanged(int index)
{
    QModelIndex modelIndex = this->index(index);

    emit dataChanged(modelIndex, modelIndex);
}

//-------------------------------------------------------------------------------------------------

/* virtual */ void WModelOutput::backendDestroyed()
{
    setBackend(NULL);
}

//-------------------------------------------------------------------------------------------------
// Properties
//-------------------------------------------------------------------------------------------------

WAbstractBackend * WModelOutput::backend() const
{
    Q_D(const WModelOutput); return d->backend;
}

void WModelOutput::setBackend(WAbstractBackend * backend)
{
    Q_D(WModelOutput);

    if (d->backend == backend) return;

    if (d->backend)
    {
        d->backend->unregisterWatcher(this);
    }

    beginResetModel();

    d->backend = backend;

    endResetModel();

    if (d->backend)
    {
        d->backend->registerWatcher(this);
    }

    emit backendChanged();
}

#endif // SK_NO_MODELOUTPUT
