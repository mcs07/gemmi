// This program re-calculates _struct_conn.pdbx_dist_value values
// and prints message if it differs by more than 0.002A from the value in file.

#include <cstdio>
#include <gemmi/gz.hpp>   // for MaybeGzipped
#include <gemmi/cif.hpp>
#include <gemmi/numb.hpp> // for as_number
#include <gemmi/mmcif.hpp>
#include <gemmi/dirwalk.hpp> // for CifWalk

using namespace gemmi;

int verbose = false;

static void check_struct_conn(cif::Block& block) {
  cif::Table struct_conn = block.find("_struct_conn.", {"id", "conn_type_id",
                                                        "ptnr2_symmetry",
                                                        "pdbx_dist_value" });
  Structure st = make_structure_from_block(block);
  for (Connection& con : st.models[0].connections) {
    const Atom* atom[2] = {nullptr, nullptr};
    for (int n : {0, 1}) {
      atom[n] = st.models[0].find_atom(con.atom[n]);
      if (!atom[n])
        std::printf("%s: %s atom not found in res. %s\n", block.name.c_str(),
                    con.name.c_str(), con.atom[n].str().c_str());
    }
    if (!atom[0] || !atom[1])
      continue;
    SymImage im = st.cell.find_nearest_image(atom[0]->pos,
                                             atom[1]->pos, con.asu);
    double dist = std::sqrt(im.dist_sq);
    cif::Table::Row row = struct_conn.find_row(con.name);
    if (!starts_with(con.name, row.str(1)))
      std::printf("%s: Unexpected connection name: %s for %s\n",
                  block.name.c_str(), con.name.c_str(), row.str(1).c_str());
    if (dist > 5)
      std::printf("%s: Long connection %s: %g\n",
                  block.name.c_str(), con.name.c_str(), dist);
    std::string ref_sym = row.str(2);
    double ref_dist = cif::as_number(row[3]);
    bool differs = std::abs(dist - ref_dist) > 0.002;
    if (verbose || differs) {
      std::printf("%s %-9s %-14s %-14s im:%s  %.3f %c= %.3f (%s)%s\n",
                  block.name.c_str(), con.name.c_str(),
                  con.atom[0].str().c_str(), con.atom[1].str().c_str(),
                  im.pdb_symbol(true).c_str(), dist, (differs ? '!' : '='),
                  ref_dist, ref_sym.c_str(),
                  st.cell.explicit_matrices ? "  {fract}" : "");
    }
  }
  for (cif::Table::Row row : struct_conn)
    if (st.models[0].find_connection_by_name(row.str(0)) == nullptr)
      std::printf("%s: connection not read: %s\n", block.name.c_str(),
                  row.str(0).c_str());
}

int main(int argc, char* argv[]) {
  int pos = 1;
  if (argc >= 3 && argv[1] == std::string("-v")) {
    ++pos;
    verbose = true;
  } else if (argc < 2) {
    return 1;
  }
  int counter = 0;
  try {
    for (; pos != argc; ++pos) {
      for (const char* path : CifWalk(expand_if_pdb_code(argv[pos]))) {
        cif::Document doc = cif::read(MaybeGzipped(path));
        check_struct_conn(doc.sole_block());
        if (++counter % 1000 == 0) {
          std::printf("[progress: %d files]\n", counter);
          std::fflush(stdout);
        }
      }
    }
  } catch (std::runtime_error& err) {
    std::fprintf(stderr, "Error: %s\n", err.what());
    return 1;
  }
  return 0;
}
