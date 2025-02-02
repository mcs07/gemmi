// Copyright 2017 Global Phasing Ltd.
//
// Read any supported coordinate file.

#ifndef GEMMI_MMREAD_HPP_
#define GEMMI_MMREAD_HPP_

#include "chemcomp_xyz.hpp" // for make_structure_from_chemcomp_block
#include "cif.hpp"       // for cif::read
#include "fail.hpp"      // for fail
#include "input.hpp"     // for BasicInput
#include "json.hpp"      // for read_mmjson
#include "mmcif.hpp"     // for make_structure_from_block
#include "model.hpp"     // for Structure
#include "pdb.hpp"       // for read_pdb
#include "util.hpp"      // for iends_with

namespace gemmi {

inline CoorFormat coor_format_from_ext(const std::string& path) {
  if (iends_with(path, ".pdb") || iends_with(path, ".ent"))
    return CoorFormat::Pdb;
  if (iends_with(path, ".cif"))
    return CoorFormat::Mmcif;
  if (iends_with(path, ".json"))
    return CoorFormat::Mmjson;
  return CoorFormat::Unknown;
}

template<typename T>
Structure read_structure(T&& input, CoorFormat format=CoorFormat::Unknown) {
  bool any = (format == CoorFormat::UnknownAny);
  if (format == CoorFormat::Unknown || any)
    format = coor_format_from_ext(input.basepath());
  switch (format) {
    case CoorFormat::Pdb:
      return read_pdb(input);
    case CoorFormat::Mmcif:
      if (any) {
        cif::Document doc = cif::read(input);
        int n = check_chemcomp_block_number(doc);
        if (n != -1)
          return make_structure_from_chemcomp_block(doc.blocks[n]);
        // mmCIF files for deposition may have more than one block:
        // coordinates in the first block and restraints in the others.
        for (size_t i = 1; i < doc.blocks.size(); ++i)
          if (doc.blocks[i].has_tag("_atom_site.id"))
            fail("Expected a single block with coordinates: " + input.path());
        return make_structure_from_block(doc.blocks.at(0));
      }
      return make_structure_from_block(cif::read(input).sole_block());
    case CoorFormat::Mmjson:
      return make_structure_from_block(cif::read_mmjson(input).sole_block());
    case CoorFormat::ChemComp:
      return make_structure_from_chemcomp_doc(cif::read(input));
    case CoorFormat::Unknown:
    case CoorFormat::UnknownAny:
      fail("Unknown format of " +
           (input.path().empty() ? "coordinate file" : input.path()) + ".");
  }
  unreachable();
}

inline Structure read_structure_file(const std::string& path,
                                     CoorFormat format=CoorFormat::Unknown) {
  return read_structure(BasicInput(path), format);
}

} // namespace gemmi
#endif
