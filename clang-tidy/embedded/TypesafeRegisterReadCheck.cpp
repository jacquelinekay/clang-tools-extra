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

#define DEBUG_TYPE ""

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
  auto DereferencedVolatilePointer = ignoringImpCasts(DereferencedVolatileCastMatcher());

  auto MaskedRead = MaskMatcher();

  auto DirectReadMatcher = binaryOperator(
    hasOperatorName("="),
    hasLHS(declRefExpr()),
    hasRHS(expr(anyOf(expr(DereferencedVolatilePointer), MaskedRead)).bind("read_expr")));
  Finder->addMatcher(DirectReadMatcher, this);

  auto DirectReadAssignMatcher = varDecl(
      hasInitializer(expr(anyOf(expr(DereferencedVolatilePointer), MaskedRead)).bind("read_expr")));
  Finder->addMatcher(DirectReadAssignMatcher, this);
}

void TypesafeRegisterReadCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedAddress = Result.Nodes.getNodeAs<Expr>("address");
  const auto *MatchedExpr = Result.Nodes.getNodeAs<Expr>("read_expr");
  const auto *MatchedMask = Result.Nodes.getNodeAs<IntegerLiteral>("mask");
  if (!MatchedAddress || !MatchedExpr || !Result.Context) {
    return;
  }

  llvm::APSInt evaluatedInt;
  if (!MatchedAddress->EvaluateAsInt(evaluatedInt, *Result.Context)) {
    DEBUG(llvm::errs() << "Couldn't evaluate address expression as an integer\n");
    return;
  }

  Address address = static_cast<Address>(evaluatedInt.getLimitedValue());
  // Address address = static_cast<Address>(MatchedAddress->EvaluateAsInt().getValue().getLimitedValue());
  if (addressMap.find(address) == addressMap.end()) {
      return;
  }
  const auto& reg = addressMap.at(address);
  std::string replacement = "";
  if (MatchedMask) {
    // Get the name from the map
    ValueT mask = UINT32_MAX - static_cast<ValueT>(MatchedMask->getValue().getLimitedValue());
    // TODO Special case for when the field has no predefined values
    if (reg.fields.find(mask) != reg.fields.end()) {
      replacement = "apply(read(" + reg.name + "::" + reg.fields.at(mask).name + "))";
    } else {
      replacement = "apply(read(" + DecomposeIntoFields(reg, mask) + "))";
    }
  } else {
    replacement = "apply(read(" + DecomposeIntoFields(reg, 0xFFFFFFFF) + "))";
  }
  auto ReplacementRange = CharSourceRange::getTokenRange(MatchedExpr->getLocStart(), MatchedExpr->getLocEnd());

  diag(MatchedExpr->getLocStart(),
       "Found read from register via volatile cast")
    << FixItHint::CreateReplacement(ReplacementRange, replacement);
}

} // namespace embedded
} // namespace tidy
} // namespace clang
