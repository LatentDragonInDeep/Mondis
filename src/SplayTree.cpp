//
// Created by caesarschen on 2018/8/21.
//

#include <limits>
#include "SplayTree.h"
#include "MondisServer.h"


void SplayTree::toJson() {
    string res;
    SplayIterator iterator = this->iterator();
    res += "{";
    res += "\n";
    while (iterator.next()) {
        string score = "\"" + to_string(iterator->score) + "\"";
        res += score;
        res += ",\n";
        res += iterator->data->getJson();
        res += ",\n";
    }
    res += "}\n";
    return res;
}


SplayTree::SplayIterator SplayTree::iterator() {
    return SplayIterator(this);
}

SplayTree::SplayTree() {
    root = head;
    head->right = tail;
    tail->parent = head;
}

SplayTree::~SplayTree() {
    deleteTree(root);
}

bool SplayTree::insert(int score, MondisObject *obj) {
    modified();
    bool leftOrRight = true;
    SplayTreeNode* cur =root;
    SplayTreeNode* parent = nullptr;
    while (true) {
        if(cur == nullptr) {
            cur = new SplayTreeNode(score,obj);
            cur->parent = parent;
            if (parent != nullptr) {
                if (leftOrRight) {
                    parent->left = cur;
                } else {
                    parent->right = cur;
                }
            }
            sizeUpdate(cur,1);
            splay(cur, nullptr);
            return true;
        }
        if(cur->score == score) {
            delete cur->data;
            cur->data = obj;
            splay(cur, nullptr);
            return false;
        } else if(cur->score>score) {
            parent = cur;
            cur = cur->left;
            leftOrRight = true;
        } else{
            parent = cur;
            cur = cur->right;
            leftOrRight = false;
        }
    }
}

MondisObject *SplayTree::getByScore(int score) {
    SplayTreeNode* cur = getNodeByScore(score);
    if(cur == nullptr) {
        return nullptr;
    }

    return cur->data;
}

MondisObject *SplayTree::getByRank(SplayTreeNode* root,int rank) {
    SplayTreeNode* node = getNodeByRank(root,rank);
    if(node == nullptr) {
        return nullptr;
    }
    return node->data;
}

MondisObject *SplayTree::getByRank(int rank) {
    return getByRank(root,rank);
}

unsigned SplayTree::getSize(SplayTreeNode *root) {
    if(root == nullptr||root == head||root == tail) {
        return 0;
    }
    return root->treeSize;
}

SplayTreeNode *SplayTree::getNodeByRank(SplayTreeNode *root, int rank) {
    if(root == nullptr) {
        return nullptr;
    }
    if (root == this->root && rank == 0) {
        return head;
    }
    if (root == this->root && rank == root->treeSize + 1) {
        return tail;
    }
    if (root == head) {
        root = root->right;
    }
    if (root == tail) {
        root = root->left;
    }
    if (rank > root->treeSize) {
        return nullptr;
    }
    unsigned leftSize = getSize(root->left);
    if (rank <= leftSize) {
        return getNodeByRank(root->left,rank);
    }
    if(rank == leftSize+1) {
        return root;
    }
    return getNodeByRank(root->right,rank-(leftSize+1));
}

SplayTreeNode *SplayTree::getNodeByScore(int score) {
    if (score == numeric_limits<int>::min()) {
        return head;
    }
    if (score == numeric_limits<int>::max()) {
        return tail;
    }
    SplayTreeNode* cur = root;
    while (true) {
        if(cur == nullptr) {
            return nullptr;
        }
        if(cur->score == score) {
            return cur;
        } else if(cur->score>score) {
            cur = cur->left;
        } else{
            cur = cur->right;
        }
    }
}

bool SplayTree::contains(int score) {
    return getByScore(score) != nullptr;
}

unsigned SplayTree::count(int startScore, int endScore) {
    if(startScore == endScore) {
        if (contains(startScore)) {
            return 1;
        }
        return 0;
    }
    SplayTreeNode *from = getLowerBound(startScore, false);
    SplayTreeNode *to = getUpperBound(endScore, true);
    splay(from, nullptr);
    splay(to, from);
    return getSize(root->right->left);
}

unsigned SplayTree::size() {
    if (root == nullptr) {
        return 0;
    }
    return root->treeSize;
}

void SplayTree::sizeUpdate(SplayTreeNode *cur,int delta) {
    while (cur!= nullptr) {
        cur->treeSize+=delta;
        cur = cur->parent;
    }
}

void SplayTree::leftRotate(SplayTreeNode *root) {
    SplayTreeNode* parent = root->parent;
    int state;
    if(parent == nullptr) {
        state = 0;
    } else if(root == root->parent->left) {
        state = 1;
    } else {
        state = 2;
    }
    SplayTreeNode* right = root->right;
    SplayTreeNode* rightLeft = right->left;
    right->left = root;
    root->parent = right;
    right->treeSize = root->treeSize;
    root->right = rightLeft;
    if(rightLeft!= nullptr) {
        rightLeft->parent = root;
    }
    root->treeSize = getSize(root->left)+getSize(root->right)+1;
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

void SplayTree::rightRotate(SplayTreeNode *root) {
    SplayTreeNode* parent = root->parent;
    int state;
    if(parent == nullptr) {
        state = 0;
    } else if(root == root->parent->left) {
        state = 1;
    } else {
        state = 2;
    }
    SplayTreeNode* left = root->left;
    SplayTreeNode* leftRight = left->right;
    left->right = root;
    root->parent = left;
    left->treeSize = root->treeSize;
    root->left = leftRight;
    if(leftRight!= nullptr) {
        leftRight->parent = root;
    }
    root->treeSize = getSize(root->left)+getSize(root->right)+1;
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

void SplayTree::leftRightRotate(SplayTreeNode *root) {
    leftRotate(root->left);
    rightRotate(root);
}

void SplayTree::rightLeftRotate(SplayTreeNode *root) {
    rightRotate(root->right);
    leftRotate(root);
}

void SplayTree::splay(SplayTreeNode *cur, SplayTreeNode *target) {
    SplayTreeNode* parent = cur->parent;
    while (parent != target) {
        if (parent == nullptr) {
            break;
        }
        SplayTreeNode* grandParent = parent->parent;
        if (grandParent == target) {
            if (cur == parent->left) {
                rightRotate(parent);
            } else {
                leftRotate(parent);
            }
            break;
        }
        if(cur == parent->left) {
            if(grandParent == nullptr) {
                rightRotate(parent);
            } else if(parent == grandParent->left) {
                rightRotate(parent);
                rightRotate(grandParent);
            } else {
                rightLeftRotate(grandParent);
            }
        } else{
            if(grandParent == nullptr) {
                leftRotate(parent);
            } else if(parent ==  grandParent->left) {
                leftRightRotate(grandParent);
            } else {
                leftRotate(parent);
                leftRotate(grandParent);
            }
        }
        parent = cur->parent;
    }
    if (target == nullptr) {
        root = cur;
    }
}

bool SplayTree::removeByScore(int score) {
    SplayTreeNode* target = getNodeByScore(score);
    return remove(target);
}

void SplayTree::deleteTree(SplayTreeNode *root) {
    if(root == nullptr) {
        return;
    }
    SplayTreeNode* left = root->left;
    SplayTreeNode* right = root->right;
    delete root;
    deleteTree(left);
    deleteTree(right);
}

SplayTreeNode *SplayTree::getSuccessor(SplayTreeNode *root) {
    SplayTreeNode* cur = root->right;
    if(cur== nullptr) {
        SplayTreeNode *parent = root->parent;
        SplayTreeNode* ch = root;
        while (parent!= nullptr&&ch == parent->right) {
            ch = parent;
            parent = parent->parent;
        }
        return parent;
    }
    while (cur->left!= nullptr) {
        cur = cur->left;
    }

    return cur;
}

bool SplayTree::removeByRank(int rank) {
    SplayTreeNode *target = getNodeByRank(rank);
    return remove(target);
}

bool SplayTree::remove(SplayTreeNode *target) {
    if(target == nullptr||target == head||target == tail) {
        return false;
    }
    modified();
    if(target->left == nullptr&&target->right == nullptr) {
        if (target == root) {
            root == nullptr;
        }
        sizeUpdate(target,-1);
        delete target;
        return true;
    }
    else if(target->left == nullptr&&target->right!= nullptr) {
        sizeUpdate(target,-1);
        SplayTreeNode* parent = target->parent;
        if(parent!= nullptr) {
            SplayTreeNode* right = target->right;
            delete target;
            parent->right = right;
            right->parent = parent;
        } else {
            root = target->right;
            delete target;
        }
        return true;
    }
    else if(target->left != nullptr&&target->right== nullptr) {
        sizeUpdate(target,-1);
        SplayTreeNode* parent = target->parent;
        if(parent!= nullptr) {
            SplayTreeNode* left = target->left;
            delete target;
            parent->left = left;
            left->parent = parent;
        } else {
            root = target->left;
            delete target;
        }
        return true;
    } else{
        SplayTreeNode* successor = getSuccessor(target);
        if(successor == tail) {
            SplayTreeNode* predecessor = getPredecessor(target);
            if (predecessor == head) {
                root = head;
                head->right = tail;
                tail->parent = head;
                delete target;
                return true;
            }
            delete target->data;
            target->data = predecessor->data;
            predecessor->data = nullptr;
            remove(predecessor);
            return true;
        }
        delete target->data;
        target->data = successor->data;
        successor->data = nullptr;
        remove(successor);
        return true;
    }
}

SplayTreeNode *SplayTree::getPredecessor(SplayTreeNode *root) {
    SplayTreeNode* cur = root->left;
    if(cur== nullptr) {
        SplayTreeNode *parent = root->parent;
        SplayTreeNode* ch = root;
        while (parent != nullptr&&ch == parent->left) {
            ch = parent;
            parent = parent->parent;
        }
        return parent;
    }
    while (cur->right!= nullptr) {
        cur = cur->right;
    }

    return cur;
}

void SplayTree::removeRangeByRank(int start, int end) {
    if (start == end) {
        return;
    }
    modified();
    SplayTreeNode *from = getNodeByRank(start - 1);
    SplayTreeNode *to = getNodeByRank(end);
    splay(from, nullptr);
    splay(to, from);
    sizeUpdate(root->right,-getSize(root->right->left));
    deleteTree(root->right->left);
    root->right->left = nullptr;
}

void SplayTree::getRangeByRank(int start, int end, vector<MondisObject*> *res) {
    if (start == end) {
        return;
    } else if (start + 1 == end) {
        SplayTreeNode *from = getNodeByRank(start);
        res->push_back(from->data);
        return;
    }
    SplayTreeNode *from = getNodeByRank(start - 1);
    SplayTreeNode *to = getNodeByRank(end);
    splay(from, nullptr);
    splay(to, from);
    inOrderTraversal(root->right->left,res);
}

void SplayTree::inOrderTraversal(SplayTreeNode *root, vector<MondisObject *> *res) {
    if(root == nullptr||root==head||root == tail) {
        return;
    }
    inOrderTraversal(root->left,res);
    res->push_back(root->data);
    inOrderTraversal(root->right,res);
}

void SplayTree::removeRangeByScore(int startScore, int endScore) {
    if(startScore == endScore) {
        return;
    }
    modified();
    SplayTreeNode *from = getLowerBound(startScore, false);
    SplayTreeNode *to = getLowerBound(endScore, true);
    if(from == tail||to == head) {
        return;
    }
    if (from == to) {
        remove(from);
        return;;
    }
    if (from->score < to->score) {
        return;
    }
    splay(from, nullptr);
    splay(to, from);
    sizeUpdate(root->right,-getSize(root->right->left));
    deleteTree(root->right->left);
    root->right->left = nullptr;
}

void SplayTree::getRangeByScore(int startScore, int endScore, vector<MondisObject *> *res) {
    if(startScore == endScore) {
        return;
    }
    SplayTreeNode *from = getLowerBound(startScore, false);
    SplayTreeNode *to = getUpperBound(endScore, true);
    if(from == tail||to == head) {
        return;
    }
    if (from == to) {
        res->push_back(from->data);
        return;;
    }
    splay(from, nullptr);
    splay(to, from);
    inOrderTraversal(root->right->left,res);

}

//大于score的最小元素
SplayTreeNode *SplayTree::getUpperBound(int score, bool canEqual) {
    SplayTreeNode* cur = root;
    while (true) {
        if(cur->score == score) {
            if (canEqual) {
                return cur;
            }
            return getSuccessor(cur);
        } else if(cur->score > score) {
            if(cur->left == nullptr) {
                return cur;
            }
            cur = cur->left;
        } else{
            if(cur->right == nullptr) {
                return getSuccessor(cur);
            }
            cur = cur->right;
        }
    }
}

//不大于score的最大元素
SplayTreeNode *SplayTree::getLowerBound(int score, bool canEqual) {
    SplayTreeNode* cur = root;
    while (true) {
        if(cur->score == score) {
            if (canEqual) {
                return cur;
            }
            return getPredecessor(cur);
        } else if(cur->score < score) {
            if(cur->right == nullptr) {
                return cur;
            }
            cur = cur->right;
        } else{
            if(cur->left == nullptr) {
                return getPredecessor(cur);
            }
            cur = cur->left;
        }
    }
}

ExecutionResult SplayTree::execute(Command *command) {
    ExecutionResult res;
    switch (command->type) {
        case ADD: {
            CHECK_PARAM_NUM(2);
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, socre)
            CHECK_PARAM_TYPE(1, STRING)
            MondisObject *data = MondisServer::getJSONParser()->parseObject((*command)[1].content);
            insert(socre, data);
            OK_AND_RETURN
        }
        case REMOVE_BY_RANK: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, rank)
            if (rank < 1) {
                res.res = "rank out of range";
                LOGIC_ERROR_AND_RETURN
            }
            if (rank > size()) {
                res.res = "rank out of range";
                LOGIC_ERROR_AND_RETURN
            }
            removeByRank(rank);
            OK_AND_RETURN
        }
        case REMOVE_BY_SCORE: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, score)
            removeByScore(score);
            OK_AND_RETURN
        }
        case REMOVE_RANGE_BY_RANK: {
            if (command->params.size() == 1) {
                CHECK_PARAM_TYPE(0, PLAIN)
                CHECK_START_AND_DEFINE(0)
                removeRangeByRank(start, size() + 1);
                OK_AND_RETURN
            } else if (command->params.size() == 2) {
                CHECK_PARAM_TYPE(0, PLAIN)
                CHECK_PARAM_TYPE(1, PLAIN)
                CHECK_START_AND_DEFINE(0)
                CHECK_END_AND_DEFINE(1, size())
                removeRangeByRank(start, end);
                OK_AND_RETURN
            } else {
                res.type = SYNTAX_ERROR;
                res.res = "arguments num error";
                return res;
            }
        }
        case REMOVE_RANGE_BY_SCORE: {
            if (command->params.size() == 1) {
                CHECK_PARAM_TYPE(0, PLAIN)
                CHECK_AND_DEFINE_INT_LEGAL(0, start)
                removeRangeByScore(start, numeric_limits<int>::max());
                OK_AND_RETURN
            } else if (command->params.size() == 2) {
                CHECK_PARAM_TYPE(0, PLAIN)
                CHECK_PARAM_TYPE(1, PLAIN)
                CHECK_AND_DEFINE_INT_LEGAL(0, start)
                CHECK_AND_DEFINE_INT_LEGAL(1, end)
                removeRangeByScore(start, end);
                OK_AND_RETURN
            } else {
                res.type = SYNTAX_ERROR;
                res.res = "arguments num error";
                return res;
            }
        }
        case GET_BY_RANK: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, rank)
            if (rank < 1) {
                res.res = "rank out of range";
                LOGIC_ERROR_AND_RETURN
            }
            if (rank > size()) {
                res.res = "rank out of range";
                LOGIC_ERROR_AND_RETURN
            }
            res.res = getByRank(rank)->getJson();
            OK_AND_RETURN
        }
        case GET_BY_SCORE: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, score)
            MondisObject *data = getByScore(score);
            if(data== nullptr) {
                res.res = string("there is no object whose score is ") + to_string(score);
                LOGIC_ERROR_AND_RETURN
            }
            res.res = data->getJson();
            OK_AND_RETURN
        }
        case GET_RANGE_BY_RANK: {
            if (command->params.size() == 1) {
                CHECK_PARAM_TYPE(0, PLAIN)
                CHECK_START_AND_DEFINE(0)
                vector<MondisObject *> v;
                getRangeByRank(start, size() + 1, &v);
                res.res += "[\n";
                for (MondisObject *every:v) {
                    res.res += every->getJson();
                    res.res += ",\n";
                }
                res.res += "]\n";
                OK_AND_RETURN
            } else if (command->params.size() == 2) {
                CHECK_PARAM_TYPE(0, PLAIN)
                CHECK_PARAM_TYPE(1, PLAIN)
                CHECK_START_AND_DEFINE(0)
                CHECK_END_AND_DEFINE(1, size())
                vector<MondisObject *> v;
                getRangeByRank(start, end, &v);
                res.res += "[\n";
                for (MondisObject *every:v) {
                    res.res += every->getJson();
                    res.res += ",\n";
                }
                res.res += "]\n";
                OK_AND_RETURN
            } else {
                res.type = SYNTAX_ERROR;
                res.res = "arguments num error";
                return res;
            }
        }
        case GET_RANGE_BY_SCORE:{
            if (command->params.size() == 1) {
                CHECK_PARAM_TYPE(0, PLAIN)
                CHECK_START_AND_DEFINE(0)
                vector<MondisObject *> v;
                getRangeByScore(start, numeric_limits<int>::max(), &v);
                res.res += "[\n";
                for (MondisObject *every:v) {
                    res.res += every->getJson();
                    res.res += ",\n";
                }
                res.res += "]\n";
                OK_AND_RETURN
            } else if (command->params.size() == 2) {
                CHECK_PARAM_TYPE(0, PLAIN)
                CHECK_PARAM_TYPE(1, PLAIN)
                CHECK_START_AND_DEFINE(0)
                CHECK_END_AND_DEFINE(1, size())
                vector<MondisObject *> v;
                getRangeByScore(start, end, &v);
                res.res += "[\n";
                for (MondisObject *every:v) {
                    res.res += every->getJson();
                    res.res += ",\n";
                }
                res.res += "]\n";
                OK_AND_RETURN
            } else {
                res.type = SYNTAX_ERROR;
                res.res = "arguments num error";
                return res;
            }
        }
        case EXISTS: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, score)
            res.res = util::to_string(contains(score));
            OK_AND_RETURN
        }
        case M_SIZE: {
            CHECK_PARAM_NUM(0)
            res.res = to_string(size());
            OK_AND_RETURN
        }
        case COUNT: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, start)
            CHECK_AND_DEFINE_INT_LEGAL(1, end)
            res.res = to_string(count(start, end));
            OK_AND_RETURN
        }
        case CHANGE_SCORE: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, oldScore)
            CHECK_AND_DEFINE_INT_LEGAL(1, newScore)
            SplayTreeNode *oldNode = getNodeByScore(oldScore);
            if (oldNode == nullptr) {
                res.res = string("element whose score is ") + PARAM(0) + " does not exists";
                LOGIC_ERROR_AND_RETURN
            }
            SplayTreeNode *newNode = getNodeByScore(newScore);
            if (newNode != nullptr) {
                res.res = string("element whose score is ") + PARAM(1) + " has existed";
                LOGIC_ERROR_AND_RETURN
            }
            MondisObject *data = oldNode->data;
            oldNode->data = nullptr;
            removeByScore(oldScore);
            insert(newScore, data);
            OK_AND_RETURN
        }
        case RANK_TO_SCORE: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, rank)
            if (rank < 1 || rank > size()) {
                res.res = "rank is out of range!";
                LOGIC_ERROR_AND_RETURN
            }
            SplayTreeNode *node = getNodeByRank(rank);
            res.res = to_string(node->score);
            OK_AND_RETURN
        }
        case SCORE_TO_RANK: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, score)
            res.res = to_string(count(numeric_limits<int>::min(), score) + 1);
            OK_AND_RETURN
        }
    }
    INVALID_AND_RETURN
}

MondisObject *SplayTree::locate(Command *command) {
    MondisObject* res;
    if (command->params.size() != 2) {
        return nullptr;
    }
    if ((*command)[0].type != Command::ParamType::PLAIN) {
        return nullptr;
    }
    if ((*command)[1].content == "RANK") {
        int rank;
        if (!util::toInteger((*command)[1].content, rank)) {
            return nullptr;
        }
        if(rank<1||rank>size()) {
            return nullptr;
        }
        return getByRank(rank);
    }
    if ((*command)[1].content == "SCORE") {
        int score;
        if (!util::toInteger((*command)[1].content, score)) {
            return nullptr;
        }
        return getByScore(score);
    }

    return nullptr;
}

SplayTreeNode *SplayTree::getNodeByRank(int rank) {
    return getNodeByRank(root, rank);
}


SplayTree::SplayIterator::SplayIterator(SplayTree *avlTree) : tree(avlTree), cur(avlTree->root) {
    dfs(cur);
}

bool SplayTree::SplayIterator::next() {
    if (s.empty()) {
        return false;
    }
    cur = s.top();
    s.pop();
    while (true) {
        if (cur == tree->head) {
            dfs(cur->right);
            if (s.empty()) {
                return false;
            }
            cur = s.top();
            s.pop();
            continue;
        } else if (cur == tree->tail) {
            if (s.empty()) {
                return false;
            }
            cur = s.top();
            s.pop();
            break;
        }
        break;
    }
    dfs(cur->right);
    return true;
}

SplayTreeNode *SplayTree::SplayIterator::operator->() {
    return cur;
}

void SplayTree::SplayIterator::dfs(SplayTreeNode *cur) {
    while (cur != nullptr) {
        s.push(cur);
        cur = cur->left;
    }
}
