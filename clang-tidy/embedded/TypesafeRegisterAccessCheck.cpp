//===--- TypesafeRegisterAccessCheck.cpp - clang-tidy----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <fstream>

#include "TypesafeRegisterAccessCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include "llvm/Support/Debug.h"

#define DEBUG_TYPE ""

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace embedded {

TypesafeRegisterAccessCheck::TypesafeRegisterAccessCheck(
    StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {
  // TODO Better error handling from this constructor...
  chipFile = Options.get("DescriptionFile", "test.yaml");

  // Load the address map for a file 
  // TODO folder location for this...
  std::ifstream istream(chipFile);
  if (!istream) {
    DEBUG(llvm::errs() << "Failed to open yaml file\n");
    return;
  }
  std::string yamlInput((std::istreambuf_iterator<char>(istream)), std::istreambuf_iterator<char>());

  std::vector<RegisterEntry> registers;
  llvm::yaml::Input yamlIn(yamlInput);
  if (yamlIn.error()) {
    DEBUG(llvm::errs() << "Failed open yaml input\n");
    return;
  }

  yamlIn >> registers;

  for (auto& reg : registers) {
    address_map[std::make_pair(reg.address, reg.value)] = reg.registerName;
    DEBUG(llvm::dbgs() << "Added register" << reg.address << "," << reg.value << ", " << reg.registerName << "\n" );
  }
}

void TypesafeRegisterAccessCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "DescriptionFile", chipFile);
}

void TypesafeRegisterAccessCheck::registerMatchers(MatchFinder *Finder) {
  // this will only be a "write" check

  auto DereferencedVolatilePointer = unaryOperator(hasOperatorName("*"), hasUnaryOperand(
            explicitCastExpr(
                hasDestinationType(isAnyPointer()),
                hasSourceExpression(integerLiteral().bind("address")))));

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

void TypesafeRegisterAccessCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedValue = Result.Nodes.getNodeAs<IntegerLiteral>("value");
  const auto *MatchedAddress = Result.Nodes.getNodeAs<IntegerLiteral>("address");
  const auto *MatchedLocation = Result.Nodes.getNodeAs<Expr>("write_expr");
  if (!MatchedValue || !MatchedAddress || !MatchedLocation) {
    return;
  }

  Address k1 = static_cast<Address>(MatchedAddress->getValue().getLimitedValue());
  ValueT k2 = static_cast<ValueT>(MatchedValue->getValue().getLimitedValue());
  auto key = std::make_pair(k1, k2);
  if (address_map.find(key) == address_map.end()) {
      return;
  }

  // cast the matched integer literal as a 32-bit unsigned
  std::string replacement = "apply(write(" + address_map[key] + "))";
 auto ReplacementRange = CharSourceRange::getTokenRange(MatchedLocation->getLocStart(), MatchedLocation->getLocEnd());

  diag(MatchedLocation->getLocStart(), "Found write to register via volatile cast")
    << FixItHint::CreateReplacement(ReplacementRange, replacement);
}

} // namespace embedded
} // namespace tidy
} // namespace clang
