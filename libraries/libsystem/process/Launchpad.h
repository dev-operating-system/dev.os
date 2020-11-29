#pragma once

#include <abi/Handle.h>
#include <abi/Launchpad.h>

Launchpad *launchpad_create(const char *name, const char *executable);

void launchpad_destroy(Launchpad *launchpad);

void launchpad_argument(Launchpad *launchpad, const char *argument);

void launchpad_environment(Launchpad *launchpad, const char *buffer);

void launchpad_handle(Launchpad *launchpad, Handle *handle_to_pass, int destination);

Result launchpad_launch(Launchpad *launchpad, int *pid);
