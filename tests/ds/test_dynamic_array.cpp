// g++ -std=c++17 -Wall -Wextra -Iinclude -Iinclude/editor tests/ds/test_dynamic_array.cpp -o build/test_dynamic_array ; if ($?) { ./build/test_dynamic_array }

#include <iostream>
#include "check.h"
#include "../../include/ds/DynamicArray.h"

// Expected: A new array is empty with zero capacity.
void test_empty_array_initialization()
{
    DynamicArray<int> a;
    CHECK(a.isEmpty() == true);
    CHECK(a.size() == 0);
    CHECK(a.capacity() == 0);
}

// Expected: pushBack grows capacity automatically.
void test_push_back_growth()
{
    DynamicArray<int> a;
    CHECK(a.pushBack(1) == true);
    CHECK(a.size() == 1);
    CHECK(a.capacity() > 0);

    int initialCap = a.capacity();
    for (int i = 2; i <= 100; ++i)
    {
        a.pushBack(i);
    }
    CHECK(a.size() == 100);
    CHECK(a.capacity() > initialCap);
}

// Expected: insertAt works at head, middle, and tail.
void test_insert_at()
{
    DynamicArray<int> a;
    a.pushBack(1);
    a.pushBack(3);

    CHECK(a.insertAt(1, 2) == true); // Middle
    CHECK(a.insertAt(0, 0) == true); // Head
    CHECK(a.insertAt(4, 4) == true); // Tail

    CHECK(a.size() == 5);
    int val;
    CHECK(a.get(0, val) && val == 0);
    CHECK(a.get(4, val) && val == 4);
}

// Expected: Invalid indices fail safely.
void test_boundary_invalid_indices()
{
    DynamicArray<int> a;
    int val;

    CHECK(a.insertAt(-1, 9) == false);
    CHECK(a.insertAt(1, 9) == false);
    CHECK(a.get(0, val) == false);
    CHECK(a.removeAt(0, val) == false);

    a.pushBack(1);
    CHECK(a.get(-1, val) == false);
    CHECK(a.get(1, val) == false);
    CHECK(a.set(1, 9) == false);
}

// Expected: removeAt and popBack shift elements correctly.
void test_remove_elements()
{
    DynamicArray<int> a;
    a.pushBack(10);
    a.pushBack(20);
    a.pushBack(30);

    int val;
    CHECK(a.removeAt(1, val) == true && val == 20);
    CHECK(a.get(1, val) == true && val == 30);

    CHECK(a.popBack(val) == true && val == 30);
    CHECK(a.size() == 1);
}

// Expected: operator[] bounds checking returns default without crashing.
void test_boundary_operator_bounds()
{
    DynamicArray<int> a;
    a.pushBack(5);

    CHECK(a[0] == 5);
    a[0] = 10;
    CHECK(a[0] == 10);

    // Must not crash
    a[-5] = 0;
    (void)a[100];
    CHECK(a.size() == 1);
}

// Expected: Copy operations perform deep copies.
void test_copy_operations()
{
    DynamicArray<int> a;
    a.pushBack(1);

    DynamicArray<int> copy(a);
    copy.pushBack(2);
    CHECK(a.size() == 1);
    CHECK(copy.size() == 2);
}

int main()
{
    test_empty_array_initialization();
    test_push_back_growth();
    test_insert_at();
    test_boundary_invalid_indices();
    test_remove_elements();
    test_boundary_operator_bounds();
    test_copy_operations();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}