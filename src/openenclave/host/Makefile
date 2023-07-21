# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

include ../../config.mk

CFLAGS=$(shell pkg-config oehost-$(COMPILER) --cflags)
LDFLAGS=$(shell pkg-config oehost-$(COMPILER) --libs)
INCDIR=$(shell pkg-config oehost-$(COMPILER) --variable=includedir)

build:
	@ echo "Compilers used: $(CC), $(CXX)"
	oeedger8r ../libdogecoin.edl --untrusted \
		--search-path $(INCDIR) \
		--search-path $(INCDIR)/openenclave/edl/sgx
	$(CC) -g -c $(CFLAGS) host.c
	$(CC) -g -c $(CFLAGS) libdogecoin_u.c
	$(CC) -o libdogecoinhost libdogecoin_u.o host.o $(LDFLAGS)

clean:
	rm -f libdogecoinhost host.o libdogecoin_u.o libdogecoin_u.c libdogecoin_u.h libdogecoin_args.h
