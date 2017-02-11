#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "EmbeddedUtilities.h"
#include "llvm/Support/Debug.h"

#include <fstream>

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
    // addressMap[std::make_pair(reg.address, reg.value)] = reg.registerName;
    addressMap[reg.address].name = reg.name;
    // DEBUG(llvm::dbgs() << "Added register: " << reg.address << "," << reg.name << "\n" );
    for (const auto& field : reg.fields) {
      addressMap[reg.address].fields[field.mask].name = field.name;
      for (const auto& value : field.values) {
        addressMap[reg.address].fields[field.mask].values[value.value] = value.name;
      }
    }
  }
  return ReadYamlErrorCode::Success;
}

internal::BindableMatcher<clang::Stmt> DereferencedPointerCastMatcher() {
  return unaryOperator(hasOperatorName("*"), hasUnaryOperand(
            explicitCastExpr(
                hasDestinationType(isAnyPointer()),
                hasSourceExpression(integerLiteral().bind("address")))));
}

} // namespace embedded
} // namespace tidy
} // namespace clang
