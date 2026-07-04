# JPEG XL conformance corpus

The valid JPEG XL files in this directory are copied from `specs/jxl` and
retain its category structure. They originate from the libjxl conformance and
test repositories described there. See `LICENSE` for the upstream license.

The `invalid` directory contains malformed variants derived from
`edge-cases/basic.jxl`. They cover an empty input, a damaged codestream
signature, a truncated header, and a truncated payload. These files are
committed so the test suite does not generate fixtures at run time.

