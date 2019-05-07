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

Text
{
    id: textSubtitle

    //---------------------------------------------------------------------------------------------
    // Properties
    //---------------------------------------------------------------------------------------------

    property int pixelSize: sizeMinimum

    property int sizeMinimum: st.textSubtitle_sizeMinimum
    property int sizeMaximum: st.textSubtitle_sizeMaximum

    //---------------------------------------------------------------------------------------------
    // Aliases
    //---------------------------------------------------------------------------------------------

    property alias backend: backend

    //---------------------------------------------------------------------------------------------
    // Settings
    //---------------------------------------------------------------------------------------------

    width : paintedWidth
    height: paintedHeight

    horizontalAlignment: Text.AlignHCenter
    verticalAlignment  : Text.AlignBottom

    text: backend.text

    color: st.textSubtitle_color

    style: Text.Outline

    styleColor: st.textSubtitle_colorStyle

    textFormat: Text.RichText

    font.family   : st.text_fontFamily
    font.pixelSize: pGetSize()
    font.bold     : true

    //---------------------------------------------------------------------------------------------
    // Functions private
    //---------------------------------------------------------------------------------------------

    function pGetSize()
    {
        if (pixelSize < sizeMinimum)
        {
            return sizeMinimum;
        }
        else if (pixelSize > sizeMaximum)
        {
            return sizeMaximum;
        }
        else return pixelSize;
    }

    //---------------------------------------------------------------------------------------------
    // Childs
    //---------------------------------------------------------------------------------------------

    BackendSubtitle { id: backend }
}