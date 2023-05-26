// https://github.com/bazelbuild/bazel/blob/abae5ca3e8142f93cf0c2597e3410ed955c4dd59/src/test/shell/bazel/cc_integration_test.sh#LL1954C1-L1957C2

// clang-format off

int main() {
  int array[10];
  return array[10];
}

// clang-format on
