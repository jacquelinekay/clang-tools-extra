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
#include "llvm/Support/YAMLParser.h"

using namespace clang::ast_matchers;


namespace clang {
namespace tidy {
namespace embedded {

TypesafeRegisterAccessCheck::TypesafeRegisterAccessCheck(
    StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {
  // TODO we need a default.
  // TODO Error handling from this constructor...
  auto chip_name = Options.get("ChipName", "");

  // test case: add a single known register value,
  address_map.emplace(std::make_pair(0xDEADBEEF, 0x1), "Deadbeef::v00");

  // Load the address map for a file 
  // TODO Clean this shit up
  /*
  std::ifstream i(chip_name + ".yaml");
  std::string yaml_lines;
  i >> yaml_lines;

  llvm::SourceMgr sm;
  StringRef input(yaml_lines);
  llvm::yaml::Stream stream(input, sm);

  for (llvm::yaml::document_iterator di = stream.begin(), de = stream.end();
       di != de; ++di) {
    llvm::yaml::Node *n = di->getRoot();
    if (n) {
      // Do something with n...
      // Check if N is a KeyValueNode
      auto kv_pair = static_cast<llvm::yaml::KeyValueNode*>(n);
      if (kv_pair) {
        auto k = static_cast<llvm::yaml::ScalarNode*>(kv_pair->getKey());
        if (!k) {
          continue;
        }
        auto v = static_cast<llvm::yaml::ScalarNode*>(kv_pair->getValue());
        if (!v) {
          continue;
        }
        // TODO add k as an Address type
        Address key;
        unsigned long long longk;
        if (!llvm::getAsUnsignedInteger(k->getRawValue(), 16, longk)) {
          continue;
        }
        key = static_cast<Address>(longk);  // TODO Boundary checking
        address_map.emplace(key, v->getRawValue().str());
      }
    } else
      break;
  }
  */
}

void TypesafeRegisterAccessCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {
  // TODO Custom type for ChipName?
  Options.store(Opts, "ChipName", "ChipName");
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

  // TODO Not binding to the address properly here
  auto IndirectWriteMatcher = binaryOperator(
      hasOperatorName("="),
      hasLHS(declRefExpr(hasType(isVolatileQualified())).bind("address")),
      hasRHS(integerLiteral().bind("value"))).bind("write_expr");

  Finder->addMatcher(IndirectWriteMatcher, this);
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
