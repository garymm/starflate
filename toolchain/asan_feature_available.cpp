// https://github.com/bazelbuild/bazel/blob/abae5ca3e8142f93cf0c2597e3410ed955c4dd59/src/test/shell/bazel/cc_integration_test.sh#L1888-L1897

// clang-format off

int main() {
  volatile int* p;

  {
    volatile int x = 0;
    p = &x;
  }

  return *p;
}

// clang-format on
