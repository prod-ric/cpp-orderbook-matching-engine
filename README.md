# Matching Engine

A limit order book and matching engine built in C++ for learning systems programming and HFT concepts.

## Features

- **Limit orders** with price-time priority matching
- **Market orders** that match immediately against resting orders
- **Order cancellation**
- **Order book visualization** (best bid/ask, spread, depth)
- **Benchmark suite** for measuring throughput and latency

## Setup (macOS)

### Prerequisites

You need a C++ compiler and CMake. Install via Homebrew:

```bash
# Install Homebrew if you don't have it
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install build tools
brew install cmake
```

You also need Xcode Command Line Tools (for the C++ compiler):

```bash
xcode-select --install
```

### Build

```bash
# From the project root directory
mkdir build
cd build
cmake ..
make
```

### Run

```bash
# Run the demo
./matching_engine

# Run the tests
./tests

# Run the benchmark
./benchmark
```

## Project Structure

```
matching-engine/
├── CMakeLists.txt           # Build configuration
├── include/                 # Header files
│   ├── Types.h              # Common types (Price, Quantity, Side, etc.)
│   ├── Order.h              # Order struct
│   ├── Trade.h              # Trade struct
│   ├── OrderBook.h          # Order book (the core data structure)
│   └── MatchingEngine.h     # Engine (main interface)
├── src/                     # Implementation files
│   ├── main.cpp             # Demo program
│   ├── benchmark.cpp        # Performance benchmarking
│   ├── OrderBook.cpp        # Order book implementation
│   └── MatchingEngine.cpp   # Engine implementation
├── tests/                   # Tests
│   └── test_matching.cpp    # Correctness tests
└── data/                    # Historical data for replay (future)
```

## Roadmap

- [x] v1: Basic matching engine with limit/market orders
- [ ] v2: CSV replay of historical order data
- [ ] v3: Performance optimization (memory pools, cache-friendly structures)
- [ ] v4: Multithreading with lock-free queues
- [ ] v5: Network layer (TCP/UDP order submission)

