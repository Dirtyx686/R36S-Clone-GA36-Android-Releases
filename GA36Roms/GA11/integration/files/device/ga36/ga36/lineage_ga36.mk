GA36_MINIMAL_CONSOLE := true
LOCAL_PATH := device/ga36/ga36
$(call inherit-product, vendor/lineage/config/common_mini_tablet_wifionly.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/aosp_base.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/go_defaults_512.mk)
$(call inherit-product, $(LOCAL_PATH)/device.mk)
PRODUCT_NAME := lineage_ga36
PRODUCT_DEVICE := ga36
PRODUCT_BRAND := GA36
PRODUCT_MANUFACTURER := GA36
PRODUCT_MODEL := GA36 Android11 Experimental

# First Android API of this board; keep legacy upgraded-device linker rules.
PRODUCT_SHIPPING_API_LEVEL := 24
