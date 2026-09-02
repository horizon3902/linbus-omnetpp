# Technical Documentation: `src/LinUtil.h`

## Overview

The `src/LinUtil.h` header file declares utility functions for processing Local Interconnect Network (LIN) bus frame components. Specifically, it provides interface declarations for calculating a Protected Identifier (PID) from a frame ID and computing checksums (both Classic and Enhanced) for LIN response messages.

---

## Included Dependencies

* **`"LinDataFrame_m.h"`**: Provides the data structure definitions required by this header (specifically `LinResponse`).

---

## File Structure & Preprocessor Directives

To prevent duplicate inclusions during compilation, the file uses standard header guards:

```cpp
#ifndef LINUTIL_H_
#define LINUTIL_H_

// Header content ...

#endif
```

---

## Key Components & Functions

### 1. `computePID`

```cpp
unsigned char computePID(int frameId);
```

* **Purpose**: Computes the Protected Identifier (PID) for a given LIN frame.
* **Parameters**:
  * `frameId` (`int`): The raw frame identifier value.
* **Return Value**: 
  * `unsigned char`: The calculated 8-bit Protected Identifier containing the frame ID and parity bits.

---

### 2. `computeChecksum`

```cpp
unsigned char computeChecksum(LinResponse *frame, int pid);
```

* **Purpose**: Computes the checksum for a `LinResponse` frame based on the specified PID type.
* **Parameters**:
  * `frame` (`LinResponse*`): Pointer to the `LinResponse` object containing the frame data.
  * `pid` (`int`): The Protected Identifier associated with the frame, used to determine checksum behavior.
* **Return Value**:
  * `unsigned char`: The calculated 8-bit checksum value.
* **Checksum Logic Rules** (as defined in code comments):
  * **Classic Checksum**: Applied when `pid` is `0x3C` or `0x3D`. Computes the checksum using **Data only**.
  * **Enhanced Checksum**: Applied for all other PID values. Computes the checksum using **PID + Data**.

---

## How It Works

1. **Header Inclusion**: Software modules that need to generate or validate LIN frame PIDs and checksums include `LinUtil.h`.
2. **PID Calculation**: Callers pass an integer `frameId` into `computePID()` to retrieve the combined ID and parity bit representation (`unsigned char`).
3. **Checksum Calculation**: Callers pass a pointer to a `LinResponse` structure along with its `pid` into `computeChecksum()`. The function evaluates the PID:
   * If `pid` equals `0x3C` or `0x3D`, it calculates a **Classic Checksum** over the data bytes only.
   * For any other `pid` value, it calculates an **Enhanced Checksum** over both the PID and the data bytes.