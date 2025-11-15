# Validation harness

To achieve bit-exactness, follow these steps:

1. Populate `tests/vectors/` with authoritative BC6H and BC7 reference blocks and their decoded RGBA outputs.
2. Run `tests/run_validation.sh` on a machine that can execute the built library (or run within Android device with proper test runner).
3. The validation runner will:
   - Use the CPU decoder to decode reference blocks.
   - Run the GPU decoder shader to decode the same blocks (via a small Vulkan test harness).
   - Compare outputs and produce diffs.

CI: GitHub Actions can be extended to run the validation if a suitable Android emulator/device is available in the runner or via connected hardware.
