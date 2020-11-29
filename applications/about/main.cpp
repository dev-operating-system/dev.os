#include <libsystem/BuildInfo.h>
#include <libwidget/Application.h>
#include <libwidget/Markup.h>
#include <libwidget/Widgets.h>
#include <libwidget/widgets/TextField.h>

static auto logo_based_on_color_scheme()
{
    auto path = theme_is_dark() ? "/Applications/about/logo-white.png"
                                : "/Applications/about/logo-black.png";

    return Bitmap::load_from_or_placeholder(path);
}

int main(int argc, char **argv)
{
    application_initialize(argc, argv);

    Window *window = window_create_from_file("/Applications/about/about.markup");

    window->with_widget<Image>("system-image", [&](auto image) {
        image->change_bitmap(logo_based_on_color_scheme());
    });

    window->with_widget<Label>("version-label", [&](auto label) {
        label->text(__BUILD_VERSION__);
    });

    window->with_widget<Label>("commit-label", [&](auto label) {
        label->text(__BUILD_GITREF__ "/" __BUILD_CONFIG__);
    });

    window->with_widget<Button>("license-button", [&](auto button) {
        button->on(Event::ACTION, [window](auto) {
            auto license_window = new Window(WINDOW_NONE);
            license_window->title("License");
            license_window->size({556, 416});

            auto field = new TextField(license_window->root(), TextModel::from_file("/Files/license.md"));
            field->attributes(LAYOUT_FILL);
            field->readonly(true);
            field->font(Font::create("mono").take_value());
            field->focus();

            license_window->show();
        });
    });

    window->with_widget<Button>("ok-button", [&](auto button) {
        button->on(Event::ACTION, [window](auto) {
            window->hide();
        });
    });

    window->show();

    return application_run();
}
