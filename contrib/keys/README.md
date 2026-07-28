# Vendored APT signing keys (OpenEnclave build leg)

The `x86_64-linux-openenclave` CI leg and the `doc/enclaves.md` build recipe add
three third-party APT repositories inside an Ubuntu 20.04 (focal) container. These
files are the **repository signing keys** for those repos, vendored so the build
depends on the upstream CDNs only for *packages*, never for *trust material*.

Fetching keys at build time over `wget/curl` proved fragile: a failed fetch from a
CI runner (e.g. Akamai/CDN edge behaviour toward cloud IP ranges, or an IPv4-only
runner reaching an IPv6-only edge) yielded an empty stream that `apt-key` reported
three commands later as `gpg: no valid OpenPGP data found`. Vendoring removes that
failure class entirely.

The build dearmors each `.asc` into `/etc/apt/keyrings/*.gpg` and pins the matching
`deb` source with `signed-by=`, so apt will reject any package not signed by these
exact keys.

## Keys

| File | Repo | Key identity | Fingerprint |
|------|------|--------------|-------------|
| `intel-sgx-deb.asc` | `https://download.01.org/intel-sgx/sgx_repo/ubuntu` | CN=Intel(R) Software Development Products | `150434D1 488BF803 08B69398 E5C7F0FA 1C6C6C3C` |
| `llvm-snapshot.asc` | `http://apt.llvm.org/focal/` | Sylvestre Ledru — Debian LLVM packages | `6084F3CF 814B57C1 CF12EFD5 15CF4D18 AF4F7421` |
| `microsoft.asc` | `https://packages.microsoft.com/ubuntu/20.04/prod` | Microsoft (Release signing) | `BC528686 B50D79E3 39D3721C EB3E94AD BE1229CF` |

Fetched 2026-07-28 from the source URLs above.

## Verifying / refreshing

Confirm a vendored key against its fingerprint:

```sh
gpg --show-keys --with-fingerprint contrib/keys/llvm-snapshot.asc
```

The LLVM (`6084F3CF…AF4F7421`) and Microsoft (`BC528686…BE1229CF`) fingerprints are
the long-standing published values for those archives; the Intel SGX key should be
cross-checked against Intel's current SGX documentation before relying on it.

To refresh a key (only when a repo rotates its signing key), re-fetch and re-verify
the fingerprint before committing:

```sh
curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key -o contrib/keys/llvm-snapshot.asc
gpg --show-keys --with-fingerprint contrib/keys/llvm-snapshot.asc   # compare fingerprint
```
