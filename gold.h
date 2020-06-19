// This file is free software, distributed under the BSD license.

#pragma once

struct gold {
    struct gold *ngold;
    int8_t gx, gy;
    long amount;
};

static inline struct gold* newgold (void)
    { return alloc (sizeof(struct gold)); }
