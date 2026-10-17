// Passes a key the plugin does not use (Space, Ctrl / Alt shortcuts) on to the DAW's own window.
// Kept in its own file: <windows.h> clashes with names JUCE code uses (Rectangle, Point, ...).
#include "VaultHostKeys.h"

#ifdef _WIN32
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>

// JUCE posts unused keys only to the plugin window's direct parent, which in some hosts (FL Studio) is a frame that
// ignores them, so Space never reached the transport. Post to the window that owns the whole chain instead.
bool vaultPostKeyToHost (void* nativeHandle, int vk, bool down, bool repeat)
{
    auto own = (HWND) nativeHandle;
    if (own == nullptr || GetParent (own) == nullptr) return false;   // a top-level window (standalone): no host to tell
    auto host = GetAncestor (own, GA_ROOTOWNER);
    if (host == nullptr || host == own) return false;
    const LPARAM scan = (LPARAM) MapVirtualKeyW ((UINT) vk, MAPVK_VK_TO_VSC) << 16;
    const bool alt = (GetKeyState (VK_MENU) & 0x8000) != 0;
    const UINT msg = alt ? (down ? WM_SYSKEYDOWN : WM_SYSKEYUP) : (down ? WM_KEYDOWN : WM_KEYUP);
    LPARAM lp = scan | 1 | (alt ? ((LPARAM) 1 << 29) : 0);
    if (down && repeat) lp |= (LPARAM) 1 << 30;
    if (! down) lp |= (LPARAM) 0xC0000000;
    return PostMessageW (host, msg, (WPARAM) vk, lp) != 0;
}
#else
bool vaultPostKeyToHost (void*, int, bool, bool) { return false; }   // macOS / Linux: an unused key already goes on to the host
#endif
