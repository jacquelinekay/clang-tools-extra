//===--- TypesafeRegisterWriteCheck.cpp - clang-tidy----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <fstream>

#include "TypesafeRegisterWriteCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#define DEBUG_TYPE ""

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace embedded {

TypesafeRegisterWriteCheck::TypesafeRegisterWriteCheck(
    StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {
  chipFile = Options.get("DescriptionFile", "test.yaml");
  ReadDescriptionYAML(chipFile, addressMap);
}

void TypesafeRegisterWriteCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "DescriptionFile", chipFile);
}

void TypesafeRegisterWriteCheck::registerMatchers(MatchFinder *Finder) {
  // this will only be a "write" check

  // TODO: writing known values vs. writing arbitrary values
  // TODO combining writes
  // TODO reset (set to clear)
  // TODO clear
  // write(peripheralName::registerName, value)
  // vs.
  // write(peripheralName::registerName::knownValue)

  auto DereferencedVolatilePointer = DereferencedPointerCastMatcher();

  auto DirectWriteMatcher = binaryOperator(
          hasOperatorName("="),
          hasLHS(allOf(DereferencedVolatilePointer,
              expr(hasType(isVolatileQualified())))),
          hasRHS(ignoringImpCasts(integerLiteral().bind("value")))).bind("write_expr");

  Finder->addMatcher(DirectWriteMatcher, this);

  /*
  auto VolatileDeclRefAssignment = varDecl(hasInitializer(explicitCastExpr(
                hasDestinationType(isAnyPointer()),
                hasSourceExpression(integerLiteral().bind("address"))))).bind("var_decl");

  // TODO Not binding to the address properly here
  auto IndirectWriteMatcher = binaryOperator(
      hasOperatorName("="),
      hasLHS(unaryOperator(hasOperatorName("*"), hasUnaryOperand(ignoringImpCasts(declRefExpr().equalsBoundNode("var_decl"))))),
      hasRHS(integerLiteral().bind("value"))).bind("write_expr");

  Finder->addMatcher(IndirectWriteMatcher, this);
  */
}

void TypesafeRegisterWriteCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedValue = Result.Nodes.getNodeAs<IntegerLiteral>("value");
  const auto *MatchedAddress = Result.Nodes.getNodeAs<IntegerLiteral>("address");
  const auto *MatchedLocation = Result.Nodes.getNodeAs<Expr>("write_expr");
  if (!MatchedValue || !MatchedAddress || !MatchedLocation) {
  //if (!MatchedAddress || !MatchedLocation) {
    return;
  }

  Address k1 = static_cast<Address>(MatchedAddress->getValue().getLimitedValue());
  ValueT k2 = static_cast<ValueT>(MatchedValue->getValue().getLimitedValue());
  if (addressMap.find(k1) == addressMap.end()) {
      return;
  }
  if (addressMap[k1].values.find(k2) == addressMap[k1].values.end()) {
      return;
  }

  // cast the matched integer literal as a 32-bit unsigned
  std::string replacement = "apply(write(" + addressMap[k1].values[k2] + "))";
  auto ReplacementRange = CharSourceRange::getTokenRange(MatchedLocation->getLocStart(), MatchedLocation->getLocEnd());

  diag(MatchedLocation->getLocStart(), "Found write to register via volatile cast")
    << FixItHint::CreateReplacement(ReplacementRange, replacement);
}

} // namespace embedded
} // namespace tidy
} // namespace clang
