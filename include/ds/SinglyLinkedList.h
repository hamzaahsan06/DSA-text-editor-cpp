#ifndef DS_SINGLY_LINKED_LIST_H
#define DS_SINGLY_LINKED_LIST_H

// Generic singly linked list.
// Nodes are singly linked (forward only), so operations that need the
// predecessor (insert/remove at an index, remove tail) traverse O(index).
template <typename T>
class SinglyLinkedList
{
public:
    // Node stores data and a pointer to the next node.
    struct Node
    {
        T data;
        Node *next;
        Node(const T &d, Node *n) : data(d), next(n) {}
    };

private:
    Node *head; // First node. nullptr when the list is empty.
    int count;  // Number of nodes currently stored.

public:
    // Constructs an empty list.
    SinglyLinkedList() : head(nullptr), count(0) {}

    // Copy constructor: deep copy, appending nodes in order.
    // tail tracks the last node built so appends stay O(1) per node.
    SinglyLinkedList(const SinglyLinkedList &other) : head(nullptr), count(0)
    {
        Node *tail = nullptr;
        for (Node *cur = other.head; cur != nullptr; cur = cur->next)
        {
            Node *n = new Node(cur->data, nullptr);
            if (tail)
            {
                tail->next = n;
                tail = n;
            }
            else
            {
                head = n;
                tail = n;
            } // First node becomes head.
        }
        count = other.count;
    }

    // Copy assignment: self assignment safe, clears existing nodes.
    // then deep-copies from other.
    SinglyLinkedList &operator=(const SinglyLinkedList &other)
    {
        if (this == &other)
            return *this;
        clear();
        Node *tail = nullptr;
        for (Node *cur = other.head; cur != nullptr; cur = cur->next)
        {
            Node *n = new Node(cur->data, nullptr);
            if (tail)
            {
                tail->next = n;
                tail = n;
            }
            else
            {
                head = n;
                tail = n;
            } // First node becomes head.
        }
        count = other.count;
        return *this;
    }

    // Destructor releases all nodes.
    ~SinglyLinkedList() { clear(); }

    // Inserts value at the front. Always succeeds.
    bool insertAtHead(const T &value)
    {
        Node *n = new Node(value, head);
        head = n;
        ++count;
        return true;
    }

    // Inserts value at the back. Always succeeds.
    // Traverses to the last node because there is no tail pointer.
    bool insertAtTail(const T &value)
    {
        Node *n = new Node(value, nullptr);
        if (head == nullptr)
        {
            head = n;
            ++count;
            return true;
        }
        Node *cur = head;
        while (cur->next != nullptr)
            cur = cur->next;
        cur->next = n;
        ++count;
        return true;
    }

    // Inserts value before index. Valid range is [0, size];
    // index == size appends. Returns false if out of range.
    // Walks to the predecessor (index - 1) to splice in the new node.
    bool insertAtPosition(int index, const T &value)
    {
        if (index < 0 || index > count)
            return false;
        if (index == 0)
            return insertAtHead(value);
        Node *prev = head;
        for (int i = 0; i < index - 1; ++i)
            prev = prev->next;
        Node *n = new Node(value, prev->next);
        prev->next = n;
        ++count;
        return true;
    }

    // Removes the head node into out.
    // Returns false if the list is empty.
    bool removeHead(T &out)
    {
        if (head == nullptr)
            return false;
        Node *n = head;
        out = n->data;
        head = n->next;
        delete n;
        --count;
        return true;
    }

    // Removes the tail node into out.
    // Returns false if the list is empty. Handles the single-node case
    // separately, then walks to the second-to-last node.
    bool removeTail(T &out)
    {
        if (head == nullptr)
            return false;
        if (head->next == nullptr)
        {
            out = head->data;
            delete head;
            head = nullptr;
            --count;
            return true;
        }
        Node *prev = head;
        while (prev->next->next != nullptr)
            prev = prev->next;
        Node *n = prev->next;
        out = n->data;
        prev->next = nullptr;
        delete n;
        --count;
        return true;
    }

    // Removes the node at index into out.
    // Valid range is [0, size-1]; returns false if out of range.
    // Walks to the predecessor and unlinks the target node.
    bool removeAtPosition(int index, T &out)
    {
        if (index < 0 || index >= count)
            return false;
        if (index == 0)
            return removeHead(out);
        Node *prev = head;
        for (int i = 0; i < index - 1; ++i)
            prev = prev->next;
        Node *n = prev->next;
        out = n->data;
        prev->next = n->next;
        delete n;
        --count;
        return true;
    }

    // Copies the value at index into out without modifying the list.
    // Returns false if index is out of range.
    bool get(int index, T &out) const
    {
        if (index < 0 || index >= count)
            return false;
        Node *cur = head;
        for (int i = 0; i < index; ++i)
            cur = cur->next;
        out = cur->data;
        return true;
    }

    // Overwrites the value at index.
    // Returns false if index is out of range.
    bool set(int index, const T &value)
    {
        if (index < 0 || index >= count)
            return false;
        Node *cur = head;
        for (int i = 0; i < index; ++i)
            cur = cur->next;
        cur->data = value;
        return true;
    }

    // Returns the first node (nullptr if empty).
    Node *getHead() const { return head; }

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
        count = 0;
    }
};

#endif // DS_SINGLY_LINKED_LIST_H