# GA36 A33 Android11 bring-up configuration. Not a boot-tested product.
# Keep the experiment locked until kernel IPC, graphics and SELinux gates pass.
ifneq ($(GA36_ALLOW_EXPERIMENTAL_BUILD),true)
$(error GA36 Android11: experimental build requires explicit preparation gate; use preflight.py)
endif
DEVICE_PATH := device/ga36/ga36
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := cortex-a7
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_USES_64_BIT_BINDER := true
# K08 provides real shared executable memfd files on the retained 3.4 kernel.
TARGET_HAS_MEMFD_BACKPORT := true
TARGET_BOARD_PLATFORM := sun8i
TARGET_BOOTLOADER_BOARD_NAME := GA36-MB
TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true
TARGET_SCREEN_DENSITY := 160
BOARD_BOOTIMG_HEADER_VERSION := 0
BOARD_KERNEL_BASE := 0x40000000
BOARD_KERNEL_OFFSET := 0x00008000
BOARD_RAMDISK_OFFSET := 0x1000000
BOARD_TAGS_OFFSET := 0x100
BOARD_KERNEL_PAGESIZE := 2048
BOARD_KERNEL_IMAGE_NAME := Image
BOARD_BOOTIMAGE_PARTITION_SIZE := 33554432
# These are separate FILE IMAGE capacities for an isolated experiment.
# They are not instructions to repartition the working Android7 card.
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 2147483648
BOARD_VENDORIMAGE_PARTITION_SIZE := 536870912
BOARD_USERDATAIMAGE_PARTITION_SIZE := 25769803776
BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_USERDATAIMAGE_FILE_SYSTEM_TYPE := ext4
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_FLASH_BLOCK_SIZE := 131072

# Experimental private image build only; do not target Android7 raw regions.
TARGET_COPY_OUT_VENDOR := vendor
BOARD_AVB_ENABLE := false
BOARD_BUILD_SYSTEM_ROOT_IMAGE := false
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/runtime/fstab.ga36
TARGET_PREBUILT_KERNEL := $(DEVICE_PATH)/kernel/Image

# Kernel3.4 requires a conservative ext4 feature set, not modern e2fsprogs defaults.
BOARD_EXT4_SHARE_DUP_BLOCKS := false
# Do not increase policy version without implementing and testing its kernel parser.
# K04 backports policy30 parser/xperms; actual policy load is still device-unverified.

# Pinned private ncurses5 ABI for the legacy RenderScript host compiler.
BUILD_BROKEN_NINJA_USES_ENV_VARS += LD_LIBRARY_PATH MKE2FS_CONFIG

# Actual legacy upgrade hardware: original Android7 first API level.
DEVICE_MANIFEST_FILE += $(DEVICE_PATH)/graphics/manifest-graphics.xml
DEVICE_MANIFEST_FILE += $(DEVICE_PATH)/runtime/manifest-keymaster.xml

# Narrow GA36 native backlight and device-node policy.
BOARD_VENDOR_SEPOLICY_DIRS += $(DEVICE_PATH)/runtime/sepolicy

DEVICE_MANIFEST_FILE += $(DEVICE_PATH)/runtime/components/audio/manifest-audio.xml

# Real upstream OMX service already installed; declare both registered interfaces.
DEVICE_MANIFEST_FILE += $(DEVICE_PATH)/runtime/manifest-media.xml
