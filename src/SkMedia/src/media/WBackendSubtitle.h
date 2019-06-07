//=================================================================================================
/*
    Copyright (C) 2015-2017 Sky kit authors united with omega. <http://omega.gg/about>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of the SkMedia module of Sky kit.

    - GNU General Public License Usage:
    This file may be used under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation and appearing in the LICENSE.md file included in the packaging
    of this file. Please review the following information to ensure the GNU General Public License
    requirements will be met: https://www.gnu.org/licenses/gpl.html.
*/
//=================================================================================================

#ifndef WBACKENDSUBTITLE_H
#define WBACKENDSUBTITLE_H

// Qt includes
#include <QObject>

// Sk includes
#include <Sk>

#ifndef SK_NO_BACKENDSUBTITLE

class WBackendSubtitlePrivate;

class SK_MEDIA_EXPORT WBackendSubtitle : public QObject, public WPrivatable
{
    Q_OBJECT

    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)

    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)

    Q_PROPERTY(int currentTime READ currentTime WRITE setCurrentTime NOTIFY currentTimeChanged)

    Q_PROPERTY(QString text READ text NOTIFY textChanged)

public:
    explicit WBackendSubtitle(QObject * parent = NULL);

protected: // Events
    /* virtual */ void timerEvent(QTimerEvent * event);

signals:
    void loaded(bool ok);

    void enabledChanged();

    void sourceChanged();

    void currentTimeChanged();

    void textChanged();

public: // Properties
    bool isEnabled() const;
    void setEnabled(bool enabled);

    QString source() const;
    void    setSource(const QString & url);

    int  currentTime() const;
    void setCurrentTime(int msec);

    QString text() const;

private:
    W_DECLARE_PRIVATE(WBackendSubtitle)

    Q_PRIVATE_SLOT(d_func(), void onQueryData(const QByteArray &, const QString &))

    Q_PRIVATE_SLOT(d_func(), void onQueryCompleted())
};

#include <private/WBackendSubtitle_p>

#endif // SK_NO_BACKENDSUBTITLE
#endif // WBACKENDSUBTITLE_H
