
## Possible bugs
- in dircache updates

## unadf

- unadf on Windows, versions 0.10.2 and earlier
  Bug in file extraction (extracted binary data might be corrupted).

## Write support

Recent issues:

- Versions 0.9.0 and earlier
  Bug on file appending on OFS (not updated data
  size in OFS data block header if written data chunk is bigger than space
  remaining in the block)
  - not the most common case and it does not cause any harm on reading/writing
    with ADFlib only - but on systems where OFS data block header information
	is used	(AmigaOS), it may lead to errors and loss of data

- Versions 0.8.0 and ealier **do not check or rebuild the block allocation
  bitmap for volumes where it is marked invalid**. This means that if
  the bitmap is really incorrect, writing to such volume may lead to data
  loss/corruption (blocks used by stored data might not be marked as such!).


## Past security bugs

Please note that several security issues/bugs has been found in the older (0.7.x)
versions of the ADFlib:
- `CVE-2016-1243` and `CVE-2016-1244`, fixed in
[8e973d7](https://github.com/adflib/ADFlib/commit/8e973d7b894552c3a3de0ccd2d1e9cb0b8e618dd)),
(found in Debian version `unadf/0.7.11a-3`, fixed in versions `unadf/0.7.11a-4`,
`unadf/0.7.11a-3+deb8u1`). See https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=838248
- Stuart Caie fixed arbitrary directory traversal in
[4ce14b2](https://github.com/adflib/ADFlib/commit/4ce14b2a8b6db84954cf9705459eafebabecf3e4)
lines 450-455

**Please update to the latest released version where these,
as well as many other things, are fixed.**
