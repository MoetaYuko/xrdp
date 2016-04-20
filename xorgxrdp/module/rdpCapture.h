/*
Copyright 2014 Laxmikant Rashinkar
Copyright 2014-2015 Jay Sorg

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

capture

*/

#ifndef __RDPCAPTURE_H
#define __RDPCAPTURE_H

#include <xorg-server.h>
#include <xorgVersion.h>
#include <xf86.h>

extern _X_EXPORT Bool
rdpCapture(rdpClientCon *clientCon,
           RegionPtr in_reg, BoxPtr *out_rects, int *num_out_rects,
           void *src, int src_left, int src_top,
           int src_width, int src_height,
           int src_stride, int src_format,
           void *dst, int dst_width, int dst_height,
           int dst_stride, int dst_format, int mode);

extern _X_EXPORT int
a8r8g8b8_to_a8b8g8r8_box(char *s8, int src_stride,
                         char *d8, int dst_stride,
                         int width, int height);

#endif
