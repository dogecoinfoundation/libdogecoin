from libc.stdint cimport uint32_t, uint8_t

cdef extern from "libdogecoin.h":
    cdef int MAX_PASS_SIZE

