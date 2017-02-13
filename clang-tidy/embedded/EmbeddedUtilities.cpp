#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "EmbeddedUtilities.h"
#include "llvm/Support/Debug.h"

#include <fstream>

#define DEBUG_TYPE ""

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace embedded {

ReadYamlErrorCode ReadDescriptionYAML(std::string const& name, AddressNameMap& addressMap) {
  // Load the address map for a file 
  std::ifstream istream(name);
  if (!istream) {
    DEBUG(llvm::errs() << "Failed to open yaml file\n");
    return ReadYamlErrorCode::OpenFileFailed;
  }
  std::string yamlInput((std::istreambuf_iterator<char>(istream)), std::istreambuf_iterator<char>());

  std::vector<RegisterEntry> registers;
  llvm::yaml::Input yamlIn(yamlInput);
  if (yamlIn.error()) {
    DEBUG(llvm::errs() << "Failed open yaml input\n");
    return ReadYamlErrorCode::ParseYAMLFailed;
  }

  yamlIn >> registers;

  for (const auto& reg : registers) {
    addressMap[reg.address].name = reg.name;
    DEBUG(llvm::dbgs() << "Added register: " << reg.address << "," << reg.name << "\n" );
    for (const auto& field : reg.fields) {
      addressMap[reg.address].fields[field.mask].name = field.name;
      for (const auto& value : field.values) {
        addressMap[reg.address].fields[field.mask].values[value.value] = value.name;
      }
    }
  }
  return ReadYamlErrorCode::Success;
}

VariadicMatcherT DereferencedVolatileCastMatcher() {
  return allOf(unaryOperator(
                   hasOperatorName("*"),
                   hasUnaryOperand(explicitCastExpr(
                       hasDestinationType(isAnyPointer()),
                       hasSourceExpression(integerLiteral().bind("address"))))),
               expr(hasType(isVolatileQualified())));
}

clang::ast_matchers::internal::Matcher<clang::Expr> MaskMatcher() {
  return ignoringParenImpCasts(binaryOperator(
                hasOperatorName("&"),
                hasEitherOperand(ignoringParenImpCasts(DereferencedVolatileCastMatcher())),
                hasEitherOperand(ignoringParenImpCasts(integerLiteral().bind("mask")))));
}

std::string DecomposeIntoFields(const MapEntry& reg, const ValueT mask) {
  std::string fields = "";

  for (const auto& field_pairs : reg.fields) {
    if ((field_pairs.first & mask) > 0) {
      // this field matches. try to match a value 
      fields += reg.name + "::" + field_pairs.second.name;
      break;
    }
  }

  return fields.substr(0, fields.length() - 3);
}


// Write case
std::string DecomposeIntoFields(const MapEntry& reg, const ValueT mask, const ValueT value) {
  std::string fields = "";

  for (const auto& field_pairs : reg.fields) {
    if ((field_pairs.first & mask) > 0) {
      // this field matches. try to match a value 
      for (const auto& value_pairs : field_pairs.second.values) {
        if ((value & field_pairs.first) > 0 || (field_pairs.first == 0 && value == 0)) {
          fields += reg.name + "::" + field_pairs.second.name + "ValC::" + value_pairs.second + ", ";
          break;
        }
      }
    }
  }

  return fields.substr(0, fields.length() - 3);
}

std::string GetAllFields(const MapEntry& reg) {
  std::string fields = "";
  // Assume if no mask is specified that we're writing to all registers.
  for (const auto& field_pairs : reg.fields) {
    fields += reg.name + "::" + field_pairs.second.name + ", ";
  }
  return fields.substr(0, fields.length() - 3);
}


} // namespace embedded
} // namespace tidy
} // namespace clang
