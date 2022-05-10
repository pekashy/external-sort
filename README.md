## Big File Sorting Implementation

### Make&Run Tests

```mkdir build && cd build && cmake .. && make && ./tests```

### Run

```./build/external_sort``` - will by default take file `input` from the same directory as input and use 128MB of RAM

```./build/external_sort 134217728 input``` - same in explicit form

The result will be written into ```output``` file.

### Input/Output file format

Files are binary. First 8 bytes - quantity of numbers in file (uint64_t). Other contents - 4 byte numbers (uin32_t).

### Possible problems

Temporaty files are generated with help of ```std::tmpnam``` which is cross-platform, but gives no guarantees at all.

### Algorithm

Sorting is done by merge-sort algorithm modified to work in external memory to handle files larger than RAM available.
Surely is some place for optimization, for example some parts could be paralleled, but I saw to point in parallel
implemement

1. Define partition size as number of elements that can fit into RAM.
2. Go over input file, load it partition by partition into memory, sort partition and write it into temporary file.
3. Add temp file names into priority_queue which will always return a file with least number of elements.
4. Start merging sorted files. Define block size as minimum number of elements read operation will return (depends on
   SSD specs).
   No point in performing smaller IO operations.
5. Define number of partitions to merge simultaneously as k = RAMsize/block - 1 size.
   The reason for that is that the more files we merge at a time, the fewer times we have to repeat this operation. But
   every open
   file stream keeps a buffer with size of block_size in memory, so we can open only k files because of RAM limits.
   We keep in mind that we also have one file opened for write, which also keeps the open buffer, so we substract one
   from RAMsize/block.
6. We also might have a number of partitions which will have non-zero remainder compared to k. In that case we first
   merge a smaller number of partitions for the number left to have a zero remainder to k.
7. Merge files using priority_queue, write merged partitions to a single partition.
8. Add a new, merged partion to partition priority_queue.
9. Repeat this while partition priority queue size is more than one. Return the last partition filename left in
   partition queue. That would be ours sorted file.
10. Copy that file to an ```output``` destination.
