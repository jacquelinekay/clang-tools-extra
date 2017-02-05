set(LLVM_LINK_COMPONENTS support)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

add_clang_library(clangTidyEmbeddedModule
  # add .cpp sources here
  EmbeddedTidyModule.cpp
  EmbeddedUtilities.cpp
  TypesafeRegisterReadCheck.cpp
  TypesafeRegisterWriteCheck.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyReadabilityModule
  clangTidyUtils
  )
