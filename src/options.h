// Copyright 2017 Global Phasing Ltd.

// Thin, leaky wrapper around The Lean Mean C++ Option Parser.

#pragma once

#include <vector>
#include <string>
#include <optionparser.h>

#ifndef GEMMI_PROG
# error "define GEMMI_PROG before including options.h"
#endif

#define GEMMI_XSTRINGIZE(s) GEMMI_STRINGIZE(s)
#define GEMMI_STRINGIZE(s) #s
#define GEMMI_XCONCAT(a, b) GEMMI_CONCAT(a, b)
#define GEMMI_CONCAT(a, b) a##b
#ifdef GEMMI_ALL_IN_ONE  // GEMMI_MAIN=foo_main EXE_NAME="gemmi foo"
# define GEMMI_MAIN GEMMI_XCONCAT(GEMMI_PROG, _main)
# define EXE_NAME "gemmi " GEMMI_XSTRINGIZE(GEMMI_PROG)
#else                    // GEMMI_MAIN=main     EXE_NAME="gemmi-foo"
# define GEMMI_MAIN main
# define EXE_NAME "gemmi-" GEMMI_XSTRINGIZE(GEMMI_PROG)
#endif

enum { NoOp=0, Help=1, Version=2 };

extern const option::Descriptor CommonUsage[];

std::vector<int> parse_comma_separated_ints(const char* arg);

struct Arg: public option::Arg {
  static option::ArgStatus Required(const option::Option& option, bool msg);
  static option::ArgStatus Char(const option::Option& option, bool msg);
  static option::ArgStatus Choice(const option::Option& option, bool msg,
                                  std::vector<const char*> choices);
  static option::ArgStatus Int(const option::Option& option, bool msg);
  static option::ArgStatus Int3(const option::Option& option, bool msg);
  static option::ArgStatus Float(const option::Option& option, bool msg);
  static option::ArgStatus CoorFormat(const option::Option& option, bool msg) {
    return Choice(option, msg, {"cif", "pdb", "json", "chemcomp"});
  }
};

struct OptParser : option::Parser {
  const char* program_name;
  std::vector<option::Option> options;
  std::vector<option::Option> buffer;
  std::vector<std::vector<int>> exclusive_groups;

  explicit OptParser(const char* prog) : program_name(prog) {}
  void simple_parse(int argc, char** argv, const option::Descriptor usage[]);
  void require_positional_args(int n);
  void require_input_files_as_args(int other_args=0);
  // returns the nth arg except that a PDB code gets replaced by $PDB_DIR/...
  std::string coordinate_input_file(int n);
  std::vector<std::string> paths_from_args_or_file(int opt, int other);
  [[noreturn]] void print_try_help_and_exit(const char* msg);
  const char* given_name(int opt) const {  // sans one dash
    return options[opt].namelen > 1 ? options[opt].name + 1
                                    : options[opt].desc->shortopt;
  }
};

namespace gemmi { enum class CoorFormat; }

// to be used with Arg::CoorFormat
gemmi::CoorFormat coor_format_as_enum(const option::Option& format_in);

// can be used with paths_from_args_or_file()
bool starts_with_pdb_code(const std::string& s);

// vim:sw=2:ts=2:et:path^=../include,../third_party
