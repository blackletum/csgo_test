﻿//==- CoreEngine.h - Path-Sensitive Dataflow Engine ----------------*- C++ -*-//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines a generic engine for intraprocedural, path-sensitive,
//  dataflow analysis via graph reachability.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_GR_COREENGINE
#define LLVM_CLANG_GR_COREENGINE

#include "clang/AST/Expr.h"
#include "clang/Analysis/AnalysisContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/BlockCounter.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ExplodedGraph.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/FunctionSummary.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/WorkList.h"
#include "llvm/ADT/OwningPtr.h"

namespace clang {

class ProgramPointTag;
  
namespace ento {

class NodeBuilder;

//===----------------------------------------------------------------------===//
/// CoreEngine - Implements the core logic of the graph-reachability
///   analysis. It traverses the CFG and generates the ExplodedGraph.
///   Program "states" are treated as opaque void pointers.
///   The template class CoreEngine (which subclasses CoreEngine)
///   provides the matching component to the engine that knows the actual types
///   for states.  Note that this engine only dispatches to transfer functions
///   at the statement and block-level.  The analyses themselves must implement
///   any transfer function logic and the sub-expression level (if any).
class CoreEngine {
  friend struct NodeBuilderContext;
  friend class NodeBuilder;
  friend class ExprEngine;
  friend class CommonNodeBuilder;
  friend class IndirectGotoNodeBuilder;
  friend class SwitchNodeBuilder;
  friend class EndOfFunctionNodeBuilder;
public:
  typedef std::vector<std::pair<BlockEdge, const ExplodedNode*> >
            BlocksExhausted;
  
  typedef std::vector<std::pair<const CFGBlock*, const ExplodedNode*> >
            BlocksAborted;

private:

  SubEngine& SubEng;

  /// G - The simulation graph.  Each node is a (location,state) pair.
  OwningPtr<ExplodedGraph> G;

  /// WList - A set of queued nodes that need to be processed by the
  ///  worklist algorithm.  It is up to the implementation of WList to decide
  ///  the order that nodes are processed.
  OwningPtr<WorkList> WList;

  /// BCounterFactory - A factory object for created BlockCounter objects.
  ///   These are used to record for key nodes in the ExplodedGraph the
  ///   number of times different CFGBlocks have been visited along a path.
  BlockCounter::Factory BCounterFactory;

  /// The locations where we stopped doing work because we visited a location
  ///  too many times.
  BlocksExhausted blocksExhausted;
  
  /// The locations where we stopped because the engine aborted analysis,
  /// usually because it could not reason about something.
  BlocksAborted blocksAborted;

  /// The information about functions shared by the whole translation unit.
  /// (This data is owned by AnalysisConsumer.)
  FunctionSummariesTy *FunctionSummaries;

  void generateNode(const ProgramPoint &Loc,
                    ProgramStateRef State,
                    ExplodedNode *Pred);

  void HandleBlockEdge(const BlockEdge &E, ExplodedNode *Pred);
  void HandleBlockEntrance(const BlockEntrance &E, ExplodedNode *Pred);
  void HandleBlockExit(const CFGBlock *B, ExplodedNode *Pred);
  void HandlePostStmt(const CFGBlock *B, unsigned StmtIdx, ExplodedNode *Pred);

  void HandleBranch(const Stmt *Cond, const Stmt *Term, const CFGBlock *B,
                    ExplodedNode *Pred);

  /// Handle conditional logic for running static initializers.
  void HandleStaticInit(const DeclStmt *DS, const CFGBlock *B,
                        ExplodedNode *Pred);

private:
  CoreEngine(const CoreEngine &) LLVM_DELETED_FUNCTION;
  void operator=(const CoreEngine &) LLVM_DELETED_FUNCTION;

  ExplodedNode *generateCallExitBeginNode(ExplodedNode *N);

public:
  /// Construct a CoreEngine object to analyze the provided CFG.
  CoreEngine(SubEngine& subengine,
             FunctionSummariesTy *FS)
    : SubEng(subengine), G(new ExplodedGraph()),
      WList(WorkList::makeDFS()),
      BCounterFactory(G->getAllocator()),
      FunctionSummaries(FS){}

  /// getGraph - Returns the exploded graph.
  ExplodedGraph& getGraph() { return *G.get(); }

  /// takeGraph - Returns the exploded graph.  Ownership of the graph is
  ///  transferred to the caller.
  ExplodedGraph* takeGraph() { return G.take(); }

  /// ExecuteWorkList - Run the worklist algorithm for a maximum number of
  ///  steps.  Returns true if there is still simulation state on the worklist.
  bool ExecuteWorkList(const LocationContext *L, unsigned Steps,
                       ProgramStateRef InitState);
  /// Returns true if there is still simulation state on the worklist.
  bool ExecuteWorkListWithInitialState(const LocationContext *L,
                                       unsigned Steps,
                                       ProgramStateRef InitState, 
                                       ExplodedNodeSet &Dst);

  /// Dispatch the work list item based on the given location information.
  /// Use Pred parameter as the predecessor state.
  void dispatchWorkItem(ExplodedNode* Pred, ProgramPoint Loc,
                        const WorkListUnit& WU);

  // Functions for external checking of whether we have unfinished work
  bool wasBlockAborted() const { return !blocksAborted.empty(); }
  bool wasBlocksExhausted() const { return !blocksExhausted.empty(); }
  bool hasWorkRemaining() const { return wasBlocksExhausted() || 
                                         WList->hasWork() || 
                                         wasBlockAborted(); }

  /// Inform the CoreEngine that a basic block was aborted because
  /// it could not be completely analyzed.
  void addAbortedBlock(const ExplodedNode *node, const CFGBlock *block) {
    blocksAborted.push_back(std::make_pair(block, node));
  }
  
  WorkList *getWorkList() const { return WList.get(); }

  BlocksExhausted::const_iterator blocks_exhausted_begin() const {
    return blocksExhausted.begin();
  }
  BlocksExhausted::const_iterator blocks_exhausted_end() const {
    return blocksExhausted.end();
  }
  BlocksAborted::const_iterator blocks_aborted_begin() const {
    return blocksAborted.begin();
  }
  BlocksAborted::const_iterator blocks_aborted_end() const {
    return blocksAborted.end();
  }

  /// \brief Enqueue the given set of nodes onto the work list.
  void enqueue(ExplodedNodeSet &Set);

  /// \brief Enqueue nodes that were created as a result of processing
  /// a statement onto the work list.
  void enqueue(ExplodedNodeSet &Set, const CFGBlock *Block, unsigned Idx);

  /// \brief enqueue the nodes corresponding to the end of function onto the
  /// end of path / work list.
  void enqueueEndOfFunction(ExplodedNodeSet &Set);

  /// \brief Enqueue a single node created as a result of statement processing.
  void enqueueStmtNode(ExplodedNode *N, const CFGBlock *Block, unsigned Idx);
};

// TODO: Turn into a calss.
struct NodeBuilderContext {
  const CoreEngine &Eng;
  const CFGBlock *Block;
  const LocationContext *LC;
  NodeBuilderContext(const CoreEngine &E, const CFGBlock *B, ExplodedNode *N)
    : Eng(E), Block(B), LC(N->getLocationContext()) { assert(B); }

  /// \brief Return the CFGBlock associated with this builder.
  const CFGBlock *getBlock() const { return Block; }

  /// \brief Returns the number of times the current basic block has been
  /// visited on the exploded graph path.
  unsigned blockCount() const {
    return Eng.WList->getBlockCounter().getNumVisited(
                    LC->getCurrentStackFrame(),
                    Block->getBlockID());
  }
};

/// \class NodeBuilder
/// \brief This is the simplest builder which generates nodes in the
/// ExplodedGraph.
///
/// The main benefit of the builder is that it automatically tracks the
/// frontier nodes (or destination set). This is the set of nodes which should
/// be propagated to the next step / builder. They are the nodes which have been
/// added to the builder (either as the input node set or as the newly
/// constructed nodes) but did not have any outgoing transitions added.
class NodeBuilder {
  virtual void anchor();
protected:
  const NodeBuilderContext &C;

  /// Specifies if the builder results have been finalized. For example, if it
  /// is set to false, autotransitions are yet to be generated.
  bool Finalized;
  bool HasGeneratedNodes;
  /// \brief The frontier set - a set of nodes which need to be propagated after
  /// the builder dies.
  ExplodedNodeSet &Frontier;

  /// Checkes if the results are ready.
  virtual bool checkResults() {
    if (!Finalized)
      return false;
    return true;
  }

  bool hasNoSinksInFrontier() {
    for (iterator I = Frontier.begin(), E = Frontier.end(); I != E; ++I) {
      if ((*I)->isSink())
        return false;
    }
    return true;
  }

  /// Allow subclasses to finalize results before result_begin() is executed.
  virtual void finalizeResults() {}
  
  ExplodedNode *generateNodeImpl(const ProgramPoint &PP,
                                 ProgramStateRef State,
                                 ExplodedNode *Pred,
                                 bool MarkAsSink = false);

public:
  NodeBuilder(ExplodedNode *SrcNode, ExplodedNodeSet &DstSet,
              const NodeBuilderContext &Ctx, bool F = true)
    : C(Ctx), Finalized(F), HasGeneratedNodes(false), Frontier(DstSet) {
    Frontier.Add(SrcNode);
  }

  NodeBuilder(const ExplodedNodeSet &SrcSet, ExplodedNodeSet &DstSet,
              const NodeBuilderContext &Ctx, bool F = true)
    : C(Ctx), Finalized(F), HasGeneratedNodes(false), Frontier(DstSet) {
    Frontier.insert(SrcSet);
    assert(hasNoSinksInFrontier());
  }

  virtual ~NodeBuilder() {}

  /// \brief Generates a node in the ExplodedGraph.
  ExplodedNode *generateNode(const ProgramPoint &PP,
                             ProgramStateRef State,
                             ExplodedNode *Pred) {
    return generateNodeImpl(PP, State, Pred, false);
  }

  /// \brief Generates a sink in the ExplodedGraph.
  ///
  /// When a node is marked as sink, the exploration from the node is stopped -
  /// the node becomes the last node on the path and certain kinds of bugs are
  /// suppressed.
  ExplodedNode *generateSink(const ProgramPoint &PP,
                             ProgramStateRef State,
                             ExplodedNode *Pred) {
    return generateNodeImpl(PP, State, Pred, true);
  }

  const ExplodedNodeSet &getResults() {
    finalizeResults();
    assert(checkResults());
    return Frontier;
  }

  typedef ExplodedNodeSet::iterator iterator;
  /// \brief Iterators through the results frontier.
  inline iterator begin() {
    finalizeResults();
    assert(checkResults());
    return Frontier.begin();
  }
  inline iterator end() {
    finalizeResults();
    return Frontier.end();
  }

  const NodeBuilderContext &getContext() { return C; }
  bool hasGeneratedNodes() { return HasGeneratedNodes; }

  void takeNodes(const ExplodedNodeSet &S) {
    for (ExplodedNodeSet::iterator I = S.begin(), E = S.end(); I != E; ++I )
      Frontier.erase(*I);
  }
  void takeNodes(ExplodedNode *N) { Frontier.erase(N); }
  void addNodes(const ExplodedNodeSet &S) { Frontier.insert(S); }
  void addNodes(ExplodedNode *N) { Frontier.Add(N); }
};

/// \class NodeBuilderWithSinks
/// \brief This node builder keeps track of the generated sink nodes.
class NodeBuilderWithSinks: public NodeBuilder {
  virtual void anchor();
protected:
  SmallVector<ExplodedNode*, 2> sinksGenerated;
  ProgramPoint &Location;

public:
  NodeBuilderWithSinks(ExplodedNode *Pred, ExplodedNodeSet &DstSet,
                       const NodeBuilderContext &Ctx, ProgramPoint &L)
    : NodeBuilder(Pred, DstSet, Ctx), Location(L) {}

  ExplodedNode *generateNode(ProgramStateRef State,
                             ExplodedNode *Pred,
                             const ProgramPointTag *Tag = 0) {
    const ProgramPoint &LocalLoc = (Tag ? Location.withTag(Tag) : Location);
    return NodeBuilder::generateNode(LocalLoc, State, Pred);
  }

  ExplodedNode *generateSink(ProgramStateRef State, ExplodedNode *Pred,
                             const ProgramPointTag *Tag = 0) {
    const ProgramPoint &LocalLoc = (Tag ? Location.withTag(Tag) : Location);
    ExplodedNode *N = NodeBuilder::generateSink(LocalLoc, State, Pred);
    if (N && N->isSink())
      sinksGenerated.push_back(N);
    return N;
  }

  const SmallVectorImpl<ExplodedNode*> &getSinks() const {
    return sinksGenerated;
  }
};

/// \class StmtNodeBuilder
/// \brief This builder class is useful for generating nodes that resulted from
/// visiting a statement. The main difference from its parent NodeBuilder is
/// that it creates a statement specific ProgramPoint.
class StmtNodeBuilder: public NodeBuilder {
  NodeBuilder *EnclosingBldr;
public:

  /// \brief Constructs a StmtNodeBuilder. If the builder is going to process
  /// nodes currently owned by another builder(with larger scope), use
  /// Enclosing builder to transfer ownership.
  StmtNodeBuilder(ExplodedNode *SrcNode, ExplodedNodeSet &DstSet,
                      const NodeBuilderContext &Ctx, NodeBuilder *Enclosing = 0)
    : NodeBuilder(SrcNode, DstSet, Ctx), EnclosingBldr(Enclosing) {
    if (EnclosingBldr)
      EnclosingBldr->takeNodes(SrcNode);
  }

  StmtNodeBuilder(ExplodedNodeSet &SrcSet, ExplodedNodeSet &DstSet,
                      const NodeBuilderContext &Ctx, NodeBuilder *Enclosing = 0)
    : NodeBuilder(SrcSet, DstSet, Ctx), EnclosingBldr(Enclosing) {
    if (EnclosingBldr)
      for (ExplodedNodeSet::iterator I = SrcSet.begin(),
                                     E = SrcSet.end(); I != E; ++I )
        EnclosingBldr->takeNodes(*I);
  }

  virtual ~StmtNodeBuilder();

  using NodeBuilder::generateNode;
  using NodeBuilder::generateSink;

  ExplodedNode *generateNode(const Stmt *S,
                             ExplodedNode *Pred,
                             ProgramStateRef St,
                             const ProgramPointTag *tag = 0,
                             ProgramPoint::Kind K = ProgramPoint::PostStmtKind){
    const ProgramPoint &L = ProgramPoint::getProgramPoint(S, K,
                                  Pred->getLocationContext(), tag);
    return NodeBuilder::generateNode(L, St, Pred);
  }

  ExplodedNode *generateSink(const Stmt *S,
                             ExplodedNode *Pred,
                             ProgramStateRef St,
                             const ProgramPointTag *tag = 0,
                             ProgramPoint::Kind K = ProgramPoint::PostStmtKind){
    const ProgramPoint &L = ProgramPoint::getProgramPoint(S, K,
                                  Pred->getLocationContext(), tag);
    return NodeBuilder::generateSink(L, St, Pred);
  }
};

/// \brief BranchNodeBuilder is responsible for constructing the nodes
/// corresponding to the two branches of the if statement - true and false.
class BranchNodeBuilder: public NodeBuilder {
  virtual void anchor();
  const CFGBlock *DstT;
  const CFGBlock *DstF;

  bool InFeasibleTrue;
  bool InFeasibleFalse;

public:
  BranchNodeBuilder(ExplodedNode *SrcNode, ExplodedNodeSet &DstSet,
                    const NodeBuilderContext &C,
                    const CFGBlock *dstT, const CFGBlock *dstF)
  : NodeBuilder(SrcNode, DstSet, C), DstT(dstT), DstF(dstF),
    InFeasibleTrue(!DstT), InFeasibleFalse(!DstF) {
    // The branch node builder does not generate autotransitions.
    // If there are no successors it means that both branches are infeasible.
    takeNodes(SrcNode);
  }

  BranchNodeBuilder(const ExplodedNodeSet &SrcSet, ExplodedNodeSet &DstSet,
                    const NodeBuilderContext &C,
                    const CFGBlock *dstT, const CFGBlock *dstF)
  : NodeBuilder(SrcSet, DstSet, C), DstT(dstT), DstF(dstF),
    InFeasibleTrue(!DstT), InFeasibleFalse(!DstF) {
    takeNodes(SrcSet);
  }

  ExplodedNode *generateNode(ProgramStateRef State, bool branch,
                             ExplodedNode *Pred);

  const CFGBlock *getTargetBlock(bool branch) const {
    return branch ? DstT : DstF;
  }

  void markInfeasible(bool branch) {
    if (branch)
      InFeasibleTrue = true;
    else
      InFeasibleFalse = true;
  }

  bool isFeasible(bool branch) {
    return branch ? !InFeasibleTrue : !InFeasibleFalse;
  }
};

class IndirectGotoNodeBuilder {
  CoreEngine& Eng;
  const CFGBlock *Src;
  const CFGBlock &DispatchBlock;
  const Expr *E;
  ExplodedNode *Pred;

public:
  IndirectGotoNodeBuilder(ExplodedNode *pred, const CFGBlock *src, 
                    const Expr *e, const CFGBlock *dispatch, CoreEngine* eng)
    : Eng(*eng), Src(src), DispatchBlock(*dispatch), E(e), Pred(pred) {}

  class iterator {
    CFGBlock::const_succ_iterator I;

    friend class IndirectGotoNodeBuilder;
    iterator(CFGBlock::const_succ_iterator i) : I(i) {}
  public:

    iterator &operator++() { ++I; return *this; }
    bool operator!=(const iterator &X) const { return I != X.I; }

    const LabelDecl *getLabel() const {
      return cast<LabelStmt>((*I)->getLabel())->getDecl();
    }

    const CFGBlock *getBlock() const {
      return *I;
    }
  };

  iterator begin() { return iterator(DispatchBlock.succ_begin()); }
  iterator end() { return iterator(DispatchBlock.succ_end()); }

  ExplodedNode *generateNode(const iterator &I,
                             ProgramStateRef State,
                             bool isSink = false);

  const Expr *getTarget() const { return E; }

  ProgramStateRef getState() const { return Pred->State; }
  
  const LocationContext *getLocationContext() const {
    return Pred->getLocationContext();
  }
};

class SwitchNodeBuilder {
  CoreEngine& Eng;
  const CFGBlock *Src;
  const Expr *Condition;
  ExplodedNode *Pred;

public:
  SwitchNodeBuilder(ExplodedNode *pred, const CFGBlock *src,
                    const Expr *condition, CoreEngine* eng)
  : Eng(*eng), Src(src), Condition(condition), Pred(pred) {}

  class iterator {
    CFGBlock::const_succ_reverse_iterator I;

    friend class SwitchNodeBuilder;
    iterator(CFGBlock::const_succ_reverse_iterator i) : I(i) {}

  public:
    iterator &operator++() { ++I; return *this; }
    bool operator!=(const iterator &X) const { return I != X.I; }
    bool operator==(const iterator &X) const { return I == X.I; }

    const CaseStmt *getCase() const {
      return cast<CaseStmt>((*I)->getLabel());
    }

    const CFGBlock *getBlock() const {
      return *I;
    }
  };

  iterator begin() { return iterator(Src->succ_rbegin()+1); }
  iterator end() { return iterator(Src->succ_rend()); }

  const SwitchStmt *getSwitch() const {
    return cast<SwitchStmt>(Src->getTerminator());
  }

  ExplodedNode *generateCaseStmtNode(const iterator &I,
                                     ProgramStateRef State);

  ExplodedNode *generateDefaultCaseNode(ProgramStateRef State,
                                        bool isSink = false);

  const Expr *getCondition() const { return Condition; }

  ProgramStateRef getState() const { return Pred->State; }
  
  const LocationContext *getLocationContext() const {
    return Pred->getLocationContext();
  }
};

} // end ento namespace
} // end clang namespace

#endif
