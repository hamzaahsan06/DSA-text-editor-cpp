// g++ -std=c++17 -Wall -Wextra -Iinclude -Iinclude/editor tests/ds/test_doubly_linked_list.cpp -o build/test_doubly_linked_list ; if ($?) { ./build/test_doubly_linked_list }#include <iostream>

#include "check.h"
#include "../../include/ds/DoublyLinkedList.h"

// Expected: A new list is empty.
void test_empty_list_initialization()
{
    DoublyLinkedList<int> l;
    CHECK(l.isEmpty() == true);
    CHECK(l.size() == 0);
    CHECK(l.getHead() == nullptr);
    CHECK(l.getTail() == nullptr);
}

// Expected: Inserting updates head and tail pointers correctly.
void test_insert_head_and_tail()
{
    DoublyLinkedList<int> l;
    CHECK(l.insertAtHead(10) == true);
    CHECK(l.getHead() == l.getTail());

    CHECK(l.insertAtTail(20) == true);
    CHECK(l.getHead()->data == 10);
    CHECK(l.getTail()->data == 20);
    CHECK(l.getHead()->next == l.getTail());
    CHECK(l.getTail()->prev == l.getHead());
}

// Expected: Inserting at a position correctly links prev and next.
void test_insert_at_position()
{
    DoublyLinkedList<int> l;
    l.insertAtTail(1);
    l.insertAtTail(3);
    CHECK(l.insertAtPosition(1, 2) == true);

    int val;
    CHECK(l.get(1, val) == true && val == 2);
    CHECK(l.getHead()->next->data == 2);
    CHECK(l.getTail()->prev->data == 2);
}

// Expected: Invalid positions fail safely.
void test_boundary_invalid_positions()
{
    DoublyLinkedList<int> l;
    int val;
    CHECK(l.get(0, val) == false);

    l.insertAtTail(1);
    CHECK(l.insertAtPosition(-1, 9) == false);
    CHECK(l.insertAtPosition(2, 9) == false);
    CHECK(l.get(1, val) == false);
}

// Expected: Removing from head, tail, and middle works correctly.
void test_remove_elements()
{
    DoublyLinkedList<int> l;
    l.insertAtTail(1);
    l.insertAtTail(2);
    l.insertAtTail(3);

    int val;
    CHECK(l.removeAtPosition(1, val) == true && val == 2);
    CHECK(l.getHead()->data == 1);
    CHECK(l.getTail()->data == 3);

    CHECK(l.removeHead(val) == true && val == 1);
    CHECK(l.removeTail(val) == true && val == 3);
    CHECK(l.isEmpty() == true);
}

// Expected: Copy operations perform deep copies maintaining links.
void test_copy_operations()
{
    DoublyLinkedList<int> l;
    l.insertAtTail(1);
    l.insertAtTail(2);

    DoublyLinkedList<int> copy(l);
    CHECK(copy.size() == 2);
    CHECK(copy.getHead()->next->prev == copy.getHead());

    int val;
    copy.removeTail(val); // Dummy op
    CHECK(l.size() == 2);
}

int main() {
    test_empty_list_initialization();
    test_insert_head_and_tail();
    test_insert_at_position();
    test_boundary_invalid_positions();
    test_remove_elements();
    test_copy_operations();

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}