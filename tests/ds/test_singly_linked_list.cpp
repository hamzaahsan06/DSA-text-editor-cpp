#include <iostream>
#include "check.h"
#include "../../include/ds/SinglyLinkedList.h"

// Expected: A new list is empty and its size is zero.
void test_empty_list_initialization() {
    SinglyLinkedList<int> l;
    CHECK(l.isEmpty() == true);
    CHECK(l.size() == 0);
    CHECK(l.getHead() == nullptr);
}

// Expected: Inserting at head puts the element at the front.
void test_insert_at_head() {
    SinglyLinkedList<int> l;
    CHECK(l.insertAtHead(10) == true);
    CHECK(l.insertAtHead(20) == true);
    CHECK(l.size() == 2);
    
    int val;
    CHECK(l.get(0, val) == true && val == 20);
    CHECK(l.get(1, val) == true && val == 10);
}

// Expected: Inserting at tail puts the element at the back.
void test_insert_at_tail() {
    SinglyLinkedList<int> l;
    CHECK(l.insertAtTail(10) == true);
    CHECK(l.insertAtTail(20) == true);
    
    int val;
    CHECK(l.get(0, val) == true && val == 10);
    CHECK(l.get(1, val) == true && val == 20);
}

// Expected: Inserting at position works at head, middle, and tail.
void test_insert_at_position() {
    SinglyLinkedList<int> l;
    l.insertAtTail(1);
    l.insertAtTail(3);
    
    // Middle
    CHECK(l.insertAtPosition(1, 2) == true);
    int val;
    CHECK(l.get(1, val) == true && val == 2);
    
    // Head
    CHECK(l.insertAtPosition(0, 0) == true);
    CHECK(l.get(0, val) == true && val == 0);
    
    // Tail
    CHECK(l.insertAtPosition(4, 4) == true);
    CHECK(l.get(4, val) == true && val == 4);
}

// Expected: Invalid positions fail safely without crashing.
void test_boundary_invalid_positions() {
    SinglyLinkedList<int> l;
    int val;
    
    // Empty list boundary
    CHECK(l.get(0, val) == false);
    CHECK(l.set(0, 1) == false);
    
    l.insertAtTail(1); // size 1
    
    // Out of bounds
    CHECK(l.insertAtPosition(-1, 99) == false);
    CHECK(l.insertAtPosition(2, 99) == false);
    CHECK(l.get(-1, val) == false);
    CHECK(l.get(1, val) == false);
    CHECK(l.set(-1, 99) == false);
    CHECK(l.set(1, 99) == false);
}

// Expected: Removing at head, tail, and position updates the list correctly.
void test_remove_elements() {
    SinglyLinkedList<int> l;
    l.insertAtTail(1);
    l.insertAtTail(2);
    l.insertAtTail(3);
    
    int val;
    // Remove Middle
    CHECK(l.removeAtPosition(1, val) == true && val == 2);
    CHECK(l.size() == 2);
    
    // Remove Head
    CHECK(l.removeHead(val) == true && val == 1);
    
    // Remove Tail
    CHECK(l.removeTail(val) == true && val == 3);
    CHECK(l.isEmpty() == true);
}

// Expected: Removing from an empty list fails safely.
void test_boundary_remove_from_empty() {
    SinglyLinkedList<int> l;
    int val;
    CHECK(l.removeHead(val) == false);
    CHECK(l.removeTail(val) == false);
    CHECK(l.removeAtPosition(0, val) == false);
}

// Expected: get and set access values correctly.
void test_get_and_set() {
    SinglyLinkedList<int> l;
    l.insertAtTail(10);
    int val;
    CHECK(l.get(0, val) == true && val == 10);
    CHECK(l.set(0, 20) == true);
    CHECK(l.get(0, val) == true && val == 20);
}

// Expected: Copy constructor and assignment perform deep copies.
void test_copy_operations() {
    SinglyLinkedList<int> l;
    l.insertAtTail(1);
    
    SinglyLinkedList<int> copy(l);
    CHECK(copy.size() == 1);
    copy.insertAtTail(2);
    CHECK(l.size() == 1); // Original untouched
    
    SinglyLinkedList<int> assigned;
    assigned = l;
    CHECK(assigned.size() == 1);
}

// Expected: clear() removes all elements.
void test_clear() {
    SinglyLinkedList<int> l;
    l.insertAtTail(1);
    l.clear();
    CHECK(l.isEmpty() == true);
    CHECK(l.getHead() == nullptr);
}

int main() {
    test_empty_list_initialization();
    test_insert_at_head();
    test_insert_at_tail();
    test_insert_at_position();
    test_boundary_invalid_positions();
    test_remove_elements();
    test_boundary_remove_from_empty();
    test_get_and_set();
    test_copy_operations();
    test_clear();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}