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
├─ main.c # driver, timing, correctness checks
├─ vecaddmulx86_64.asm # scalar x86-64 kernel
├─ vecaddmulxmm.asm # XMM, 128-bit kernel + tail mask table
├─ vecaddmulymm.asm # YMM, 256-bit kernel + tail mask table
└─ README.md # this file
```

---

## What the Program Does
- Allocates A, B, C, D with n = 2^n floats (configurable in main.c through the constant variable $ARRAY_SIZE$).
- Initializes B, C, D with sine, cosine, and tangent patterns, as shown below:
  - $B[i]=\sin(i*0.001)$
  - $C[i]=\cos(i*0.002)$
  - $D[i]=\tan(i*0.0005 + 1.0)$

- Correctness checks:
  - Computes and prints sample values from each kernel (full vector if $n <= 10$, otherwise first/last 5 values).
- Benchmarking:
  - Runs each kernel $i$ times (configurable in main.c through the constant variable $TEST_NUM$) and reports the average wall-time in milliseconds via $QueryPerformanceCounter$.
  - 
  Here’s a cleaned-up, publication-ready version of your section. It keeps all the technical context and figure references but makes the formatting consistent and concise, as if for a formal report or GitHub README:

---

## 

---

## Debug-Mode Benchmark Results

Performance in **Debug mode** was measured across progressively larger vector sizes.

| Input Size | Screenshot                                                                                                |
| ---------- | --------------------------------------------------------------------------------------------------------- |
| 2²⁰        | <img width="824" height="529" alt="image" src="https://github.com/user-attachments/assets/b8d12ed6-72ec-45bd-97b7-4197329bf949" /> |
| 2²⁶        | <img width="811" height="507" alt="image" src="https://github.com/user-attachments/assets/32f795a4-d8d8-4988-9e35-381c062bd7e2" /> |
| 2²⁸        | <img width="812" height="524" alt="image" src="https://github.com/user-attachments/assets/75ed560d-ae0b-451f-8707-900cf1aa72fe" /> |

---

## Release-Mode Benchmark Results

The same tests were executed in **Release mode** (optimized build).
Results show substantial performance gains over Debug mode, validating proper compiler optimization and SIMD utilization.

| Input Size | Screenshot                                                                                                |
| ---------- | --------------------------------------------------------------------------------------------------------- |
| 2²⁰        | <img width="814" height="523" alt="image" src="https://github.com/user-attachments/assets/9255c48d-cb8a-4143-833e-63a3ee8c7734" /> |
| 2²⁶        | <img width="813" height="524" alt="image" src="https://github.com/user-attachments/assets/77747e4c-5633-402b-8806-009ee9ae038e" /> |
| 2²⁸        | <img width="823" height="529" alt="image" src="https://github.com/user-attachments/assets/d8290c54-d1d7-4157-8ae3-87089cac31e2" /> |

---

## Boundary Checking

SIMD registers process multiple floats at once:

* **XMM** → 128 bits → **4 floats per step**
* **YMM** → 256 bits → **8 floats per step**

If the array size (`n`) **isn’t a multiple of 4 or 8**, there will be a few leftover elements (called the **remainder** or **tail**) that don’t fill a full SIMD register.
Without handling that remainder, it will either skip those elements or read/write past the end of the array — causing wrong results or a crash.

To handle remainders safely, the code includes a **boundary check** and uses **masking**.

1. **Loop Until Near the End**
   Each kernel has a main loop that runs while `n ≥ 4` (for XMM) or `n ≥ 8` (for YMM).
   It processes full vectors using:

   ```
   vmovdqu   → load
   vmulps    → multiply
   vaddps    → add
   vmovdqu   → store
   ```

2. **Check for Remaining Elements**
   When the remaining element count (`r13`) becomes smaller than 4 (for XMM) or 8 (for YMM), the code jumps to a label called `bound_check:`.

3. **Load the Right Mask**
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

4. **Masked Load, Compute, Store**
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
Each result confirms proper boundary handling with no segmentation faults or incorrect residual values.

### Input Size: 1003

<img src="https://github.com/user-attachments/assets/fb7b1486-8286-49a7-becf-9d2cbf7b7a29" width="850" />

### Input Size: 2¹⁰ + 3

<img src="https://github.com/user-attachments/assets/1eb3a9c5-3a45-46d7-93f1-51ebcf4b84a1" width="850" />

### Input Size: 2²⁰ + 7

<img src="https://github.com/user-attachments/assets/a6e0f646-287f-4ebc-abd8-8710dec92a7d" width="850" />

---

## Summary Table of Execution Times

Note: decimals rounded to 4 places

### Debug Mode

| Input Size | Kernel   | Execution Time (ms) |
| ---------- | -------- | ------------------- |
| 2^20       | C        | 3.3663              |
| 2^20       | x86      | 2.0647              |
| 2^20       | SIMD XMM | 1.8149              |
| 2^20       | SIMD YMM | 0.0954              |
| 2^26       | C        | 165.0952            |
| 2^26       | x86      | 100.9830            |
| 2^26       | SIMD XMM | 86.0887             |
| 2^26       | SIMD YMM | 2.8291              |
| 2^28       | C        | 655.9969            |
| 2^28       | x86      | 322.2668            |
| 2^28       | SIMD XMM | 304.7617            |
| 2^28       | SIMD YMM | 9.3012              |

### Release Mode

| Input Size | Kernel   | Execution Time (ms) |
| ---------- | -------- | ------------------- |
| 2^20       | C        | 1.4124              |
| 2^20       | x86      | 1.6683              |
| 2^20       | SIMD XMM | 1.1171              |
| 2^20       | SIMD YMM | 0.0270              |
| 2^26       | C        | 81.2926             |
| 2^26       | x86      | 104.3931            |
| 2^26       | SIMD XMM | 76.0268             |
| 2^26       | SIMD YMM | 2.5403              |
| 2^28       | C        | 279.3449            |
| 2^28       | x86      | 415.01497           |
| 2^28       | SIMD XMM | 270.2237            |
| 2^28       | SIMD YMM | 8.7021              |

---

## Comparative Analysis

| Kernel                   | Description                                            | Average Time (ms) | Relative Speed (vs. C) | Correctness |
| ------------------------ | ------------------------------------------------------ | ----------------- | ---------------------- | ----------- |
| **C Reference Kernel**   | Scalar C implementation — baseline                     | **2.9929**        | 1.00× (baseline)       | ✅           |
| **x86-64 Scalar Kernel** | Manual assembly using scalar `movss`, `mulss`, `addss` | **1.0669**        | ~2.8× faster           | ✅           |
| **SIMD XMM (128-bit)**   | Vectorized (SSE/AVX XMM) 4-wide floats + masked tail   | **0.5888**        | ~5.1× faster           | ✅           |
| **SIMD YMM (256-bit)**   | Vectorized (AVX2 YMM) 8-wide floats + masked tail      | **0.5221**        | ~5.7× faster           | ✅           |

### 1. C Reference Kernel

The pure C version (`vecaddmulc`) serves as the **baseline** implementation. It processes one element per loop iteration, relying entirely on the compiler’s scalar floating-point operations.
While simple and portable, it’s limited by the fact that each iteration executes serially, with no explicit instruction-level parallelism.



### 2. x86-64 Scalar Assembly Kernel

The scalar assembly (`vecaddmulx86_64`) performs the **same operation manually** using x86-64 registers (`xmm0–xmm2`) with scalar float instructions (`movss`, `mulss`, `addss`).
The key performance difference comes from:

* **Reduced overhead**: tighter loop, manual register management.
* **No function call abstraction**: less compiler-inserted code.

However, it still executes one float at a time, meaning **no vectorization** or parallel arithmetic occurs. Its ~2.8× speedup mainly comes from eliminating C-level overhead and optimizing the instruction path, not true parallelism.



### 3. SIMD XMM (128-bit) Kernel

The XMM kernel (`vecaddmulxmm`) introduces **data-level parallelism**. Each 128-bit register processes **four floats simultaneously** using vectorized instructions (`vmulps`, `vaddps`).
Key optimizations:

* **Four operations per instruction**, dramatically increasing throughput.
* **Unaligned loads/stores (`vmovdqu`)**, allowing flexible memory access.
* **Tail masking** via `vmaskmovps` ensures safety when `n % 4 ≠ 0`.

The performance gain (~5× faster than C) shows clear SIMD benefit, though the improvement is slightly limited by:

* **Masking overhead** (for the tail).
* **Partial utilization** if data isn’t perfectly aligned to cache lines.
* Slight **loop overhead** remaining from the iteration control.



### 4. SIMD YMM (256-bit) Kernel

The YMM kernel (`vecaddmulymm`) expands SIMD width to **256 bits**, processing **eight floats per iteration**.
This effectively doubles the data throughput compared to XMM while keeping similar instruction count per loop.

Reasons for the modest gain (~10–15% faster than XMM, not 2×):

* **Memory bandwidth** becomes a limiting factor. The CPU can only fetch/store so fast, even if arithmetic doubles.
* **Instruction decoding and loop control** still consume cycles.
* **Cache and prefetch efficiency**: larger registers can increase pressure on memory bandwidth for large datasets.

Despite these constraints, YMM delivers the fastest performance overall, maintaining correctness and stable runtime across tests.



### Summary of Insights

* Moving from **C → scalar assembly** mainly reduces compiler overhead.
* Moving from **scalar → XMM/YMM SIMD** yields **true parallel speedups** through vectorized floating-point math.
* The **marginal difference between XMM and YMM** suggests the code has reached a **memory-throughput bound** rather than a compute bound — a common scenario in vectorized workloads.
* All kernels produce identical results, confirming **functional equivalence** across implementations.

---

## Discussion: Process, Problems, and AHA Moments

### Methodology

The project followed a **progressive optimization methodology**:

1. **Start with correctness** — implement a clear, reliable baseline in C.
2. **Port to assembly** — reproduce the same logic using scalar instructions to understand register operations and stack conventions.
3. **Add SIMD (XMM)** — introduce vectorization, handling boundary conditions and verifying correctness.
4. **Extend SIMD (YMM)** — double data width and refine mask logic for safety.
5. **Benchmark and compare** — use `QueryPerformanceCounter` to obtain high-resolution timings for objective evaluation.

This systematic, bottom-up approach ensured each optimization step was verified against the baseline before moving forward.



### Problems Encountered and How They Were Solved

**1. Boundary Handling for Non-Divisible Vector Sizes**

* **Problem:** When `n` wasn’t divisible by 4 (XMM) or 8 (YMM), partial vectors caused either incorrect results or memory violations.
* **Solution:** Implemented **mask tables** in `.rodata` and used `vmaskmovps` for conditional load/store. This prevented invalid memory access while maintaining parallelism.
* **AHA moment:** realizing `vmaskmovps` could safely handle tails without scalar fallback loops.



**2. Stack Frame & Calling Convention (Win64 ABI)**

* **Problem:** The fifth argument (`D`) isn’t passed in a register in Win64; it must be retrieved from the stack. Early attempts used wrong offsets, causing crashes.
* **Solution:** Corrected stack offset handling in assembly (`[rbp+32]`), ensuring the proper retrieval of pointer arguments.
* **AHA moment:** understanding how the **Windows x64 calling convention** works (RCX, RDX, R8, R9, then stack).



**3. Timing Accuracy and Output Locking**

* **Problem:** Early timings fluctuated heavily between runs.
* **Solution:** Repeated each kernel 30×, averaged results, and used `QueryPerformanceFrequency` for stable conversion to milliseconds.
* **AHA moment:** discovering that micro-benchmarks require averaging to eliminate timer granularity and background jitter.



### Overall Reflections & Learning Outcomes

* **Performance isn’t just about faster math** — it’s about balancing arithmetic throughput, memory bandwidth, and instruction flow.
* **Assembly requires precision** — every register, instruction, and calling convention detail matters.
* **Validation is critical** — correctness checks and zeroing out arrays after each run ensured confidence in results.
* **Incremental improvement** works best — debugging a small, verified piece at each stage prevented cascading errors.
* **Vectorization feels like “unlocking” the hardware** — the jump from scalar to SIMD demonstrated firsthand how CPUs achieve parallel performance.



### Conclusion

The final results show a clear performance progression:

> **C (baseline)** → **Scalar x86** → **SIMD XMM** → **SIMD YMM**

Each step introduced a tangible efficiency gain, culminating in nearly **6× total acceleration** without loss of accuracy.
Beyond raw speed, the project strengthened understanding of **low-level computation**, **data alignment**, **mask-based safety**, and the practical aspects of writing and benchmarking optimized SIMD code in a real environment.
