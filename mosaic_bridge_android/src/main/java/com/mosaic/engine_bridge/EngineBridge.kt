package com.mosaic.engine_bridge

import android.util.Log
import android.app.Activity
import java.lang.ref.WeakReference

/**
 * EngineBridge provides a bridge between the native engine and the Android APIs.
 */
object EngineBridge {
    private var activityRef: WeakReference<Activity>? = null
    private val activity: Activity? get() = activityRef?.get()

    fun initialize(activity: Activity) {
        activityRef = WeakReference(activity)
    }

    fun shutdown() {
        activityRef?.clear()
        activityRef = null
    }

    @JvmStatic
    fun showQuestionDialog(title: String, message: String, showCancel: Boolean): Boolean? {
        var result: Boolean? = null

        activity?.runOnUiThread {
            val dialogBuilder = android.app.AlertDialog.Builder(activity)
                .setTitle(title)
                .setMessage(message)
                .setPositiveButton("OK") { _, _ -> result = true }
                .setNegativeButton("NO") { _, _ -> result = false }

            if (showCancel) {
                dialogBuilder.setNeutralButton("Cancel") { _, _ -> result = null }
            }

            dialogBuilder.show()
        } ?: run {
            Log.w("EngineBridge", "Attempted to show alert with no valid activity.")
        }

        // TODO: Handle asynchronous nature of dialog result

        return result
    }

    @JvmStatic
    fun showInfoDialog(title: String, message: String) {
        activity?.runOnUiThread {
            android.app.AlertDialog.Builder(activity)
                .setTitle(title)
                .setMessage(message)
                .setPositiveButton("OK", null)
                .show()
        } ?: run {
            Log.w("EngineBridge", "Attempted to show alert with no valid activity.")
        }
    }

    @JvmStatic
    fun showWarningDialog(title: String, message: String) {
        activity?.runOnUiThread {
            android.app.AlertDialog.Builder(activity)
                .setTitle(title)
                .setMessage(message)
                .setPositiveButton("OK", null)
                .show()
        } ?: run {
            Log.w("EngineBridge", "Attempted to show alert with no valid activity.")
        }
    }

    @JvmStatic
    fun showErrorDialog(title: String, message: String) {
        activity?.runOnUiThread {
            android.app.AlertDialog.Builder(activity)
                .setTitle(title)
                .setMessage(message)
                .setPositiveButton("OK", null)
                .show()
        } ?: run {
            Log.w("EngineBridge", "Attempted to show alert with no valid activity.")
        }
    }
}