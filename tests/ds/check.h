#ifndef DS_TEST_CHECK_H
#define DS_TEST_CHECK_H

#include <iostream>

static int passed = 0;
static int failed = 0;

#define CHECK(cond)                                                   \
    do                                                                \
    {                                                                 \
        if (cond)                                                     \
            ++passed;                                                 \
        else                                                          \
        {                                                             \
            ++failed;                                                 \
            std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; \
        }                                                             \
    } while (0)

#endif // DS_TEST_CHECK_H