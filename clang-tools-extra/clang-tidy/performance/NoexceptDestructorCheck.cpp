//===--- NoexceptDestructorCheck.cpp - clang-tidy -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "NoexceptDestructorCheck.h"
#include "../utils/LexerUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang::tidy::performance {

void NoexceptDestructorCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      functionDecl(unless(isDeleted()), cxxDestructorDecl()).bind("decl"),
      this);
}

void NoexceptDestructorCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *FuncDecl = Result.Nodes.getNodeAs<FunctionDecl>("decl");
  assert(FuncDecl);

  if (SpecAnalyzer.analyze(FuncDecl) !=
      utils::ExceptionSpecAnalyzer::State::Throwing)
    return;

  // Don't complain about nothrow(false), but complain on nothrow(expr)
  // where expr evaluates to false.
  const auto *ProtoType = FuncDecl->getType()->castAs<FunctionProtoType>();
  const Expr *NoexceptExpr = ProtoType->getNoexceptExpr();
  if (NoexceptExpr) {
    NoexceptExpr = NoexceptExpr->IgnoreImplicit();
    if (!isa<CXXBoolLiteralExpr>(NoexceptExpr)) {
      diag(NoexceptExpr->getExprLoc(),
           "noexcept specifier on the destructor evaluates to 'false'");
    }
    return;
  }

  auto Diag = diag(FuncDecl->getLocation(), "destructors should "
                                            "be marked noexcept");

  // Add FixIt hints.
  const SourceManager &SM = *Result.SourceManager;

  const SourceLocation NoexceptLoc =
      utils::lexer::getLocationForNoexceptSpecifier(FuncDecl, SM);
  if (NoexceptLoc.isValid())
    Diag << FixItHint::CreateInsertion(NoexceptLoc, " noexcept ");
}

} // namespace clang::tidy::performance
