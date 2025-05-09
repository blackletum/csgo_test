﻿//== AnalysisManager.h - Path sensitive analysis data manager ------*- C++ -*-//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the AnalysisManager class that manages the data and policy
// for path sensitive analysis.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_GR_ANALYSISMANAGER_H
#define LLVM_CLANG_GR_ANALYSISMANAGER_H

#include "clang/Analysis/AnalysisContext.h"
#include "clang/StaticAnalyzer/Core/AnalyzerOptions.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "clang/StaticAnalyzer/Core/BugReporter/PathDiagnostic.h"
#include "clang/StaticAnalyzer/Core/PathDiagnosticConsumers.h"

namespace clang {

namespace ento {
  class CheckerManager;

class AnalysisManager : public BugReporterData {
  virtual void anchor();
  AnalysisDeclContextManager AnaCtxMgr;

  ASTContext &Ctx;
  DiagnosticsEngine &Diags;
  const LangOptions &LangOpts;
  PathDiagnosticConsumers PathConsumers;

  // Configurable components creators.
  StoreManagerCreator CreateStoreMgr;
  ConstraintManagerCreator CreateConstraintMgr;

  CheckerManager *CheckerMgr;

public:
  AnalyzerOptions &options;
  
  AnalysisManager(ASTContext &ctx,DiagnosticsEngine &diags,
                  const LangOptions &lang,
                  const PathDiagnosticConsumers &Consumers,
                  StoreManagerCreator storemgr,
                  ConstraintManagerCreator constraintmgr, 
                  CheckerManager *checkerMgr,
                  AnalyzerOptions &Options);

  ~AnalysisManager();
  
  void ClearContexts() {
    AnaCtxMgr.clear();
  }
  
  AnalysisDeclContextManager& getAnalysisDeclContextManager() {
    return AnaCtxMgr;
  }

  StoreManagerCreator getStoreManagerCreator() {
    return CreateStoreMgr;
  }

  ConstraintManagerCreator getConstraintManagerCreator() {
    return CreateConstraintMgr;
  }

  CheckerManager *getCheckerManager() const { return CheckerMgr; }

  virtual ASTContext &getASTContext() {
    return Ctx;
  }

  virtual SourceManager &getSourceManager() {
    return getASTContext().getSourceManager();
  }

  virtual DiagnosticsEngine &getDiagnostic() {
    return Diags;
  }

  const LangOptions &getLangOpts() const {
    return LangOpts;
  }

  ArrayRef<PathDiagnosticConsumer*> getPathDiagnosticConsumers()  {
    return PathConsumers;
  }

  void FlushDiagnostics();

  bool shouldVisualize() const {
    return options.visualizeExplodedGraphWithGraphViz ||
           options.visualizeExplodedGraphWithUbiGraph;
  }

  bool shouldInlineCall() const {
    return options.getIPAMode() != IPAK_None;
  }

  CFG *getCFG(Decl const *D) {
    return AnaCtxMgr.getContext(D)->getCFG();
  }

  template <typename T>
  T *getAnalysis(Decl const *D) {
    return AnaCtxMgr.getContext(D)->getAnalysis<T>();
  }

  ParentMap &getParentMap(Decl const *D) {
    return AnaCtxMgr.getContext(D)->getParentMap();
  }

  AnalysisDeclContext *getAnalysisDeclContext(const Decl *D) {
    return AnaCtxMgr.getContext(D);
  }
};

} // enAnaCtxMgrspace

} // end clang namespace

#endif
