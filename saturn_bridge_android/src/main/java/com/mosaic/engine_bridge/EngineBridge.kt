package com.saturn.engine_bridge

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

    fun requireActivity(): Activity {
        return activity
            ?: throw IllegalStateException("Activity is not initialized. Call EngineBridge.initialize(activity) first.")
    }
}
