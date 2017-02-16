#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "EmbeddedUtilities.h"
#include "llvm/Support/Debug.h"

#include <iomanip>
#include <fstream>
#include <sstream>

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
  // Important
  // operand is not necessarily an integer literal.
  /*
  auto addressExpr = anyOf(integerLiteral().bind("address"),
      binaryOperator(hasEitherOperand(CastExpr(expr().bind("address_operand")))
      );
      */
  return allOf(unaryOperator(
                   hasOperatorName("*"),
                   hasUnaryOperand(ignoringParenImpCasts(explicitCastExpr(
                       hasDestinationType(isAnyPointer()),
                       hasSourceExpression(expr().bind("address"))
                   )))
               ),
               expr(hasType(isVolatileQualified())));
}

clang::ast_matchers::internal::Matcher<clang::Expr> MaskMatcher() {
  return ignoringParenImpCasts(binaryOperator(
                hasOperatorName("&"),
                hasEitherOperand(ignoringParenImpCasts(DereferencedVolatileCastMatcher())),
                hasEitherOperand(ignoringParenImpCasts(integerLiteral().bind("mask")))));
}

std::string DecomposeIntoFields(const MapEntry& reg, ValueT mask) {
  std::string fields = "";

  for (const auto& field_pairs : reg.fields) {
    if ((field_pairs.first & mask) > 0) {
      fields += reg.name + "::" + field_pairs.second.name;
      break;
    }
  }

  return fields.substr(0, fields.length() - 3);
}


// Write case
std::string DecomposeIntoFields(const MapEntry& reg, ValueT mask, ValueT value) {
  std::string fields = "";

  for (const auto& field_pairs : reg.fields) {
    if ((field_pairs.first & mask) > 0) {
      if (field_pairs.second.values.size() == 0) {
        fields += reg.name + "::" + field_pairs.second.name + ", " + AsHex(value);
        return fields;
        // Might want to break here since I'm not sure Kvasir supports multiple field, value pairs
      } else {
        // this field matches, try to match a value 
        for (const auto& value_pairs : field_pairs.second.values) {
          if ((value & field_pairs.first) > 0 || (field_pairs.first == 0 && value == 0)) {
            fields += reg.name + "::" + field_pairs.second.name + "ValC::" + value_pairs.second + ", ";
            break;
          }
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

std::string AsHex(ValueT n) {
  std::stringstream s;
  s << "0x" << std::hex << n;
  return s.str();
}

// Traverse the tree, evaluate the result
// Aggressively get the value of any integerLiterals in this expression and try to evaluate
// the expression without pointer casts
bool evaluateDiscardingPointerCasts(const Expr* currentExpr, Address& address, ASTContext& context) {
  llvm::APSInt evaluatedInt;
  if (currentExpr->EvaluateAsInt(evaluatedInt, context)) {
    address = evaluatedInt.getLimitedValue();
    return true;
  }
  if (const clang::ParenExpr* paren = dyn_cast<const clang::ParenExpr>(currentExpr)) {
    return evaluateDiscardingPointerCasts(paren->getSubExpr(), address, context);
  }
  if (const clang::CastExpr* cast = dyn_cast<const clang::CastExpr>(currentExpr)) {
    // get the cast operand as an integer, sum it to the total
    // TODO check the type of the cast, Ignoring imp casts, etc. convert DeclrefExpr
    //
    if (const clang::IntegerLiteral* lit = dyn_cast<const clang::IntegerLiteral>(cast->getSubExpr())) {
      address = lit->getValue().getLimitedValue();
      return true;
    }
    return evaluateDiscardingPointerCasts(cast->getSubExpr(), address, context);
  }
  if (const clang::BinaryOperator* op = dyn_cast<clang::BinaryOperator>(currentExpr)) {
    // Evaluate each operand separately, trying to discard pointer casts, then apply the operand
    Address result1, result2;
    if (!evaluateDiscardingPointerCasts(op->getLHS(), result1, context)) {
      return false;
    }
    if (!evaluateDiscardingPointerCasts(op->getRHS(), result2, context)) {
      return false;
    }
    if (op->isAdditiveOp()) {
      address = result1 + result2;
    } else if (op->isMultiplicativeOp()) {
      address = result1 * result2;
    } else if (op->isShiftOp()) {
      if (op->getOpcode() == clang::BinaryOperatorKind::BO_Shl) {
        address = result1 << result2;
      } else if (op->getOpcode() == clang::BinaryOperatorKind::BO_Shr) {
        address = result1 >> result2;
      } else {
        return false;
      }
    } else {
      return false;
    }
    return true;
  }
  return false;
}



} // namespace embedded
} // namespace tidy
} // namespace clang
