/**
 * @file str_utils.h
 * @brief String utility functions
 *
 * Tests for string utility functions such as safe snprintf and truncating
 * strcpy.
 *
 * @author Ayush Garg
 * @date 2025-11-09
 */

#include "error.h"
#include "logger.h"
#include "str_utils.h"
#include <stdio.h>
#include <string.h>

void test_snprintf_len()
{
    LOG_DEBUG("=== Testing snprintf_len ===");

    char buffer[10];

    // Normal case: string fits in buffer
    int len = snprintf_len(buffer, sizeof(buffer), "Hello");
    ASSERT(len == 6); // "Hello" + null terminator
    ASSERT(strcmp(buffer, "Hello") == 0);

    // String of length 10 should be 9 + 1 for null terminator
    len = snprintf_len(buffer, sizeof(buffer), "abcdefghij");
    ASSERT(len == 10);
    ASSERT(strcmp(buffer, "abcdefghi") == 0); // truncated to fit buffer

    // String longer than buffer
    len = snprintf_len(buffer, sizeof(buffer), "This is a long string");
    ASSERT(len == 10);                        // buffer size
    ASSERT(strcmp(buffer, "This is a") == 0); // truncated to fit buffer

    LOG_DEBUG("✓ snprintf_len tests passed");
}

void test_strcpy_trunc()
{
    LOG_DEBUG("=== Testing strcpy_trunc ===");

    char buffer[6];
    strcpy_trunc(buffer, "Hi", sizeof(buffer));
    ASSERT(buffer[2] == '\0');
    ASSERT(strcmp(buffer, "Hi") == 0);

    strcpy_trunc(buffer, "Hello", sizeof(buffer));
    ASSERT(buffer[5] == '\0');
    ASSERT(strcmp(buffer, "Hello") == 0);

    strcpy_trunc(buffer, "Hello, World!", sizeof(buffer));
    ASSERT(buffer[5] == '\0');
    ASSERT(strcmp(buffer, "Hello") ==
           0); // truncated to fit buffer, including null terminator

    LOG_DEBUG("✓ strcpy_trunc tests passed");
}

int main()
{
    LOG_DEBUG("=== String Utilities Tests ===");

    test_snprintf_len();
    test_strcpy_trunc();

    LOG_DEBUG("✓ All string utilities tests passed");
    return 0;
}
