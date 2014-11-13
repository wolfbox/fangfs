Goals
=====

- Any (key, nonce) pair is never reused.
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

In the rest of this document, the following symbols are used:

- ``hash(msg)``
  - 128-bit BLAKE2 hash function.
- ``authenc(message, nonce, key)``
  - Authenticated encryption of the message, using the given nonce and key.
- ``authdec(ciphertext, nonce, key)``
  - Inverse of ``authenc``.
- MasterKey
  - The FangFS filesystem-wide master key. Randomly-generated.
- FilenameNonce
  - The FangFS filesystem-wide nonce for *filenames only*. Randomly-generated.

Anatomy of a File
=================

    filename: authenc(hash(orig.origPath) . orig.filename, FilenameNonce, MasterKey)
    data: [block_nonce . authenc(block.data) for block in orig.data]
    stat: ?:nobody 600
          mtime: epoch
          atime: epoch
    xattrs
        __fangfs-stat: authenc(orig.stat)
        xattr: authenc(orig.<xattr>)

Operational Design
==================

Files are divided into file system-sized blocks, and each block is
encrypted along with its nonce in the following un-padded structure:

    char nonce[NONCEBYTES];
    char ciphertext[LEN];

where LEN is on 0..BLOCKSIZE inclusive.  Generating a new nonce on every edit
is necessary, because one key is used for the entire FangFS filesystem.

A file called /.__FANGFS_META in the *source* filesystem contains the
following unpadded little-endian fields:

    uint8_t version;
    uint32_t block_size;
    uint8_t filename_nonce[24];
  
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

Filename Encryption
===================

It is crucial in an encrypted filesystem to also keep the names of each file
a secret. This is a common thing for stacked encrypted filesystems to punt,
and make optional, just because there's no "optimal" solution.

For FangFS, this is not acceptable. The solution it takes should perform at
least modestly, but does not leak any information.

Anatomy of a Filename
---------------------

A filename in FangFS is the concatenation of the following:

    authenc(hash(path) . filename)

where ``path`` is the full path including filename. This yields
32 bytes of overhead: 16 for the BLAKE2 hash, and 16 for the MAC.

Encoding
--------

Base32 with no padding.  May change.

Path Lookup
-----------

    def lookup_path(path):
        components = []
        current_path = ""
        for segment in path:
            current_path = path_join([current_path, segment])
            enc_current_path = authenc(hash(current_path) + segment), FilenameNonce, MasterKey)
            components.append(enc_current_path)

        return path_join(components)

Directory Iteration
-------------------

In the following, HASH_LENGTH is the length of the 128-bit BLAKE2 hash output.

    def iterate(dirpath):
        real_path = lookup_path(dirpath)
        for file in real_path:
        	decrypted = authdec(path_join(dirpath, file.name), FilenameNonce, MasterKey)
        	filename = decrypted[HASH_LENGTH:]
        	verify(hash(path_join(dirpath, filename), decrypted[:HASH_LENGTH]))
        	yield filename

Implications
------------

- Because the filename nonce is global, any unique path will always be
  encrypted in the same way. This is theoretically an information leak,
  but is nonetheless desirable for efficient path lookup, and is not a bug.
