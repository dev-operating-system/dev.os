#include <libgraphic/Painter.h>
#include <libwidget/widgets/Image.h>

Image::Image(Widget *parent, RefPtr<Bitmap> bitmap)
    : Widget(parent), _bitmap(bitmap)
{
}

void Image::change_bitmap(RefPtr<Bitmap> bitmap)
{
    if (_bitmap != bitmap)
    {
        _bitmap = bitmap;
        should_repaint();
    }
}

void Image::change_scaling(ImageScalling scaling)
{
    if (_scalling != scaling)
    {
        _scalling = scaling;
        should_repaint();
    }
}

void Image::paint(Painter &painter, Rectangle rectangle)
{
    __unused(rectangle);

    if (!_bitmap)
    {
        return;
    }

    Rectangle destination = bound();

    if (_scalling == ImageScalling::CENTER)
    {
        destination = _bitmap->bound().centered_within(bound());
    }
    else if (_scalling == ImageScalling::STRETCH)
    {
        destination = bound();
    }

    painter.blit_bitmap(*_bitmap, _bitmap->bound(), destination);
}

Vec2i Image::size()
{
    if (_bitmap)
    {
        return _bitmap->bound().size();
    }
    else
    {
        return bound().size();
    }
}
