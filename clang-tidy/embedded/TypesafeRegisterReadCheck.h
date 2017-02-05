//===--- TypesafeRegisterReadCheck.h - clang-tidy----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_EMBEDDED_TYPESAFE_REGISTER_READ_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_EMBEDDED_TYPESAFE_REGISTER_READ_H

#include "../ClangTidy.h"
#include "EmbeddedUtilities.h"

namespace clang {
namespace tidy {
namespace embedded {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/embedded-typesafe-register-read.html
class TypesafeRegisterReadCheck : public ClangTidyCheck {
public:
  TypesafeRegisterReadCheck(StringRef Name, ClangTidyContext *Context);
  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  template<typename T>
  void emitDiagnosticFor(const T* MatchedNode, const IntegerLiteral* MatchedAddress);
  std::string chipFile;
  AddressNameMap addressMap;
};

} // namespace embedded
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_EMBEDDED_TYPESAFE_REGISTER_READ_H
