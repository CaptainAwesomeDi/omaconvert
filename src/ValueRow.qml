import QtQuick

// One side of the conversion: a unit chip on the left and the value on the
// right. Chips share a fixed column so the two sides align, and the focused
// chip inverts, marking which side the arrow keys change.
Item {
    id: row

    property string symbol
    property string unitName  // Full name for assistive technology.
    property string valueText
    property bool chipFocused: false
    property int chipWidth: 96
    property int chipHeight: 38
    property color pageColor: "#101010"
    property color inkColor: "#eeeeee"
    property color valueColor: inkColor
    property int valuePixelSize: 30
    property int minimumValuePixelSize: 18
    property int valueMargin: 12
    property bool fitValue: false
    property bool valueAreaClickable: true

    signal chipActivated()

    function mixColors(base, tint, amount) {
        return Qt.rgba(
            base.r + (tint.r - base.r) * amount,
            base.g + (tint.g - base.g) * amount,
            base.b + (tint.b - base.b) * amount, 1);
    }

    Accessible.role: Accessible.Button
    Accessible.name: unitName ? qsTr("Select unit") + ": " + unitName
                             : (symbol ? qsTr("Select unit") + ": " + symbol
                                       : qsTr("Select unit"))
    Accessible.onPressAction: chipActivated()

    Rectangle {
        id: chip
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: row.chipWidth
        height: row.chipHeight
        radius: Math.min(12, height * 0.3)
        color: row.chipFocused
            ? row.mixColors(row.inkColor, row.pageColor,
                            chipHit.pressed ? 0.22 : (chipHit.containsMouse ? 0.1 : 0))
            : row.mixColors(row.pageColor, row.inkColor,
                            0.16 + (chipHit.pressed ? 0.09
                                     : (chipHit.containsMouse ? 0.045 : 0)))
        border.width: row.chipFocused ? 0 : 1
        border.color: row.mixColors(row.pageColor, row.inkColor, 0.13)

        Text {
            id: chipLabel
            anchors.centerIn: parent
            text: row.symbol + " ▾"
            color: row.chipFocused ? row.pageColor : row.inkColor
            font.family: "iA Writer Mono S"
            font.pixelSize: Math.round(parent.height * 0.42)
        }

        MouseArea {
            id: chipHit
            anchors.fill: parent
            hoverEnabled: true
            onClicked: row.chipActivated()
        }
    }

    Text {
        id: value
        anchors.right: parent.right
        anchors.left: chip.right
        anchors.leftMargin: row.valueMargin
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        text: row.valueText
        color: row.valueColor
        elide: row.fitValue ? Text.ElideRight : Text.ElideLeft
        fontSizeMode: row.fitValue ? Text.HorizontalFit : Text.FixedSize
        minimumPixelSize: row.minimumValuePixelSize
        font.family: "iA Writer Mono S"
        font.pixelSize: row.valuePixelSize
    }

    // The result row's value side opens the picker too — a generous mouse
    // target for picking exactly what to measure.
    MouseArea {
        anchors.left: chip.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        enabled: row.valueAreaClickable
        hoverEnabled: row.valueAreaClickable
        cursorShape: Qt.PointingHandCursor
        onClicked: row.chipActivated()
    }
}
