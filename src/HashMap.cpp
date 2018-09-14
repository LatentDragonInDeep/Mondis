//
// Created by caesarschen on 2018/8/20.
//

#include "HashMap.h"

HashMap::HashMap ():HashMap(16,0.75f) {}

HashMap::HashMap (unsigned int capacity, float loadFactor): HashMap(capacity,loadFactor, false, false) {}

bool HashMap::put(Key *key, MondisObject *value)
{
    checkType(key);
    int index = getIndex(key->hashCode());
    Content &content = arrayFrom[index];
    if(content.isList) {
        Entry *cur = content.head->next;
        for (; cur != content.tail; cur = cur->next) {
            if (key->equals(*cur->key)) {
                delete cur->object;
                cur->object = value;
                modified();
                return true;
            }
        }
        Entry* newEntry = new Entry;
        newEntry->object = value;
        newEntry->key = key;
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
            return true;
        } else if (content.listLen > treeThreshold) {
            toTree(index);
            return true;
        }
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

MondisObject *HashMap::get (Key &key)
{
    if(isIntset&&!key.isInteger()) {
        return nullptr;
    }
    if(!isIntset) {
        key.toString();
    }
    int index = getIndex(key.hashCode());
    Content &content = arrayFrom[index];
    if(content.isList) {
        Entry *cur = content.head->next;
        for (; cur != content.tail; cur = cur->next) {
            if(key.equals(*cur->key))
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

bool HashMap::containsKey (Key &key)
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

bool HashMap::remove (Key &key)
{
    if(isIntset&&!key.isInteger()) {
        return false;
    }
    if(!isIntset) {
        key.toString();
    }
    int index = getIndex(key.hashCode());
    Content &content = arrayFrom[index];
    if(content.isList) {
        Entry *cur = content.head->next;
        for (; cur != content.tail; cur = cur->next) {
            if(key.equals(*cur->key)) {
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
            for (Entry *cur = arrayFrom[i].head; cur != nullptr;)
            {
                Entry* next = cur->next;
                int index = cur->key->hashCode()&(capacity-1);
                add(index,cur);
            }
        }
        else{
            auto treeIterator = arrayFrom[i].tree->iterator();
            while (treeIterator.next()) {
                int index = treeIterator->data->key->hashCode()&(capacity-1);
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
    json = "";
    json += "{\n";
    auto iter = iterator();
    while (iter.next()) {
        json += iter->getJson();
        json += ",\n";
    }
    json += '}';
}

HashMap::MapIterator HashMap::iterator() {
    return HashMap::MapIterator(this);
}

HashMap::HashMap(const bool isIntset) : HashMap(1024, 0.75f, isIntset, true) {}

MondisObject *HashMap::locate(Command *command) {
    if (command->params.size() != 1) {
        return nullptr;
    }
    if ((*command)[0].type != Command::ParamType::PLAIN) {
        return nullptr;
    }
    Key key((*command)[0].content);
    return get(key);
}

ExecutionResult HashMap::execute(Command *command) {
    ExecutionResult res;
    switch (command->type) {
        case ADD: {
            CHECK_PARAM_NUM(1)
            Key *key = new Key((*command)[0].content);
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
            res.res = to_string(containsKey(key));
            OK_AND_RETURN
        }
        case SIZE: {
            CHECK_PARAM_NUM(0)
            res.res = to_string(size());
            OK_AND_RETURN
        }
    }
    INVALID_AND_RETURN
}

void HashMap::toStringSet() {
    if(!isIntset) {
        return;
    }
    arrayTo = new Content[capacity];
    for (int i = 0; i < capacity;++i)
    {
        if(arrayFrom[i].isList) {
            for (Entry *cur = arrayFrom[i].head; cur != nullptr;)
            {
                Entry* next = cur->next;
                Key* k = cur->key;
                k->toString();
                int index = k->hashCode()&(capacity-1);
                add(index,cur);
            }
        }
        else{
            auto treeIterator = arrayFrom[i].tree->iterator();
            while (treeIterator.next()) {
                Key* k = treeIterator->data->key;
                k->toString();
                int index = k->hashCode()&(capacity-1);
                add(index,new Entry(treeIterator->data));
                delete arrayFrom[i].tree;
            }
        }
    }
    delete[] arrayFrom;
    arrayFrom = arrayTo;
    isIntset = false;
}

void HashMap::checkType(Key *key) {
    if(isIntset) {
        if (!key->isInteger()) {
            toStringSet();
        }
    }
}

unsigned HashMap::size() {
    return _size;
}

HashMap::HashMap(float loadFactor, unsigned int capa, bool isIntset, bool isValueNull) :
        loadFactor(loadFactor), capacity(capa), isIntset(isIntset), isValueNull(isValueNull) {
    arrayFrom = new Content[capacity];
}

void HashMap::clear() {
    for (int i = 0; i < capacity; ++i) {
        arrayFrom[i].clear();
    }
}

