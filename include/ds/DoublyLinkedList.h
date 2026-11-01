#ifndef DS_DOUBLY_LINKED_LIST_H
#define DS_DOUBLY_LINKED_LIST_H

// Generic doubly linked list; index traversal from nearest end.
template <typename T>
class DoublyLinkedList {
public:
    struct Node {
        T data;
        Node* next;
        Node* prev;
        Node(const T& d, Node* n, Node* p) : data(d), next(n), prev(p) {}
    };

private:
    Node* head;
    Node* tail;
    int count;

    // Find node at index, from nearest end. Returns nullptr if bad index.
    Node* nodeAt(int index) const {
        if (index < 0 || index >= count) return nullptr;
        if (index < count / 2) {
            Node* cur = head;
            for (int i = 0; i < index; ++i) cur = cur->next;
            return cur;
        } else {
            Node* cur = tail;
            for (int i = count - 1; i > index; --i) cur = cur->prev;
            return cur;
        }
    }

public:
    DoublyLinkedList() : head(nullptr), tail(nullptr), count(0) {}

    // Deep copy.
    DoublyLinkedList(const DoublyLinkedList& other)
        : head(nullptr), tail(nullptr), count(0) {
        for (Node* cur = other.head; cur != nullptr; cur = cur->next) {
            Node* n = new Node(cur->data, nullptr, tail);
            if (tail) tail->next = n;
            else head = n;
            tail = n;
        }
        count = other.count;
    }

    // Deep copy assign.
    DoublyLinkedList& operator=(const DoublyLinkedList& other) {
        if (this == &other) return *this;
        clear();
        for (Node* cur = other.head; cur != nullptr; cur = cur->next) {
            Node* n = new Node(cur->data, nullptr, tail);
            if (tail) tail->next = n;
            else head = n;
            tail = n;
        }
        count = other.count;
        return *this;
    }

    ~DoublyLinkedList() { clear(); }

    // Insert at head.
    bool insertAtHead(const T& value) {
        Node* n = new Node(value, head, nullptr);
        if (head) head->prev = n;
        else tail = n;
        head = n;
        ++count;
        return true;
    }

    // Insert at tail.
    bool insertAtTail(const T& value) {
        Node* n = new Node(value, nullptr, tail);
        if (tail) tail->next = n;
        else head = n;
        tail = n;
        ++count;
        return true;
    }

    // Insert before index; valid range [0, size].
    bool insertAtPosition(int index, const T& value) {
        if (index < 0 || index > count) return false;
        if (index == 0) return insertAtHead(value);
        if (index == count) return insertAtTail(value);
        Node* cur = nodeAt(index);
        Node* n = new Node(value, cur, cur->prev);
        cur->prev->next = n;
        cur->prev = n;
        ++count;
        return true;
    }

    // Remove head into out-param.
    bool removeHead(T& out) {
        if (head == nullptr) return false;
        Node* n = head;
        out = n->data;
        head = n->next;
        if (head) head->prev = nullptr;
        else tail = nullptr;
        delete n;
        --count;
        return true;
    }

    // Remove tail into out-param.
    bool removeTail(T& out) {
        if (tail == nullptr) return false;
        Node* n = tail;
        out = n->data;
        tail = n->prev;
        if (tail) tail->next = nullptr;
        else head = nullptr;
        delete n;
        --count;
        return true;
    }

    // Remove at index into out-param.
    bool removeAtPosition(int index, T& out) {
        if (index < 0 || index >= count) return false;
        if (index == 0) return removeHead(out);
        if (index == count - 1) return removeTail(out);
        Node* cur = nodeAt(index);
        out = cur->data;
        cur->prev->next = cur->next;
        cur->next->prev = cur->prev;
        delete cur;
        --count;
        return true;
    }

    // Copy value at index into out-param.
    bool get(int index, T& out) const {
        Node* cur = nodeAt(index);
        if (cur == nullptr) return false;
        out = cur->data;
        return true;
    }

    // Overwrite value at index.
    bool set(int index, const T& value) {
        Node* cur = nodeAt(index);
        if (cur == nullptr) return false;
        cur->data = value;
        return true;
    }

    Node* getHead() const { return head; }

    Node* getTail() const { return tail; }

    int size() const { return count; }

    bool isEmpty() const { return count == 0; }

    void clear() {
        while (head != nullptr) {
            Node* n = head;
            head = head->next;
            delete n;
        }
        tail = nullptr;
        count = 0;
    }
};

#endif // DS_DOUBLY_LINKED_LIST_H