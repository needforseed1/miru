#pragma once

class QWindow;

namespace MacWindowChrome {

// Hide the native macOS title bar chrome: transparent titlebar, no title
// text, and content extending into the titlebar area. Traffic-light buttons
// stay, floating over the QML header. No-op if the native window is missing.
void apply(QWindow *window);

} // namespace MacWindowChrome
