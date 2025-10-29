# Parallel Merge Sort (pthreads)

**Authors:** Nguyen Do Nhat Hoang, Ngo Minh Tuan, Sebastian Lien  
**Group name:** Group 108

## Overview

This project implements a **parallel merge sort** in C using **POSIX threads (pthreads)**. Given an input size `n`, a depth **cutoff** for thread spawning, and a RNG seed, the program:

1) generates a random array `A` of `n` integers,  
2) sorts it using either serial or parallel merge sort (depending on `cutoff`), and  
3) reports wall-clock time.

Parallelism follows a **divide-and-conquer fork–join** pattern: each task splits its range, spawns two child threads while `level < cutoff`, waits (`pthread_join`), then merges the two sorted halves. No mutexes are needed because subranges are disjoint and merges happen only after joins.

---

## Manifest

- **`mergesort.h`** – Public header (globals `A`, `B`, `cutoff`; `struct argument`; prototypes).
- **`mergesort.c`** – Implementation:
  - `merge(leftstart, leftend, rightstart, rightend)` – stable merge into `B`, then bulk copy back to `A` via `memcpy`.
  - `my_mergesort(left, right)` – serial, recursive merge sort.
  - `parallel_mergesort(void *arg)` – depth-bounded parallel recursion using `pthread_create/join`.
  - `buildArgs(left, right, level)` – heap-allocates and initializes `struct argument`.
- **`test-mergesort.c`** – Official harness (allocates `A/B`, parses args, times runs, validates sortedness).
- **`test-parallel-mergesort.c`** *(optional)* – Simple custom harness for local experiments.
- **`Makefile`** – Builds `test-mergesort` (and optionally `test-parallel-mergesort`).
- **`README.md`** – This document.

---

## Building

Requirements: `gcc`, `make`, and pthreads (Linux/macOS; on Windows use WSL).

```bash
make                # builds ./test-mergesort
# optionally:
make test-parallel-mergesort
```

If you see `Permission denied` when running an executable, ensure it’s built and executable:

```bash
make clean && make
# or
chmod +x ./test-mergesort
```

**Sanitizers (optional):**
```bash
make clean
CFLAGS='-Wall -Wextra -O2 -fsanitize=address,undefined -g -pthread' make
```

---

## Usage

**Executable:** `./test-mergesort <n> <cutoff> <seed>`

Examples:
```bash
# Serial baseline (no extra threads)
./test-mergesort 1000000 0 1234

# Parallel with one level (2 workers at the leaves)
./test-mergesort 1000000 1 1234

# Deeper fork-join
./test-mergesort 1000000 3 1234
```

Key behaviors:
- **Stable merge**: uses `A[i] <= A[j]` to preserve equal-key order.
- **Depth cutoff**: when `level >= cutoff`, the branch switches to `my_mergesort` (serial) — prevents thread explosion.
- **Memory model**: `A` and `B` are global and allocated once; `merge` writes into `B` then bulk-copies back to `A` via `memcpy` over the merged span.
- **Ownership**: Each *child* thread frees its own `struct argument` before returning (`if (level > 0) free(a);`). The *root* argument is freed by the harness, which prevents double-free.

---

## Quick sanity tests (copy-paste)

```bash
# --- Smoke (tiny arrays) ---
./test-mergesort 2 0 1
./test-mergesort 3 1 1
./test-mergesort 10 0 7
./test-mergesort 10 2 7

# --- Small/functional ---
./test-mergesort 100 0 1234
./test-mergesort 100 1 1234
./test-mergesort 1000 2 1234

# --- Medium perf sanity ---
./test-mergesort 100000 0 42
./test-mergesort 100000 1 42
./test-mergesort 100000 2 42
./test-mergesort 100000 3 42
./test-mergesort 100000 4 42

# --- Larger (ensure enough RAM; avoid printing arrays during timing) ---
./test-mergesort 1000000 0 1234
./test-mergesort 1000000 1 1234
./test-mergesort 1000000 2 1234
./test-mergesort 1000000 3 1234
./test-mergesort 1000000 4 1234
```

---

## Performance

**How to measure**
- Run each config 3–5× and average.
- Avoid printing arrays during timing (I/O dominates).
- Keep the machine otherwise idle for more stable numbers.

**Example timings on our machine (n = 1,000,000):**

| n        | cutoff | time (s) | speedup vs. cutoff=0 |
|----------|--------|----------|----------------------|
| 1,000,000| 0      | 0.29     | 1.00×               |
| 1,000,000| 1      | 0.17     | 1.71×               |
| 1,000,000| 2      | 0.09     | 3.22×               |
| 1,000,000| 3      | 0.09     | 3.22×               |
| 1,000,000| 4      | 0.10     | 2.90×               |

> Interpretation: parallelism helps up to a point; beyond that, thread overhead and oversubscription eat the gains.

**Memory note:** `n = 100,000,000` requires ~**762 MiB** for two `int` arrays (`A` and `B`)—ensure enough RAM and swap before attempting.

---

## Testing

**Correctness**
- Program validates sortedness and reports a failure if any `A[i] > A[i+1]`.
- Test cases: tiny (0/1/2), already sorted, reverse sorted, all equal, many duplicates, random.

**Concurrency**
- No shared-state races: each thread works on **disjoint** subranges of `A/B`. Merges happen only **after** both children `pthread_join`, so no mutex is required.

**Memory**
- Verified no leaks/double-frees with AddressSanitizer/Valgrind.
- Critical pattern: the **root** `arg` is freed by the harness; each **child** frees its *own* `arg`.

---

## Known Bugs / Limitations

- **Double-free if ownership is wrong**: Freeing the root `arg` inside `parallel_mergesort` leads to `free(): double free detected`. Fix: child threads free their own `arg` (`if (level > 0)`), the root is freed in `main`.
- **Thread creation failures**: If `pthread_create` fails on a branch, free any allocated `leftArg/rightArg` and fall back to `my_mergesort` on that branch.
- **Over-parallelization**: Large `cutoff` on few cores can be slower due to thread overhead/oversubscription.
- **Very large `n`**: Can exceed memory on low-RAM machines.

---

## Reflection and Self-Assessment

I found the serial merge sort straightforward, but the **parallel** version forced me to internalize three things: (1) **ownership & lifetime** for per-thread arguments, (2) a second base case via **`level`/`cutoff`** to stop thread fan-out, and (3) when synchronization is actually unnecessary.

**What I fixed/learned**
- **Double-free crash** was caused by freeing the root `arg` inside the worker. The rule that clicked: *caller owns its arg; each callee owns its own arg*.
- **Cutoff** prevents thread explosion and matches parallelism to core count.
- **No mutexes needed**: work is on disjoint segments; `join` gives a happens-before for the merge.
- **Midpoint** should be `left + (right - left)/2` to avoid precedence/overflow pitfalls.

Once I framed the recursion tree as an **ownership tree** and used **cutoff** as the fan-out brake, the implementation became clean, leak-free, and consistently faster up to a sensible depth.

---

## Sources Used

- POSIX man pages: `pthread_create(3)`, `pthread_join(3)`, `memcpy(3)`
- Viblo articles (conceptual refresher on multithreading models):  
  - *Làm quen với multithreading trong C#* — https://viblo.asia/p/lam-quen-voi-multithreading-trong-c-qm6RWQYXGeJE  
  - *Tìm hiểu về xử lí đa luồng trong Java* — https://viblo.asia/p/tim-hieu-ve-xu-li-da-luong-trong-java-m68Z0xyQZkG
- Course materials: **Lecture 05 — Concurrency** (class slides/notes).
- AI assistance: drafting and technical review using ChatGPT (GPT-5 Thinking).
