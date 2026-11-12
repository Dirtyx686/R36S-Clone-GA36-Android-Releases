/*
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#ifndef LOG_TAG
#warning "Gralloc0Hal.h included without LOG_TAG"
#endif

#include <cstring>  // for strerror
#include <cutils/properties.h>
#include <inttypes.h>

#include <allocator-hal/2.0/AllocatorHal.h>
#include <hardware/gralloc.h>
#include <log/log.h>
#include <mapper-passthrough/2.0/GrallocBufferDescriptor.h>

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V2_0 {
namespace passthrough {

namespace detail {

using mapper::V2_0::BufferDescriptor;
using mapper::V2_0::Error;
using mapper::V2_0::passthrough::grallocDecodeBufferDescriptor;

// Gralloc0HalImpl implements V2_*::hal::AllocatorHal on top of gralloc0
template <typename Hal>
class Gralloc0HalImpl : public Hal {
   public:
    ~Gralloc0HalImpl() {
        if (mDevice) {
            gralloc_close(mDevice);
        }
    }

    bool initWithModule(const hw_module_t* module) {
        int result = gralloc_open(module, &mDevice);
        if (result) {
            ALOGE("failed to open gralloc0 device: %s", strerror(-result));
            mDevice = nullptr;
            return false;
        }

        return true;
    }

    std::string dumpDebugInfo() override {
        char buf[4096] = {};
        if (mDevice->dump) {
            mDevice->dump(mDevice, buf, sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';
        }

        return buf;
    }

    Error allocateBuffers(const BufferDescriptor& descriptor, uint32_t count, uint32_t* outStride,
                          std::vector<const native_handle_t*>* outBuffers) override {
        mapper::V2_0::IMapper::BufferDescriptorInfo descriptorInfo;
        if (!grallocDecodeBufferDescriptor(descriptor, &descriptorInfo)) {
            return Error::BAD_DESCRIPTOR;
        }

        Error error = Error::NONE;
        uint32_t stride = 0;
        std::vector<const native_handle_t*> buffers;
        buffers.reserve(count);

        // allocate the buffers
        for (uint32_t i = 0; i < count; i++) {
            const native_handle_t* tmpBuffer;
            uint32_t tmpStride;
            error = allocateOneBuffer(descriptorInfo, &tmpBuffer, &tmpStride);
            if (error != Error::NONE) {
                break;
            }

            buffers.push_back(tmpBuffer);

            if (stride == 0) {
                stride = tmpStride;
            } else if (stride != tmpStride) {
                // non-uniform strides
                error = Error::UNSUPPORTED;
                break;
            }
        }

        if (error != Error::NONE) {
            freeBuffers(buffers);
            return error;
        }

        *outStride = stride;
        *outBuffers = std::move(buffers);

        return Error::NONE;
    }

    void freeBuffers(const std::vector<const native_handle_t*>& buffers) override {
        for (auto buffer : buffers) {
            int result = mDevice->free(mDevice, buffer);
            if (result != 0) {
                ALOGE("failed to free buffer %p: %d", buffer, result);
            }
        }
    }

   protected:
    Error allocateOneBuffer(const mapper::V2_0::IMapper::BufferDescriptorInfo& info,
                            const native_handle_t** outBuffer, uint32_t* outStride) {
        if (info.layerCount > 1 || (info.usage >> 32) != 0) {
            return Error::BAD_VALUE;
        }

        const native_handle_t* buffer = nullptr;
        int stride = 0;
        // Legacy framebuffer handles include process-local state and two fixed
        // slots. HIDL transports client targets across processes: use the
        // driver's shareable ION path on GA36, and its existing post-copy path.
        char board[PROPERTY_VALUE_MAX] = {};
        property_get("ro.hardware.gralloc", board, "");
        const bool sharedTarget = (info.usage & GRALLOC_USAGE_HW_FB) &&
                                  strcmp(board, "ga36") == 0;
        const uint64_t allocationUsage = sharedTarget ?
                info.usage & ~GRALLOC_USAGE_HW_FB : info.usage;
        int result = mDevice->alloc(mDevice, info.width, info.height, static_cast<int>(info.format),
                                    allocationUsage, &buffer, &stride);
        if (sharedTarget) {
            ALOGI("GA36_HIDL_ION_CLIENT_TARGET result=%d %ux%u usage=%" PRIu64,
                  result, info.width, info.height, allocationUsage);
        }
        if (result != 0) {
            ALOGE("legacy gralloc allocation failed result=%d %ux%u format=%d usage=%" PRIu64,
                  result, info.width, info.height, static_cast<int>(info.format), info.usage);
        }
        switch (result) {
            case 0:
                *outBuffer = buffer;
                *outStride = stride;
                return Error::NONE;
            case -EINVAL:
                return Error::BAD_VALUE;
            default:
                return Error::NO_RESOURCES;
        }
    }

    alloc_device_t* mDevice = nullptr;
};

}  // namespace detail

using Gralloc0Hal = detail::Gralloc0HalImpl<hal::AllocatorHal>;

}  // namespace passthrough
}  // namespace V2_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android
