/**
 * MARUK (SOCCER) - Kinematics Math Header (2WD & 4WD)
 * Pure C++ Header-only library: Zero hardware dependencies.
 */

#ifndef MARUK_KINEMATICS_H
#define MARUK_KINEMATICS_H

#include <math.h>

struct DiffSpeeds {
    float left;   // Normalized -1.0 to +1.0
    float right;  // Normalized -1.0 to +1.0
};

struct WheelSpeeds {
    float fl;
    float fr;
    float rl;
    float rr;
};

inline float apply_deadzone(float val, float threshold = 0.08f) {
    if (fabsf(val) < threshold) {
        return 0.0f;
    }
    float sign = (val > 0.0f) ? 1.0f : -1.0f;
    return sign * (fabsf(val) - threshold) / (1.0f - threshold);
}

/**
 * 2-Wheel Differential Drive Mixing (MIT-WEEK 2026)
 * Left  = vx - omega
 * Right = vx + omega
 */
inline DiffSpeeds compute_diff_speeds(
    float vx, 
    float omega, 
    float max_speed = 1.0f,
    float deadzone = 0.08f
) {
    vx = apply_deadzone(vx, deadzone);
    omega = apply_deadzone(omega, deadzone);

    float left = vx + omega;
    float right = vx - omega;

    float max_val = fabsf(left);
    if (fabsf(right) > max_val) max_val = fabsf(right);

    if (max_val > 1.0f) {
        left /= max_val;
        right /= max_val;
    }

    DiffSpeeds speeds;
    speeds.left = left * max_speed;
    speeds.right = right * max_speed;
    return speeds;
}

/**
 * 4-Wheel Mecanum Mixing
 */
inline WheelSpeeds compute_mecanum_speeds(
    float vx, 
    float vy, 
    float omega, 
    float max_speed = 1.0f,
    float deadzone = 0.08f
) {
    vx = apply_deadzone(vx, deadzone);
    vy = apply_deadzone(vy, deadzone);
    omega = apply_deadzone(omega, deadzone);

    float fl = vx + vy + omega;
    float fr = vx - vy - omega;
    float rl = vx - vy + omega;
    float rr = vx + vy - omega;

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

    WheelSpeeds speeds;
    speeds.fl = fl * max_speed;
    speeds.fr = fr * max_speed;
    speeds.rl = rl * max_speed;
    speeds.rr = rr * max_speed;
    return speeds;
}

#endif // MARUK_KINEMATICS_H
