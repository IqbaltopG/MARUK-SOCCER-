"""
Unit Tests for MARUK Soccer Kinematics Module
Covers 2WD Differential Drive (MIT-WEEK 2026) and 4-Wheel Mecanum.
"""

import unittest
from kinematics import compute_differential_speeds, compute_mecanum_speeds, apply_deadzone

class TestSoccerKinematics(unittest.TestCase):
    def test_deadzone(self):
        self.assertEqual(apply_deadzone(0.05, 0.08), 0.0)
        self.assertEqual(apply_deadzone(-0.05, 0.08), 0.0)
        self.assertAlmostEqual(apply_deadzone(1.0, 0.08), 1.0)
        self.assertAlmostEqual(apply_deadzone(-1.0, 0.08), -1.0)

    # ==========================================
    # 2WD DIFFERENTIAL DRIVE TESTS (MIT-WEEK 2026)
    # ==========================================
    def test_diff_pure_forward(self):
        speeds = compute_differential_speeds(vx=1.0, omega=0.0)
        self.assertAlmostEqual(speeds["left"], 1.0)
        self.assertAlmostEqual(speeds["right"], 1.0)

    def test_diff_pure_backward(self):
        speeds = compute_differential_speeds(vx=-1.0, omega=0.0)
        self.assertAlmostEqual(speeds["left"], -1.0)
        self.assertAlmostEqual(speeds["right"], -1.0)

    def test_diff_spin_clockwise(self):
        # Rotate CW: left forward, right backward
        speeds = compute_differential_speeds(vx=0.0, omega=1.0)
        self.assertAlmostEqual(speeds["left"], -1.0)
        self.assertAlmostEqual(speeds["right"], 1.0)

    def test_diff_spin_counter_clockwise(self):
        # Rotate CCW: left backward, right forward
        speeds = compute_differential_speeds(vx=0.0, omega=-1.0)
        self.assertAlmostEqual(speeds["left"], 1.0)
        self.assertAlmostEqual(speeds["right"], -1.0)

    def test_diff_normalization_over_limit(self):
        # Full forward (1.0) + full turn right (1.0)
        speeds = compute_differential_speeds(vx=1.0, omega=1.0)
        self.assertLessEqual(abs(speeds["left"]), 1.0001)
        self.assertLessEqual(abs(speeds["right"]), 1.0001)

    # ==========================================
    # 4-WHEEL MECANUM DRIVE TESTS
    # ==========================================
    def test_mecanum_pure_forward(self):
        speeds = compute_mecanum_speeds(vx=1.0, vy=0.0, omega=0.0)
        self.assertAlmostEqual(speeds["fl"], 1.0)
        self.assertAlmostEqual(speeds["fr"], 1.0)
        self.assertAlmostEqual(speeds["rl"], 1.0)
        self.assertAlmostEqual(speeds["rr"], 1.0)

    def test_mecanum_pure_strafe_right(self):
        speeds = compute_mecanum_speeds(vx=0.0, vy=1.0, omega=0.0)
        self.assertAlmostEqual(speeds["fl"], 1.0)
        self.assertAlmostEqual(speeds["fr"], -1.0)
        self.assertAlmostEqual(speeds["rl"], -1.0)
        self.assertAlmostEqual(speeds["rr"], 1.0)

if __name__ == "__main__":
    unittest.main()
