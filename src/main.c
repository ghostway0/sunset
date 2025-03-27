#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include <cglm/mat4.h>
#include <log.h>

#include "sunset/ecs.h"
#include "sunset/engine.h"
#include "sunset/render.h"
#include "sunset/vector.h"

int main() {
    int retval = 0;

    RenderConfig render_config = {
            .window_width = 1080,
            .window_height = 720,
    };

    World world;
    ecs_init(&world);

    Game game = {
            .world = world,
    };

    vector_init(game.plugins);
    vector_init(game.resources);

    vector_append(game.plugins,
            (Plugin){.object_path = "build/libbuilder.dylib"});

    log_set_level(LOG_TRACE);

    engine_run(render_config, &game);

    return retval;
}
