#include "core.h"
#include "fileops.h"

// #include "hkArray.h"
#include "hkArrayT-primitives.h"

# define N 1024
u8 block[N];

int main(int argc, char *argv[]) {
    i32 res = 0;

    hkArrayf32 arrayf32 = hkArrayf32Create(8);
    for( int i = 0; i < arrayf32.length; ++i) {
        arrayf32.data[i] = i;
    }
    res = hkArrayf32IsEmpty(&arrayf32);
    printf("%d\n", res);
    hkArrayf32Destroy(&arrayf32);

    hkArrayi32 arrayi32 = hkArrayi32Create(8);
    for( int i = 0; i < arrayi32.length; ++i) {
        arrayi32.data[i] = i;
    }
    res = hkArrayi32IsEmpty(&arrayi32);
    printf("%d\n", res);
    hkArrayi32Destroy(&arrayi32);

    hkArrayi64 arr = hkArrayi64Create(8);
    hkArrayi64Destroy(&arr);

    // fops_read("test.txt");
    // printf("%s", fops_buffer);

    // initialize
    u8 *mem = block;
    u64 pos = (u64)block;
    
    // alloc 0
    const i32 numslen = 32;
    f32 *nums = (f32 *)pos;
    pos += sizeof(f32) * numslen;
    if ((pos - (u64)mem) > N) { return -1; }
    for (i32 i = 0; i < numslen; ++i) {
        nums[i] = (f32)i;
    }

    // alloc 1
    const i32 charlen = 0xFF;
    i8 *chars = (i8 *)pos;
    pos += sizeof(i8) * charlen;
    if ((pos - (u64)mem) > N) { return -1; }
    for (i32 i = 0; i < charlen; ++i) {
        chars[i] = (i8)i;
    }

    // dealloc all
    pos = (u64)mem;

    /*
     * What is a Queue?
     * first in first out
     * .                    | empty
     * 0                    | enqueue
     * 0 -> 1               | enqueue
     * 0 -> 1 -> 2          | enqueue
     * 0 -> 1 -> 2 -> 3     | enqueue
     * 1 -> 2 -> 3          | dequeue
     * 2 -> 3               | dequeue
     * 3                    | dequeue
     * .                    | empty
     */

    return 0;
}
