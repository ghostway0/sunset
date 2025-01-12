#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include <cglm/types.h>

#include "internal/time_utils.h"
#include "sunset/ecs.h"
#include "sunset/images.h"
#include "sunset/render.h"

#define ANIMATION_DONE 1

typedef enum Interpolation {
    INTERPOLATION_LINEAR,
    INTERPOLATION_CUBICSPLINE,
    INTERPOLATION_STEP,
} Interpolation;

typedef enum KeyframeType {
    KEYFRAME_TRANSFORM,
    KEYFRAME_MORPH,
    KEYFRAME_WEIGHTS,
    KEYFRAME_VISIBILITY,
    KEYFRAME_COLOR,
    KEYFRAME_INTENSITY,
    KEYFRAME_TEXTURE,
    KEYFRAME_MATERIAL,
} KeyframeType;

typedef struct KeyframeTransform {
    vec3 direction;
    vec3 rotation;
    float scale;
    Interpolation dir_inter;
    Interpolation rot_inter;
    Interpolation scale_inter;
} KeyframeTransform;

typedef struct KeyframeMorph {
    size_t target;
    float weight;
    Interpolation weight_inter;
} KeyframeMorph;

typedef struct KeyframeWeights {
    size_t target;
    float weight;
    Interpolation weight_inter;
} KeyframeWeights;

typedef struct KeyframeVisibility {
    bool visible;
    Interpolation inter;
} KeyframeVisibility;

typedef struct KeyframeColor {
    Color color;
    Interpolation inter;
} KeyframeColor;

typedef struct KeyframeTexture {
    size_t target;
    struct texture *texture;
    Interpolation inter;
} KeyframeTexture;

typedef struct Keyframe {
    float duration;
    KeyframeType tag;

    union {
        KeyframeTransform transform;
        KeyframeMorph morph;
        KeyframeWeights weights;
        KeyframeVisibility visibility;
    };
} Keyframe;

typedef struct Animation {
    char const *name;
    Keyframe *keyframes;
    size_t num_keyframes;
} Animation;

typedef struct ActiveAnimation {
    float keyframe_time;
    Animation *animation;
    size_t keyframe;
} ActiveAnimation;

DECLARE_COMPONENT_ID(ActiveAnimation);
DECLARE_COMPONENT_ID(Morph);
DECLARE_COMPONENT_ID(Weights);
DECLARE_COMPONENT_ID(Color);

float interpolate_linear(float start, float end, float t) {
    return start + (end - start) * t;
}

float interpolate_cubic_spline(float start, float end, float t) {
    float t2 = t * t;
    float t3 = t * t * t;
    return start * (2 * t3 - 3 * t2 + 1) + end * (t3 - 2 * t2 + t);
}

float interpolate_step(float start, float end, float t) {
    return t < 0.5f ? start : end;
}

float interpolate(float start, float end, float t, Interpolation type) {
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

void interpolate_transform(World *world,
        EntityPtr eptr,
        KeyframeTransform *start_keyframe,
        KeyframeTransform *end_keyframe,
        float t) {
    Transform *transform =
            ecs_component_from_ptr(world, eptr, COMPONENT_ID(Transform));

    // ...

    transform->scale = interpolate(start_keyframe->scale,
            end_keyframe->scale,
            t,
            start_keyframe->scale_inter);
}

// void interpolate_morph(World *world,
//         EntityPtr eptr,
//         KeyframeMorph *start_keyframe,
//         KeyframeMorph *end_keyframe,
//         float t) {
//     Morph *morph = ecs_component_from_ptr(world, eptr,
//     COMPONENT_ID(Morph));
//
//     float weight = interpolate(start_keyframe->weight,
//             end_keyframe->weight,
//             t,
//             start_keyframe->weight_inter);
//
//     morph->weight = weight;
// }
//
// void interpolate_weights(World *world,
//         EntityPtr eptr,
//         KeyframeWeights *start_keyframe,
//         KeyframeWeights *end_keyframe,
//         float t) {
//     // Get the weights component
//     Weights *weights =
//             ecs_component_from_ptr(world, eptr, COMPONENT_ID(Weights));
//
//     // Interpolate the weight
//     float weight = interpolate(start_keyframe->weight,
//             end_keyframe->weight,
//             t,
//             start_keyframe->weight_inter);
//
//     // Apply the interpolated weight to the weights component
//     weights->weight = weight;
// }
//
// void interpolate_visibility(World *world,
//         EntityPtr eptr,
//         KeyframeVisibility *start_keyframe,
//         KeyframeVisibility *end_keyframe,
//         float t) {
//     // Get the visibility component
//     Visibility *visibility =
//             ecs_component_from_ptr(world, eptr,
//             COMPONENT_ID(Visibility));
//
//     // Interpolate the visibility
//     bool visible = start_keyframe->visible;
//     if (end_keyframe->visible != start_keyframe->visible) {
//         if (t >= 0.5f) {
//             visible = end_keyframe->visible;
//         }
//     }
//
//     // Apply the interpolated visibility to the visibility component
//     visibility->visible = visible;
// }

int anim_tick(World *world, EntityPtr eptr, float dt) {
    ActiveAnimation *anim = ecs_component_from_ptr(
            world, eptr, COMPONENT_ID(ActiveAnimation));

    anim->keyframe_time += dt;

    if (anim->keyframe + 1 >= anim->animation->num_keyframes) {
        return ANIMATION_DONE;
    }

    Keyframe *keyframe = &anim->animation->keyframes[anim->keyframe];

    while (anim->keyframe_time >= keyframe->duration) {
        anim->keyframe++;
        anim->keyframe_time -= keyframe->duration;

        if (anim->keyframe + 1 >= anim->animation->num_keyframes) {
            return ANIMATION_DONE;
        }

        keyframe = &anim->animation->keyframes[anim->keyframe];
    }

    Keyframe *next_keyframe =
            &anim->animation->keyframes[anim->keyframe + 1];

    assert(keyframe->tag == next_keyframe->tag);

    float t = anim->keyframe_time / keyframe->duration;

    switch (keyframe->tag) {
        case KEYFRAME_TRANSFORM:
            interpolate_transform(world,
                    eptr,
                    &keyframe->transform,
                    &next_keyframe->transform,
                    t);
            break;
        // case KEYFRAME_MORPH:
        //     interpolate_morph(world,
        //             eptr,
        //             &keyframe->morph,
        //             &next_keyframe->morph,
        //             t);
        //     break;
        // case KEYFRAME_WEIGHTS:
        //     interpolate_weights(world,
        //             eptr,
        //             &keyframe->weights,
        //             &next_keyframe->weights,
        //             t);
        //     break;
        // case KEYFRAME_VISIBILITY:
        //     interpolate_visibility(world,
        //             eptr,
        //             &keyframe->visibility,
        //             &next_keyframe->visibility,
        //             t);
        //     break;
        default:
            unreachable();
    }

    return 0;
}
