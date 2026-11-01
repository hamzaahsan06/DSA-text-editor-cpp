#ifndef DS_SINGLY_LINKED_LIST_H
#define DS_SINGLY_LINKED_LIST_H

// Generic singly linked list.
template <typename T>
class SinglyLinkedList {
public:
    struct Node {
        T data;
        Node* next;
        Node(const T& d, Node* n) : data(d), next(n) {}
    };

private:
    Node* head;
    int count;

public:
    SinglyLinkedList() : head(nullptr), count(0) {}

    // Deep copy.
    SinglyLinkedList(const SinglyLinkedList& other) : head(nullptr), count(0) {
        Node* tail = nullptr;
        for (Node* cur = other.head; cur != nullptr; cur = cur->next) {
            Node* n = new Node(cur->data, nullptr);
            if (tail) { tail->next = n; tail = n; }
            else { head = n; tail = n; }
        }
        count = other.count;
    }

    // Deep copy assign.
    SinglyLinkedList& operator=(const SinglyLinkedList& other) {
        if (this == &other) return *this;
        clear();
        Node* tail = nullptr;
        for (Node* cur = other.head; cur != nullptr; cur = cur->next) {
            Node* n = new Node(cur->data, nullptr);
            if (tail) { tail->next = n; tail = n; }
            else { head = n; tail = n; }
        }
        count = other.count;
        return *this;
    }

    ~SinglyLinkedList() { clear(); }

    // Insert at head.
    bool insertAtHead(const T& value) {
        Node* n = new Node(value, head);
        head = n;
        ++count;
        return true;
    }

    // Insert at tail.
    bool insertAtTail(const T& value) {
        Node* n = new Node(value, nullptr);
        if (head == nullptr) { head = n; ++count; return true; }
        Node* cur = head;
        while (cur->next != nullptr) cur = cur->next;
        cur->next = n;
        ++count;
        return true;
    }

    // Insert before index; valid range [0, size].
    bool insertAtPosition(int index, const T& value) {
        if (index < 0 || index > count) return false;
        if (index == 0) return insertAtHead(value);
        Node* prev = head;
        for (int i = 0; i < index - 1; ++i) prev = prev->next;
        Node* n = new Node(value, prev->next);
        prev->next = n;
        ++count;
        return true;
    }

    // Remove head into out-param.
    bool removeHead(T& out) {
        if (head == nullptr) return false;
        Node* n = head;
        out = n->data;
        head = n->next;
        delete n;
        --count;
        return true;
    }

    // Remove tail into out-param.
    bool removeTail(T& out) {
        if (head == nullptr) return false;
        if (head->next == nullptr) {
            out = head->data;
            delete head;
            head = nullptr;
            --count;
            return true;
        }
        Node* prev = head;
        while (prev->next->next != nullptr) prev = prev->next;
        Node* n = prev->next;
        out = n->data;
        prev->next = nullptr;
        delete n;
        --count;
        return true;
    }

    // Remove at index into out-param.
    bool removeAtPosition(int index, T& out) {
        if (index < 0 || index >= count) return false;
        if (index == 0) return removeHead(out);
        Node* prev = head;
        for (int i = 0; i < index - 1; ++i) prev = prev->next;
        Node* n = prev->next;
        out = n->data;
        prev->next = n->next;
        delete n;
        --count;
        return true;
    }

    // Copy value at index into out-param.
    bool get(int index, T& out) const {
        if (index < 0 || index >= count) return false;
        Node* cur = head;
        for (int i = 0; i < index; ++i) cur = cur->next;
        out = cur->data;
        return true;
    }

    // Overwrite value at index.
    bool set(int index, const T& value) {
        if (index < 0 || index >= count) return false;
        Node* cur = head;
        for (int i = 0; i < index; ++i) cur = cur->next;
        cur->data = value;
        return true;
    }

    // Pointer to head node (may be nullptr).
    Node* getHead() const { return head; }

    int size() const { return count; }

    bool isEmpty() const { return count == 0; }

    void clear() {
        while (head != nullptr) {
            Node* n = head;
            head = head->next;
            delete n;
        }
        count = 0;
    }
};

#endif // DS_SINGLY_LINKED_LIST_H