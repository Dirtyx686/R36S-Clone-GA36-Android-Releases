# Android11 HIDL wrappers around the actual GA36 legacy gralloc/framebuffer.
# AOSP HwcLoader supports gralloc framebuffer composition via HWC2OnFbAdapter.
PRODUCT_PACKAGES += \
    libga36rt libMali libEGL_mali libGLESv1_CM_mali libGLESv2_mali ga36_gralloc \
    android.hardware.graphics.allocator@2.0-impl \
    android.hardware.graphics.allocator@2.0-service \
    android.hardware.graphics.mapper@2.0-impl \
    android.hardware.graphics.composer@2.1-service
PRODUCT_PROPERTY_OVERRIDES += \
    ro.hardware.gralloc=ga36 \
    ro.hardware.egl=mali \
    ro.opengles.version=131072
