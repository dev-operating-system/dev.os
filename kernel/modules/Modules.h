#pragma once

#include "kernel/handover/Handover.h"

void modules_initialize(Handover *handover);

void ramdisk_load(Module *module);
