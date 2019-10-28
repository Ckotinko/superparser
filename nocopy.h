#pragma once

#define NOCOPY(x) \
    x(const x&) = delete;\
    x& operator = (const x&) = delete;

#define NOMOVE(x) \
    x(x&&) = delete;\
    x& operator = (x&&) = delete;
