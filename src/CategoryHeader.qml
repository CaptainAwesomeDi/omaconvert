import QtQuick

// The category button at the top of the face: a bordered chip like the unit
// chips, so it reads as clickable and opens the category grid.
Item {
    id: header

    property string name
    property color pageColor: "#101010"
    property color inkColor: "#eeeeee"
    property int pixelSize: 18
    property int chipHeight: 36

    signal activated()

    function mixColors(base, tint, amount) {
        return Qt.rgba(
            base.r + (tint.r - base.r) * amount,
            base.g + (tint.g - base.g) * amount,
            base.b + (tint.b - base.b) * amount, 1);
    }

    implicitWidth: label.implicitWidth + height * 0.9
    implicitHeight: chipHeight

    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Choose category") + ": " + name
    Accessible.onPressAction: activated()

    Rectangle {
        anchors.fill: parent
        radius: Math.min(12, height * 0.3)
        color: header.mixColors(header.pageColor, header.inkColor,
                                hit.pressed ? 0.14
                                  : (hit.containsMouse ? 0.09 : 0.05))
        border.width: 1
        border.color: header.mixColors(header.pageColor, header.inkColor, 0.13)

        Text {
            id: label
            anchors.centerIn: parent
            text: header.name + " ▾"
            color: header.inkColor
            font.family: "iA Writer Mono S"
            font.pixelSize: header.pixelSize
        }

        MouseArea {
            id: hit
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: header.activated()
        }
    }
}
