# Quickstart Guide

> **Note:** The source context provided does not contain explicit build scripts, installation commands, compiler requirements, or system environment variables. This guide strictly documents the module configuration parameters required to set up and run nodes and bus logic in this simulation environment based on the source code.

---

## Configuration Parameters

The module behaviors are controlled via configuration parameters (retrieved via `par(...)` in the implementation). Configure these parameters in your simulation configuration setup (e.g., `omnetpp.ini` or module parameters).

### 1. `LinNode` Module Configuration

#### General Parameters
* `isMaster` *(bool)*: Defines if the node operates as a LIN Master (`true`) or LIN Slave (`false`).
* `nodeAddress` *(int)*: Unique address identifier for the node.
* `baudRate` *(double)*: Bus communication baud rate.
* `propDelay` *(double)*: Physical propagation delay.
* `interByteTime` *(double)*: Time delay between transmitted bytes.
* `breakBits` *(int)*: Number of break bits (defaults to `13` if `< 1`).
* `delimiterBits` *(int)*: Number of delimiter bits (defaults to `1` if `< 0`).
* `inactivitySleepTime` *(double)*: Duration of bus inactivity (in seconds) before the node transitions to sleep mode.
* `wakeupTime` *(double, optional)*: Simulation time at which the node automatically schedules a wakeup pulse.

#### Event & Diagnostics Parameters
* `eventRearmPeriod` *(double)*: Period interval for re-arming event triggers.
* `eventProbability` *(double)*: Probability (`0.0` to `1.0`) of an event occurring when re-armed.

#### String Configuration Strings
* `signalConfiguration` *(string)*: Configuration string for signal parsing.
* `errorSignalConfiguration` *(string)*: Configuration string for error signal mappings.
* `frameDlcConfiguration` *(string)*: Mapping configuration for Frame Data Length Code (DLC).
* `eventFrameConfiguration` *(string)*: Event-Triggered Frame (ETF) and UCF mapping configuration.

#### Master-Specific Parameters (`isMaster = true`)
* `scheduleConfiguration` *(string)*: Primary schedule table for the LIN Master.
* `collisionScheduleConfiguration` *(string, optional)*: Collision Resolution Schedule table for handling ETF collisions.

---

### 2. `LinBusLogic` Module Configuration

* `baudRate` *(double)*: Bus baud rate.
* `propDelay` *(double)*: Propagation delay of the bus network.
* `interByteTime` *(double)*: Inter-byte delay on the bus.
* `frameErrorRate` *(double)*: Probability/rate of frame corruption errors on the bus.
* `frameDropRate` *(double)*: Probability/rate of frame drop events on the bus.

---

## Setup & Execution Notice

Because no build files (`Makefile`, `CMakeLists.txt`), installation scripts, or system environment setup instructions were provided in the extracted snippets, refer to your local simulation framework documentation (OMNeT++) to build and run the executable using the parameter definitions listed above.