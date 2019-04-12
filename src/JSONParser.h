//
// Created by 11956 on 2018/9/3.
//

#ifndef MONDIS_JSONPARSER_H
#define MONDIS_JSONPARSER_H

#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <exception>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "Command.h"

class HashMap;
class MondisObject;
class KeyValue;

namespace util {
    void eraseBackSlash(std::string &data);
};


enum ParserTokenType {
    LEFT_SQUARE_BRACKET,
    RIGHT_SQUARE_BRACKET,
    LEFT_ANGLE_BRACKET,
    RIGHT_ANGLE_BRACKET,
    STRING,
    COMMA,
    COLON,
    TERMINATOR,
};

class Token {
public:
    ParserTokenType type;
    std::string content;

    Token(ParserTokenType type) : type(type) {

    };
    Token(Token&& other) = default;

    Token(Token &other) = default;

    Token(std::string &data) : type(STRING), content(data) {

    };

    Token &operator=(Token &&other) = default;

    Token &operator=(Token &other) = default;
    Token():Token(TERMINATOR){};
    static Token* leftSquareBracket;
    static Token* rightSquareBracket;
    static Token* leftAngleBracket;
    static Token* rightAngleBracket;
    static Token* comma;
    static Token* terminator;
    static Token* colon;

    static Token *getToken(ParserTokenType type) {
        switch (type) {
            case LEFT_SQUARE_BRACKET:
                return leftSquareBracket;
            case RIGHT_SQUARE_BRACKET:
                return rightSquareBracket;
            case LEFT_ANGLE_BRACKET:
                return leftAngleBracket;
            case RIGHT_ANGLE_BRACKET:
                return rightAngleBracket;
            case COMMA:
                return comma;
            case COLON:
                return colon;
            case TERMINATOR:
                return terminator;
        }
    };
};

class JSONParser {
public:
    class LexicalParser{
    private:
        std::string source;
        unsigned curIndex = 0;
        unsigned preIndex = 0;
        bool hasBacked = false;
        bool isEnd = false;
        static std::unordered_map<char,Token*> directRecognize;
    public:
        static void init();

    private:
        void skip();;
    public:
        LexicalParser();

        LexicalParser(const std::string &s);

        LexicalParser(const char *filePath);;

        LexicalParser(LexicalParser &) = default;

        LexicalParser &operator=(LexicalParser &&) = default;
        Token nextToken();

        bool back();

        void reset();

        void setSource(std::string newSrc);
    };
    Token current;
    JSONParser();
    JSONParser(const char *filePath);
    JSONParser(const std::string&);
    void parse(HashMap* keySpace);

    void parseAll(std::vector<HashMap *> &dbs);

    MondisObject *parseObject(std::string &content);

    KeyValue parseEntry(std::string &content);
private:
    LexicalParser* lexicalParser;

    MondisObject *parseObject(LexicalParser *lp);

    MondisObject *parseJSONObject(LexicalParser *lp, bool isNeedNext, bool isInteger);

    MondisObject *parseJSONArray(LexicalParser *lp, bool isNeedNext);

    KeyValue parseEntry(LexicalParser *lp);

    void matchToken(LexicalParser *lp, ParserTokenType type);
};


#endif //MONDIS_JSONPARSER_H
