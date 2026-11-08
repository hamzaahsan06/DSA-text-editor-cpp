#ifndef UNDO_REDO_H
#define UNDO_REDO_H

#include "Stack.h"

class DocString {
private:
    char* data;
    int len;

    static int cstrLen(const char* s) {
        int n = 0;
        while (s[n] != '\0') ++n;
        return n;
    }

    // Replace contents with a copy of s
    void set(const char* s, int n) {
        char* buf = new char[n + 1];
        for (int i = 0; i < n; ++i) buf[i] = s[i];
        buf[n] = '\0';
        delete[] data;
        data = buf;
        len = n;
    }

    // Basis for (insert/erase/replace) all three operations in one function .
    bool splice(int pos, int count, const DocString& t) {
        if (pos < 0 || count < 0 || pos + count > len) return false;
        int newLen = len - count + t.len;
        char* buf = new char[newLen + 1];
        int k = 0;
        for (int i = 0; i < pos; ++i) buf[k++] = data[i];
        for (int i = 0; i < t.len; ++i) buf[k++] = t.data[i];
        for (int i = pos + count; i < len; ++i) buf[k++] = data[i];
        buf[k] = '\0';
        delete[] data;
        data = buf;
        len = newLen;
        return true;
    }

public:
    DocString() : data(nullptr), len(0) { set("", 0); }
    DocString(const char* s) : data(nullptr), len(0) { set(s, cstrLen(s)); }
    DocString(const DocString& o) : data(nullptr), len(0) { set(o.data, o.len); }

    DocString& operator=(const DocString& o) {
        if (this != &o) set(o.data, o.len);
        return *this;
    }

    ~DocString() { delete[] data; }

    int length() const { return len; }
    const char* c_str() const { return data; }

    bool insert(int pos, const DocString& t) { return splice(pos, 0, t); }
    bool erase(int pos, int count) { return splice(pos, count, DocString()); }
    bool replace(int pos, int count, const DocString& t) { return splice(pos, count, t); }
};

class UndoRedo {
private:
    enum OperationType { INSERT, ERASE, REPLACE };

    struct Operation {
        OperationType type;
        int position;
        DocString oldText;
        DocString newText;

        Operation(OperationType t, int p, const DocString& oldT, const DocString& newT)
            : type(t), position(p), oldText(oldT), newText(newT) {}
    };

    Stack<Operation> undoStack;
    Stack<Operation> redoStack;

    bool record(const Operation& op) {
        if (op.position < 0) return false;
        if (!undoStack.push(op)) return false;
        redoStack.clear();   // new action will invaildes redo history and clear it
        return true;
    }

public:
    // These only RECORD an edit if same edit is applied by user.
    bool recordInsert(int position, const DocString& text) {
        return record(Operation(INSERT, position, "", text));
    }

    bool recordErase(int position, const DocString& erasedText) {
        return record(Operation(ERASE, position, erasedText, ""));
    }

    bool recordReplace(int position, const DocString& oldText, const DocString& newText) {
        return record(Operation(REPLACE, position, oldText, newText));
    }

    // Apply the inverse of the latest operation, then move it to the redo stack.
    bool undo(DocString& document) {
        Operation op(INSERT, 0, "", "");
        if (redoStack.isFull() || !undoStack.peek(op)) return false;

        bool ok = false;
        switch (op.type) {
            case INSERT:  ok = document.erase(op.position, op.newText.length()); break;
            case ERASE:   ok = document.insert(op.position, op.oldText); break;
            case REPLACE: ok = document.replace(op.position, op.newText.length(), op.oldText); break;
        }
        if (!ok) return false;   // document untouched, stacks untouched 

        redoStack.push(op);      // cannot fail: isFull() checked above
        undoStack.pop(op);
        return true;
    }

    // Re-apply the latest undone operation, then move it back to the undo stack.
    bool redo(DocString& document) {
        Operation op(INSERT, 0, "", "");
        if (undoStack.isFull() || !redoStack.peek(op)) return false;

        bool ok = false;
        switch (op.type) {
            case INSERT:  ok = document.insert(op.position, op.newText); break;
            case ERASE:   ok = document.erase(op.position, op.oldText.length()); break;
            case REPLACE: ok = document.replace(op.position, op.oldText.length(), op.newText); break;
        }
        if (!ok) return false;

        undoStack.push(op);
        redoStack.pop(op);
        return true;
    }

    bool canUndo() const { return !undoStack.isEmpty(); }
    bool canRedo() const { return !redoStack.isEmpty(); }

    void clearHistory() {
        undoStack.clear();
        redoStack.clear();
    }
};

#endif // UNDO_REDO_H