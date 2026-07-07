# Constant-time verification (dudect)

This directory holds statistical constant-time tests for libdogecoin's
secret-handling code, built on [dudect](https://github.com/oreparaz/dudect)
(Reparaz, Balasch, Verbauwhede — "Dude, is my code constant time?").

## Why this exists

The security assurance case (Tier 1) requires that secret-dependent timing
not leak key material. Source review can say a function *looks* constant-time,
but the compiler is free to reintroduce data-dependent branches at `-O2` — so
the property must be verified on the *compiled binary at the release
optimisation level*, which is what these tests do.

dudect measures execution-time distributions for two input classes (a fixed
secret vs. random secrets) and applies Welch's t-test. A constant-time
function shows the t-statistic bounded and non-growing as measurements
accumulate; a leaky one shows |t| climbing past the ~4.5 threshold.

## Tests

- `ct_mem_cmp_ct.c` — the library's core constant-time comparison primitive,
  `dogecoin_mem_cmp_ct` (`src/mem.c`), used wherever secret material is
  compared. Links and calls the **real exported symbol** (no copy, no
  keep-in-sync caveat) — the strongest form of the test.
- `ct_bip38_mem_eq.c` — the BIP38 fixed-length equality check on
  secret-derived data (`bip38_mem_eq` in `src/bip38.c`), an XOR-accumulate
  comparison written to avoid `memcmp`'s early-exit timing. Compiles a *copy*
  of the function (that library symbol is static).

## Results (recorded)

Measured with clang 18 at `-O2` on x86-64:

| function under test | linked/copy | measurements | max \|t\| | verdict |
|---|---|---|---|---|
| `dogecoin_mem_cmp_ct` | linked (real symbol) | 38 M | 2.2 | constant-time |
| `bip38_mem_eq` | copy | 72 M | 1.8 | constant-time |
| positive control (branch on secret) | copy | — | 691 | leak detected (as expected) |

The positive control — a variant that branches on the secret's first byte —
is what makes the negative results trustworthy: it confirms the harness
*can* detect a leak, so the stable low t-values are real passes, not tests
that cannot fail. (The control is not shipped; it is described here and easy
to reproduce by replacing the compared function with an early-return branch
keyed on `a[0]`.)

## Running

Copy tests (no library build needed):

```
make -f ct.mk -C contrib/ct         # builds ct_bip38_mem_eq
./ct_bip38_mem_eq                   # runs until leakage found or interrupted
```

Linked tests (need a built libdogecoin.a). From the repo root:

```
./autogen.sh && ./configure CC=clang CFLAGS="-O2 -g" --enable-static --disable-shared
make -C src/secp256k1 && make libdogecoin.la
make -f ct.mk -C contrib/ct \
  LIBDOGECOIN=../../.libs/libdogecoin.a \
  SECP256K1=../../src/secp256k1/.libs/libsecp256k1.a linked
./contrib/ct/ct_mem_cmp_ct
```

Exit status is 0 when no leakage evidence is found and non-zero when a leak is
detected, so the test is usable as a CI gate. Note that dudect runs
indefinitely by design (accumulating confidence); for CI, bound it with a
measurement cap or a timeout and treat "no leak within budget" as a pass.

## Caveats

- These tests compile a **local copy** of the function under test, because the
  library implementations are `static`. The copy must be kept byte-identical
  to the library source; if `bip38_mem_eq` changes, update the harness (or
  export a testable symbol). A comment in each harness flags this.
- Timing measurement is host- and load-sensitive. Run on an otherwise-idle
  machine; a noisy host inflates the measurement variance and can mask a small
  leak. The positive control is the check that the setup is sensitive enough.
- A pass means "no timing leak detectable by this method at this optimisation
  level on this host." It is strong evidence, not a proof.
