//===-- IncludeFixerContext.cpp - Include fixer context ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "IncludeFixerContext.h"
#include <algorithm>

namespace clang {
namespace include_fixer {

namespace {

// Splits a multiply qualified names (e.g. a::b::c).
llvm::SmallVector<llvm::StringRef, 8>
SplitQualifiers(llvm::StringRef StringQualifiers) {
  llvm::SmallVector<llvm::StringRef, 8> Qualifiers;
  StringQualifiers.split(Qualifiers, "::");
  return Qualifiers;
}

std::string createQualifiedNameForReplacement(
    llvm::StringRef RawSymbolName,
    llvm::StringRef SymbolScopedQualifiersName,
    const find_all_symbols::SymbolInfo &MatchedSymbol) {
  // No need to add missing qualifiers if SymbolIndentifer has a global scope
  // operator "::".
  if (RawSymbolName.startswith("::"))
    return RawSymbolName;

  std::string QualifiedName = MatchedSymbol.getQualifiedName();

  // For nested classes, the qualified name constructed from database misses
  // some stripped qualifiers, because when we search a symbol in database,
  // we strip qualifiers from the end until we find a result. So append the
  // missing stripped qualifiers here.
  //
  // Get stripped qualifiers.
  auto SymbolQualifiers = SplitQualifiers(RawSymbolName);
  std::string StrippedQualifiers;
  while (!SymbolQualifiers.empty() &&
         !llvm::StringRef(QualifiedName).endswith(SymbolQualifiers.back())) {
    StrippedQualifiers = "::" + SymbolQualifiers.back().str();
    SymbolQualifiers.pop_back();
  }
  // Append the missing stripped qualifiers.
  std::string FullyQualifiedName = QualifiedName + StrippedQualifiers;

  // Try to find and skip the common prefix qualifiers.
  auto FullySymbolQualifiers = SplitQualifiers(FullyQualifiedName);
  auto ScopedQualifiers = SplitQualifiers(SymbolScopedQualifiersName);
  auto FullySymbolQualifiersIter = FullySymbolQualifiers.begin();
  auto SymbolScopedQualifiersIter = ScopedQualifiers.begin();
  while (FullySymbolQualifiersIter != FullySymbolQualifiers.end() &&
         SymbolScopedQualifiersIter != ScopedQualifiers.end()) {
    if (*FullySymbolQualifiersIter != *SymbolScopedQualifiersIter)
      break;
    ++FullySymbolQualifiersIter;
    ++SymbolScopedQualifiersIter;
  }
  std::string Result;
  for (; FullySymbolQualifiersIter != FullySymbolQualifiers.end();
       ++FullySymbolQualifiersIter) {
    if (!Result.empty())
      Result += "::";
    Result += *FullySymbolQualifiersIter;
  }
  return Result;
}

} // anonymous namespace

IncludeFixerContext::IncludeFixerContext(
    const QuerySymbolInfo &QuerySymbol,
    const std::vector<find_all_symbols::SymbolInfo> Symbols)
    : MatchedSymbols(std::move(Symbols)), QuerySymbol(QuerySymbol) {
  for (const auto &Symbol : MatchedSymbols) {
    HeaderInfos.push_back(
        {Symbol.getFilePath().str(),
         createQualifiedNameForReplacement(
             QuerySymbol.RawIdentifier, QuerySymbol.ScopedQualifiers, Symbol)});
  }
  // Deduplicate header infos.
  HeaderInfos.erase(std::unique(HeaderInfos.begin(), HeaderInfos.end(),
                                [](const HeaderInfo &A, const HeaderInfo &B) {
                                  return A.Header == B.Header &&
                                         A.QualifiedName == B.QualifiedName;
                                }),
                    HeaderInfos.end());
}

} // include_fixer
} // clang