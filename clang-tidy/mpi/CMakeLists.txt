set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyMPIModule
  MPITidyModule.cpp
  TypeMismatchCheck.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyUtils
  clangTooling
  clangStaticAnalyzerCheckers
  )
