# Technical Documentation: `src/LinNode.h`

## Overview

The `src/LinNode.h` header file defines the `LinNode` C++ class—an OMNeT++ simple module (`cSimpleModule`) representing a Local Interconnect Network (LIN) bus node. It supports master and slave operations, scheduling, frame transmission and reception, event-triggered frame (ETF) resolving, PHY timing parameters, error handling/binding, and an infrastructure for LIN diagnostics.

---

## Preprocessor Directives & Included Headers

- **Include Guards**: `#ifndef LINNODE_H_` / `#define LINNODE_H_`
- **Standard Headers**: `<vector>`, `<string>`, `<unordered_map>`, `<cstdint>`
- **OMNeT++ Infrastructure**: `<omnetpp.h>`
- **Generated Frame Definitions**: `"LinDataFrame_m.h"`
- **Namespace**: Uses `namespace omnetpp`.

---

## Global Constants & Enumerations

### Diagnostic Constants
- `LIN_DIAG_MASTER_REQUEST_ID = 0x3C`: Raw ID for diagnostic master request frames.
- `LIN_DIAG_SLAVE_RESPONSE_ID = 0x3D`: Raw ID for diagnostic slave response frames.

### Enums

#### `LinNodeState`
Defines the primary operational states of the LIN node:
- `IDLE`: Node is idle, awaiting a trigger or schedule event.
- `WAIT_FOR_SYNC`: Node is awaiting a synchronization byte.
- `WAIT_FOR_PID`: Node is awaiting a Protected Identifier (PID).
- `TRANSMITTING`: Node is currently transmitting payload/data.
- `RECEIVING`: Node is currently receiving payload/data.
- `SLEEP`: Node is in low-power sleep state.

#### `LinDiagRole`
Defines the logical diagnostic role assigned to the node:
- `DIAG_NONE`: Diagnostics are disabled on this node.
- `DIAG_MASTER`: Node functions as a diagnostic tester.
- `DIAG_SLAVE`: Node functions as a diagnostic responder.

#### `LinDiagTpState`
Defines the Transport Protocol (TP) state for LIN diagnostics:
- `DIAG_TP_IDLE`: No active diagnostic transfer.
- `DIAG_TP_RX_IN_PROGRESS`: Currently receiving a multi-frame diagnostic payload.
- `DIAG_TP_TX_IN_PROGRESS`: Currently transmitting a multi-frame diagnostic payload.
- `DIAG_TP_WAIT_FC`: Waiting for a flow control frame.

---

## Data Structures

### `LinDiagContext`
Holds per-node state and buffers for diagnostic transport protocol handling.
- `bool enabled = false`: Diagnostic feature toggle.
- `LinDiagRole role = DIAG_NONE`: The node's diagnostic role.
- `LinDiagTpState tpState = DIAG_TP_IDLE`: Current transport protocol state.
- `uint8_t expectedSeq = 0`: Sequence number expected in multi-frame reception.
- `uint8_t nextTxSeq = 0`: Next sequence number to send in multi-frame transmission.
- `std::vector<uint8_t> rxBuffer`: Buffer for reassembling received diagnostic bytes.
- `uint16_t expectedLength = 0`: Total expected diagnostic message length for RX.
- `std::vector<uint8_t> txBuffer`: Payload buffer pending transmission.
- `uint16_t txOffset = 0`: Transmission offset within `txBuffer`.
- `uint8_t targetAddress = 0xFF`: Target Node Address (NAD) / logical address (0x7F is broadcast).
- `uint8_t sourceAddress = 0x00`: Source NAD / logical address.
- `simtime_t lastActivity`: Timestamp of last diagnostic activity for timeout tracking.

### `LinSlot`
Represents an entry in a LIN schedule table.
- `int frameId`: Frame ID associated with the slot.
- `std::string publisher`: Name or identifier of the node publishing the frame.
- `double period`: Slot duration or schedule period.
- `bool isETF = false`: Flag indicating whether the slot is for an Event-Triggered Frame (ETF).

### `LinSignal`
Represents a LIN signal embedded within a frame.
- `int frameId`: ID of the frame containing the signal.
- `int byteOffset`: Byte position of the signal inside the frame payload.
- `int value`: Current signal value.

### `LinErrorSignalBinding`
Maps error injection bits to positions within a frame.
- `int frameId`: Targeted frame ID.
- `int byteOffset`: Target byte offset within the frame.
- `int bitIndex`: Target bit index within the specified byte.

### `LinErrorState`
Tracks historical error conditions experienced by the node.
- `bool lastHeaderError = false`: Indicates whether the last transaction experienced a header error.
- `bool lastChecksumError = false`: Indicates whether the last transaction experienced a checksum error.
- `bool lastTimeoutError = false`: Indicates whether the last transaction experienced a timeout error.

---

## Class: `LinNode`

Inherits from `omnetpp::cSimpleModule`.

### Member Variables

#### Node Identity and Primary State
- `bool isMaster`: Configures the node as a Master (`true`) or Slave (`false`).
- `int nodeAddress`: Unique numeric address for the node.
- `LinNodeState state`: Current state of the node (`LinNodeState`).
- `LinDiagContext diagCtx`: Diagnostic transport protocol context.

#### Timers (`cMessage*`)
- `scheduleTimer`: Controls standard schedule table execution interval.
- `headerTimer`: Handles timing between sending/receiving header components.
- `responseTimeoutTimer`: Detects missing response frames on the bus.
- `wakeupTimer`: Controls timing for wakeup signal sequences.
- `wakeupExpirationTimer`: Tracks timeout for wakeup sequence responses.
- `goToSleepTimer`: Timers related to transitioning to sleep.
- `eventRearmTimer`: Timer for re-arming pending events in event-triggered logic.
- `inactivityTimer`: Tracks node inactivity to trigger sleep.
- `double inactivitySleepTime = 4.0`: Duration (in seconds) before entering sleep due to inactivity.

#### Internal State Variables
- `int pendingResponseFrameId = -1`: Frame ID currently pending response transmission.
- `bool scheduleRunning = true`: Flag indicating if the schedule table execution is active.

#### Reception (RX) Context
- `bool rxSawBreak = false`: Flag indicating a break signal was detected.
- `bool rxAwaitingResponse = false`: Flag indicating node is expecting a response payload.
- `int rxExpectedResponseId = -1`: Expected response frame ID.

#### Wakeup Logic State
- `int wakeupRetryCount = 0`: Counter for wakeup attempt retries.
- `bool pendingWakeup = false`: Indicates if a wakeup pulse is queued or active.

#### Physical Layer (PHY) Parameters
- `double baudRate`: Transmission speed in bits per second.
- `double propDelay`: Propagation delay across the network interface.
- `double interByteTime`: Delay between individual bytes in a frame.
- `int breakBits = 13`: Duration of the break signal measured in bit-times.
- `int delimiterBits = 1`: Duration of the break delimiter measured in bit-times.

#### Data Length Code (DLC) Mapping
- `std::unordered_map<int, int> frameIdToDlc`: Maps frame IDs to payload byte lengths.

#### Scheduling Tables & Collision Handling
- `std::vector<LinSlot> scheduleTable`: Main schedule table.
- `int currentSlotIndex = 0`: Current pointer in `scheduleTable`.
- `std::vector<LinSlot> collisionScheduleTable`: Schedule table used during ETF collision resolution.
- `bool resolvingCollision = false`: Flag indicating active collision resolution state.
- `int collisionSlotIndex = 0`: Current pointer in `collisionScheduleTable`.
- `LinSlot pendingHeaderSlot`: Stores a slot that is pending transmission.
- `bool pendingHeaderValid = false`: Validity flag for `pendingHeaderSlot`.
- `endResolveAfterHeader = false`: Controls collision resolution termination after header processing.

#### Signals & Error Injections
- `std::vector<LinSignal> signalTable`: Array of mapped LIN signals.
- `std::vector<LinErrorSignalBinding> errorBindings`: Mapping of error injection targets.
- `LinErrorState errorState`: Current error tracking status flags.

#### Event-Triggered Frame (ETF) & Event Logic
- `std::unordered_map<int, std::vector<int>> etfToUcfs`: Maps ETF frame IDs to Unconditional Frame (UCF) IDs.
- `std::unordered_map<int, bool> ucfEventPending`: Tracks whether an event is pending for a given UCF ID.
- `double eventRearmPeriod = -1`: Interval to re-arm events.
- `double eventProbability = 1.0`: Probability of an event occurring when re-armed.

---

### Member Functions

#### Lifecycle Overrides (OMNeT++)
- `virtual void initialize() override`: Module initialization logic.
- `virtual void handleMessage(cMessage *msg) override`: Primary message and event handling entry point.
- `virtual void finish() override`: Finalization and statistics collection logic upon simulation end.

#### Configuration Parsing & DLC Helpers
- `void parseSchedule(const char* config, std::vector<LinSlot>& table)`: Parses schedule configurations into target slot vectors.
- `void parseSignals(const char* config)`: Parses signal mapping configurations.
- `void parseErrorSignals(const char* config)`: Parses error binding configurations.
- `void parseFrameDlcs(const char* config)`: Parses frame DLC lookup parameters.
- `int dlcForFrame(int frameId) const`: Returns payload DLC size for a given frame ID.

#### ETF (Event-Triggered Frame) Helpers
- `void parseEventFrames(const char* config)`: Parses ETF-to-UCF relationships from configuration.
- `void initializeEventFlags()`: Sets initial state for pending event flags.
- `bool isEventFrameId(int etfId) const`: Checks if a given frame ID corresponds to an ETF.
- `bool selectUcfForEventFrame(int etfId, int& outUcfId) const`: Selects a UCF associated with an ETF if an event is pending.
- `void setEventPending(int ucfId, bool pending)`: Sets or clears the pending status of an event on a UCF.

#### Power & State Management
- `void resetInactivityTimer()`: Resets the node inactivity timer.
- `void enterSleep(const char *reason)`: Transitions node state to `SLEEP`.
- `void resetRxContext()`: Resets reception state variables and tracking flags.

#### Protocol Logic & Framing Helpers
- `bool isPublisherFor(int frameId)`: Determines if this node is configured to publish the specified frame ID.
- `void applyErrorBits(int logicalFrameId, LinResponse *rsp, int payloadBase)`: Injects configured error bit patterns into a response frame payload.
- `bool isGoToSleepCommand(const LinResponse *rsp) const`: Evaluates if a received response is a standard Go-To-Sleep command.
- `bool isEchoFromSelf(int sourceAddr) const`: Returns `true` if the message's source address matches `nodeAddress` (and `nodeAddress >= 0`).

#### Timing Calculation Helpers
- `double headerRemainderTxTime() const`: Calculates transmission time for the remainder of a header after the break.
- `double responseTxTime(int dataLength) const`: Calculates transmission time for a response payload of a specified length.

#### Protocol Actions
- `void sendBreakSignal()`: Initiates transmission of the LIN Break signal.
- `void sendWakeupPulse()`: Initiates transmission of a LIN wakeup pulse.
- `void sendHeaderRemainder(int frameId, std::string publisher)`: Transmits the remainder of the header (Sync byte and PID).
- `void sendResponse(int frameId, double initialDelay)`: Sends a LIN response payload after a calculated delay.
- `void sendEventResponse(int etfId, int ucfId, double initialDelay)`: Sends a response for an ETF/UCF event after an initial delay.
- `void processReceivedHeader(LinHeader *header)`: Processes an incoming `LinHeader` object.
- `void startCollisionResolution()`: Switches scheduling to collision resolution mode.
- `bool isWakeupPulse(LinBreakSignal *brk)`: Determines if a received break signal represents a wakeup attempt.