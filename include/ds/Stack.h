#ifndef DS_STACK_H
#define DS_STACK_H

// Generic linked-node stack (LIFO).
// The top of the stack is the head of the singly linked node chain,
// so push/pop are O(1). An optional maximum capacity may be set
// (0 means unlimited).
template <typename T>
class Stack
{
private:
    // Node stores data and a pointer to the next (lower) stack node.
    struct Node
    {
        T data;
        Node *next;
        Node(const T &d, Node *n) : data(d), next(n) {}
    };

    Node *topNode; // Top of stack (head of chain); nullptr when empty.
    int count;     // Number of elements currently stored.
    int capacity;  // Maximum allowed size; 0 means unlimited.

public:
    // Constructs an empty stack. A negative capacity is normalized to 0
    // (unlimited) so isFull() stays consistent.
    explicit Stack(int maxCapacity = 0)
        : topNode(nullptr), count(0), capacity(maxCapacity < 0 ? 0 : maxCapacity) {}

    // Copy constructor: deep copy preserving capacity.
    // Rebuilds the chain in original order by first pushing into a temp
    // stack (which reverses order), then reversing the temp's chain.
    Stack(const Stack &other) : topNode(nullptr), count(0), capacity(other.capacity)
    {
        Stack temp(other.capacity);
        for (Node *cur = other.topNode; cur != nullptr; cur = cur->next)
        {
            temp.push(cur->data);
        }
        Node *prev = nullptr;
        Node *cur = temp.topNode;
        temp.topNode = nullptr;
        temp.count = 0;
        // Reverse the temp chain so topNode again matches other's order.
        while (cur != nullptr)
        {
            Node *nxt = cur->next;
            cur->next = prev;
            prev = cur;
            cur = nxt;
        }
        topNode = prev;
        for (Node *p = topNode; p != nullptr; p = p->next)
            ++count;
    }

    // Copy-assignment: self-assignment safe; clears existing nodes and
    // capacity, then deep-copies using the same reverse strategy as above.
    Stack &operator=(const Stack &other)
    {
        if (this == &other)
            return *this;
        clear();
        capacity = other.capacity;
        Stack temp(other.capacity);
        for (Node *cur = other.topNode; cur != nullptr; cur = cur->next)
        {
            temp.push(cur->data);
        }
        Node *prev = nullptr;
        Node *cur = temp.topNode;
        temp.topNode = nullptr;
        temp.count = 0;
        // Reverse the temp chain so topNode again matches other's order.
        while (cur != nullptr)
        {
            Node *nxt = cur->next;
            cur->next = prev;
            prev = cur;
            cur = nxt;
        }
        topNode = prev;
        for (Node *p = topNode; p != nullptr; p = p->next)
            ++count;
        return *this;
    }

    // Destructor releases all nodes.
    ~Stack() { clear(); }

    // Pushes value onto the top. Returns false if the stack is full.
    bool push(const T &value)
    {
        if (isFull())
            return false;
        Node *n = new Node(value, topNode);
        topNode = n;
        ++count;
        return true;
    }

    // Removes the top element into out.
    // Returns false if the stack is empty.
    bool pop(T &out)
    {
        if (isEmpty())
            return false;
        Node *n = topNode;
        out = n->data;
        topNode = n->next;
        delete n;
        --count;
        return true;
    }

    // Reads the top element into out without removing it.
    // Returns false if the stack is empty.
    bool peek(T &out) const
    {
        if (isEmpty())
            return false;
        out = topNode->data;
        return true;
    }

    // Returns true if the stack contains no elements.
    bool isEmpty() const { return count == 0; }

    // Returns true if a finite capacity is set and it has been reached.
    bool isFull() const { return capacity != 0 && count >= capacity; }

    // Returns the number of elements currently stored.
    int size() const { return count; }

    // Removes and deletes all nodes, resetting the stack to empty.
    void clear()
    {
        while (topNode != nullptr)
        {
            Node *n = topNode;
            topNode = topNode->next;
            delete n;
        }
        count = 0;
    }
};

#endif // DS_STACK_H