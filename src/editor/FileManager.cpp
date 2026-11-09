#include<FileManager.h>

bool newDocument(DoublyLinkedList<std::string>& doc, std::string& filename, bool& modified){

}
bool openFile(DoublyLinkedList<std::string>& doc, std::string& filename, bool& modified, const std::string& path);
bool saveFile(const DoublyLinkedList<std::string>& doc, const std::string& filename, bool& modified);
bool saveFileAs(const DoublyLinkedList<std::string>& doc, std::string& filename, bool& modified, const std::string& path);
bool exitEditor(bool modified){
    if(!modified)
        return true;
    return false;
}
