#ifndef SUNSET_ANIM_H_
#define SUNSET_ANIM_H_

#include <stdbool.h>
#include <stddef.h>

#include <cglm/types.h>

#include "sunset/ecs.h"
#include "sunset/images.h"

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
    float keyframe_start_ms;
    Animation *animation;
    size_t keyframe;
} ActiveAnimation;

DECLARE_COMPONENT_ID(ActiveAnimation);

#endif // SUNSET_ANIM_H_
