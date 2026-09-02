# Technical Documentation: `src/LinBusLogic.cc`

## Overview

The `LinBusLogic` class implements an OMNeT++ module that acts as the central physical/bus communication logic for a Local Interconnect Network (LIN) bus model. It provides timed packet forwarding to all connected gates, enforces propagation delays, models transmission durations (for headers and responses), and handles error injections (frame drop and corruption) alongside collision detection for overlapping response frames.

---

## Class Parameters and State Variables

### OMNeT++ Parameters (Loaded in `initialize()`)
* **`baudRate`**: Bus baud rate used to calculate transmission durations.
* **`propDelay`**: Bus propagation delay applied when forwarding frames across gates.
* **`interByteTime`**: Time delay parameter configured from module parameters.
* **`frameErrorRate`**: Probability threshold ($[0, 1]$) for corrupting incoming frames.
* **`frameDropRate`**: Probability threshold ($[0, 1]$) for dropping incoming frames.

### State Variables
* **`isResponseOnBus`** (`bool`): Indicates whether a response transmission window is currently active on the bus.
* **`transmissionEndTimer`** (`cMessage*`): A self-message timer scheduled to trigger when a response frame finishes transmitting.
* **`pendingResponse`** (`LinResponse*`): Pointer holding the currently transmitting LIN response frame until its transmission window expires.

---

## Member Functions

### `void initialize()`
Initializes module parameters, logs module startup, resets state flags, and instantiates the `transmissionEndTimer` self-message.

### `void finish()`
Cleans up dynamically allocated memory when the simulation ends:
* Cancels and deletes `transmissionEndTimer`.
* Deletes `pendingResponse` if it has not yet been delivered or deleted.

### `double headerRemainderTxTime() const`
Calculates the transmission time required for the remainder of a LIN Header (Sync byte + PID byte).
* **Calculation**: Each byte consists of 10 bits (UART style: start bit, 8 data bits, stop bit). Total = 20 bits.
* **Return Value**: $\frac{20.0}{\text{baudRate}}$ seconds.

### `double responseTxTime(int dataLength) const`
Calculates the transmission duration of a LIN Response frame based on its data length.
* **Bounds**: Clamps `dataLength` between $0$ and $8$ bytes.
* **Calculation**: Total bytes = `dataLength + 1` (data bytes + 1 checksum byte). Each byte is 10 bits.
* **Return Value**: $\frac{\text{Total Bits}}{\text{baudRate}}$ seconds.

### `void forwardToAll(cPacket *frame, simtime_t extraDelay)`
Broadcasts a packet to all vector gates matching output gate array `"gate$o"`.
* Iterates over all connections in `"gate$o"`.
* Duplicates the frame (`frame->dup()`) for all output ports except the last port, which receives the original `frame` pointer.
* Sends frames using `sendDelayed()` with the specified `extraDelay`.

---

## Message Handling Logic (`handleMessage`)

When `handleMessage(cMessage *msg)` is invoked, processing follows a sequential 10-step pipeline:

```
[Incoming Message / Self-Message]
       │
       ├── (1) Is transmissionEndTimer? ──► [Clear bus flag, forward pending response with propDelay]
       │
       ├── (2) Determine ingress port index
       ├── (3) Cast message to cPacket
       ├── (4) Set ingress port index on LIN frame
       ├── (5) Validate bus status (warn if header/break arrives during active response)
       │
       ├── (6) Error Injection: Drop? (uniform < frameDropRate)
       │        └── Yes: Delete frame and exit
       │
       ├── (7) Error Injection: Corrupt? (uniform < frameErrorRate)
       │        └── Yes: XOR PID (Header) or flip byte (Response)
       │
       ├── (8) Is LinResponse?
       │        ├── Bus free: Schedule transmissionEndTimer, hold response
       │        └── Bus busy: Mark collision on pending response, delete incoming frame
       │
       ├── (9) Is LinHeader? ──► Forward to all with (headerRemainderTxTime + propDelay)
       │
       └── (10) Is Break / Other? ──► Forward to all with propDelay
```

### Detailed Pipeline Breakdown

1. **Self-Message Handling (End of Response Window)**
   * If `msg == transmissionEndTimer`:
     * Sets `isResponseOnBus = false`.
     * If `pendingResponse` exists, forwards it to all gates via `forwardToAll(pendingResponse, propDelay)` and resets `pendingResponse = nullptr`.

2. **Source Gate Identification**
   * Inspects `msg->getArrivalGate()` to capture `srcGateIndex` (or `-1` if null).

3. **Packet Casting**
   * Converts `cPacket* frame = check_and_cast<cPacket *>(msg)`.

4. **Ingress Port Recording**
   * Sets `setIngressPortIndex(srcGateIndex)` on `LinBreakSignal`, `LinHeader`, or `LinResponse` objects without altering original sender attributes.

5. **Sanity Check**
   * Logs a warning if a non-response frame arrives while `isResponseOnBus` is `true`.

6. **Error Injection: Frame Drop**
   * Evaluates `uniform(0, 1) < frameDropRate`. If true, logs a warning, deletes the frame, and halts processing.

7. **Error Injection: Corruption**
   * Evaluates `uniform(0, 1) < frameErrorRate`. If true:
     * **`LinResponse`**: Selects a random byte index (`intuniform(0, len - 1)`) and flips bits using XOR (`oldVal ^ 0xFF`).
     * **`LinHeader`**: Inverts the Protected Identifier (PID) using XOR (`getPid() ^ 0xFF`).
     * **`LinBreakSignal`**: Logged, but bit pattern/duration remains unchanged.

8. **Response Frame Processing & Collision Handling**
   * If frame is a `LinResponse`:
     * **No collision (`!isResponseOnBus`)**: Sets `isResponseOnBus = true`, stores frame in `pendingResponse`, and schedules `transmissionEndTimer` at `simTime() + responseTxTime(dataLength)`.
     * **Collision detected (`isResponseOnBus == true`)**: Flags `pendingResponse->setCollision(true)`, updates name to `"LIN_COLLISION"`, and deletes the colliding incoming response frame.

9. **Header Frame Processing**
   * If frame is a `LinHeader`: Calculates remainder duration via `headerRemainderTxTime()` and calls `forwardToAll(hdr, tx + propDelay)`.

10. **Break Signal & Other Processing**
    * For `LinBreakSignal` or unhandled packets: Calls `forwardToAll(frame, propDelay)`.