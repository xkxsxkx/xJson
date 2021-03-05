/*copyright 2021 xkxsxkx*/
#include "xjson.h"
#include <assert.h>
#include <stdlib.h>
#include <iostream>

using xJson::xValue;
using xJson::xState;
using xJson::xType;
using xJson::xHelper;

#ifndef X_PARSE_STACK_INIT_SIZE
#define X_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch) do { assert(*c->json == (ch)); c->json++;} while (0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

#define xSetNull(v) xFree(v)

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
    static xState parseString(xContext* c, xValue* v) {
        size_t head = c->top, len;
        const char* p;
        EXPECT(c, '\"');
        p = c->json;
        for (;;) {
            char ch = *p++;
            switch (ch) {
            case '\"':
                len = c->top - head;
                xHelper::xSetString(v,
                    (const char*)xContextPop(c, len), len);
                c->json = p;
                return xState::X_PARSE_OK;
            case '\0':
                c->top = head;
                return xState::X_PARSE_MISS_QUOTATION_MARK;
            default:
                PUTC(c, ch);
            }
        }
    }
    static xState parseValue(xValue* v, xContext* c) {
        switch (*c->json) {
            case 't': return xParse::parseTrue(c, v);
            case 'f': return xParse::parseFalse(c, v);
            case 'n': return xParse::parseNull(c, v);
            default: return xParse::parseNumber(c, v);
            case '"': return xParse::parseString(c, v);
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

void xJson::xFree(xValue* v) {
    assert(v != nullptr);
    if (v->type == xType::X_TYPE_STRING)
        free(v->str.s);
    v->type = xType::X_TYPE_NULL;
}

xHelper::xHelper(xValue* v) {
    this->value = v;
    xInit(this->value);
}

xHelper::~xHelper() {
    xFree(this->value);
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
