# Constant-time tests (dudect). Two flavours:
#
#  * copy tests (ct_bip38_mem_eq): compile a local copy of a *static* library
#    function at -O2. Standalone, no library build needed. Keep the copy in
#    sync with the library source (flagged in the harness).
#
#  * linked tests (ct_mem_cmp_ct): link the *real* exported symbol from a built
#    libdogecoin.a, so there is no copy to keep in sync. Requires the library
#    to be built first (autotools) and its path passed in.
#
# Usage:
#   make -f ct.mk                          # build copy tests only
#   make -f ct.mk LIBDOGECOIN=../../.libs/libdogecoin.a \
#                 SECP256K1=../../src/secp256k1/.libs/libsecp256k1.a linked
#   make -f ct.mk run                      # build + bounded-run copy tests
#   make -f ct.mk clean
#
# For the linked tests, build the library first, e.g. from the repo root:
#   ./autogen.sh && ./configure CC=clang CFLAGS="-O2 -g" --enable-static --disable-shared
#   make -C src/secp256k1 && make libdogecoin.la

CC      ?= clang
CFLAGS  ?= -O2 -g -Wall
INCLUDE ?= -I../../include
LDLIBS  := -lm
RUN_TIMEOUT ?= 120

# copy-based tests (no library link)
COPY_TESTS   := ct_bip38_mem_eq
# linked tests (need LIBDOGECOIN + SECP256K1 paths)
LINKED_TESTS := ct_mem_cmp_ct

all: $(COPY_TESTS)

ct_bip38_mem_eq: ct_bip38_mem_eq.c dudect.h
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

linked: $(LINKED_TESTS)

ct_mem_cmp_ct: ct_mem_cmp_ct.c dudect.h
	@if [ -z "$(LIBDOGECOIN)" ]; then \
	  echo "error: set LIBDOGECOIN=<path/to/libdogecoin.a> (and SECP256K1=...) to build linked tests"; exit 1; fi
	$(CC) $(CFLAGS) $(INCLUDE) $< $(LIBDOGECOIN) $(SECP256K1) -levent -levent_core $(LDLIBS) -o $@

# Bounded run for CI: "no leak within budget" exits 0 (pass); a detected
# leak makes the test exit non-zero before the timeout.
run: $(COPY_TESTS)
	@for t in $(COPY_TESTS); do \
	  echo "== $$t =="; \
	  timeout $(RUN_TIMEOUT) ./$$t; \
	  rc=$$?; \
	  if [ $$rc -eq 124 ]; then echo "  no leak within $(RUN_TIMEOUT)s budget (pass)"; \
	  elif [ $$rc -ne 0 ]; then echo "  LEAK DETECTED (exit $$rc)"; exit 1; \
	  else echo "  completed clean"; fi; \
	done

clean:
	rm -f $(COPY_TESTS) $(LINKED_TESTS)

.PHONY: all linked run clean
