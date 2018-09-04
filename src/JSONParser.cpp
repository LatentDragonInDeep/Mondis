//
// Created by 11956 on 2018/9/3.
//

#include "JSONParser.h"
#include "HashMap.h"

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>

#define src (*source)

JSONParser::JSONParser(std::string &filePath) {

    lexicalParser = new LexicalParser(filePath.c_str());
}

void JSONParser::parse(HashMap *keySpace) {
    matchToken(LEFT_ANGLE_BRACKET);
    pair<Key*,MondisObject*> cur;
    while(true) {
        cur = parseEntry(lexicalParser);
        if(cur.first == nullptr) {
            return;
        }
        keySpace->put(cur.first,cur.second);
    }
}

void JSONParser::matchToken(TokenType type) {
    matchToken(lexicalParser,type);
}

pair<Key*, MondisObject*> JSONParser::parseEntry(LexicalParser* lp) {
    lexicalParser = lp;
    pair<Key*,MondisObject*> res;
    res.first = nullptr;
    Token * next = lp->nextToken();
    if(next->type == COMMA) {
        delete next;
        return res;
    }
    matchToken(STRING);
    Key* key = new Key;
    key->flag = true;
    key->key.str = new string(*current->content);
    res.first = key;
    matchToken(COLON);
    res.second = parseObject(lp);

    return res;
}

pair<Key *, MondisObject *> JSONParser::parseEntry(string &content) {
    LexicalParser parser(content);
    return parseEntry(&parser);
}

MondisObject *JSONParser::parseObject(string &content) {
    LexicalParser parser(content);
    return parseObject(&parser);
}

MondisObject *JSONParser::parseObject(JSONParser::LexicalParser *lp) {
    Token* next = lp->nextToken();
    if(next->type == LEFT_ANGLE_BRACKET) {
        return parseJSONObject(lp, false);
    } else if(next->type == LEFT_SQAURE_BRACKET) {
        return parseJSONArray(lp, false);
    }
    else if(next->type == STRING) {
        MondisObject *res = new MondisObject;
        if (next->content->size() > 11) {
            if (next->content->substr(0, 11) == "LatentDragon") {
                res->type = RAW_BIN;
                MondisBinary *binary = MondisBinary::allocate(next->content->size()-11);
                binary->write(next->content->size()-11,next->content->data());
                res->objectData = (void*)binary;
                return res;
            }
        }

        int *data = new int;
        ss << *next->content;
        ss >> *data;
        if (ss.fail()) {
            res->type = RAW_STRING;
            res->objectData = (void*)new string(*next->content);
            return res;
        }
        res->type = RAW_INT;
        res->objectData = (void*)data;
        ss.clear();
        return res;
    }
    return nullptr;
}

MondisObject *JSONParser::parseJSONObject(JSONParser::LexicalParser *lp, bool isNeedNext) {
    if(isNeedNext) {
        matchToken(lp,LEFT_ANGLE_BRACKET);
    }
    auto first = parseEntry(lp);
    MondisObject* res = new MondisObject;
    if(first.first = nullptr) {
        res->type = HASH;
        res->objectData = (void*)new AVLTree;
        return res;
    }
    if(*first.first->key.str == "InMemoryType") {
        if(*first.first->key.str == "HASH") {
            res->type = HASH;
            AVLTree* tree = new AVLTree;
            matchToken(lp,COMMA);
            pair<Key*,MondisObject*> cur;
            while (true) {
                cur = parseEntry(lp);
                if(cur.first = nullptr) {
                    break;
                }
                tree->insert(*new Entry(cur.first,cur.second));
                cur = parseEntry(lp);
                matchToken(lp,COMMA);
            }
            res->objectData = (void*)tree;
        } else if(*first.first->key.str == "ZSET") {
            res->type = ZSET;
            SkipList* skipList = new SkipList;
            matchToken(lp,COMMA);
            pair<Key*,MondisObject*> cur;
            while (true) {
                cur = parseEntry(lp);
                if(cur.first = nullptr) {
                    break;
                }
                skipList->insert(cur.first,cur.second);
                cur = parseEntry(lp);
                matchToken(lp,COMMA);
            }
            res->objectData = (void*)skipList;
        }
    }

    return res;
}

void JSONParser::matchToken(JSONParser::LexicalParser *lp, TokenType type) {
    if(current->type == STRING) {
        delete current;
    }
    current = lp->nextToken();
    if(current->type != type) {
        throw new invalid_argument("unexpected token!");
    };
}

MondisObject *JSONParser::parseJSONArray(JSONParser::LexicalParser *lp, bool isNeedNext) {
    if(isNeedNext) {
        matchToken(lp,LEFT_SQAURE_BRACKET);
    }

}
