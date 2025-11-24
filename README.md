# webTRDP

Repository skeleton for a TRDP simulator composed of a C++ core library, Drogon backend, and React + Vite frontend.

## Layout

```
/ (project root)
  CMakeLists.txt       # Builds the C++ targets
  cmake/               # Shared CMake helper modules
  trdp-core/           # Core TRDP engine library (static)
  backend/             # Drogon HTTP/WebSocket server
  frontend/            # React + Vite single page app
  scripts/             # Helper scripts to run the backend/frontend
  third_party/         # Git submodules (Drogon, TRDP, fmt, etc.)
```

## Building the backend

```bash
mkdir -p build
cmake -S . -B build
cmake --build build --target trdp-backend
```

Run the backend executable:

```bash
./build/backend/trdp-backend
```

## Running the frontend

```bash
cd frontend
npm install
npm run dev -- --host
```

## Notes

* `trdp-core` is a standalone static library and should remain independent from web concerns.
* The backend links against `trdp-core` and Drogon.
* The frontend is managed with Vite and React and served separately from the C++ build.
