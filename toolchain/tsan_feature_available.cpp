// https://github.com/bazelbuild/bazel/blob/abae5ca3e8142f93cf0c2597e3410ed955c4dd59/src/test/shell/bazel/cc_integration_test.sh#L1918-L1933

// clang-format off
// NOLINTBEGIN

#include <thread>

int value = 0;

void increment() {
  ++value;
}

int main() {
  for (int i = 0; i < 100; ++i) {
    std::thread t1(increment);
    std::thread t2(increment);
    t1.join();
    t2.join();
  }

  return value;
}

// NOLINTEND
// clang-format on
