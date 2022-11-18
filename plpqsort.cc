#include <cassert>
#include <random>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <span>

const size_t init_prefix = 15;
const size_t min_prefix = 3;

struct sortable_t {
  int value_;
  sortable_t() {}
  explicit sortable_t(int i) : value_(i) {}
  std::string str() const {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "%02x", value_);
    return std::string(tmp);
  }
  operator std::string() const { return str(); }
};

std::ostream& operator<<(std::ostream& os, sortable_t v) {
  return os << std::string(v);
}
bool operator<(sortable_t x, sortable_t y) {
  return x.value_ < y.value_;
}
bool operator==(sortable_t x, sortable_t y) {
  return x.value_ == y.value_;
}

void ce(sortable_t& a, sortable_t& b) {
  sortable_t c = std::min(a, b);
  sortable_t d = std::max(a, b);
  a = c;
  b = d;
}

struct prng_t : public std::mt19937 {
  prng_t() {
    std::random_device rd;
    seed(rd());
  }
} prng;
template <typename T>
T randint(T lo, T hi) {
  std::uniform_int_distribution<T> d(lo, hi - 1);
  return d(prng);
}

struct once_t {
  bool ready_ = true;
  operator bool() {
    bool result = ready_;
    ready_ = false;
    return result;
  }
};

bool is_sorted(std::span<sortable_t> list, size_t prefix = SIZE_MAX,
               std::string header = "") {
  once_t send_header;
  const int stride = 16;
  bool success = true;
  for (size_t i = 0; i < list.size(); i += stride) {
    bool row_ok = true;
    auto prev = list[i];
    for (size_t j = 0; j < stride && i + j < list.size(); ++j) {
      auto p = list[i + j];
      bool ok = !(p < prev);
      row_ok &= ok;
      prev = p;
    }
    if (!row_ok) {
      if (send_header && header.size()) std::cout << header << std::endl;
      std::cout << "unsorted +" << std::setw(3) << i << "/" << list.size();
      auto prev = list[i];
      for (size_t j = 0; j < stride && i + j < list.size(); ++j) {
        auto p = list[i + j];
        char sep = ' ';
        if (i + j == prefix) sep = ':';
        if (p < prev) sep = '*';
        std::cout << sep << std::string(p);
        prev = p;
      }
      std::cout << std::endl;
      success = false;
    }
  }
  return success;
}

bool is_bounded(std::span<sortable_t> list, size_t prefix, bool left,
                sortable_t pivot, std::string header = "") {
  once_t send_header;
  const int stride = 16;
  bool success = true;
  for (size_t i = 0; i < list.size(); i += stride) {
    bool row_ok = true;
    for (size_t j = 0; j < stride && i + j < list.size(); ++j) {
      auto p = list[i + j];
      bool ok = left ? p < pivot : !(p < pivot);
      row_ok &= ok;
    }
    if (!row_ok) {
      if (send_header && header.size()) std::cout << header << std::endl;
      std::cout << "OOB " << pivot << " +" << std::setw(3)
                << i << "/" << list.size();
      for (size_t j = 0; j < stride && i + j < list.size(); ++j) {
        auto p = list[i + j];
        bool ok = left ? p < pivot : !(p < pivot);
        char sep = ' ';
        if (i + j == prefix) sep = ':';
        if (!ok) sep = '*';
        std::cout << sep << std::string(p);
      }
      std::cout << std::endl;
      success = false;
    }
  }
  return success;
}

void sort(std::span<sortable_t> list, size_t prefix = 0);

static sortable_t const* debug_base = nullptr;

void debug_sort(std::span<sortable_t> list) {
  debug_base = &list[0];
  sort(list);
  debug_base = nullptr;
}

void debug_sort(std::span<sortable_t> list, size_t prefix, bool left,
                sortable_t pivot) {
  size_t offset = &list[0] - debug_base;
  std::stringstream header;
  header << (left ? ",left < " : "`right>=") << pivot << ": ";
  header << std::setw(3) << list.size() << " (" << offset;
  if (list.size() == 0) {
    header << ")";
  } else {
    header << "-" << (offset + list.size()) << "), prefix: " << prefix;
    bool ordered = true;
    auto prev = list[0];
    for (size_t i = 0; i < prefix; ++i) {
      bool ok = !(list[i] < prev);
      header << (ok ? ' ' : '*') << list[i];
      ordered &= ok;
      prev = list[i];
    }
    auto min = list[prefix];
    auto max = list[prefix];
    for (size_t i = prefix; i < list.size(); ++i) {
        min = std::min(min, list[i]);
        max = std::max(max, list[i]);
    }
    header << " (" << min << "-" << max << ")";
    std::cout << header.str() << (ordered ? "" : " unsorted prefix")
              << std::endl;
    is_bounded(list, prefix, left, pivot);
    sort(list, prefix);
    is_sorted(list, prefix, header.str());
    is_bounded(list, prefix, left, pivot, header.str());
  }
}

int main(void) {
  static sortable_t big_array[2][10001];
  std::span<sortable_t> dut(big_array[0]);
  std::span<sortable_t> ref(big_array[1]);
  for (int i = 0; i < 1000; ++i) {
    std::uniform_int_distribution d(0, 255);
    for (auto& p : dut) p = sortable_t(d(prng));
    std::copy(dut.begin(), dut.end(), ref.begin());
    sort(dut);
    if (!is_sorted(dut)) {
      printf("check failed.\n");
    }
    std::sort(ref.begin(), ref.end());
    if (!std::equal(ref.begin(), ref.end(), dut.begin())) {
      printf("mismatch.\n");
      return EXIT_FAILURE;
    }
  }
  return 0;
}



bool specialcase(std::span<sortable_t> list, size_t prefix) {
  if (prefix == list.size()) return true;
#if 1
  switch (list.size()) {
    // {{{
  case 0: case 1:
    return true;
  case 2:
    ce(list[0], list[1]);
    return true;
  case 3:
    ce(list[0], list[2]);
    ce(list[0], list[1]);
    ce(list[1], list[2]);
    return true;
  case 4:
    ce(list[0], list[2]);
    ce(list[1], list[3]);
    ce(list[0], list[1]);
    ce(list[2], list[3]);
    ce(list[1], list[2]);
    return true;
  case 5:
    ce(list[0], list[3]);
    ce(list[1], list[4]);
    ce(list[0], list[2]);
    ce(list[1], list[3]);
    ce(list[0], list[1]);
    ce(list[2], list[4]);
    ce(list[1], list[2]);
    ce(list[3], list[4]);
    ce(list[2], list[3]);
    return true;
  case 6:
    ce(list[0], list[5]);
    ce(list[1], list[3]);
    ce(list[2], list[4]);
    ce(list[1], list[2]);
    ce(list[3], list[4]);
    ce(list[0], list[3]);
    ce(list[2], list[5]);
    ce(list[0], list[1]);
    ce(list[2], list[3]);
    ce(list[4], list[5]);
    ce(list[1], list[2]);
    ce(list[3], list[4]);
    return true;
  case 7:
    ce(list[0], list[6]);
    ce(list[2], list[3]);
    ce(list[4], list[5]);
    ce(list[0], list[2]);
    ce(list[1], list[4]);
    ce(list[3], list[6]);
    ce(list[0], list[1]);
    ce(list[2], list[5]);
    ce(list[3], list[4]);
    ce(list[1], list[2]);
    ce(list[4], list[6]);
    ce(list[2], list[3]);
    ce(list[4], list[5]);
    ce(list[1], list[2]);
    ce(list[3], list[4]);
    ce(list[5], list[6]);
    return true;
  case 8:
    ce(list[0], list[2]); ce(list[1], list[3]); ce(list[4], list[6]); ce(list[5], list[7]);
    ce(list[0], list[4]); ce(list[1], list[5]); ce(list[2], list[6]); ce(list[3], list[7]);
    ce(list[0], list[1]); ce(list[2], list[3]); ce(list[4], list[5]); ce(list[6], list[7]);
    ce(list[2], list[4]); ce(list[3], list[5]);
    ce(list[1], list[4]); ce(list[3], list[6]);
    ce(list[1], list[2]); ce(list[3], list[4]); ce(list[5], list[6]);
    return true;
  case 9:
    ce(list[0], list[3]); ce(list[1], list[7]); ce(list[2], list[5]); ce(list[4], list[8]);
    ce(list[0], list[7]); ce(list[2], list[4]); ce(list[3], list[8]); ce(list[5], list[6]);
    ce(list[0], list[2]); ce(list[1], list[3]); ce(list[4], list[5]); ce(list[7], list[8]);
    ce(list[1], list[4]); ce(list[3], list[6]); ce(list[5], list[7]);
    ce(list[0], list[1]); ce(list[2], list[4]); ce(list[3], list[5]); ce(list[6], list[8]);
    ce(list[2], list[3]); ce(list[4], list[5]); ce(list[6], list[7]);
    ce(list[1], list[2]); ce(list[3], list[4]); ce(list[5], list[6]);
    return true;
  case 10:
    ce(list[0], list[1]); ce(list[2], list[5]); ce(list[3], list[6]); ce(list[4], list[7]); ce(list[8], list[9]);
    ce(list[0], list[6]); ce(list[1], list[8]); ce(list[2], list[4]); ce(list[3], list[9]); ce(list[5], list[7]);
    ce(list[0], list[2]); ce(list[1], list[3]); ce(list[4], list[5]); ce(list[6], list[8]); ce(list[7], list[9]);
    ce(list[0], list[1]); ce(list[2], list[7]); ce(list[3], list[5]); ce(list[4], list[6]); ce(list[8], list[9]);
    ce(list[1], list[2]); ce(list[3], list[4]); ce(list[5], list[6]); ce(list[7], list[8]);
    ce(list[1], list[3]); ce(list[2], list[4]); ce(list[5], list[7]); ce(list[6], list[8]);
    ce(list[2], list[3]); ce(list[4], list[5]); ce(list[6], list[7]);
    return true;
  case 11:
    ce(list[0], list[9]); ce(list[1], list[6]); ce(list[2], list[4]); ce(list[3], list[7]); ce(list[5], list[8]);
    ce(list[0], list[1]); ce(list[3], list[5]); ce(list[4], list[10]); ce(list[6], list[9]); ce(list[7], list[8]);
    ce(list[1], list[3]); ce(list[2], list[5]); ce(list[4], list[7]); ce(list[8], list[10]);
    ce(list[0], list[4]); ce(list[1], list[2]); ce(list[3], list[7]); ce(list[5], list[9]); ce(list[6], list[8]);
    ce(list[0], list[1]); ce(list[2], list[6]); ce(list[4], list[5]); ce(list[7], list[8]); ce(list[9], list[10]);
    ce(list[2], list[4]); ce(list[3], list[6]); ce(list[5], list[7]); ce(list[8], list[9]);
    ce(list[1], list[2]); ce(list[3], list[4]); ce(list[5], list[6]); ce(list[7], list[8]);
    ce(list[2], list[3]); ce(list[4], list[5]); ce(list[6], list[7]);
    return true;
  case 12:
    ce(list[0], list[8]); ce(list[1], list[7]); ce(list[2], list[6]); ce(list[3], list[11]); ce(list[4], list[10]); ce(list[5], list[9]);
    ce(list[0], list[2]); ce(list[1], list[4]); ce(list[3], list[5]); ce(list[6], list[8]); ce(list[7], list[10]); ce(list[9], list[11]);
    ce(list[0], list[1]); ce(list[2], list[9]); ce(list[4], list[7]); ce(list[5], list[6]); ce(list[10], list[11]);
    ce(list[1], list[3]); ce(list[2], list[7]); ce(list[4], list[9]); ce(list[8], list[10]);
    ce(list[0], list[1]); ce(list[2], list[3]); ce(list[4], list[5]); ce(list[6], list[7]); ce(list[8], list[9]); ce(list[10], list[11]);
    ce(list[1], list[2]); ce(list[3], list[5]); ce(list[6], list[8]); ce(list[9], list[10]);
    ce(list[2], list[4]); ce(list[3], list[6]); ce(list[5], list[8]); ce(list[7], list[9]);
    ce(list[1], list[2]); ce(list[3], list[4]); ce(list[5], list[6]); ce(list[7], list[8]); ce(list[9], list[10]);
    return true;
  case 13:
    ce(list[0], list[11]); ce(list[1], list[7]); ce(list[2], list[4]); ce(list[3], list[5]); ce(list[8], list[9]); ce(list[10], list[12]);
    ce(list[0], list[2]); ce(list[3], list[6]); ce(list[4], list[12]); ce(list[5], list[7]); ce(list[8], list[10]);
    ce(list[0], list[8]); ce(list[1], list[3]); ce(list[2], list[5]); ce(list[4], list[9]); ce(list[6], list[11]); ce(list[7], list[12]);
    ce(list[0], list[1]); ce(list[2], list[10]); ce(list[3], list[8]); ce(list[4], list[6]); ce(list[9], list[11]);
    ce(list[1], list[3]); ce(list[2], list[4]); ce(list[5], list[10]); ce(list[6], list[8]); ce(list[7], list[9]); ce(list[11], list[12]);
    ce(list[1], list[2]); ce(list[3], list[4]); ce(list[5], list[8]); ce(list[6], list[9]); ce(list[7], list[10]);
    ce(list[2], list[3]); ce(list[4], list[7]); ce(list[5], list[6]); ce(list[8], list[11]); ce(list[9], list[10]);
    ce(list[4], list[5]); ce(list[6], list[7]); ce(list[8], list[9]); ce(list[10], list[11]);
    ce(list[3], list[4]); ce(list[5], list[6]); ce(list[7], list[8]); ce(list[9], list[10]);
    return true;
  case 14:
    ce(list[0], list[1]); ce(list[2], list[3]); ce(list[4], list[5]); ce(list[6], list[7]); ce(list[8], list[9]); ce(list[10], list[11]); ce(list[12], list[13]);
    ce(list[0], list[2]); ce(list[1], list[3]); ce(list[4], list[8]); ce(list[5], list[9]); ce(list[10], list[12]); ce(list[11], list[13]);
    ce(list[0], list[10]); ce(list[1], list[6]); ce(list[2], list[11]); ce(list[3], list[13]); ce(list[5], list[8]); ce(list[7], list[12]);
    ce(list[1], list[4]); ce(list[2], list[8]); ce(list[3], list[6]); ce(list[5], list[11]); ce(list[7], list[10]); ce(list[9], list[12]);
    ce(list[0], list[1]); ce(list[3], list[9]); ce(list[4], list[10]); ce(list[5], list[7]); ce(list[6], list[8]); ce(list[12], list[13]);
    ce(list[1], list[5]); ce(list[2], list[4]); ce(list[3], list[7]); ce(list[6], list[10]); ce(list[8], list[12]); ce(list[9], list[11]);
    ce(list[1], list[2]); ce(list[3], list[5]); ce(list[4], list[6]); ce(list[7], list[9]); ce(list[8], list[10]); ce(list[11], list[12]);
    ce(list[2], list[3]); ce(list[4], list[5]); ce(list[6], list[7]); ce(list[8], list[9]); ce(list[10], list[11]);
    ce(list[3], list[4]); ce(list[5], list[6]); ce(list[7], list[8]); ce(list[9], list[10]);
    return true;

  case 15:
    ce(list[0], list[6]); ce(list[1], list[10]); ce(list[2], list[14]); ce(list[3], list[9]); ce(list[4], list[12]); ce(list[5], list[13]); ce(list[7], list[11]);
    ce(list[0], list[7]); ce(list[2], list[5]); ce(list[3], list[4]); ce(list[6], list[11]); ce(list[8], list[10]); ce(list[9], list[12]); ce(list[13], list[14]);
    ce(list[1], list[13]); ce(list[2], list[3]); ce(list[4], list[6]); ce(list[5], list[9]); ce(list[7], list[8]); ce(list[10], list[14]); ce(list[11], list[12]);
    ce(list[0], list[3]); ce(list[1], list[4]); ce(list[5], list[7]); ce(list[6], list[13]); ce(list[8], list[9]); ce(list[10], list[11]); ce(list[12], list[14]);
    ce(list[0], list[2]); ce(list[1], list[5]); ce(list[3], list[8]); ce(list[4], list[6]); ce(list[7], list[10]); ce(list[9], list[11]); ce(list[12], list[13]);
    ce(list[0], list[1]); ce(list[2], list[5]); ce(list[3], list[10]); ce(list[4], list[8]); ce(list[6], list[7]); ce(list[9], list[12]); ce(list[11], list[13]);
    ce(list[1], list[2]); ce(list[3], list[4]); ce(list[5], list[6]); ce(list[7], list[9]); ce(list[8], list[10]); ce(list[11], list[12]);
    ce(list[3], list[5]); ce(list[4], list[6]); ce(list[7], list[8]); ce(list[9], list[10]);
    ce(list[2], list[3]); ce(list[4], list[5]); ce(list[6], list[7]); ce(list[8], list[9]); ce(list[10], list[11]);
    return true;
    // }}}
  }
#else
  if (list.size() < 16) {
    std::sort(list.begin(), list.end());
    return true;
  }
#endif
  return false;
}

void sort(std::span<sortable_t> list, size_t prefix) {
  if (specialcase(list, prefix)) return;

  if (prefix < min_prefix) {
    size_t new_prefix = std::min(init_prefix, list.size());
    for (size_t i = prefix; i < new_prefix; ++i) {
      size_t j = randint(i, list.size());
      std::swap(list[i], list[j]);
    }
    if (specialcase(list.first(new_prefix), prefix) == false) {
      // To recurse here would be a bit weird.  Let's not think about it.
      assert(!"no special-case solution for prefix sort");
    }
    prefix = new_prefix;
  }

  auto unsorted = list.begin() + prefix;
  auto lo = unsorted;
  auto hi = list.end();

  // Pick central pivot, then rewind to first instance of that value.
  size_t left_prefix = prefix / 2;
  while (left_prefix > 0 && !(list[left_prefix - 1] < list[left_prefix]))
    --left_prefix;

  // partition
  {
    auto& pivot = list[left_prefix];
    for (;;) {
      while (lo[0] < pivot && lo < hi)
        ++lo;
      while (!(hi[-1] < pivot) && lo < hi)
        --hi;
      if (!(lo < hi)) break;
      std::swap(lo[0], hi[-1]);
    }
  }

  // move half of prefix up to right partition
  size_t right_prefix = prefix - left_prefix;
  for (size_t i = 0; i < right_prefix; ++i) {
    std::swap(*--lo, *--unsorted);
  }

  auto& pivot = *lo;
  hi = lo + 1;

  // cull all values equal to pivot from right partition
  while (hi < list.end() && !(pivot < *hi))
    ++hi;

  if (hi < lo + right_prefix) {
    right_prefix = (lo + right_prefix) - hi;
  } else {
    // Whole prefix was equal to pivot; good chance most of the array is.
    // TODO: Do swaps to squeeze out everything else equal to pivot.
    right_prefix = 0;
  }

  auto left = list.first(lo - list.begin());
  auto right = list.subspan(hi - list.begin());

  if (debug_base) {
    debug_sort(left, left_prefix, true, pivot);
    debug_sort(right, right_prefix, false, pivot);
  } else {
    sort(left, left_prefix);
    sort(right, right_prefix);
  }
}
