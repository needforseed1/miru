#include "MacWindowChrome.h"

#include <QWindow>

#import <AppKit/AppKit.h>

namespace MacWindowChrome {

void apply(QWindow *window)
{
    if (!window) {
        return;
    }
    // winId() forces creation of the native window if it doesn't exist yet.
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    NSWindow *nsWindow = view.window;
    if (!nsWindow) {
        return;
    }
    nsWindow.titlebarAppearsTransparent = YES;
    nsWindow.titleVisibility = NSWindowTitleHidden;
    nsWindow.styleMask |= NSWindowStyleMaskFullSizeContentView;
}

} // namespace MacWindowChrome
