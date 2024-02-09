#!/usr/bin/python3
import os
import sys
import requests, zipfile
from io import BytesIO
import hashlib
import subprocess
import tarfile
import glob
import shutil
import argparse
parser = argparse.ArgumentParser()
parser.add_argument("--host", help="provide target host triplet")
args = parser.parse_args()
host = ""
if args.host:
    host = args.host
    os.environ['host'] = host
elif os.environ['host']:
    host = os.environ['host']

assert host in ("arm-linux-gnueabihf",
                "aarch64-linux-gnu",
                "x86_64-linux-gnu",
                "x86_64-apple-darwin14",
                "x86_64-w64-mingw32",
                "i686-w64-mingw32",
                "i686-pc-linux-gnu",), "Invalid architecture."

hash = ""
if host == "arm-linux-gnueabihf":
    ext = ".tar.gz"
    hash = "d0b7f5f4fbabb6a10078ac9cde1df7eb37bef4c2627cecfbf70746387c59f914  dogecoin-1.14.6-arm-linux-gnueabihf.tar.gz"
elif host == "aarch64-linux-gnu":
    ext = ".tar.gz"
    hash = "87419c29607b2612746fccebd694037e4be7600fc32198c4989f919be20952db  dogecoin-1.14.6-aarch64-linux-gnu.tar.gz"
elif host == "x86_64-w64-mingw32":
    ext = ".zip"
    hash = "709490ac8464b015266884831a2b5b594efc8b2c17a7e6b85255058cbee049de  dogecoin-1.14.6-win64.zip"
elif host == "i686-w64-mingw32":
    ext = ".zip"
    hash = "c919fdc966764ec554273791699426448eedd8d305e76284e4f1cd1d5f0b5a7a  dogecoin-1.14.6-win32.zip"
elif host == "x86_64-apple-darwin14":
    ext = ".tar.gz"
    hash = "bf6123a91ba2829e03dc7c48865971dd615b78249f6ee41a4e796f942bd93869  dogecoin-1.14.6-osx-unsigned.dmg"
elif host == "x86_64-linux-gnu":
    ext = ".tar.gz"
    hash = "908c5dfc9e4b617aae0df9c8cd6986b5988a6b5086136df5cbac40ec63e0c31c  dogecoin-1.14.6-x86_64-linux-gnu.tar.gz"
elif host == "i686-pc-linux-gnu":
    ext = ".tar.gz"
    hash = "d70a438a3bc7d74e8baa99a00b70e33a806db19b673fb36617307603186208a4  dogecoin-1.14.6-i686-pc-linux-gnu.tar.gz"

print('Downloading started')
file = "dogecoin-1.14.6-" + host
base = "https://github.com/dogecoin/dogecoin/releases/download/v1.14.6/"
url = base + file + ext
sha256sums = base + "SHA256SUMS.asc"

req_sha = requests.get(sha256sums)
checksum = sha256sums.split('/')[-1]
with open(checksum,'wb') as output_checksum:
    output_checksum.write(req_sha.content)
print("\033[1;32m> Downloading SHA256SUMS.asc Completed\033[0m")

req = requests.get(url)
filename = url.split('/')[-1]
with open(filename,'wb') as output_file:
    output_file.write(req.content)
print("\033[1;32m> Downloading " + filename + " Completed\033[0m")

sha256_hash = hashlib.sha256()
with open(filename,"rb") as f:
    for byte_block in iter(lambda: f.read(4096),b""):
        sha256_hash.update(byte_block)

with open("SHA256SUMS.asc", "r") as get_hash:
    for line in get_hash.readlines():
        if filename and hash in line:
            if line.strip() == hash:
                if sha256_hash.hexdigest() != line.split()[0]:
                    print("\033[31m> checksums don't match!\033[0m")
                    exit(1)
                else:
                    print("\033[1;32m> checksums match!\033[0m")
            else:
                print("\033[31m> no valid checksum found!\033[0m")
                exit(1)

if ext == ".zip":
    zipfile= zipfile.ZipFile(BytesIO(req.content))
    zipfile.extractall(os.getcwd())
else:
    with tarfile.open(fileobj=BytesIO(req.content), mode='r:gz') as tar:
        tar.extractall(os.getcwd())
        tar.close()

deps_path = ["dogecoind"]
for f in deps_path:
    src = "dogecoin-1.14.6/bin/" + f
    src_path = os.path.join(os.getcwd(), src)
    if os.path.isdir('/usr/local/bin'):
        dst_path = os.path.join('/usr/local/bin', f)
        shutil.move(src_path, dst_path)

subprocess.run([os.path.join(os.getcwd(), "rpctest/spvtool.py")])

rmlist = ['./dogecoin-*', '*.dmg', '*.tar.gz', '*.zip', '*.asc']
for path in rmlist:
    for name in glob.glob(path):
        if os.path.isdir(name):
            try:
                shutil.rmtree(name)
            except OSError as e:
                print("Error: %s : %s" % (name, e.strerror))
        if os.path.isfile(name):
            try:
                os.remove(name)
            except OSError as e:
                print("Error: %s : %s" % (name, e.strerror))
