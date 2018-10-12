//
// Created by 11956 on 2018/9/10.
//

#include "MondisObject.h"


string MondisData::getJson() {
    if (hasSerialized) {
        return json;
    }
    toJson();
    hasSerialized = true;
    return json;
}

MondisData::~MondisData() {
}

ExecutionResult MondisData::execute(Command *command) {
    return ExecutionResult();
}

MondisObject *MondisData::locate(Command *command) {
    return nullptr;
}

bool MondisData::modified() {
    hasSerialized = false;
}

void handleEscapeChar(string &raw) {
    int modCount = 0;
    for (int i = 0; i < raw.size() + modCount; ++i) {
        if (raw[i] == '"') {
            raw.insert(i, "\\");
            ++i;
            modCount++;
        }
    }
}

MondisObject::~MondisObject() {
    switch (type) {
        case MondisObjectType::RAW_STRING:
            delete reinterpret_cast<string *>(objectData);
            break;
        case MondisObjectType::RAW_INT:
            delete reinterpret_cast<int *>(objectData);
            break;
        case RAW_BIN:
        case LIST:
        case SET:
        case ZSET:
        case HASH:
            MondisData *data = (MondisData *) (objectData);
            delete data;
            break;
    }
}

MondisObject *MondisObject::locate(Command *command) {
    if (type == RAW_INT || type == RAW_STRING || type == RAW_BIN) {
        return nullptr;
    }
    MondisData *data = (MondisData *) objectData;
    return data->locate(command);
}

ExecutionResult MondisObject::executeString(Command *command) {
    string *data = (string *) objectData;
    ExecutionResult res;
    switch (command->type) {
        case BIND: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, STRING)
            CHECK_AND_DEFINE_INT_LEGAL(0, index)
            if ((*command)[1].content.size() != 1) {
                res.res = "error data";
            }
            (*data)[index] = (*command)[1].content[0];
            modified();
            OK_AND_RETURN
        }
        case GET: {
            CHECK_PARAM_NUM(1)
            CHECK_AND_DEFINE_INT_LEGAL(0, index)
            if ((*command)[1].content.size() != 1) {
                res.res = "error data";
            }
            res.res = data->substr(index, 1);
            OK_AND_RETURN
        }
        case SET_RANGE: {
            CHECK_PARAM_NUM(3)
            CHECK_START_AND_DEFINE(0)
            CHECK_END_AND_DEFINE(1, data->size())
            if ((*command)[2].content.size() < end - start) {
                res.res = "data length too short!";
                return res;
            }
            for (int i = start; i < end; ++i) {
                (*data)[i] = (*command)[2].content[i - start];
            }
            modified();
            OK_AND_RETURN
        }
        case GET_RANGE: {
            CHECK_PARAM_NUM(2)
            CHECK_START_AND_DEFINE(0)
            CHECK_END_AND_DEFINE(1, data->size())

            res.res = data->substr(start, end - start);

            OK_AND_RETURN
        }
        case STRLEN: {
            CHECK_PARAM_NUM(0)
            res.res = std::to_string(data->size());

            OK_AND_RETURN
        }
        case APPEND: {
            CHECK_PARAM_NUM(1)
            (*data) += (*command)[0].content;
            modified();
            OK_AND_RETURN
        }
        case TO_INTEGER: {
            int *newData = new int;
            bool success = util::toInteger(*data, *newData);
            if (success) {
                delete (string *) objectData;
                objectData = newData;
                OK_AND_RETURN
            }
            res.res = "can not transform to integer";
            LOGIC_ERROR_AND_RETURN
        }
        case REMOVE_RANGE: {
            if (command->params.size() == 1) {
                CHECK_PARAM_TYPE(0, PLAIN)
                CHECK_AND_DEFINE_INT_LEGAL(0, start);
                if (start < 0) {
                    res.res = "start under zero!";
                    LOGIC_ERROR_AND_RETURN
                }
                if (start > data->size()) {
                    res.res = "start over the size of data!";
                    LOGIC_ERROR_AND_RETURN
                }
                data->erase(data->begin() + start, data->end());
                OK_AND_RETURN
            }
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, start);
            CHECK_AND_DEFINE_INT_LEGAL(1, end);
            if (start < 0) {
                res.res = "start under zero!";
                LOGIC_ERROR_AND_RETURN
            }
            if (start > data->size()) {
                res.res = "start over the size of data!";
                LOGIC_ERROR_AND_RETURN
            }
            if (end < start) {
                res.res = "the end is smaller than start!";
                LOGIC_ERROR_AND_RETURN
            }
            if (end > data->size()) {
                data->erase(data->begin() + start, data->end());
                OK_AND_RETURN
            }
            data->erase(data->begin() + start, data->begin() + end);
            OK_AND_RETURN
        }
    }
    res.res = "Invalid command";

    return res;
}

ExecutionResult MondisObject::executeInteger(Command *command) {
    int *data = (int *) objectData;
    ExecutionResult res;
    switch (command->type) {
        case INCR:
            CHECK_PARAM_NUM(0)
            (*data)++;
            modified();
            OK_AND_RETURN
        case DECR:
            CHECK_PARAM_NUM(0)
            (*data)--;
            modified();
            OK_AND_RETURN
        case INCR_BY: {
            CHECK_PARAM_NUM(1)
            CHECK_AND_DEFINE_INT_LEGAL(0, delta)
            (*data) += delta;
            modified();
            OK_AND_RETURN
        }
        case DECR_BY: {
            CHECK_PARAM_NUM(1)
            CHECK_AND_DEFINE_INT_LEGAL(0, delta)
            (*data) -= delta;
            modified();
            OK_AND_RETURN
        }
        case TO_STRING: {
            string *str = new string(to_string(*data));
            type = RAW_STRING;
            delete data;
            objectData = str;
            modified();
            OK_AND_RETURN
        }
    }
    INVALID_AND_RETURN
}

ExecutionResult MondisObject::execute(Command *command) {
    if (command->type == TYPE) {
        ExecutionResult res;
        res.type = OK;
        res.res = typeStrs[type];
        return res;
    }
    if (type == RAW_STRING) {
        return executeString(command);
    } else if (type == RAW_INT) {
        return executeInteger(command);
    } else {
        MondisData *data = (MondisData *) objectData;
        return data->execute(command);
    }
}

string MondisObject::getJson() {
    if ((type == RAW_STRING || type == RAW_INT) && hasSerialized) {
        return json;
    }
    json = "";
    switch (type) {
        case MondisObjectType::RAW_STRING:
            handleEscapeChar(*(string *) objectData);
            json += "\"";
            json += *(string *) objectData;
            json += "\"";
            break;
        case MondisObjectType::RAW_INT:
            json += ("\"" + std::to_string(*((int *) objectData)) + "\"");
            break;
        case RAW_BIN:
        case LIST:
        case SET:
        case ZSET:
        case HASH:
            MondisData *data = static_cast<MondisData *>(objectData);
            json = data->getJson();
    }

    hasSerialized = true;
    return json;
}

string MondisObject::getTypeStr() {
    return typeStrs[type];
}

MondisObject *MondisObject::getNullObject() {
    return nullObj;
}

string MondisObject::typeStrs[] = {"RAW_STRING", "RAW_INT", "RAW_BIN", "LIST", "SET", "ZSET", "HASH", "EMPTY"};
MondisObject *MondisObject::nullObj = new MondisObject;

bool MondisObject::modified() {
    hasSerialized = false;
}
