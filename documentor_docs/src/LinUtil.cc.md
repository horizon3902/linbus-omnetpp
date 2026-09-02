# Technical Documentation: `src/LinUtil.cc`

## Overview

The `src/LinUtil.cc` file provides utility functions for handling Local Interconnect Network (LIN) protocol calculations. Specifically, it implements functions to compute the Protected Identifier (PID) from a raw frame ID and calculate the frame Checksum (supporting both Classic and Enhanced checksum algorithms) according to LIN specification rules.

---

## Dependencies

* **`LinUtil.h`**: Header file providing function declarations and related data type definitions.
* **`LinResponse`**: An external/included object type passed by pointer to `computeChecksum`. The code interacts with the following methods on `LinResponse`:
  * `int getDataLength()`: Returns the number of data bytes in the response frame.
  * `<type> getData(int index)`: Returns the data byte at the specified index.

---

## Functions Detail

### 1. `computePID`

Calculates the 8-bit Protected Identifier (PID) for a given raw LIN frame ID using LIN 2.0+ parity rules.

#### Function Signature
```cpp
unsigned char computePID(int frameId);
```

#### Parameters
* `frameId` (`int`): The raw LIN frame identifier. Only the lower 6 bits (bits 0 to 5) are used.

#### Return Value
* `unsigned char`: The 8-bit Protected Identifier containing the 6-bit frame ID and 2 parity bits (P0 and P1).

#### How It Works
1. **Extract 6-bit Frame ID**: Applies a bitwise AND mask (`0x3F`) to `frameId` to isolate the lower 6 bits ($ID_0$ through $ID_5$).
2. **Calculate Parity Bit P0**: Computes parity bit $P0$ at bit position 6 using XOR logic across bits 0, 1, 2, and 4:
   $$\text{P0} = ID_0 \oplus ID_1 \oplus ID_2 \oplus ID_4$$
3. **Calculate Parity Bit P1**: Computes parity bit $P1$ at bit position 7 using inverted XOR logic across bits 1, 3, 4, and 5:
   $$\text{P1} = \neg (ID_1 \oplus ID_3 \oplus ID_4 \oplus ID_5)$$
4. **Assemble PID**: Combines the original 6-bit frame ID, bitwise left-shifted `p0` by 6 bits, and bitwise left-shifted `p1` by 7 bits using bitwise OR operations.

---

### 2. `computeChecksum`

Calculates the 8-bit LIN checksum for a frame response based on its PID and data bytes.

#### Function Signature
```cpp
unsigned char computeChecksum(LinResponse *frame, int pid);
```

#### Parameters
* `frame` (`LinResponse*`): Pointer to a `LinResponse` instance containing frame payload data and data length information.
* `pid` (`int`): The Protected Identifier byte associated with the frame.

#### Return Value
* `unsigned char`: The inverted 8-bit checksum value.

#### How It Works
1. **Retrieve Payload Length**: Calls `frame->getDataLength()` to obtain the total number of bytes in the data payload (`len`).
2. **Determine Checksum Mode (Classic vs. Enhanced)**:
   * Extracts the raw 6-bit ID from `pid` via `pid & 0x3F`.
   * Checks if `rawId` equals `0x3C` or `0x3D`.
   * If `rawId` is `0x3C` or `0x3D`, `classic` is set to `true` (Classic Checksum). Otherwise, `classic` is `false` (Enhanced Checksum).
3. **Initialize Sum Accumulator**:
   * Initializes `sum` as an `unsigned int` set to `0`.
   * For **Enhanced Checksum** (`!classic`), the raw `pid` byte cast to `unsigned char` is added to `sum`.
   * For **Classic Checksum**, `pid` is omitted from `sum`.
4. **Add Payload Data**:
   * Iterates through all payload bytes from index `0` to `len - 1` via `frame->getData(i)`.
   * Casts each byte to `unsigned char` and adds it to `sum`.
5. **Carry-over Folding**:
   * Uses a `while` loop to fold 16-bit overflow carries into the 8-bit low byte:
     ```cpp
     while (sum > 0xFF) {
         sum = (sum & 0xFF) + (sum >> 8);
     }
     ```
   * Any value exceeding `0xFF` has its high byte added back to its low byte until the sum fits within an 8-bit representation (`<= 0xFF`).
6. **Invert Sum**:
   * Applies a bitwise NOT operation (`~sum`) to invert all bits of the sum.
   * Masks the result with `0xFF` to return a clean `unsigned char`.