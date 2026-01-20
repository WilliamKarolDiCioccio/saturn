@file:Suppress("unused")

package com.saturn.engine_bridge

object SystemUI {
    @JvmStatic
    @NativeExport
    fun showQuestionDialog(title: String, message: String, showCancel: Boolean): Boolean? {
        val activity = EngineBridge.requireActivity()
        var result: Boolean? = null
        var finished = false

        activity.runOnUiThread {
            val dialogBuilder =
                    android.app.AlertDialog.Builder(activity)
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
        }

        while (!finished) Thread.sleep(100)

        return result
    }

    @JvmStatic
    @NativeExport
    fun showInfoDialog(title: String, message: String) {
        val activity = EngineBridge.requireActivity()

        activity.runOnUiThread {
            android.app.AlertDialog.Builder(activity)
                    .setTitle(title)
                    .setMessage(message)
                    .setPositiveButton("OK", null)
                    .show()
        }
    }

    @JvmStatic
    @NativeExport
    fun showWarningDialog(title: String, message: String) {
        val activity = EngineBridge.requireActivity()

        activity.runOnUiThread {
            android.app.AlertDialog.Builder(activity)
                    .setTitle(title)
                    .setMessage(message)
                    .setPositiveButton("OK", null)
                    .show()
        }
    }

    @JvmStatic
    @NativeExport
    fun showErrorDialog(title: String, message: String) {
        val activity = EngineBridge.requireActivity()

        activity.runOnUiThread {
            android.app.AlertDialog.Builder(activity)
                    .setTitle(title)
                    .setMessage(message)
                    .setPositiveButton("OK", null)
                    .show()
        }
    }
}
