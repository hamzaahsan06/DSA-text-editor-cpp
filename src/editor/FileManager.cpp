#include "editor/FileManager.h"

#include <fstream>
#include <iostream>

// Write every line with a terminating newline. A final newline lets a second
// newline represent a trailing empty line when the file is read back.
// Walks nodes via getHead()/next so the whole save is O(n).
static bool writeToFile(Editor &ed, const std::string &path)
{
    std::ofstream out(path.c_str());
    if (!out.is_open())
        return false;

    typedef DoublyLinkedList<Line>::Node Node;
    for (Node *cur = ed.document.getHead(); cur != nullptr; cur = cur->next)
    {
        out << cur->data.text;
        out << '\n';
    }
    out.flush();
    return out.good();
}

bool newDocument(Editor &ed)
{
    ed.reset();
    return true;
}

bool openFile(Editor &ed, const std::string &path)
{
    if (path.empty())
    {
        std::cout << "Error: no file name given.\n";
        return false;
    }

    std::ifstream in(path.c_str());
    if (!in.is_open())
    {
        std::cout << "Error: cannot open '" << path << "'.\n";
        return false;
    }

    // Read into a temporary list so a failed read never destroys the document.
    DoublyLinkedList<Line> temp;
    std::string lineText;
    while (std::getline(in, lineText))
    {
        if (!lineText.empty() && lineText[lineText.size() - 1] == '\r')
            lineText.erase(lineText.size() - 1);
        
        Line l;
        l.text = lineText;
        temp.insertAtTail(l);
    }
    if (in.bad())
    {
        std::cout << "Error: failed while reading '" << path << "'.\n";
        return false;
    }

    // Keep the editor invariant that a document always has an editable line,
    // including when opening an empty file.
    if (temp.isEmpty())
    {
        Line emptyLine;
        temp.insertAtTail(emptyLine);
    }

    ed.document = temp;
    ed.undoStack.clear();
    ed.redoStack.clear();
    ed.bookmarks.clear();
    ed.cursorLine = 0;
    ed.cursorCol = 0;
    ed.setFilename(path);
    ed.setModified(false);
    return true;
}

bool saveFile(Editor &ed)
{
    if (ed.getFilename().empty())
    {
        std::cout << "Error: no file name yet, use Save As.\n";
        return false;
    }
    if (!writeToFile(ed, ed.getFilename()))
    {
        std::cout << "Error: cannot write '" << ed.getFilename() << "'.\n";
        return false;
    }
    ed.setModified(false);
    return true;
}

bool saveFileAs(Editor &ed, const std::string &path)
{
    if (path.empty())
    {
        std::cout << "Error: no file name given.\n";
        return false;
    }
    if (!writeToFile(ed, path))
    {
        std::cout << "Error: cannot write '" << path << "'.\n";
        return false;
    }
    ed.setFilename(path);
    ed.setModified(false);
    return true;
}

bool exitEditor(Editor &ed)
{
    return !ed.isModified();
}
