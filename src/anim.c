#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "internal/time_utils.h"
#include "vector.h"

#include "sunset/anim.h"

static float interpolate_linear(float start, float end, float t) {
    return start + (end - start) * t;
}

static float interpolate_cubic_spline(float start, float end, float t) {
    float t2 = t * t;
    float t3 = t * t * t;
    return start * (2 * t3 - 3 * t2 + 1) + end * (t3 - 2 * t2 + t);
}

static float interpolate_step(float start, float end, float t) {
    return t < 0.5f ? start : end;
}

static float interpolate(
        float start, float end, float t, Interpolation type) {
    switch (type) {
        case INTERPOLATION_LINEAR:
            return interpolate_linear(start, end, t);
        case INTERPOLATION_CUBICSPLINE:
            return interpolate_cubic_spline(start, end, t);
        case INTERPOLATION_STEP:
            return interpolate_step(start, end, t);
        default:
            unreachable();
    }
}

static int anim_step(ActiveAnimation *anim, float dt) {
    anim->keyframe_time += dt;

    if (anim->keyframe >= anim->animation->num_keyframes) {
        return ANIMATION_DONE;
    }

    Keyframe *keyframe = &anim->animation->keyframes[anim->keyframe];

    // advances keyframes until current time
    while (anim->keyframe_time >= keyframe->duration) {
        anim->keyframe++;
        anim->keyframe_time -= keyframe->duration;

        if (anim->keyframe >= anim->animation->num_keyframes) {
            break;
        }

        keyframe = &anim->animation->keyframes[anim->keyframe];
    }

    float t = anim->keyframe_time / keyframe->duration;

    for (size_t i = 0; i < keyframe->num_targets; i++) {
        KeyframeTarget *target = &keyframe->targets[i];
        *target->value =
                interpolate(target->start, target->end, t, target->interp);
    }

    return anim->keyframe >= anim->animation->num_keyframes;
}

int anim_tick(AnimationContext *anim_ctx, float dt) {
    for (size_t i = 0; i < vector_size(anim_ctx->anims); i++) {
        if (anim_step(&anim_ctx->anims[i], dt) == ANIMATION_DONE) {
            vector_remove_index(anim_ctx->anims, i);
        }
    }

    return 0;
}
