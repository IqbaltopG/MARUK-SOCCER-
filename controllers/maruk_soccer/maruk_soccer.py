"""
MARUK (SOCCER) - Webots Robot Controller
Architecture: Sense (UDP / Keyboard) -> Think (Mecanum Kinematics) -> Act (Motors & Kicker)

Listens on UDP 0.0.0.0:5005 for commands from teleop/udp_sender.py.
Also supports built-in Webots Keyboard when clicking inside the 3D window!
"""

import sys
import os
import json
import socket
import select
from controller import Robot, Keyboard

# Add common directory to sys.path for kinematics
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "common")))
from kinematics import compute_mecanum_speeds

TIME_STEP = 16          # 16 ms simulation step (~60 FPS physics)
MAX_WHEEL_SPEED = 18.0  # Max rad/s for wheel motors
DRIBBLER_SPEED = 30.0   # Fast spin for dribbler roller
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
        self.dribble_toggle = False
        print(f"[MARUK SENSE] UDP Receiver listening on 0.0.0.0:{self.port}")

    def read(self, current_time_ms: int):
        """
        Returns: (vx, vy, omega, kick, dribble)
        Prioritizes UDP packet; falls back to Webots Keyboard.
        """
        vx, vy, omega = 0.0, 0.0, 0.0
        kick = 0
        dribble = 0
        udp_received = False

        # Drain socket buffer to get the freshest UDP packet
        while True:
            try:
                data, _ = self.sock.recvfrom(512)
                packet = json.loads(data.decode("utf-8"))
                vx = float(packet.get("vx", 0.0))
                vy = float(packet.get("vy", 0.0))
                omega = float(packet.get("w", 0.0))
                kick = int(packet.get("k", 0))
                dribble = int(packet.get("d", 0))
                udp_received = True
                self.last_udp_time = current_time_ms
            except (BlockingIOError, socket.error):
                break
            except Exception as e:
                print(f"[MARUK SENSE] UDP parse warning: {e}")
                break

        # If UDP is fresh, use it
        if udp_received or (current_time_ms - self.last_udp_time < FAILSAFE_TIMEOUT_MS and self.last_udp_time > 0):
            return vx, vy, omega, kick, dribble

        # Fallback: Webots Keyboard directly inside 3D viewport
        key = self.keyboard.getKey()
        while key > 0:
            if key in (ord('W'), Keyboard.UP):
                vx += 1.0
            elif key in (ord('S'), Keyboard.DOWN):
                vx -= 1.0
            elif key == ord('D'):
                vy += 1.0
            elif key == ord('A'):
                vy -= 1.0
            elif key in (ord('E'), Keyboard.RIGHT):
                omega += 1.0
            elif key in (ord('Q'), Keyboard.LEFT):
                omega -= 1.0
            elif key == ord(' '):
                kick = 1
            elif key in (Keyboard.SHIFT, ord('R')):
                self.dribble_toggle = not self.dribble_toggle

            key = self.keyboard.getKey()

        dribble = 1 if self.dribble_toggle else 0
        return vx, vy, omega, kick, dribble

# ==========================================
# LAYER 2: THINK (Kinematics & Logic)
# ==========================================
class SoccerBrain:
    def __init__(self, max_speed: float):
        self.max_speed = max_speed

    def process(self, vx: float, vy: float, omega: float, kick: int, dribble: int):
        # 1. Compute wheel speeds using pure Mecanum kinematics
        wheel_speeds = compute_mecanum_speeds(
            vx=vx, vy=vy, omega=omega, max_speed=self.max_speed
        )

        # 2. Dribbler roller target velocity
        dribbler_vel = DRIBBLER_SPEED if dribble else 0.0

        return wheel_speeds, dribbler_vel, bool(kick)

# ==========================================
# LAYER 3: ACT (Motor Actuation)
# ==========================================
class SoccerActuators:
    def __init__(self, robot: Robot):
        self.robot = robot

        # Wheel motors
        self.fl_motor = robot.getDevice("fl_motor")
        self.fr_motor = robot.getDevice("fr_motor")
        self.rl_motor = robot.getDevice("rl_motor")
        self.rr_motor = robot.getDevice("rr_motor")

        for motor in [self.fl_motor, self.fr_motor, self.rl_motor, self.rr_motor]:
            motor.setPosition(float('inf'))
            motor.setVelocity(0.0)

        # Front dribbler roller
        self.dribbler = robot.getDevice("dribbler_motor")
        self.dribbler.setPosition(float('inf'))
        self.dribbler.setVelocity(0.0)

    def apply(self, wheel_speeds: dict, dribbler_vel: float, kick: bool):
        # Apply velocities to all 4 wheels
        self.fl_motor.setVelocity(wheel_speeds["fl"])
        self.fr_motor.setVelocity(wheel_speeds["fr"])
        self.rl_motor.setVelocity(wheel_speeds["rl"])
        self.rr_motor.setVelocity(wheel_speeds["rr"])

        # Spin dribbler
        self.dribbler.setVelocity(dribbler_vel)

        # Kicker pulse
        if kick:
            # High speed momentary lunge to kick the ball forward
            self.fl_motor.setVelocity(MAX_WHEEL_SPEED)
            self.fr_motor.setVelocity(MAX_WHEEL_SPEED)
            self.rl_motor.setVelocity(MAX_WHEEL_SPEED)
            self.rr_motor.setVelocity(MAX_WHEEL_SPEED)

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
    print("[MARUK] MARUK RC Soccer Bot Controller READY!")
    print("[MARUK] SENSE: UDP 5005 + Webots Keyboard (WASD/QE)")
    print("[MARUK] THINK: 4-Wheel Mecanum Kinematics")
    print("[MARUK] ACT:   4 Wheel Motors + Front Dribbler")
    print("[MARUK] =========================================")

    step_count = 0
    while robot.step(TIME_STEP) != -1:
        current_time_ms = step_count * TIME_STEP
        step_count += 1

        # 1. SENSE
        vx, vy, omega, kick, dribble = sense.read(current_time_ms)

        # 2. THINK
        wheel_speeds, dribbler_vel, is_kicking = brain.process(vx, vy, omega, kick, dribble)

        # 3. ACT
        act.apply(wheel_speeds, dribbler_vel, is_kicking)

if __name__ == "__main__":
    main()
