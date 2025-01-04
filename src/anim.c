#include <stdbool.h>
#include <stddef.h>

#include <cglm/types.h>

typedef enum {
    INTERPOLATION_LINEAR,
    INTERPOLATION_CUBICSPLINE,
    INTERPOLATION_STEP,
} Interpolation;

typedef enum {
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
    vec4 color;
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
        KeyframeColor color;
    };
} Keyframe;

typedef struct Animation {
    char const *name;
    Keyframe *keyframes;
    size_t num_keyframes;
} Animation;

struct ActiveAnimation {
    float start_time;
    Animation *animation;
    size_t keyframe;
} typedef ActiveAnimation;
