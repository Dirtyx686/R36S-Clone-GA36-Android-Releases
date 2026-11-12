/* SPDX-License-Identifier: Apache-2.0
 * Standard Android YV12 geometry only; no private Mali handle interpretation.
 * The buffer is already allocated/imported and mapped by the normal gralloc.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static inline bool ga36Yv12Geometry(int width, int height, int stride) {
    if (width <= 0 || height <= 0 || (width & 1) || (height & 1) ||
            stride < width || (stride & 15)) return false;
    const size_t limit = SIZE_MAX;
    const size_t ys = (size_t)stride, rows = (size_t)height;
    const size_t cs = (ys / 2 + 15) & ~(size_t)15;
    if (ys > limit / rows || cs > limit / (rows / 2)) return false;
    const size_t y = ys * rows, c = cs * (rows / 2);
    return c <= (limit - y) / 2;
}

static inline bool ga36Yv12Layout(void *base, int width, int height, int stride,
                                  android_ycbcr *layout) {
    if (!base || !layout || !ga36Yv12Geometry(width, height, stride)) return false;
    const size_t ys = (size_t)stride, cs = (ys / 2 + 15) & ~(size_t)15;
    const size_t y = ys * (size_t)height, c = cs * ((size_t)height / 2);
    if ((uintptr_t)base > UINTPTR_MAX - (y + c * 2)) return false;
    unsigned char *bytes = (unsigned char *)base;
    memset(layout, 0, sizeof(*layout));
    layout->y = bytes;
    layout->cr = bytes + y;
    layout->cb = bytes + y + c;
    layout->ystride = ys;
    layout->cstride = cs;
    layout->chroma_step = 1;
    return true;
}
