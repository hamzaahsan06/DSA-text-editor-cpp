#ifndef DS_DOUBLY_LINKED_LIST_H
#define DS_DOUBLY_LINKED_LIST_H

// Generic doubly linked list.
// Index-based operations traverse from the nearest end (head or tail)
// to keep access O(n/2) in the worst case.
template <typename T>
class DoublyLinkedList
{
public:
    // Node stores data plus links to both neighbors.
    // Constructor links the node between n (next) and p (prev).
    struct Node
    {
        T data;
        Node *next;
        Node *prev;
        Node(const T &d, Node *n, Node *p) : data(d), next(n), prev(p) {}
    };

private:
    Node *head; // First node; nullptr when the list is empty.
    Node *tail; // Last node; nullptr when the list is empty.
    int count;  // Number of nodes currently stored.

    // Returns the node at index, or nullptr if index is out of range.
    // Chooses the closer end to minimize traversal steps.
    Node *nodeAt(int index) const
    {
        if (index < 0 || index >= count)
            return nullptr;
        // Traverse forward from head for the first half.
        if (index < count / 2)
        {
            Node *cur = head;
            for (int i = 0; i < index; ++i)
                cur = cur->next;
            return cur;
        }
        // Traverse backward from tail for the second half.
        else
        {
            Node *cur = tail;
            for (int i = count - 1; i > index; --i)
                cur = cur->prev;
            return cur;
        }
    }

public:
    // Constructs an empty list.
    DoublyLinkedList() : head(nullptr), tail(nullptr), count(0) {}

    // Copy constructor: performs a deep copy of other,
    // appending nodes in order so the new list is independent.
    DoublyLinkedList(const DoublyLinkedList &other)
        : head(nullptr), tail(nullptr), count(0)
    {
        for (Node *cur = other.head; cur != nullptr; cur = cur->next)
        {
            Node *n = new Node(cur->data, nullptr, tail);
            if (tail)
                tail->next = n;
            else
                head = n; // First node becomes head.
            tail = n;
        }
        count = other.count;
    }

    // Copy-assignment: self-assignment safe; clears existing nodes
    // before deep-copying from other.
    DoublyLinkedList &operator=(const DoublyLinkedList &other)
    {
        if (this == &other)
            return *this;
        clear();
        for (Node *cur = other.head; cur != nullptr; cur = cur->next)
        {
            Node *n = new Node(cur->data, nullptr, tail);
            if (tail)
                tail->next = n;
            else
                head = n; // First node becomes head.
            tail = n;
        }
        count = other.count;
        return *this;
    }

    // Destructor releases all nodes.
    ~DoublyLinkedList() { clear(); }

    // Inserts value at the front. Always succeeds.
    bool insertAtHead(const T &value)
    {
        Node *n = new Node(value, head, nullptr);
        if (head)
            head->prev = n;
        else
            tail = n; // List was empty; new node is also the tail.
        head = n;
        ++count;
        return true;
    }

    // Inserts value at the back. Always succeeds.
    bool insertAtTail(const T &value)
    {
        Node *n = new Node(value, nullptr, tail);
        if (tail)
            tail->next = n;
        else
            head = n; // List was empty; new node is also the head.
        tail = n;
        ++count;
        return true;
    }

    // Inserts value before index. Valid range is [0, size];
    // index == size appends at the tail. Returns false if out of range.
    bool insertAtPosition(int index, const T &value)
    {
        if (index < 0 || index > count)
            return false;
        if (index == 0)
            return insertAtHead(value);
        if (index == count)
            return insertAtTail(value);
        // Insert before the node currently occupying index.
        Node *cur = nodeAt(index);
        Node *n = new Node(value, cur, cur->prev);
        cur->prev->next = n;
        cur->prev = n;
        ++count;
        return true;
    }

    // Removes the head node, storing its value in out.
    // Returns false if the list is empty.
    bool removeHead(T &out)
    {
        if (head == nullptr)
            return false;
        Node *n = head;
        out = n->data;
        head = n->next;
        if (head)
            head->prev = nullptr;
        else
            tail = nullptr; // List became empty.
        delete n;
        --count;
        return true;
    }

    // Removes the tail node, storing its value in out.
    // Returns false if the list is empty.
    bool removeTail(T &out)
    {
        if (tail == nullptr)
            return false;
        Node *n = tail;
        out = n->data;
        tail = n->prev;
        if (tail)
            tail->next = nullptr;
        else
            head = nullptr; // List became empty.
        delete n;
        --count;
        return true;
    }

    // Removes the node at index, storing its value in out.
    // Valid range is [0, size-1]; returns false if out of range.
    bool removeAtPosition(int index, T &out)
    {
        if (index < 0 || index >= count)
            return false;
        if (index == 0)
            return removeHead(out);
        if (index == count - 1)
            return removeTail(out);
        // Unlink an interior node from both of its neighbors.
        Node *cur = nodeAt(index);
        out = cur->data;
        cur->prev->next = cur->next;
        cur->next->prev = cur->prev;
        delete cur;
        --count;
        return true;
    }

    // Copies the value at index into out without modifying the list.
    // Returns false if index is out of range.
    bool get(int index, T &out) const
    {
        Node *cur = nodeAt(index);
        if (cur == nullptr)
            return false;
        out = cur->data;
        return true;
    }

    // Overwrites the value at index with value.
    // Returns false if index is out of range.
    bool set(int index, const T &value)
    {
        Node *cur = nodeAt(index);
        if (cur == nullptr)
            return false;
        cur->data = value;
        return true;
    }

    // Returns the first node (nullptr if empty).
    Node *getHead() const { return head; }

    // Returns the last node (nullptr if empty).
    Node *getTail() const { return tail; }

    // Returns the number of nodes currently stored.
    int size() const { return count; }

    // Returns true if the list contains no nodes.
    bool isEmpty() const { return count == 0; }

    // Removes and deletes all nodes, resetting the list to empty.
    void clear()
    {
        while (head != nullptr)
        {
            Node *n = head;
            head = head->next;
            delete n;
        }
        tail = nullptr;
        count = 0;
    }
};

#endif // DS_DOUBLY_LINKED_LIST_H