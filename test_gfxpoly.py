import gfxpoly

# basic functionality
p = gfxpoly.GfxPoly(gridsize=0.1)
assert p.gridsize == 0.1
assert p.area() == 0.0
assert p.union(p).area() == 0.0
assert p.intersect(p).area() == 0.0
assert p.bbox() == [0., 0., 0., 0.]
assert p.decompose() == []

# canvas
c = gfxpoly.GfxCanvas(gridsize=0.1)
c.moveTo(100, 150)
c.lineTo(200, 150)
c.lineTo(200, 250)
c.lineTo(100, 250)
c.lineTo(100, 150)
c.close()
p = c.result()

# intersection
p2 = p.move(50, 51)
i = p.intersect(p2)
assert i.bbox() == [150.0, 201.0, 200.0, 250.0]
assert i.decompose() == [('m', 150.0, 201.0), ('l', 150.0, 250.0), ('l', 200.0, 250.0), ('l', 200.0, 201.0), ('l', 150.0, 201.0)]

# union
i = p.union(p2)
assert i.bbox() == [100.0, 150.0, 250.0, 301.0]
assert i.decompose() == [('m', 100.0, 150.0), ('l', 100.0, 250.0), ('l', 150.0, 250.0), ('l', 150.0, 301.0), ('l', 250.0, 301.0), ('l', 250.0, 201.0), ('l', 200.0, 201.0), ('l', 200.0, 150.0), ('l', 100.0, 150.0)]

# subtract
i = p.subtract(p2)
assert i.bbox() == [100.0, 150.0, 200.0, 250.0]
assert i.decompose() == [('m', 100.0, 150.0), ('l', 100.0, 250.0), ('l', 150.0, 250.0), ('l', 150.0, 201.0), ('l', 200.0, 201.0), ('l', 200.0, 150.0), ('l', 100.0, 150.0)]
