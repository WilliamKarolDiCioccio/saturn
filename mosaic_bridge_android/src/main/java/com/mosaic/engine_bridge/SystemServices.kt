@file:Suppress("unused")

package com.saturn.engine_bridge

import android.content.Context

object SystemServices {
    @JvmStatic
    @NativeExport
    fun setClipboard(text: String) {
        val activity = EngineBridge.requireActivity()
        val clipboard =
            activity.getSystemService(Context.CLIPBOARD_SERVICE) as android.content.ClipboardManager
        val clip = android.content.ClipData.newPlainText("Copied Text", text)

        clipboard.setPrimaryClip(clip)
    }

    @JvmStatic
    @NativeExport
    fun getClipboard(): String? {
        val activity = EngineBridge.requireActivity()
        val clipboard =
            activity.getSystemService(Context.CLIPBOARD_SERVICE) as android.content.ClipboardManager
        val clip = clipboard.primaryClip

        return if (clip != null && clip.itemCount > 0) {
            clip.getItemAt(0).coerceToText(activity).toString()
        } else {
            null
        }
    }
}