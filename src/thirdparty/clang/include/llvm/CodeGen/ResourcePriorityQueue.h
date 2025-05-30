﻿//===----- ResourcePriorityQueue.h - A DFA-oriented priority queue -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the ResourcePriorityQueue class, which is a
// SchedulingPriorityQueue that schedules using DFA state to
// reduce the length of the critical path through the basic block
// on VLIW platforms.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_RESOURCEPRIORITYQUEUE_H
#define LLVM_CODEGEN_RESOURCEPRIORITYQUEUE_H

#include "llvm/CodeGen/DFAPacketizer.h"
#include "llvm/CodeGen/ScheduleDAG.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/MC/MCInstrItineraries.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetRegisterInfo.h"

namespace llvm {
  class ResourcePriorityQueue;

  /// Sorting functions for the Available queue.
  struct resource_sort : public std::binary_function<SUnit*, SUnit*, bool> {
    ResourcePriorityQueue *PQ;
    explicit resource_sort(ResourcePriorityQueue *pq) : PQ(pq) {}

    bool operator()(const SUnit* left, const SUnit* right) const;
  };

  class ResourcePriorityQueue : public SchedulingPriorityQueue {
    /// SUnits - The SUnits for the current graph.
    std::vector<SUnit> *SUnits;

    /// NumNodesSolelyBlocking - This vector contains, for every node in the
    /// Queue, the number of nodes that the node is the sole unscheduled
    /// predecessor for.  This is used as a tie-breaker heuristic for better
    /// mobility.
    std::vector<unsigned> NumNodesSolelyBlocking;

    /// Queue - The queue.
    std::vector<SUnit*> Queue;

    /// RegPressure - Tracking current reg pressure per register class.
    ///
    std::vector<unsigned> RegPressure;

    /// RegLimit - Tracking the number of allocatable registers per register
    /// class.
    std::vector<unsigned> RegLimit;

    resource_sort Picker;
    const TargetRegisterInfo *TRI;
    const TargetLowering *TLI;
    const TargetInstrInfo *TII;
    const InstrItineraryData* InstrItins;
    /// ResourcesModel - Represents VLIW state.
    /// Not limited to VLIW targets per say, but assumes
    /// definition of DFA by a target.
    DFAPacketizer *ResourcesModel;

    /// Resource model - packet/bundle model. Purely
    /// internal at the time.
    std::vector<SUnit*> Packet;

    /// Heuristics for estimating register pressure.
    unsigned ParallelLiveRanges;
    signed HorizontalVerticalBalance;

  public:
    ResourcePriorityQueue(SelectionDAGISel *IS);

    ~ResourcePriorityQueue() {
      delete ResourcesModel;
    }

    bool isBottomUp() const { return false; }

    void initNodes(std::vector<SUnit> &sunits);

    void addNode(const SUnit *SU) {
      NumNodesSolelyBlocking.resize(SUnits->size(), 0);
    }

    void updateNode(const SUnit *SU) {}

    void releaseState() {
      SUnits = 0;
    }

    unsigned getLatency(unsigned NodeNum) const {
      assert(NodeNum < (*SUnits).size());
      return (*SUnits)[NodeNum].getHeight();
    }

    unsigned getNumSolelyBlockNodes(unsigned NodeNum) const {
      assert(NodeNum < NumNodesSolelyBlocking.size());
      return NumNodesSolelyBlocking[NodeNum];
    }

    /// Single cost function reflecting benefit of scheduling SU
    /// in the current cycle.
    signed SUSchedulingCost (SUnit *SU);

    /// InitNumRegDefsLeft - Determine the # of regs defined by this node.
    ///
    void initNumRegDefsLeft(SUnit *SU);
    void updateNumRegDefsLeft(SUnit *SU);
    signed regPressureDelta(SUnit *SU, bool RawPressure = false);
    signed rawRegPressureDelta (SUnit *SU, unsigned RCId);

    bool empty() const { return Queue.empty(); }

    virtual void push(SUnit *U);

    virtual SUnit *pop();

    virtual void remove(SUnit *SU);

    virtual void dump(ScheduleDAG* DAG) const;

    /// scheduledNode - Main resource tracking point.
    void scheduledNode(SUnit *Node);
    bool isResourceAvailable(SUnit *SU);
    void reserveResources(SUnit *SU);

private:
    void adjustPriorityOfUnscheduledPreds(SUnit *SU);
    SUnit *getSingleUnscheduledPred(SUnit *SU);
    unsigned numberRCValPredInSU (SUnit *SU, unsigned RCId);
    unsigned numberRCValSuccInSU (SUnit *SU, unsigned RCId);
  };
}

#endif
