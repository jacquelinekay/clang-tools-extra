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
  address_map.emplace(0xDEADBEEF, "deadbeef");

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
  // Match all compound statements
  // TODO Add more operators
  // TODO Check if address is a memory field type. need another data table for this
  // Check for user defined literals
  // find "x & r"
  /*
  auto AndStatement = binaryOperator(
        hasOperatorName("&"),
        hasLHS(expr().bind("address")),
        hasRHS(integerLiteral().bind("literal")));
  auto AssignStatement(binaryOperator(
        hasOperatorName("="),
        hasLHS(equalsBoundNode("address")),
        hasRHS(AndStatement)));
  */
  auto CompoundAssignStatement = binaryOperator(
        hasOperatorName("&="),
        // hasLHS(rValueReferenceType(isVolatileQualified()).bind("address")),
        hasRHS(integerLiteral().bind("literal")));
  Finder->addMatcher(CompoundAssignStatement, this);
}

void TypesafeRegisterAccessCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedLiteral = Result.Nodes.getNodeAs<IntegerLiteral>("literal");
  if (!MatchedLiteral) {
    // fail silently :(
    return;
  }

  Address key = static_cast<Address>(MatchedLiteral->getValue().getLimitedValue());
  // cast the matched integer literal as a 32-bit unsigned
  if (address_map.find(key) != address_map.end()) {
    StringRef replacement = "set(" + address_map[key] + ")";
    /*
    diag(MatchedLiteral->getLocation(), "Found a raw compound assignment")
      << MatchedLiteral
      << FixItHint::CreateReplacement(MatchedLiteral->getLocation(), replacement);
      */
  }

  // TODO check if literal is in the map

  /*
  if (MatchedDecl->getName().startswith("awesome_"))
    return;
  diag(MatchedDecl->getLocation(), "function %0 is insufficiently awesome")
      << MatchedDecl
      << FixItHint::CreateInsertion(MatchedDecl->getLocation(), "awesome_");
  */
}

} // namespace embedded
} // namespace tidy
} // namespace clang
