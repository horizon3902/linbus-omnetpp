# LIN Bus Simulation (OMNeT++)

A discrete-event simulation of a **LIN (Local Interconnect Network) bus**
built on OMNeT++. A configurable master node drives a schedule of
Break → Sync/PID Header → Response slots over a shared bus; any number of
slave nodes can be configured (purely from the `.ini` file, no code changes)
to publish responses, and the bus can inject random frame corruption/drops
and detect response collisions.

> This README describes the codebase as of the files in `src/` listed
> below. If you're working from an older drop of this repo that only has a
> fixed 3-node network with a hardcoded schedule, that version is superseded
> by this one — see section 7 for exactly what changed and what still isn't
> implemented.

## 1. Project layout

```
src/
├── LinNetwork.ned        # Top-level network: bus + master + slaves[numSlaves]
├── LinBusLogic.ned/.h/.cc  # Bus: forwards frames, injects errors, detects collisions
├── LinBus.ned             # Thin gate-array wrapper around LinBusLogic — NOT used by
│                           #   LinNetwork (which instantiates LinBusLogic directly).
│                           #   Kept in the tree but currently orphaned.
├── LinNode.ned/.h/.cc      # Master/slave node: schedule engine, response
│                           #   publishing, error-bit signaling, sleep state
├── LinDataFrame.msg        # LinBreakSignal / LinHeader / LinResponse (+ collision flag)
├── LinUtil.h/.cc           # computePID() and computeChecksum()
└── Makefile                # opp_makemake -f --deep output (OBJS list matches this file set)
```

`.ini` files in this repo: `omnetpp.ini`, `power_window.ini`,
`event_triggered.ini`, `milestone2.ini`, `sanity.ini`, `all-test.ini`,
`all-test-2.ini`, `sleep-wakeup.ini`, `sleep-wakeup-etf.ini`. **Not all of
them match what this source actually implements — see section 6 for a
file-by-file compatibility rundown before you pick one to run.**

## 2. Requirements

- **OMNeT++ 6.0.3**, with `opp_makemake`, `opp_run`, `Cmdenv`/`Qtenv`
  and a C++ compiler on `PATH` (source OMNeT++'s environment setup script
  first).
- Linux, macOS, or Windows with the matching OMNeT++ toolchain.

```bash
opp_run -v
which opp_makemake
```

## 3. Project setup

1. Create/open an OMNeT++ project folder, e.g. `linbus/`.
2. Put all files under `linbus/src/` (every `.ned`/`.msg` file declares
   `package src;`, so the folder must be named `src`).
3. Put the `.ini` files in `linbus/` (project root), since they all set
   `network = src.LinNetwork`.

```
linbus/
├── src/            # everything from section 1
└── omnetpp.ini      # + the other .ini files
```

## 4. Build

### Command line

```bash
cd linbus/src
make
```

Produces `linbus` (or `linbus.exe`) in `src/`. If you add/remove `.cc` or
`.msg` files, regenerate the Makefile from the project root instead:

```bash
cd linbus
opp_makemake -f --deep -o linbus -O out -I src src/*.cc
```

### IDE

Import as an existing OMNeT++ project and build normally
(**Project → Build Project**).

## 5. Run

```bash
cd linbus
./src/linbus -u Cmdenv -n src omnetpp.ini
# or, with the graphical runtime:
./src/linbus -u Qtenv -n src omnetpp.ini
```

To run a specific named config from a multi-config `.ini` file:

```bash
./src/linbus -u Cmdenv -n src -c M123_Functional all-test.ini
```

`-n src` points the NED loader at the `src/` folder — adjust if your layout
differs. From the IDE: right-click the `.ini` file → **Run As → OMNeT++
Simulation**, then pick the config from the launch dialog.

**Start with `omnetpp.ini`** — of everything provided, it's the one whose
`.ini` settings line up most completely with what `LinNode`/`LinBusLogic`
actually implement (see the matrix in section 8).

## 6. What the current source actually implements

### Topology
`LinNetwork.ned` takes a `numSlaves` parameter (default 2) and builds
`master` + `slaves[numSlaves]`, all wired to one `bus` (a `LinBusLogic`
instance sized `1 + numSlaves` gates). Set the slave count with
`*.numSlaves = N` in your ini — every `slaves[i]` you reference must be
`< N`.

### Master schedule (`LinNode`)
`scheduleConfiguration` (master-only) and `collisionScheduleConfiguration`
(master-only) are semicolon-separated lists of
`FrameIdHex:Label:PeriodSeconds`, e.g. `"10:M:0.05;20:ETF(E):0.05"`.
- IDs are parsed as **hex** regardless of a `0x` prefix (`3C` → 0x3C).
- The `Label` field (`M`, `ETF(E)`, `RES`, `Slave0_Unique`, ...) is
  **cosmetic only** — it's logged but never used to decide who responds.
  Don't rely on it to route frames.
- For each slot the master sends a Break, then (after 13-bit-time break +
  inter-byte gap) a Header carrying the frame ID and a computed PID.
- `collisionScheduleConfiguration` is only entered when the bus reports a
  collision (or a checksum error — see the caveat below) on a response
  *and* that table is non-empty; the master then works through it once and
  returns to the normal schedule.

### Who responds to a header ("publisher")
There's **no publisher name resolution** — any node (master or slave) that
has an entry for a given frame ID in its own `signalConfiguration` will
respond to that header, full stop. This means:
- **Two nodes both configured to publish the same ID = a real collision**,
  detected by the bus. This is how `event_triggered.ini` and `sanity.ini`
  create their intentional collisions, and it works correctly.
- There is **no separate "event-triggered frame" concept** — no code reads
  an `eventFrameConfiguration` mapping (e.g. `"20:21,22"`) to make slaves
  answer header `0x20` with their own unrelated `0x21`/`0x22` payload. If a
  config's slaves only have `signalConfiguration` entries for `0x21`/`0x22`
  but *not* `0x20` itself, header `0x20` simply times out — no response, no
  collision, nothing to resolve. This affects `milestone2.ini` and most of
  `all-test.ini`/`all-test-2.ini` (see matrix below).

### Signals — `signalConfiguration`
`FrameIdHex:ByteOffset:Value; ...`. Sets `data[ByteOffset] = Value` in the
8-byte response this node sends when it's a publisher for that ID. Bytes
not mentioned default to `0`.

### Error-bit reporting — `errorSignalConfiguration`
`FrameIdHex:ByteOffset:BitIndex; ...`. When a node has just flagged a
header-PID mismatch, a response checksum/collision error, or (master only)
a response timeout, it OR's bit `BitIndex` of byte `ByteOffset` into the
**next** response it publishes for the given frame ID, then clears its
error flags. If a node never publishes the ID named here, the mapping is
inert.

### Bus fault injection & collision detection — `LinBusLogic`
`baudRate`, `propDelay`, `interByteTime`, `frameErrorRate`, `frameDropRate`
are all real, working parameters:
- `frameDropRate`: chance to silently drop any frame (Break/Header/Response).
- `frameErrorRate`: chance to flip a random data byte (Response) or the PID
  (Header).
- Collision detection: the bus tracks whether a Response is already "on
  the wire"; if a second Response arrives before the first one's computed
  transmission time has elapsed, it's flagged `collision=true` and the
  master (if it has PID/checksum validation active) reports a Response
  error and can switch to `collisionScheduleConfiguration`.
- **Caveat:** the master treats *any* response checksum failure — random
  bus corruption included, not just genuine collisions — as a trigger for
  collision resolution when a resolution table is configured. With
  `frameErrorRate > 0` and a non-empty `collisionScheduleConfiguration`,
  expect occasional resolution-schedule detours triggered by plain bit
  corruption, not just real collisions.

### PID and checksum
Unchanged: `computePID()` implements LIN 2.x parity (P0/P1); `computeChecksum()`
uses the classic checksum (data only) for `0x3C`/`0x3D` and the enhanced
checksum (PID + data) otherwise.

### Sleep — go-to-sleep frame
Works: if a node (any node — master or slave) publishes response ID `0x3C`
with `data[0] == 0x00` (i.e. it has `signalConfiguration` like `"3C:0:0"`),
every node that receives that response enters `SLEEP`. On the master, this
also pauses the schedule timer. While asleep, a node ignores everything
except a long dominant pulse ("wakeup pulse", > 20 bit-times) on the bus.

### Wakeup — **not wired up**
`LinNode` has the mechanics to *send* and *recognize* a wakeup pulse
(`sendWakeupPulse()`, `isWakeupPulse()`, a `wakeupTimer` message and its
handler), but **nothing ever schedules `wakeupTimer`** — there's no
`scheduleAt(..., wakeupTimer)` call anywhere in `initialize()`, and
`LinNode.ned` doesn't even declare a `wakeupTime` parameter. Any
`*.slaves[i].wakeupTime = ...` line in an ini file is currently a no-op:
once a node/network goes to sleep via `0x3C`, it stays asleep for the rest
of the run. To make the wakeup demos in `sanity.ini` /
`sleep-wakeup*.ini`/`all-test*.ini` actually work, `LinNode` needs a
`wakeupTime` NED parameter and a `scheduleAt(simTime() + par("wakeupTime"),
wakeupTimer)` call (guarded for negative/disabled values) in `initialize()`.

### Not implemented at all
These parameters appear in one or more `.ini` files but have no
corresponding NED parameter or parsing code anywhere in `LinNode`/
`LinBusLogic`, so assignments to them are simply unused:
`eventFrameConfiguration`, `eventRearmPeriod`, `eventProbability`,
`nodeAddress`, `frameDlcConfiguration`, `breakBits`, `delimiterBits`,
`inactivitySleepTime`. Unused ini assignments don't crash the run — they're
just silently ignored.

## 7. What changed vs. the earlier (simpler) source drop

If you previously had a version where `LinNetwork.ned` hardcoded exactly
`master`/`slave1`/`slave2` and `LinNode::initialize()` hardcoded its own
3-slot schedule in C++ with no `.ini`-driven configuration at all, that's
now superseded by the file set in section 1: topology, schedule, signals,
error-bit mapping, bus fault injection, and collision detection are all
config-driven. `LinBus.ned` is a leftover from that shape (a generic
gate-array-wrapper-around-`LinBusLogic` module) that `LinNetwork.ned`
doesn't reference anymore — it can be deleted or ignored.

## 8. Compatibility matrix — which `.ini` files actually work

| File | Config(s) | Status | Notes |
|---|---|---|---|
| `omnetpp.ini` | `[General]` only | **Works** | numSlaves=3, schedule/signal/error-signal all match declared params; error injection exercises corruption/drop/timeout paths. Best starting point. |
| `power_window.ini` | `[General]` only | **Bug — fix before running** | References `slaves[2]` but never sets `*.numSlaves = 3` (default is 2), so the "Rear" slave module doesn't exist and its config is silently dropped; master will time out waiting for `0x22`. Add `*.numSlaves = 3`. |
| `event_triggered.ini` | `[General]` only | **Works** | Both slaves publish `0x10` directly → genuine collision → resolves via `0x20`/`0x21`. Good worked example of the collision mechanism. |
| `sanity.ini` | `[General]` only | **Mostly works** | Both slaves publish `0x20` directly (real collision, resolves via `0x21`/`0x22` — works), and the `0x3C` go-to-sleep works. `nodeAddress` is unused (ignored). `wakeupTime` is a no-op (section 6) — the bus stays asleep after the sleep slot; it does **not** wake at 0.20s as the comments imply. |
| `milestone2.ini` | `[General]` only | **Partially works** | Base `0x10` unconditional frame works. The `0x20` "ETF" slot times out every cycle — no slave has a direct `signalConfiguration` entry for `0x20`, and `eventFrameConfiguration` isn't implemented, so it's a no-op collision demo, not a real one. |
| `all-test.ini` | `M123_Functional`, `M123_ErrorInjection`, `M3_InactivitySleep` | **Partially works** | Same ETF gap as `milestone2.ini` (`0x20` slot always times out; the intended collision + resolution never triggers). `0x3C` sleep does fire but nothing wakes it (no `wakeupTime` support). `M123_ErrorInjection`'s `frameErrorRate`/`frameDropRate` do work. `M3_InactivitySleep` relies entirely on unimplemented `inactivitySleepTime` — behaves like a plain schedule run with a long idle gap, no inactivity-triggered sleep. |
| `all-test-2.ini` | `M_AllFeatures_Sanity`, `M_DLC_Stress`, `M_ErrorInjection_Regression`, `M_SleepWake`, `M_InactivitySleep` | **Mostly unsupported** | All configs depend on `eventFrameConfiguration`/`nodeAddress`/`frameDlcConfiguration`/`breakBits`/`delimiterBits`/`inactivitySleepTime`/`eventRearmPeriod`/`eventProbability` — none implemented. `M_ErrorInjection_Regression`'s bus fault-injection rates and `M_SleepWake`'s `0x3C` sleep are the only parts that will actually do something; everything else is a no-op. |
| `sleep-wakeup.ini` | `[General]` only | **Partially works** | `0x10` unconditional traffic and the `0x3C` sleep transition both work. `wakeupTime` is a no-op — the bus does not resume at 0.6s. |
| `sleep-wakeup-etf.ini` | `[General]` only | **Mostly works** | Both slaves publish `0x20` directly → real collision → resolves via `0x21`/`0x22` (works, same pattern as `event_triggered.ini`). `0x3C` sleep works. `wakeupTime` is a no-op — the 1.2s wakeup in the comments will not happen. |

**Rule of thumb:** a config's "collision" only actually fires if two nodes
share a literal `signalConfiguration` entry for the *same* frame ID (as in
`event_triggered.ini`, `sanity.ini`, `sleep-wakeup-etf.ini`). Anything
relying on `eventFrameConfiguration` to make one header ID fan out to
separate UCF IDs is not implemented. Anything relying on `wakeupTime` will
leave the bus asleep permanently once a `0x3C` frame with `data[0]=0` is
published.

## 9. Reading the output

Console (`Cmdenv`) output shows lines like:

```
>>> Starting Slot for Frame ID 0x20 [RESOLVING]
Bus: COLLISION DETECTED on Bus for ID 0x20
Received RESPONSE ID 0x21 Byte0=100
Received GO-TO-SLEEP Command. Entering Sleep Mode.
```

With `record-eventlog = true` (already set in most of these `.ini` files),
an `.elog` file is written under `results/` — open it in the IDE's **Event
Log** / **Sequence Chart** view to see Break → Header → Response timing and
any collisions visually.

## 10. Troubleshooting

- **`opp_makemake`/`opp_run` not found** — source your OMNeT++ environment
  script so its `bin/` is on `PATH`.
- **`Config file 'Makefile.inc' does not exist`** — same cause; `opp_configfilepath`
  can't find your OMNeT++ install.
- **A slave never responds / master keeps timing out on one ID** — check
  that some node's `signalConfiguration` actually has an entry for that
  exact frame ID (hex). Remember: publisher = "has a `signalConfiguration`
  entry for this ID", nothing else.
- **"Collision" never happens even though two slaves seem configured for
  it** — confirm both nodes list the *same* frame ID directly in
  `signalConfiguration`; an `eventFrameConfiguration`-style mapping will
  not create a collision (unimplemented — section 6).
- **Bus goes silent partway through the run** — a `0x3C` response with
  `data[0]=0` was published (intentionally or via a stray `signalConfiguration`
  entry) and put everything to sleep; nothing will wake it (section 6).
- **`Cannot find network 'src.LinNetwork'`** — run with `-n src` (or set
  `ned-path`) so the NED loader can see the files under `src/`.
- **Assigning `*.slaves[N].xxx` seems to have no effect** — check `*.numSlaves`
  is set high enough that `slaves[N]` actually exists (see the
  `power_window.ini` bug above).
