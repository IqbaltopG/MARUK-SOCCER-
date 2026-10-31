#!/usr/bin/env python3
"""
MARUK (SOCCER) - Unified Teleoperation Station (2WD Differential Drive)
Conforms to MIT-WEEK 2026 Car Soccer Rules.

Controls:
  Keyboard (Pygame Window):
    W / S (or UP / DOWN)       : Forward / Backward (Vx)
    A / D (or LEFT / RIGHT)    : Turn Left / Turn Right (Omega)
    LSHIFT                     : Turbo / Sprint Mode (Max speed)
    ESC                        : Quit

  Gamepad / Drone RC USB:
    Left Stick Y / Pitch       : Forward / Backward (Vx)
    Right Stick X / Roll / Yaw : Turn Left / Turn Right (Omega)

Broadcasts JSON packets over UDP:
  {"vx": float, "w": float}
"""

import sys
import time
import json
import socket
import argparse
import os

os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = "hide"
import pygame

DEFAULT_TARGET_IP = "127.0.0.1"   # Default to Webots localhost
DEFAULT_TARGET_PORT = 5005
RATE_HZ = 30                      # 30 packets per second

def main():
    parser = argparse.ArgumentParser(description="MARUK 2WD Soccer Teleop Station")
    parser.add_argument("--ip", default=DEFAULT_TARGET_IP, help="Target IP (Default: 127.0.0.1 for Webots, or ESP32 IP)")
    parser.add_argument("--port", type=int, default=DEFAULT_TARGET_PORT, help="Target UDP Port (Default: 5005)")
    parser.add_argument("--speed", type=float, default=1.0, help="Max speed multiplier (0.1 to 1.0)")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    target_addr = (args.ip, args.port)

    pygame.init()
    pygame.joystick.init()

    joystick = None
    if pygame.joystick.get_count() > 0:
        joystick = pygame.joystick.Joystick(0)
        joystick.init()
        print(f"[MARUK TELEOP] Gamepad / RC Transmitter detected: {joystick.get_name()}")
    else:
        print("[MARUK TELEOP] No USB joystick detected. Using Keyboard (WASD).")

    width, height = 480, 320
    screen = pygame.display.set_mode((width, height))
    pygame.display.set_caption("MARUK 2WD Soccer Teleop (MIT-WEEK 2026)")
    font = pygame.font.SysFont("monospace", 16)
    font_bold = pygame.font.SysFont("monospace", 18, bold=True)
    clock = pygame.time.Clock()

    print(f"[MARUK TELEOP] Broadcasting to {target_addr[0]}:{target_addr[1]} at {RATE_HZ}Hz")

    running = True
    while running:
        vx, omega = 0.0, 0.0
        turbo = False

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False

        keys = pygame.key.get_pressed()
        if keys[pygame.K_w] or keys[pygame.K_UP]:
            vx += 1.0
        if keys[pygame.K_s] or keys[pygame.K_DOWN]:
            vx -= 1.0
        if keys[pygame.K_d] or keys[pygame.K_RIGHT]:
            omega += 1.0   # Turn right
        if keys[pygame.K_a] or keys[pygame.K_LEFT]:
            omega -= 1.0   # Turn left
        if keys[pygame.K_LSHIFT] or keys[pygame.K_RSHIFT]:
            turbo = True

        # Read Gamepad / Drone RC USB interface
        if joystick is not None:
            joy_pitch = -joystick.get_axis(1)
            joy_yaw = joystick.get_axis(0) if joystick.get_numaxes() > 0 else 0.0
            if joystick.get_numaxes() > 2 and abs(joystick.get_axis(2)) > 0.1:
                joy_yaw = joystick.get_axis(2)

            if abs(joy_pitch) > 0.1: vx = joy_pitch
            if abs(joy_yaw) > 0.1: omega = joy_yaw
            if joystick.get_numbuttons() > 5 and joystick.get_button(5):
                turbo = True

        speed_scale = args.speed * 1.6 if turbo else args.speed
        vx = round(vx * speed_scale, 3)
        omega = round(omega * speed_scale, 3)

        packet = {
            "vx": vx,
            "w": omega
        }

        payload = json.dumps(packet).encode("utf-8")
        try:
            sock.sendto(payload, target_addr)
        except Exception as e:
            print(f"[ERROR] UDP send failed: {e}")

        # Render UI
        screen.fill((20, 24, 30))
        title = font_bold.render("MARUK 2WD SOCCER TELEOP", True, (0, 255, 180))
        sub = font.render("MIT-WEEK 2026 Edition", True, (160, 160, 170))
        target_info = font.render(f"Target: {target_addr[0]}:{target_addr[1]}", True, (180, 180, 180))
        cmd_info = font.render(f"Forward (Vx): {vx:+.2f}   Steer (W): {omega:+.2f}", True, (255, 255, 255))
        mode_col = (255, 200, 50) if turbo else (100, 200, 255)
        mode_info = font.render(f"Speed Mode:   {'TURBO SPRINT (160% GAS POL)' if turbo else 'NORMAL (100%)'}", True, mode_col)
        help_info1 = font.render("W/S (or UP/DOWN): Drive Forward/Back", True, (140, 140, 160))
        help_info2 = font.render("A/D (or L/R):     Turn Left/Right", True, (140, 140, 160))
        help_info3 = font.render("SHIFT:            Hold for Turbo", True, (140, 140, 160))

        screen.blit(title, (20, 20))
        screen.blit(sub, (20, 45))
        screen.blit(target_info, (20, 75))
        pygame.draw.line(screen, (50, 60, 80), (20, 105), (460, 105), 2)
        screen.blit(cmd_info, (20, 125))
        screen.blit(mode_info, (20, 155))
        pygame.draw.line(screen, (50, 60, 80), (20, 195), (460, 195), 1)
        screen.blit(help_info1, (20, 215))
        screen.blit(help_info2, (20, 240))
        screen.blit(help_info3, (20, 265))

        pygame.display.flip()
        clock.tick(RATE_HZ)

    zero_packet = json.dumps({"vx": 0.0, "w": 0.0}).encode("utf-8")
    sock.sendto(zero_packet, target_addr)
    sock.close()
    pygame.quit()

if __name__ == "__main__":
    main()
