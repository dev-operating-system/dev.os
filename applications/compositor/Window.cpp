#include <libsystem/Assert.h>
#include <libsystem/Logger.h>
#include <libsystem/core/CString.h>

#include "compositor/Client.h"
#include "compositor/Manager.h"
#include "compositor/Protocol.h"
#include "compositor/Renderer.h"
#include "compositor/Window.h"

Window::Window(
    int id,
    WindowFlag flags,
    WindowType type,
    struct Client *client,
    Rectangle bound,
    RefPtr<Bitmap> frontbuffer,
    RefPtr<Bitmap> backbuffer)
    : _id(id),
      _flags(flags),
      _type(type),
      _client(client),
      _bound(bound),
      _frontbuffer(frontbuffer),
      _backbuffer(backbuffer)
{
    manager_register_window(this);
}

Window::~Window()
{
    manager_unregister_window(this);
}

Rectangle Window::bound()
{
    return _bound;
}

Rectangle Window::cursor_capture_bound()
{
    if (_flags & WINDOW_RESIZABLE)
    {
        return bound().expended({16});
    }
    else
    {
        return bound();
    }
}

void Window::move(Vec2i new_position)
{
    renderer_region_dirty(bound());

    _bound = _bound.moved(new_position);

    renderer_region_dirty(bound());
}

void Window::resize(Rectangle new_bound)
{
    renderer_region_dirty(bound());

    _bound = new_bound;

    renderer_region_dirty(bound());
}

void Window::send_event(Event event)
{
    CompositorMessage message = {
        .type = COMPOSITOR_MESSAGE_EVENT_WINDOW,
        .event_window = {
            .id = _id,
            .event = event,
        },
    };

    _client->send_message(message);
}

void Window::handle_mouse_move(Vec2i old_position, Vec2i position, MouseButton buttons)
{
    Event event = {
        .type = Event::MOUSE_MOVE,
        .accepted = false,
        .mouse = {
            .position = position,
            .old_position = old_position,
            .button = MOUSE_NO_BUTTON,
            .buttons = buttons,
        },

        .keyboard = {},
    };

    send_event(event);
}

static void handle_mouse_button(Window &window, MouseButton button, MouseButton old_buttons, MouseButton buttons, Vec2i position)
{
    bool was_button_pressed = old_buttons & button;
    bool is_button_pressed = buttons & button;

    if (is_button_pressed && !was_button_pressed)
    {
        Event event = {
            .type = Event::MOUSE_BUTTON_PRESS,
            .accepted = false,
            .mouse = {
                .position = position,
                .old_position = position,
                .button = button,
                .buttons = buttons,
            },

            .keyboard = {},
        };

        window.send_event(event);
    }

    if (was_button_pressed && !is_button_pressed)
    {
        Event event = {
            .type = Event::MOUSE_BUTTON_RELEASE,
            .accepted = false,
            .mouse = {
                .position = position,
                .old_position = position,
                .button = button,
                .buttons = buttons,
            },

            .keyboard = {},
        };

        window.send_event(event);
    }
}

void Window::handle_mouse_buttons(MouseButton old_buttons, MouseButton buttons, Vec2i position)
{
    handle_mouse_button(*this, MOUSE_BUTTON_LEFT, old_buttons, buttons, position);
    handle_mouse_button(*this, MOUSE_BUTTON_RIGHT, old_buttons, buttons, position);
    handle_mouse_button(*this, MOUSE_BUTTON_MIDDLE, old_buttons, buttons, position);
}

void Window::handle_double_click(Vec2i position)
{
    Event event = {
        .type = Event::MOUSE_DOUBLE_CLICK,
        .accepted = false,
        .mouse = {
            .position = position,
            .old_position = position,
            .button = MOUSE_BUTTON_LEFT,
            .buttons = MOUSE_BUTTON_LEFT,
        },

        .keyboard = {},
    };

    send_event(event);
}

void Window::get_focus()
{
    renderer_region_dirty(bound());

    Event event = {};
    event.type = Event::GOT_FOCUS;
    send_event(event);
}

void Window::lost_focus()
{
    renderer_region_dirty(bound());

    Event event = {};
    event.type = Event::LOST_FOCUS;
    send_event(event);
}

void Window::flip_buffers(int frontbuffer_handle, Vec2i frontbuffer_size, int backbuffer_handle, Vec2i backbuffer_size, Rectangle region)
{
    swap(_frontbuffer, _backbuffer);

    if (_frontbuffer->handle() != frontbuffer_handle)
    {

        auto new_frontbuffer = Bitmap::create_shared_from_handle(frontbuffer_handle, frontbuffer_size);

        if (!new_frontbuffer.success())
        {
            logger_error("Client application gave us a jankie shared memory object id");
            return;
        }

        _frontbuffer = new_frontbuffer.take_value();
    }

    if (_backbuffer->handle() != backbuffer_handle)
    {

        auto new_backbuffer = Bitmap::create_shared_from_handle(backbuffer_handle, backbuffer_size);

        if (!new_backbuffer.success())
        {
            logger_error("Client application gave us a jankie shared memory object id");
            return;
        }

        _backbuffer = new_backbuffer.take_value();
    }

    renderer_region_dirty(region.offset(bound().position()));
}
