//===--- TypesafeRegisterAccessCheck.h - clang-tidy--------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_EMBEDDED_TYPESAFE_REGISTER_ACCESS_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_EMBEDDED_TYPESAFE_REGISTER_ACCESS_H

#include "../ClangTidy.h"
#include "llvm/Support/YAMLTraits.h"
#include <vector>

namespace clang {
namespace tidy {
namespace embedded {

using Address = uint32_t;
using ValueT = uint32_t;

/// Check for hardcoded register writes and replace with "apply".
class TypesafeRegisterAccessCheck : public ClangTidyCheck {
public:
  using AddressKey = std::pair<Address, ValueT>;
  TypesafeRegisterAccessCheck(StringRef Name, ClangTidyContext *Context);

  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  std::string chipFile;
  std::map<AddressKey, std::string> address_map;
};

struct RegisterEntry {
  llvm::yaml::Hex32 address;
  llvm::yaml::Hex32 value;
  std::string registerName;
};

} // namespace embedded
} // namespace tidy
} // namespace clang

namespace llvm {
namespace yaml {

template <>
struct MappingTraits<clang::tidy::embedded::RegisterEntry> {
  static void mapping(IO &io, clang::tidy::embedded::RegisterEntry &entry) {
    io.mapRequired("address",       entry.address);
    io.mapRequired("value",         entry.value);
    io.mapRequired("register-name", entry.registerName);
  }
};

}  // namespace yaml
}  // namespace llvm

LLVM_YAML_IS_SEQUENCE_VECTOR(clang::tidy::embedded::RegisterEntry)

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_EMBEDDED_TYPESAFE_REGISTER_ACCESS_H
