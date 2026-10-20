#!/usr/bin/env python3
"""
MARUK (SOCCER) - Unified Teleoperation Station (UDP Transmitter)
Author: MARUK Robotics Framework

Controls:
  Keyboard (Pygame Window):
    W / S          : Forward / Backward (Vx)
    A / D          : Strafe Left / Strafe Right (Vy)
    Q / E          : Rotate CCW / Rotate CW (Omega)
    SPACE          : Kicker Pulse
    LSHIFT / R     : Dribbler Toggle (On / Off)
    ESC            : Quit

  Gamepad / Joystick:
    Left Stick     : Translation (Vx, Vy)
    Right Stick X  : Rotation (Omega)
    Button A / Cross: Kick
    Right Bumper (R1): Dribbler Toggle

Broadcasts JSON packets over UDP:
  {"vx": float, "vy": float, "w": float, "k": int, "d": int}
"""

import sys
import time
import json
import socket
import argparse
import os

# Suppress Pygame welcome banner
os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = "hide"
import pygame

DEFAULT_TARGET_IP = "127.0.0.1"   # Default to Webots localhost
DEFAULT_TARGET_PORT = 5005
RATE_HZ = 30                      # 30 packets per second (ideal for ESP32 and Webots)

def main():
    parser = argparse.ArgumentParser(description="MARUK RC Soccer Bot Teleop Station")
    parser.add_argument("--ip", default=DEFAULT_TARGET_IP, help="Target IP (Default: 127.0.0.1 for Webots, or ESP32 IP)")
    parser.add_argument("--port", type=int, default=DEFAULT_TARGET_PORT, help="Target UDP Port (Default: 5005)")
    parser.add_argument("--speed", type=float, default=1.0, help="Max speed multiplier (0.1 to 1.0)")
    args = parser.parse_args()

    # Setup UDP Socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    target_addr = (args.ip, args.port)

    # Initialize Pygame
    pygame.init()
    pygame.joystick.init()

    # Check for gamepads
    joystick = None
    if pygame.joystick.get_count() > 0:
        joystick = pygame.joystick.Joystick(0)
        joystick.init()
        print(f"[MARUK TELEOP] Gamepad detected: {joystick.get_name()}")
    else:
        print("[MARUK TELEOP] No gamepad detected. Using Keyboard control.")

    # Create Pygame status window
    width, height = 480, 320
    screen = pygame.display.set_mode((width, height))
    pygame.display.set_caption("MARUK Soccer Teleop")
    font = pygame.font.SysFont("monospace", 16)
    font_bold = pygame.font.SysFont("monospace", 18, bold=True)
    clock = pygame.time.Clock()

    dribbler_active = False
    kicker_pulse = False
    last_dribble_btn_state = False

    print(f"[MARUK TELEOP] Broadcasting to {target_addr[0]}:{target_addr[1]} at {RATE_HZ}Hz")
    print("[MARUK TELEOP] Window active. Focus window to drive!")

    running = True
    while running:
        vx, vy, omega = 0.0, 0.0, 0.0
        kicker_pulse = False

        # Event handling
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
                elif event.key in (pygame.K_LSHIFT, pygame.K_r):
                    dribbler_active = not dribbler_active
                elif event.key == pygame.K_SPACE:
                    kicker_pulse = True

        # 1. Read Keyboard input
        keys = pygame.key.get_pressed()
        if keys[pygame.K_w] or keys[pygame.K_UP]:
            vx += 1.0
        if keys[pygame.K_s] or keys[pygame.K_DOWN]:
            vx -= 1.0
        if keys[pygame.K_d]:
            vy += 1.0   # Strafe Right
        if keys[pygame.K_a]:
            vy -= 1.0   # Strafe Left
        if keys[pygame.K_e] or keys[pygame.K_RIGHT]:
            omega += 1.0 # Rotate CW
        if keys[pygame.K_q] or keys[pygame.K_LEFT]:
            omega -= 1.0 # Rotate CCW
        if keys[pygame.K_SPACE]:
            kicker_pulse = True

        # 2. Read Gamepad if connected
        if joystick is not None:
            # Axis 1: Left stick Y (inverted: up is negative in pygame)
            joy_y = -joystick.get_axis(1)
            # Axis 0: Left stick X
            joy_x = joystick.get_axis(0)
            # Axis 2 or 3: Right stick X (rotation)
            joy_rot = joystick.get_axis(2) if joystick.get_numaxes() > 2 else 0.0

            if abs(joy_y) > 0.1: vx = joy_y
            if abs(joy_x) > 0.1: vy = joy_x
            if abs(joy_rot) > 0.1: omega = joy_rot

            # Buttons (Button 0 = A / Cross)
            if joystick.get_button(0):
                kicker_pulse = True

            # Button 5 = RB / R1 for dribbler toggle
            current_rb = joystick.get_button(5) if joystick.get_numbuttons() > 5 else False
            if current_rb and not last_dribble_btn_state:
                dribbler_active = not dribbler_active
            last_dribble_btn_state = current_rb

        # Scale speeds
        vx = round(vx * args.speed, 3)
        vy = round(vy * args.speed, 3)
        omega = round(omega * args.speed, 3)

        # 3. Build & Send MARUK UDP packet
        packet = {
            "vx": vx,
            "vy": vy,
            "w": omega,
            "k": 1 if kicker_pulse else 0,
            "d": 1 if dribbler_active else 0
        }

        payload = json.dumps(packet).encode("utf-8")
        try:
            sock.sendto(payload, target_addr)
        except Exception as e:
            print(f"[ERROR] UDP send failed: {e}")

        # 4. Render UI
        screen.fill((20, 24, 30))
        title = font_bold.render("MARUK SOCCER TELEOP", True, (0, 255, 180))
        target_info = font.render(f"Target: {target_addr[0]}:{target_addr[1]}", True, (180, 180, 180))
        cmd_info = font.render(f"Vx:{vx:+.2f}  Vy:{vy:+.2f}  W:{omega:+.2f}", True, (255, 255, 255))
        dribble_col = (50, 255, 50) if dribbler_active else (120, 120, 120)
        kick_col = (255, 50, 50) if kicker_pulse else (120, 120, 120)
        dribble_info = font.render(f"Dribbler: {'ON' if dribbler_active else 'OFF'}", True, dribble_col)
        kick_info = font.render(f"Kicker:   {'FIRE' if kicker_pulse else 'READY'}", True, kick_col)
        help_info1 = font.render("WASD: Move | QE: Rotate", True, (140, 140, 160))
        help_info2 = font.render("SPACE: Kick | SHIFT: Dribble", True, (140, 140, 160))

        screen.blit(title, (20, 20))
        screen.blit(target_info, (20, 50))
        pygame.draw.line(screen, (50, 60, 80), (20, 80), (460, 80), 2)
        screen.blit(cmd_info, (20, 100))
        screen.blit(dribble_info, (20, 130))
        screen.blit(kick_info, (20, 160))
        pygame.draw.line(screen, (50, 60, 80), (20, 200), (460, 200), 1)
        screen.blit(help_info1, (20, 220))
        screen.blit(help_info2, (20, 250))

        pygame.display.flip()
        clock.tick(RATE_HZ)

    # Cleanup: Send zero packet before exiting
    zero_packet = json.dumps({"vx": 0.0, "vy": 0.0, "w": 0.0, "k": 0, "d": 0}).encode("utf-8")
    sock.sendto(zero_packet, target_addr)
    sock.close()
    pygame.quit()
    print("[MARUK TELEOP] Teleop shutdown safely.")

if __name__ == "__main__":
    main()
