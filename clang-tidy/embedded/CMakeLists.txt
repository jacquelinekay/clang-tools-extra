set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyEmbeddedModule
  # add .cpp sources here
  EmbeddedTidyModule.cpp
  TypesafeRegisterAccessCheck.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyReadabilityModule
  clangTidyUtils
  )
