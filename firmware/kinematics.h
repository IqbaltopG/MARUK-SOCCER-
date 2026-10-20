/**
 * MARUK (SOCCER) - Mecanum Kinematics Math Header
 * Pure C++ Header-only library: Zero hardware dependencies.
 */

#ifndef MARUK_KINEMATICS_H
#define MARUK_KINEMATICS_H

#include <math.h>

struct WheelSpeeds {
    float fl; // Front-Left
    float fr; // Front-Right
    float rl; // Rear-Left
    float rr; // Rear-Right
};

inline float apply_deadzone(float val, float threshold = 0.08f) {
    if (fabsf(val) < threshold) {
        return 0.0f;
    }
    float sign = (val > 0.0f) ? 1.0f : -1.0f;
    return sign * (fabsf(val) - threshold) / (1.0f - threshold);
}

inline WheelSpeeds compute_mecanum_speeds(
    float vx, 
    float vy, 
    float omega, 
    float max_speed = 1.0f,
    float deadzone = 0.08f
) {
    // 1. Deadzone
    vx = apply_deadzone(vx, deadzone);
    vy = apply_deadzone(vy, deadzone);
    omega = apply_deadzone(omega, deadzone);

    // 2. Mecanum mixing equations
    float fl = vx + vy + omega;
    float fr = vx - vy - omega;
    float rl = vx - vy + omega;
    float rr = vx + vy - omega;

    // 3. Normalization if any speed exceeds 1.0
    float max_val = fabsf(fl);
    if (fabsf(fr) > max_val) max_val = fabsf(fr);
    if (fabsf(rl) > max_val) max_val = fabsf(rl);
    if (fabsf(rr) > max_val) max_val = fabsf(rr);

    if (max_val > 1.0f) {
        fl /= max_val;
        fr /= max_val;
        rl /= max_val;
        rr /= max_val;
    }

    // 4. Return scaled speeds
    WheelSpeeds speeds;
    speeds.fl = fl * max_speed;
    speeds.fr = fr * max_speed;
    speeds.rl = rl * max_speed;
    speeds.rr = rr * max_speed;
    return speeds;
}

#endif // MARUK_KINEMATICS_H
