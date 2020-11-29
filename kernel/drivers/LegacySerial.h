#pragma once

#include <libsystem/thread/Lock.h>
#include <libutils/RingBuffer.h>

#include "architectures/x86/kernel/COM.h"

#include "kernel/devices/LegacyDevice.h"

class LegacySerial : public LegacyDevice
{
private:
    RingBuffer _buffer{4096};
    Lock _buffer_lock;

    COMPort port() { return (COMPort)legacy_address(); }

public:
    LegacySerial(DeviceAddress address);

    void handle_interrupt() override;

    bool can_read(FsHandle &handle) override;

    ResultOr<size_t> read(FsHandle &handle, void *buffer, size_t size) override;

    ResultOr<size_t> write(FsHandle &handle, const void *buffer, size_t size) override;
};
