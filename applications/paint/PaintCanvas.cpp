#include <libgraphic/Painter.h>

#include "paint/PaintCanvas.h"
#include "paint/PaintTool.h"

PaintCanvas::PaintCanvas(Widget *parent, RefPtr<PaintDocument> document)
    : Widget(parent), _document(document)
{
}

void PaintCanvas::paint(Painter &painter, Rectangle rectangle)
{
    __unused(rectangle);

    painter.fill_checkboard(paint_area(), 8, Colors::WHITE, Colors::GAINSBORO);
    painter.draw_rectangle(paint_area(), color(THEME_BORDER));

    painter.blit_bitmap(
        _document->bitmap(),
        _document->bound(), paint_area());
}

void PaintCanvas::event(Event *event)
{
    if (is_mouse_event(event))
    {
        Event event_copy = *event;
        event_copy.mouse.old_position = event_copy.mouse.old_position - paint_area().position();
        event_copy.mouse.position = event_copy.mouse.position - paint_area().position();

        Color color = Colors::MAGENTA;
        if (event_copy.mouse.buttons & MOUSE_BUTTON_LEFT)
        {
            color = _document->primary_color();
        }
        else if (event_copy.mouse.buttons & MOUSE_BUTTON_RIGHT)
        {
            color = _document->secondary_color();
        }

        if (_tool)
            _tool->event(*_document, event_copy, color);

        if (event_copy.accepted)
        {
            event->accepted = true;
        }

        if (_document->dirty())
        {
            should_repaint();
            _document->dirty(false);
        }
    }
}
