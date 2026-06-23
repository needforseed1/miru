pragma Singleton

import QtQuick

QtObject {
    id: theme

    // ---- Surfaces ----------------------------------------------------------
    readonly property color bg: "#090a10"            // window base
    readonly property color bgElevated: "#0f111b"    // top/bottom bars
    readonly property color surface: "#161a26"       // cards
    readonly property color surfaceAlt: "#1b2031"    // inputs, secondary cards
    readonly property color surfaceHover: "#232a40"  // hovered cards
    readonly property color line: "#272d40"          // borders
    readonly property color lineStrong: "#3a4360"    // stronger borders

    // ---- Accent ------------------------------------------------------------
    readonly property color accent: "#6d7bff"
    readonly property color accentBright: "#9a86ff"
    readonly property color accentSoft: "#2a2f57"

    // ---- Text --------------------------------------------------------------
    readonly property color text: "#eef1fa"
    readonly property color textDim: "#aeb6d0"
    readonly property color textMute: "#737c98"

    // ---- Semantic ----------------------------------------------------------
    readonly property color gold: "#f3c64b"
    readonly property color success: "#43d39a"
    readonly property color danger: "#ff5d6c"

    // ---- Spacing -----------------------------------------------------------
    readonly property int s4: 4
    readonly property int s8: 8
    readonly property int s12: 12
    readonly property int s16: 16
    readonly property int s20: 20
    readonly property int s24: 24
    readonly property int s32: 32
    readonly property int s40: 40

    // ---- Radius ------------------------------------------------------------
    readonly property int rSm: 8
    readonly property int rMd: 12
    readonly property int rLg: 18
    readonly property int rXl: 26
    readonly property int rPill: 999

    // ---- Typography --------------------------------------------------------
    readonly property int fH1: 42
    readonly property int fH2: 30
    readonly property int fH3: 22
    readonly property int fTitle: 16
    readonly property int fBody: 14
    readonly property int fSmall: 12
    readonly property int fTiny: 11

    // ---- Motion ------------------------------------------------------------
    readonly property int durFast: 110
    readonly property int durMed: 180

    // ---- Fonts -------------------------------------------------------------
    // Monospace so AIOStreams' aligned name/description blocks line up.
    readonly property string monoFont: "monospace"

    // ---- Helpers -----------------------------------------------------------
    function qualityColor(quality) {
        switch (quality) {
        case "2160p": return "#f0a23c"   // amber for 4K
        case "1080p": return "#5b8cff"   // blue
        case "720p":  return "#46b6ff"   // light blue
        default:      return "#5a6280"   // muted
        }
    }

    function ratingColor(rating) {
        const r = parseFloat(rating)
        if (isNaN(r)) return textMute
        if (r >= 7.5) return success
        if (r >= 6.0) return gold
        return textDim
    }
}
