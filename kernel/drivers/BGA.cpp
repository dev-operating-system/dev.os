#include <libsystem/math/MinMax.h>

#include "kernel/drivers/BGA.h"
#include "kernel/graphics/Graphics.h"
#include "kernel/handover/Handover.h"
#include "kernel/interrupts/Interupts.h"

void BGA::write_register(uint16_t address, uint16_t data)
{
    out16(BGA_ADDRESS, address);
    out16(BGA_DATA, data);
}

uint16_t BGA::read_register(uint16_t address)
{
    out16(BGA_ADDRESS, address);
    return in16(BGA_DATA);
}

Result BGA::set_resolution(int width, int height)
{
    if (width * height * sizeof(uint32_t) > _framebuffer->size())
    {
        logger_warn("Not enoughs VRAM for setting the resolution to %dx%d.", width, height);

        return ERR_OUT_OF_MEMORY;
    }
    else
    {
        write_register(BGA_REG_ENABLE, BGA_DISABLED);
        write_register(BGA_REG_XRES, width);
        write_register(BGA_REG_YRES, height);
        write_register(BGA_REG_BPP, 32);
        write_register(BGA_REG_ENABLE, BGA_ENABLED | BGA_LINEAR_FRAMEBUFFER);

        _width = width;
        _height = height;

        logger_info("Resolution set to %dx%d.", width, height);

        return SUCCESS;
    }
}

BGA::BGA(DeviceAddress address) : PCIDevice(address, DeviceClass::FRAMEBUFFER)
{
    _framebuffer = make<MMIORange>(bar(0).range());
    set_resolution(handover()->framebuffer_width, handover()->framebuffer_height);
    graphic_did_find_framebuffer(_framebuffer->base(), handover()->framebuffer_width, handover()->framebuffer_height);
}

size_t BGA::size(FsHandle &handle)
{
    __unused(handle);

    return _framebuffer->size();
}

ResultOr<size_t> BGA::read(FsHandle &handle, void *buffer, size_t size)
{
    return _framebuffer->read(handle.offset(), buffer, size);
}

ResultOr<size_t> BGA::write(FsHandle &handle, const void *buffer, size_t size)
{
    return _framebuffer->write(handle.offset(), buffer, size);
}

Result BGA::call(FsHandle &handle, IOCall request, void *args)
{
    __unused(handle);

    if (request == IOCALL_DISPLAY_GET_MODE)
    {
        IOCallDisplayModeArgs *mode = (IOCallDisplayModeArgs *)args;

        mode->width = _width;
        mode->height = _height;

        return SUCCESS;
    }
    else if (request == IOCALL_DISPLAY_SET_MODE)
    {
        IOCallDisplayModeArgs *mode = (IOCallDisplayModeArgs *)args;

        Result result = set_resolution(mode->width, mode->height);

        if (result == SUCCESS)
        {
            graphic_did_find_framebuffer(_framebuffer->base(), mode->width, mode->height);
        }

        return result;
    }
    else if (request == IOCALL_DISPLAY_BLIT)
    {
        IOCallDisplayBlitArgs *blit = (IOCallDisplayBlitArgs *)args;

        for (int y = MAX(0, blit->blit_y); y < MIN(_height, blit->blit_y + blit->blit_height); y++)
        {
            for (int x = MAX(0, blit->blit_x); x < MIN(_width, blit->blit_x + blit->blit_width); x++)
            {
                uint32_t pixel = blit->buffer[y * blit->buffer_width + x];

                uint32_t converted_pixel = ((pixel >> 16) & 0x000000ff) |
                                           ((pixel)&0xff00ff00) |
                                           ((pixel << 16) & 0x00ff0000);

                _framebuffer->write32((y * _width + x) * sizeof(uint32_t), converted_pixel);
            }
        }

        return SUCCESS;
    }
    else
    {
        return ERR_INAPPROPRIATE_CALL_FOR_DEVICE;
    }
}
