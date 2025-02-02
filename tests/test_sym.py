#!/usr/bin/env python

import math
import unittest
import random
import gemmi
try:
    from cctbx import sgtbx
    print('(w/ sgtbx)')
except ImportError:
    sgtbx = None

D = gemmi.Op.DEN

CANONICAL_SINGLES = {
    "x"     : [ D, 0, 0, 0],
    "z"     : [ 0, 0, D, 0],
    "-y"    : [ 0,-D, 0, 0],
    "-z"    : [ 0, 0,-D, 0],
    "x-y"   : [ D,-D, 0, 0],
    "-x+y"  : [-D, D, 0, 0],
    "x+1/2" : [ D, 0, 0, D//2],
    "y+1/4" : [ 0, D, 0, D//4],
    "z+3/4" : [ 0, 0, D, D*3//4],
    "z+1/3" : [ 0, 0, D, D*1//3],
    "z+1/6" : [ 0, 0, D, D//6],
    "z+2/3" : [ 0, 0, D, D*2//3],
    "z+5/6" : [ 0, 0, D, D*5//6],
    "-x+1/4": [-D, 0, 0, D//4],
    "-y+1/2": [ 0,-D, 0, D//2],
    "-y+3/4": [ 0,-D, 0, D*3//4],
    "-z+1/3": [ 0, 0,-D, D//3],
    "-z+1/6": [ 0, 0,-D, D//6],
    "-z+2/3": [ 0, 0,-D, D*2//3],
    "-z+5/6": [ 0, 0,-D, D*5//6],
}

OTHER_SINGLES = {
    # order and letter case may vary
    "Y-x"   : [-D, D, 0,  0],
    "-X"    : [-D, 0, 0,  0],
    "-1/2+Y": [ 0, D, 0, -D//2],
    # we want to handle non-crystallographic translations
    "x+3"   : [ D, 0, 0,  D*3],
    "1+Y"   : [ 0, D, 0,  D],
    "-2+Y"  : [ 0, D, 0, -D*2],
    "-z-5/6": [ 0, 0,-D, -D*5//6],
}


class TestSymmetry(unittest.TestCase):
    def test_parse_triplet_part(self):
        for single, row in CANONICAL_SINGLES.items():
            calculated = gemmi.parse_triplet_part(single)
            self.assertEqual(calculated, row)
        for single, row in OTHER_SINGLES.items():
            calculated = gemmi.parse_triplet_part(single)
            self.assertEqual(calculated, row)

    def test_make_triplet_part(self):
        self.assertEqual(gemmi.make_triplet_part(0, 0, 0, 1),
                         '1/%d' % gemmi.Op.DEN)
        for single, row in CANONICAL_SINGLES.items():
            calculated = gemmi.make_triplet_part(*row)
            self.assertEqual(calculated, single)

    def test_triplet_roundtrip(self):
        singles = list(CANONICAL_SINGLES.keys())
        for i in range(4):
            items = [random.choice(singles) for j in range(3)]
            triplet = ','.join(items)
            op = gemmi.parse_triplet(triplet)
            self.assertEqual(op.triplet(), triplet)
        self.assertEqual(gemmi.Op(' x , - y, + z ').triplet(), 'x,-y,z')

    def test_combine(self):
        a = gemmi.Op('x+1/3,z,-y')
        self.assertEqual(a.combine(a).triplet(), 'x+2/3,-y,-z')
        self.assertEqual('x,-y,z' * gemmi.Op('-x,-y,z'), '-x,y,z')
        a = gemmi.Op('-y+1/4,x+3/4,z+1/4')
        b = gemmi.Op('-x+1/2,y,-z')
        self.assertEqual((a * b).triplet(), '-y+1/4,-x+1/4,-z+1/4')
        c = '-y,-z,-x'
        self.assertNotEqual(b * c, c * b)
        self.assertEqual((a * c).triplet(), 'z+1/4,-y+3/4,-x+1/4')
        self.assertEqual(b * c, gemmi.Op('y+1/2,-z,x'))
        self.assertEqual(c * b, '-y,z,x+1/2')

    def test_invert(self):
        for xyz in ['-y,-x,-z+1/4', 'y,-x,z+3/4', 'y,x,-z', 'y+1/2,x,-z+1/3']:
            op = gemmi.Op(xyz)
            self.assertEqual(op * op.inverse(), 'x,y,z')
            self.assertEqual(op.inverse().inverse(), op)
        # test change-of-basis op between hexagonal and trigonal settings
        op = gemmi.Op("-y+z,x+z,-x+y+z")  # det=3
        self.assertEqual(op.det_rot(), 3 * gemmi.Op.DEN**3)
        inv = op.inverse()
        self.assertEqual(inv * op, 'x,y,z')
        self.assertEqual(op * inv, 'x,y,z')
        expected_inv = '-1/3*x+2/3*y-1/3*z,-2/3*x+1/3*y+1/3*z,1/3*x+1/3*y+1/3*z'
        self.assertEqual(inv.triplet(), expected_inv)
        self.assertEqual(gemmi.Op(expected_inv), inv)
        op = gemmi.Op('1/2*x+1/2*y,-1/2*x+1/2*y,z')
        self.assertEqual(op.inverse().triplet(), 'x-y,x+y,z')

    def test_generators_from_hall(self):
        # first test on example matrices from
        # http://cci.lbl.gov/sginfo/hall_symbols.html
        self.assertEqual(gemmi.generators_from_hall('p -2xc').sym_ops,
                         ['x,y,z', '-x,y,z+1/2'])
        self.assertEqual(gemmi.generators_from_hall('p 3*').sym_ops,
                         ['x,y,z', 'z,x,y'])
        self.assertEqual(gemmi.generators_from_hall('p 4vw').sym_ops,
                         ['x,y,z', '-y,x+1/4,z+1/4'])
        self.assertEqual(gemmi.generators_from_hall('p 61 2 (0 0 -1)').sym_ops,
                         ['x,y,z', 'x-y,x,z+1/6', '-y,-x,-z+5/6'])
        # then on examples from the 530 settings
        self.assertEqual(gemmi.generators_from_hall('P -2 -2').sym_ops,
                         ['x,y,z', 'x,y,-z', '-x,y,z'])
        # the same operations in different notation
        a = gemmi.generators_from_hall('P 3*')
        b = gemmi.generators_from_hall('R 3 (-y+z,x+z,-x+y+z)')
        self.assertEqual(a.sym_ops, b.sym_ops)
        self.assertEqual(a.cen_ops, b.cen_ops)

    def compare_hall_symops_with_sgtbx(self, hall, existing_group=True):
        cctbx_sg = sgtbx.space_group(hall)
        cctbx_triplets = set(m.as_xyz() for m in cctbx_sg.all_ops(mod=1))
        gemmi_gops = gemmi.symops_from_hall(hall)
        self.assertEqual(len(gemmi_gops.sym_ops), cctbx_sg.order_p())
        self.assertEqual(len(gemmi_gops.cen_ops), cctbx_sg.n_ltr())
        self.assertEqual(len(gemmi_gops), cctbx_sg.order_z())
        self.assertEqual(gemmi_gops.is_centric(), cctbx_sg.is_centric())
        gemmi_triplets = set(m.triplet() for m in gemmi_gops)
        self.assertEqual(cctbx_triplets, gemmi_triplets)
        gemmi_sg = gemmi.find_spacegroup_by_ops(gemmi_gops)
        if existing_group:
            self.assertEqual(gemmi_sg.point_group_hm(),
                             cctbx_sg.point_group_type())
            self.assertEqual(gemmi_sg.crystal_system_str(),
                             cctbx_sg.crystal_system().lower())
        else:
            self.assertIsNone(gemmi_sg)

    def test_with_sgtbx(self):
        if sgtbx is None:
            return
        for s in sgtbx.space_group_symbol_iterator():
            self.compare_hall_symops_with_sgtbx(s.hall())
        self.compare_hall_symops_with_sgtbx('C -4 -2b', existing_group=False)

    def test_table(self):
        for sg in gemmi.spacegroup_table():
            if sg.ccp4 != 0:
                self.assertEqual(sg.ccp4 % 1000, sg.number)
            if sg.operations().is_centric():
                self.assertEqual(sg.laue_str(), sg.point_group_hm())
            else:
                self.assertNotEqual(sg.laue_str(), sg.point_group_hm())
            if sgtbx:
                hall = sg.hall.encode()
                cctbx_sg = sgtbx.space_group(hall)
                cctbx_info = sgtbx.space_group_info(group=cctbx_sg)
                self.assertEqual(sg.is_reference_setting(),
                                 cctbx_info.is_reference_setting())
                #to_ref = cctbx_info.change_of_basis_op_to_reference_setting()
                #from_ref = '%s' % cob_to_ref.inverse().c()

    def test_find_spacegroup(self):
        self.assertEqual(gemmi.SpaceGroup('P21212').hm, 'P 21 21 2')
        self.assertEqual(gemmi.find_spacegroup_by_name('P21').hm, 'P 1 21 1')
        self.assertEqual(gemmi.find_spacegroup_by_name('P 2').hm, 'P 1 2 1')
        def check_xhm(name, xhm):
            self.assertEqual(gemmi.SpaceGroup(name).xhm(), xhm)
        check_xhm('R 3 2', 'R 3 2:H')
        check_xhm('R 3 2', 'R 3 2:H')
        check_xhm('R32:H', 'R 3 2:H')
        check_xhm('H32', 'R 3 2:H')
        check_xhm('R 3 2:R', 'R 3 2:R')
        check_xhm('P6', 'P 6')
        check_xhm('P 6', 'P 6')
        check_xhm('P65', 'P 65')
        check_xhm('I1211', 'I 1 21 1')
        check_xhm('Aem2', 'A b m 2')
        check_xhm('C c c e', 'C c c a:1')
        check_xhm('i2', 'I 1 2 1')
        self.assertRaises(ValueError, gemmi.SpaceGroup, 'i3')
        self.assertEqual(gemmi.find_spacegroup_by_number(5).hm, 'C 1 2 1')
        self.assertEqual(gemmi.SpaceGroup(4005).hm, 'I 1 2 1')
        self.assertIsNone(gemmi.find_spacegroup_by_name('abc'))

    def test_groupops(self):
        gops = gemmi.GroupOps([gemmi.Op(t) for t in ['x, y, z',
                                                     'x, -y, z+1/2',
                                                     'x+1/2, y+1/2, z',
                                                     'x+1/2, -y+1/2, z+1/2']])
        self.assertEqual(gops.find_centering(), 'C')
        self.assertEqual(len(gops), 4)
        self.assertEqual(gemmi.find_spacegroup_by_ops(gops).hm, 'C 1 c 1')

    def change_basis(self, name_a, name_b, basisop_triplet):
        basisop = gemmi.Op(basisop_triplet)
        a = gemmi.find_spacegroup_by_name(name_a)
        b = gemmi.find_spacegroup_by_name(name_b)
        ops = a.operations()
        ops.change_basis(basisop)
        self.assertEqual(ops, b.operations())
        ops.change_basis(basisop.inverse())
        self.assertEqual(ops, a.operations())

    def test_change_basis(self):
        self.change_basis('I2', 'C2', 'x,y,x+z')
        self.change_basis('C 1 c 1', 'C 1 n 1', 'x+1/4,y+1/4,z')
        self.change_basis('R 3 :H', 'R 3 :R', '-y+z,x+z,-x+y+z')
        self.change_basis('A -1', 'P -1', '-x,-y+z,y+z')

    def test_short_name(self):
        for (longer, shorter) in [('P 21 2 21', 'P21221'),
                                  ('P 1 2 1',   'P2'),
                                  ('P 1',       'P1'),
                                  ('R 3 2:R',   'R32'),
                                  ('R 3 2:H',   'H32')]:
            self.assertEqual(gemmi.SpaceGroup(longer).short_name(), shorter)

    def compare_short_names_with_symop_lib():
        for line in open('symop.lib'):
            if line and not line[0].isspace():
                fields = line.partition('!')[0].split(None, 6)
                #spacegroups = shlex.split(fields[-1])
                g = gemmi.find_spacegroup_by_number(int(fields[0]))
                if fields[3] != g.short_name():
                    print('[%s] %s %s' % (g.xhm(), g.short_name(), fields[3]))

    def test_operations(self):
        gops = gemmi.symops_from_hall('-P 2a 2ac (z,x,y)')
        self.assertEqual(set(gemmi.SpaceGroup('Pbaa').operations()), set(gops))
        self.assertEqual(gemmi.find_spacegroup_by_ops(gops).hm, 'P b a a')

    def test_find_grid_factors(self):
        def fact(name):
            return gemmi.SpaceGroup(name).operations().find_grid_factors()
        self.assertEqual(fact('P21'), [1, 2, 1])
        self.assertEqual(fact('P61'), [1, 1, 6])

    # based on example from pages 9-10 in
    # http://oldwww.iucr.org/iucr-top/comm/cteach/pamphlets/9/9.pdf
    def test_phase_shift(self):
        ops = gemmi.find_spacegroup_by_name('P 31 2 1').operations()
        refl = [3, 0, 1]
        expected_equiv = [
            # in the paper the last two reflections are swapped
            [3, 0, 1], [0, -3, 1], [-3, 3, 1],
            [0, 3, -1], [3, -3, -1], [-3, 0, -1]]
        self.assertEqual([op.apply_to_hkl(refl) for op in ops], expected_equiv)
        expected_shifts = [0, -120, -240, 0, -240, -120]
        for op, expected in zip(ops, expected_shifts):
            shift = math.degrees(op.phase_shift(*refl))
            self.assertAlmostEqual((shift - expected) % 360, 0)

if __name__ == '__main__':
    unittest.main()
