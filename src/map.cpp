// Copyright 2017 Global Phasing Ltd.

#include "gemmi/ccp4.hpp"
#include "gemmi/gz.hpp"  // for MaybeGzipped
#include "gemmi/util.hpp"  // for trim_str
#include "gemmi/symmetry.hpp"
#include <cmath>     // for floor
#include <cstdio>    // for fprintf
#include <algorithm> // for nth_element, count_if
#define USE_UNICODE
#ifdef USE_UNICODE
#include <clocale>  // for setlocale
#include <cwchar>  // for wint_t
#endif
#define GEMMI_PROG map
#include "options.h"

enum OptionIndex { Verbose=3, Deltas, CheckSym, Reorder, Full };

static const option::Descriptor Usage[] = {
  { NoOp, 0, "", "", Arg::None,
    "Usage:\n " EXE_NAME " [options] CCP4_MAP[...]\n" },
  CommonUsage[Help],
  CommonUsage[Version],
  { Verbose, 0, "", "verbose", Arg::None, "  --verbose  \tVerbose output." },
  { Deltas, 0, "", "deltas", Arg::None,
    "  --deltas  \tStatistics of dx, dy and dz." },
  { CheckSym, 0, "", "check-symmetry", Arg::None,
    "  --check-symmetry  \tCompare the values of symmetric points." },
  { Reorder, 0, "", "write-xyz", Arg::Required,
    "  --write-xyz=FILE  \tWrite transposed map with fast X axis and slow Z." },
  { Full, 0, "", "write-full", Arg::Required,
    "  --write-full=FILE  \tWrite map extended to cover whole unit cell." },
  { 0, 0, 0, 0, 0, 0 }
};

template<typename T>
void print_histogram(const std::vector<T>& data, double min, double max) {
#ifdef USE_UNICODE
  std::setlocale(LC_ALL, "");
  constexpr int rows = 12;
#else
  constexpr int rows = 24;
#endif
  const int cols = 80; // TODO: use $COLUMNS
  std::vector<int> bins(cols+1, 0);
  double delta = max - min;
  for (T d : data) {
    int n = (int) std::floor((d - min) * (cols / delta));
    bins[n >= 0 ? (n < cols ? n : cols - 1) : 0]++;
  }
  double max_h = *std::max_element(std::begin(bins), std::end(bins));
  for (int i = rows; i > 0; --i) {
    for (int j = 0; j < cols; ++j) {
      double h = bins[j] / max_h * rows;
#ifdef USE_UNICODE
      wint_t c = ' ';
      if (h > i) {
        c = 0x2588; // 0x2581 = one eighth block, ..., 0x2588 = full block
      } else if (h > i - 1) {
        c = 0x2581 + static_cast<int>((h - (i - 1)) * 7);
      }
      printf("%lc", c);
#else
      std::putchar(h > i + 0.5 ? '#' : ' ');
#endif
    }
    std::putchar('\n');
  }
}

template<typename T>
gemmi::GridStats print_info(const gemmi::Ccp4<T>& map) {
  const gemmi::Grid<T>& grid = map.grid;
  std::printf("Map mode: %d\n", map.header_i32(4));
  std::printf("Endiannes: %snative\n", map.same_byte_order ? "" : "NOT ");
  std::printf("Number of columns, rows, sections: %5d %5d %5d %6s %d points\n",
              grid.nu, grid.nv, grid.nw, "->", grid.nu * grid.nv * grid.nw);
  int u0 = map.header_i32(5);
  int v0 = map.header_i32(6);
  int w0 = map.header_i32(7);
  std::printf("                             from: %5d %5d %5d\n", u0, v0, w0);
  std::printf("                               to: %5d %5d %5d\n",
              u0 + grid.nu - 1, v0 + grid.nv - 1, w0 + grid.nw - 1);
  std::printf("Fast, medium, slow axes: %c %c %c\n",
              'X' + map.header_i32(17) - 1,
              'X' + map.header_i32(18) - 1,
              'X' + map.header_i32(19) - 1);
  int mx = map.header_i32(8);
  int my = map.header_i32(9);
  int mz = map.header_i32(10);
  std::printf("Grid sampling on x, y, z: %5d %5d %5d    %12s %d points/cell\n",
              mx, my, mz, "->", mx * my * mz);
  const gemmi::UnitCell& cell = grid.unit_cell;
  const gemmi::SpaceGroup* sg = grid.spacegroup;
  std::printf("Space group: %d  (%s)\n",
              sg ? sg->ccp4 : 0, sg ? sg->hm : "unknown");
  int order = sg ? sg->operations().order() : 1;
  std::printf("SG order: %-3d      %40s %d points/ASU\n",
               order, "->", mx * my * mz / order);
  std::printf("Cell dimensions: %g %g %g  %g %g %g\n",
              cell.a, cell.b, cell.c, cell.alpha, cell.beta, cell.gamma);
  int origin[3] = {
    map.header_i32(50),
    map.header_i32(51),
    map.header_i32(52)
  };
  if (origin[0] != 0 || origin[1] != 0 || origin[2] != 0)
    std::printf("Non-zero origin: %d %d %d\n", origin[0], origin[1], origin[2]);

  std::printf("\nStatistics from HEADER and DATA\n");
  gemmi::GridStats st = gemmi::calculate_grid_statistics(grid.data);
  std::printf("Minimum: %12.5f  %12.5f\n", map.hstats.dmin, st.dmin);
  std::printf("Maximum: %12.5f  %12.5f\n", map.hstats.dmax, st.dmax);
  std::printf("Mean:    %12.5f  %12.5f\n", map.hstats.dmean, st.dmean);
  std::printf("RMS:     %12.5f  %12.5f\n", map.hstats.rms, st.rms);
  std::vector<T> data = grid.data;  // copy b/c nth_element() reorders data
  size_t mpos = data.size() / 2;
  std::nth_element(data.begin(), data.begin() + mpos, data.end());
  std::printf("Median:                %12.5f\n", data[mpos]);
  bool mask = std::all_of(data.begin(), data.end(),
                          [&st](T x) { return x == st.dmin || x == st.dmax; });
  double margin = mask ? 7 * (st.dmax - st.dmin) : 0;
  print_histogram(data, st.dmin - margin, st.dmax + margin);
  int nlabl = map.header_i32(56);
  if (nlabl != 0)
    std::printf("\n");
  for (int i = 0; i < nlabl && i < 10; ++i) {
    std::string label = gemmi::trim_str(map.header_str(57 + i * 20));
    std::printf("Label #%d\n%s\n", i, label.c_str());
  }
  int nsymbt = map.header_i32(24);
  if (nsymbt != 0)
    std::printf("\n");
  for (int i = 0; i * 80 < nsymbt; i++) {
    std::string symop = map.header_str(256 + i * 20 /*words not bytes*/, 80);
    std::printf("Sym op #%d: %s\n", i + 1, gemmi::trim_str(symop).c_str());
  }
  return st;
}

template<typename T>
void print_deltas(const gemmi::Grid<T>& grid, double dmin, double dmax) {
  std::vector<double> deltas;
  deltas.reserve(grid.data.size());
  for (int i = 0; i < 3; ++i) {
    int f[3] = {0, 0, 0};
    f[i] = 1;
    for (int w = f[2]; w < grid.nw; ++w)
      for (int v = f[1]; v < grid.nv; ++v)
        for (int u = f[0]; u < grid.nu; ++u)
          deltas.push_back(grid.get_value_q(u, v, w) -
                           grid.get_value_q(u - f[0], v - f[1], w - f[2]));
    gemmi::GridStats st = gemmi::calculate_grid_statistics(deltas);
    std::printf("\nd%c: min: %.5f  max: %.5f  mean: %.5f  std.dev: %.5f\n",
                "XYZ"[i], st.dmin, st.dmax, st.dmean, st.rms);
    print_histogram(deltas, dmin, dmax);
    deltas.clear();
  }
}

int GEMMI_MAIN(int argc, char **argv) {
  OptParser p(EXE_NAME);
  p.simple_parse(argc, argv, Usage);
  p.require_input_files_as_args();
  bool verbose = p.options[Verbose];

  if (p.nonOptionsCount() > 1 && (p.options[Reorder] || p.options[Full])) {
    std::fprintf(stderr, "Option --write-... can be only used "
                         "with a single input file.\n");
    return 1;
  }

  try {
    for (int i = 0; i < p.nonOptionsCount(); ++i) {
      const char* input = p.nonOption(i);
      gemmi::Ccp4<> map;
      if (i != 0)
        std::printf("\n\n");
      if (verbose)
        std::fprintf(stderr, "Reading %s ...\n", input);
      map.read_ccp4(gemmi::MaybeGzipped(input));
      gemmi::GridStats stats = print_info(map);
      if (p.options[Deltas])
        print_deltas(map.grid, stats.dmin, stats.dmax);
      if (p.options[Reorder]) {
        map.setup(gemmi::GridSetup::ReorderOnly, NAN);
        map.write_ccp4_map(p.options[Reorder].arg);
      }
      if (p.options[CheckSym]) {
        // TODO check labels vs group numbers
        double max_err = map.setup(gemmi::GridSetup::ResizeOnly, NAN);
        if (max_err != 0.0)
          std::printf("Max. difference for point images in P1: %g\n", max_err);
        const double eps = 0.01;
        max_err = 0;
        map.grid.symmetrize([&](float a, float b) {
            if (a < b || a > b) {
              double diff = std::fabs(a - b);
              if (diff > eps)
                std::printf("Symmetry-equivalent values differ: "
                            "%g != %g  diff: %g\n", a, b, diff);
              max_err = std::max(max_err, diff);
            }
            return std::isnan(a) ? b : a;
        });
        if (max_err != 0.0)
          std::printf("Max. difference in symmetry images: %g\n", max_err);
      }
      if (p.options[Full]) {
        double err = map.setup(gemmi::GridSetup::FullCheck, NAN);
        size_t nn = std::count_if(map.grid.data.begin(), map.grid.data.end(),
                                  [](float x) { return std::isnan(x); });
        if (err != 0.0)
          std::fprintf(stderr, "WARNING: different values for equivalent "
                               "points, max diff: %g\n", err);
        if (nn != 0)
          std::fprintf(stderr, "WARNING: %zu unknown values set to NAN\n", nn);
        map.write_ccp4_map(p.options[Full].arg);
      }
    }
  } catch (std::runtime_error& e) {
    std::fprintf(stderr, "ERROR: %s\n", e.what());
    return 1;
  }
  return 0;
}

// vim:sw=2:ts=2:et:path^=../include,../third_party
