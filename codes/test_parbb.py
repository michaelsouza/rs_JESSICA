import unittest
import numpy as np
from parbb import BBTask  # Assuming the class is in a file named bb_job.py


class TestBBJob(unittest.TestCase):
    def setUp(self):
        self.root_level = 0
        self.values = np.array([0, 0, 0])
        self.max_values = np.array([2, 3, 2])
        self.job = BBTask(self.root_level, self.values, self.max_values)

    def test_initialization(self):
        self.assertEqual(self.job.root_level, 0)
        self.assertEqual(self.job.level, self.job.root_level)
        np.testing.assert_array_equal(self.job.values, np.array([0, 0, 0]))
        np.testing.assert_array_equal(self.job.max_values, np.array([2, 3, 2]))
        self.assertEqual(self.job.length, 3)

    def test_backtracking(self):
        # Test normal backtracking
        self.job.values = np.array([1, 1, 1])
        self.job.level = 2
        self.job.backtracking()
        self.assertEqual(self.job.level, 2)
        np.testing.assert_array_equal(self.job.values, np.array([1, 1, 2]))

        # Test backtracking with reset
        self.job.values = np.array([1, 2, 2])
        self.job.level = 2
        self.job.backtracking()
        self.assertEqual(self.job.level, 1)
        np.testing.assert_array_equal(self.job.values, np.array([1, 3, 0]))

        # Test backtracking with no solution
        self.job.values = np.array([2, 3, 2])
        self.job.level = 2
        self.job.backtracking()
        self.assertEqual(self.job.level, -1)
        np.testing.assert_array_equal(self.job.values, np.array([0, 0, 0]))

    def test_next_node(self):
        # Test moving to next level
        self.job.level = 0
        self.assertTrue(self.job.next_node(True))
        self.assertEqual(self.job.level, 1)
        np.testing.assert_array_equal(self.job.values, np.array([0, 0, 0]))

        # Test backtracking
        self.job.level = 2
        self.job.values = np.array([1, 2, 2])
        self.assertTrue(self.job.next_node(False))
        self.assertEqual(self.job.level, 1)
        np.testing.assert_array_equal(self.job.values, np.array([1, 3, 0]))

        # Test no solution
        self.job.level = 2
        self.job.values = np.array([2, 3, 2])
        self.assertFalse(self.job.next_node(False))

    def test_split_job(self):
        # Test normal split
        self.job.values = np.array([1, 1, 1])
        new_job = self.job.split_job()
        self.assertIsNotNone(new_job)
        self.assertEqual(new_job.root_level, 0)
        np.testing.assert_array_equal(new_job.values, np.array([2, 1, 1]))
        np.testing.assert_array_equal(new_job.max_values, np.array([2, 3, 2]))
        np.testing.assert_array_equal(self.job.max_values, np.array([1, 3, 2]))

        # Test split with no available split
        self.job.values = np.array([2, 3, 2])
        new_job = self.job.split_job()
        self.assertIsNone(new_job)


if __name__ == "__main__":
    unittest.main()
