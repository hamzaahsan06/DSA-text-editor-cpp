#ifndef DS_DYNAMIC_ARRAY_H
#define DS_DYNAMIC_ARRAY_H

// Generic resizable array; capacity doubles when full.
template <typename T>
class DynamicArray {
private:
    T* data;
    int count;
    int cap;

    // Grow capacity (doubles, min 1).
    void grow() {
        int newCap = (cap == 0) ? 1 : cap * 2;
        T* newData = new T[newCap];
        for (int i = 0; i < count; ++i) newData[i] = data[i];
        delete[] data;
        data = newData;
        cap = newCap;
    }

    // Ensure room for one more element.
    void ensureCapacity() {
        if (count == cap) grow();
    }

public:
    DynamicArray() : data(nullptr), count(0), cap(0) {}

    // Deep copy.
    DynamicArray(const DynamicArray& other)
        : data(nullptr), count(0), cap(0) {
        if (other.cap > 0) {
            data = new T[other.cap];
            cap = other.cap;
            for (int i = 0; i < other.count; ++i) data[i] = other.data[i];
            count = other.count;
        }
    }

    // Deep copy assign.
    DynamicArray& operator=(const DynamicArray& other) {
        if (this == &other) return *this;
        delete[] data;
        data = nullptr;
        count = 0;
        cap = 0;
        if (other.cap > 0) {
            data = new T[other.cap];
            cap = other.cap;
            for (int i = 0; i < other.count; ++i) data[i] = other.data[i];
            count = other.count;
        }
        return *this;
    }

    ~DynamicArray() { delete[] data; }

    // Append.
    bool pushBack(const T& value) {
        ensureCapacity();
        data[count++] = value;
        return true;
    }

    // Remove last into out-param.
    bool popBack(T& out) {
        if (count == 0) return false;
        out = data[--count];
        return true;
    }

    // Insert before index; valid range [0, size].
    bool insertAt(int index, const T& value) {
        if (index < 0 || index > count) return false;
        ensureCapacity();
        for (int i = count; i > index; --i) data[i] = data[i - 1];
        data[index] = value;
        ++count;
        return true;
    }

    // Remove at index into out-param.
    bool removeAt(int index, T& out) {
        if (index < 0 || index >= count) return false;
        out = data[index];
        for (int i = index; i < count - 1; ++i) data[i] = data[i + 1];
        --count;
        return true;
    }

    // Copy value at index into out-param.
    bool get(int index, T& out) const {
        if (index < 0 || index >= count) return false;
        out = data[index];
        return true;
    }

    // Overwrite value at index.
    bool set(int index, const T& value) {
        if (index < 0 || index >= count) return false;
        data[index] = value;
        return true;
    }

    // Checked access; returns default T on bad index.
    T& operator[](int index) {
        static T dummy{};
        if (index < 0 || index >= count) return dummy;
        return data[index];
    }

    const T& operator[](int index) const {
        static const T dummy{};
        if (index < 0 || index >= count) return dummy;
        return data[index];
    }

    int size() const { return count; }

    int capacity() const { return cap; }

    bool isEmpty() const { return count == 0; }

    // Logical clear (keeps allocated buffer).
    void clear() { count = 0; }
};

#endif // DS_DYNAMIC_ARRAY_H