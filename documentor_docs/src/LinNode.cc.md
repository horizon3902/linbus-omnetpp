# Technical Documentation: `src/LinNode.cc`

## Overview

The `LinNode.cc` source file defines the primary simulation module for a Local Interconnect Network (LIN) node within an OMNeT++ network simulation environment. The module implements the LIN protocol state machine, supporting both **Master** and **Slave** operational roles, schedule table execution, Event-Triggered Frames (ETF) with Unconditional Frames (UCF) collision resolution, sleep/wakeup power management, error tracking, and signal/DLC parsing.

---

## Key Responsibilities

1. **Node Role Initialization**: Configures node parameters, parses configurations for signals, schedules, DLCs, and event-triggered frames, and sets node roles (Master vs. Slave).
2. **Schedule Management (Master)**: Executes normal schedule tables and switches to collision resolution schedules when collisions are detected on Event-Triggered Frames.
3. **LIN Frame Transmission & Processing**: Handles break signals, headers, responses, and checksum verification for LIN bus communication.
4. **Event-Triggered Frame Handling**: Maps ETF frame IDs to associated UCF IDs, tracks event pending states, and builds/transmits event responses carrying UCF PIDs and payloads.
5. **Sleep & Wakeup Logic**: Controls node state transitions between active operation and `SLEEP` mode triggered by master commands, inactivity timeouts, or wakeup pulses.
6. **Error Bit Injection & Tracking**: Tracks checksum, header, and timeout errors, and applies configured error bits to outgoing frame payloads.

---

## Module Timers (`cMessage`)

The module manages several internal timers created during `initialize()` and deleted during `finish()`:

| Timer Name | Purpose |
| :--- | :--- |
| `scheduleTimer` | Triggers the next slot in the active LIN schedule table (Master only). |
| `headerTimer` | Transmits the remainder of the LIN header after the break signal and inter-byte delay. |
| `responseTimeoutTimer` | Detects response timeouts for Master nodes expecting a Slave response. |
| `wakeupTimer` | Triggers a wakeup pulse transmission sequence. |
| `wakeupExpirationTimer` | Monitors timeouts for sent wakeup pulses and manages retries (up to 3 attempts). |
| `goToSleepTimer` | Handles local completion of the Go-To-Sleep command on Master nodes. |
| `eventRearmTimer` | Periodically sets pending UCF event flags based on a configured probability (`eventProbability`). |
| `inactivityTimer` | Monitors bus inactivity and transitions the node to `SLEEP` mode when expired. |

---

## Core Operations & Functions

### 1. Initialization and Lifecycle

#### `initialize()`
* Parses module parameters (`isMaster`, `nodeAddress`, `baudRate`, `propDelay`, `interByteTime`, `breakBits`, `delimiterBits`).
* Sets initial state to `IDLE` and enables schedule execution (`scheduleRunning = true`).
* Initializes diagnostics context (`diagCtx`) role (`DIAG_MASTER` or `DIAG_SLAVE`).
* Instantiates all eight `cMessage` timers.
* Resets receiving contexts and inactivity timers.
* Calls parsing routines for signals, error signals, DLC maps, and ETF configurations.
* Schedules initial events:
  * `eventRearmTimer` if `eventRearmPeriod >= 0`.
  * `wakeupTimer` if parameter `wakeupTime` exists and is `>= 0`.
  * `scheduleTimer` for Master nodes based on the first slot in `scheduleTable`.

#### `finish()`
* Safely cancels and deletes all instantiated `cMessage` timers to prevent memory leaks upon simulation termination.

---

### 2. Configuration Parsing Functions

* **`parseFrameDlcs(const char* config)`**: Parses semicolon-delimited string key-value pairs (`"ID=DLC;"` in hex/dec format) into the `frameIdToDlc` map. DLCs are bounded between 0 and 8 bytes.
* **`parseEventFrames(const char* config)`**: Parses ETF mappings formatted as `"ETF_ID:UCF_ID1,UCF_ID2;"` into `etfToUcfs`.
* **`parseSchedule(const char* config, std::vector<LinSlot>& table)`**: Parses frame schedule tables formatted as `"FrameId:Publisher:Period;"`. Handles ETF markers indicated by `"(E)"`.
* **`parseSignals(const char* config)`**: Parses signal mapping strings (`"FrameId:ByteOffset:Value;"`) into `signalTable`.
* **`parseErrorSignals(const char* config)`**: Parses error signal bindings (`"FrameId:ByteOffset:BitIndex;"`) into `errorBindings`.

---

### 3. LIN Timing & Calculation Helpers

* **`headerRemainderTxTime() const`**: Returns the transmission duration for the remainder of the LIN header:
  $$\text{TxTime} = \frac{2 \times 10}{\text{baudRate}}$$
* **`responseTxTime(int dataLength) const`**: Bounds `dataLength` between 0 and 8 bytes and calculates response transmission duration:
  $$\text{TxTime} = \frac{(\text{dataLength} + 1) \times 10}{\text{baudRate}}$$
* **`dlcForFrame(int frameId) const`**: Queries `frameIdToDlc` for the specified `frameId`. Defaults to 8 bytes if not found.

---

### 4. Sleep & Wakeup Management

* **`resetInactivityTimer()`**: Reschedules `inactivityTimer` to `simTime() + inactivitySleepTime` if `inactivitySleepTime >= 0` and the node is not in `SLEEP` mode.
* **`enterSleep(const char *reason)`**: 
  * Sets `state = SLEEP`.
  * Cancels pending response timeouts, inactivity timers, and master schedule timers.
  * Resets reception context.
* **`sendWakeupPulse()`**: Sends a `LinBreakSignal` with a duration of $30 / \text{baudRate}$ seconds.
* **`isWakeupPulse(LinBreakSignal *brk)`**: Evaluates if a break signal duration exceeds 20 bit times ($> 20 / \text{baudRate}$).
* **`isGoToSleepCommand(const LinResponse *rsp) const`**: Checks if a response matches frame ID `0x3C` with byte 0 set to `0x00` and all subsequent bytes set to `0xFF`.

---

### 5. Event-Triggered Frames (ETF) & Unconditional Frames (UCF)

* **`initializeEventFlags()`**: Sets `ucfEventPending` flags to `true` for all frame IDs published in `signalTable`.
* **`isEventFrameId(int etfId) const`**: Checks if `etfId` is present in the `etfToUcfs` map.
* **`selectUcfForEventFrame(int etfId, int& outUcfId) const`**: Scans UCF candidates associated with `etfId`. Returns `true` and assigns `outUcfId` if a candidate is published by this node and has a pending event flag.
* **`setEventPending(int ucfId, bool pending)`**: Updates the pending status in `ucfEventPending`.

---

### 6. Main Handler: `handleMessage(cMessage *msg)`

`handleMessage` processes both internal self-messages (timers) and incoming LIN network traffic. Any received network traffic triggers `resetInactivityTimer()`.

#### Message Processing Logic:

```
                          cMessage Received
                                  │
     ┌────────────────────────────┼────────────────────────────┐
     ▼                            ▼                            ▼
Timer Messages             Node in SLEEP State         Active Bus Traffic
     │                            │                            │
     ├─ inactivityTimer           └─ LinBreakSignal            ├─ LinBreakSignal
     │  └─ enterSleep()              └─ If Wakeup Pulse:       │  ├─ Wakeup Pulse -> IDLE
     ├─ eventRearmTimer                 State -> IDLE          │  └─ Normal -> WAIT_FOR_SYNC
     │  └─ Randomize UCF flags          (Master resumes)       ├─ LinHeader
     ├─ goToSleepTimer                                         │  ├─ Validate PID
     │  └─ enterSleep()                                        │  └─ processReceivedHeader()
     ├─ scheduleTimer (Master)                                 └─ LinResponse
     │  ├─ Select normal/collision table                          ├─ Validate Checksum/Collisions
     │  ├─ sendBreakSignal()                                      ├─ Trigger ETF Collision Res.
     │  └─ Schedule headerTimer & next scheduleTimer              └─ Check GoToSleep Command
     ├─ headerTimer
     │  ├─ sendHeaderRemainder()
     │  └─ Arm responseTimeoutTimer (if Master listener)
     ├─ responseTimeoutTimer -> Log timeout error
     ├─ wakeupTimer -> sendWakeupPulse()
     └─ wakeupExpirationTimer -> Retry or fail wakeup
```

---

### 7. Frame Processing & Transmission

#### `sendBreakSignal()`
Constructs and transmits a `LinBreakSignal` with duration calculated from `breakBits / baudRate`.

#### `sendHeaderRemainder(int frameId, std::string publisher)`
Constructs and transmits a `LinHeader` containing the frame ID, computed Protected Identifier (PID via `computePID`), and sync byte `0x55`. If the node publishes the non-ETF frame ID, it schedules `sendResponse` after a header remainder delay and inter-byte time.

#### `processReceivedHeader(LinHeader *header)`
* Sets state to `WAIT_FOR_PID`.
* **ETF Frame**: Calls `selectUcfForEventFrame()`. If a UCF is pending, transmits via `sendEventResponse()`. Otherwise, sets state to `RECEIVING`.
* **Standard Frame**: If the node publishes the frame ID, transmits via `sendResponse()`. Otherwise, sets state to `RECEIVING`.

#### `sendResponse(int frameId, double initialDelay)`
* Creates a `LinResponse` message with payload size retrieved from `dlcForFrame()`.
* Fills payload bytes using matching entries in `signalTable` (default byte value `0xFF`).
* Injects error bits via `applyErrorBits()`.
* Computes and attaches checksum via `computeChecksum()`.
* Transmits response using `sendDelayed()`.
* If the master node transmits a Go-To-Sleep command, it schedules `goToSleepTimer`.

#### `sendEventResponse(int etfId, int ucfId, double initialDelay)`
* Creates a `LinResponse` message for the ETF ID.
* Byte 0 of payload is populated with the **PID of the UCF ID**.
* UCF payload signals are placed into the payload starting at byte offset + 1.
* Computes checksum over ETF PID and payload, then sends frame via `sendDelayed()`.

#### `applyErrorBits(int logicalFrameId, LinResponse *rsp, int payloadBase)`
If `lastChecksumError`, `lastHeaderError`, or `lastTimeoutError` is flag-set:
* Iterates through `errorBindings` matching `logicalFrameId`.
* Sets specified bit at `byteOffset + payloadBase` in the response payload.
* Resets error state flags (`lastChecksumError`, `lastHeaderError`, `lastTimeoutError`).

#### `startCollisionResolution()`
Triggers when a collision or checksum error is detected on an Event-Triggered Frame slot:
* Sets `resolvingCollision = true`.
* Resets `collisionSlotIndex = 0`.
* Cancels `scheduleTimer` and schedules immediate collision resolution schedule execution (5ms delay).