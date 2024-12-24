#include <stdbool.h>

#include "sunset/commands.h"
#include "sunset/physics.h"
#include "sunset/vector.h"

struct event_queue;
struct font;
struct scene;
struct button;
struct ui_context;

struct engine_context {
    vector(struct ui_context) ui_contexts;
    struct ui_context *active_ui;

    struct command_buffer command_buffer;
    void *render_context;
    struct scene *scene;

    struct event_queue event_queue;

    float dt;
};

int engine_run(void);
