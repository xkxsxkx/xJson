/*copyright 2021 xkxsxkx*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xjson.h"

using namespace xJson;

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actural: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while (0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

static void test_parse_null() {
    xValue v;
    // xInit(&v);
    xHelper xHelper(&v);
    xHelper::xSetBoolean(&v, 0);
    EXPECT_EQ_INT(xState::X_PARSE_OK, xParse(&v, "null"));
    EXPECT_EQ_INT(xType::X_TYPE_NULL, xHelper::xGetType(&v));
    // xFree(&v);
}
static void test_parse_true() {
    xValue v;
    // xInit(&v);
    xHelper helper(&v);
    xHelper::xSetBoolean(&v, 0);
    // v.type = xType::X_TYPE_FALSE;
    EXPECT_EQ_INT(xState::X_PARSE_OK, xParse(&v, "true"));
    EXPECT_EQ_INT(xType::X_TYPE_TRUE, xHelper::xGetType(&v));
    // xFree(&v);
}

static void test_parse_false() {
    xValue v;
    // xInit(&v);
    xHelper helper(&v);
    xHelper::xSetBoolean(&v, 1);
    EXPECT_EQ_INT(xState::X_PARSE_OK, xParse(&v, "false"));
    EXPECT_EQ_INT(xType::X_TYPE_FALSE, xHelper::xGetType(&v));
    // xFree(&v);
}

#define TEST_NUMBER(expect, json)\
    do {\
        xValue v;\
        EXPECT_EQ_INT(xState::X_PARSE_OK, xParse(&v, json));\
        EXPECT_EQ_INT(xType::X_TYPE_NUMBER, xHelper::xGetType(&v));\
        EXPECT_EQ_DOUBLE(expect, xHelper::xGetNumber(&v));\
    } while (0)

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_STRING(expect, json)\
    do {\
        xValue v;\
        xHelper helper(&v);\
        EXPECT_EQ_INT(xState::X_PARSE_OK, xParse(&v, json));\
        EXPECT_EQ_INT(xType::X_TYPE_STRING, xHelper::xGetType(&v));\
        EXPECT_EQ_STRING(expect, xHelper::xGetString(&v),xHelper::xGetStringLength(&v));\
    } while(0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
#if 0
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t",
        "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
#endif
}

#define TEST_ERROR(error, json)\
    do {\
        xValue v;\
        xHelper helper(&v);\
        EXPECT_EQ_INT(error, xParse(&v, json));\
        EXPECT_EQ_INT(xType::X_TYPE_NULL, xHelper::xGetType(&v));\
    } while (0)

static void test_parse_expect_value() {
    TEST_ERROR(xState::X_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(xState::X_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(xState::X_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(xState::X_PARSE_INVALID_VALUE, "?");

#if 0
    /* invalid number*/
    TEST_ERROR(xState::X_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(xState::X_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(xState::X_PARSE_INVALID_VALUE, ".123");
    TEST_ERROR(xState::X_PARSE_INVALID_VALUE, "1.");
    TEST_ERROR(xState::X_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(xState::X_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(xState::X_PARSE_INVALID_VALUE, "?");
    TEST_ERROR(xState::X_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(xState::X_PARSE_INVALID_VALUE, "nan");
#endif
}

static void test_parse_root_not_singular() {
    TEST_ERROR(xState::X_PARSE_ROOT_NOT_SINGULAR, "null x");

#if 0
    /* invalid number */
    TEST_ERROR(xState::X_PARSE_ROOT_NOT_SINGULAR, "0123");
    TEST_ERROR(xState::X_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(xState::X_PARSE_ROOT_NOT_SINGULAR, "0x123");
#endif
}

static void test_parse_number_too_big() {
#if 0
    TEST_ERROR(xState::X_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(xState::X_PARSE_NUMBER_TOO_BIG, "-1e309");
#endif
}

static void test_parse_missing_quotation_mark() {
    TEST_ERROR(xState::X_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(xState::X_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
#if 0
    TEST_ERROR(xState::X_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(xState::X_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(xState::X_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(xState::X_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
#endif
}

static void test_parse_invalid_string_char() {
#if 0
    TEST_ERROR(xState::X_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(xState::X_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
#endif
}

static void test_access_null() {
    xValue v;
    xHelper helper(&v);
    xHelper::xSetString(&v, "a", 1);
    xHelper::xSetNull(&v);
    EXPECT_EQ_INT(xType::X_TYPE_NULL, xHelper::xGetType(&v));
}

static void test_access_boolean() {
    /* \TODO*/
}

static void test_access_number() {
    /* \TODO */
}

static void test_access_string() {
    xValue v;
    xHelper helper(&v);
    xHelper::xSetString(&v, "", 0);
    EXPECT_EQ_STRING("", xHelper::xGetString(&v),
        xHelper::xGetStringLength(&v));
    xHelper::xSetString(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", xHelper::xGetString(&v),
        xHelper::xGetStringLength(&v));
}

static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
}

static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(xState::X_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();

    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n",
        test_pass, test_count, test_pass  * 100.0 / test_count);
    return main_ret;
}
