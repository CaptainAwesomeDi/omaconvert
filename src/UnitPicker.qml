import QtQuick
import QtQuick.Controls

// The single picker mechanism: everything is directly clickable, arrows and
// Enter/=/Esc work from any state. A filter field only exists for lists too
// long to scan — with every shipped category at nine units or fewer it
// never shows, but it stays wired for a future longer list. The provider
// function supplies the ranked entries (units are ranked in C++, where the
// ranking is tested).
Popup {
    id: picker

    property var provider: null      // function(filterText) => [{id, symbol, name}]
    property string currentId: ""
    property bool gridMode: false
    property bool showFilter: false  // Only lists beyond a screenful earn one.
    property real preferredWidth: 300
    property color pageColor: "#101010"
    property color inkColor: "#eeeeee"
    property color accentColor: "#5584aa"
    property string placeholderText: ""
    property real uiScale: 1
    property real maxHeight: 360

    signal chosen(string id)

    modal: true
    dim: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 12

    function mixColors(base, tint, amount) {
        return Qt.rgba(
            base.r + (tint.r - base.r) * amount,
            base.g + (tint.g - base.g) * amount,
            base.b + (tint.b - base.b) * amount, 1);
    }

    function scaledSize(pixels) {
        return Math.max(1, Math.round(pixels * uiScale));
    }

    function refresh() {
        entries = provider ? provider(filter.text) : [];
        if (gridMode) {
            flow.currentIndex = 0;
            for (var i = 0; i < entries.length; ++i) {
                if (entries[i].id === picker.currentId) {
                    flow.currentIndex = i;
                    break;
                }
            }
        } else {
            list.currentIndex = 0;
            for (var j = 0; j < entries.length; ++j) {
                if (entries[j].id === picker.currentId) {
                    list.currentIndex = j;
                    break;
                }
            }
            placeSelection();
        }
    }

    // The selection must stay on screen while it moves: a long unit list
    // arrowing off the viewport hides the highlight. The pill flow always
    // fits, so only the unit list needs scrolling into view.
    function placeSelection() {
        if (!gridMode && list.currentIndex >= 0 && list.count > 0)
            list.positionViewAtIndex(list.currentIndex, ListView.Contain);
    }

    function commit(id) {
        chosen(id);
        close();
    }

    function moveSelection(key) {
        if (gridMode) {
            var delta = (key === Qt.Key_Down || key === Qt.Key_Right) ? 1
                      : (key === Qt.Key_Up || key === Qt.Key_Left) ? -1 : 0;
            if (delta !== 0 && entries.length > 0)
                flow.currentIndex = (flow.currentIndex + delta + entries.length)
                                    % entries.length;
        } else {
            if (key === Qt.Key_Down) list.incrementCurrentIndex();
            else if (key === Qt.Key_Up) list.decrementCurrentIndex();
            placeSelection();
        }
    }

    function commitCurrent() {
        if (gridMode) {
            if (flow.currentIndex >= 0 && flow.currentIndex < entries.length)
                commit(entries[flow.currentIndex].id);
        } else {
            if (list.currentIndex >= 0 && list.currentIndex < entries.length)
                commit(entries[list.currentIndex].id);
        }
    }

    function handleKey(event) {
        if (event.key === Qt.Key_Down || event.key === Qt.Key_Up
                || event.key === Qt.Key_Left || event.key === Qt.Key_Right) {
            moveSelection(event.key);
            event.accepted = true;
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                   || event.text === "=") {
            commitCurrent();
            event.accepted = true;
        } else if (event.key === Qt.Key_Escape) {
            close();
            event.accepted = true;
        }
    }

    property var entries: []

    onOpened: {
        filter.clear();
        if (showFilter)
            filter.forceActiveFocus();
        else
            column.forceActiveFocus();
        refresh();
    }

    onProviderChanged: refresh()

    background: Rectangle {
        color: picker.mixColors(picker.pageColor, picker.inkColor, 0.04)
        border.width: 1
        border.color: picker.mixColors(picker.pageColor, picker.inkColor, 0.2)
        radius: picker.scaledSize(14)
    }

    contentItem: Column {
        id: column
        spacing: picker.scaledSize(8)

        Keys.onPressed: function(event) {
            if (!picker.showFilter)
                picker.handleKey(event);
        }

        TextField {
            id: filter
            visible: picker.showFilter
            width: parent.width
            padding: picker.scaledSize(10)
            font.family: "iA Writer Mono S"
            font.pixelSize: picker.scaledSize(16)
            color: picker.inkColor
            placeholderTextColor: picker.mixColors(picker.pageColor, picker.inkColor, 0.5)
            selectionColor: picker.accentColor
            selectedTextColor: picker.pageColor
            placeholderText: picker.placeholderText
            background: Rectangle {
                radius: picker.scaledSize(10)
                color: picker.mixColors(picker.pageColor, picker.inkColor, 0.07)
                border.width: 1
                border.color: picker.mixColors(picker.pageColor, picker.inkColor, 0.16)
            }
            onTextChanged: picker.refresh()
            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Escape) {
                    picker.close();
                    event.accepted = true;
                    return;
                }
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                        || event.text === "=" || event.key === Qt.Key_Down
                        || event.key === Qt.Key_Up || event.key === Qt.Key_Left
                        || event.key === Qt.Key_Right) {
                    // Arrows move the selection instead of the caret; the
                    // text field keeps every other editing key.
                    picker.handleKey(event);
                }
            }
        }

        // Categories are a single-column vertical list: one full-width row
        // per name, nothing truncated, no multi-row wrap to scan.
        Flow {
            id: flow
            visible: picker.gridMode
            width: parent.width
            spacing: picker.scaledSize(8)
            property int currentIndex: 0

            Repeater {
                model: picker.entries

                delegate: Rectangle {
                    id: pill
                    required property var modelData
                    required property int index
                    width: flow.width
                    height: picker.scaledSize(44)
                    radius: Math.min(12, height * 0.3)
                    color: pill.index === flow.currentIndex
                        ? picker.inkColor
                        : (pillHit.containsMouse
                              ? picker.mixColors(picker.pageColor, picker.inkColor, 0.12)
                              : picker.mixColors(picker.pageColor, picker.inkColor, 0.07))
                    border.width: pill.index === flow.currentIndex ? 0 : 1
                    border.color: picker.mixColors(picker.pageColor, picker.inkColor, 0.13)

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        text: pill.modelData.name
                        color: pill.index === flow.currentIndex
                            ? picker.pageColor : picker.inkColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: picker.scaledSize(16)
                    }

                    MouseArea {
                        id: pillHit
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: picker.commit(pill.modelData.id)
                    }

                    Accessible.role: Accessible.Button
                    Accessible.name: pill.modelData.name
                    Accessible.onPressAction: picker.commit(pill.modelData.id)
                }
            }
        }

        ListView {
            id: list
            visible: !picker.gridMode
            width: parent.width
            height: Math.min(contentHeight,
                             picker.maxHeight
                                 - (picker.showFilter ? filter.height + column.spacing : 0))
            clip: true
            spacing: 2
            model: picker.entries

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Item {
                id: delegate
                required property var modelData
                required property int index
                width: list.width
                height: picker.scaledSize(38)
                visible: height > 0

                Rectangle {
                    anchors.fill: parent
                    radius: picker.scaledSize(9)
                    color: delegate.index === list.currentIndex
                        ? picker.mixColors(picker.pageColor, picker.inkColor, 0.12)
                        : (rowHit.containsMouse
                              ? picker.mixColors(picker.pageColor, picker.inkColor, 0.06)
                              : "transparent")
                }

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: picker.scaledSize(12)
                    anchors.verticalCenter: parent.verticalCenter
                    text: delegate.modelData.symbol === undefined
                          ? "" : delegate.modelData.symbol
                    color: picker.inkColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: picker.scaledSize(17)
                }

                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: picker.scaledSize(12)
                    anchors.left: parent.left
                    anchors.leftMargin: picker.scaledSize(12) + picker.scaledSize(90)
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideLeft
                    text: delegate.modelData.name === undefined
                          ? "" : delegate.modelData.name
                    color: picker.mixColors(picker.pageColor, picker.inkColor, 0.55)
                    font.family: "iA Writer Mono S"
                    font.pixelSize: picker.scaledSize(15)
                }

                MouseArea {
                    id: rowHit
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: picker.commit(delegate.modelData.id)
                }

                Accessible.role: Accessible.Button
                Accessible.name: (delegate.modelData.symbol === undefined ? ""
                              : delegate.modelData.symbol + " ")
                             + (delegate.modelData.name === undefined ? ""
                                : delegate.modelData.name)
                Accessible.onPressAction: picker.commit(delegate.modelData.id)
            }
        }
    }
}
