# SIMD Vector Add–Multiply (A = B + C * D)

This project implements and benchmarks four kernels that compute, element-wise:

$A[i] = B[i] + C[i] * D[i]$, for $i$ in $[0, n)$.

Kernels:
1. **C reference** (`vecaddmulc`) – portable baseline in C.
2. **Scalar x86-64** (`vecaddmulx86_64`) – assembly with scalar `movss/mulss/addss`.
3. **SIMD AVX/XMM** (`vecaddmulxmm`) – 128-bit vectors (4 floats per step) with masked tail.
4. **SIMD AVX2/YMM** (`vecaddmulymm`) – 256-bit vectors (8 floats per step) with masked tail.

A correctness pass prints sample outputs, and all kernels are benchmarked using the methods $QueryPerformanceQuery$ and $QueryPerformanceCounter$ from the $windows.h$ c library.

---

## Repository Layout
```
/ (root)
├─ main.c # driver, timing, correctness checks
├─ vecaddmulx86_64.asm # scalar x86-64 kernel
├─ vecaddmulxmm.asm # AVX (XMM, 128-bit) kernel + tail mask table
├─ vecaddmulymm.asm # AVX2 (YMM, 256-bit) kernel + tail mask table
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
| 2²⁰        | <img src="https://github.com/user-attachments/assets/f84b2bff-f2b1-4e65-9bfd-24e493fa3161" width="850" /> |
| 2²⁶        | <img src="https://github.com/user-attachments/assets/f873f506-61d9-4afb-86da-e8611c8bf9ee" width="850" /> |
| 2²⁸        | <img src="https://github.com/user-attachments/assets/3bfc3aa7-4eba-43d6-84a9-5e6d39edd509" width="850" /> |

---

## Release-Mode Benchmark Results

The same tests were executed in **Release mode** (optimized build).
Results show substantial performance gains over Debug mode, validating proper compiler optimization and SIMD utilization.

| Input Size | Screenshot                                                                                                |
| ---------- | --------------------------------------------------------------------------------------------------------- |
| 2²⁰        | <img src="https://github.com/user-attachments/assets/e055c46c-1a0d-4501-a0f8-d85e61e2a681" width="850" /> |
| 2²⁶        | <img src="https://github.com/user-attachments/assets/99ef71aa-405e-4559-a45f-0fc7b10a6e2e" width="850" /> |
| 2²⁸        | <img src="https://github.com/user-attachments/assets/afe56aa5-015a-4110-92da-4f417651f1ec" width="850" /> |

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

---

## Discussion
