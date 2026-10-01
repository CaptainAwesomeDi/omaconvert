import QtQuick

// One keypad button, lifted from Omacalc's CalcButton: numbers sit almost
// flush with the page, functions (backspace, clear, sign) lift a step
// lighter, and the primary key — swap — inverts to the ink color.
Rectangle {
    id: control

    property string label
    property string keyValue: label
    property string kind: "number" // "number", "function" or "primary"
    property string iconName
    property color pageColor: "#101010"
    property color inkColor: "#eeeeee"

    signal activated()

    Accessible.role: Accessible.Button
    Accessible.name: {
        if (iconName === "backspace") return "Backspace";
        if (iconName === "swap") return "Swap units";
        if (label === "AC") return "All clear";
        if (label === "±") return "Toggle sign";
        if (label === ".") return "Decimal point";
        return label;
    }
    Accessible.onPressAction: activated()

    function mixColors(base, tint, amount) {
        return Qt.rgba(
            base.r + (tint.r - base.r) * amount,
            base.g + (tint.g - base.g) * amount,
            base.b + (tint.b - base.b) * amount, 1);
    }

    readonly property real restingLift: kind === "function" ? 0.16 : 0.05
    readonly property real activeLift: restingLift
        + (hitArea.pressed ? 0.09 : (hitArea.containsMouse ? 0.045 : 0))

    radius: Math.min(14, height * 0.18)
    color: kind === "primary"
        ? mixColors(inkColor, pageColor, hitArea.pressed ? 0.22 : (hitArea.containsMouse ? 0.1 : 0))
        : mixColors(pageColor, inkColor, activeLift)
    border.width: kind === "number" ? 1 : 0
    border.color: mixColors(pageColor, inkColor, 0.13)

    Text {
        anchors.centerIn: parent
        visible: control.iconName === ""
        text: control.label
        color: control.kind === "primary" ? control.pageColor : control.inkColor
        font.family: "iA Writer Mono S"
        font.pixelSize: Math.round(Math.min(parent.height * 0.42, parent.width * 0.3))
    }

    Canvas {
        id: backspaceIcon
        anchors.centerIn: parent
        width: Math.round(Math.min(parent.height * 0.42, parent.width * 0.3) * 1.3)
        height: Math.round(width * 0.72)
        visible: control.iconName === "backspace"

        onPaint: {
            var context = getContext("2d");
            var w = width;
            var h = height;
            var notch = w * 0.28;
            context.clearRect(0, 0, w, h);
            context.strokeStyle = String(control.inkColor);
            context.lineWidth = Math.max(1.4, w * 0.07);
            context.lineCap = "round";
            context.lineJoin = "round";

            // The key cap: a rectangle whose left edge tapers to a point.
            context.beginPath();
            context.moveTo(notch, 1);
            context.lineTo(w - 1, 1);
            context.lineTo(w - 1, h - 1);
            context.lineTo(notch, h - 1);
            context.lineTo(1, h / 2);
            context.closePath();
            context.stroke();

            // The x inside.
            var cx = notch + (w - notch) / 2;
            var cy = h / 2;
            var arm = h * 0.18;
            context.beginPath();
            context.moveTo(cx - arm, cy - arm);
            context.lineTo(cx + arm, cy + arm);
            context.moveTo(cx + arm, cy - arm);
            context.lineTo(cx - arm, cy + arm);
            context.stroke();
        }

        Connections {
            target: control
            function onInkColorChanged() { backspaceIcon.requestPaint(); }
            function onWidthChanged() { backspaceIcon.requestPaint(); }
            function onHeightChanged() { backspaceIcon.requestPaint(); }
        }
    }

    Canvas {
        id: swapIcon
        anchors.centerIn: parent
        width: Math.round(Math.min(parent.height * 0.42, parent.width * 0.3) * 1.3)
        height: width
        visible: control.iconName === "swap"

        onPaint: {
            var context = getContext("2d");
            var w = width;
            var h = height;
            context.clearRect(0, 0, w, h);
            // The primary key inverts to ink, so its icon draws in page color;
            // every other swap button draws in ink like the backspace icon.
            context.strokeStyle = String(control.kind === "primary"
                                         ? control.pageColor : control.inkColor);
            context.lineWidth = Math.max(1.4, w * 0.08);
            context.lineCap = "round";
            context.lineJoin = "round";

            // Two opposing arrows: the answer trades places with the input.
            var inset = w * 0.24;
            var head = w * 0.16;
            var x1 = w / 2 - inset / 2;
            var x2 = w / 2 + inset / 2;

            context.beginPath();
            context.moveTo(x1, h * 0.75);
            context.lineTo(x1, h * 0.25);
            context.moveTo(x1 - head, h * 0.25 + head);
            context.lineTo(x1, h * 0.25);
            context.lineTo(x1 + head, h * 0.25 + head);
            context.stroke();

            context.beginPath();
            context.moveTo(x2, h * 0.25);
            context.lineTo(x2, h * 0.75);
            context.moveTo(x2 - head, h * 0.75 - head);
            context.lineTo(x2, h * 0.75);
            context.lineTo(x2 + head, h * 0.75 - head);
            context.stroke();
        }

        Connections {
            target: control
            function onInkColorChanged() { swapIcon.requestPaint(); }
            function onPageColorChanged() { swapIcon.requestPaint(); }
            function onWidthChanged() { swapIcon.requestPaint(); }
            function onHeightChanged() { swapIcon.requestPaint(); }
        }
    }

    MouseArea {
        id: hitArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: control.activated()
    }
}
