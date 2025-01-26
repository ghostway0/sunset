#include "sunset/input.h"
#include "sunset/engine.h"

void input_setup(EngineContext *engine_context) {
    Focus *focus = sunset_malloc(sizeof(Focus));
    *focus = FOCUS_NULL;

    REGISTER_RESOURCE(&engine_context->rman, input_focus, focus);
}
