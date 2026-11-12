LOCAL_PATH := device/ga36/ga36
# Low-RAM ARM32 console target; no GApps, camera, calendar or music bundle.
PRODUCT_CHARACTERISTICS := tablet
PRODUCT_AAPT_CONFIG := normal
PRODUCT_AAPT_PREF_CONFIG := mdpi
PRODUCT_PROPERTY_OVERRIDES += ro.config.low_ram=true ro.radio.noril=true
# No Java-debugger transport while USB development is deferred; retain SD logcat.
PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.jdwp-provider=none
# System-server compiler filter is inherited once from go_defaults_512.mk.
# Retained Mali gralloc lacks Codec2 flexible-YUV buffers/lockYCbCr. Use OMX
# software codecs and the compatible legacy SoftwareRenderer for video.
PRODUCT_PROPERTY_OVERRIDES += debug.stagefright.ccodec=0
PRODUCT_USE_PROFILE_FOR_BOOT_IMAGE := true
PRODUCT_ART_TARGET_INCLUDE_DEBUG_BUILD := false
# Working Android7 HALs are staged privately, but are not declared as Android11
# services until their adapters and interface contracts have passed tests.

# Console essentials. Hardware services are gated separately until their ABI works.
PRODUCT_PACKAGES += GA36Home Settings DocumentsUI ga36-memory
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/permissions/privapp-permissions-ga36.xml:$(TARGET_COPY_OUT_SYSTEM)/etc/permissions/privapp-permissions-ga36.xml

# No camera client on this gaming handheld. Keep core media codecs and providers.
PRODUCT_NO_CAMERA := true

# Native Android11 wrappers for GA36 Mali/framebuffer; device validation pending.
$(call inherit-product, $(LOCAL_PATH)/graphics/graphics.mk)

# Provision the dedicated console without an account/setup wizard.
DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

# A real primary PCM HAL is required by AudioFlinger during core startup.
# Speaker-only first bring-up; no unsupported microphone/USB/offload profiles.
PRODUCT_PACKAGES += audio.primary.ga36 android.hardware.audio.service \
    android.hardware.audio@2.0-impl android.hardware.audio.effect@2.0-impl
PRODUCT_PROPERTY_OVERRIDES += ro.hardware.audio.primary=ga36
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/runtime/components/audio/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
    frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml \
    frameworks/av/media/libeffects/data/audio_effects.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_effects.xml

# P07 native board init and backlight support.
PRODUCT_PACKAGES += lights.ga36 android.hardware.light@2.0-impl android.hardware.light@2.0-service
PRODUCT_PROPERTY_OVERRIDES += ro.hardware.lights=ga36
# BatteryService in Android11 requires a live IHealth callback during boot.
# Use the real standard power_supply monitor, including the retained AXP22 PMIC.
PRODUCT_PACKAGES += android.hardware.health@2.1-impl android.hardware.health@2.1-service
# Required Android security interfaces on this legacy board. The upstream
# implementations report software security; no unavailable TEE is advertised.
PRODUCT_PACKAGES += \
    android.hardware.keymaster@3.0-impl \
    android.hardware.keymaster@3.0-service \
    android.hardware.gatekeeper@1.0-service.software
# The stock legacy VINTF matrix also requires default DRM factory interfaces.
# Upstream legacy wrapper loads the already-retained software ClearKey plugin.
PRODUCT_PACKAGES += android.hardware.drm@1.0-impl android.hardware.drm@1.0-service
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/runtime/fstab.ga36:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.ga36 \
    $(LOCAL_PATH)/runtime/init.ga36.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.ga36.rc \
    $(LOCAL_PATH)/runtime/ueventd.ga36.rc:$(TARGET_COPY_OUT_VENDOR)/ueventd.rc
PRODUCT_COPY_FILES += $(LOCAL_PATH)/runtime/input/ga36-idc/GA36_UI_Pointer.idc:$(TARGET_COPY_OUT_VENDOR)/usr/idc/GA36_UI_Pointer.idc
PRODUCT_COPY_FILES += $(LOCAL_PATH)/runtime/input/ga36-idc/axp22-supplyer.idc:$(TARGET_COPY_OUT_VENDOR)/usr/idc/axp22-supplyer.idc
PRODUCT_COPY_FILES += $(LOCAL_PATH)/runtime/input/ga36-idc/micro_gamepad.idc:$(TARGET_COPY_OUT_VENDOR)/usr/idc/micro_gamepad.idc
PRODUCT_COPY_FILES += $(LOCAL_PATH)/runtime/input/ga36-idc/udt_keyboard.idc:$(TARGET_COPY_OUT_VENDOR)/usr/idc/udt_keyboard.idc
PRODUCT_COPY_FILES += $(LOCAL_PATH)/runtime/input/ga36-keylayout/axp22-supplyer.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/axp22-supplyer.kl
PRODUCT_COPY_FILES += $(LOCAL_PATH)/runtime/input/ga36-keylayout/micro_gamepad.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/micro_gamepad.kl
PRODUCT_COPY_FILES += $(LOCAL_PATH)/runtime/input/ga36-keylayout/sunxi-keyboard.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/sunxi-keyboard.kl
PRODUCT_COPY_FILES += $(LOCAL_PATH)/runtime/input/ga36-keylayout/udt_keyboard.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/udt_keyboard.kl

# Initial device bring-up needs persistent evidence; not installed in user builds.
ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
PRODUCT_COPY_FILES += $(LOCAL_PATH)/runtime/ga36-bootlog.rc:$(TARGET_COPY_OUT_SYSTEM)/etc/init/ga36-bootlog.rc
endif

# One-frame black boot image keeps texture memory small.
TARGET_BOOTANIMATION := device/ga36/ga36/runtime/bootanimation.zip

# Choose GA36 Home on first boot; retain the recents backend without a HOME chooser.
PRODUCT_COPY_FILES += $(LOCAL_PATH)/runtime/ga36-home-preferred.xml:$(TARGET_COPY_OUT_SYSTEM)/etc/preferred-apps/ga36-home.xml
