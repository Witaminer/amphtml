#ifndef CPP_HTMLPARSER_GRAMMAR_TABLEBUILDER_H_
#define CPP_HTMLPARSER_GRAMMAR_TABLEBUILDER_H_

//
// NOTE: This is very basic right now. No validation of grammar syntax. Lack of
// verbose errors and not well tested. Parsers using table generated by this
// class must be thoroughly tested.
//
// Builds a state table by reading grammar file that contains rules for parsing
// a basic (limited), context free, unambigous grammar.
//
// Using TableBuilder one can generate parser states by writing rules in a
// text file. See htmlparser/data/jsongrammar.txt.
//
// Grammar text file contains rules which lists states and its transition
// from one state to another as parser reads input characters. The parse
// table is pushdown automation that uses stack to push and pop parsing
// states. Unline LR parsers there is no shift at each stage of parsing.

// See grammar.txt tutorial for learning grammar syntax.
// TODO: Add grammar tutorial.
//
// Notes:
//  - Supports up to 255 unique states.
//  - Supports up to 255 unique callbacks.
//  - Support for only ascii chars grammar.

#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace htmlparser::grammar {

struct Input {
  // If true, the any char is matched except the input char.
  bool exclude = false;
  // The set of characters to match.
  std::set<char> charset;
};

struct Rule {
  std::string state;
  Input input;
  std::vector<std::string> transition;
  std::string callback;
};

class TableBuilder  {
 public:
  struct ParseOptions {
    std::string output_file_path;
    std::string ifdef_guard;
    std::string cpp_namespace;
    // Not all syntax has a definite terminal point.
    // Example: in json ] ends the array, } ends the dict etc, so it can stop
    // processing or shift to END_* state upon encountering such token.
    // But in some cases we have to hint parser when to terminate (end of input)
    // parsing, this character helps generate the state for last token that is
    // termination_sentinel.
    uint8_t termination_sentinel;
  };

  TableBuilder(std::string_view grammar_file_path, ParseOptions options);

  bool ParseRulesAndGenerateTable();

 private:
  bool OutputHeaderFile(const std::set<std::string>& declared_states,
                        const std::array<int, 127>& tokenindexes,
                        const std::map<uint8_t, std::vector<uint32_t>>& table);

  // The state code contains information about the state, transition,
  // shifting.
  //
  // lowest 5 bits are reserved for future use.
  // 6th bit contains shift bit.
  // 7th bit contains pop bit.
  // 8th bit contains push bit.
  // next 8 bits contains active state code.
  // next 8 bits contains shift state code.
  // next 8 bits contains callback code.
  //
  // 0b11111111  11111111   11111111   1      1      1      11111
  // ----------  --------   --------   -      -      -      -----
  //     |           |         |       |      |      |        |
  //  callback  shift code   state   push    pop   shift   reserved.
  //
  std::optional<uint32_t> ComputeState(Rule r);
  bool ParseGrammarFile();
  void RemoveLeadingWhitespace(std::string_view* line) const;
  std::optional<Rule> ReadRule(std::string_view line, int line_no) const;
  std::optional<uint32_t> ComputeStateBits(uint8_t callback_code,
                                           uint8_t push_state_code,
                                           uint8_t current_state_code,
                                           bool push,
                                           bool pop,
                                           bool shift);

  std::vector<Rule> raw_rules_;
  std::string grammar_file_path_;
  ParseOptions parse_options_;
  std::map<std::string, uint8_t> state_codes_{};
  std::map<std::string, uint8_t> callback_codes_{};
  std::set<uint8_t> charset_{};
};

}  // namespace htmlparser::grammar


#endif  // CPP_HTMLPARSER_GRAMMAR_TABLEBUILDER_H_
