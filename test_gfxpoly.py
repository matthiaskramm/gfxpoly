import unittest
import sys
import os

# Add the python directory to the sys.path to find gfxpoly
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), 'python')))

import gfxpoly

class TestGfxPoly(unittest.TestCase):

    def test_basic_functionality(self):
        p = gfxpoly.GfxPoly(gridsize=0.1)
        self.assertEqual(p.gridsize, 0.1)
        self.assertEqual(p.area(), 0.0)
        self.assertEqual(p.union(p).area(), 0.0)
        self.assertEqual(p.intersect(p).area(), 0.0)
        self.assertEqual(p.bbox(), [0., 0., 0., 0.])
        self.assertEqual(p.decompose(), [])

    def test_canvas_and_operations(self):
        c = gfxpoly.GfxCanvas(gridsize=0.1)
        c.moveTo(100, 150)
        c.lineTo(200, 150)
        c.lineTo(200, 250)
        c.lineTo(100, 250)
        c.lineTo(100, 150)
        c.close()
        p = c.result()

        # Test intersection
        p2 = p.move(50, 51)
        i = p.intersect(p2)
        self.assertEqual(i.bbox(), [150.0, 201.0, 200.0, 250.0])
        self.assertEqual(i.decompose(), [('m', 150.0, 201.0), ('l', 150.0, 250.0), ('l', 200.0, 250.0), ('l', 200.0, 201.0), ('l', 150.0, 201.0)])

        # Test union
        i = p.union(p2)
        self.assertEqual(i.bbox(), [100.0, 150.0, 250.0, 301.0])
        self.assertEqual(i.decompose(), [('m', 100.0, 150.0), ('l', 100.0, 250.0), ('l', 150.0, 250.0), ('l', 150.0, 301.0), ('l', 250.0, 301.0), ('l', 250.0, 201.0), ('l', 200.0, 201.0), ('l', 200.0, 150.0), ('l', 100.0, 150.0)])

        # Test subtract
        i = p.subtract(p2)
        self.assertEqual(i.bbox(), [100.0, 150.0, 200.0, 250.0])
        self.assertEqual(i.decompose(), [('m', 100.0, 150.0), ('l', 100.0, 250.0), ('l', 150.0, 250.0), ('l', 150.0, 201.0), ('l', 200.0, 201.0), ('l', 200.0, 150.0), ('l', 100.0, 150.0)])

if __name__ == '__main__':
    unittest.main()
