[![Build Status](https://travis-ci.org/wolfbox/fangfs.png)](https://travis-ci.org/wolfbox/fangfs)
<a href="https://scan.coverity.com/projects/3203">
    <img alt="Coverity Scan Build Status"
	     src="https://scan.coverity.com/projects/3203/badge.svg"/>
</a>

FangFS is a FUSE-based encrypted filesystem using libsodium.

Why?
====

A few reasons.

* File systems, especially encrypted file systems, do not belong in the
  kernel.

* libsodium primitives such as salsa20 are more future-proof and
  yield higher performance than more traditional choices.

* Using FUSE allows for more usable file systems.

* Because FangFS uses FUSE, it is operating-system and filesystem-agnostic,
  and should work on any platform supported by both FUSE and libsodium.

* Certain other FUSE-based encrypted filesystem services are plagued
  by poor use of cryptographic constructions.

Should I use this?
==================

GOSH no.  This is an experiment---nothing more.

Repeated: DO NOT USE for serious business.  FangFS may devour you alive!
