Goals
=====

- Any (key, nonce) pair is NEVER reused.
- Key revocation.
- As many file attributes as possible are encrypted.

Cryptographic Primitives
========================

Traditionally, the AES cipher is combined with some algorithm
such as CTR, GCM, or in the case of block device encryption, XTS.
Key derivation is typically performed through PBKDF2.

There is nothing wrong with this approach. However, stronger primitives are
now available with higher-level abstractions, and so the libsodium fork
of djb's excellent NaCl cryptography toolkit is used.

The upshot of this is that the xsalsa20 stream cipher provides the symmetric
encryption, Poly1305 provides authentication, and a key derivation function
derived from the memory-hard scrypt function is used to stretch the encryption
key.

Anatomy of a File
=================

Structure
---------

    filename: authenc(orig.filename)
    data: header . authenc(orig.data)
    stat: ?:nobody 600
          mtime: epoch
          atime: epoch
    xattrs
        __fangfs-stat: authenc(orig.stat)
        xattr: authenc(orig.<xattr>)

Header
------

The header contains the following unpadded little-endian fields:

    uint8_t version;

Operational Design
==================

EVERY time a file or its attributes are modified, a new nonce is generated.
Files are divided into file system-sized blocks, and each block is
encrypted along with its nonce in following un-padded structure:

    char nonce[NONCEBYTES];
    char ciphertext[LEN];

where LEN is on 0..BLOCKSIZE inclusive.  Generating a new nonce on every edit
is necessary, because one key is used for the entire FangFS filesystem.

A file called /.__FANGFS_META in the *source* filesystem contains the
following unpadded little-endian fields:

    uint8_t version;
    uint32_t block_size;
  
    for each child key:
      uint32_t opslimit;
      uint32_t memlimit;
      authenc(MasterKey, ChildKey)

Access Revocation
=================

If a key is thought to be compromised, it may be useful to revoke that key
without needing to reencrypt the entire filesystem. Additionally, if multiple
users need to be able to mount the filesystem, it is best if they all do not
need to share the same key.

To explain the FangFS solution to this, consider the following example:

Bob runs an encrypted filesystem. The meta file contains two key fields:

    authenc(MasterKey, BobKey)
    authenc(MasterKey, AliceKey)

When Alice mounts the filesystem, FangFS decrypts MasterKey using her key.
At any time, her key may be removed, or more keys may be added.

It is important to understand that Alice *does* know MasterKey, and so this
approach is *not* intended to prevent potentially hostile users from being
able to access the filesystem in the future.
