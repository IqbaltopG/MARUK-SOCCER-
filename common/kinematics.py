"""
MARUK (SOCCER) - Common Kinematics Module
Provides wheel speed calculations for:
1. 2-Wheel Differential Drive (2WD with Caster) - MIT-WEEK 2026 Official Spec
2. 4-Wheel Mecanum Drive (Holonomic Omni)
Pure functions: Zero hardware dependencies.
"""

from typing import Dict

def apply_deadzone(val: float, threshold: float = 0.08) -> float:
    """Zero out small joystick drift values."""
    if abs(val) < threshold:
        return 0.0
    sign = 1.0 if val > 0 else -1.0
    return sign * (abs(val) - threshold) / (1.0 - threshold)


def compute_differential_speeds(
    vx: float, 
    omega: float, 
    max_speed: float = 1.0, 
    deadzone: float = 0.08
) -> Dict[str, float]:
    """
    Computes wheel speeds for a 2-Wheel Differential Drive robot with caster.
    
    Coordinate frame:
      vx    : Linear velocity (Forward: +1.0, Backward: -1.0)
      omega : Angular velocity (Turn Right / Clockwise: +1.0, Turn Left / CCW: -1.0)
      
    Differential mixing:
      left  = vx - omega
      right = vx + omega
      
    Returns:
      dict with keys: 'left', 'right' scaled to [-max_speed, max_speed]
    """
    vx = apply_deadzone(vx, deadzone)
    omega = apply_deadzone(omega, deadzone)

    left = vx - omega
    right = vx + omega

    # Normalize if any wheel exceeds 1.0 (preserves turn arc)
    max_val = max(abs(left), abs(right))
    if max_val > 1.0:
        left /= max_val
        right /= max_val

    return {
        "left": round(left * max_speed, 4),
        "right": round(right * max_speed, 4)
    }


def compute_mecanum_speeds(
    vx: float, 
    vy: float, 
    omega: float, 
    max_speed: float = 1.0,
    deadzone: float = 0.08
) -> Dict[str, float]:
    """
    Computes individual wheel speeds for a 4-wheel Mecanum base.
    """
    vx = apply_deadzone(vx, deadzone)
    vy = apply_deadzone(vy, deadzone)
    omega = apply_deadzone(omega, deadzone)

    fl = vx + vy + omega
    fr = vx - vy - omega
    rl = vx - vy + omega
    rr = vx + vy - omega

    max_val = max(abs(fl), abs(fr), abs(rl), abs(rr))
    if max_val > 1.0:
        fl /= max_val
        fr /= max_val
        rl /= max_val
        rr /= max_val

    return {
        "fl": round(fl * max_speed, 4),
        "fr": round(fr * max_speed, 4),
        "rl": round(rl * max_speed, 4),
        "rr": round(rr * max_speed, 4),
    }
