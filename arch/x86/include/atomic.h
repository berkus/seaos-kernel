#ifndef ATOMIC_H
#define ATOMIC_H

#define bts_atomic(ptr, v) ((__sync_fetch_and_or(ptr, 1 << v) & (1 << v)) ? 1 : 0)
#define btr_atomic(ptr, v) ((__sync_fetch_and_and(ptr, ~(1 << v)) & (1 << v)) ? 1 : 0)
#define add_atomic(ptr, v) (__sync_add_and_fetch(ptr, v))
#define sub_atomic(ptr, v) (__sync_sub_and_fetch(ptr, v))

#endif
