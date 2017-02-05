//===--- TypesafeRegisterReadCheck.cpp - clang-tidy------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TypesafeRegisterReadCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace embedded {

TypesafeRegisterReadCheck::TypesafeRegisterReadCheck(StringRef Name, ClangTidyContext *Context)
  : ClangTidyCheck(Name, Context) {
  chipFile = Options.get("DescriptionFile", "test.yaml");
  ReadDescriptionYAML(chipFile, addressMap);
}

void TypesafeRegisterReadCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "DescriptionFile", chipFile);
}
void TypesafeRegisterReadCheck::registerMatchers(MatchFinder *Finder) {
  auto DereferencedVolatilePointer = ignoringImpCasts(
      allOf(DereferencedPointerCastMatcher(),
              expr(hasType(isVolatileQualified()))));

  auto DirectReadMatcher = binaryOperator(
    hasOperatorName("="),
    hasLHS(declRefExpr()),
    hasRHS(expr(DereferencedVolatilePointer).bind("read_expr")));
  Finder->addMatcher(DirectReadMatcher, this);

  auto AssignPointer = ignoringImpCasts(DereferencedPointerCastMatcher());

  auto DirectReadAssignMatcher = varDecl(
      hasInitializer(expr(AssignPointer).bind("read_expr")));
  Finder->addMatcher(DirectReadAssignMatcher, this);
}

void TypesafeRegisterReadCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedAddress = Result.Nodes.getNodeAs<IntegerLiteral>("address");
  const auto *MatchedExpr = Result.Nodes.getNodeAs<Expr>("read_expr");
  emitDiagnosticFor(MatchedExpr, MatchedAddress);
}

template <typename T>
void TypesafeRegisterReadCheck::emitDiagnosticFor(
    const T *MatchedNode, const IntegerLiteral *MatchedAddress) {
  Address k1 = static_cast<Address>(MatchedAddress->getValue().getLimitedValue());
  // ValueT k2 = static_cast<ValueT>(MatchedNode->getValue().getLimitedValue());
  if (addressMap.find(k1) == addressMap.end()) {
      return;
  }
  /*
  if (addressMap[k1].values.find(k2) == addressMap[k1].values.end()) {
      return;
  }
  */
  std::string replacement = "apply(read(" + addressMap[k1].name + "))";
  auto ReplacementRange = CharSourceRange::getTokenRange(MatchedNode->getLocStart(), MatchedNode->getLocEnd());

  diag(MatchedNode->getLocStart(),
       "Found read from register via volatile cast")
    << FixItHint::CreateReplacement(ReplacementRange, replacement);
}

} // namespace embedded
} // namespace tidy
} // namespace clang
