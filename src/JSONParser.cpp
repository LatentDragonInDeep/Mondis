//
// Created by 11956 on 2018/9/3.
//

#include <unistd.h>
#include "JSONParser.h"
#include "HashMap.h"
#include "MondisList.h"
#include "SplayTree.h"


JSONParser::JSONParser(std::string &source) {

    lexicalParser = new LexicalParser(source);
}


void JSONParser::parse(HashMap *keySpace) {
    Token next = lexicalParser->nextToken();
    if (next.type != LEFT_ANGLE_BRACKET) {
        return;
    }
    KeyValue cur;
    while(true) {
        cur = parseEntry(lexicalParser);
        if(cur.key == "") {
            break;
        }
        keySpace->put(cur.key,cur.value);
    }
    Token end = lexicalParser->nextToken();
    if (end.type != RIGHT_ANGLE_BRACKET) {
        keySpace->clear();
        throw new std::invalid_argument("unexpected token!");
    }
    Token ter = lexicalParser->nextToken();
    if (ter.type != TERMINATOR) {
        keySpace->clear();
        throw new std::invalid_argument("unexpected token!");
    }
}

KeyValue JSONParser::parseEntry(LexicalParser *lp) {
    Token next = lp->nextToken();
    KeyValue res;
    if (next.type != STRING) {
        lp->back();
        return res;
    }
    res.key = current.content;
    matchToken(lp,COLON);
    res.value = parseObject(lp);
    Token n = lp->nextToken();
    if (n.type == RIGHT_ANGLE_BRACKET) {
        lp->back();
    } else if (n.type != COMMA) {
        throw std::invalid_argument("unexpected token!");
    }

    return res;
}

KeyValue JSONParser::parseEntry(string &content) {
    lexicalParser->reset();
    lexicalParser->setSource(content);
    return parseEntry(lexicalParser);
}

MondisObject *JSONParser::parseObject(string &content) {
    lexicalParser->reset();
    lexicalParser->setSource(content);
    return parseObject(lexicalParser);
}

MondisObject *JSONParser::parseObject(LexicalParser *lp) {
    Token next = lp->nextToken();
    if(next.type == LEFT_ANGLE_BRACKET) {
        return parseJSONObject(lp, false, false);
    } else if (next.type == LEFT_SQUARE_BRACKET) {
        return parseJSONArray(lp, false);
    }
    else if(next.type == STRING) {
        MondisObject *res = new MondisObject;
        long long *data = new long long;
        bool to = util::toInteger(next.content, *data);
        if (!to) {
            res->type = RAW_STRING;
            res->objData = (void *) new string(next.content);
            delete data;
            return res;
        }
        res->type = RAW_INT;
        res->objData = (void *) data;
        return res;
    } else {
        lp->back();
        return nullptr;
    }
}

MondisObject *JSONParser::parseJSONObject(LexicalParser *lp, bool isNeedNext, bool isInteger) {
    if(isNeedNext) {
        Token next = lp->nextToken();
        matchToken(lp,LEFT_ANGLE_BRACKET);
    }
    MondisObject* res = new MondisObject;
    res->type = HASH;
    AVLTree* tree = new AVLTree;
    KeyValue cur;
    while (true) {
        cur = parseEntry(lp);
        if (cur.key == "") {
            break;
        }
        tree->insert(new KeyValue(cur.key, cur.value));
    }
    res->objData = (void *) tree;
    matchToken(lp, RIGHT_ANGLE_BRACKET);
    return res;
}

MondisObject *JSONParser::parseJSONArray(LexicalParser *lp, bool isNeedNext) {
    if(isNeedNext) {
        matchToken(lp, LEFT_SQUARE_BRACKET);
    }
    MondisObject* res = new MondisObject;
    MondisObject *first = parseObject(lp);
    if (first != nullptr) {
        if (first->type == RAW_STRING) {
            if (first->getJson() == "\"LIST\"") {
                list:
                res->type = LIST;
                MondisList *data = new MondisList;
                res->objData = data;
                MondisObject *cur = nullptr;
                while (true) {
                    cur = parseObject(lp);
                    if (cur != nullptr) {
                        data->pushBack(cur);
                    } else {
                        break;
                    }
                }
                delete first;
                matchToken(lp, RIGHT_SQUARE_BRACKET);
                return res;
            } else if (first->getJson() == "\"SET\"") {
                res->type = SET;
                HashMap *data = new HashMap();
                res->objData = data;
                MondisObject *cur = nullptr;
                while (true) {
                    cur = parseObject(lp);
                    if (cur != nullptr) {
                        if (cur->type != RAW_STRING || cur->type != RAW_INT) {
                            throw "unexpected element";
                        }
                        string json = cur->getJson();
                        data->put(json, nullptr);
                    } else {
                        break;
                    }
                }
                delete first;
                matchToken(lp, RIGHT_SQUARE_BRACKET);
                return res;
            } else if (first->getJson() == "\"ZSET\"") {
                res->type = ZSET;
                SplayTree *data = new SplayTree;
                res->objData = data;
                MondisObject *cur = nullptr;
                MondisObject *next = nullptr;
                while (true) {
                    cur = parseObject(lp);
                    next = parseObject(lp);
                    if (cur == nullptr) {
                        break;
                    }
                    if (cur->type != RAW_INT) {
                        throw "unexpected element";
                    }
                    string curScore = cur->getJson();
                    int score = *reinterpret_cast<int *>(curScore.data());
                    delete cur;
                    data->insert(score, next);
                }
                delete first;
                matchToken(lp, RIGHT_SQUARE_BRACKET);
                return res;
            }
        } else {
            goto list;
        }

    } else {
        res->type = LIST;
        res->objData = new MondisList;
        return res;
    }
}

JSONParser::JSONParser() {}

void JSONParser::matchToken(LexicalParser *lp, ParserTokenType type) {
    Token next = lp->nextToken();
    if (next.type != type) {
        throw new std::invalid_argument("unexpected token!");
    }
}

JSONParser::JSONParser(const char *filePath) {
    lexicalParser = new LexicalParser(filePath);
}

void JSONParser::parseAll(std::vector<HashMap *> &dbs) {
    Token next = lexicalParser->nextToken();
    if (next.type != LEFT_ANGLE_BRACKET) {
        return;
    }
    KeyValue cur;
    while (true) {
        cur = parseEntry(lexicalParser);
        if (cur.key == "") {
            break;
        }
        int dbIndex = atoi(cur.key.c_str());
        AVLTree *tree = (AVLTree *) cur.value->objData;
        AVLTree::AVLIterator iter = tree->iterator();
        while (iter.next()) {
            dbs[dbIndex]->put(iter->data->key, iter->data->value);
            tree->remove(iter->data->key);
        }
    }
    Token end = lexicalParser->nextToken();
    if (end.type != RIGHT_ANGLE_BRACKET) {
        throw new std::invalid_argument("unexpected token!");
    }
    Token ter = lexicalParser->nextToken();
    if (ter.type != TERMINATOR) {
        throw new std::invalid_argument("unexpected token!");
    }
}

JSONParser::JSONParser(const std::string &s){
    lexicalParser = new LexicalParser(s);
}

Token *Token::leftSquareBracket = new Token(LEFT_SQUARE_BRACKET);
Token *Token::rightSquareBracket = new Token(RIGHT_SQUARE_BRACKET);
Token *Token::leftAngleBracket = new Token(LEFT_ANGLE_BRACKET);
Token *Token::rightAngleBracket = new Token(RIGHT_ANGLE_BRACKET);
Token *Token::comma = new Token(COMMA);
Token *Token::terminator = new Token(TERMINATOR);
Token *Token::colon = new Token(COLON);

std::unordered_map<char, Token *> JSONParser::LexicalParser::directRecognize = {
        {'[', Token::getToken(LEFT_SQUARE_BRACKET)},
    {']', Token::getToken(RIGHT_SQUARE_BRACKET)},
    {'{', Token::getToken(LEFT_ANGLE_BRACKET)},
    {'}', Token::getToken(RIGHT_ANGLE_BRACKET)},
    {',', Token::getToken(COMMA)},
    {':', Token::getToken(COLON)}
};

void JSONParser::LexicalParser::skip() {
    if (curIndex >= source.size()) {
        isEnd = true;
        return;
    }
    while (source[curIndex]==' ' || source[curIndex]== '\n' || source[curIndex]== '\r' ||
           source[curIndex]== '\t') {
        curIndex++;
        if (curIndex >= source.size()) {
            isEnd = true;
            break;
        }
    }
}

JSONParser::LexicalParser::LexicalParser(std::string &s) : source(s) {}

JSONParser::LexicalParser::LexicalParser() {}

JSONParser::LexicalParser::LexicalParser(const char *filePath) {
    struct stat info;
    int fd = open(filePath, O_RDONLY);
    fstat(fd, &info);

    source.reserve(info.st_size);

    std::ifstream in;
    in.open(filePath);
    std::string line;
    while (std::getline(in, line)) {
        source += line;
        source += "\n";
    }
    close(fd);
    in.close();
}

Token JSONParser::LexicalParser::nextToken() {
    preIndex = curIndex;
    hasBacked = false;
    Token res;
    skip();
    if (isEnd) {
        return res;
    }
    int start;
    int end;
    if (directRecognize.find(source[curIndex]) != directRecognize.end()) {
        res = *directRecognize[source[curIndex]];
        curIndex++;
        return res;
    }
    if (source[curIndex]== '"') {
        start = curIndex;
        while (true) {
            curIndex++;
            if (source[curIndex]== '"') {
                if (source[curIndex - 1] != '\\') {
                    end = curIndex;
                    res.type = STRING;
                    std::string raw(source, start + 1, end - start - 1);
                    util::eraseBackSlash(raw);
                    curIndex++;
                    res.content = raw;
                    return res;
                }
            }
        }
    }

};

bool JSONParser::LexicalParser::back() {
    if (hasBacked) {
        return false;
    }
    curIndex = preIndex;
    hasBacked = true;
    return true;
};

void JSONParser::LexicalParser::reset() {
    source = "";
    curIndex = 0;
    isEnd = false;
};

void JSONParser::LexicalParser::setSource(std::string newSrc) {
    source = newSrc;
};

void util::eraseBackSlash(std::string &data) {
    auto iter = data.begin();
    for (; iter != data.end(); ++iter) {
        if (*iter == '\\') {
            if (*(iter + 1) == '"') {
                iter = data.erase(iter);
            }
        }
    }
};
JSONParser::LexicalParser::LexicalParser(const std::string& s):source(s){
}

