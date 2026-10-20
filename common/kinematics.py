"""
MARUK (SOCCER) - Common Kinematics Module
Provides 4-Wheel Mecanum / Holonomic Omni wheel speed calculations.
Pure functions: Zero hardware dependencies.
"""

from typing import Tuple, Dict

def apply_deadzone(val: float, threshold: float = 0.08) -> float:
    """Zero out small joystick drift values."""
    if abs(val) < threshold:
        return 0.0
    # Scale smoothly from threshold to 1.0
    sign = 1.0 if val > 0 else -1.0
    return sign * (abs(val) - threshold) / (1.0 - threshold)


def compute_mecanum_speeds(
    vx: float, 
    vy: float, 
    omega: float, 
    max_speed: float = 1.0,
    deadzone: float = 0.08
) -> Dict[str, float]:
    """
    Computes individual wheel speeds for a 4-wheel Mecanum base.
    
    Coordinate frame:
      vx    : Linear velocity along X-axis (Forward: +1.0, Backward: -1.0)
      vy    : Linear velocity along Y-axis (Strafe Right: +1.0, Strafe Left: -1.0)
      omega : Angular velocity around Z-axis (Clockwise / Turn Right: +1.0, CCW / Turn Left: -1.0)
    
    Returns:
      dict with keys: 'fl', 'fr', 'rl', 'rr' scaled to [-max_speed, max_speed]
    """
    # 1. Apply deadzone filtering
    vx = apply_deadzone(vx, deadzone)
    vy = apply_deadzone(vy, deadzone)
    omega = apply_deadzone(omega, deadzone)

    # 2. Mecanum mixing equations
    fl = vx + vy + omega
    fr = vx - vy - omega
    rl = vx - vy + omega
    rr = vx + vy - omega

    # 3. Normalization (preserve movement vector geometry if any wheel exceeds 1.0)
    max_val = max(abs(fl), abs(fr), abs(rl), abs(rr))
    if max_val > 1.0:
        fl /= max_val
        fr /= max_val
        rl /= max_val
        rr /= max_val

    # 4. Scale by max_speed
    return {
        "fl": fl * max_speed,
        "fr": fr * max_speed,
        "rl": rl * max_speed,
        "rr": rr * max_speed,
    }
