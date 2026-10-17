#pragma once

// Posts a key down / up (Windows virtual-key code) to the DAW window that owns the plugin window with this native handle.
// Returns false when there is no such host window (standalone, or not Windows).
bool vaultPostKeyToHost (void* nativeHandle, int vk, bool down, bool repeat = false);
