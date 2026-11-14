#include "editor/FileManager.h"

#include <fstream>
#include <iostream>

// Write every line to path, one per line. Returns false on any I/O error.
// Walks nodes via getHead()/next so the whole save is O(n).
static bool writeToFile(const DoublyLinkedList<std::string> &doc,
                        const std::string &path)
{

    std::ofstream out(path.c_str());
    if (!out.is_open())
        return false;

    typedef DoublyLinkedList<std::string>::Node Node;
    for (Node *cur = doc.getHead(); cur != nullptr; cur = cur->next)
    {
        out << cur->data;
        if (cur->next != nullptr)
            out << '\n';
    }
    out.flush();
    return out.good();
}

bool newDocument(DoublyLinkedList<std::string> &doc,
                 std::string &filename, bool &modified)
{
    doc.clear();
    filename.clear();
    modified = false;
    return true;
}

bool openFile(DoublyLinkedList<std::string> &doc,
              std::string &filename, bool &modified,
              const std::string &path)
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
    DoublyLinkedList<std::string> temp;
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        temp.insertAtTail(line);
    }
    if (in.bad())
    {
        std::cout << "Error: failed while reading '" << path << "'.\n";
        return false;
    }

    doc = temp;
    filename = path;
    modified = false;
    return true;
}

bool saveFile(const DoublyLinkedList<std::string> &doc,
              const std::string &filename, bool &modified)
{
    if (filename.empty())
    {
        std::cout << "Error: no file name yet, use Save As.\n";
        return false;
    }
    if (!writeToFile(doc, filename))
    {
        std::cout << "Error: cannot write '" << filename << "'.\n";
        return false;
    }
    modified = false;
    return true;
}

bool saveFileAs(const DoublyLinkedList<std::string> &doc,
                std::string &filename, bool &modified,
                const std::string &path)
{
    if (path.empty())
    {
        std::cout << "Error: no file name given.\n";
        return false;
    }
    if (!writeToFile(doc, path))
    {
        std::cout << "Error: cannot write '" << path << "'.\n";
        return false;
    }
    filename = path;
    modified = false;
    return true;
}

bool exitEditor(bool modified)
{
    return !modified;
}