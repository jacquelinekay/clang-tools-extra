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

#include "llvm/IR/Constants.h"

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

  // TODO: Make sure we are referencing the same register within the same expression.
  // TODO: writing known values vs. writing arbitrary values
  // TODO combining writes
  // TODO reset (set to clear)
  // TODO clear

  auto DereferencedVolatilePointer = DereferencedVolatileCastMatcher();

  // If there's no mask, we expect this to write to all fields, need to decompose
  auto DirectWriteMatcher = binaryOperator(
          hasOperatorName("="),
          hasLHS(DereferencedVolatilePointer),
          hasRHS(ignoringParenImpCasts(integerLiteral().bind("value")))).bind("write_expr");

  Finder->addMatcher(DirectWriteMatcher, this);

  auto MaskedWriteMatcher = binaryOperator(
          hasOperatorName("="),
          hasLHS(DereferencedVolatilePointer),
          hasRHS(
            ignoringParenImpCasts(binaryOperator(
              hasOperatorName("|"),
              hasEitherOperand(MaskMatcher()),
              hasEitherOperand(ignoringParenImpCasts(integerLiteral().bind("value")))
            )
          ))
        ).bind("write_expr");
  Finder->addMatcher(MaskedWriteMatcher, this);

  /*
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
  const auto *MatchedAddress = Result.Nodes.getNodeAs<Expr>("address");
  const auto *MatchedLocation = Result.Nodes.getNodeAs<Expr>("write_expr");
  const auto *MatchedMask = Result.Nodes.getNodeAs<IntegerLiteral>("mask");
  Address address = 0;

  DEBUG(llvm::errs() << "Matched expression\n");

  if (!MatchedValue || !MatchedAddress || !MatchedLocation || !Result.Context) {
    return;
  }

  const Expr* currentExpr = MatchedAddress;
  if (!evaluateDiscardingPointerCasts(currentExpr, address, *Result.Context)) {
    DEBUG(llvm::errs() << "Couldn't evaluate expression as int\n");
    return;
  }

  ValueT value = static_cast<ValueT>(MatchedValue->getValue().getLimitedValue());

  if (addressMap.find(address) == addressMap.end()) {
    DEBUG(llvm::errs() << "address not in map!\n");
    return;
  }

  const auto r = addressMap[address];
  std::string replacement = "";

  if (MatchedMask) {
    // Invert the mask, since it describes fields we aren't touching
    ValueT mask = UINT32_MAX - static_cast<ValueT>(MatchedMask->getValue().getLimitedValue());

    // TODO Special case for when the field has no predefined values
    if (r.fields.find(mask) != r.fields.end()) {
      auto field = r.fields.at(mask);
      if (field.values.find(value) == field.values.end()) {
        // Raw write to field
        replacement = "apply(write(" + r.name + "::" + field.name + ", " + AsHex(value) + "))";
      } else {
        replacement = "apply(write(" + r.name + "::" + field.name + "ValC::" + field.values.at(value) + "))";
        DEBUG(llvm::errs() << "string: " << replacement + "\n");
      }
    } else {
      DEBUG(llvm::errs() << "Found decomposition case\n");
      // The mask might describe multiple fields,
      // in which case we need try to decompose the mask into constituent parts
      replacement = "apply(write(" + DecomposeIntoFields(r, mask, value) + "))";
      return;
    }
  } else {
    replacement = "apply(write(" + DecomposeIntoFields(r, 0xFFFFFFFF, value) + "))";
  }
  auto ReplacementRange = CharSourceRange::getTokenRange(MatchedLocation->getLocStart(), MatchedLocation->getLocEnd());

  diag(MatchedLocation->getLocStart(), "Found write to register via volatile cast")
    << FixItHint::CreateReplacement(ReplacementRange, replacement);
}

} // namespace embedded
} // namespace tidy
} // namespace clang
