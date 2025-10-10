# SIMD_project

### Initialization

Vectors $B$, $C$, and $D$ were initialized as follows:
- $B[i]=\sin(i*0.001)$
- $C[i]=\cos(i*0.002)$
- $D[i]=\tan(i*0.0005 + 1.0)$

## Boundary Checks

### Input Size: 1003

<img width="853" height="320" alt="image" src="https://github.com/user-attachments/assets/fb7b1486-8286-49a7-becf-9d2cbf7b7a29" />

### Input Size: 2^10 + 3

<img width="859" height="311" alt="image" src="https://github.com/user-attachments/assets/1eb3a9c5-3a45-46d7-93f1-51ebcf4b84a1" />

### Input Size: 2^20 + 7

<img width="851" height="321" alt="image" src="https://github.com/user-attachments/assets/a6e0f646-287f-4ebc-abd8-8710dec92a7d" />

## Debug Mode Results

### Input Size: 2^20

<img width="851" height="313" alt="image" src="https://github.com/user-attachments/assets/f84b2bff-f2b1-4e65-9bfd-24e493fa3161" />

### Input Size: 2^26

<img width="865" height="323" alt="image" src="https://github.com/user-attachments/assets/f873f506-61d9-4afb-86da-e8611c8bf9ee" />

### Input Size: 2^28

<img width="867" height="331" alt="image" src="https://github.com/user-attachments/assets/3bfc3aa7-4eba-43d6-84a9-5e6d39edd509" />

## Release Mode Results

### Input Size: 2^20

<img width="854" height="319" alt="image" src="https://github.com/user-attachments/assets/e055c46c-1a0d-4501-a0f8-d85e61e2a681" />

### Input Size: 2^26

<img width="856" height="317" alt="image" src="https://github.com/user-attachments/assets/99ef71aa-405e-4559-a45f-0fc7b10a6e2e" />

### Input Size: 2^28

<img width="858" height="316" alt="image" src="https://github.com/user-attachments/assets/afe56aa5-015a-4110-92da-4f417651f1ec" />

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

### Analysis

## Discussion
