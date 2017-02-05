//===--- EmbeddedTidyModule.cpp - clang-tidy -----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "TypesafeRegisterReadCheck.h"
#include "TypesafeRegisterWriteCheck.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace embedded {

class EmbeddedModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<TypesafeRegisterReadCheck>(
        "embedded-typesafe-register-read");
    CheckFactories.registerCheck<TypesafeRegisterWriteCheck>(
        "embedded-typesafe-register-write");
  }

  ClangTidyOptions getModuleOptions() override {
    ClangTidyOptions Options;
    auto &Opts = Options.CheckOptions;
    Opts["embedded-typesafe-register-write.DescriptionFile"] = "test.yaml";
    return Options;
  }
};

// Register the EmbeddedTidyModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<EmbeddedModule> X("embedded-module",
                                                       "Add checks for embedded C++.");

} // namespace embedded

// This anchor is used to force the linker to link in the generated object file
// and thus register the EmbeddedModule.
volatile int EmbeddedModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
