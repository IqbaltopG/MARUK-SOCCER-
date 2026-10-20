"""
Unit Tests for MARUK Soccer Kinematics Module
"""

import unittest
from kinematics import compute_mecanum_speeds, apply_deadzone

class TestMecanumKinematics(unittest.TestCase):
    def test_deadzone(self):
        self.assertEqual(apply_deadzone(0.05, 0.08), 0.0)
        self.assertEqual(apply_deadzone(-0.05, 0.08), 0.0)
        self.assertAlmostEqual(apply_deadzone(1.0, 0.08), 1.0)
        self.assertAlmostEqual(apply_deadzone(-1.0, 0.08), -1.0)

    def test_pure_forward(self):
        speeds = compute_mecanum_speeds(vx=1.0, vy=0.0, omega=0.0)
        self.assertAlmostEqual(speeds["fl"], 1.0)
        self.assertAlmostEqual(speeds["fr"], 1.0)
        self.assertAlmostEqual(speeds["rl"], 1.0)
        self.assertAlmostEqual(speeds["rr"], 1.0)

    def test_pure_backward(self):
        speeds = compute_mecanum_speeds(vx=-1.0, vy=0.0, omega=0.0)
        self.assertAlmostEqual(speeds["fl"], -1.0)
        self.assertAlmostEqual(speeds["fr"], -1.0)
        self.assertAlmostEqual(speeds["rl"], -1.0)
        self.assertAlmostEqual(speeds["rr"], -1.0)

    def test_pure_strafe_right(self):
        speeds = compute_mecanum_speeds(vx=0.0, vy=1.0, omega=0.0)
        self.assertAlmostEqual(speeds["fl"], 1.0)
        self.assertAlmostEqual(speeds["fr"], -1.0)
        self.assertAlmostEqual(speeds["rl"], -1.0)
        self.assertAlmostEqual(speeds["rr"], 1.0)

    def test_pure_strafe_left(self):
        speeds = compute_mecanum_speeds(vx=0.0, vy=-1.0, omega=0.0)
        self.assertAlmostEqual(speeds["fl"], -1.0)
        self.assertAlmostEqual(speeds["fr"], 1.0)
        self.assertAlmostEqual(speeds["rl"], 1.0)
        self.assertAlmostEqual(speeds["rr"], -1.0)

    def test_pure_rotate_clockwise(self):
        speeds = compute_mecanum_speeds(vx=0.0, vy=0.0, omega=1.0)
        self.assertAlmostEqual(speeds["fl"], 1.0)
        self.assertAlmostEqual(speeds["fr"], -1.0)
        self.assertAlmostEqual(speeds["rl"], 1.0)
        self.assertAlmostEqual(speeds["rr"], -1.0)

    def test_normalization_over_limit(self):
        # Combined full forward and full strafe right (sum would be 2.0 without norm)
        speeds = compute_mecanum_speeds(vx=1.0, vy=1.0, omega=0.0)
        for key in ["fl", "fr", "rl", "rr"]:
            self.assertLessEqual(abs(speeds[key]), 1.0001)

if __name__ == "__main__":
    unittest.main()
