# FYP2_SimulationCode

This project simulates LTE handover mechanisms using the ns-3 network simulator. It compares the performance of two handover algorithms — A3-RSRP and A2-A4-RSRQ — under realistic traffic and mobility conditions. The simulation gathers key performance indicators such as downlink throughput, ANOH (Average Number of Handovers), and Optimization Ratio.

---

## 📦 Requirements

- **ns-3 version**: [ns-3.41](https://www.nsnam.org/releases/ns-3-41/)
- **OS**: Ubuntu 20.04 LTS or newer (recommended)
- **Compiler**: `g++` (version 9.4+)
- **Other tools**: `cmake`, `python3`, `git`

### Required ns-3 Modules

- `lte`
- `internet`
- `mobility`
- `applications`
- `point-to-point`
- `flow-monitor`
- `core`
- `network`

---

## 🔧 Setup Instructions

1. **Download and build ns-3.41**:

   ```bash
   git clone https://gitlab.com/nsnam/ns-3-dev.git ns-3.41
   cd ns-3.41
   git checkout ns-3.41
   ./ns3 configure --enable-examples --enable-tests
   ./ns3 build

2. **Add the simulation code**:

Move the provided simulation file into the `scratch/` directory of the ns-3 folder:

   ```bash
   cp /path/to/FYP2_SimulationCode.cpp scratch/
   ```

3. **Build the simulation**:

From the root directory of your `ns-3.41` folder, run:

```bash
./ns3 build
```
## ▶️ Run the Simulation

To execute the simulation with default parameters:

```bash
./ns3 run "scratch/FYP2_SimulationCode"
./ns3 run "scratch/FYP2_SimulationCode --useA2A4=true --enableFading=true"
./ns3 run "scratch/FYP2_SimulationCode --minSpeed=40 --maxSpeed=80 --simTime=50"
```

---

## 📋 Runtime Parameters

| Parameter               | Description                                        | Default    |
|------------------------|----------------------------------------------------|------------|
| `numberOfUes`          | Number of UEs in the simulation                    | 41         |
| `numberOfEnbs`         | Number of eNodeBs (7 sites × 3 sectors = 21 total) | 21         |
| `simTime`              | Duration of simulation in seconds                  | 50.0       |
| `useA2A4`              | Use A2-A4-RSRQ handover instead of A3-RSRP         | false      |
| `enableFading`         | Enable EVA/ETU fading using trace file             | false      |
| `txPower`              | eNB transmission power in dBm                      | 46.0       |
| `minSpeed`             | Minimum UE speed in km/h                           | 20.0       |
| `maxSpeed`             | Maximum UE speed in km/h                           | 120.0      |
| `fadingTrace`          | Path to fading trace file                          | `src/lte/model/fading-traces/fading_trace_EVA_60kmph.fad` |

---

## 📊 Output Metrics

Once the simulation finishes, the following results will be printed to your terminal:

- **Total Downlink Throughput** (in Mbps)
- **ANOH** (Average Number of Handovers per UE per second)
- **Optimization Ratio** (Throughput / ANOH)

These results help evaluate handover efficiency under different mobility and fading conditions.

