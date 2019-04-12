//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_HASHMAP_H
#define MONDIS_HASHMAP_H

#include <stack>
#include <mutex>
#include <shared_mutex>

#include "MondisObject.h"
#include "Command.h"
#include "AVLTree.h"

using namespace std;

class Entry:public KeyValue{
public:
    Entry* pre = nullptr;
    Entry* next = nullptr;
    Entry(string& k,MondisObject* v):KeyValue(k,v){};
    Entry():KeyValue(){};
};

class HashMap:public MondisData
{
private:
    float loadFactor = 0.75f;
    unsigned int capacity = 16;
    unsigned int _size = 0;
    const int treeThreshold = 8;
    const bool isValueNull = false;

    struct Content {
        Entry *head = new Entry;
        Entry *tail = new Entry;
        AVLTree *tree = nullptr;
        int listLen = 0;
        bool isList = true;

        Content() {
            head->next = tail;
            tail->pre = head;
        }

        void reset() {
            if (isList) {
                for(Entry *cur = head->next;cur!=tail;cur = cur->next) {
                    Entry* next = cur->next;
                    delete cur;
                    cur = next;
                }
                head->next = tail;
                tail->pre = head;
            } else {
                delete tree;
            }
            isList = true;
            listLen = 0;
        }

        ~Content() {
            delete head;
            delete tail;
            delete tree;
        }
    };
    Content * arrayFrom = nullptr;
    Content * arrayTo = nullptr;
    mutex* mutexes = nullptr;
    shared_mutex globalMutex;
    class MapIterator{
    private:
        KeyValue* cur = nullptr;
        Content * array;
        unsigned slotIndex = 0;
        AVLTree::AVLIterator avlIterator;
        HashMap* map = nullptr;
        Content* curContent = nullptr;
        bool lookForNextSlot() {
            while (true) {
                slotIndex++;
                if (slotIndex >= map->capacity) {
                    return false;
                }
                Content &current = array[slotIndex];
                if(current.isList) {
                    if (current.head->next == current.tail) {
                        slotIndex++;
                        continue;
                    }
                    curContent = &current;
                    cur = current.head->next;
                    return true;
                } else {
                    avlIterator = current.tree->iterator();
                    cur = avlIterator.getData();
                    return true;
                }
            }
        };
    public:
        MapIterator(HashMap* map) : map(map),array(map->arrayFrom){
            curContent = &array[0];
            cur = curContent->head;
        };
        bool next() {
            if (curContent->isList) {
                Entry* entry = dynamic_cast<Entry*>(cur);
                Entry* next = entry->next;
                if (next!=curContent->tail) {
                    cur = next;
                    return true;
                }
            } else {
                bool hasNext = avlIterator.next();
                if(hasNext) {
                    cur = avlIterator.getData();
                    return true;
                }
            }
            return lookForNextSlot();
        };

        KeyValue* operator->() {
            return cur;
        };
    };
public:
    HashMap();
    HashMap(unsigned int capacity, float loadFactor,bool = false);
    ~HashMap();

    bool put(string& key, MondisObject *value);
    MondisObject* get (string &key);
    bool containsKey (string &key);
    bool remove (string &key);
    unsigned size();
    void clear();
private:
    unsigned int hash(string &str);
    void rehash();
    void toTree (int index);
    int getCapacity(int);
    unsigned getIndex(unsigned hash);
    void addToSlot(int index, Entry *entry);
    void toJson();
public:
    HashMap::MapIterator iterator();
    virtual ExecRes execute(Command *command);

    virtual MondisObject *locate(Command *command);
};


#endif //MONDIS_HASHMAP_H
