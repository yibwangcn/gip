# Scheduler Architecture

## Goal

The scheduler is split into three layers so that request routing, waiting-set organization, and execution do not get mixed into one class.

## Layer 1: Policy Selector

Files:

- include/scheduler/PolicySelector.hpp

Responsibility:

- Choose which scheduling strategy a new request should enter.
- Use both model static configuration and current runtime snapshot.

Typical inputs:

- model preferred strategy
- max batch size
- queue limits
- worker availability
- inflight request count

Non-responsibility:

- Reordering tasks inside a specific strategy
- Executing inference

## Layer 2: Strategy

Files:

- include/scheduler/strategies/IStrategy.hpp
- include/scheduler/strategies/StrategyFIFO.hpp

Responsibility:

- Maintain the waiting set for one scheduling mode.
- Decide how pending tasks are organized and when they can form a dispatch unit.

Examples:

- FIFO: first in, first out
- Priority: reorder by request priority or deadline
- Batch: group compatible requests into one dispatch unit

Interface meaning:

- admit: accept a new task into this strategy
- select: emit a ready DispatchUnit when the strategy decides work is ready
- revoke: remove a task that has not started execution yet

## Layer 3: Scheduler Executor

Files:

- include/scheduler/Scheduler.hpp
- src/scheduler/Scheduler.cpp

Responsibility:

- Drive strategy polling
- Send ready dispatch units to the worker pool
- Execute inference against backends
- Update operation state in the repository

Non-responsibility:

- Choosing task order inside a strategy
- Deciding cross-strategy routing policy

## Why DispatchUnit Exists

DispatchUnit is the boundary between strategy and execution.

It allows one strategy to return:

- a single task for FIFO
- multiple tasks for future batch execution

That keeps strategy extensible without forcing the executor API to change later.

## Current Status

Implemented:

- DefaultPolicySelector
- FIFO strategy
- Scheduler execution flow for DispatchUnit

Reserved for future extension:

- priority-based strategy
- batching strategy
- more load-aware policy selection
