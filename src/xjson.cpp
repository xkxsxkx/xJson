/*copyright 2021 xkxsxkx*/
#include "xjson.h"
#include <assert.h>
#include <stdlib.h>
#include <iostream>

using xJson::xValue;
using xJson::xState;
using xJson::xType;
using xJson::xHelper;
using xJson::xMember;

#ifndef X_PARSE_STACK_INIT_SIZE
#define X_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch) do { assert(*c->json == (ch)); c->json++;} while (0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

/** @fn void xFree(xValue* v)
 * @brief 
 * @param v 
 */
static void xFree(xValue* v) {
    size_t i;
    assert(v != nullptr);
    switch (v->type) {
        case xType::X_TYPE_STRING:
            free(v->str.s);
            break;
        case xType::X_TYPE_ARRAY:
            for (i = 0; i < v->array.len; i++)
                xFree(&v->array.e[i]);
            free(v->array.e);
            break;
        case xType::X_TYPE_OBJECT:
            for (i = 0; i < v->object.size; i++) {
                free(v->object.m[i].k);
                xFree(&v->object.m[i].v);
            }
            free(v->object.m);
            break;
        default: break;
    }
    v->type = xType::X_TYPE_NULL;
}

// #define xSetNull(v) xFree(v)
#define xInit(v) do { (v)->type = xType::X_TYPE_NULL; } while (0)

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
} xContext;

/**
 * @brief expend memory.
 * @param c context which need to expand
 * @param size extend value
 * @return void* 
 */
void* xContextPush(xContext* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = X_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1;
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}
/**
 * @brief reduce memory
 * @param c context
 * @param size 
 * @return void* 
 */
void* xContextPop(xContext* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

#define PUTC(c, ch) do { *(char*)xContextPush(c, sizeof(char)) = (ch); } while (0)

class xParse {
 public:
    static void parseWhiteSpace(xContext* c) {
        const char *p = c->json;
        while (*p == ' ' || *p == '\t'
            || *p == '\n' || *p == '\r') {
            p++;
        }
        c->json = p;
    }
    static xState parseLiteral(xContext* c, xValue* v,
        const char* literal, xType type) {
        size_t i;
        EXPECT(c, literal[0]);
        for (i = 0; literal[i + 1]; i++)
            if (c->json[i] != literal[i + 1])
                return xState::X_PARSE_INVALID_VALUE;
        c->json += i;
        v->type = type;
        return xState::X_PARSE_OK;
    }
    static xState parseTrue(xContext* c, xValue* v) {
        EXPECT(c, 't');
        if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
            return xState::X_PARSE_INVALID_VALUE;
        c->json += 3;
        v->type = xType::X_TYPE_TRUE;
        return xState::X_PARSE_OK;
    }
    static xState parseFalse(xContext* c, xValue* v) {
        EXPECT(c, 'f');
        if (c->json[0] != 'a' || c->json[1] != 'l'
            || c->json[2] != 's' || c->json[3] != 'e')
            return xState::X_PARSE_INVALID_VALUE;
        c->json += 4;
        v->type = xType::X_TYPE_FALSE;
        return xState::X_PARSE_OK;
    }
    static xState parseNull(xContext* c, xValue* v) {
        EXPECT(c, 'n');
        if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
            return xState::X_PARSE_INVALID_VALUE;
        c->json += 3;
        v->type = xType::X_TYPE_NULL;
        return xState::X_PARSE_OK;
    }
    static xState parseNumber(xContext* c, xValue* v) {
        const char* p = c->json;
        if (*p == '-') p++;
        if (*p == '0') p++;
        else {
            if (!ISDIGIT1TO9(*p))
                return xState::X_PARSE_INVALID_VALUE;
            for (p++; ISDIGIT(*p); p++) {}
        }
        if (*p == '.') {
            p++;
            if (!ISDIGIT(*p))
                return xState::X_PARSE_INVALID_VALUE;
            for (p++; ISDIGIT(*p); p++) {}
        }
        if (*p == 'e' || *p == 'E') {
            p++;
            if (*p == '+' || *p == '-') p++;
            if (!ISDIGIT(*p)) return xState::X_PARSE_INVALID_VALUE;
            for (p++; ISDIGIT(*p); p++) {}
        }
        errno = 0;
        char* end;
        v->n = strtod(c->json, &end);
        if (errno == ERANGE && (v->n == HUGE_VAL
            || v->n == -HUGE_VAL))
            return xState::X_PARSE_NUMBER_TOO_BIG;
        c->json = p;
        v->type = xType::X_TYPE_NUMBER;
        return xState::X_PARSE_OK;
    }
    static const char* parseHex4(const char* p, unsigned* u) {
        int i;
        *u = 0;
        for (i = 0; i < 4; i++) {
            char ch = *p++;
            *u <<= 4;
            if (ch >= '0' && ch <= '9') *u |= ch - '0';
            else if (ch >= 'A' && ch <= 'F') *u |= ch - ('A' - 10);
            else if (ch >= 'a' && ch <= 'f') *u |= ch - ('a' - 10);
            else return nullptr;
        }
        return p;
    }
    static void encodeUtf8(xContext* c, unsigned u) {
        if (u <= 0x7F)
            PUTC(c, u & 0xFF);
        else if (u <= 0x7FF) {
            PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
            PUTC(c, 0x80 | (u & 0x3F));
        } else if (u <= 0xFFFF) {
            PUTC(c, 0xE0 | (u >> 12) & 0xFF);
            PUTC(c, 0x80 | (u >> 6) & 0x3F);
            PUTC(c, 0x80 | (u & 0x3F));
        } else {
            assert(u <= 0x10FFFF);
            PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
            PUTC(c, 0x80 | ((u >> 12) & 0x3F));
            PUTC(c, 0x80 | ((u >> 6) & 0x3F));
            PUTC(c, 0x80 | (u & 0x3F));
        }
    }
    #define STRING_ERROR(ret) do {c->top = head; return ret; } while (0)
    static xState parseStringRaw(xContext* c, char** str, size_t* len) {
        size_t head = c->top;
        unsigned u, u2;
        const char* p;
        EXPECT(c, '\"');
        p = c->json;
        for (;;) {
            char ch = *p++;
            switch (ch) {
            case '\"':
                *len = c->top - head;
                *str = (char*)xContextPop(c, *len);
                /*
                xHelper::xSetString(v,
                    (const char*)xContextPop(c, len), len);
                */
                c->json = p;
                return xState::X_PARSE_OK;
            case '\\':
                switch (*p++) {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/': PUTC(c, '/'); break;
                    case 'b': PUTC(c, '\b'); break;
                    case 'f': PUTC(c, '\f'); break;
                    case 'n': PUTC(c, '\n'); break;
                    case 'r': PUTC(c, '\r'); break;
                    case 't': PUTC(c, '\t'); break;
                    case 'u':
                        if (!(p = xParse::parseHex4(p, &u)))
                            STRING_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX);
                        if (u >= 0xD800 && u <= 0xDBFF) {
                            if (*p++ != '\\')
                                STRING_ERROR(
                                    xState::X_PARSE_INVALID_UNICODE_SURROGATE);
                            if (*p++ != 'u')
                                STRING_ERROR(
                                    xState::X_PARSE_INVALID_UNICODE_SURROGATE);
                            if (!(p = xParse::parseHex4(p, &u2)))
                                STRING_ERROR(
                                    xState::X_PARSE_INVALID_UNICODE_HEX);
                            if (u2 < 0xDC00 || u2 > 0xDFFF)
                                STRING_ERROR(
                                    xState::X_PARSE_INVALID_UNICODE_SURROGATE);
                            u = (((u - 0xD800) << 10) | (u2 - 0xDC00))
                                + 0x10000;
                        }
                        xParse::encodeUtf8(c, u);
                        break;
                    default:
                        STRING_ERROR(xState::X_PARSE_INVALID_STRING_ESCAPE);
                }
                break;
            case '\0':
                STRING_ERROR(xState::X_PARSE_MISS_QUOTATION_MARK);
            default:
                if ((unsigned char)ch < 0x20) {
                    STRING_ERROR(xState::X_PARSE_INVALID_STRING_CHAR);
                }
                PUTC(c, ch);
            }
        }
    }
    static xState parseString(xContext* c, xValue* v) {
        xState ret;
        char* s;
        size_t len;
        if ((ret = parseStringRaw(c, &s, &len)) == xState::X_PARSE_OK)
            xHelper::xSetString(v, s, len);
        return ret;
    }
    static xState parseArray(xContext* c, xValue* v) {
        size_t i, size = 0;
        xState ret;
        EXPECT(c, '[');
        parseWhiteSpace(c);
        if (*c->json == ']') {
            c->json++;
            v->type = xType::X_TYPE_ARRAY;
            v->array.len = 0;
            v->array.e = nullptr;
            return xState::X_PARSE_OK;
        }
        for (;;) {
            xValue e;
            xInit(&e);
            if ((ret = parseValue(&e, c)) != xState::X_PARSE_OK)
                break;
            memcpy(xContextPush(c, sizeof(xValue)), &e, sizeof(xValue));
            size++;
            parseWhiteSpace(c);
            if (*c->json == ',') {
                c->json++;
                parseWhiteSpace(c);
            } else if (*c->json == ']') {
                c->json++;
                v->type == xType::X_TYPE_ARRAY;
                v->array.len = size;
                size *= sizeof(xValue);
                memcpy(v->array.e = (xValue*)malloc(size),
                    xContextPop(c, size), size);
                return xState::X_PARSE_OK;
            } else {
                ret = xState::X_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
                break;
            }
        }
        for (i = 0; i < size; i++)
            xFree((xValue*)xContextPop(c, sizeof(xValue)));
        return ret;
    }
    static xState parseObject(xContext* c, xValue* v) {
        size_t i, size;
        xMember m;
        xState ret;
        EXPECT(c, '{');
        parseWhiteSpace(c);
        if (*c->json == '}') {
            c->json++;
            v->type = xType::X_TYPE_OBJECT;
            v->object.m = 0;
            v->object.size = 0;
            return xState::X_PARSE_OK;
        }
        m.k = nullptr;
        size = 0;
        for (;;) {
            char* str;
            xInit(&m.v);
            if (*c->json != '"') {
                ret = xState::X_PARSE_MISS_KEY;
                break;
            }
            if ((ret = xParse::parseStringRaw(c, &str, &m.klen))
                != xState::X_PARSE_OK)
                break;
            memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen);
            m.k[m.klen] = '\0';
            parseWhiteSpace(c);
            if (*c->json != ':') {
                ret = xState::X_PARSE_MISS_COLON;
                break;
            }
            c->json++;
            parseWhiteSpace(c);
            if ((ret = parseValue(&m.v, c)) != xState::X_PARSE_OK)
                break;
            memcpy(xContextPush(c, sizeof(xMember)), &m, sizeof(xMember));
            size++;
            m.k = nullptr;
            parseWhiteSpace(c);
            if (*c->json == ',') {
                c->json++;
                parseWhiteSpace(c);
            } else if (*c->json == '}') {
                size_t s = sizeof(xMember) * size;
                c->json++;
                v->type = xType::X_TYPE_OBJECT;
                v->object.size = size;
                memcpy(v->object.m = (xMember*)malloc(s),
                    xContextPop(c, s), s);
                return xState::X_PARSE_OK;
            } else {
                ret = xState::X_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
                break;
            }
        }
        free(m.k);
        for (i = 0; i < size; i++) {
            xMember* m = (xMember*)xContextPop(c, sizeof(xMember));
            free(m->k);
            xFree(&m->v);
        }
        v->type = xType::X_TYPE_NULL;
        return ret;
    }
    static xState parseValue(xValue* v, xContext* c) {
        switch (*c->json) {
            case 't': return xParse::parseLiteral(c, v,
                "true", xType::X_TYPE_TRUE);
            case 'f': return xParse::parseLiteral(c, v,
                "false", xType::X_TYPE_FALSE);
            case 'n': return xParse::parseLiteral(c, v,
                "null", xType::X_TYPE_NULL);
            default: return xParse::parseNumber(c, v);
            case '"': return xParse::parseString(c, v);
            case '[': return xParse::parseArray(c, v);
            case '{': return xParse::parseObject(c, v);
            case '\0': return xState::X_PARSE_EXPECT_VALUE;
        }
    }
};

xState xJson::xParse(xValue* v, const char* json) {
    xContext c;
    xState ret;
    assert(v != nullptr);
    c.json = json;
    c.stack = nullptr;
    v->type = xType::X_TYPE_NULL;
    c.size = c.top = 0;
    xInit(v);
    xParse::parseWhiteSpace(&c);
    if ((ret = xParse::parseValue(v, &c)) == xState::X_PARSE_OK) {
        xParse::parseWhiteSpace(&c);
        if (*c.json != '\0') {
            v->type = xType::X_TYPE_NULL;
            ret = xState::X_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);
    free(c.stack);
    return ret;
}


xHelper::xHelper(xValue* v) {
    this->value = v;
    xInit(this->value);
}

xHelper::~xHelper() {
    xFree(this->value);
}

void xHelper::xSetNull(xValue* v) {
    xFree(v);
}

xType xHelper::xGetType(const xValue* v) {
    assert(v != nullptr);
    return v->type;
}

int xHelper::xGetBoolean(const xValue* v) {
    /* \TODO */
    assert(v != nullptr
        && (v->type == xType::X_TYPE_TRUE
        || v->type == xType::X_TYPE_FALSE));
    return v->type == xType::X_TYPE_TRUE;
}

void xHelper::xSetBoolean(xValue* v, int b) {
    xFree(v);
    v->type = b ? xType::X_TYPE_TRUE : xType::X_TYPE_FALSE;
}

double xHelper::xGetNumber(const xValue* v) {
    assert(v != nullptr && v->type == xType::X_TYPE_NUMBER);
    return v->n;
}

void xHelper::xSetNumber(xValue* v, double n) {
    xFree(v);
    v->n = n;
    v->type = xType::X_TYPE_NUMBER;
}

const char* xHelper::xGetString(const xValue* v) {
    assert(v != nullptr && v->type == xType::X_TYPE_STRING);
    return v->str.s;
}

size_t xHelper::xGetStringLength(const xValue* v) {
    assert(v != nullptr && v->type == xType::X_TYPE_STRING);
    return v->str.len;
}

void xHelper::xSetString(xValue* v, const char* s, size_t len) {
    assert(v != nullptr && (s != nullptr || len == 0));
    xFree(v);
    v->str.s = (char*)malloc(len + 1);
    memcpy(v->str.s, s, len);
    v->str.s[len] = '\0';
    v->str.len = len;
    v->type = xType::X_TYPE_STRING;
}

size_t xHelper::xGetArraySize(const xValue* v) {
    assert(v != nullptr && v->type == xType::X_TYPE_ARRAY);
    return v->array.len;
}

xValue* xHelper::xGetArrayElement(const xValue* v, size_t index) {
    assert(v != nullptr && v->type == xType::X_TYPE_ARRAY);
    assert(index < v->array.len);
    return &v->array.e[index];
}

size_t xHelper::xGetObjectSize(const xValue* v) {
    assert(v != nullptr && v->type == xType::X_TYPE_OBJECT);
    return v->object.size;
}

const char* xHelper::xGetObjectKey(const xValue* v, size_t index) {
    assert(v != nullptr && v->type == xType::X_TYPE_OBJECT);
    assert(index < v->object.size);
    return v->object.m[index].k;
}

size_t xHelper::xGetObjectKeyLength(const xValue* v, size_t index) {
    assert(v != nullptr && v->type == xType::X_TYPE_OBJECT);
    assert(index < v->object.size);
    return v->object.m[index].klen;
}

xValue* xHelper::xGetObjectValue(const xValue* v, size_t index) {
    assert(v != nullptr && v->type == xType::X_TYPE_OBJECT);
    assert(index < v->object.size);
    return &v->object.m[index].v;
}
