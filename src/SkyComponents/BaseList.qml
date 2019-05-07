//=================================================================================================
/*
    Copyright (C) 2015-2017 Sky kit authors united with omega. <http://omega.gg/about>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of the SkyComponents module of Sky kit.

    - GNU General Public License Usage:
    This file may be used under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation and appearing in the LICENSE.md file included in the packaging
    of this file. Please review the following information to ensure the GNU General Public License
    requirements will be met: https://www.gnu.org/licenses/gpl.html.
*/
//=================================================================================================

import QtQuick 1.0
import Sky     1.0

ListVertical
{
    //---------------------------------------------------------------------------------------------
    // Properties
    //---------------------------------------------------------------------------------------------

    property variant scrollArea: null

    //---------------------------------------------------------------------------------------------
    // Settings
    //---------------------------------------------------------------------------------------------

    itemSize: st.list_itemSize

    //---------------------------------------------------------------------------------------------
    // Functions
    //---------------------------------------------------------------------------------------------

    function focus()
    {
        if (count) forceActiveFocus();
    }

    //---------------------------------------------------------------------------------------------

    function scrollToItem(index)
    {
        if (scrollArea == null || index < 0 || index >= count) return;

        scrollArea.ensureVisible(y + index * itemSize, itemSize);
    }

    function scrollToItemTop(index)
    {
        if (scrollArea == null || index < 0 || index >= count) return;

        scrollArea.ensureVisible(y + (index - 1) * itemSize + scrollArea.height, itemSize);
    }
}