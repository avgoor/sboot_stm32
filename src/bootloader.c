/* This file is the part of the STM32 secure bootloader
 *
 * Copyright ©2016 Dmitry Filimonchuk <dmitrystu[at]gmail[dot]com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdbool.h>
#include "stm32.h"
#include "usb.h"
#include "inc/usb_dfu.h"
#include "../config.h"
#include "descriptors.h"
#include "flash.h"
#include "crypto.h"

typedef uint8_t(*flash_rom)(void *romptr, const void *buffer, uint32_t blksize);


/* extern vaiables from linker */
extern uint8_t __app_start;
extern uint8_t __romend;
extern uint8_t __ee_start;
extern uint8_t __ee_end;


static uint32_t dfu_buffer[0x88];
static usbd_device dfu;



static struct {
    flash_rom   flash;
    void        *dptr;
    int32_t     remained;
    uint8_t     interface;
    uint8_t     bStatus;
    uint8_t     bState;

} dfu_data;

/** Processing DFU_SET_IDLE request */
static usbd_respond dfu_set_idle(void) {
#if defined(DFU_USE_CIPHER)
    static const uint8_t key[] = {DFU_AES_KEY};
    aes_init(key);
#endif
    dfu_data.bState = USB_DFU_STATE_DFU_IDLE;
    dfu_data.bStatus = USB_DFU_STATUS_OK;
    switch (dfu_data.interface){
#ifdef DFU_INTF_EEPROM
    case 1:
        dfu_data.dptr = &__ee_start;
        dfu_data.remained = &__ee_end - &__ee_start;
        dfu_data.flash = program_eeprom;
        break;
#endif
    default:
        dfu_data.dptr = &__app_start;
        dfu_data.remained = &__romend - &__app_start;
        dfu_data.flash = program_flash;
        break;
    }
    return usbd_ack;
}

static usbd_respond dfu_err_badreq(void) {
    dfu_data.bState  = USB_DFU_STATE_DFU_ERROR;
    dfu_data.bStatus = USB_DFU_STATUS_ERR_STALLEDPKT;
    return usbd_fail;
}


static usbd_respond dfu_upload(usbd_device *dev, int32_t blksize) {
    switch (dfu_data.bState) {
#ifdef DFU_CAN_UPLOAD
    case USB_DFU_STATE_DFU_IDLE:
    case USB_DFU_STATE_DFU_UPLOADIDLE:
        if (dfu_data.remained == 0) {
            dev->status.data_count = 0;
            return dfu_set_idle();
        }
#ifdef DFU_USE_CIPHER
        aes_encrypt(dev->status.data_ptr, dfu_data.dptr, blksize);
#else
        dev->status.data_ptr = dfu_data.dptr;
#endif
        dfu_data.remained -= blksize;
        dfu_data.dptr += blksize;
        return usbd_ack;
#endif
    default:
        return dfu_err_badreq();
    }
}

static usbd_respond dfu_dnload(void *buf, int32_t blksize) {
    switch(dfu_data.bState) {
    case    USB_DFU_STATE_DFU_DNLOADIDLE:
    case    USB_DFU_STATE_DFU_DNLOADSYNC:
    case    USB_DFU_STATE_DFU_IDLE:
        if (blksize == 0) {
            dfu_data.bState = USB_DFU_STATE_DFU_MANIFESTSYNC;
            return usbd_ack;
        }
        if (blksize > dfu_data.remained) {
            dfu_data.bStatus = USB_DFU_STATUS_ERR_ADDRESS;
            dfu_data.bState = USB_DFU_STATE_DFU_ERROR;
            return usbd_ack;
        }
#ifdef DFU_USE_CIPHER
        aes_decrypt(buf, buf, blksize );
#endif
        dfu_data.bStatus = dfu_data.flash(dfu_data.dptr, buf, blksize);

        if (dfu_data.bStatus == USB_DFU_STATUS_OK) {
            dfu_data.dptr += blksize;
            dfu_data.remained -= blksize;
#ifdef DFU_DNLOAD_NOSYNC
            dfu_data.bState = USB_DFU_STATE_DFU_DNLOADIDLE;
#else
            dfu_data.bState = USB_DFU_STATE_DFU_DNLOADSYNC;
#endif
            return usbd_ack;
        } else {
            dfu_data.bState = USB_DFU_STATE_DFU_ERROR;
            return usbd_ack;
        }
    default:
        return dfu_err_badreq();
    }
}


static usbd_respond dfu_getstatus(void *buf) {
    /* make answer */
    struct usb_dfu_status *stat = buf;
    stat->bStatus = dfu_data.bStatus;
    stat->bState = dfu_data.bState;
    stat->bPollTimeout = (DFU_POLL_TIMEOUT & 0xFF);
    stat->wPollTimeout = (DFU_POLL_TIMEOUT >> 8);
    stat->iString = NO_DESCRIPTOR;


    switch (dfu_data.bState) {
    case USB_DFU_STATE_DFU_IDLE:
    case USB_DFU_STATE_DFU_DNLOADIDLE:
    case USB_DFU_STATE_DFU_UPLOADIDLE:
    case USB_DFU_STATE_DFU_ERROR:
        return usbd_ack;
    case USB_DFU_STATE_DFU_DNLOADSYNC:
        dfu_data.bState = USB_DFU_STATE_DFU_DNLOADIDLE;
        return usbd_ack;
    case USB_DFU_STATE_DFU_MANIFESTSYNC:
        return dfu_set_idle();
    default:
        return dfu_err_badreq();
    }
}

static usbd_respond dfu_getstate(uint8_t *buf) {
    *buf = dfu_data.bState;
    return usbd_ack;
}

static usbd_respond dfu_abort() {
    switch (dfu_data.bState) {
    case USB_DFU_STATE_DFU_IDLE:
    case USB_DFU_STATE_DFU_DNLOADSYNC:
    case USB_DFU_STATE_DFU_DNLOADIDLE:
    case USB_DFU_STATE_DFU_MANIFESTSYNC:
    case USB_DFU_STATE_DFU_UPLOADIDLE:
        return dfu_set_idle();
    default:
        return dfu_err_badreq();
    }
}

static usbd_respond dfu_clrstatus() {
    if (dfu_data.bState == USB_DFU_STATE_DFU_ERROR)  {
        return dfu_set_idle();
    } else {
        return dfu_err_badreq();
    }
}

static void dfu_reset(usbd_device *dev, uint8_t ev, uint8_t ep) {
    (void)dev;
    (void)ev;
    (void)ep;
    /** TODO : add firmware checkout */
    NVIC_SystemReset();
}

static usbd_respond dfu_control (usbd_device *dev, usbd_ctlreq *req, usbd_ctl_complete *callback) {
    (void)callback;
    if ((req->bmRequestType  & (USB_REQ_TYPE | USB_REQ_RECIPIENT)) == (USB_REQ_STANDARD | USB_REQ_INTERFACE)) {
        switch (req->bRequest) {
        case USB_STD_SET_INTERFACE:
            if (req->wIndex != 0) return usbd_fail;
            switch (req->wValue) {
            case 0: break;
#ifdef DFU_INTF_EEPROM
            case 1: break;
#endif
            default:
                return usbd_fail;
            }
            dfu_data.interface = req->wValue;
            return dfu_set_idle();
        case USB_STD_GET_INTERFACE:
            req->data[0] = dfu_data.interface;
            return usbd_ack;
        default:
            return usbd_fail;
        }
    }
    if ((req->bmRequestType & (USB_REQ_TYPE | USB_REQ_RECIPIENT)) == (USB_REQ_CLASS | USB_REQ_INTERFACE)) {
        switch (req->bRequest) {
#ifdef DFU_DETACH_ENABLED
        case USB_DFU_DETACH:
            *callback = (usbd_ctl_complete)dfu_reset;
            return usbd_ack;
#endif
        case USB_DFU_DNLOAD:
            return dfu_dnload(req->data, req->wLength);
        case USB_DFU_UPLOAD:
            return dfu_upload(dev, req->wLength);
        case USB_DFU_GETSTATUS:
            return dfu_getstatus(req->data);
        case USB_DFU_CLRSTATAUS:
            return dfu_clrstatus();
        case USB_DFU_GETSTATE:
            return dfu_getstate(req->data);
        case USB_DFU_ABORT:
            return dfu_abort();
        default:
            return dfu_err_badreq();
        }
    }
    return false;
}


static bool dfu_config(usbd_device *dev, uint8_t config) {
    (void)dev;
    if (config == 1) {
        usbd_reg_event(&dfu, usbd_evt_reset, dfu_reset);
        return true;
    } else {
        usbd_reg_event(&dfu, usbd_evt_reset, 0);
    }
    return false;
}



static void dfu_init (void) {
    dfu_set_idle();
    usbd_init(&dfu, &usbd_hw, DFU_EP0_SIZE, dfu_buffer, sizeof(dfu_buffer));
    usbd_reg_config(&dfu, dfu_config);
    usbd_reg_control(&dfu, dfu_control);
    usbd_reg_descr(&dfu, dfu_get_descriptor);
    usbd_control(&dfu, usbd_cmd_enable);
    usbd_control(&dfu, usbd_cmd_connect);
}


int main (void) {
    dfu_init();
    while(1) {
        usbd_poll(&dfu);
    }
}



