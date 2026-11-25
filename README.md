# LIN Bus Simulator for OMNeT++
 
An open-source model of the Local Interconnect Network (LIN) protocol for the OMNeT++ discrete-event network simulator. It follows the timing and framing rules of ISO 17987-3 (LIN 2.x) and is configured entirely through `.ini` files.
 
## Features
 
- Master and slave nodes with a master-driven schedule table
- Frame framing and timing derived from the baud rate: break, sync byte, protected identifier (with parity), data field and checksum
- Classic checksum for diagnostic frames (0x3C, 0x3D) and enhanced checksum for all other frames
- Event-triggered frames, collision detection on the bus, and a master collision-resolution schedule
- Go-to-sleep command, wake-up pulse detection, wake-up retries, and inactivity-based sleep
- Fault injection on the bus: frame drop rate and frame corruption rate, with error reporting through signals
- Master response timeout handling
- Optional LIN diagnostics (single frames, disabled by default)
## Requirements
 
- OMNeT++ 6.0.3 (https://omnetpp.org/download/)
- A C++ compiler toolchain supported by OMNeT++ (installed with OMNeT++ as described in its installation guide)
## Setup
 
1. Install OMNeT++ 6.0.3 and verify that the environment is active (`opp_run -v` should print the version).
2. Start the OMNeT++ IDE and create a new project named `linbus`: **File > New > OMNeT++ Project**, choose an empty project.
3. Make sure the project contains a folder named `src`. The NED files declare `package src;`, so the folder name must be exactly `src`. If the wizard did not create it, create it manually.
4. Copy the repository contents into the project:
   - `src/` (all `.cc`, `.h`, `.ned` and `.msg` files) goes into the project's `src` folder.
   - The `.ini` files go into the project root.
5. Build the project: **Project > Build Project**.
Command-line alternative, from the project root:
 
```
opp_makemake -f --deep -o linbus -O out -I src src/*.cc
make
```
 
If the `.msg` file is changed, regenerate the Makefile with the command above before building.
 
## Running
 
### From the IDE
 
Right-click an `.ini` file and choose **Run As > OMNeT++ Simulation**. For files with several configurations, select the configuration in the run dialog.
 
### From the command line
 
From the project root:
 
```
./linbus -u Cmdenv -n . -f full-test.ini -c T6_ETF_Collision
```
 
Use `-u Qtenv` instead of `-u Cmdenv` for the graphical runtime. Results are written to `results/`.
 
### Configurations
 
`omnetpp.ini` contains a default two-slave network for a quick first run. `full-test.ini` contains the demonstration scenarios:
 
| Configuration | What it shows |
|---|---|
| `T1_UCF_Nominal` | Unconditional frames on a nominal schedule |
| `T3_DLC_Stress` | Frames with DLC 1, 2, 4 and 8 and the resulting response times |
| `T5_Diagnostics_SF` | Diagnostic single frames (diagnostics enabled) |
| `T14_DiagDisabled_GoToSleep` | Go-to-sleep handling with diagnostics disabled |
| `T6_ETF_Collision` | Event-triggered frame collision and the collision-resolution schedule |
| `T8_ErrorInjection` | Frame drop and corruption with master timeouts |
| `T10_SleepWakeup` | Go-to-sleep command, wake-up pulse, and schedule restart |
| `T12_InactivitySleep` | Inactivity timer configuration |
 
Scenarios T3, T6, T8 and T10 are the ones described in the accompanying paper.
 
## Configuration
 
All parameters are set per module in the `.ini` file, for example `*.master.scheduleConfiguration`. The network is `src.LinNetwork`; the number of slaves is set with `*.numSlaves` (default 2).
 
### Node parameters (`LinNode`)
 
| Parameter | Default | Description |
|---|---|---|
| `isMaster` | `false` | Marks the node as the master |
| `nodeAddress` | `-1` | Node address (used by diagnostics) |
| `scheduleConfiguration` | `""` | Master schedule, entries `ID:Name:slotSeconds` separated by `;` (hexadecimal IDs), e.g. `"10:Event:0.05; 30:Status:0.05"` |
| `collisionScheduleConfiguration` | `""` | Schedule run after an event-triggered collision, same format |
| `collisionStartDelay` | `-1s` | Start of the collision schedule; a negative value means the next slot boundary |
| `eventFrameConfiguration` | `""` | Maps an event-triggered ID to its unconditional frame IDs, e.g. `"20:21,22"` |
| `eventProbability` | `1.0` | Probability that a slave answers an event-triggered frame |
| `eventRearmPeriod` | `-1s` | Re-arm period for event triggers |
| `signalConfiguration` | `""` | Data a node publishes per frame ID, entries `ID:byteIndex:value` |
| `errorSignalConfiguration` | `""` | Maps the last detected error into a data byte of a frame |
| `frameDlcConfiguration` | `""` | Data length per frame ID |
| `breakBits`, `delimiterBits` | `13`, `1` | Break and break-delimiter length in bit times |
| `wakeupTime` | `-1s` | Time at which the node issues a wake-up pulse; negative disables it |
| `wakeupDetectThreshold` | `150us` | Minimum pulse length detected as a wake-up |
| `masterWakeupDelay` | `2ms` | Delay before the master restarts its schedule after wake-up |
| `wakeupExpiryTime` | `150ms` | Wake-up request expiry |
| `inactivitySleepTime` | `4s` | Bus inactivity before a node goes to sleep |
| `enableDiagnostics` | `false` | Enables the diagnostic frame handling |
| `diagTargetAddress` | `2` | Target address for master diagnostic requests |
 
### Bus parameters (`LinBusLogic`)
 
| Parameter | Default | Description |
|---|---|---|
| `baudRate` | `19200bps` | Bus bit rate |
| `propDelay` | `0.0001s` | Propagation delay |
| `interByteTime` | `0.0001s` | Inter-byte space |
| `frameErrorRate` | `0` | Probability that a transmitted frame is corrupted |
| `frameDropRate` | `0` | Probability that a transmitted frame is dropped |
 
### Example
 
```
[General]
network = src.LinNetwork
sim-time-limit = 2s
*.numSlaves = 2
 
*.master.isMaster = true
*.master.scheduleConfiguration = "21:Slave0:0.05; 22:Slave1:0.05"
*.slaves[0].signalConfiguration = "21:0:17"
*.slaves[1].signalConfiguration = "22:0:34"
*.bus.frameErrorRate = 0.0
```
 
## Modeling scope
 
The model works at frame level on a single shared bus. Bit-level wired-AND arbitration and read-back are not modeled; a collision is detected at the bus, where the first response is flagged and the second is discarded. The sync byte is carried in the header but not verified. Sporadic frames, node configuration services, and LDF import are not included. Timing values are derived from the specification and have not been calibrated against hardware captures.
 
## Repository layout
 
```
src/                  Simulator source
  LinNode.*           Master and slave node model
  LinBusLogic.*       Bus model: forwarding, collisions, fault injection
  LinNetwork.ned      Network topology
  LinDataFrame.msg    Break, header and response messages
  LinUtil.*           PID parity and checksum helpers
omnetpp.ini           Default quick-start configuration
full-test.ini         Demonstration scenarios
```
 
## Citation
 
If you use this simulator in academic work, please cite the accompanying paper (reference to be added after publication).
 
