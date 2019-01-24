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

MouseArea
{
    id: playerBrowser

    //---------------------------------------------------------------------------------------------
    // Properties
    //---------------------------------------------------------------------------------------------

    /* mandatory */ property Player player

    /* read */ property TabTrack playerTab: player.tab

    property TabTrack tab: playerTab

    property bool loading: false

    property bool enableTitle : true
    property bool enableAuthor: true

    //---------------------------------------------------------------------------------------------
    // Private

    property int pScroll: -1

    property bool pBackward: false
    property bool pForward : false

    //---------------------------------------------------------------------------------------------
    // Aliases
    //---------------------------------------------------------------------------------------------

    property alias title: itemTitle.text

    //---------------------------------------------------------------------------------------------

    property alias barTitle  : barTitle
    property alias barDetails: barDetails

    property alias areaBackward: areaBackward
    property alias areaForward : areaForward

    property alias buttonPrevious: buttonPrevious
    property alias buttonPlay    : buttonPlay
    property alias buttonNext    : buttonNext

    //---------------------------------------------------------------------------------------------
    // Signals
    //---------------------------------------------------------------------------------------------

    signal titleClicked (variant mouse)
    signal authorClicked(variant mouse)

    signal contextual

    //---------------------------------------------------------------------------------------------
    // Settings
    //---------------------------------------------------------------------------------------------

    acceptedButtons: Qt.RightButton

    wheelEnabled: true

    //---------------------------------------------------------------------------------------------
    // Events
    //---------------------------------------------------------------------------------------------

    onPressed: contextual()

    onWheeled:
    {
        if (timerScroll.running) return;

        var i = steps;

        if (i > 0)
        {
            while (i)
            {
                setNext();

                i--;
            }

            pFlashNext();
        }
        else
        {
            while (i)
            {
                setPrevious();

                i++;
            }

            pFlashPrevious();
        }
    }

    //---------------------------------------------------------------------------------------------
    // Functions
    //---------------------------------------------------------------------------------------------

    function play()
    {
        if (tabs.highlightedTab)
        {
            tabs.highlightedTab = null;
        }

        player.play();

        window.clearFocus();
    }

    //---------------------------------------------------------------------------------------------

    function setPrevious()
    {
        if (playerTab == tab)
        {
            player.setPreviousTrack();
        }
        else tab.setPreviousTrack();
    }

    function setNext()
    {
        if (playerTab == tab)
        {
            player.setNextTrack();
        }
        else tab.setNextTrack();
    }

    //---------------------------------------------------------------------------------------------

    function flashPrevious()
    {
        setPrevious();

        pFlashPrevious();
    }

    function flashNext()
    {
        setNext();

        pFlashNext();
    }

    //---------------------------------------------------------------------------------------------

    function scrollPrevious()
    {
        if (buttonPrevious.visible == false || (pScroll == 0 && timerScroll.running)) return;

        pScroll = 0;

        timerScroll.restart();

        flashPrevious();
    }

    function scrollNext()
    {
        if (buttonNext.visible == false || (pScroll == 1 && timerScroll.running)) return;

        pScroll = 1;

        timerScroll.restart();

        flashNext();
    }

    function scrollClear()
    {
        timerScroll.stop();
    }

    //---------------------------------------------------------------------------------------------
    // Private

    function pFlashPrevious()
    {
        pBackward = true;

        timerBackward.restart();
    }

    function pFlashNext()
    {
        pForward = true;

        timerForward.restart();
    }

    //---------------------------------------------------------------------------------------------

    function pGetWidth()
    {
        var width = Math.round(playerBrowser.width / 6);

        width = Math.max(st.dp48, width);

        return Math.min(width, st.dp80);
    }

    //---------------------------------------------------------------------------------------------
    // Childs
    //---------------------------------------------------------------------------------------------

    Timer
    {
        id: timerScroll

        interval: st.playerBrowser_interval

        repeat: true

        onTriggered:
        {
            if (pScroll)
            {
                 flashNext();
            }
            else flashPrevious();
        }
    }

    Timer
    {
        id: timerBackward

        interval: st.playerBrowser_interval

        onTriggered: pBackward = false
    }

    Timer
    {
        id: timerForward

        interval: st.playerBrowser_interval

        onTriggered: pForward = false
    }

    MouseArea
    {
        id: areaBackward

        anchors.top   : (barTitle  .visible) ? barTitle.bottom : parent.top
        anchors.bottom: (barDetails.visible) ? barDetails.top  : parent.bottom

        width: Math.round(parent.width / 4)

        visible: buttonPrevious.visible

        hoverEnabled: buttonPrevious.visible

        cursor: Qt.PointingHandCursor

        onPressed : scrollPrevious()
        onReleased: scrollClear   ()

        Behavior on visible
        {
            enabled: (areaBackward.visible)

            PropertyAnimation { duration: st.ms1000 }
        }
    }

    MouseArea
    {
        id: areaForward

        anchors.top   : areaBackward.top
        anchors.bottom: areaBackward.bottom
        anchors.right : parent.right

        width: Math.round(parent.width / 4)

        visible: buttonNext.visible

        hoverEnabled: buttonNext.visible

        cursor: Qt.PointingHandCursor

        onPressed : scrollNext ()
        onReleased: scrollClear()

        Behavior on visible
        {
            enabled: (areaForward.visible)

            PropertyAnimation { duration: st.ms1000 }
        }
    }

    MouseArea
    {
        id: areaPlay

        anchors.centerIn: parent

        width : Math.round(buttonPlay.width * 1.4)
        height: width

        visible: buttonPlay.visible

        hoverEnabled: buttonPlay.visible

        cursor: Qt.PointingHandCursor

        onClicked: play()
    }

    /*RectangleShadow
    {
        anchors.left  : areaBackward.left
        anchors.right : areaBackward.right
        anchors.top   : parent.top
        anchors.bottom: parent.bottom

        anchors.rightMargin: Math.round(areaBackward.width / 3)

        opacity: (areaBackward.containsMouse || pBackward) ? st.playerBrowser_opacity : 0.0

        direction: Sk.Right

        filter: st.playerBrowser_filterShadow

        Behavior on opacity
        {
            PropertyAnimation
            {
                duration: st.duration_fast

                easing.type: st.easing
            }
        }
    }

    RectangleShadow
    {
        anchors.left  : areaForward.left
        anchors.right : areaForward.right
        anchors.top   : parent.top
        anchors.bottom: parent.bottom

        anchors.leftMargin: Math.round(areaForward.width / 3)

        opacity: (areaForward.containsMouse || pForward) ? st.playerBrowser_opacity : 0.0

        filter: st.playerBrowser_filterShadow

        Behavior on opacity
        {
            PropertyAnimation
            {
                duration: st.duration_fast

                easing.type: st.easing
            }
        }
    }*/

    ButtonPushOverlay
    {
        id: buttonPrevious

        anchors.left: parent.left

        anchors.leftMargin: st.dp8

        anchors.verticalCenter: parent.verticalCenter

        width : pGetWidth()
        height: width

        iconWidth : Math.round(width / 1.8)
        iconHeight: iconWidth

        borderSize: st.border_sizeFocus

        visible: (playerTab == tab) ? player.hasPreviousTrack
                                    : tab   .hasPreviousTrack

        isHovered: (containsMouse || areaBackward.containsMouse)

        isPressed: (pressed || isReturnPressed || areaBackward.pressed || pBackward)

        icon       : st.icon_backward
        iconScaling: true

        cursor: Qt.PointingHandCursor

        onPressed : scrollPrevious()
        onReleased: scrollClear   ()
    }

    ButtonPushOverlay
    {
        id: buttonNext

        anchors.right: parent.right

        anchors.rightMargin: st.dp8

        anchors.verticalCenter: parent.verticalCenter

        width : buttonPrevious.width
        height: buttonPrevious.height

        iconWidth : buttonPrevious.iconWidth
        iconHeight: buttonPrevious.iconHeight

        borderSize: buttonPrevious.borderSize

        visible: (playerTab == tab) ? player.hasNextTrack
                                    : tab   .hasNextTrack

        isHovered: (containsMouse || areaForward.containsMouse)

        isPressed: (pressed || isReturnPressed || areaForward.pressed || pForward)

        icon       : st.icon_forward
        iconScaling: true

        cursor: Qt.PointingHandCursor

        onPressed : scrollNext ()
        onReleased: scrollClear()
    }

    ButtonPushOverlay
    {
        id: buttonPlay

        anchors.centerIn: parent

        width : buttonPrevious.width
        height: buttonPrevious.height

        iconWidth : buttonPrevious.iconWidth
        iconHeight: buttonPrevious.iconHeight

        borderSize: buttonPrevious.borderSize

        visible: (loading || enabled)

        enabled: (tab != null && tab.isValid)

        isHovered: (containsMouse || areaPlay.containsMouse)

        isPressed: (pressed || isReturnPressed || areaPlay.pressed)

        icon       : st.icon_play
        iconScaling: true

        cursor: Qt.PointingHandCursor

        itemIcon.visible: (loading == false)

        onClicked: play()

        IconLoading
        {
            id: itemLoading

            anchors.centerIn: parent

            width : Math.round(buttonPlay.width / 1.6)
            height: width

            visible: loading

            scaling: true

            filterDefault: st.labelRound_filterIcon
        }
    }

    Item
    {
        id: barTitle

        anchors.left : parent.left
        anchors.right: parent.right

        height: st.dp32

        visible: (buttonPlay.visible && title)

        Behavior on opacity
        {
            PropertyAnimation
            {
                duration: st.duration_fast

                easing.type: st.easing
            }
        }

        Rectangle
        {
            anchors.fill: parent

            opacity: st.playerBrowser_opacity

            color: st.playerBrowser_colorBar
        }

        TextLink
        {
            id: itemTitle

            anchors.fill: parent

            leftMargin : st.dp8
            rightMargin: st.dp8

            verticalAlignment: Text.AlignVCenter

            enabled: enableTitle

            text: (tab) ? tab.title : ""

            color: (isHovered) ? st.playerBrowser_colorTitleHover
                               : st.playerBrowser_colorTitle

            style: st.text_raised

            font.pixelSize: st.dp16

            onClicked: titleClicked(mouse)
        }
    }

    LabelLoadingText
    {
        anchors.bottom: parent.bottom

        anchors.horizontalCenter: parent.horizontalCenter

        visible: (tab && tab.isLoading)

        text: qsTr("Loading...")
    }

    Item
    {
        id: barDetails

        anchors.left  : parent.left
        anchors.right : parent.right
        anchors.bottom: parent.bottom

        height: st.dp32

        visible: (tab && tab.isLoaded)

        Behavior on opacity
        {
            PropertyAnimation
            {
                duration: st.duration_fast

                easing.type: st.easing
            }
        }

        Rectangle
        {
            anchors.fill: parent

            opacity: st.playerBrowser_opacity

            color: st.playerBrowser_colorBar
        }

        TextBase
        {
            id: itemDuration

            anchors.left  : parent.left
            anchors.top   : parent.top
            anchors.bottom: parent.bottom

            leftMargin: st.dp8

            verticalAlignment: Text.AlignVCenter

            text: (tab) ? gui.getTrackDuration(tab.duration) : ""

            color: st.playerBrowser_colorText

            style: st.text_raised

            font.pixelSize: st.dp14
        }

        TextDate
        {
            anchors.left  : itemDuration.right
            anchors.top   : parent.top
            anchors.bottom: parent.bottom

            leftMargin: st.dp4
            topMargin : st.dp1

            horizontalAlignment: Text.AlignRight
            verticalAlignment  : Text.AlignVCenter

            width: Math.max(0, itemAuthor.width - itemAuthor.mouseArea.width - st.dp8)

            date: (tab) ? tab.date : null

            color: (isHovered) ? st.playerBrowser_colorTextHover
                               : st.playerBrowser_colorText

            style: st.text_raised
        }

        TextLink
        {
            id: itemAuthor

            anchors.left  : itemDuration.right
            anchors.right : parent.right
            anchors.top   : parent.top
            anchors.bottom: parent.bottom

            leftMargin : st.dp4
            rightMargin: st.dp8

            horizontalAlignment: Text.AlignRight
            verticalAlignment  : Text.AlignVCenter

            enabled: enableAuthor

            text: (tab) ? gui.getTrackAuthor(tab.author, tab.feed) : ""

            color: (isHovered) ? st.playerBrowser_colorTextHover
                               : st.playerBrowser_colorText

            style: st.text_raised

            font.pixelSize: st.dp14

            onClicked: authorClicked(mouse)
        }
    }
}
