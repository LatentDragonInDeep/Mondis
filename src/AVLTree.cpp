//
// Created by caesarschen on 2018/8/20.
//

#include "HashMap.h"
#include "MondisServer.h"

using namespace std;

void AVLTree::insert(Entry *entry) {
    realInsert(entry->toKeyValue());
}

void AVLTree::realInsert(KeyValue *kv) {
    bool leftOrRight = true;
    AVLTreeNode *cur = root;
    AVLTreeNode *parent = nullptr;
    while (true) {
        if (cur == nullptr) {
            cur = new AVLTreeNode;
            cur->data = kv;
            _size++;
            modified();
            cur->parent = parent;
            if (parent != nullptr) {
                if (leftOrRight) {
                    parent->left = cur;
                } else {
                    parent->right = cur;
                }
            }
            if (parent == nullptr) {
                root = cur;
            }
            rebalance(cur);
            return;
        } else if (kv->compare(*cur->data)) {
            parent = cur;
            cur = cur->right;
            leftOrRight = false;
        } else if (kv->equals(*cur->data)) {
            delete cur->data->value;
            cur->data->value = kv->value;
            kv->value = nullptr;
            modified();
            return;
        } else {
            parent = cur;
            cur = cur->left;
            leftOrRight = true;
        }

    }
}

void AVLTree::remove(Key &key) {
    KeyValue temp;
    temp.key = &key;
    realRemove(temp, root);
    temp.key = nullptr;
}

AVLTree::AVLIterator AVLTree::iterator ()
{
    return AVLTree::AVLIterator(this);
}

void AVLTree::leftRotate(AVLTreeNode *root) {
    AVLTreeNode *parent = root->parent;
    int state;
    if (parent == nullptr) {
        state = 0;
    } else if (root == root->parent->left) {
        state = 1;
    } else {
        state = 2;
    }
    AVLTreeNode* right = root->right;
    AVLTreeNode* rightLeft = right->left;
    right->left = root;
    root->parent = right;
    root->right = rightLeft;
    root->height = max(getHeight(root->left), getHeight(root->right)) + 1;
    right->height = max(getHeight(right->left), getHeight(right->right)) + 1;
    if (rightLeft != nullptr) {
        rightLeft->parent = root;
    }
    switch (state) {
        case 0:
            right->parent = nullptr;
            break;
        case 1:
            right->parent = parent;
            parent->left = right;
            break;
        case 2:
            right->parent = parent;
            parent->right = right;
            break;
    }
}

void AVLTree::rightRotate(AVLTreeNode *root) {
    AVLTreeNode *parent = root->parent;
    int state;
    if (parent == nullptr) {
        state = 0;
    } else if (root == root->parent->left) {
        state = 1;
    } else {
        state = 2;
    }
    AVLTreeNode* left = root->left;
    AVLTreeNode* leftRight = left->right;
    left->right = root;
    root->parent = left;
    root->left = leftRight;
    root->height = max(getHeight(root->left), getHeight(root->right)) + 1;
    left->height = max(getHeight(left->left), getHeight(left->right)) + 1;
    if (leftRight != nullptr) {
        leftRight->parent = root;
    }
    switch (state) {
        case 0:
            left->parent = nullptr;
            break;
        case 1:
            left->parent = parent;
            parent->left = left;
            break;
        case 2:
            left->parent = parent;
            parent->right = left;
            break;
    }
}

void AVLTree::leftRightRotate(AVLTreeNode *root) {
    leftRotate(root->left);
    rightRotate(root);
}

void AVLTree::rightLeftRotate(AVLTreeNode *root) {
    rightRotate(root->right);
    leftRotate(root);
}

int AVLTree::getHeight (AVLTreeNode *root)
{
    if(root == nullptr) {
        return 0;
    }
    return root->height;
}

void AVLTree::realRemove(KeyValue &kv, AVLTreeNode *root) {
    AVLTreeNode *cur = root;
    while (true) {
        if (cur == nullptr) {
            return;
        }
        if (kv.equals(*cur->data)) {
            if (cur->left == nullptr && cur->right == nullptr) {
                if (cur->parent != nullptr) {
                    if (cur == cur->parent->left) {
                        cur->parent->left = nullptr;
                    }
                    if (cur == cur->parent->right) {
                        cur->parent->right = nullptr;
                    }
                }
                rebalance(cur->parent);
                if (cur == root) {
                    root = nullptr;
                }
                delete cur;
                modified();
                _size--;
                return;
            } else if (cur->left != nullptr && cur->right == nullptr) {
                if (cur->parent != nullptr) {
                    if (cur == cur->parent->left) {
                        cur->parent->left = cur->left;
                        cur->left->parent = cur->parent;
                    }
                    if (cur == cur->parent->right) {
                        cur->parent->right = cur->left;
                        cur->left->parent = cur->parent;
                    }
                }
                rebalance(cur->parent);
                if (cur == root) {
                    root = cur->left;
                }
                delete cur;
                modified();
                _size--;
                return;
            } else if (cur->left == nullptr && cur->right != nullptr) {
                if (cur->parent != nullptr) {
                    if (cur == cur->parent->left) {
                        cur->parent->left = cur->right;
                        cur->right->parent = cur->parent;
                    }
                    if (cur == cur->parent->right) {
                        cur->parent->right = cur->right;
                        cur->right->parent = cur->parent;
                    }
                }
                rebalance(cur->parent);
                if (cur == root) {
                    root = cur->right;
                }
                delete cur;
                modified();
                _size--;
                return;
            } else {
                AVLTreeNode *successor = getSuccessor(root);
                root->data = successor->data;
                kv.key = successor->data->key;
                realRemove(kv, root->right);
            }
        } else if (kv.compare(*root->data)) {
            cur = cur->right;
        } else {
            cur = cur->left;
        }
    }
}

AVLTreeNode *AVLTree::getSuccessor (AVLTreeNode *root)
{
    AVLTreeNode* cur = root->right;
    while (cur->left!= nullptr) {
        cur = cur->left;
    }

    return cur;
}

void AVLTree::toJson() {
    json = "";
    AVLIterator iterator = this->iterator();
    json += "{\n";
    while (iterator.next()) {
        json += iterator->data->getJson();
        json += ",";
        json += "\n";
    }
    json += "}\n";
}

AVLTree::~AVLTree() {
    deleteTree(root);
}

void AVLTree::insert(KeyValue *kv) {
    realInsert(kv);
}

KeyValue *AVLTree::get(Key &key) {
    if(key.isInteger()) {
        key.toString();
    }
    AVLTreeNode* cur = root;
    while (true) {
        if(cur == nullptr) {
            return nullptr;
        }
        if(key.equals(*cur->data->key)) {
            return cur->data;
        } else if(key.compare(*cur->data->key)) {
            cur = cur->right;
            continue;
        } else{
            cur = cur->left;
            continue;
        }
    }
}

MondisObject *AVLTree::getValue(Key &key) {
    KeyValue* kv = get(key);
    if(kv == nullptr) {
        return nullptr;
    }
    return kv->value;
}

bool AVLTree::containsKey(Key &key) {
    return get(key) != nullptr;
}

MondisObject *AVLTree::locate(Command *command) {
    if (command->params.size() != 1) {
        return nullptr;
    }
    if ((*command)[0].type != Command::ParamType::PLAIN) {
        return nullptr;
    }
    KEY(0)
    return getValue(key);
}

ExecutionResult AVLTree::execute(Command *command) {
    ExecutionResult res;
    switch (command->type) {
        case BIND: {
            CHECK_PARAM_NUM(2);
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, STRING)
            Key *key = new Key((*command)[0].content);
            insert(key, MondisServer::getJSONParser()->parseObject((*command)[1].content));
            OK_AND_RETURN;
        }
        case GET: {
            CHECK_PARAM_NUM(1);
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            auto *obj = getValue(key);
            if (obj != nullptr) {
                res.res = obj->getJson();
            }
            OK_AND_RETURN;
        }
        case DEL: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            remove(key);
            OK_AND_RETURN
        }
        case EXISTS: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            res.res = util::to_string(containsKey(key));
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

void AVLTree::insert(Key *key, MondisObject *value) {
    insert(new KeyValue(key, value));
}

unsigned AVLTree::size() {
    return _size;
}

void AVLTree::deleteTree(AVLTreeNode *root) {
    if(root == nullptr) {
        return;
    }
    AVLTreeNode* left = root->left;
    AVLTreeNode* right = root->right;
    delete root;
    deleteTree(left);
    deleteTree(right);
}

void AVLTree::rebalance(AVLTreeNode *root) {
    AVLTreeNode *cur = root;
    while (true) {
        if (cur == nullptr) {
            return;
        }
        int balanceFactor = getHeight(cur->left) - getHeight(cur->right);
        if (balanceFactor == 2) {
            AVLTreeNode *left = cur->left;
            int leftFactor = getHeight(left->left) - getHeight(left->right);
            if (leftFactor == 0 || leftFactor == 1) {
                rightRotate(cur);
            } else if (leftFactor == -1) {
                leftRightRotate(cur);
            }
        } else if (balanceFactor == -2) {
            AVLTreeNode *right = cur->right;
            int leftFactor = getHeight(right->left) - getHeight(right->right);
            if (leftFactor == 0 || leftFactor == -1) {
                leftRotate(cur);
            } else if (leftFactor == 1) {
                rightLeftRotate(cur);
            }
        }
        if (cur->parent == nullptr) {
            root = cur;
            return;
        }
        cur = cur->parent;
    }
}

bool AVLTree::isModified() {
    return !hasSerialized;
}

