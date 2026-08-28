// swift-tools-version: 5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "libdogecoin",
    platforms: [
        .iOS(.v15),
        .macOS(.v12)
    ],
    products: [
        // C library only - Swift wrapper lives in DogecoinKit
        .library(
            name: "clibdogecoin",
            targets: ["clibdogecoin"]
        )
    ],
    targets: [
        // MARK: - C Library Target
        .target(
            name: "clibdogecoin",
            path: ".",
            exclude: [
                // Exclude CLI executables (but keep tool.c which has utility functions)
                "src/cli/such.c",
                "src/cli/sendtx.c",
                "src/cli/spvnode.c",
                // Exclude networking (requires libevent) - networking is done in Swift
                "src/libevent",
                "src/net.c",
                "src/spv.c",
                "src/rest.c",
                "src/protocol.c",
                "src/headersdb_file.c",
                "src/headersdb.c",
                // Exclude platform-specific code
                "src/intel",
                "src/openenclave",
                "src/optee",
                // Exclude benchmarks and tests
                "src/bench.c",
                "src/scrypt-sse2.c",
                "test",
                // Exclude secp256k1 files that are #included, not compiled separately
                "src/secp256k1/src/tests.c",
                "src/secp256k1/src/tests_exhaustive.c",
                "src/secp256k1/src/bench.c",
                "src/secp256k1/src/bench_ecmult.c",
                "src/secp256k1/src/bench_internal.c",
                "src/secp256k1/src/valgrind_ctime_test.c",
                "src/secp256k1/src/precompute_ecmult.c",
                "src/secp256k1/src/precompute_ecmult_gen.c",
                // Exclude logdb tests
                "src/logdb/test",
                // Exclude documentation and examples
                "doc",
                "contrib",
                "depends",
                // Exclude build artifacts and config
                "config"
            ],
            sources: [
                // Core dogecoin sources
                "src/address.c",
                "src/aes.c",
                "src/arith_uint256.c",
                "src/auxpow.c",
                "src/base58.c",
                "src/bip32.c",
                "src/bip39.c",
                "src/bip44.c",
                "src/block.c",
                "src/buffer.c",
                "src/chacha20.c",
                "src/chainparams.c",
                "src/context.c",
                "src/cstr.c",
                "src/ctaes.c",
                "src/ecc.c",
                "src/eckey.c",
                "src/key.c",
                "src/koinu.c",
                "src/map.c",
                "src/mem.c",
                "src/moon.c",
                "src/pow.c",
                "src/png.c",
                "src/jpeg.c",
                "src/qr.c",
                "src/qrengine.c",
                "src/random.c",
                "src/rmd160.c",
                "src/script.c",
                "src/scrypt.c",
                "src/seal.c",
                "src/serialize.c",
                "src/sha2.c",
                "src/sign.c",
                "src/transaction.c",
                "src/tx.c",
                "src/utf8proc.c",
                // Note: utf8proc_data.c is #included by utf8proc.c, not compiled separately
                "src/utils.c",
                "src/validation.c",
                "src/vector.c",
                "src/wallet.c",
                // CLI tool utilities (not the executables)
                "src/cli/tool.c",
                // logdb sources
                "src/logdb/logdb_core.c",
                "src/logdb/logdb_memdb_llist.c",
                "src/logdb/logdb_memdb_rbtree.c",
                "src/logdb/logdb_rec.c",
                "src/logdb/misc.c",
                "src/logdb/red_black_tree.c",
                "src/logdb/stack.c",
                // secp256k1 - main library file (includes other .h implementations)
                "src/secp256k1/src/secp256k1.c",
                // secp256k1 - precomputed tables
                "src/secp256k1/src/precomputed_ecmult.c",
                "src/secp256k1/src/precomputed_ecmult_gen.c"
            ],
            publicHeadersPath: "include",
            cSettings: [
                // Header search paths
                .headerSearchPath("include"),
                .headerSearchPath("src/secp256k1"),
                .headerSearchPath("src/secp256k1/include"),
                .headerSearchPath("src/secp256k1/src"),
                .headerSearchPath("src/logdb/include"),

                // secp256k1 configuration - 32-bit field/scalar for maximum compatibility
                .define("USE_FIELD_10X26", to: "1"),
                .define("USE_SCALAR_8X32", to: "1"),
                .define("USE_FIELD_INV_BUILTIN", to: "1"),
                .define("USE_SCALAR_INV_BUILTIN", to: "1"),
                .define("ENABLE_MODULE_RECOVERY", to: "1"),
                .define("ECMULT_WINDOW_SIZE", to: "15"),
                .define("ECMULT_GEN_PREC_BITS", to: "4"),

                // libdogecoin configuration
                .define("RANDOM_DEVICE", to: "\"/dev/urandom\""),
                .define("WITH_WALLET", to: "1"),
                .define("WITH_LOGDB", to: "1"),
                .define("PACKAGE_NAME", to: "\"libdogecoin\""),
                .define("PACKAGE_VERSION", to: "\"0.1.5\""),

                // Disable networking (we'll use Swift networking in DogecoinKit)
                .define("WITH_NET", to: "0"),

                // Suppress warnings for third-party code and undefine HAVE_CONFIG_H
                .unsafeFlags([
                    "-UHAVE_CONFIG_H",
                    "-D_GNU_SOURCE",
                    "-Wno-shorten-64-to-32",
                    "-Wno-deprecated-non-prototype",
                    "-Wno-pointer-bool-conversion",
                    "-Wno-unused-function",
                    "-Wno-unused-variable"
                ])
            ]
        )
    ],
    cLanguageStandard: .c99
)
