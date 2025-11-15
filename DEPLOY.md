Deploying to Winlator:
1. Build on CI using the included GitHub Actions workflow, or build locally with Android NDK r25b.
2. Extract the produced tarball and copy usr/ to Winlator's driver directory (per Winlator docs).
3. Use included profiles to force the layer and tune performance.

Notes about bit-exactness:
- Test on device with authoritative test vectors in tests/vectors/.
- Adjust shader parsing bit offsets if validation diffs show mismatches.
