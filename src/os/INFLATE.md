# Bundled gzip inflate manifest

The game decodes its gzip data with `inflate.c` from GNU gzip 1.2.4, Mark Adler's public domain
decoder at version c10p1 (10 January 1993). The routines are C with unmangled names. The image also
links the gzip compression side (`zip`, `deflate`, `trees`, and `bits`), and no caller uses it.

The upstream files are vendored at `3rdparty/gzip-1.2.4/` with the release's top directory name
preserved. Only `inflate.c`, the two headers it includes (`gzip.h` and `tailor.h`), `README`, and
`COPYING` are imported. `gzip.h` and `tailor.h` are under the GPL, and `inflate.c` is public
domain.

[inflate.c](inflate.c) is the game side. It maps the upstream names onto the loader's gzip state,
replaces the allocator, includes the vendored `inflate.c`, and defines the table pool.

## Version evidence

| Feature in the image                                                             | 1.2.4 | 1.3.4 | 1.3.9 |
| -------------------------------------------------------------------------------- | ----- | ----- | ----- |
| `huft_build` reports 0 for all-zero lengths (`0x0063c7e0`)                       | yes   | yes   | no    |
| `inflate_dynamic` does not reject a null bit-length table                        | yes   | no    | no    |
| `fill_inbuf` is called without first storing the window position to `outcnt`     | yes   | no    | no    |
| `" incomplete literal tree\n"` and `" incomplete distance tree\n"` via `fprintf` | yes   | yes   | no    |
| `lbits` 9 and `dbits` 6 as data (`0x007c3c0c`, `0x007c3c10`)                     | yes   | yes   | yes   |

No 1.2 release after 1.2.4 exists, and the 1.3 changes above are absent from both regions.

The build options read from the image are these:

- `PKZIP_BUG_WORKAROUND` is undefined. The count test is `nl > 286 || nd > 30` (`sltiu` 287 and
  31), and an incomplete distance tree fails the block.
- `CRYPT` and `DEBUG` are undefined, and `NOMEMCPY` is undefined. `inflate_codes` copies through
  `memcpy` when the source and destination do not overlap.

## Game configuration

These macros precede the include and do not modify the upstream file:

| Upstream name  | Game definition                                         |
| -------------- | ------------------------------------------------------- |
| `inbuf`        | `gzipInbuf`                                             |
| `insize`       | `gzipInsize`                                            |
| `inptr`        | `gzipInptr`                                             |
| `outcnt`       | `gzipOutcnt` (upstream `wp`)                            |
| `window`       | `gzipWindow` (upstream `slide`)                         |
| `fill_inbuf`   | `GzipRefillInputBuffer`                                 |
| `flush_window` | `GzipFlushWindow`                                       |
| `malloc`       | `HuftAlloc`, given the entry count rather than the size |
| `free`         | Nothing                                                 |

`HuftAlloc` carves tables from a 2048-entry pool. It advances the cursor even when the allocation
fails, fails an allocation that ends exactly at the end of the pool, and logs
`"HUFT MEMORY EXCEEDED!!\n"` through `LogPrintf` on failure. The image inlines it into
`huft_build`.

Retail `huft_free` is reduced to `return 0` and is inlined away in every caller after it. The
vendored `huft_free` still walks the table chain and calls the empty `free` for each table. The
walk only reads the link entries of tables the same block built, and the result is the same zero.

## Edit to the vendored source

`inflate()` calls `HuftReset()` before each block, ahead of `hufts = 0`. The call is marked with a
comment in `3rdparty/gzip-1.2.4/inflate.c`. `HuftReset` records the most pool entries one block has
used and rewinds the pool. The image inlines it into `inflate()`. The peak is never read elsewhere.

## Differences that do not change behaviour

- `ulg` is 64 bits wide in the image and 32 bits wide in this build. The bit buffer never stores
  more than 23 bits.
- Upstream's local `h` in `inflate()` tracks the most `hufts` of one block. It is unused without
  `DEBUG`, and the image omits it.

## Addresses

| Routine           | NTSC-U/C     | PAL          |
| ----------------- | ------------ | ------------ |
| `huft_build`      | `0x0063c748` | `0x0067d2d8` |
| `inflate_codes`   | `0x0063cd50` | `0x0067d8e0` |
| `inflate_stored`  | `0x0063d348` | `0x0067ded8` |
| `inflate_fixed`   | `0x0063d5c8` | `0x0067e158` |
| `inflate_dynamic` | `0x0063d720` | `0x0067e2b0` |
| `inflate_block`   | `0x0063def8` | `0x0067ea88` |
| `inflate`         | `0x0063e0b0` | `0x0067ec40` |
| `HuftReset`       | `0x0063e1a8` | `0x0067ed38` |
| `HuftAlloc`       | `0x0063e1e0` | `0x0067ed70` |
| `huft_free`       | `0x0063e230` | `0x0067edc0` |

| Global      | NTSC-U/C     | PAL          |
| ----------- | ------------ | ------------ |
| `border`    | `0x007c3a98` | `0x00807798` |
| `cplens`    | `0x007c3ae8` | `0x008077e8` |
| `cplext`    | `0x007c3b28` | `0x00807828` |
| `cpdist`    | `0x007c3b68` | `0x00807868` |
| `cpdext`    | `0x007c3ba8` | `0x008078a8` |
| `mask_bits` | `0x007c3be8` | `0x008078e8` |
| `lbits`     | `0x007c3c0c` | `0x0080790c` |
| `dbits`     | `0x007c3c10` | `0x00807910` |
| `bb`        | `0x008ee930` | `0x00933930` |
| `bk`        | `0x008ee938` | `0x00933938` |
| `hufts`     | `0x008ee93c` | `0x0093393c` |
| `huftTable` | `0x008ea930` | `0x0092f930` |
| `pHuftNext` | `0x007c3a90` | `0x00807790` |
| `highWater` | `0x007c3a94` | `0x00807794` |
