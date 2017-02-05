//===--- TypesafeRegisterWriteCheck.h - clang-tidy--------------*- C++ -*-===//
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
#include "EmbeddedUtilities.h"
#include <vector>

namespace clang {
namespace tidy {
namespace embedded {

/// Check for hardcoded register writes and replace with "apply".
class TypesafeRegisterWriteCheck : public ClangTidyCheck {
public:
  TypesafeRegisterWriteCheck(StringRef Name, ClangTidyContext *Context);

  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  std::string chipFile;
  AddressNameMap addressMap;
};

} // namespace embedded
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_EMBEDDED_TYPESAFE_REGISTER_ACCESS_H
