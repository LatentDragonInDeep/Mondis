//
// Created by caesarschen on 2018/8/20.
//

#include "HashMap.h"

HashMap::HashMap ():HashMap(16,0.75f) {}

HashMap::HashMap (unsigned int capacity, float loadFactor): HashMap(capacity,loadFactor, false, false) {}

bool HashMap::put(Key &key, MondisObject *value)
{
    int index = getIndex(key.hashCode());
    Content content= arrayFrom[index];
    if(content.isList) {
        Entry* cur = content.first;
        for (;cur!=nullptr;cur = cur->next) {
            if(key.equals(*cur->key)) {
                cur->object = value;
                return true;
            }
        }
        Entry* newEntry = new Entry;
        newEntry->object = value;
        newEntry->key = &key;
        cur->next = newEntry;
        newEntry->pre = cur;
        content.end = newEntry;
        content.listLen++;
        if(content.listLen>treeThreshold) {
            toTree(index);
            content.isList = false;
        }
    }
    Entry* newEntry = new Entry;
    newEntry->object = value;
    newEntry->key = &key;
    content.tree->insert(*newEntry);
    size++;
    if(size/capacity>loadFactor) {
        rehash();
        return true;
    }
}

MondisObject *HashMap::get (Key &key)
{
    int index = getIndex(key.hashCode());
    Content content= arrayFrom[index];
    if(content.isList) {
        Entry* cur = content.first;
        for (;cur!=nullptr;cur = cur->next) {
            if(key.equals(*cur->key))
            {
                return cur->object;
            }
        }
        return nullptr;
    }

    return content.tree->get(key)->value;
}

bool HashMap::containsKey (Key &key)
{
    return get(key) == nullptr;
}

int HashMap::getIndex (int hash)
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
    if(! containsKey(key)) {
        return false;
    }
    int index = getIndex(key.hashCode());
    Content content= arrayFrom[index];
    if(content.isList) {
        Entry* cur = content.first;
        for (;cur!=nullptr;cur = cur->next) {
            if(key.equals(*cur->key)) {
                Entry* pre = cur->pre;
                pre->next = cur->next;
                cur->next->pre = pre;
                delete cur;
                size--;
                return true;
            }
        }
    }
    if(content.tree->get(key)) {
        size--;
        content.tree->remove(key);
        return true;
    }
    else{
        return false;
    }

}

void HashMap::toTree (int index)
{
    AVLTree * tree = new AVLTree;
    Content content = arrayFrom[index];
    Entry* cur = content.first;
    for (;cur!= nullptr;cur = cur->next)
    {
        tree->insert(*cur);
    }
    content.isList = false;
    content.tree = tree;
}

void HashMap::rehash ()
{
    capacity<<=1;
    arrayTo = new Content[capacity];
    for (int i = 0; i < capacity>>1;++i)
    {
        if(arrayFrom[i].isList) {
            for (Entry* cur = arrayFrom[i].first;cur!= nullptr;)
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
    for (int i = 0; i <capacity; ++i)
    {
        if(arrayFrom[i].isList) {
            for (Entry* cur = arrayFrom[i].first;cur!= nullptr;)
            {
                Entry* next = cur->next;
                delete cur;
                cur = next;
            }
        }
        else{
            delete arrayFrom[i].tree;
        }
    }

    delete[] arrayFrom;
}

void HashMap::add (int index, Entry *entry)
{
    Entry* cur = entry;
    if(arrayTo[index].first == nullptr) {
        arrayTo[index].first = cur;
        arrayTo[index].end = cur;
    }
    else{
        Entry* pre = arrayTo[index].end->pre;
        pre->next = cur;
        cur->pre = pre;
        arrayTo[index].end = cur;
        arrayTo[index].listLen++;
    }
}

void HashMap::toJson() {
    *json+="{\n";
    auto iter = iterator();
    while (iter.next()) {
        *json+=*iter->getJson();
        *json+=",\n";
    }
    *json+='}';
}

HashMap::MapIterator HashMap::iterator() {
    return HashMap::MapIterator(this);
}

HashMap::HashMap(float loadFactor, unsigned int capacity, const bool isValueNull, const bool isIntset) : loadFactor(
        loadFactor), capacity(getCapacity(capacity)), isValueNull(isValueNull), isIntset(isIntset) {
    arrayFrom = new Content[capacity];
}

HashMap::HashMap(const bool isValueNull, const bool isIntset) : HashMap(16,0.75f,isValueNull,isIntset) {}

