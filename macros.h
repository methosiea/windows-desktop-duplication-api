#pragma once
#include <cassert>

// Expects an expression to return a successful HRESULT code.
#ifdef _DEBUG
#define ASSERT_SUCCEEDED(expression, message) assert((message, SUCCEEDED(expression)))
#else
#define ASSERT_SUCCEEDED(expression, message) expression
#endif