#include <log.h>

#include "internal/mem_utils.h"
#include "internal/time_utils.h"
#include "sunset/backend.h"
#include "sunset/commands.h"
#include "sunset/engine.h"
#include "sunset/events.h"
#include "sunset/fonts.h"
#include "sunset/images.h"
#include "sunset/input.h"
#include "sunset/rman.h"
#include "sunset/ui.h"

static void clicked(EngineContext *) {
    log_debug("clicked!");
}

static void thing(
        EngineContext *engine_context, void *, Event const event) {
    Key *key = (Key *)event.data;

    if (*key == KEY_ESCAPE) {
        uint32_t *focus =
                rman_get(&engine_context->rman, RESOURCE_ID(input_focus));
        *focus = FOCUS_UI;
        backend_show_mouse(&engine_context->render_context);
    }
}

static void thing2(
        EngineContext *engine_context, void *, Event const event) {
    MouseClickEvent *click = (MouseClickEvent *)event.data;
    uint32_t *focus =
            rman_get(&engine_context->rman, RESOURCE_ID(input_focus));

    if (click->button == MOUSE_BUTTON_LEFT
            && one_matches(*focus, FOCUS_NULL, FOCUS_NULL)) {
        *focus = FOCUS_MAIN;
        backend_hide_mouse(&engine_context->render_context);
    }
}

DECLARE_RESOURCE_ID(default_font);

void *init_default_font(void) {
    Font *font = sunset_malloc(sizeof(Font));
    load_font_psf2("font.psf", font);

    return font;
}

int plugin_load(EngineContext *engine_context) {
    event_queue_add_handler(&engine_context->event_queue,
            SYSTEM_EVENT_MOUSE_CLICK,
            (EventHandler){.handler_fn = thing2, .local_context = NULL});
    event_queue_add_handler(&engine_context->event_queue,
            SYSTEM_EVENT_KEY_PRESSED,
            (EventHandler){.handler_fn = thing, .local_context = NULL});

    Font *font = sunset_malloc(sizeof(Font));
    load_font_psf2("font.psf", font);

    REGISTER_RESOURCE(&engine_context->rman, default_font, font);

    UIContext *uictx = sunset_malloc(sizeof(UIContext));
    ui_init(uictx);

    Widget *widget1 = sunset_malloc(sizeof(Widget));
    *widget1 = (Widget){.tag = WIDGET_TEXT,
            .text = {"test", font, 24},
            .active = true,
            .bounds = {100, 80, 100, 10},
            .parent = NULL,
            .children = NULL};
    ui_add_widget(uictx->root, widget1);

    Widget *widget2 = sunset_malloc(sizeof(Widget));
    *widget2 = (Widget){.tag = WIDGET_BUTTON,
            .button = {.clicked_callback = clicked},
            .bounds = {100, 100, 100, 100},
            .active = true,
            .parent = NULL,
            .style = {.color = COLOR_WHITE, .solid = false},
            .children = NULL};
    ui_add_widget(uictx->root, widget2);

    Widget *widget3 = sunset_malloc(sizeof(Widget));
    *widget3 = (Widget){.tag = WIDGET_TEXT,
            .text = {"fun", font, 24},
            .bounds = {30, 30, 0, 0},
            .style = {.relative = true},
            .active = true};
    ui_add_widget(widget2, widget3);

    Widget *widget4 = sunset_malloc(sizeof(Widget));
    *widget4 = (Widget){.tag = WIDGET_INPUT,
            .input = {.text = NULL, .font = font, 24},
            .bounds = {30, 30, 100, 100},
            .style = {.color = COLOR_WHITE},
            .active = true};
    ui_add_widget(uictx->root, widget4);

    engine_context->active_ui = uictx;

    return 0;
}
