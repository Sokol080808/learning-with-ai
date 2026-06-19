// Эталонный ответ (answer key). Не поставляется учащимся.
#include "task02.hpp"

int factorial(int n) {
    int result = 1;
    for (int i = 2; i <= n; ++i) result *= i;
    return result;
}

int fib(int n) {
    if (n <= 0) return 0;
    int prev = 0, curr = 1;
    for (int i = 1; i < n; ++i) {
        int next = prev + curr;
        prev = curr;
        curr = next;
    }
    return curr;
}

int gcd(int a, int b) {
    while (b != 0) {
        int r = a % b;
        a = b;
        b = r;
    }
    return a;
}

std::string fizzbuzz(int n) {
    if (n % 15 == 0) return "FizzBuzz";
    if (n % 3 == 0) return "Fizz";
    if (n % 5 == 0) return "Buzz";
    return std::to_string(n);
}

int area(int side) {
    return side * side;
}

int area(int w, int h) {
    return w * h;
}

int lcm(int a, int b) {
    if (a == 0 || b == 0) return 0;
    return (a / gcd(a, b)) * b;
}

std::vector<int> sieve(int n) {
    std::vector<int> result;
    if (n < 2) return result;
    std::vector<bool> is_prime(n + 1, true);
    is_prime[0] = is_prime[1] = false;
    for (int i = 2; i <= n; ++i) {
        if (!is_prime[i]) continue;
        for (long long j = static_cast<long long>(i) * i; j <= n; j += i)
            is_prime[j] = false;
    }
    for (int i = 2; i <= n; ++i)
        if (is_prime[i]) result.push_back(i);
    return result;
}

int binary_search(const std::vector<int>& v, int target) {
    std::size_t lo = 0, hi = v.size();
    while (lo < hi) {
        std::size_t mid = lo + (hi - lo) / 2;
        if (v[mid] == target) return static_cast<int>(mid);
        if (v[mid] < target) lo = mid + 1;
        else hi = mid;
    }
    return -1;
}
