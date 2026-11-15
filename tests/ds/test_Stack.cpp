#include <iostream>
#include "check.h"
#include "../../include/ds/Stack.h"

// Expected: A new stack is empty and its size is zero.
void test_empty_stack_initialization()
{
    Stack<int> s;
    CHECK(s.isEmpty() == true);
    CHECK(s.size() == 0);
}

// Expected: Pushing to the stack increases size and updates the top element.
void test_push_updates_top()
{
    Stack<int> s;
    CHECK(s.push(10) == true);
    CHECK(s.size() == 1);
    CHECK(s.isEmpty() == false);

    int val = -1;
    CHECK(s.peek(val) == true);
    CHECK(val == 10);
}

// Expected: Popping returns the LIFO element and decreases size.
void test_pop_returns_lifo_elements()
{
    Stack<int> s;
    s.push(1);
    s.push(2);
    s.push(3);

    int val = -1;
    CHECK(s.pop(val) == true && val == 3);
    CHECK(s.pop(val) == true && val == 2);
    CHECK(s.pop(val) == true && val == 1);
    CHECK(s.pop(val) == false); // Empty now
}

// Expected: Popping or peeking an empty stack fails safely without crashing.
void test_boundary_empty_stack_operations()
{
    Stack<int> s;
    int val = -1;
    CHECK(s.pop(val) == false);
    CHECK(s.peek(val) == false);
}

// Expected: isFull correctly respects the capacity limit.
void test_stack_capacity_limit()
{
    Stack<int> s(2);
    CHECK(s.isFull() == false);
    CHECK(s.push(1) == true);
    CHECK(s.isFull() == false);
    CHECK(s.push(2) == true);
    CHECK(s.isFull() == true);

    // Invalid input: pushing to a full stack
    CHECK(s.push(3) == false);
    CHECK(s.size() == 2);
}

// Expected: Copy constructor creates a deep copy of the stack.
void test_copy_constructor_deep_copy()
{
    Stack<int> orig;
    orig.push(1);
    orig.push(2);

    Stack<int> copy(orig);
    CHECK(copy.size() == 2);

    // Modify copy, original should stay intact
    int val;
    copy.pop(val);
    CHECK(copy.size() == 1);
    CHECK(orig.size() == 2);
}

// Expected: Copy assignment creates a deep copy of the stack.
void test_copy_assignment_deep_copy()
{
    Stack<int> orig;
    orig.push(10);

    Stack<int> assigned;
    assigned.push(99);
    assigned = orig;

    CHECK(assigned.size() == 1);
    int val;
    CHECK(assigned.pop(val) == true && val == 10);
}

// Expected: Clear removes all elements from the stack.
void test_clear_empties_stack()
{
    Stack<int> s;
    s.push(1);
    s.push(2);
    s.clear();
    CHECK(s.isEmpty() == true);
    CHECK(s.size() == 0);
}

int main()
{
    test_empty_stack_initialization();
    test_push_updates_top();
    test_pop_returns_lifo_elements();
    test_boundary_empty_stack_operations();
    test_stack_capacity_limit();
    test_copy_constructor_deep_copy();
    test_copy_assignment_deep_copy();
    test_clear_empties_stack();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}