# Secure Enclaves Integration with libdogecoin

## Introduction

This document discusses our research on enhancing the security of **libdogecoin** key management operations using **Intel SGX** and **ARM TrustZone** as secure enclaves. By performing key management within secure enclaves, we significantly reduce the risk of key exposure, thereby increasing the overall security of Dogecoin transactions. This document includes an overview of the research, build instructions for the key management enclaves, and a step-by-step tutorial on using the host command line interface to interact with the enclaves in both **OP-TEE** and **OpenEnclave** environments. Additionally, we outline critical vulnerabilities in Trusted Execution Environments (TEEs) and provide recommendations for mitigating these risks.

This document covers mnemonic generation, public key derivation, address generation, message signing, and transaction signing as part of secure enclave operations.

## What are Secure Enclaves?

Secure enclaves are isolated environments that provide a secure space for sensitive operations. These enclaves are isolated from the rest of the system, ensuring that sensitive data and operations are protected from unauthorized access. **Secure enclaves** within a processor can be implemented using different technologies such as **TEEs**, hardware security modules (**HSMs**), and virtualization-based solutions. To ensure a robust defense, secure enclaves must guarantee **confidentiality**, **integrity**, and **availability** for sensitive operations. However, enclaves are not a silver bullet and must be integrated within a broader security strategy.

Compared to hardware security modules (HSMs) and trusted platform modules (TPMs), secure enclaves provide in-CPU isolation, which makes them ideal for high-performance cryptographic operations without requiring external hardware.

## Why Use Secure Enclaves?

Secure enclaves offer several advantages for key management operations:

- **Isolation**: Enclaves provide a secure environment that is isolated from the rest of the system, protecting sensitive data and operations.
- **Confidentiality**: Enclaves ensure that sensitive data is encrypted and protected from unauthorized access.
- **Integrity**: Enclaves guarantee the integrity of sensitive operations, ensuring that they have not been tampered with.
- **Remote Attestation**: Enclaves can be remotely attested to verify their integrity and authenticity to external parties.
- **Key Management**: Enclaves can securely generate, store, and manage cryptographic keys, protecting them from unauthorized access.

By performing key operations within secure enclaves, developers can enhance the security of their applications and protect sensitive data from unauthorized access.

## Secure Enclaves in libdogecoin

### Key Concepts

In this section, we define some key concepts central to secure enclaves and their implementation.

- **Host**: The normal world that interacts with the enclave.
- **Enclave**: The secure world within the CPU where sensitive operations are performed.
- **Trusted Execution Environment (TEE)**: A hardware-based secure environment that protects sensitive operations from external interference and tampering.
- **ECALLS**: Enclave calls used in **OpenEnclave** that allow the host to interact with the secure enclave.
- **SMCs**: Secure Monitor Calls that manage communication between the normal and secure worlds in **ARM TrustZone**.
- **Remote Attestation**: The process of verifying the integrity and authenticity of an enclave to a remote party.

By performing key operations such as **seedphrase generation**, **public key derivation**, and **message and transaction signing** within these enclaves, we can greatly reduce the risk of private key exposure, even in the event of a host system compromise.

## What are Trusted Execution Environments (TEEs)?

TEEs are secure environments within a processor that provide a trusted execution domain for sensitive operations. They ensure that sensitive data and operations are protected from unauthorized access and tampering. TEEs can be implemented using hardware-based security technologies such as **Intel SGX**, **ARM TrustZone**, or other vendor-specific technologies. While they offer significant advantages in terms of isolating critical operations, TEEs also come with their own set of challenges, particularly in terms of addressing vulnerabilities and maintaining performance.

### Contrast Between Intel SGX and ARM TrustZone

| **Criteria**               | **Intel SGX (OpenEnclave)**                     | **ARM TrustZone (OP-TEE)**                    |
|----------------------------|-------------------------------------------------|-----------------------------------------------|
| **Target Platform**         | Primarily x86 server-class CPUs                 | Primarily ARM mobile and embedded devices     |
| **Security Model**          | Memory encryption and isolated execution        | Secure world execution isolated from normal world |
| **Key Vulnerabilities**     | Foreshadow, Plundervolt, SGAxe                  | CVE-2021-34387, CVE-2020-16273                |
| **Performance Overhead**    | Higher, especially for I/O and large memory use | Lower, better for embedded and mobile devices |
| **Use Cases**               | Server applications, enterprise security        | IoT, mobile devices, embedded systems         |

## Hardware and Software

### Intel SGX and OpenEnclave

Intel SGX (Software Guard Extensions) is a hardware-based security technology that creates isolated TEEs within some Intel CPUs. OpenEnclave is an open-source SDK that enables developers to build host and enclave applications that run on Intel SGX. SGX works by partitioning secure memory regions within the CPU that are isolated from the rest of the system, ensuring that sensitive data is protected from unauthorized access. These memory regions are encrypted and authenticated, providing a secure environment for cryptographic operations.

OpenEnclave offers an interface for developing applications that run within SGX enclaves, enabling developers to build secure applications that protect sensitive data and operations. The host interacts with the enclave using ECALLS, allowing the enclave to perform cryptographic operations securely. Remote attestation is used to verify the integrity of the enclave to remote parties, ensuring that the enclave has not been tampered with.

### ARM TrustZone and OP-TEE

ARM TrustZone is another hardware-based security technology that creates TEEs within ARM CPUs. OP-TEE (Open Portable Trusted Execution Environment) is an open-source software framework that facilitates the development of applications running within ARM TrustZone enclaves. TrustZone creates a secure world and a normal world within the CPU, isolating sensitive operations from the rest of the system. The processor switches between the secure and normal worlds, ensuring that sensitive operations are performed securely. Memory is partitioned between the secure and normal worlds using page tables, protecting sensitive data from unauthorized access.

OP-TEE provides a secure world OS for cryptographic operations in TrustZone firmware, ensuring that private keys and sensitive data are protected from unauthorized access. The host interacts with the secure world using SMCs, allowing the secure enclave to perform cryptographic operations securely. OP-TEE is widely used in IoT devices, mobile phones, and embedded systems to protect sensitive data and operations.

## Key Management Enclaves

### Visual Representation (Simplified)

```text
+-----------------------------------------+               +-----------------------------------------+
| Host                                    |               | Host                                    |
|                                         |               |                                         |
|  +-----------------------------------+  |               |  +-----------------------------------+  |
|  |      command line interface       |  |               |  |      command line interface       |  |
|  |                                   |  |               |  |                                   |  |
|  |  +-----------------------------+  |  |               |  |  +-----------------------------+  |  |
|  |  |         libdogecoin         |  |  |               |  |  |         libdogecoin         |  |  |
|  +-----------------------------------+  |               |  +-----------------------------------+  |
|  |         OpenEnclave Host          |  |               |  |           OP-TEE Client           |  |
+-----------------------------------------+               +-----------------------------------------+
|                Linux OS                 |               |                Linux OS                 |
+-----------------------------------------+               +-----------------------------------------+
| Enclave            ^                    |               | Enclave            ^                    |
|                 ECALLS                  |               |                   SMCs                  |
|                    |                    |               |                    |                    |
|        Messages and Transactions        |               |        Messages and Transactions        |
|        Public Keys and Addresses        |               |        Public Keys and Addresses        |
|                    |                    |               |                    |                    |
|                    v                    |               |                    v                    |
|  +-----------------------------------+  |               |  +-----------------------------------+  |
|  |          Key Management           |  |               |  |          Key Management           |  |
|  |  - Seedphrase Generation          |  |               |  |  - Seedphrase Generation          |  |
|  |  - Public Key Generation          |  |               |  |  - Public Key Generation          |  |
|  |  - Address Generation             |  |               |  |  - Address Generation             |  |
|  |  - Sign Message                   |  |               |  |  - Sign Message                   |  |
|  |  - Sign Transaction               |  |               |  |  - Sign Transaction               |  |
|  |                                   |  |               |  |                                   |  |
|  |  +-----------------------------+  |  |               |  |  +-----------------------------+  |  |
|  |  |         libdogecoin         |  |  |               |  |  |         libdogecoin         |  |  |
+-----------------------------------------+               +-----------------------------------------+
|              OpenEnclave                |               |       OP-TEE OS + Trusted Firmware      |
+-----------------------------------------+               +-----------------------------------------+
|               Intel SGX                 |               |             ARMv8 TrustZone             |
+-----------------------------------------+               +-----------------------------------------+
```

### Key Management Operations

The secure enclave handles the following operations:
- **Seedphrase Generation**: Protecting the creation of seedphrases for wallet recovery.
- **Public Key Generation**: Deriving public keys in the enclave, ensuring private keys are never exposed.
- **Address Generation**: Generating secure Dogecoin addresses within the enclave.
- **Sign Message/Transaction**: Ensuring the integrity and authenticity of signed messages and transactions.

### Limitations and Assumptions

While secure enclaves provide powerful protection, they are **not infallible**. The **Foreshadow** vulnerability is a good example of how attackers can bypass enclave protections. Furthermore, the security model assumes that the host environment is insecure, relying entirely on the enclave for protection. Developers should adopt a **defense-in-depth** strategy, using **secure coding practices** and **regular security audits** to complement enclave security.

### Performance trade-offs

Secure enclaves introduce performance overhead due to the encryption and isolation mechanisms they employ. For example, **Intel SGX** enclaves have higher performance overhead compared to **ARM TrustZone** enclaves, especially for I/O and large memory use. **ARM TrustZone** enclaves are better suited for embedded and mobile devices due to their lower performance impact. However key management operations are typically not performance-critical, making secure enclave protection a worthwhile trade-off for the increased security it provides.

### Secure Storage

Mnemonic seedphrases are securely encrypted by the enclave to prevent unauthorized access. Encrypted data should still be backed up using the **rule of three**: store copies in three distinct locations to ensure recovery.

### Time-Based One-Time Password (TOTP) Authentication

TOTP authentication using a **YubiKey** further enhances security by ensuring that access to sensitive enclave operations is restricted to authorized users. When combined with password-based authentication, TOTP provides an additional layer of security. This two-factor authentication approach increases security during enclave operations, preventing unauthorized key usage even if the host environment is compromised.

## Future Research

To further improve the security and functionality of Dogecoin’s ecosystem, we recommend exploring **Remote Attestation** to validate the integrity of enclaves in distributed systems. This would allow external parties to verify the authenticity of enclaves, ensuring that they have not been tampered with. Intel SGX supports remote attestation, while ARM TrustZone can be extended to include this feature.

Performance optimizations for secure enclaves are another area of interest, as reducing overhead can make enclaves more practical for a wider range of applications. Additonal analysis is needed to evaluate the performance impact of secure enclaves on key management operations at scale.

Additionally, alternative secure technologies like **AMD SEV** and **RISC-V** should be explored to broaden the options available for TEE-based applications. These technologies offer different security models and performance characteristics that may be better suited to specific use cases.

## References

- [Intel SGX](https://software.intel.com/en-us/sgx)
- [ARM TrustZone](https://www.arm.com/solutions/security-on-arm/trustzone)
- [OpenEnclave](https://www.openenclave.io/)
- [OP-TEE](https://optee.readthedocs.io/en/latest/)

## TEE Vulnerabilities

It is crucial to keep systems up-to-date with the latest security fixes and patches to mitigate the risks associated with any vulnerabilities. Regularly applying security patches and following best practices for secure enclave development can help protect sensitive data and operations. Below are some recent vulnerabilities that have been discovered in Intel SGX, ARM TrustZone, OpenEnclave, and OP-TEE. It is essential to stay informed about these vulnerabilities and take appropriate measures to address them.

**Intel SGX:**
Several vulnerabilities have been discovered in Intel SGX:

1. **Foreshadow (CVE-2018-3615):** This vulnerability allows attackers to access sensitive information stored in SGX enclaves. Details can be found in the [Intel Security Advisory](https://www.intel.com/content/www/us/en/security-center/advisory/intel-sa-00161.html).

2. **Plundervolt (CVE-2019-11157):** This vulnerability exploits the CPU's undervolting features to breach SGX enclave protections. More information is available in the [Intel Security Advisory](https://www.intel.com/content/www/us/en/security-center/advisory/intel-sa-00289.html).

3. **SGAxe (CVE-2020-0551):** This vulnerability allows side-channel attacks to extract sensitive information from SGX enclaves. Refer to the [Intel Security Advisory](https://www.intel.com/content/www/us/en/security-center/advisory/intel-sa-00334.html) for further details.

**ARM TrustZone:**
While many vulnerabilities related to ARM TrustZone tend to be vendor-specific or related to specific implementations, the core TrustZone architecture itself has also been associated with certain security concerns. Below are examples of both types:

1. **CVE-2021-34387:** This vulnerability in the ARM TrustZone technology, on which Trusty is based, allows unauthorized write access to kernel code and data that is mapped as read-only, affecting the DRAM reserved for TrustZone. Further details can be found [here](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2021-34387).

2. **CVE-2021-33478:** This issue affects Broadcom MediaxChange firmware, potentially allowing an unauthenticated attacker to achieve arbitrary code execution in the TrustZone TEE of an affected device. More information is available [here](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2021-33478).

3. **CVE-2021-29415:** This vulnerability involves the ARM TrustZone CryptoCell 310 in Nordic Semiconductor nRF52840, allowing adversaries to recover private ECC keys during ECDSA operations. Details can be found [here](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2021-29415).

4. **CVE-2020-16273:** This vulnerability in ARMv8-M processors' software stack selection mechanism can be exploited by a stack-underflow attack, allowing changes to the stack pointer from a non-secure application. More information is available [here](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-16273).

5. **CVE-2022-47549:** This issue in OP-TEE, which is closely associated with ARM TrustZone, allows bypassing signature verification and installing malicious trusted applications via electromagnetic fault injections. More details are available [here](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2022-47549).

It's important to recognize that while TrustZone provides a secure environment, its effectiveness is heavily dependent on how it is implemented and integrated with other system components. Architectural issues or specific use-case vulnerabilities may arise, emphasizing the need for rigorous security practices.

**OpenEnclave:**
For vulnerabilities and security advisories related to OpenEnclave, refer to their [GitHub security page](https://github.com/openenclave/openenclave/security).

**OP-TEE:**
For vulnerabilities and security advisories related to OP-TEE, refer to their [GitHub security page](https://github.com/OP-TEE/optee_os/security).

To mitigate the risks associated with these vulnerabilities, it is essential to stay informed about the latest security advisories and best practices for secure enclave development. Regularly applying security patches and following secure coding practices can help protect sensitive data and operations from unauthorized access.

## Disclaimer

Enclaves are not a silver bullet. As a risk mitigation measure, enclaves can provide significant benefits in protecting sensitive data and operations. We encourage developers to explore secure enclaves and their potential benefits, but caution that they should be used as part of a comprehensive security strategy that includes other security measures such as secure coding practices and audits. Enclaves are not immune to vulnerabilities, and it is essential to stay informed about the latest security advisories and best practices for secure enclave development.

## Build Instructions

### Building OP-TEE SDK and Client

**Dependencies**

- Ubuntu 20.04 or later
- Docker
- Libdogecoin

```sh
sudo apt-get install docker.io

mkdir -p /doge
cd /doge
git clone https://github.com/dogecoinfoundation/libdogecoin.git
cd libdogecoin
```

The SDK has several components and requires over 10GB of disk space to build. The build process can take over 30 minutes on a modern machine. Docker is used to build the SDK and client in a clean environment.

### Step 1 (NanoPC): Building OP-TEE SDK and Client

This command builds the latest SDK and client for NanoPC-T6 (nanopc-t6.xml).  When complete, the image will be located in `/doge/libdogecoin/optee/out/nanopc-t6.img`. Burn this image to an SD card to boot the NanoPC-T6. Connect an Ethernet cable, USB keyboard and HDMI to the NanoPC-T6 and power it on. The default IP address is configured using DHCP. Login as root via ssh (e.g. `ssh root@192.168.137.19`) or using the HDMI console.

`LINUX_MODULES=y` is used to build the Linux kernel modules. The `CFG_TEE_CORE_LOG_LEVEL=0` environment variable sets the log level to 0 (no logging). The `CFG_ATTESTATION_PTA=y` environment variable enables the attestation PTA. The `CFG_ATTESTATION_PTA_KEY_SIZE=1024` environment variable sets the attestation PTA key size to 1024 bits. The `CFG_WITH_USER_TA=y` environment variable enables user TAs. The `CFG_WITH_SOFTWARE_PRNG=n` environment variable enables the hardware PRNG.

An RSA private key is generated and overwrites the default Trusted Application (TA) key. This key is used to sign the enclave binaries during development. In the Continuous Integration (CI) environment, an Actions secret is used. Subkeys are generated for testing purposes but are not used to sign the enclave binaries.

```sh
docker pull jforissier/optee_os_ci:qemu_check
docker run -v "$(pwd):/src" -w /src jforissier/optee_os_ci:qemu_check /bin/bash -c "\
    # Set up the environment and build the OP-TEE SDK
    set -e && \
    apt update && \
    apt -y upgrade && \
    apt -y install libusb-1.0-0-dev swig python3-dev python3-setuptools e2tools && \
    curl https://storage.googleapis.com/git-repo-downloads/repo > /bin/repo && chmod a+x /bin/repo && \
    mkdir -p optee && \
    cd optee && \
    # repo init -u https://github.com/OP-TEE/manifest.git -m qemu_v8.xml -b 4.0.0
    repo init -u https://github.com/OP-TEE/manifest.git -m nanopc-t6.xml -b master && \
    export FORCE_UNSAFE_CONFIGURE=1 && \
    repo sync -j 4 --force-sync && \
    cd build && \
    make toolchains -j 4 && \
    export CFG_TEE_CORE_LOG_LEVEL=0 && \
    export CFG_ATTESTATION_PTA=y && \
    export CFG_ATTESTATION_PTA_KEY_SIZE=1024 && \
    export CFG_WITH_USER_TA=y && \
    export CFG_WITH_SOFTWARE_PRNG=n && \

    # Generate RSA private key and overwrite the default TA key
    openssl genpkey -algorithm RSA -out rsa_private.pem -pkeyopt rsa_keygen_bits:2048 && \
    mv rsa_private.pem /src/optee/optee_os/keys/default_ta.pem && \

    # Generate subkeys
    openssl genrsa -out /src/optee/optee_test/ta/top_level_subkey.pem && \
    openssl genrsa -out /src/optee/optee_test/ta/mid_level_subkey.pem && \
    openssl genrsa -out /src/optee/optee_test/ta/identity_subkey2.pem && \

    # Sign the top-level subkey with the root key
    /src/optee/optee_os/scripts/sign_encrypt.py sign-subkey \
        --uuid f04fa996-148a-453c-b037-1dcfbad120a6 \
        --key /src/optee/optee_os/keys/default_ta.pem --in /src/optee/optee_test/ta/top_level_subkey.pem \
        --out /src/optee/optee_test/ta/top_level_subkey.bin --max-depth 4 --name-size 64 \
        --subkey-version 1 && \

    # Generate UUID for the mid-level subkey
    /src/optee/optee_os/scripts/sign_encrypt.py subkey-uuid --in /src/optee/optee_test/ta/top_level_subkey.bin \
        --name mid_level_subkey && \

    # Sign the mid-level subkey with the top-level subkey
    /src/optee/optee_os/scripts/sign_encrypt.py sign-subkey \
        --uuid 1a5948c5-1aa0-518c-86f4-be6f6a057b16 \
        --key /src/optee/optee_test/ta/top_level_subkey.pem --subkey /src/optee/optee_test/ta/top_level_subkey.bin \
        --name-size 64 --subkey-version 1 \
        --name mid_level_subkey \
        --in /src/optee/optee_test/ta/mid_level_subkey.pem --out /src/optee/optee_test/ta/mid_level_subkey.bin && \

    # Generate UUID for subkey1_ta
    /src/optee/optee_os/scripts/sign_encrypt.py subkey-uuid --in /src/optee/optee_test/ta/mid_level_subkey.bin \
        --name subkey1_ta && \

    # Sign the identity subkey2 with the root key
    /src/optee/optee_os/scripts/sign_encrypt.py sign-subkey \
        --uuid a720ccbb-51da-417d-b82e-e5445d474a7a \
        --key /src/optee/optee_os/keys/default_ta.pem --in /src/optee/optee_test/ta/identity_subkey2.pem \
        --out /src/optee/optee_test/ta/identity_subkey2.bin --max-depth 0 --name-size 0 \
        --subkey-version 1 && \

    # Build and test the OP-TEE OS and client
    # make -j 4 check
    make LINUX_MODULES=y -j 4 && \
    cd /src && \
    [ ! -d optee_client ] && git clone https://github.com/OP-TEE/optee_client.git && \
    cd optee_client && \
    mkdir -p build && \
    cd build && \
    export PATH=/src/optee/toolchains/aarch64/bin:$PATH && \
    export CC=aarch64-linux-gnu-gcc && \
    cmake .. -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_INSTALL_PREFIX=/src/optee/toolchains/aarch64 && \
    make -j 4 VERBOSE=1 && \
    make install"
```

### Step 1 (QEMU): Building OP-TEE SDK and Client

This command builds the SDK (version 4.0.0) and client for ARMv8 QEMU emulation (qemu_v8.xml). For other platforms, change the manifest file in the `repo init` command accordingly. Replace `4.0.0` with the desired version and `qemu_v8.xml` with the desired platform. Refer to the [OP-TEE documentation](https://optee.readthedocs.io/en/latest/building/index.html) for more information.

```sh
docker pull jforissier/optee_os_ci:qemu_check
docker run -v "$(pwd):/src" -w /src jforissier/optee_os_ci:qemu_check /bin/bash -c "\
    # Set up the environment and build the OP-TEE SDK
    set -e && \
    apt update && \
    apt -y upgrade && \
    apt -y install libusb-1.0-0-dev swig python3-dev python3-setuptools e2tools && \
    curl https://storage.googleapis.com/git-repo-downloads/repo > /bin/repo && chmod a+x /bin/repo && \
    mkdir -p optee && \
    cd optee && \
    repo init -u https://github.com/OP-TEE/manifest.git -m qemu_v8.xml -b 4.0.0 && \
    export FORCE_UNSAFE_CONFIGURE=1 && \
    repo sync -j 4 --force-sync && \
    patch -N -F 4 /src/optee/build/common.mk < /src/src/optee/common.mk.patch && \
    cd build && \
    make toolchains -j 4 && \
    export CFG_TEE_CORE_LOG_LEVEL=0 && \
    export CFG_ATTESTATION_PTA=y && \
    export CFG_ATTESTATION_PTA_KEY_SIZE=1024 && \
    export CFG_WITH_USER_TA=y && \

    # Generate RSA private key and overwrite the default TA key
    openssl genpkey -algorithm RSA -out rsa_private.pem -pkeyopt rsa_keygen_bits:2048 && \
    mv rsa_private.pem /src/optee/optee_os/keys/default_ta.pem && \

    # Generate subkeys
    openssl genrsa -out /src/optee/optee_test/ta/top_level_subkey.pem && \
    openssl genrsa -out /src/optee/optee_test/ta/mid_level_subkey.pem && \
    openssl genrsa -out /src/optee/optee_test/ta/identity_subkey2.pem && \

    # Sign the top-level subkey with the root key
    /src/optee/optee_os/scripts/sign_encrypt.py sign-subkey \
        --uuid f04fa996-148a-453c-b037-1dcfbad120a6 \
        --key /src/optee/optee_os/keys/default_ta.pem --in /src/optee/optee_test/ta/top_level_subkey.pem \
        --out /src/optee/optee_test/ta/top_level_subkey.bin --max-depth 4 --name-size 64 \
        --subkey-version 1 && \

    # Generate UUID for the mid-level subkey
    /src/optee/optee_os/scripts/sign_encrypt.py subkey-uuid --in /src/optee/optee_test/ta/top_level_subkey.bin \
        --name mid_level_subkey && \

    # Sign the mid-level subkey with the top-level subkey
    /src/optee/optee_os/scripts/sign_encrypt.py sign-subkey \
        --uuid 1a5948c5-1aa0-518c-86f4-be6f6a057b16 \
        --key /src/optee/optee_test/ta/top_level_subkey.pem --subkey /src/optee/optee_test/ta/top_level_subkey.bin \
        --name-size 64 --subkey-version 1 \
        --name mid_level_subkey \
        --in /src/optee/optee_test/ta/mid_level_subkey.pem --out /src/optee/optee_test/ta/mid_level_subkey.bin && \

    # Generate UUID for subkey1_ta
    /src/optee/optee_os/scripts/sign_encrypt.py subkey-uuid --in /src/optee/optee_test/ta/mid_level_subkey.bin \
        --name subkey1_ta && \

    # Sign the identity subkey2 with the root key
    /src/optee/optee_os/scripts/sign_encrypt.py sign-subkey \
        --uuid a720ccbb-51da-417d-b82e-e5445d474a7a \
        --key /src/optee/optee_os/keys/default_ta.pem --in /src/optee/optee_test/ta/identity_subkey2.pem \
        --out /src/optee/optee_test/ta/identity_subkey2.bin --max-depth 0 --name-size 0 \
        --subkey-version 1 && \

    # Build and test the OP-TEE OS and client
    make -j 4 check
    cd /src && \
    [ ! -d optee_client ] && git clone https://github.com/OP-TEE/optee_client.git && \
    cd optee_client && \
    mkdir -p build && \
    cd build && \
    export PATH=/src/optee/toolchains/aarch64/bin:$PATH && \
    export CC=aarch64-linux-gnu-gcc && \
    cmake .. -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_INSTALL_PREFIX=/src/optee/toolchains/aarch64 && \
    make -j 4 VERBOSE=1 && \
    make install"
```

### Step 2 (QEMU or NanoPC): Building OP-TEE Libdogecoin Key Manager Enclave

This command builds the OP-TEE Libdogecoin Key Manager Enclave for QEMU ARMv8 or NanoPC-T6. The enclave is built using the OP-TEE SDK and client. The enclave binary is located in `/doge/libdogecoin/optee/out/bin/libdogecoin.img`.

Note that libdogecoin is built twice: once for the host and once for OP-TEE. The host build is used to build the host application, while the OP-TEE build is used to build the enclave. The CFLAGS=-Wp,-D_FORTIFY_SOURCE=0 flag is used to disable fortify source checks, which are not supported by OP-TEE.

```sh
docker run --privileged -v "$(pwd):/src" -w /src jforissier/optee_os_ci:qemu_check /bin/bash -c "\
    # Set up the environment and build libdogecoin
    apt-get update && \
    apt-get install -y autoconf automake libtool-bin build-essential curl python3 valgrind g++-aarch64-linux-gnu qemu-user-static qemu-user && \

    # Build libdogecoin for Host
    make -j 4 -C depends HOST=aarch64-linux-gnu && \
    ./autogen.sh && \
    ./configure --prefix=/src/depends/aarch64-linux-gnu LIBS=-levent_pthreads --enable-static --disable-shared --enable-test-passwd HOST=aarch64-linux-gnu && \
    make -j 4 && \
    make install && \

    export PATH=/src/optee/toolchains/aarch64/bin:$PATH && \
    export CC=aarch64-linux-gnu-gcc && \

    # Build the Host
    cd /src/src/optee/host && \
    make -j 4 \
      CROSS_COMPILE=aarch64-linux-gnu- \
      LDFLAGS=\"-L/src/optee/toolchains/aarch64/lib -L/src/depends/aarch64-linux-gnu/lib -ldogecoin -lunistring\" \
      CFLAGS=\"-I/src/optee/toolchains/aarch64/include -I/src/src/optee/ta/include -I/src/depends/aarch64-linux-gnu/include -I/src/depends/aarch64-linux-gnu/include/ykpers-1 -I/src/depends/aarch64-linux-gnu/include/dogecoin\" && \

    # Build libdogecoin for OP-TEE
    cd /src/ && \
    make -j 4 -C depends CFLAGS=-Wp,-D_FORTIFY_SOURCE=0 HOST=aarch64-linux-gnu && \
    ./configure --prefix=/src/depends/aarch64-linux-gnu LIBS=-levent_pthreads --enable-static --disable-shared --enable-test-passwd --enable-optee CFLAGS=-Wp,-D_FORTIFY_SOURCE=0 HOST=aarch64-linux-gnu && \
    make -j 4 && \
    make install && \

    # Build the Enclave
    cd /src/src/optee/ta && \
    make -j 4 \
      CROSS_COMPILE=aarch64-linux-gnu- \
      LIBDIR=/src/depends/aarch64-linux-gnu/lib \
      LDFLAGS=\"-L/src/depends/aarch64-linux-gnu/lib -ldogecoin -lunistring\" \
      CFLAGS=\"-I/src/depends/aarch64-linux-gnu/include -I/src/depends/aarch64-linux-gnu/include/dogecoin\" \
      PLATFORM=vexpress-qemu_armv8a \
      TA_DEV_KIT_DIR=/src/optee/optee_os/out/arm/export-ta_arm64 && \

    # Create symbolic links and prepare image
    mkdir -p /src/optee/out/bin && \
    cd /src/optee/out/bin && \
    ln -sf ../../linux/arch/arm64/boot/Image Image && \
    ln -sf ../../trusted-firmware-a/build/qemu/release/bl1.bin bl1.bin && \
    ln -sf ../../trusted-firmware-a/build/qemu/release/bl2.bin bl2.bin && \
    ln -sf ../../trusted-firmware-a/build/qemu/release/bl31.bin bl31.bin && \
    ln -sf ../../optee_os/out/arm/core/tee-header_v2.bin bl32.bin && \
    ln -sf ../../optee_os/out/arm/core/tee-pager_v2.bin bl32_extra1.bin && \
    ln -sf ../../optee_os/out/arm/core/tee-pageable_v2.bin bl32_extra2.bin && \
    ln -sf ../../edk2/Build/ArmVirtQemuKernel-AARCH64/RELEASE_GCC5/FV/QEMU_EFI.fd bl33.bin && \
    ln -sf ../../out-br/images/rootfs.cpio.gz rootfs.cpio.gz && \
    dd if=/dev/zero of=/src/optee/out/bin/libdogecoin.img bs=1M count=32 && \
    mkfs.ext4 /src/optee/out/bin/libdogecoin.img && \
    mkdir -p /src/optee/out-br/mnt && \
    mount -o loop /src/optee/out/bin/libdogecoin.img /src/optee/out-br/mnt && \
    cp /src/src/optee/ta/*.ta /src/optee/out-br/mnt && \
    cp /src/src/optee/host/optee_libdogecoin /src/optee/out-br/mnt && \
    cp /src/spvnode /src/optee/out-br/mnt && \
    cp /src/sendtx /src/optee/out-br/mnt && \
    cp /src/such /src/optee/out-br/mnt && \
    cp /src/tests /src/optee/out-br/mnt && \
    cp /src/bench /src/optee/out-br/mnt && \
    mkdir -p /src/optee/out-br/mnt/data/tee && \
    umount /src/optee/out-br/mnt && \
    exit"
```

### Step 3 (NanoPC): Running OP-TEE Libdogecoin Key Manager Enclave

Use scp to copy the /doge/libdogecoin/optee/out/bin/libdogecoin.img to the NanoPC-T6 (e.g. `scp /doge/libdogecoin/optee/out/bin/libdogecoin.img root@192.168.137.19:/root/`). Then, SSH into the NanoPC-T6 and run the following commands:

```sh
mkdir /media/libdogecoin
mount /root/libdogecoin.img /media/libdogecoin
cd /media/libdogecoin
cp /media/libdogecoin/62d95dc0-7fc2-4cb3-a7f3-c13ae4e633c4.ta /lib/optee_armtz/
./optee_libdogecoin -c generate_mnemonic
```

### Step 3 (QEMU): Running OP-TEE Libdogecoin Key Manager Enclave

```sh
docker run --privileged -v /dev/bus/usb:/dev/bus/usb -it -v "$(pwd):/src" -w /src jforissier/optee_os_ci:qemu_check /bin/bash -c "\
  chmod 777 /src/optee/qemu/build/aarch64-softmmu/qemu-system-aarch64 && \
  cd /src/optee/out/bin && \
  /src/optee/qemu/build/aarch64-softmmu/qemu-system-aarch64 \
    -L /src/optee/qemu/pc-bios \
    -nographic \
    -serial mon:stdio \
    -serial file:/src/optee/serial1.log \
    -smp 2 \
    -machine virt,secure=on,mte=off,gic-version=3 \
    -cpu max,pauth-impdef=on \
    -d unimp \
    -semihosting-config enable=on,target=native \
    -m 1057 \
    -bios bl1.bin \
    -initrd rootfs.cpio.gz \
    -kernel Image \
    -no-acpi \
    -drive file=libdogecoin.img,format=raw,id=libdogecoin,if=none \
    -device virtio-blk-device,drive=libdogecoin \
    -append 'console=ttyAMA0,38400 keep_bootcon root=/dev/vda2' \
    -usb \
    -device pci-ohci,id=ohci \
    -device usb-host,vendorid=0x1050,productid=0x0407"
```

`-v /dev/bus/usb:/dev/bus/usb` is used to pass the USB bus and YubiKey device to the container. The `-device usb-host,vendorid=0x1050,productid=0x0407` flag is used to pass the YubiKey to the QEMU VM. Replace `0x1050` and `0x0407` with the YubiKey's vendor and product IDs.

The QEMU ARMv8 emulator will boot the OP-TEE OS and Trusted Firmware, and the libdogecoin TA will be loaded into the enclave. The host application can then interact with the enclave to perform key management operations.

In the QEMU terminal, run the following commands to start the libdogecoin TA:

```sh
# Mount the drive and copy the TA, host application, and data if needed
mkdir /mnt/libdogecoin && \
mount /dev/vda /mnt/libdogecoin && \
cp /mnt/libdogecoin/62d95dc0-7fc2-4cb3-a7f3-c13ae4e633c4.ta /lib/optee_armtz/ && \
cp /mnt/libdogecoin/optee_libdogecoin /usr/bin/ && \
if [ "$(ls -A /mnt/libdogecoin/data/tee/)" ]; then
  cp /mnt/libdogecoin/data/tee/* /data/tee/ && \
  chown tee:tee /data/tee/*; \
fi
cd /usr/bin/ && \
chmod 777 optee_libdogecoin
chmod 644 /lib/optee_armtz/62d95dc0-7fc2-4cb3-a7f3-c13ae4e633c4.ta

# Run the OP-TEE Libdogecoin Key Manager Enclave (see tutorial for commands)
optee_libdogecoin -c generate_mnemonic

# When finished, copy the data back to the drive
cp -r /data/tee/* /mnt/libdogecoin/data/tee/

# Unmount the drive and power off the system
umount /mnt/libdogecoin
poweroff
```

### Building OpenEnclave Libdogecoin Key Manager Enclave

**Dependencies**

- Ubuntu 20.04 or later
- Docker
- Libdogecoin

```sh
sudo apt-get install docker.io

mkdir -p /doge
cd /doge
git clone https://github.com/dogecoinfoundation/libdogecoin.git
cd libdogecoin
```

This command uses package installs for the OpenEnclave SDK and Docker to build the OpenEnclave Libdogecoin Key Manager Enclave. Docker is used to build the enclave in a clean environment. Refer to the [OpenEnclave documentation](https://github.com/openenclave/openenclave/blob/master/docs/GettingStartedDocs/Contributors/BuildingInADockerContainer.md) for more information.

```sh
docker run --device /dev/sgx_enclave:/dev/sgx_enclave --device /dev/sgx_provision:/dev/sgx_provision -it -v $PWD:/src -w /src ubuntu:20.04 bash -c "\
  # Set up the environment and build libdogecoin
  export DEBIAN_FRONTEND=noninteractive && \
  apt-get update && \
  apt-get install -y autoconf automake libtool-bin build-essential curl python3 valgrind python3-dev python3-dbg pkg-config && \
  cd /src && \

  # Build libdogecoin for Enclave
  make -j 4 -C depends HOST=x86_64-pc-linux-gnu && \
  ./autogen.sh && \
  ./configure --prefix=/src/depends/x86_64-pc-linux-gnu --enable-openenclave --enable-test-passwd CFLAGS=-Wp,-D_FORTIFY_SOURCE=0 && \
  make && \
  make install && \

  # Build libdogecoin for Host
  make -j 4 -C depends HOST=x86_64-pc-linux-gnu/host && \
  ./configure --prefix=/src/depends/x86_64-pc-linux-gnu/host --enable-test-passwd && \
  make && \
  make install && \

  # Set up the OpenEnclave environment and build the enclave
  apt-get install -y wget gnupg2 cmake && \
  echo 'deb [arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu focal main' | tee /etc/apt/sources.list.d/intel-sgx.list && \
  wget -qO - https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | apt-key add - && \
  echo 'deb http://apt.llvm.org/focal/ llvm-toolchain-focal-11 main' | tee /etc/apt/sources.list.d/llvm-toolchain-focal-11.list && \
  wget -qO - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
  echo 'deb [arch=amd64] https://packages.microsoft.com/ubuntu/20.04/prod focal main' | tee /etc/apt/sources.list.d/msprod.list && \
  wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add - && \
  apt update && \
  apt -y install clang-11 libssl-dev gdb libsgx-enclave-common libsgx-quote-ex libprotobuf17 libsgx-dcap-ql libsgx-dcap-ql-dev az-dcap-client open-enclave && \
  apt -y install dkms && \
  source /opt/openenclave/share/openenclave/openenclaverc && \
  mkdir -p /src/src/openenclave/build && cd /src/src/openenclave/build && \
  cmake .. && make && make run && \
  exec bash"
```

Once the build is complete, see the OpenEnclave Host Command Line Tutorial below for instructions on running the OpenEnclave Libdogecoin Key Manager Enclave.

## Host Command Line Tutorials

In this section, we provide a step-by-step tutorial on using the host command line interface to interact with the key management enclaves in both environments. This tutorial will cover the basic operations including generating mnemonic seedphrases, generating public keys, generating addresses, signing messages, and signing transactions.

### Prerequisites

Before proceeding, ensure you have successfully built and set up the OP-TEE and OpenEnclave environments as described in the previous sections.

### OP-TEE Host Command Line Tutorial

#### Generating a Mnemonic Seedphrase

Generate a mnemonic seedphrase for backup and recovery purposes. This is the first step in creating a new wallet. It will only be displayed once, so make sure to back it up.

This command will generate a mnemonic seedphrase and display it on the screen. Either a shared secret for TOTP or a password must be provided. All other flags are optional, and the user will be prompted for input if not provided.

Enter the shared secret for TOTP when prompted if enabled. The shared secret must be 40 hex characters (*e.g., `f38243e0e3e97a5c8aa5cc481a815add6c119648`*).  If no shared secret is provided, a random one will be generated. The shared secret will be set on the YubiKey (if present) by the host application, and the mnemonic will be generated and displayed. If OTP Slot 1 is already programmed, the host application will prompt you to overwrite it. Ensure that your YubiKey slot configuration is not overwritten unless intended.

```sh
optee_libdogecoin -c generate_mnemonic -n <mnemonic_input> -s <shared_secret> -e <entropy_size> -p <password> -f <flags> -z
```

The `-n` flag is used to provide the mnemonic input for recovery purposes if needed. Replace `<mnemonic_input>` with the mnemonic seedphrase you want to use for recovery. If no mnemonic input is provided, the enclave will generate a random mnemonic seedphrase.

The `-s` flag is used to provide the shared secret for TOTP authentication from command line instead of prompting the user. Replace `<shared_secret>` with the shared secret you want to use for TOTP authentication.

The `-e` flag is used to provide the entropy size for the mnemonic seedphrase. Replace `<entropy_size>` with the desired entropy size in bits. If no entropy size is provided, the default value of `"256"` bits (24 words) will be used.

The `-p` flag is used to provide the password for the mnemonic seedphrase. Replace `<password>` with the password you want to use for the mnemonic seedphrase.

The `-f` flag is used to provide additional flags for the mnemonic seedphrase. Replace `<flags>` with the desired flags for the mnemonic seedphrase. The `"delegate"` flag is used to delegate account keys to a third party.

The `-z` flag is used to enable YubiKey authentication. If the YubiKey is present, the shared secret will be set on the YubiKey.

#### Generating an Extended Public Key

Generate an extended public key using the account and change level. The change level can be set to 0 for external addresses and 1 for internal addresses. The account number is the BIP-44 account. The public key will be derived from the seedphrase stored within the enclave.

The `-o` flag is used to provide the account number for the extended public key. Replace `<account_number>` with the desired account number.

The `-l` flag is used to provide the change level for the extended public key. Replace `<change_level>` with the desired change level.

The `-h` flag is used to provide a custom path for the extended public key. Replace `<custom_path>` with the desired path for the extended public key.

The `-a` flag is used to provide the auth token if a Yubikey is not present. Use a tool like `oathtool` to generate TOTP (*e.g., `oathtool --totp "f38243e0e3e97a5c8aa5cc481a815add6c119648"`).

The `-p` flag is used to provide the password for the mnemonic seedphrase.

The `-z` flag is used to enable YubiKey authentication. If the YubiKey is present, the TOTP code will be retrieved from the YubiKey and used as the authentication token.

```sh
optee_libdogecoin -c generate_extended_public_key -o <account_number> -l <change_level> -h <custom_path> -a <auth_token> or -p <password> -z
```

#### Generating an Address

Generate a Dogecoin address using the account, address index, and change level.

```sh
optee_libdogecoin -c generate_address -o <account_number> -l <change_level> -i <address_index> -h <custom_path> -a <auth_token> or -p <password> -z
```

Replace `<account_number>`, `<address_index>`, and `<change_level>` with appropriate values. The change level can be set to 0 for external addresses and 1 for internal addresses. The account number is the BIP-44 account. The address index is the index of the address within the account. The address will be derived from the seedphrase stored within the enclave.

#### Signing a Message

Sign a message using the private key stored within the enclave. The message will be signed using the private key derived from the seedphrase stored within the enclave. If no message is provided, an example message will be signed for demonstration purposes.

```sh
optee_libdogecoin -c sign_message -o <account_number> -l <change_level> -i <address_index> -m <message> -h <custom_path> -a <auth_token> or -p <password> -z
```

Replace `<message>` with the message you want to sign.

#### Signing a Transaction

Sign a raw transaction using the private key stored within the enclave. The transaction will be signed using the private key derived from the seedphrase stored within the enclave. A raw transaction is a hexadecimal string representing the transaction data. Currently, if no transaction data is provided, an example transaction will be signed for demonstration purposes.

```sh
optee_libdogecoin -c sign_transaction -o <account_number> -l <change_level> -i <address_index> -t <raw_transaction> -h <custom_path> -a <auth_token> or -p <password> -z
```

Replace `<raw_transaction>` with the raw transaction data.

#### Delegate Key

Delegate account keys to a third party. This operation allows a third party to manage the keys for a specific account. The third party will be able to export the account keys on behalf of the user using the delegate password.

```sh
optee_libdogecoin -c delegate_key -o <account_number> -d <delegate_password> -h <custom_path> -a <auth_token> or -p <password> -z
```

Replace `<delegate_password>` with the password for the delegate account.

#### Export Delegate Key

Export the delegated account keys using the delegate password. This operation allows the third party to export the account keys on behalf of the user.

```sh
optee_libdogecoin -c export_delegate_key -o <account_number> -d <delegate_password>
```

Replace `<delegate_password>` with the password for the delegate account.

### OpenEnclave Host Command Line Tutorial

Note: In OpenEnclave, `--simulate` is used to run the enclave in simulation mode. This is useful for testing without SGX hardware, but it is not secure. For production use, remove `--simulate` to run on real SGX hardware.

#### Generating a Mnemonic Seedphrase

Generate a mnemonic seedphrase for backup and recovery purposes. This is the first step in creating a new wallet. It will only be displayed once, so make sure to back it up.

This command will generate a mnemonic seedphrase and display it on the screen. Either a shared secret for TOTP or a password must be provided. All other flags are optional, and the user will be prompted for input if not provided.

Enter the shared secret for TOTP when prompted if enabled. The shared secret must be 40 hex characters (*e.g., `f38243e0e3e97a5c8aa5cc481a815add6c119648`*).  If no shared secret is provided, a random one will be generated. The shared secret will be set on the YubiKey (if present) by the host application, and the mnemonic will be generated and displayed. If OTP Slot 1 is already programmed, the host application will prompt you to overwrite it.

```sh
/doge/libdogecoin/src/openenclave/build/host/host /doge/libdogecoin/src/openenclave/build/enclave/enclave.signed --simulate -c generate_mnemonic -n <mnemonic_input> -s <shared_secret> -e <entropy_size> -p <password> -z
```

The `-n` flag is used to provide the mnemonic input for recovery purposes if needed. Replace `<mnemonic_input>` with the mnemonic seedphrase you want to use for recovery. If no mnemonic input is provided, the enclave will generate a random mnemonic seedphrase.

The `-s` flag is used to provide the shared secret for TOTP authentication from command line instead of prompting the user. Replace `<shared_secret>` with the shared secret you want to use for TOTP authentication.

The `-e` flag is used to provide the entropy size for the mnemonic seedphrase. Replace `<entropy_size>` with the desired entropy size in bits. If no entropy size is provided, the default value of `"256"` bits (24 words) will be used.

The `-p` flag is used to provide the password for the mnemonic seedphrase. Replace `<password>` with the password you want to use for the mnemonic seedphrase.

The `-z` flag is used to enable YubiKey authentication. If the YubiKey is present, the shared secret will be set on the YubiKey.

#### Generating an Extended Public Key

Generate an extended public key using the account and change level. The change level can be set to 0 for external addresses and 1 for internal addresses. The account number is the BIP-44 account.

The TOTP code will be retrieved from the YubiKey (if present) and used as the authentication token for this operation.

```sh
/doge/libdogecoin/src/openenclave/build/host/host /doge/libdogecoin/src/openenclave/build/enclave/enclave.signed --simulate -c generate_extended_public_key -o <account_number> -l <change_level> -h <custom_path> -a <auth_token> or -p <password> -z
```

The `-o` flag is used to provide the account number for the extended public key. Replace `<account_number>` with the desired account number.

The `-l` flag is used to provide the change level for the extended public key. Replace `<change_level>` with the desired change level.

The `-h` flag is used to provide a custom path for the extended public key. Replace `<custom_path>` with the desired path for the extended public key.

The `-a` flag is used to provide the auth token if a Yubikey is not present. Use a tool like `oathtool` to generate TOTP (*e.g., `oathtool --totp "f38243e0e3e97a5c8aa5cc481a815add6c119648"`).

The `-p` flag is used to provide the password for the mnemonic seedphrase.

The `-z` flag is used to enable YubiKey authentication. If the YubiKey is present, the TOTP code will be retrieved from the YubiKey and used as the authentication token.

#### Generating an Address

Generate a Dogecoin address using the account, address index, and change level.

```sh
/doge/libdogecoin/src/openenclave/build/host/host /doge/libdogecoin/src/openenclave/build/enclave/enclave.signed --simulate -c generate_address -o <account_number> -l <change_level> -i <address_index> -h <custom_path> -a <auth_token> or -p <password> -z
```

Replace `<account_number>`, `<address_index>`, and `<change_level>` with appropriate values.

#### Signing a Message

Sign a message using the private key stored within the enclave. If no message is provided, an example message will be signed for demonstration purposes.

```sh
/doge/libdogecoin/src/openenclave/build/host/host /doge/libdogecoin/src/openenclave/build/enclave/enclave.signed --simulate -c sign_message -o <account_number> -l <change_level> -i <address_index> -m "This is just a test message" -h <custom_path> -a <auth_token> or -p <password> -z
```

Replace `"This is just a test message"` with the message you want to sign.

#### Signing a Transaction

Sign a raw transaction using the private key stored within the enclave. The transaction will be signed using the private key derived from the seedphrase stored within the enclave. A raw transaction is a hexadecimal string representing the transaction data. Currently, if no transaction data is provided, an example transaction will be signed for demonstration purposes.

```sh
/doge/libdogecoin/src/openenclave/build/host/host /doge/libdogecoin/src/openenclave/build/enclave/enclave.signed --simulate -c sign_transaction -o <account_number> -l <change_level> -i <address_index> -t <raw_transaction> -h <custom_path> -a <auth_token> or -p <password> -z
```

Replace `<raw_transaction>` with the raw transaction data.

## Conclusion

In this document, we explored the integration of libdogecoin with secure enclaves in Trusted Execution Environments (TEEs). We discussed the benefits of using Intel SGX and ARM TrustZone to secure key management operations, including mnemonic seedphrase generation, public key generation, address generation, message signing, and transaction signing. We provided a step-by-step tutorial on using the host command line interface to interact with the key management enclaves in both OP-TEE and OpenEnclave environments. We also outlined critical vulnerabilities in TEEs and provided recommendations for mitigating these risks. We hope this document serves as a valuable resource for developers looking to enhance the security of Dogecoin transactions using secure enclaves.
