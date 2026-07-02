pragma Singleton

import QtQuick

QtObject {
    id: theme

    // ---- Surfaces ----------------------------------------------------------
    // "Obsidian" scale: near-black base with a cold blue undertone, surfaces
    // step up by adding faint blue-white light rather than plain gray.
    readonly property color bg: "#07070c"            // window base
    readonly property color bgElevated: "#0c0d15"    // top/bottom bars
    readonly property color surface: "#12131e"       // cards
    readonly property color surfaceAlt: "#181a29"    // inputs, secondary cards
    readonly property color surfaceHover: "#222439"  // hovered cards
    readonly property color line: "#20233a"          // borders
    readonly property color lineStrong: "#363b5c"    // stronger borders

    // Translucent chrome (header/footer) so content feels layered under it.
    readonly property color glassBar: "#f00a0b13"

    // ---- Accent ------------------------------------------------------------
    // Iris violet as the working accent; hot pink only as the far end of
    // brand gradients (progress, logo, loading bar) — never as flat fill.
    readonly property color accent: "#8460ff"
    readonly property color accentBright: "#ab92ff"
    readonly property color accent2: "#ff5e9a"
    readonly property color accentSoft: "#241d49"

    // ---- Text --------------------------------------------------------------
    readonly property color text: "#f2f3fb"
    readonly property color textDim: "#a7adc9"
    readonly property color textMute: "#666d8c"

    // ---- Semantic ----------------------------------------------------------
    readonly property color gold: "#f5c951"
    readonly property color success: "#3ddc97"
    readonly property color danger: "#ff5470"

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
    readonly property int rLg: 16
    readonly property int rXl: 24
    readonly property int rPill: 999

    // ---- Typography --------------------------------------------------------
    readonly property string sans: "Inter Variable"
    readonly property int fH1: 44
    readonly property int fH2: 32
    readonly property int fH3: 20
    readonly property int fTitle: 16
    readonly property int fBody: 14
    readonly property int fSmall: 12
    readonly property int fTiny: 11

    // Uppercase micro-label ("eyebrow") tracking, in px.
    readonly property real trackWide: 1.4

    // ---- Motion ------------------------------------------------------------
    readonly property int durFast: 130
    readonly property int durMed: 220
    readonly property int durSlow: 360

    // ---- Fonts -------------------------------------------------------------
    // Monospace so AIOStreams' aligned name/description blocks line up.
    readonly property string monoFont: "monospace"

    // ---- Helpers -----------------------------------------------------------
    function alpha(c, a) {
        return Qt.rgba(c.r, c.g, c.b, a)
    }

    function qualityColor(quality) {
        switch (quality) {
        case "2160p": return "#f5a83c"   // amber for 4K
        case "1080p": return "#6c8fff"   // blue
        case "720p":  return "#4cc2ff"   // light blue
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
