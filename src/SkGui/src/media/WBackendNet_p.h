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

#ifndef WBACKENDNET_P_H
#define WBACKENDNET_P_H

/*  W A R N I N G
    -------------

    This file is not part of the Sk API. It exists purely as an
    implementation detail. This header file may change from version to
    version without notice, or even be removed.

    We mean it.
*/

// Qt includes
#include <QMetaMethod>

// Private includes
#include <private/Sk_p>

#ifndef SK_NO_BACKENDNET

class SK_GUI_EXPORT WBackendNetPrivate : public WPrivate
{
public:
    WBackendNetPrivate(WBackendNet * p);

    /* virtual */ ~WBackendNetPrivate();

    void init();

public: // Slots
    void onLoadSource  (WNetReplySource   * reply) const;
    void onLoadTrack   (WNetReplyTrack    * reply) const;
    void onLoadPlaylist(WNetReplyPlaylist * reply) const;
    void onLoadFolder  (WNetReplyFolder   * reply) const;

public: // Variables
    QMetaMethod methodSource;
    QMetaMethod methodTrack;
    QMetaMethod methodPlaylist;
    QMetaMethod methodFolder;

protected:
    W_DECLARE_PUBLIC(WBackendNet)
};

#endif // SK_NO_BACKENDNET
#endif // WBACKENDNET_P_H
