bazel build :samwise --config=$1

bazel build --config=$1 \
    --aspects @pico-sdk//tools:uf2_aspect.bzl%pico_uf2_aspect \
    --output_groups=+pico_uf2_files \
    :samwise

picotool load bazel-bin/samwise.uf2 -f
