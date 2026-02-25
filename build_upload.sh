bazel build :samwise --config=$1

picotool load bazel-bin/samwise.uf2 -f
