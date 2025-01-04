#include <cglm/types.h>

enum LightType {
    LIGHT_DIRECTIONAL,
    LIGHT_SPOTLIGHT,
    LIGHT_POINT,
    LIGHT_AMBIENT,
} typedef LightType;

struct Light {
    LightType type;
    vec3 position;
    vec3 color;
    float intensity;
} typedef Light;
