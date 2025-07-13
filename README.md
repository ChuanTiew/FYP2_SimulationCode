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
