"""
MARUK (SOCCER) - Webots Robot Controller (2WD Differential Drive)
Architecture: Sense (UDP / Webots Keyboard) -> Think (2WD Kinematics) -> Act (Left & Right Motors)
Conforms 100% to MIT-WEEK 2026 Car Soccer Rules.
"""

import sys
import os
import json
import socket
from controller import Robot, Keyboard

# Add common directory to sys.path for kinematics
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "common")))
from kinematics import compute_differential_speeds

TIME_STEP = 16          # 16 ms simulation step (~60 FPS physics)
MAX_WHEEL_SPEED = 14.0  # Max rad/s for standard yellow TT motor (~200 RPM)
UDP_PORT = 5005
FAILSAFE_TIMEOUT_MS = 400

# ==========================================
# LAYER 1: SENSE (Input Gathering)
# ==========================================
class InputSense:
    def __init__(self, port: int, keyboard: Keyboard):
        self.keyboard = keyboard
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(("0.0.0.0", self.port))
        self.sock.setblocking(False)

        self.last_udp_time = 0
        print(f"[MARUK SENSE] 2WD UDP Receiver listening on 0.0.0.0:{self.port}")

    def read(self, current_time_ms: int):
        """
        Returns: (vx, omega)
        Prioritizes UDP packet; falls back to Webots Keyboard.
        """
        vx, omega = 0.0, 0.0
        udp_received = False

        # Drain socket buffer to get the freshest UDP packet
        while True:
            try:
                data, _ = self.sock.recvfrom(512)
                packet = json.loads(data.decode("utf-8"))
                vx = float(packet.get("vx", 0.0))
                omega = float(packet.get("w", packet.get("omega", 0.0)))
                udp_received = True
                self.last_udp_time = current_time_ms
            except (BlockingIOError, socket.error):
                break
            except Exception as e:
                print(f"[MARUK SENSE] UDP parse warning: {e}")
                break

        # If UDP packet is fresh, return it
        if udp_received or (current_time_ms - self.last_udp_time < FAILSAFE_TIMEOUT_MS and self.last_udp_time > 0):
            return vx, omega

        # Fallback: Webots Keyboard directly inside 3D viewport
        key = self.keyboard.getKey()
        while key > 0:
            if key in (ord('W'), Keyboard.UP):
                vx += 1.0
            elif key in (ord('S'), Keyboard.DOWN):
                vx -= 1.0
            if key in (ord('D'), Keyboard.RIGHT):
                omega += 1.0   # Turn Right (CW)
            elif key in (ord('A'), Keyboard.LEFT):
                omega -= 1.0   # Turn Left (CCW)

            key = self.keyboard.getKey()

        return vx, omega

# ==========================================
# LAYER 2: THINK (2WD Kinematics)
# ==========================================
class SoccerBrain:
    def __init__(self, max_speed: float):
        self.max_speed = max_speed

    def process(self, vx: float, omega: float):
        return compute_differential_speeds(
            vx=vx, omega=omega, max_speed=self.max_speed
        )

# ==========================================
# LAYER 3: ACT (Motor Actuation)
# ==========================================
class SoccerActuators:
    def __init__(self, robot: Robot):
        self.robot = robot

        # 2 Driven Wheels (Yellow Motors)
        self.left_motor = robot.getDevice("left_motor")
        self.right_motor = robot.getDevice("right_motor")

        self.left_motor.setPosition(float('inf'))
        self.right_motor.setPosition(float('inf'))
        self.left_motor.setVelocity(0.0)
        self.right_motor.setVelocity(0.0)

    def apply(self, wheel_speeds: dict):
        self.left_motor.setVelocity(wheel_speeds["left"])
        self.right_motor.setVelocity(wheel_speeds["right"])

# ==========================================
# SANG MANDOR (MAIN LOOP)
# ==========================================
def main():
    robot = Robot()
    keyboard = robot.getKeyboard()
    keyboard.enable(TIME_STEP)

    sense = InputSense(port=UDP_PORT, keyboard=keyboard)
    brain = SoccerBrain(max_speed=MAX_WHEEL_SPEED)
    act = SoccerActuators(robot)

    print("[MARUK] =========================================")
    print("[MARUK] MARUK 2WD Robot Soccer Controller READY!")
    print("[MARUK] Regulations: MIT-WEEK 2026 Compliant")
    print("[MARUK] SENSE: UDP 5005 / Webots Keyboard (WASD)")
    print("[MARUK] THINK: 2WD Differential Drive Mixing")
    print("[MARUK] ACT:   Left & Right Yellow TT Motors")
    print("[MARUK] =========================================")

    step_count = 0
    while robot.step(TIME_STEP) != -1:
        current_time_ms = step_count * TIME_STEP
        step_count += 1

        # 1. SENSE
        vx, omega = sense.read(current_time_ms)

        # 2. THINK
        wheel_speeds = brain.process(vx, omega)

        # 3. ACT
        act.apply(wheel_speeds)

if __name__ == "__main__":
    main()
