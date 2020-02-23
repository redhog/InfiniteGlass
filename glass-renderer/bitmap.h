#ifndef BITMAP_H
#define BITMAP_H

#define BITMAP(arr, idx) ((arr[(idx) / 8] & (1 << ((idx) % 8))) >> ((idx) % 8))
#define BITMAP_SET(arr, idx, val) (arr[(idx) / 8] = ((arr[(idx) / 8] & ~(1 << ((idx) % 8))) | ((val) << ((idx) % 8))))

#endif
