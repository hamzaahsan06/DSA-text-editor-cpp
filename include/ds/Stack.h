#ifndef DS_STACK_H
#define DS_STACK_H

// Generic linked-node stack. Optional max capacity (0 = unlimited).
template <typename T>
class Stack {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& d, Node* n) : data(d), next(n) {}
    };

    Node* topNode;
    int count;
    int capacity; // 0 = unlimited

public:
    // Construct empty stack with optional capacity.
    explicit Stack(int maxCapacity = 0)
        : topNode(nullptr), count(0), capacity(maxCapacity < 0 ? 0 : maxCapacity) {}

    // Deep copy.
    Stack(const Stack& other) : topNode(nullptr), count(0), capacity(other.capacity) {
        Stack temp(other.capacity);
        for (Node* cur = other.topNode; cur != nullptr; cur = cur->next) {
            temp.push(cur->data);
        }
        Node* prev = nullptr;
        Node* cur = temp.topNode;
        temp.topNode = nullptr;
        temp.count = 0;
        while (cur != nullptr) {
            Node* nxt = cur->next;
            cur->next = prev;
            prev = cur;
            cur = nxt;
        }
        topNode = prev;
        for (Node* p = topNode; p != nullptr; p = p->next) ++count;
    }

    // Deep copy assign.
    Stack& operator=(const Stack& other) {
        if (this == &other) return *this;
        clear();
        capacity = other.capacity;
        Stack temp(other.capacity);
        for (Node* cur = other.topNode; cur != nullptr; cur = cur->next) {
            temp.push(cur->data);
        }
        Node* prev = nullptr;
        Node* cur = temp.topNode;
        temp.topNode = nullptr;
        temp.count = 0;
        while (cur != nullptr) {
            Node* nxt = cur->next;
            cur->next = prev;
            prev = cur;
            cur = nxt;
        }
        topNode = prev;
        for (Node* p = topNode; p != nullptr; p = p->next) ++count;
        return *this;
    }

    // Free all nodes.
    ~Stack() { clear(); }

    // Push if capacity allows.
    bool push(const T& value) {
        if (isFull()) return false;
        Node* n = new Node(value, topNode);
        topNode = n;
        ++count;
        return true;
    }

    // Pop top into out-param.
    bool pop(T& out) {
        if (isEmpty()) return false;
        Node* n = topNode;
        out = n->data;
        topNode = n->next;
        delete n;
        --count;
        return true;
    }

    // Read top into out-param without removing.
    bool peek(T& out) const {
        if (isEmpty()) return false;
        out = topNode->data;
        return true;
    }

    bool isEmpty() const { return count == 0; }

    bool isFull() const { return capacity != 0 && count >= capacity; }

    int size() const { return count; }

    // Remove all nodes.
    void clear() {
        while (topNode != nullptr) {
            Node* n = topNode;
            topNode = topNode->next;
            delete n;
        }
        count = 0;
    }
};

#endif // DS_STACK_H