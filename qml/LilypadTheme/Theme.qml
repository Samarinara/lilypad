pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property color darkEmerald: "#066633"
    readonly property color petalFrost: "#f8d1e8"
    readonly property color softLinen: "#e8ebe4"
    readonly property color balticBlue: "#145c9e"
    readonly property color shadowGrey: "#211a1d"
    readonly property color mutedBalticBlue: "#0f4070"

    readonly property color lightBg: softLinen
    readonly property color lightSurface: "#f5f5f5"
    readonly property color lightHover: petalFrost
    readonly property color lightActive: "#d0d0d0"
    readonly property color lightBorder: "#ddd"
    readonly property color lightText: darkBg    // Light mode text = dark mode background
    readonly property color lightUrlBar: "#ffffff"

    readonly property color darkBg: shadowGrey
    readonly property color darkSurface: "#2a2226"
    readonly property color darkHover: mutedBalticBlue
    readonly property color darkActive: "#3a3236"
    readonly property color darkBorder: "#444"
    readonly property color darkText: lightBg    // Dark mode text = light mode background
    readonly property color darkUrlBar: shadowGrey

    readonly property color focusBorder: darkEmerald
    readonly property color destructive: "#ff5555"
}
