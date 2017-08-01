#!/usr/bin/env python

import os
import json
import unittest

import gemmi

class TestCifAsJson(unittest.TestCase):
  def test_misc(self):
    basename = os.path.join(os.path.dirname(__file__), "misc")
    cif_doc = gemmi.cif.read_file(basename + ".cif")
    json_str = cif_doc.as_json()
    json_from_cif = json.loads(json_str)
    with open(basename + ".json") as f:
      reference_json = json.load(f)
    self.assertEqual(json_from_cif, reference_json)

if __name__ == '__main__':
  unittest.main()

# vim:sw=2:ts=2:et
