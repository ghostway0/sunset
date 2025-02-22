#include "sunset/engine.h"

#include "sunset/input.h"

DECLARE_RESOURCE_ID(input_focus);

void input_setup(EngineContext *engine_context) {
    Focus focus = FOCUS_NULL;;
    REGISTER_RESOURCE(&engine_context->rman, input_focus, focus);
}
