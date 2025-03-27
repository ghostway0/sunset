#ifndef SUNSET_ANIM_H_
#define SUNSET_ANIM_H_

#include <stddef.h>

#include "sunset/ecs.h"

#define ANIMATION_LOOP 1
#define ANIMATION_DONE 0

typedef enum Interpolation {
    INTERPOLATION_LINEAR,
    INTERPOLATION_CUBICSPLINE,
    INTERPOLATION_STEP,
} Interpolation;

typedef struct KeyframeTarget {
    float start;
    float end;
    float *value;
    Interpolation interp;
} KeyframeTarget;

typedef struct Keyframe {
    float duration;
    KeyframeTarget *targets;
    size_t num_targets;
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

extern DECLARE_COMPONENT_ID(ActiveAnimation);

typedef struct AnimationContext {
    vector(ActiveAnimation) anims;
} AnimationContext;

#endif // SUNSET_ANIM_H_
