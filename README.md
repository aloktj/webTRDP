---

# 📘 webTRDP – Project Overview

This project implements a **generic TRDP (Train Real-time Data Protocol) Simulator** with a modern **web UI**, built around the TCNopen TRDP stack.
It consists of three major components:

```
trdp-core/   – C++ TRDP engine (XML loader, PD/MD runtime)
backend/     – C++ Drogon webserver (REST + WebSocket APIs)
frontend/    – React + Vite web UI
external/    – Git submodules (TRDP, Drogon, fmt, spdlog, gtest, etc.)
```

The simulator loads TRDP XML configuration files, starts TRDP communication threads, shows decoded telegrams in a web UI, allows editing outgoing PD values, and manages multiple XML configurations.

This README defines the **reference architecture**, **data models**, and **expected behavior** that the AI (Codex) should follow while generating code.

---

# 📂 Repository Structure

```
/ (project root)
  README.md
  CMakeLists.txt
  cmake/
  external/
    TCNopen/       ← TRDP stack (git submodule)
    drogon/        ← Drogon framework (optional submodule)
    json/          ← nlohmann/json
    fmt/           ← fmtlib
    spdlog/        ← spdlog
    googletest/    ← gtest (optional)
  trdp-core/
    include/
    src/
    tests/
  backend/
    include/
    src/
  frontend/
    package.json
    vite.config.*
    src/
  scripts/
    *.sh
```

---

# 🧠 System Architecture

## 1. TRDP Engine (`trdp-core/`)

The TRDP engine is a standalone C++ library responsible for:

* Loading TRDP configuration from XML using **tau_xml**:

  * `tau_prepareXmlDoc`
  * `tau_readXmlDeviceConfig`
  * `tau_readXmlInterfaceConfig`
  * `tau_readXmlDatasetConfig`
* Parsing:

  * Interfaces
  * PD telegram definitions
  * MD telegram definitions (future)
  * Dataset definitions
* Managing TRDP runtime:

  * Running TRDP in **threaded mode** so TRDP handles:

    * PD receive
    * MD receive
    * MD timeouts and retries
  * Maintaining internal state for each telegram:

    * Tx payload
    * Rx payload
    * Timing (cycle, last period, avg period)
    * Stats (rx/tx count, timeout count)
  * Running a **PD transmit scheduler thread**

    * All outgoing PD telegrams are sent by this scheduler
    * TRDP does NOT send PD automatically
* Decoding received PD payloads based on dataset definitions.
* Encoding outgoing PD payloads based on dataset definitions.
* Exposing thread-safe snapshots for the backend.

### TRDP Engine Main Class

```cpp
class TrdpEngine {
public:
    void loadConfig(const std::string& xml_path, const std::string& host_name);
    void start();
    void stop();

    std::vector<PdRuntime> getPdSnapshot() const;

    void enablePd(uint32_t com_id, bool enable);
    void setPdValues(uint32_t com_id, const std::map<std::string, double>& values);
};
```

### Key Data Structures

```cpp
enum class Direction { Source, Sink, SourceSink };

struct DatasetElement {
    std::string name;
    uint32_t    type;
    uint32_t    array_size;
};

struct Dataset {
    uint32_t id;
    std::string name;
    std::vector<DatasetElement> elements;
};

struct InterfaceDef {
    std::string name;    
    uint32_t    network_id;
    std::string host_ip;
};

struct PdTelegramDef {
    std::string name;
    uint32_t    com_id;
    uint32_t    dataset_id;
    Direction   direction;
    uint32_t    cycle_us;
    bool        marshall;
    std::string interface_name;
};

struct PdRuntime {
    const PdTelegramDef* def;

    std::vector<uint8_t> tx_payload;
    bool tx_enabled = false;

    std::chrono::steady_clock::time_point next_tx_due{};

    std::vector<uint8_t> last_rx_payload;
    std::chrono::steady_clock::time_point last_rx_time{};
    bool last_rx_valid = false;

    uint64_t tx_count = 0;
    uint64_t rx_count = 0;
    uint64_t timeout_count = 0;

    double last_period_us = 0.0;
    double avg_period_us  = 0.0;
};
```

TRDP callbacks update `PdRuntime`, and the scheduler reads `tx_payload` to send telegrams.

---

# 🌐 Backend (`backend/`)

Backend is built with **Drogon C++ framework**.

Responsibilities:

* Own a single global `TrdpEngine` instance.
* Expose **REST APIs**:

### 🔹 API Endpoints

#### Config Management

```
POST /api/configs/load
GET  /api/configs
GET  /api/configs/{id}/summary
POST /api/configs/{id}/activate
```

#### PD Telegrams

```
GET    /api/pd/telegrams
POST   /api/pd/{com_id}/enable
PATCH  /api/pd/{com_id}/values
```

Payloads are always JSON.

Backend converts runtime C++ structs → JSON like:

```json
{
  "interface": "eth0",
  "com_id": 1001,
  "name": "tlg1001",
  "dataset_id": 1001,
  "direction": "SOURCE",
  "cycle_us": 10000,
  "stats": { ... },
  "last_rx": {
    "timestamp": "...",
    "valid": true,
    "raw_hex": "00AF10...",
    "decoded_fields": [
      { "name": "u8_A", "type": "UINT8", "value": 12 }
    ]
  }
}
```

WebSocket endpoints will be added later for live updates.

---

# 🖥 Frontend (`frontend/`)

Frontend is a **React + Vite** SPA.

Pages:

* **Dashboard**
* **PD View**

  * List of PD telegrams
  * Detail panel with decoded fields + raw payload + stats
  * Controls:

    * Enable/disable Tx
    * Edit outgoing values
* **MD View** (future)
* **Configuration Manager**

  * Upload XML
  * View summary
  * Activate configuration
* **Stats & Logs** (future)

Frontend consumes the REST API and later WebSocket events.

---

# 🧩 Development Guidelines for Codex

Codex must:

* Keep code inside the correct module:

  * Never mix backend logic into `trdp-core`.
  * Never use TRDP APIs inside the frontend or backend directly.
  * The backend must communicate *only* via the public `TrdpEngine` API.
* Maintain thread-safety in `trdp-core`.
* Use JSON consistently for all APIs.
* Follow CMake best practices with separate targets.
* Avoid unnecessary abstractions; keep the system simple and readable.
* Generate code using C++17/20 and idiomatic modern patterns.
* Do not duplicate logic across modules.

---

# 🚀 Build & Run

To initialize submodules:

```bash
git submodule update --init --recursive
```

To build everything:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

Run backend:

```bash
./backend/trdp-backend
```

Run frontend:

```bash
cd frontend
npm install
npm run dev
```

---

# ✔ Status

We build the project **in phases**:

1. Repo skeleton
2. XML loader
3. TRDP engine core
4. Drogon backend APIs
5. React frontend
6. MD support
7. WebSocket live updates

Codex will receive tasks **phase-by-phase**.

---
