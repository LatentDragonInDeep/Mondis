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

enum TokenType {
    LEFT_SQAURE_BRACKET,
    RIGHT_SQUARE_BRACKET,
    LEFT_ANGLE_BRACKET,
    RIGHT_ANGLE_BRACKET,
    STRING,
    COMMA,
    COLON,
    TERMINATOR
};

class Token {
public:
    TokenType type;
    std::string content;
    Token(TokenType type):type(type) {

    }
    Token(Token&& other) = default;

    Token(Token &other) = default;

    Token(std::string &data) : type(STRING), content(data) {

    }

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

    static Token* getToken(TokenType type) {
        switch (type) {
            case LEFT_SQAURE_BRACKET:
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
    }
};

class JSONParser {
public:
    class LexicalParser{
    private:
        std::string source;
        unsigned curIndex = 0;
        bool isEnd = false;
        static std::unordered_map<char,Token*> directRecognize;
    public:
        static void init();

    private:
        void skip() {
            while (source[curIndex] == ' ' || source[curIndex] == '\n' || source[curIndex] == '\r' ||
                   source[curIndex] == '\t') {
                curIndex++;
                if (curIndex > source.size()) {
                    isEnd = true;
                    break;
                }
            }
        };
    public:
        LexicalParser(std::string &s) : source(s) {};
        LexicalParser(const char* filePath){
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
        };
        Token nextToken() {
            Token res;
            if(isEnd) {
                return res;
            }
            skip();
            int start;
            int end;
            if (directRecognize.find(source[curIndex]) != directRecognize.end()) {
                res = *directRecognize[source[curIndex]];
                curIndex++;
                return res;
            }
            if (source[curIndex] == '"') {
                start = curIndex;
                while (true) {
                    curIndex++;
                    if (source[curIndex] == '"') {
                        if (source[curIndex - 1] != '\\') {
                            end = curIndex;
                            res.type = STRING;
                            res.content = std::string(source, start, end - start);
                            return res;
                        }
                    }
                }
            }

        }

        void reset() {
            source = "";
            curIndex = 0;
            isEnd = false;
        }

        void setSource(std::string newSrc) {
            source = newSrc;
        }
    };
    Token current;
    JSONParser();
    JSONParser(std::string& filePath);
    void parse(HashMap* keySpace);

    MondisObject *parseObject(std::string &content);

    KeyValue parseEntry(std::string &content);
private:
    LexicalParser* lexicalParser;
    MondisObject* parseObject(LexicalParser* lp);
    MondisObject *parseJSONObject(JSONParser::LexicalParser *lp, bool isNeedNext, bool isInteger);
    MondisObject* parseJSONArray(LexicalParser* lp, bool isNeedNext);

    KeyValue parseEntry(LexicalParser* lp);
    void matchToken(LexicalParser* lp,TokenType type);
    void matchToken(TokenType type);
};


#endif //MONDIS_JSONPARSER_H
