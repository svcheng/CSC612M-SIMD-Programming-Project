# SIMD Vector Add–Multiply (A = B + C * D)

This project implements and benchmarks four kernels that compute, element-wise:

$A[i] = B[i] + C[i] * D[i]$, for $i$ in $[0, n)$.

Kernels:
1. **C reference** (`vecaddmulc`) – portable baseline in C.
2. **Scalar x86-64** (`vecaddmulx86_64`) – assembly with scalar `movss/mulss/addss`.
3. **SIMD XMM** (`vecaddmulxmm`) – 128-bit vectors (4 floats per step) with masked tail.
4. **SIMD YMM** (`vecaddmulymm`) – 256-bit vectors (8 floats per step) with masked tail.

A correctness pass prints sample outputs, and all kernels are benchmarked using the methods $QueryPerformanceQuery$ and $QueryPerformanceCounter$ from the $windows.h$ c library.

---

## Repository Layout
```
/ (root)
├─ main.c              # driver, timing, correctness checks
├─ vecaddmulx86_64.asm # scalar x86-64 kernel
├─ vecaddmulxmm.asm    # XMM, 128-bit kernel + tail mask table
├─ vecaddmulymm.asm    # YMM, 256-bit kernel + tail mask table
└─ README.md # this file
```

---

## What the Program Does
- Allocates A, B, C, D with n = 2^n floats (configurable in main.c through the constant variable `ARRAY_SIZE`).
- Initializes B, C, and D with sine, cosine, and tangent patterns, as shown below:
  - $B[i]=\sin(i*0.001)$
  - $C[i]=\cos(i*0.002)$
  - $D[i]=\tan(i*0.0005 + 1.0)$

- Correctness checks:
  - Computes and prints sample values from each kernel (full vector if $n \le 10$, otherwise first/last 5 values).
- Benchmarking:
  - Runs each kernel $k$ times (configurable in main.c through the constant variable `TEST_NUM`) and reports the average wall-time in milliseconds via $QueryPerformanceCounter$.

---

## Debug-Mode Benchmark Results

Performance in **Debug mode** was measured across progressively larger vector sizes.

| Input Size | Screenshot                                                                                                |
| ---------- | --------------------------------------------------------------------------------------------------------- |
| $2^{20}$   | <img width="824" height="529" alt="image" src="https://github.com/user-attachments/assets/b8d12ed6-72ec-45bd-97b7-4197329bf949" /> |
| $2^{26}$   | <img width="811" height="507" alt="image" src="https://github.com/user-attachments/assets/32f795a4-d8d8-4988-9e35-381c062bd7e2" /> |
| $2^{28}$   | <img width="812" height="524" alt="image" src="https://github.com/user-attachments/assets/75ed560d-ae0b-451f-8707-900cf1aa72fe" /> |

---

## Release-Mode Benchmark Results

The same tests were executed in **Release mode** (optimized build).
Results show substantial performance gains over Debug mode, validating proper compiler optimization and SIMD utilization.

| Input Size | Screenshot                                                                                                |
| ---------- | --------------------------------------------------------------------------------------------------------- |
| $2^{20}$   | <img width="814" height="523" alt="image" src="https://github.com/user-attachments/assets/9255c48d-cb8a-4143-833e-63a3ee8c7734" /> |
| $2^{26}$   | <img width="813" height="524" alt="image" src="https://github.com/user-attachments/assets/77747e4c-5633-402b-8806-009ee9ae038e" /> |
| $2^{28}$   | <img width="823" height="529" alt="image" src="https://github.com/user-attachments/assets/d8290c54-d1d7-4157-8ae3-87089cac31e2" /> |

---

## Boundary Checking

SIMD registers process multiple floats at once:

* **XMM** → 128 bits → **4 floats per step**
* **YMM** → 256 bits → **8 floats per step**

If the array size (`n`) **isn’t a multiple of 4 or 8**, there will be a few leftover elements (called the **remainder** or **tail**) that don’t fill a full SIMD register.
Without handling that remainder, it will either skip those elements or read/write past the end of the array — causing wrong results or a crash.

To handle remainders safely, the code includes a **boundary check** and uses **masking**.

1. **Loop Until Near the End:**
   Each kernel has a main loop that runs while `n ≥ 4` (for XMM) or `n ≥ 8` (for YMM).
   It processes full vectors using:

   ```
   vmovdqu   → load
   vmulps    → multiply
   vaddps    → add
   vmovdqu   → store
   ```

2. **Check for Remaining Elements:**
   When the remaining element count (`r13`) becomes smaller than 4 (for XMM) or 8 (for YMM), the code jumps to a label called `bound_check:`.

3. **Load the Right Mask:**
   Each `.asm` file has a small **mask_table** declared in `.rodata`, which is a special memory section in an assembly program that stores constant data that never changes while the program runs.

   * For XMM: 4 entries
   * For YMM: 8 entries

   Each mask entry tells the CPU which lanes (positions in the SIMD register) are valid.

   Example for XMM (4 floats):

   ```
   dd 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000  ; process only 1 lane
   dd 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000  ; process 2 lanes
   dd 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000  ; process 3 lanes
   ```

   The code picks the right mask using:

   ```asm
   lea rax, [mask_table]
   imul r13, 16 or 32   ; (16 bytes for XMM, 32 for YMM)
   vmovdqu xmm0/ymm0, [rax+r13]
   ```

   → This loads the correct mask for however many elements are left.

4. **Masked Load, Compute, Store:**
   Then, instead of a normal load/store, the code uses `vmaskmovps`:

   ```asm
   vmaskmovps xmm1, xmm0, [r8+rsi]   ; load only valid B values
   vmaskmovps xmm2, xmm0, [r9+rsi]   ; load only valid C values
   vmaskmovps xmm3, xmm0, [rbx+rsi]  ; load only valid D values

   vmulps xmm4, xmm2, xmm3
   vaddps xmm4, xmm1, xmm4

   vmaskmovps [rdx+rsi], xmm0, xmm4  ; store only valid A values
   ```

   * The **mask (`xmm0` or `ymm0`)** tells which lanes are real data and which should be ignored.
   * Invalid lanes aren’t read or written — so the program never go out of bounds.

In simple words, think of it like filling boxes of 4 (for XMM) or 8 (for YMM).
If your total number of items doesn’t fit perfectly into full boxes, the last box isn’t full, and you only open slots for the remaining items.

* The **mask table** says which slots are still “real”.
* `vmaskmovps` makes sure you **only read/write** those valid slots.
* That’s what makes your program safe and correct even when `n` isn’t divisible by 4 or 8.

To verify correct tail-masking behavior for vector sizes not divisible by 4 (XMM) or 8 (YMM), several non-power-of-two input sizes were tested.
Each result shown below confirms proper boundary handling with no segmentation faults or incorrect residual values.

### Input Size: 1003

<img src="https://github.com/user-attachments/assets/80537612-7e63-48f0-a6e9-05ba7c7df060" width="850" />

### Input Size: $2^{10}$ + 3

<img src="https://github.com/user-attachments/assets/ce096108-a53c-47ca-b289-eb11d50d112c" width="850" />

### Input Size: $2^{20}$ + 7

<img src="https://github.com/user-attachments/assets/7108afff-e809-43e5-968d-cc28473bd12a" width="850" />

---

## Summary Table of Execution Times

*(All values in milliseconds; rounded to 4 decimal places)*

### **Debug Mode Results**

| **Input Size** | **C (Baseline)** | **x86 (Scalar ASM)** | **SIMD XMM (128-bit)** | **SIMD YMM (256-bit)** |
| :------------: | ---------------: | -------------------: | ---------------------: | ---------------------: |
|     $2^{20}$   |           2.6223 |               1.1926 |             **0.9538** |                 1.0293 |
|     $2^{26}$   |         157.5850 |              72.0365 |                66.9026 |            **64.4475** |
|     $2^{28}$   |         638.6596 |             328.8114 |               286.1022 |           **283.1424** |

### **Release Mode Results**

| **Input Size** | **C (Baseline)** | **x86 (Scalar ASM)** | **SIMD XMM (128-bit)** | **SIMD YMM (256-bit)** |
| :------------: | ---------------: | -------------------: | ---------------------: | ---------------------: |
|    $2^{20}$    |           1.6153 |               1.6468 |             **0.7475** |                 0.7908 |
|    $2^{26}$    |          88.1513 |             120.4012 |            **78.8263** |                82.9230 |
|    $2^{28}$    |         288.6141 |             397.4300 |               264.9209 |           **261.5920** |

### Key Observations

* Execution time consistently increases with input size.
* SIMD (XMM/YMM) kernels consistently outperform non-parallelized implementations.
* In Release mode, x86 scalar assembly becomes **slower** than its C counterpart because the compiler optimizes C aggressively, while manual assembly remains the same.

---

## Comparative Analysis

### Speedup

A comparison table is presented below for clearer visualization of the performance differences among the four kernels. The average execution times were measured in Debug Mode using an input size of $2^{28}$ elements. The largest input size was selected to ensure that each kernel processed a sufficiently large dataset, thereby highlighting differences in performance under realistic, high-load conditions. Meanwhile, Debug Mode was used to provide a more transparent view of the kernels' performances, minimizing compiler optimizations that could obscure the inherent performance characteristics of each kernel. 

| Kernel                   | Description                                            | Average Time in Debug Mode (ms) | Relative Speed (vs. C) |
| ------------------------ | ------------------------------------------------------ | ------------------------------- | ---------------------- |
| **C Reference Kernel**   | Scalar C implementation — baseline                     | **638.6596**                    | 1.00× (baseline)       |
| **x86-64 Scalar Kernel** | Manual assembly using scalar `movss`, `mulss`, `addss` | **328.8114**                    | ~1.9× faster           |
| **SIMD XMM (128-bit)**   | Vectorized 4-wide floats + masked tail                 | **286.1022**                    | ~2.2× faster           |
| **SIMD YMM (256-bit)**   | Vectorized 8-wide floats + masked tail                 | **283.1424**                    | ~2.3× faster           |

#### 1. C Reference Kernel

The pure C version (`vecaddmulc`) serves as the **baseline** implementation. It processes one element per loop iteration, relying entirely on the compiler’s scalar floating-point operations. It is limited by the fact that iterations execute serially, with no explicit instruction-level parallelism.

#### 2. x86-64 Scalar Assembly Kernel

The scalar assembly (`vecaddmulx86_64`) performs the same operation manually using x86-64 registers (`xmm0–xmm2`) with scalar float instructions (`movss`, `mulss`, `addss`).
The key performance difference comes from:

* Reduced overhead from tighter loop and manual register management.
* Less compiler-inserted code since there is no function call abstraction.

However, it still executes one float at a time, meaning no vectorization or parallelization occurs. Its ~1.9× speedup mainly comes from eliminating C-level overhead and optimizing the memory use.

#### 3. SIMD XMM (128-bit) Kernel

The XMM kernel (`vecaddmulxmm`) improves upon the scalar assembly kernel by introducing **data-level parallelism**. Each 128-bit XMM register processes **four floats simultaneously** using vectorized instructions (`vmulps`, `vaddps`).
Key optimizations:

* Dramatically increased throughput from four floats being processed simultaneously.
* **Unaligned loads/stores (`vmovdqu`)**, allowing flexible memory access.

The performance gain over scalar assembly kernel (~2.2x instead of ~1.9x speedup) shows there is a performance benefit to using SIMD registers, though the improvement is slightly limited by:

* **Masking overhead** (for the tail).
* **Partial utilization** if data isn’t perfectly aligned to cache lines.
* Slight **loop overhead** remaining from the iteration control.

#### 4. SIMD YMM (256-bit) Kernel

The YMM kernel (`vecaddmulymm`) uses the YMM registers instead of XMM, which can store 256 bits instead of 128, and process eight floats per iteration instead of 4.
In theory, this effectively doubles the data throughput compared to XMM, however our test results only show a slight improvement over the XMM kernel (~2.3x speedup vs. ~2.2x).

Possible reasons for the gain being small:

* **Memory bandwidth** becomes a limiting factor. The CPU can only fetch/store so fast, even if arithmetic doubles.
* **Instruction decoding and loop control** still consume cycles.
* **Cache and prefetch efficiency**: larger registers can increase pressure on memory bandwidth for large datasets.
* **Randomness** in execution times resulting from nondeterministic process scheduling
* **Hardware/OS-specific quirks**: tests were run on a Windows machine with an Intel i5-9300H CPU.

Despite these constraints, YMM delivers the fastest performance overall, maintaining correctness and stable runtime across tests.

### Debug Mode vs. Release Mode Behavior

The performance of the x86-64 scalar kernel performed strangely in release mode. While it consistently outperformed the C kernel in debug mode, it was consistently **outperformed** by C in release mode. This can be explained by the fact the in release mode, the compiler aggressively optimizes C functions (e.g. through loop unrolling, instruction scheduling, vectorization, and register reuse), whereas assembly code is not modified much or at all. The optimizations can be enough for the C version of the kernel to outperform a non-parallel assembly implementation. This finding highlights a valuable insight:

> **Handwritten scalar assembly is not always faster than compiler-optimized C**, especially for simple arithmetic operations where the compiler can automatically apply SIMD vectorization and instruction scheduling.

Another interesting observation that can be made is that the x86-64 scalar assembly kernel actually ran **slower relative to itself** in release mode than in debug mode. Though the other assembly kernels were also sometimes slower in release mode, the x86-64 scalar kernel was **consistently** slower. This may just be a result of randomness, however, a technical reason that might explain why this occurred is the **stack frame and alignment differences** in release mode. Release mode often omits frame pointers and may realign stack variables for optimization. Since the assembly kernel accesses parameters directly from the stack (e.g., `[rbp+32]`), these differences can slightly affect memory access latency, causing slowdowns.
 
---

## Discussion: Process, Problems, and AHA Moments

### Methodology

The project followed the following methodology:

1. Start by ensuring correctness by implementing a simple, reliable baseline in C.
2. Port to assembly — reproduce the same logic using scalar instructions.
3. Parallelize — introduce vectorization by using SIMD XMM registers, verifying correctness and making sure to handle boundary conditions.
4. Extend with (YMM) — use SIMD registers with double the width and refine mask logic to maintain correctness when boundary elements are present.
5. Benchmark and compare — use `QueryPerformanceCounter` from `Windows.h` to obtain high-resolution execution times in order to measure speedups and compare performance between kernels.

This systematic, bottom-up approach ensured each kernel was correctly implemented.

### Problems Encountered and How They Were Solved

**1. Boundary Handling for Non-Divisible Vector Sizes**

* **Problem:** When `n` wasn’t divisible by 4 (XMM) or 8 (YMM), partial vectors caused either incorrect results or memory violations.
* **Solution:** Implemented **mask tables** in `.rodata` and used `vmaskmovps` for conditional load/store. This prevented invalid memory access while maintaining parallelism.
* **AHA moment:** realizing `vmaskmovps` could safely handle tails without scalar fallback loops.



**2. Stack Frame & Calling Convention (Win64 ABI)**

* **Problem:** The fifth argument (`D`) isn’t passed in a register in Win64; it must be retrieved from the stack. Early attempts did not properly retrieve the parameter from the stack, causing crashes.
* **Solution:** Corrected stack offset handling in assembly (`[rbp+32]`), ensuring the proper retrieval of pointer arguments.
* **AHA moment:** understanding how the **Windows x64 calling convention** works (RCX, RDX, R8, R9, then stack).



**3. Measuring and Comparing Execution Time Accurately**

* **Problem:** Timings fluctuate heavily between runs.
* **Solution:** Repeated the test for each kernel 30× then averaged results, and used `QueryPerformanceFrequency` for high resolution measurements.
* **AHA moment:** discovering that micro-benchmarks require averaging to work around inconsistent timings and background "jitter".

### Overall Reflections & Learning Outcomes

* Using SIMD parallelism is beneficial for reducing execution time, even compared to optimized C code.
* Real-world performance is not exactly the same as theoretical performance. Due to external factors like the compiler, hardware, OS, programs may perform slower or faster than we expect.
* The techniques used to compare the runtimes of different methods must be adapted to deal with inconsistency (e.g. averaging over many runs).

---

## Declaration of AI Usage

This project utilized **OpenAI’s GPT-5 (ChatGPT)** solely for the purpose of improving the **documentation, structure, and readability** of this repository — particularly the GitHub README.

AI assistance was confined to:

* Rewording explanations for clarity and conciseness
* Formatting Markdown elements (headings, tables, and summary boxes)
* Structuring the comparative analysis and discussion sections for readability
* Creating a consistent academic and technical tone across the documentation

The AI did **not** generate or influence any conceptual, mathematical, or implementation-level content. It was purely used as a writing and formatting aid.
All project ideas, algorithms, assembly code, analyses, interpretations, etc. were entirely formulated by us.

### Prompts Used

To ensure transparency, below are examples of actual prompts used when refining documentation and explanations:

1. *“Can you rephrase this paragraph for the README to make it more formal and concise?”*
2. *“Format this benchmark table neatly for GitHub Markdown.”*
3. *“Turn my bullet points on the comparison of the four kernels (C, x86, XMM, YMM) to a neat section.”*
4. *“Improve the README structure of the Analysis and Discussion section for clarity, keeping the technical meaning intact.”*

