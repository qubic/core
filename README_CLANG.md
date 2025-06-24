# qubic clang port
The Qubic Core is currently ported to be compiled with clang. As this requires a lot of changes the port is develop along the main development work and continuously integrated. However this also implies that the compilation of the core code with clang is not resulting in a working node at the moment. Hence it shall now be used in production!


## Prerequisites
To be able to compile the core code with clang the following prerequisites have to be meet:

> Clang >= 18.1.0
> cmake >= 3.14
> nasm >= 2.16.03

Potentially it also works with lower versions but isn't tested yet.

## Compilation
Currently compilation with cmake and clang is only supported on Linux systems. Window and OSX might work but is not properly tested.
For a example compilation execute the following commands:

1.  **Navigate to the Project Root:**
    Open your terminal and change the directory to the root of the project where the main `CMakeLists.txt` file is located.

    ```bash
    cd /path/to/your/project
    ```

2.  **Create and Enter Build Directory:**
    It's standard practice to perform an "out-of-source" build. Create a build directory (e.g., `build`) and navigate into it.

    ```bash
    # Create the build directory if it doesn't exist
    mkdir -p build

    # Enter the build directory
    cd build
    ```

3. **Configure project**
    Instruct cmake to use `clang` and `clang++` as well configure the rest of the build.

    ```bash
    # In your build directory
    cmake .. -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++ -D BUILD_TESTS:BOOL=ON -D BUILD_EFI:BOOL=OFF -D CMAKE_BUILD_TYPE=Debug -D ENABLE_AVX512=ON
    ```

    There are a few option to set during configuration:

* **`-D BUILD_TESTS=<ON|OFF>`**
    * **Values:** `ON`, `OFF`
    * **Meaning:** `ON` builds the test suite, `OFF` skips building tests.

* **`-D BUILD_EFI=<ON|OFF>`**
    * **Values:** `ON`, `OFF`
    * **Meaning:** `ON` builds the EFI file, `OFF` skips building EFI file. Currently this build is not working.

* **`-D BUILD_BENCHMARK=<ON|OFF>`**
    * **Values:** `ON`, `OFF`
    * **Meaning:** `ON` builds a EFI file that allows to run a benchmark directly in the uefi. `OFF` skips building this EFI Benchmark.

* **`-D CMAKE_BUILD_TYPE=<Type>`**
    * **Values:** `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`
    * **Meaning:** Sets the build mode for optimization and debug info (e.g., `Debug` for debugging, `Release` for performance).

* **`-D ENABLE_AVX512=<ON|OFF>`**
    * **Values:** `ON`, `OFF`
    * **Meaning:** `ON` enables code using AVX-512 CPU instructions (requires support), `OFF` disables it.

* **`-D USE_SANITIZER=<ON|OFF>`**
    * **Values:** `ON`, `OFF`
    * **Meaning:** `ON` enables linking to santizers when building with clang, `OFF` disables sanitizers.

4.  **Build the Project:**
    Use the CMake `--build` command to invoke the underlying build tool (like `make` or `ninja`).

    ```bash
    # Build the project using the configuration generated in the previous step
    cmake --build .

    # Optional: Build in parallel (e.g., using 4 cores if using make)
    # cmake --build . -- -j4
    # Or if using Ninja (often the default if installed), it usually uses all cores by default
    ```


## State
At the moment the basic build scripts are setup and tests will be ported piece by piece.

Current state of the working tests:
| Test Module        | Source File             | Status      |
| :----------------- | :---------------------- | :---------- |
| Assets             | `assets.cpp`            | Pending     |
| Common Def         | `common_def.cpp`        | Pending     |
| Contract Core      | `contract_core.cpp`     | Pending     |
| Contract qEarn     | `contract_qearn.cpp`    | Pending     |
| Contract qVault    | `contract_qvault.cpp`   | Pending     |
| Contract qX        | `contract_qx.cpp`       | Pending     |
| KangarooTwelve     | `kangaroo_twelve.cpp`   | Pending     |
| M256               | `m256.cpp`              | **Working** |
| Math Lib           | `math_lib.cpp`          | **Working** |
| Network Messages   | `network_messages.cpp`  | **Working** |
| Platform           | `platform.cpp`          | Pending     |
| QPI Collection     | `qpi_collection.cpp`    | Pending     |
| QPI                | `qpi.cpp`               | Pending     |
| QPI Hash Map       | `qpi_hash_map.cpp`      | Pending     |
| Score Cache        | `score_cache.cpp`       | Pending     |
| Score              | `score.cpp`             | Pending     |
| Spectrum           | `spectrum.cpp`          | Pending     |
| Stdlib Impl        | `stdlib_impl.cpp`       | Pending     |
| Tick Storage       | `tick_storage.cpp`      | Pending     |
| TX Status Request  | `tx_status_request.cpp` | Pending     |
| Vote Counter       | `vote_counter.cpp`      | Pending     |
