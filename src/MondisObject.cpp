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

ExecRes MondisData::execute(Command *command) {
    return ExecRes();
}

MondisObject *MondisData::locate(Command *command) {
    return nullptr;
}

bool MondisData::hasModified() {
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
            delete reinterpret_cast<string *>(objData);
            break;
        case MondisObjectType::RAW_INT:
            delete reinterpret_cast<int *>(objData);
            break;
        case LIST:
        case SET:
        case ZSET:
        case HASH:
            MondisData *data = (MondisData *) (objData);
            delete data;
            break;
    }
}

MondisObject *MondisObject::locate(Command *command) {
    if (type == RAW_INT || type == RAW_STRING) {
        return nullptr;
    }
    MondisData *data = (MondisData *) objData;
    return data->locate(command);
}

ExecRes MondisObject::executeString(Command *command) {
    string *data = (string *) objData;
    ExecRes res;
    switch (command->type) {
        case BIND: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, STRING)
            CHECK_AND_DEFINE_INT_LEGAL(0, index)
            if ((*command)[1].content.size() != 1) {
                res.desc = "error data";
            }
            (*data)[index] = (*command)[1].content[0];
            modified();
            OK_AND_RETURN
        }
        case GET: {
            CHECK_PARAM_NUM(1)
            CHECK_AND_DEFINE_INT_LEGAL(0, index)
            if ((*command)[1].content.size() != 1) {
                res.desc = "error data";
            }
            res.desc = data->substr(index, 1);
            OK_AND_RETURN
        }
        case SET_RANGE: {
            if (command->params.size() == 2) {
                CHECK_PARAM_TYPE(0, PLAIN)
                CHECK_PARAM_TYPE(1, STRING)
                CHECK_START_AND_DEFINE(0)
                if ((*command)[2].content.size() < data->size() - start) {
                    res.desc = "data length too short!";
                    return res;
                }
                for (int i = start; i < data->size(); ++i) {
                    (*data)[i] = (*command)[2].content[i - start];
                }
                modified();
                OK_AND_RETURN
            } else if (command->params.size() == 3) {
                CHECK_START_AND_DEFINE(0)
                CHECK_END_AND_DEFINE(1, data->size())
                if ((*command)[2].content.size() < end - start) {
                    res.desc = "data length too short!";
                    return res;
                }
                for (int i = start; i < end; ++i) {
                    (*data)[i] = (*command)[2].content[i - start];
                }
                modified();
                OK_AND_RETURN
            } else {
                res.desc = "arguments num error!";
                LOGIC_ERROR_AND_RETURN
            }
        }
        case GET_RANGE: {
            if (command->params.size() == 1) {
                CHECK_START_AND_DEFINE(0)
                res.desc = data->substr(start, data->size() - start);
                OK_AND_RETURN
            }
            if (command->params.size() == 2) {
                CHECK_START_AND_DEFINE(0)
                CHECK_END_AND_DEFINE(1, data->size())
                res.desc = data->substr(start, end - start);
                OK_AND_RETURN
            } else {
                res.desc = "arguments num error!";
                LOGIC_ERROR_AND_RETURN
            }
        }
        case STRLEN: {
            CHECK_PARAM_NUM(0)
            res.desc = std::to_string(data->size());

            OK_AND_RETURN
        }
        case INSERT: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, STRING)
            CHECK_AND_DEFINE_INT_LEGAL(0, index);
            if (index < 0 || index > data->size()) {
                res.desc = "index out of range";
                LOGIC_ERROR_AND_RETURN
            }
            data->insert(index, PARAM(1));
            modified();
            OK_AND_RETURN
        }
        case TO_INTEGER: {
            long long *newData = new long long;
            bool success = util::toInteger(*data, *newData);
            if (success) {
                delete (string *) objData;
                objData = newData;
                OK_AND_RETURN
            }
            res.desc = "can not transform to integer";
            LOGIC_ERROR_AND_RETURN
        }
        case REMOVE_RANGE: {
            if (command->params.size() == 1) {
                CHECK_PARAM_TYPE(0, PLAIN)
                CHECK_AND_DEFINE_INT_LEGAL(0, start);
                if (start < 0) {
                    res.desc = "start under zero!";
                    LOGIC_ERROR_AND_RETURN
                }
                if (start > data->size()) {
                    res.desc = "start over the size of data!";
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
                res.desc = "start under zero!";
                LOGIC_ERROR_AND_RETURN
            }
            if (start > data->size()) {
                res.desc = "start over the size of data!";
                LOGIC_ERROR_AND_RETURN
            }
            if (end < start) {
                res.desc = "the end is smaller than start!";
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
    res.desc = "Invalid command";

    return res;
}

ExecRes MondisObject::executeInteger(Command *command) {
    long long *data = (long long *) objData;
    ExecRes res;
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
            objData = str;
            modified();
            OK_AND_RETURN
        }
    }
    INVALID_AND_RETURN
}

ExecRes MondisObject::execute(Command *command) {
    if (command->type == TYPE) {
        ExecRes res;
        res.type = OK;
        res.desc = typeStrs[type];
        return res;
    }
    if (type == RAW_STRING) {
        return executeString(command);
    } else if (type == RAW_INT) {
        return executeInteger(command);
    } else {
        MondisData *data = (MondisData *) objData;
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
            handleEscapeChar(*(string *) objData);
            json += "\"";
            json += *(string *) objData;
            json += "\"";
            break;
        case MondisObjectType::RAW_INT:
            json += std::to_string(*((long long *) objData));
            break;
        case LIST:
        case SET:
        case ZSET:
        case HASH:
            MondisData *data = static_cast<MondisData *>(objData);
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

string MondisObject::typeStrs[] = {"RAW_STRING", "RAW_INT", "LIST", "SET", "ZSET", "HASH", "EMPTY"};
MondisObject *MondisObject::nullObj = new MondisObject;

bool MondisObject::modified() {
    hasSerialized = false;
}
