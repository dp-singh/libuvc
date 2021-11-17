/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (C) 2010-2012 Ken Tossell
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the author nor other contributors may be
*     used to endorse or promote products derived from this software
*     without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/
/**
\mainpage libuvc: a cross-platform library for USB video devices

\b libuvc is a library that supports enumeration, control and streaming
for USB Video Class (UVC) devices, such as consumer webcams.

\section features Features
\li UVC device \ref device "discovery and management" API
\li \ref streaming "Video streaming" (device to host) with asynchronous/callback and synchronous/polling modes
\li Read/write access to standard \ref ctrl "device settings"
\li \ref frame "Conversion" between various formats: RGB, YUV, JPEG, etc.
\li Tested on Mac and Linux, portable to Windows and some BSDs

\section roadmap Roadmap
\li Bulk-mode image capture
\li One-shot image capture
\li Improved support for standard settings
\li Support for "extended" (vendor-defined) settings

\section misc Misc.
\p The source code can be found at https://github.com/ktossell/libuvc. To build
the library, install <a href="http://libusb.org/">libusb</a> 1.0+ and run:

\code
$ git clone https://github.com/ktossell/libuvc.git
$ cd libuvc
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make && make install
\endcode

\section Example
In this example, libuvc is used to acquire images in a 30 fps, 640x480
YUV stream from a UVC device such as a standard webcam.

\include example.c

*/

/**
 * @defgroup init Library initialization/deinitialization
 * @brief Setup routines used to construct UVC access contexts
 */
#include <unistd.h>
#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

#define HASH_ADD_GUID(head,add) HASH_ADD(hh,head,guid,16,add)
#define HASH_FIND_GUID(head,findkey,out) HASH_FIND(hh,head,findkey,16,out)

void uvc_register_video_handler(uvc_context_t *ctx, const uint8_t guid[16], uvc_video_handler_t *video_handler) {
  struct uvc_video_payload_handler *vph = calloc(1, sizeof(*vph));
  vph->video_handler = video_handler;
  memcpy(vph->guid, guid, 16);
  HASH_ADD_GUID(ctx->video_payload_handlers, vph);
}

uvc_video_handler_t* uvc_get_video_handler(uvc_context_t *ctx, const uint8_t guid[16]) {
  struct uvc_video_payload_handler *vph = NULL;
  HASH_FIND_GUID(ctx->video_payload_handlers, guid, vph);
  if (vph == NULL) return NULL;
  return vph->video_handler;
}

#define REGISTER(handler, ...) do { \
    uint8_t GUID[16] = __VA_ARGS__; \
    extern struct uvc_video_handler handler; \
    uvc_register_video_handler(ctx, GUID, &handler); \
  } while(0)

static void _uvc_prefill_video_handlers(uvc_context_t *ctx) {
  REGISTER(frame_vid_handler, {'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71});
  REGISTER(frame_vid_handler, {'U',  'Y',  'V',  'Y', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71});
  REGISTER(frame_vid_handler, {'Y',  '8',  '0',  '0', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71});
  REGISTER(frame_vid_handler, {'Y',  '1',  '6',  ' ', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71});
  REGISTER(frame_vid_handler, {'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71});
  REGISTER(frame_vid_handler, {'B',  'Y',  '8',  ' ', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71});
  REGISTER(frame_vid_handler, {'B',  'A',  '8',  '1', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71});
  REGISTER(frame_vid_handler, {'G',  'R',  'B',  'G', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71});
  REGISTER(frame_vid_handler, {'G',  'B',  'R',  'G', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71});
  REGISTER(frame_vid_handler, {'R',  'G',  'G',  'B', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71});
  REGISTER(frame_vid_handler, {'B',  'G',  'G',  'R', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71});
  #ifdef LIBUVC_HAS_JPEG
  REGISTER(mjpeg_vid_handler, {'M','J','P','G',0,});
  #endif
}


/** @internal
 * @brief Event handler thread
 * There's one of these per UVC context.
 * @todo We shouldn't run this if we don't own the USB context
 */
void *_uvc_handle_events(void *arg) {
  uvc_context_t *ctx = (uvc_context_t *) arg;
  nice(-10);
  while (!ctx->kill_handler_thread)
    libusb_handle_events_completed(ctx->usb_ctx, &ctx->kill_handler_thread);
  return NULL;
}

/** @brief Initializes the UVC context
 * @ingroup init
 *
 * @note If you provide your own USB context, you must handle
 * libusb event processing using a function such as libusb_handle_events.
 *
 * @param[out] pctx The location where the context reference should be stored.
 * @param[in]  usb_ctx Optional USB context to use
 * @return Error opening context or UVC_SUCCESS
 */
uvc_error_t uvc_init(uvc_context_t **pctx, struct libusb_context *usb_ctx) {
  uvc_error_t ret = UVC_SUCCESS;
  uvc_context_t *ctx = calloc(1, sizeof(*ctx));

  if (usb_ctx == NULL) {
    ret = libusb_init(&ctx->usb_ctx);
    ctx->own_usb_ctx = 1;
    if (ret != UVC_SUCCESS) {
      free(ctx);
      ctx = NULL;
    }
  } else {
    ctx->own_usb_ctx = 0;
    ctx->usb_ctx = usb_ctx;
  }

  _uvc_prefill_video_handlers(ctx);

  if (ctx != NULL)
    *pctx = ctx;

  return ret;
}

/**
 * @brief Closes the UVC context, shutting down any active cameras.
 * @ingroup init
 *
 * @note This function invalides any existing references to the context's
 * cameras.
 *
 * If no USB context was provided to #uvc_init, the UVC-specific USB
 * context will be destroyed.
 *
 * @param ctx UVC context to shut down
 */
void uvc_exit(uvc_context_t *ctx) {
  uvc_device_handle_t *devh;

  DL_FOREACH(ctx->open_devices, devh) {
    uvc_close(devh);
  }

  if (ctx->own_usb_ctx)
    libusb_exit(ctx->usb_ctx);

  free(ctx);
}

/**
 * @internal
 * @brief Spawns a handler thread for the context
 * @ingroup init
 *
 * This should be called at the end of a successful uvc_open if no devices
 * are already open (and being handled).
 */
void uvc_start_handler_thread(uvc_context_t *ctx) {
  if (ctx->own_usb_ctx) {
    ctx->kill_handler_thread = 0;
    pthread_create(&ctx->handler_thread, NULL, _uvc_handle_events, (void *) ctx);
  }
}

