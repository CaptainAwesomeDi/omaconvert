import QtQuick
import QtQml
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: win
    width: 400
    height: 568
    minimumWidth: 300
    minimumHeight: 430
    visible: true
    title: "Omaunits"

    readonly property bool darkMode: theme.darkMode
    readonly property color pageColor: theme.background
    readonly property color inkColor: theme.foreground
    // Every hardcoded size in the interface is expressed at the 400 × 568
    // design size; resizing the window scales the whole face with it.
    readonly property real uiScale: Math.min(width / 400, height / 568)
    // Snapshot, not binding: the resize ratio must compare against the last
    // scale we actually applied, or the first portal update resizes by 1.
    property real appliedTextScale: 1

    readonly property var categoryOrder: ["weight", "length", "temperature",
                                           "volume", "data"]
    property string pickerMode: "units"
    property string pickerSide: "to"

    Connections {
        target: backend

        function onTextScaleChanged() {
            var factor = backend.textScale / win.appliedTextScale;
            win.appliedTextScale = backend.textScale;
            if (win.visibility === Window.Windowed) {
                win.width = Math.round(win.width * factor);
                win.height = Math.round(win.height * factor);
            }
        }
    }

    function mixColors(base, tint, amount) {
        return Qt.rgba(
            base.r + (tint.r - base.r) * amount,
            base.g + (tint.g - base.g) * amount,
            base.b + (tint.b - base.b) * amount, 1);
    }
    readonly property color mutedColor: mixColors(pageColor, inkColor, 0.5)

    function scaledSize(pixels) {
        return Math.max(1, Math.round(pixels * uiScale));
    }

    Material.theme: darkMode ? Material.Dark : Material.Light
    Material.accent: theme.accent
    color: pageColor

    // Clipboard shortcuts stand down while the picker's filter field has
    // focus, so Ctrl+C/V act on the selected text there, not on the result.
    Shortcut {
        sequences: ["Ctrl+C", "Meta+C"]
        context: Qt.ApplicationShortcut
        enabled: !picker.opened
        onActivated: backend.copyResult()
    }

    Shortcut {
        sequence: "Ctrl+Shift+C"
        context: Qt.ApplicationShortcut
        enabled: !picker.opened
        onActivated: backend.copyStatement()
    }

    Shortcut {
        sequences: ["Ctrl+V", "Meta+V"]
        context: Qt.ApplicationShortcut
        enabled: !picker.opened
        onActivated: backend.pasteNumber()
    }

    Shortcut {
        sequence: "Ctrl+Q"
        context: Qt.ApplicationShortcut
        onActivated: win.close()
    }

    // Shortcut is a QObject, so Instantiator (not Repeater) carries them.
    // Like the clipboard shortcuts, they stand down while a picker is open,
    // or committing would switch category under a stale unit list.
    Instantiator {
        model: win.categoryOrder

        Shortcut {
            required property int index
            required property string modelData
            sequence: "Ctrl+" + (index + 1)
            context: Qt.ApplicationShortcut
            enabled: !picker.opened
            onActivated: backend.selectCategory(modelData)
        }
    }

    function openUnitPicker(side) {
        win.pickerMode = "units"
        win.pickerSide = side
        backend.setFocusedSide(side)  // Mouse and arrows agree on the side.
        picker.gridMode = false
        picker.showFilter = backend.unitsModel().length > 12
        picker.preferredWidth = win.scaledSize(300)  // Symbol + name rows.
        picker.currentId = side === "from" ? backend.fromId : backend.toId
        picker.provider = function(filter) { return backend.filteredUnits(filter); }
        picker.open()
    }

    function openCategoryPicker() {
        win.pickerMode = "categories"
        picker.gridMode = true
        picker.showFilter = false  // Five names; a filter would be noise.
        picker.preferredWidth = win.scaledSize(230)  // One word per row.
        picker.currentId = backend.categoryId
        picker.provider = function(filter) {
            var all = backend.categoriesModel();
            var query = filter.trim().toLowerCase();
            if (!query)
                return all;
            return all.filter(function(entry) {
                return entry.name.toLowerCase().indexOf(query) === 0;
            });
        }
        picker.open()
    }

    Item {
        id: face
        anchors.fill: parent
        anchors.margins: win.scaledSize(20)
        focus: true

        Keys.onPressed: function(event) {
            if (event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier))
                return;

            if (event.key === Qt.Key_Backspace) {
                backend.pressKey("backspace");
            } else if (event.key === Qt.Key_Escape || event.key === Qt.Key_Delete) {
                backend.pressKey("clear");
            } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                       || event.text === "=") {
                backend.copyResult();
            } else if (event.key === Qt.Key_Left) {
                backend.cycleCategory(-1);
            } else if (event.key === Qt.Key_Right) {
                backend.cycleCategory(1);
            } else if (event.key === Qt.Key_Up) {
                // Bare arrows follow the focus mark; Shift always means the
                // input side, matching the published keyboard map.
                backend.cycleUnit(-1, event.modifiers & Qt.ShiftModifier
                                  ? "from" : backend.focusedSide);
            } else if (event.key === Qt.Key_Down) {
                backend.cycleUnit(1, event.modifiers & Qt.ShiftModifier
                                  ? "from" : backend.focusedSide);
            } else if (event.key === Qt.Key_Space) {
                // Space picks the focused side; Shift goes to the other.
                var side = backend.focusedSide;
                if (event.modifiers & Qt.ShiftModifier)
                    side = side === "to" ? "from" : "to";
                win.openUnitPicker(side);
            } else if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
                backend.setFocusedSide(backend.focusedSide === "to" ? "from" : "to");
            } else if (event.text === "," || event.text === ".") {
                backend.pressKey(".");
            } else if (event.text === "c" || event.text === "C") {
                backend.pressKey("clear");
            } else if (event.text === "s" || event.text === "S"
                       || event.text === "-") {
                backend.pressKey("sign");
            } else if (event.text === "x" || event.text === "X") {
                backend.swap();
            } else if (/^[0-9]$/.test(event.text)) {
                backend.pressKey(event.text);
            } else {
                return;
            }
            event.accepted = true;
        }

        Item {
            id: headerRow
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: categoryHeader.height + rateNote.height + win.scaledSize(6)

            CategoryHeader {
                id: categoryHeader
                anchors.top: parent.top
                anchors.left: parent.left
                name: backend.categoryName
                pixelSize: win.scaledSize(17)
                chipHeight: win.scaledSize(36)
                pageColor: win.pageColor
                inkColor: win.inkColor
                onActivated: win.openCategoryPicker()
            }

            // The conversion teacher line gets a full-width row of its own
            // so it never fights the category button for space.
            Text {
                id: rateNote
                anchors.top: categoryHeader.bottom
                anchors.topMargin: win.scaledSize(6)
                anchors.left: parent.left
                anchors.right: parent.right
                horizontalAlignment: Text.AlignRight
                text: backend.rateNote
                color: win.mutedColor
                elide: Text.ElideLeft
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
            }
        }

        Rectangle {
            id: divider
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: keypad.top
            anchors.bottomMargin: win.scaledSize(14)
            height: 1
            color: win.mixColors(win.pageColor, win.inkColor, 0.16)
        }

        // The conversion block, in Omacalc's reading order: the muted
        // context above, and the number you are typing as the hero at the
        // bottom, next to the keypad that feeds it. The result sits above
        // the seam; the input below it. Bounded by the divider, so nothing
        // ever crowds the keypad.
        Item {
            id: valueArea
            anchors.top: headerRow.bottom
            anchors.topMargin: win.scaledSize(12)
            anchors.bottom: divider.top
            anchors.bottomMargin: win.scaledSize(10)
            anchors.left: parent.left
            anchors.right: parent.right

            ValueRow {
                id: toRow
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: win.scaledSize(48)
                chipWidth: win.scaledSize(110)
                chipHeight: win.scaledSize(38)
                symbol: backend.toSymbol
                unitName: backend.toName
                valueText: backend.display
                chipFocused: backend.focusedSide === "to"
                pageColor: win.pageColor
                inkColor: win.inkColor
                valueColor: win.mixColors(win.pageColor, win.inkColor, 0.72)
                valuePixelSize: win.scaledSize(30)
                minimumValuePixelSize: win.scaledSize(16)
                fitValue: true
                onChipActivated: win.openUnitPicker("to")
            }

            Item {
                id: seam
                anchors.top: toRow.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                implicitHeight: win.scaledSize(36)

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    height: 1
                    color: win.mixColors(win.pageColor, win.inkColor, 0.16)
                }

                // The visible swap affordance rides the chip axis — the ⇅
                // sits directly between the two unit chips it trades, with
                // air on both sides so nothing touches.
                ConvertButton {
                    anchors.centerIn: parent
                    width: win.scaledSize(32)
                    height: win.scaledSize(32)
                    label: ""
                    keyValue: "swap"
                    kind: "function"
                    iconName: "swap"
                    pageColor: win.pageColor
                    inkColor: win.inkColor
                    onActivated: backend.swap()
                }
            }

            ValueRow {
                id: fromRow
                anchors.top: seam.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                chipWidth: win.scaledSize(110)
                chipHeight: win.scaledSize(38)
                symbol: backend.fromSymbol
                unitName: backend.fromName
                valueText: backend.inputDisplay
                chipFocused: backend.focusedSide === "from"
                pageColor: win.pageColor
                inkColor: win.inkColor
                valuePixelSize: win.scaledSize(58)
                minimumValuePixelSize: win.scaledSize(18)
                fitValue: true
                valueAreaClickable: false  // The input number is not a picker.
                onChipActivated: win.openUnitPicker("from")
            }
        }

        // The pad stays pure digits for the whole session — Omacalc's
        // contract — with swap taking the inverted bottom-right key.
        GridLayout {
            id: keypad
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: Math.round(parent.height * 0.54)
            columns: 4
            rowSpacing: win.scaledSize(12)
            columnSpacing: win.scaledSize(12)

            Repeater {
                model: [
                    { label: "7", key: "7", kind: "number" },
                    { label: "8", key: "8", kind: "number" },
                    { label: "9", key: "9", kind: "number" },
                    { label: "", key: "backspace", kind: "function", icon: "backspace" },
                    { label: "4", key: "4", kind: "number" },
                    { label: "5", key: "5", kind: "number" },
                    { label: "6", key: "6", kind: "number" },
                    { label: "AC", key: "clear", kind: "function" },
                    { label: "1", key: "1", kind: "number" },
                    { label: "2", key: "2", kind: "number" },
                    { label: "3", key: "3", kind: "number" },
                    { label: "±", key: "sign", kind: "function" },
                    { label: "0", key: "0", kind: "number" },
                    { label: ".", key: ".", kind: "number" },
                    { label: "⇅", key: "swap", kind: "primary", icon: "swap", span: 2 }
                ]

                ConvertButton {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.columnSpan: modelData.span === undefined ? 1 : modelData.span
                    label: modelData.label
                    keyValue: modelData.key
                    kind: modelData.kind
                    iconName: modelData.icon === undefined ? "" : modelData.icon
                    pageColor: win.pageColor
                    inkColor: win.inkColor
                    onActivated: {
                        if (keyValue === "swap")
                            backend.swap();
                        else
                            backend.pressKey(keyValue);
                    }
                }
            }
        }
    }

    UnitPicker {
        id: picker
        parent: Overlay.overlay
        x: Math.round((win.width - width) / 2)
        y: win.scaledSize(96)
        width: Math.min(face.width, picker.preferredWidth)
        maxHeight: win.height - win.scaledSize(192)
        uiScale: win.uiScale
        pageColor: win.pageColor
        inkColor: win.inkColor
        accentColor: theme.accent
        placeholderText: backend.filterPlaceholder

        onChosen: function(id) {
            if (win.pickerMode === "categories")
                backend.selectCategory(id);
            else
                backend.selectUnit(win.pickerSide, id);
        }
    }

    // Remember the last windowed geometry rather than whatever the window
    // happens to measure at teardown: a maximized window reports screen-sized
    // dimensions, and the close sequence hides the window before destruction.
    property rect normalGeometry: Qt.rect(x, y, width, height)
    property bool wasMaximized: false

    function trackNormalGeometry() {
        if (visibility === Window.Windowed)
            normalGeometry = Qt.rect(x, y, width, height);
    }

    onXChanged: trackNormalGeometry()
    onYChanged: trackNormalGeometry()
    onWidthChanged: trackNormalGeometry()
    onHeightChanged: trackNormalGeometry()

    onVisibilityChanged: {
        if (visibility === Window.Maximized || visibility === Window.FullScreen)
            wasMaximized = true;
        else if (visibility === Window.Windowed)
            wasMaximized = false;
    }

    Component.onCompleted: {
        appliedTextScale = backend.textScale;

        // --screenshot renders the canonical first-run face at design size;
        // interactive launches restore the remembered geometry instead.
        if (screenshotPath !== "") {
            width = 400;
            height = 568;
            if (screenshotOpen === "category")
                openCategoryPicker();
            else if (screenshotOpen === "units-from")
                openUnitPicker("from");
            else if (screenshotOpen === "units-to")
                openUnitPicker("to");
        } else {
            var geometry = backend.windowGeometry();
            if (geometry.valid) {
                x = geometry.x;
                y = geometry.y;
                width = geometry.width;
                height = geometry.height;
                if (geometry.maximized) showMaximized();
            } else {
                // First run: open at the design size, grown by the desktop text scale.
                width = Math.round(400 * backend.textScale);
                height = Math.round(568 * backend.textScale);
            }
        }

        // Headless smoke: --smoke opens both pickers and reports their state.
        // `visible` flips synchronously in open(); `opened` waits out the
        // transition, which never runs before Qt.quit().
        // --smoke-open=<category|units-from|units-to> opens one picker and
        // keeps the app running, for visual inspection.
        var openArg = Qt.application.arguments.find(
            function(arg) { return arg.indexOf("--smoke-open=") === 0; });
        if (openArg !== undefined) {
            var what = openArg.substring("--smoke-open=".length);
            if (what === "category")
                openCategoryPicker();
            else
                openUnitPicker(what === "units-to" ? "to" : "from");
        } else if (Qt.application.arguments.indexOf("--smoke") !== -1) {
            openUnitPicker("from");
            var unitOk = picker.visible && picker.entries.length > 0;
            var unitEntries = picker.entries.length;
            picker.close();
            openCategoryPicker();
            var categoryOk = picker.visible && picker.entries.length > 0;
            var categoryEntries = picker.entries.length;
            backend.smokeReport("unit " + unitOk + " " + unitEntries
                                + " category " + categoryOk + " " + categoryEntries);
            Qt.quit();
        }
    }

    Component.onDestruction: backend.saveWindowGeometry(
        normalGeometry.x, normalGeometry.y,
        normalGeometry.width, normalGeometry.height, wasMaximized)
}
