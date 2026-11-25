# LIN Protocol Implementation Status

This report details the current state of the LIN protocol implementation in OMNeT++, highlighting what is complete, what is missing, deviations from the specification, and areas for improvement.

## 1. What is Done

*   **Core Protocol:**
    *   **Master/Slave Roles:** Distinct behaviors for master (scheduling) and slave (responding) are implemented.
    *   **Frame Structure:** Basic LIN frame components (Break, Sync, PID, Data, Checksum) are modeled.
    *   **Scheduling:** Master node supports schedule tables, including a specific collision resolution schedule.
    *   **Event Triggered Frames (ETF):** Support for ETFs, collision detection, and a mechanism to switch to a collision resolution schedule.
    *   **Sleep/Wakeup:** Logic for entering sleep mode via the Go-To-Sleep command (0x3C, byte 0=0x00) and waking up via a Wakeup pulse (long dominant signal).
    *   **Inactivity Timeout:** Nodes enter sleep mode after a configurable period of inactivity (default 4s).

*   **Physical Layer Simulation:**
    *   **Bus Arbitration:** Packet-level arbitration is simulated.
    *   **Error Injection:** Configurable frame error rates (corruption) and drop rates.
    *   **Timing:** Basic timing for header and response transmission based on baud rate.

*   **Data Handling:**
    *   **Signal Mapping:** Signals are mapped to frames and byte offsets via configuration strings.
    *   **DLC Handling:** Data Length Code (DLC) is configurable per frame ID.

## 2. What is Left / Missing

*   **Diagnostic Transport Layer (TL):**
    *   **Multi-Frame Support:** Currently, only Single Frame (SF) transmission is supported. First Frame (FF), Consecutive Frame (CF), and Flow Control (FC) logic are completely missing.
    *   **Segmentation/Reassembly:** No logic to handle messages larger than a single frame.
    *   **Timing Parameters:** P2/P2* timing requirements for diagnostic responses are not explicitly enforced.

*   **Node Configuration Services:**
    *   **Standard SIDs:** Services such as `Read by Identifier` (0xB2), `Assign NAD` (0xB0), `Save Configuration` (0xB6), etc., are not implemented.
    *   **Status Management:** Error counters and status flags accessible via diagnostics are missing.

*   **LDF Parsing:**
    *   **Configuration:** The project relies on ad-hoc string parsing for schedules and signals. It does not parse standard LIN Description Files (LDF).

## 3. Not Up To Spec / Deviations

*   **Checksum Calculation:**
    *   The implementation assumes **Classic Checksum** only for diagnostic frames (0x3C/0x3D) and **Enhanced Checksum** for everything else. This is valid for LIN 2.x but inflexible. LIN 1.x used Classic Checksum for all frames. The type should be configurable per frame or node version.

*   **Collision Resolution:**
    *   The collision resolution logic in `LinNode.cc` is simplified. It runs the collision schedule once. The spec requires checking if the collision is resolved and potentially re-running or continuing until successful transmission or timeout.

*   **Go-To-Sleep Command:**
    *   The command validation is "tolerant" and does not strictly enforce that data bytes 1-7 are 0xFF as per the LIN 2.x specification.

*   **Physical Layer Abstraction:**
    *   **Dominant/Recessive States:** The bus models collisions at the packet level. It does not simulate the physical superposition of dominant and recessive bits, which is critical for accurate arbitration in edge cases.
    *   **Header/Response Split:** The simulation treats Header and Response as separate packets. In reality, a LIN frame is a continuous stream of bytes. This abstraction simplifies the model but might miss byte-level timing issues.

## 4. Improvements

*   **Refactor Diagnostics:** Move diagnostic logic from `LinNode.cc` to `LinDiag.cc` and implement a full state machine for the Transport Layer (SF, FF, CF, FC).
*   **Enhance Configuration:** Implement an LDF parser or a tool to convert LDF files into the current configuration format.
*   **Add Checksum Configuration:** Allow configuration of checksum type (Classic vs Enhanced) per frame or globally based on LIN version.
*   **Improve Timing Models:** Incorporate jitter and tolerance checks for bit timing and inter-byte spacing (T_Header_Nominal vs T_Header_Max).
*   **Standard Services:** Implement mandatory node configuration services to fully comply with LIN 2.x conformance tests.
