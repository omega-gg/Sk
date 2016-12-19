//=================================================================================================
/*
    Copyright (C) 2015-2016 Sky kit authors united with omega. <http://omega.gg/about>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of the SkyComponents module of Sky kit.

    - GNU General Public License Usage:
    This file may be used under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation and appearing in the LICENSE.md file included in the packaging
    of this file. Please review the following information to ensure the GNU General Public License
    requirements will be met: https://www.gnu.org/licenses/gpl.html.
*/
//=================================================================================================

import QtQuick 1.1
import Sky     1.0

BaseButtonPush
{
    id: buttonPushFull

    //---------------------------------------------------------------------------------------------
    // Properties
    //---------------------------------------------------------------------------------------------

    property int padding: st.buttonPush_padding

    property int paddingLeft : padding
    property int paddingRight: padding

    property int spacing: st.buttonPush_spacing

    property int minimumWidth: -1
    property int maximumWidth: -1

    //---------------------------------------------------------------------------------------------
    // Style

    property ImageColorFilter filterIconCheck : st.buttonPush_filterIconCheck

    //---------------------------------------------------------------------------------------------
    // Aliases
    //---------------------------------------------------------------------------------------------

    property alias isIconDefault: itemIcon.isSourceDefault

    property alias icon       : itemIcon.source
    property alias iconDefault: itemIcon.sourceDefault

    property alias iconWidth : itemIcon.iconWidth
    property alias iconHeight: itemIcon.iconHeight

    property alias iconSourceSize: itemIcon.sourceSize
    property alias iconSourceArea: itemIcon.sourceArea

    property alias iconLoadMode: itemIcon.loadMode
    property alias iconFillMode: itemIcon.fillMode

    property alias iconAsynchronous: itemIcon.asynchronous
    property alias iconCache       : itemIcon.cache

    property alias iconScaling: itemIcon.scaling

    property alias iconStyle: itemIcon.iconStyle

    property alias text: itemText.text
    property alias font: itemText.font

    //---------------------------------------------------------------------------------------------

    property alias itemIcon: itemIcon
    property alias itemText: itemText

    //---------------------------------------------------------------------------------------------
    // Style

    property alias enableFilter: itemIcon.enableFilter

    property alias filterIcon      : itemIcon.filterDefault
    property alias filterIconShadow: itemIcon.filterShadow
    property alias filterIconSunken: itemIcon.filterSunken

    //---------------------------------------------------------------------------------------------
    // Settings
    //---------------------------------------------------------------------------------------------

    width: getPreferredWidth()

    height: st.buttonPush_height

    //---------------------------------------------------------------------------------------------
    // Functions
    //---------------------------------------------------------------------------------------------

    function getPreferredWidth()
    {
        var size;

        if (iconWidth)
        {
            if (text)
            {
                size = iconWidth + sk.textWidth(font, text) + paddingLeft + paddingRight + spacing;
            }
            else size = itemIcon.width + paddingLeft + paddingRight;
        }
        else if (text)
        {
            size = sk.textWidth(font, text) + paddingLeft + paddingRight;
        }

        if (minimumWidth != -1)
        {
            size = Math.max(minimumWidth, size);
        }

        if (maximumWidth != -1)
        {
            size = Math.min(size, maximumWidth);
        }

        return size;
    }

    //---------------------------------------------------------------------------------------------
    // Childs
    //---------------------------------------------------------------------------------------------

    Icon
    {
        id: itemIcon

        anchors.left: parent.left

        anchors.leftMargin: paddingLeft

        anchors.verticalCenter: parent.verticalCenter

        sourceSize.height: (scaling) ? -1 : parent.height

        opacity: (buttonPushFull.enabled) ? 1.0 : st.icon_opacityDisable

        iconStyle: (checked) ? Sk.IconRaised
                             : Sk.IconSunken

        filter:
        {
            if (enableFilter)
            {
                if (checked && checkIcon) return filterIconCheck;
                else                      return filterIcon;
            }
            else return null;
        }

        filterDefault: st.buttonPush_filterIcon
        filterShadow : st.buttonPush_filterIconShadow
        filterSunken : st.buttonPush_filterIconSunken
    }

    TextBase
    {
        id: itemText

        anchors.left  : itemIcon.right
        anchors.right : parent.right
        anchors.top   : parent.top
        anchors.bottom: parent.bottom

        leftMargin: (iconWidth) ? spacing : 0

        rightMargin: paddingRight

        verticalAlignment: Text.AlignVCenter

        opacity: (buttonPushFull.enabled) ? 1.0 : st.text_opacityDisable

        style: (checked) ? Text.Raised
                         : Text.Sunken

        Behavior on opacity
        {
            PropertyAnimation { duration: st.duration_fast }
        }
    }
}
