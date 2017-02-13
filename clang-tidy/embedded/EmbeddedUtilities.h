#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_EMBEDDED_EMBEDDED_UTILITIES_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_EMBEDDED_EMBEDDED_UTILITIES_H

#include "llvm/Support/YAMLTraits.h"
#include <unordered_map>
#include <string>

namespace clang {
namespace tidy {
namespace embedded {

using Address = uint32_t;
using MaskT = uint32_t;
using ValueT = uint32_t;

struct RegisterValue {
  llvm::yaml::Hex32 value;
  std::string name;
};

struct RegisterField {
  std::string name;
  llvm::yaml::Hex32 mask;
  std::vector<RegisterValue> values;
};

struct RegisterEntry {
  llvm::yaml::Hex32 address;
  std::string name;
  std::vector<RegisterField> fields;
};

struct FieldEntry {
  std::string name;
  std::unordered_map<ValueT, std::string> values;
};

struct MapEntry {
  std::string name;
  std::unordered_map<MaskT, FieldEntry> fields;
};

using AddressNameMap = std::unordered_map<Address, MapEntry>;

enum class ReadYamlErrorCode : char {
  Success,
  OpenFileFailed,
  ParseYAMLFailed
};

ReadYamlErrorCode ReadDescriptionYAML(std::string const& name, AddressNameMap& addressMap);

using VariadicMatcherT = clang::ast_matchers::internal::VariadicOperatorMatcher<
    clang::ast_matchers::internal::BindableMatcher<clang::Stmt>,
    clang::ast_matchers::internal::BindableMatcher<clang::Stmt>>;
VariadicMatcherT DereferencedVolatileCastMatcher();

clang::ast_matchers::internal::Matcher<clang::Expr> MaskMatcher();

// Given a mask and an MapEntry, return a sequence of Fields that the mask touches
std::string DecomposeIntoFields(const MapEntry& reg, const ValueT mask);

std::string DecomposeIntoFields(const MapEntry& reg, const ValueT mask, const ValueT value);

std::string GetAllFields(const MapEntry& reg);

} // namespace embedded
} // namespace tidy
} // namespace clang

namespace llvm {
namespace yaml {

template <>
struct MappingTraits<clang::tidy::embedded::RegisterValue> {
  static void mapping(IO &io, clang::tidy::embedded::RegisterValue &entry) {
    // TODO
    io.mapRequired("name",  entry.name);
    io.mapRequired("value", entry.value);
  }
};

template <>
struct MappingTraits<clang::tidy::embedded::RegisterField> {
  static void mapping(IO &io, clang::tidy::embedded::RegisterField &entry) {
    // TODO
    io.mapRequired("name",  entry.name);
    io.mapRequired("mask", entry.mask);
    io.mapRequired("values", entry.values);
  }
};

template <>
struct MappingTraits<clang::tidy::embedded::RegisterEntry> {
  static void mapping(IO &io, clang::tidy::embedded::RegisterEntry &entry) {
    io.mapRequired("address", entry.address);
    io.mapRequired("name",    entry.name);
    io.mapRequired("fields",  entry.fields);
  }
};

}  // namespace yaml
}  // namespace llvm

LLVM_YAML_IS_SEQUENCE_VECTOR(clang::tidy::embedded::RegisterField)
LLVM_YAML_IS_SEQUENCE_VECTOR(clang::tidy::embedded::RegisterEntry)
LLVM_YAML_IS_SEQUENCE_VECTOR(clang::tidy::embedded::RegisterValue)

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_EMBEDDED_EMBEDDED_UTILITIES_H
