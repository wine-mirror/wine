/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* MinGW/libstdc++ test: does NOT assert MSVC STL compatibility. */
#include <algorithm>
#include <atomic>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
int main() {
  bool good = true;
  std::vector<int> values(512);
  std::iota(values.begin(), values.end(), 1);
  std::sort(values.begin(), values.end(), std::greater<int>());
  const long sum = std::accumulate(values.begin(), values.end(), 0L);
  good = good && (sum == 131328) && (values.front() == 512);
  std::atomic<int> counter{0};
  std::vector<std::thread> threads;
  for (int i = 0; i < 4; ++i)
    threads.emplace_back([&counter]() { for (int j=0; j<5000; ++j) ++counter; });
  for (auto &thread : threads) thread.join();
  good = good && counter == 20000;
  bool caught = false;
  try { throw std::runtime_error("wine-cxx-exception"); }
  catch (const std::runtime_error &error) { caught = std::string(error.what()) == "wine-cxx-exception"; }
  good = good && caught;
  std::cout << "CXX_RUNTIME_TEST bits=" << (sizeof(void*)*8)
            << " sum=" << sum << " threads=" << counter.load()
            << " exception=" << (caught ? "PASS" : "FAIL")
            << " RESULT=" << (good ? "PASS" : "FAIL") << std::endl;
  return good ? 0 : 1;
}
