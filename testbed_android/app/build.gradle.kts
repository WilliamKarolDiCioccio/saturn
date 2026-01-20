import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
}

android {
    namespace = "com.saturn.testbed"
    compileSdk = 35
    ndkVersion = "29.0.13113456"

    // Define flavor dimensions for ABI variants
    flavorDimensions += listOf("abi")
    productFlavors {
        create("arm64") {
            dimension = "abi"
            ndk.abiFilters.clear()
            ndk.abiFilters.add("arm64-v8a")
            externalNativeBuild.cmake.arguments += "-DVCPKG_TARGET_TRIPLET=arm64-android"
        }
        create("x86_64") {
            dimension = "abi"
            ndk.abiFilters.clear()
            ndk.abiFilters.add("x86_64")
            externalNativeBuild.cmake.arguments += "-DVCPKG_TARGET_TRIPLET=x64-android"
        }
    }

    defaultConfig {
        applicationId = "com.saturn.testbed"
        minSdk = 28
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        debug {
            isMinifyEnabled = false
            externalNativeBuild.cmake.arguments += "-DCMAKE_BUILD_TYPE=Debug"
        }
        create("dev") {
            isDebuggable = true
            isJniDebuggable = false
            externalNativeBuild.cmake.arguments += "-DCMAKE_BUILD_TYPE=Dev"
        }
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            externalNativeBuild.cmake.arguments += "-DCMAKE_BUILD_TYPE=Release"
        }
    }

    buildFeatures { prefab = true }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlin {
        jvmToolchain(17)
    }

    buildFeatures {
        prefab = true
        dataBinding = true
        viewBinding = true
    }

    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            version = "3.31.0"
        }
    }

    // Common CMake arguments for all variants
    val commonCmakeArgs =
        listOf(
            "-DANDROID=TRUE",
            "-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH",
            "-DCMAKE_TOOLCHAIN_FILE=${System.getenv("VCPKG_ROOT")}/scripts/buildsystems/vcpkg.cmake",
            "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=${System.getenv("ANDROID_NDK_HOME")}/build/cmake/android.toolchain.cmake",
            "-DANDROID_STL=c++_shared",
            "-DANDROID_CPP_FEATURES=rtti exceptions"
        )

    // Apply common arguments to all configurations
    defaultConfig.externalNativeBuild.cmake.arguments += commonCmakeArgs
    productFlavors.forEach { flavor ->
        flavor.externalNativeBuild.cmake.arguments += commonCmakeArgs
    }
}

dependencies {
    // Engine dependencies
    implementation(project(":engine_bridge"))
    implementation(libs.androidx.games.activity)

    // Android dependencies
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)

    // Test dependencies
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
}
