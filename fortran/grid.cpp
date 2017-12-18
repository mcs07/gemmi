
#include <cstring>
#include "gemmi/grid.hpp"

extern "C" {
#include "grid.h"
}

using gemmi::Grid;

using GridC = Grid<signed char>;
static cGridC* as_c(GridC* sg) { return reinterpret_cast<cGridC*>(sg); }
static GridC* as_cpp(cGridC* sg) { return reinterpret_cast<GridC*>(sg); }

extern "C" {

cGridC* GridC_init(int nx, int ny, int nz) {
  GridC* grid = new GridC;
  grid->set_size(nx, ny, nz);
  return as_c(grid);
}

void GridC_set_unit_cell(cGridC* grid, double a, double b, double c,
                         double alpha, double beta, double gamma) {
  as_cpp(grid)->set_unit_cell(a, b, c, alpha, beta, gamma);
}

void GridC_mask_atom(cGridC* grid, double x, double y, double z,
                     double radius) {
  as_cpp(grid)->mask_atom(x, y, z, radius);
}

void GridC_apply_space_group(cGridC* grid, int ccp4_num) {
  GridC* g = as_cpp(grid);
  g->space_group = gemmi::find_spacegroup_by_number(ccp4_num);
  g->symmetrize([](signed char a, signed char b) { return a > b ? a : b; });
}

signed char* GridC_data(cGridC* grid) {
  return as_cpp(grid)->data.data();
}

void GridC_free(cGridC* grid) {
  delete as_cpp(grid);
}

}
// vim:sw=2:ts=2:et
