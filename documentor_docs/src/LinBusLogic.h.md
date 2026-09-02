# Technical Documentation: `src/LinBusLogic.h`

## Overview

The `src/LinBusLogic.h` header file defines the `LinBusLogic` class, an OMNeT++ simple module (`cSimpleModule`) responsible for simulating the physical and logical behavior of a Local Interconnect Network (LIN) bus. 

It manages physical layer parameters (baud rate, propagation delay, inter-byte delays), models channel impairments (frame error rates and frame drop rates), handles message timing based on LIN UART framing specifications, and tracks bus response windows for collision modeling.

---

## Class Definition: `LinBusLogic`

* **Base Class:** `cSimpleModule` (OMNeT++ framework)
* **Header Dependencies:**
  * `<omnetpp.h>`
  * `"LinDataFrame_m.h"` (Provides definitions such as `LinResponse`)

---

## Member Variables

### Bus Configuration & Physical Layer Parameters
* **`double baudRate`**: Data transmission speed on the LIN bus.
* **`double propDelay`**: Signal propagation delay along the bus.
* **`double interByteTime`**: Time delay between consecutive bytes during frame transmission.

### Channel Impairments
* **`double frameErrorRate`**: The rate/probability at which frames encounter errors.
* **`double frameDropRate`**: The rate/probability at which frames are dropped during transmission.

### Collision & Response Window Tracking
* **`bool isResponseOnBus`**: Tracks whether a LIN response is currently active on the bus to aid collision modeling.
* **`cMessage *transmissionEndTimer`**: Timer message pointer used to schedule and detect the end of a frame transmission.
* **`LinResponse *pendingResponse`**: Temporarily holds a pointer to the first response frame until the end of the response window, allowing subsequent collisions to invalidate it if necessary.

---

## Member Functions

### Protected OMNeT++ Lifecycle Methods

These methods override standard lifecycle functions defined in `cSimpleModule`:

* **`virtual void initialize() override`**
  Initializes module parameters, variables, and timers at the start of the simulation run.

* **`virtual void handleMessage(cMessage *msg) override`**
  Handles incoming messages and events (e.g., timer expirations, frame transmissions from connected modules).

* **`virtual void finish() override`**
  Executes cleanup actions and final logic at the conclusion of the simulation run.

---

### Private Helper Methods

* **`double headerRemainderTxTime() const`**
  Calculates the nominal transmission duration (in seconds) for the remainder of a LIN header.
  * *Calculated size:* 2 bytes (Sync byte + PID byte), corresponding to **20 bits** based on standard 10-bit UART framing (1 start bit + 8 data bits + 1 stop bit per byte).

* **`double responseTxTime(int dataLength) const`**
  Calculates the nominal transmission duration (in seconds) for a LIN response given a payload size.
  * **`dataLength`**: Number of payload data bytes ($N$).
  * *Calculated size:* $(N + 1)$ bytes ($N$ data bytes + 1 checksum byte), corresponding to **$(N + 1) \times 10$ bits**.

* **`void forwardToAll(cPacket *frame, simtime_t extraDelay)`**
  Forwards a packet frame to all relevant destinations/gates on the bus, applying an additional specified time delay (`extraDelay`).

---

## Operational Logic Summary

Based on the header declarations and inline comments:

1. **LIN Framing Calculations**: Bit durations for headers and responses are modeled on standard UART framing (10 bits per byte).
   * **Header Remainder**: $1 \text{ Sync Byte} + 1 \text{ PID Byte} = 2 \text{ Bytes} (20 \text{ bits})$.
   * **Response Payload**: $N \text{ Data Bytes} + 1 \text{ Checksum Byte} = (N + 1) \text{ Bytes} ((N + 1) \times 10 \text{ bits})$.
2. **Collision Modeling**:
   * Uses `isResponseOnBus` to determine if a response window is currently open.
   * `pendingResponse` buffers the initial response frame until `transmissionEndTimer` expires. If another response arrives during this window, a collision occurs and can invalidate the pending response.
3. **Impairments**: Applies configurable error (`frameErrorRate`) and drop (`frameDropRate`) probabilities during frame forwarding via `forwardToAll`.