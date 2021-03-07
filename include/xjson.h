/* copyright 2021 xkxsxkx */
#ifndef __XJSON__H__
#define __XJSON__H__

#include <stddef.h>

namespace xJson {
enum class xType {
    X_TYPE_NULL, X_TYPE_FALSE, X_TYPE_TRUE, X_TYPE_NUMBER,
    X_TYPE_STRING, X_TYPE_ARRAY, X_TYPE_OBJECT
};

enum class xState {
    X_PARSE_OK,
    X_PARSE_EXPECT_VALUE,
    X_PARSE_INVALID_VALUE,
    X_PARSE_ROOT_NOT_SINGULAR,
    X_PARSE_NUMBER_TOO_BIG,
    X_PARSE_MISS_QUOTATION_MARK,
    X_PARSE_INVALID_STRING_ESCAPE,
    X_PARSE_INVALID_STRING_CHAR,
    X_PARSE_INVALID_UNICODE_HEX,
    X_PARSE_INVALID_UNICODE_SURROGATE,
    X_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    X_PARSE_MISS_KEY,
    X_PARSE_MISS_COLON,
    X_PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

typedef struct xMember xMember;
typedef struct xValue xValue;
struct xValue{
    union {
        struct {
            char* s;
            size_t len;
        } str;
        struct {
            xValue* e;
            size_t len;
        } array;
        struct {
            xMember* m;
            size_t size;
        } object;
        double n;
    };
    xType type;
};

struct xMember {
    char* k;
    size_t klen;
    xValue v;
};

/** @fn int xParse(xValue* v, const char* json)
 * @brief parse json to get corresponding value.
 * @param v 
 * @param json json text as c-type string
 * @return xState 
*/
xState xParse(xValue* v, const char* json);

char* xStringify(const xValue* v, size_t* length);

class xHelper {
 private:
    xValue* value;

 public:
    explicit xHelper(xValue* v);
    ~xHelper();
    /**
     * @brief 
     * 
     * @param v 
     */
    static void xSetNull(xValue* v);
    /** @fn xType xGetType(const xValue* v)
     * @brief get type of value.
     * @param v the value which need be judge type.
     * @return xType type of value.
     */
    static xType xGetType(const xValue* v);

    /**
     * @brief 
     * 
     * @param v 
     * @return int 
     */
    static int xGetBoolean(const xValue* v);

    /**
     * @brief 
     * 
     * @param v 
     * @param b 
     */
    static void xSetBoolean(xValue* v, int b);

    /** @fn double xGetNumber(const xValue* v)
     * @brief 
     * 
     * @param v 
     * @return double 
     */
    static double xGetNumber(const xValue* v);

    /**
     * @brief 
     * 
     * @param v 
     * @param n 
     */
    static void xSetNumber(xValue* v, double n);

    static const char* xGetString(const xValue* v);

    static size_t xGetStringLength(const xValue* v);

    static void xSetString(xValue* v, const char* s, size_t len);

    static size_t xGetArraySize(const xValue* v);

    xValue* xGetArrayElement(const xValue* v, size_t index);

    size_t xGetObjectSize(const xValue* v);
    const char* xGetObjectKey(const xValue* v, size_t index);
    size_t xGetObjectKeyLength(const xValue* v, size_t index);
    xValue* xGetObjectValue(const xValue* v, size_t index);
};
}  // namespace xJson

#endif  //!__XJSON__H__