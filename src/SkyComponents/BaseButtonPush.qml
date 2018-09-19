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

BaseButton
{
    id: baseButtonPush

    //---------------------------------------------------------------------------------------------
    // Properties
    //---------------------------------------------------------------------------------------------

    property int margins: height / 8

//#QT_4
    property int radius: (height - margins * 2) / 3.75
//#ELSE
    property int radius: background.height / 3.75
//#END

    property int borderSize: st.border_size

    //---------------------------------------------------------------------------------------------
    // Style

    property color colorA: (highlighted) ? st.buttonPush_colorHighlightA
                                         : st.buttonPush_colorA

    property color colorB: (highlighted) ? st.buttonPush_colorHighlightB
                                         : st.buttonPush_colorB

    property color colorHoverA: (highlighted) ? st.buttonPush_colorHighlightHoverA
                                              : st.buttonPush_colorHoverA

    property color colorHoverB: (highlighted) ? st.buttonPush_colorHighlightHoverB
                                              : st.buttonPush_colorHoverB

    property color colorPressA: (highlighted || checkable) ? st.buttonPush_colorCheckA
                                                           : st.buttonPush_colorPressA

    property color colorPressB: (highlighted || checkable) ? st.buttonPush_colorCheckB
                                                           : st.buttonPush_colorPressB

    property color colorPressHoverA: (highlighted || checkable) ? st.buttonPush_colorCheckHoverA
                                                                : st.buttonPush_colorPressHoverA

    property color colorPressHoverB: (highlighted || checkable) ? st.buttonPush_colorCheckHoverB
                                                                : st.buttonPush_colorPressHoverB

    property color colorFocus: st.button_colorFocus

    //---------------------------------------------------------------------------------------------
    // Aliases
    //---------------------------------------------------------------------------------------------

    property alias background: background
    property alias itemFocus : itemFocus

    //---------------------------------------------------------------------------------------------
    // Settings
    //---------------------------------------------------------------------------------------------

    cursor: Qt.ArrowCursor

    //---------------------------------------------------------------------------------------------
    // Childs
    //---------------------------------------------------------------------------------------------

    Rectangle
    {
        id: itemFocus

        anchors.fill: background

        anchors.margins: -borderSize

        radius: baseButtonPush.radius

        opacity: (window.isActive && isFocused)

        color: "transparent"

//#QT_4
        smooth: true
//#END

        border.width: borderSize
        border.color: colorFocus

        Behavior on opacity
        {
            PropertyAnimation { duration: st.duration_fast }
        }
    }

    Rectangle
    {
        id: background

        anchors.fill: parent

        anchors.margins: margins

        radius: baseButtonPush.radius

        gradient: Gradient
        {
            GradientStop
            {
                position: 0.0

                color:
                {
                    if (isPressed)
                    {
                        return colorPressA;
                    }
                    else if (checked)
                    {
                        if (isHovered && checkHover)
                        {
                             return colorPressHoverA;
                        }
                        else return colorPressA;
                    }
                    else if (isHovered)
                    {
                         return colorHoverA;
                    }
                    else return colorA;
                }
            }

            GradientStop
            {
                position: 1.0

                color:
                {
                    if (isPressed)
                    {
                        return colorPressB;
                    }
                    else if (checked)
                    {
                        if (isHovered && checkHover)
                        {
                             return colorPressHoverB;
                        }
                        else return colorPressB;
                    }
                    else if (isHovered)
                    {
                         return colorHoverB;
                    }
                    else return colorB;
                }
            }
        }

//#QT_4
        smooth: true
//#END

        border.width: borderSize
        border.color: st.border_color
    }
}
