#pragma once

#include <string>
#include "ds/DoublyLinkedList.h"
#include "ds/Stack.h"
#include "ds/DynamicArray.h"
#include "editor/Types.h"

class Editor
{
public:
    // Core document data structures
    DoublyLinkedList<Line> document;
    Stack<Command> undoStack;
    Stack<Command> redoStack;
    DynamicArray<Bookmark> bookmarks;

    // Editor state
    int cursorLine = 0;
    int cursorCol = 0;
    std::string filename = "";
    bool modified = false;

    // Constructor initializes the editor.
    Editor() { reset(); }

    // Disable copying to prevent accidental deep-copies of the whole editor.
    Editor(const Editor &) = delete;
    Editor &operator=(const Editor &) = delete;

    // Returns the total number of lines in the document.
    int getLineCount() const
    {
        return document.size();
    }

    // Returns the total number of characters across all lines.
    int getCharCount() const
    {
        int total = 0;
        for (int i = 0; i < document.size(); ++i)
        {
            Line l;
            if (document.get(i, l))
            {
                total += l.text.length();
            }
        }
        return total;
    }

    // Returns true if the document has no lines at all.
    bool isEmpty() const
    {
        return document.isEmpty();
    }

    // Returns a pointer to the Line at the given index, or nullptr if out of bounds.
    Line *getLine(int index)
    {
        if (index < 0 || index >= document.size())
        {
            return nullptr;
        }
        // Since DoublyLinkedList::get() requires an out-parameter, we use a trick:
        // we can't return a reference directly without modifying the ADT,
        // so we check the head and walk (or we can just use the ADT's public traversal).
        // Wait! DoublyLinkedList has getHead() and nodes are public.
        // Let's use the public node traversal to get a pointer without copying.
        DoublyLinkedList<Line>::Node *curr = document.getHead();
        for (int i = 0; i < index && curr != nullptr; ++i)
        {
            curr = curr->next;
        }
        return curr ? &(curr->data) : nullptr;
    }

    const Line *getLine(int index) const
    {
        if (index < 0 || index >= document.size()) return nullptr;
        const DoublyLinkedList<Line>::Node *curr = document.getHead();
        for (int i = 0; i < index && curr != nullptr; ++i) curr = curr->next;
        return curr ? &(curr->data) : nullptr;
    }

    // Returns the current modified status of the editor.
    bool isModified() const
    {
        return modified;
    }

    // Sets the modified status of the editor.
    void setModified(bool status)
    {
        modified = status;
    }

    // Returns the filename associated with the current document.
    std::string getFilename() const
    {
        return filename;
    }

    // Updates the filename of the current document.
    void setFilename(const std::string &name)
    {
        filename = name;
    }

    // Clears everything and resets the editor to a blank state with one empty line.
    void reset()
    {
        document.clear();
        undoStack.clear();
        redoStack.clear();
        bookmarks.clear();

        cursorLine = 0;
        cursorCol = 0;
        filename = "";
        modified = false;

        // Start with one empty line so the user has a place to type.
        Line emptyLine;
        document.insertAtTail(emptyLine);
    }
};
