//
// Created by caesarschen on 2018/8/20.
//

#include "HashMap.h"

HashMap::HashMap ():HashMap(16,0.75f) {}

bool HashMap::put(string& key, MondisObject *value)
{
    int index = getIndex(hash(key));
    Content &content = arrayFrom[index];
    if(content.isList) {
        Entry *cur = content.head->next;
        for (; cur != content.tail; cur = cur->next) {
            if (key == cur->key) {
                delete cur->object;
                cur->object = value;
                modified();
                return true;
            }
        }
        Entry *newEntry = new Entry(key, value);
        cur = content.head;
        Entry *next = cur->next;
        cur->next = newEntry;
        newEntry->pre = cur;
        newEntry->next = next;
        next->pre = newEntry;
        content.listLen++;
        _size++;
        modified();
        if (((double) _size) / capacity > loadFactor) {
            rehash();
        } else if (content.listLen > treeThreshold) {
            toTree(index);
        }
        return true;
    }
    _size -= content.tree->size();
    Entry* newEntry = new Entry;
    newEntry->object = value;
    newEntry->key = key;
    content.tree->insert(newEntry);
    if (content.tree->modified()) {
        modified();
    }
    _size += content.tree->size();
    if (((double) _size) / capacity > loadFactor) {
        rehash();
        return true;
    }
}

MondisObject *HashMap::get (string &key)
{
    int index = getIndex(hash(key));
    Content &content = arrayFrom[index];
    if(content.isList) {
        Entry *cur = content.head->next;
        for (; cur != content.tail; cur = cur->next) {
            if(key == cur->key)
            {
                return cur->object;
            }
        }
        return nullptr;
    }

    KeyValue *kv = content.tree->get(key);
    if (kv == nullptr) {
        return nullptr;
    }
    return kv->value;
}

bool HashMap::containsKey (string &key)
{
    return get(key) != nullptr;
}

unsigned HashMap::getIndex(unsigned hash)
{
    return hash&(capacity-1);
}

int HashMap::getCapacity (int capa)
{
    int init = 1;
    while ((init<<1)<capa) {
        init<<=1;
    }

    return init;
}

bool HashMap::remove (string &key)
{
    int index = getIndex(hash(key));
    Content &content = arrayFrom[index];
    if(content.isList) {
        Entry *cur = content.head->next;
        for (; cur != content.tail; cur = cur->next) {
            if(key == cur->key) {
                Entry* pre = cur->pre;
                pre->next = cur->next;
                cur->next->pre = pre;
                delete cur;
                _size--;
                return true;
            }
        }
    }
    if(content.tree->get(key)) {
        _size--;
        content.tree->remove(key);
        return true;
    }
    return false;
}

void HashMap::toTree (int index)
{
    AVLTree * tree = new AVLTree;
    Content &content = arrayFrom[index];
    Entry *cur = content.head;
    for (; cur != content.tail; cur = cur->next)
    {
        tree->insert(cur);
    }
    content.isList = false;
    content.tree = tree;
    content.reset();
}

void HashMap::rehash ()
{
    capacity<<=1;
    arrayTo = new Content[capacity];
    for (int i = 0; i < capacity>>1;++i)
    {
        if(arrayFrom[i].isList) {
            for (Entry *cur = arrayFrom[i].head->next; cur != arrayFrom[i].tail;)
            {
                int index = hash(cur->key)&(capacity-1);
                add(index,cur);
            }
        }
        else{
            auto treeIterator = arrayFrom[i].tree->iterator();
            while (treeIterator.next()) {
                int index = hash(treeIterator->data->key)&(capacity-1);
                add(index,new Entry(treeIterator->data));
                delete arrayFrom[i].tree;
            }
        }
    }
    delete[] arrayFrom;
    arrayFrom = arrayTo;
}

HashMap::~HashMap ()
{
    delete[] arrayFrom;
}

void HashMap::add (int index, Entry *entry)
{
    Entry* cur = entry;
    if (arrayTo[index].head == nullptr) {
        arrayTo[index].head = cur;
        arrayTo[index].tail = cur;
    }
    else{
        Entry *pre = arrayTo[index].tail->pre;
        pre->next = cur;
        cur->pre = pre;
        arrayTo[index].tail = cur;
        arrayTo[index].listLen++;
    }
}

void HashMap::toJson() {
    if (isValueNull) {
        json = "";
        json += "[\n";
        auto iter = iterator();
        while (iter.next()) {
            json += iter->getJson();
            json += ",\n";
        }
        json += ']';
    } else {
        json = "";
        json += "{\n";
        auto iter = iterator();
        while (iter.next()) {
            json += iter->getJson();
            json += ",\n";
        }
        json += '}';
    }
}

HashMap::MapIterator HashMap::iterator() {
    return HashMap::MapIterator(this);
}

MondisObject *HashMap::locate(Command *command) {
    if (command->params.size() != 1) {
        return nullptr;
    }
    if ((*command)[0].type != Command::ParamType::PLAIN) {
        return nullptr;
    }
    KEY(0)
    return get(key);
}

ExecRes HashMap::execute(Command *command) {
    ExecRes res;
    switch (command->type) {
        case ADD: {
            CHECK_PARAM_NUM(1)
            KEY(0)
            put(key, nullptr);
            OK_AND_RETURN
        }
        case REMOVE: {
            CHECK_PARAM_NUM(1)
            KEY(0)
            remove(key);
            OK_AND_RETURN
        }
        case EXISTS: {
            CHECK_PARAM_NUM(1)
            KEY(0)
            res.desc = util::to_string(containsKey(key));
            OK_AND_RETURN
        }
        case M_SIZE: {
            CHECK_PARAM_NUM(0)
            res.desc = to_string(size());
            OK_AND_RETURN
        }
    }
    INVALID_AND_RETURN
}

unsigned int HashMap::hash(string &str) {
    int len = str.size();
    const char * key = str.c_str();
    uint32_t seed = 2017;
    const uint32_t m = 0x5bd1e995;
    const int r = 24;

    uint32_t h = seed ^len;

    const unsigned char *data = (const unsigned char *)key;

    while(len >= 4) {
        uint32_t k = *(uint32_t*)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch(len) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0]; h *= m;
    };


    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return (unsigned int)h;
}

unsigned HashMap::size() {
    return _size;
}

void HashMap::clear() {
    for (int i = 0; i < capacity; ++i) {
        arrayFrom[i].clear();
    }
}

