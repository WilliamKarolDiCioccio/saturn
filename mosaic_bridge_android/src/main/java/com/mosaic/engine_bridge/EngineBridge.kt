package com.mosaic.engine_bridge

import android.util.Log
import android.app.Activity
import java.lang.ref.WeakReference

/**
 * Marks a Kotlin function as a native-exported method that should be bound via JNI.
 */
@Retention(AnnotationRetention.SOURCE)
@Target(AnnotationTarget.FUNCTION)
annotation class NativeExport

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
    @NativeExport
    fun showQuestionDialog(title: String, message: String, showCancel: Boolean): Boolean? {
        var result: Boolean? = null
        var finished: Boolean = false

        activity?.runOnUiThread {
            val dialogBuilder = android.app.AlertDialog.Builder(activity)
                .setTitle(title)
                .setMessage(message)
                .setPositiveButton("OK") { _, _ ->
                    result = true
                    finished = true
                }
                .setNegativeButton("NO") { _, _ ->
                    result = false
                    finished = true
                }

            if (showCancel) {
                dialogBuilder.setNeutralButton("Cancel") { _, _ ->
                    result = null
                    finished = true
                }
            }

            dialogBuilder.show()
        } ?: run {
            Log.w("EngineBridge", "Attempted to show alert with no valid activity.")
        }

        while (!finished) Thread.sleep(100)

        return result
    }

    @JvmStatic
    @NativeExport
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
    @NativeExport
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
    @NativeExport
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