#ifndef DS_DYNAMIC_ARRAY_H
#define DS_DYNAMIC_ARRAY_H

// Generic resizable array backed by a heap-allocated buffer.
// Capacity starts at 0 and doubles whenever the buffer fills,
// giving amortized O(1) append.
template <typename T>
class DynamicArray
{
private:
    T *data;   // Heap buffer of cap slots; nullptr when cap == 0.
    int count; // Number of live elements (always <= cap).
    int cap;   // Allocated buffer size.

    // Grows the buffer: doubles capacity, or allocates 1 slot if empty.
    // Copies existing elements into the new buffer and frees the old one.
    void grow()
    {
        int newCap = (cap == 0) ? 1 : cap * 2;
        T *newData = new T[newCap];
        for (int i = 0; i < count; ++i)
            newData[i] = data[i];
        delete[] data;
        data = newData;
        cap = newCap;
    }

    // Ensures the buffer has room for at least one more element.
    void ensureCapacity()
    {
        if (count == cap)
            grow();
    }

public:
    // Constructs an empty array with no allocated buffer.
    DynamicArray() : data(nullptr), count(0), cap(0) {}

    // Copy constructor: deep copy, preserving other's capacity.
    DynamicArray(const DynamicArray &other)
        : data(nullptr), count(0), cap(0)
    {
        if (other.cap > 0)
        {
            data = new T[other.cap];
            cap = other.cap;
            for (int i = 0; i < other.count; ++i)
                data[i] = other.data[i];
            count = other.count;
        }
    }

    // Copy assignment: self-assignment safe; releases current buffer,
    // then deep copies other including its capacity.
    DynamicArray &operator=(const DynamicArray &other)
    {
        if (this == &other)
            return *this;
        delete[] data;
        data = nullptr;
        count = 0;
        cap = 0;
        if (other.cap > 0)
        {
            data = new T[other.cap];
            cap = other.cap;
            for (int i = 0; i < other.count; ++i)
                data[i] = other.data[i];
            count = other.count;
        }
        return *this;
    }

    // Destructor releases the buffer.
    ~DynamicArray() { delete[] data; }

    // Appends value at the end, growing the buffer if needed.
    bool pushBack(const T &value)
    {
        ensureCapacity();
        data[count++] = value;
        return true;
    }

    // Removes the last element into out.
    // Returns false if the array is empty.
    bool popBack(T &out)
    {
        if (count == 0)
            return false;
        out = data[--count];
        return true;
    }

    // Inserts value before index. Valid range is [0, size];
    // index == size appends. Returns false if out of range.
    // Shifts subsequent elements one slot to the right.
    bool insertAt(int index, const T &value)
    {
        if (index < 0 || index > count)
            return false;
        ensureCapacity();
        for (int i = count; i > index; --i)
            data[i] = data[i - 1];
        data[index] = value;
        ++count;
        return true;
    }

    // Removes the element at index into out.
    // Valid range is [0, size-1]; returns false if out of range.
    // Shifts subsequent elements one slot to the left.
    bool removeAt(int index, T &out)
    {
        if (index < 0 || index >= count)
            return false;
        out = data[index];
        for (int i = index; i < count - 1; ++i)
            data[i] = data[i + 1];
        --count;
        return true;
    }

    // Copies the value at index into out without modifying the array.
    // Returns false if index is out of range.
    bool get(int index, T &out) const
    {
        if (index < 0 || index >= count)
            return false;
        out = data[index];
        return true;
    }

    // Overwrites the value at index.
    // Returns false if index is out of range.
    bool set(int index, const T &value)
    {
        if (index < 0 || index >= count)
            return false;
        data[index] = value;
        return true;
    }

    // Checked non const access. Returns a static dummy on a bad index
    // so the caller cannot corrupt memory. Prefer get()/set() for safety.
    T &operator[](int index)
    {
        static T dummy{};
        if (index < 0 || index >= count)
            return dummy;
        return data[index];
    }

    // Checked const access; mirrors the non-const overload.
    const T &operator[](int index) const
    {
        static const T dummy{};
        if (index < 0 || index >= count)
            return dummy;
        return data[index];
    }

    // Returns the number of live elements.
    int size() const { return count; }

    // Returns the current allocated buffer size.
    int capacity() const { return cap; }

    // Returns true if the array contains no elements.
    bool isEmpty() const { return count == 0; }

    // Resets the logical size to 0 without releasing the buffer,
    // so subsequent pushes can reuse existing capacity.
    void clear() { count = 0; }
};

#endif // DS_DYNAMIC_ARRAY_H