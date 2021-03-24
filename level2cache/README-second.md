Please View In "RAW" format to understand format of text files and proper outputs


Part 2 - Two Level Cache

Most of the modern CPUs have multiple level of caches. In the second part of the assignment,
you have to simulate a system with a two-level of cache (i.e. L1 and L2). Multi-level caches
can be designed in various ways depending on whether the content of one cache is present in
other levels or not. In this assignment you implement an exclusive cache: the lower level
cache (i.e. L2) contains only blocks that are not present in the upper level cache (i.e. L1).

Consider the case when L2 is exclusive of L1. Suppose there is a read request for block X.
If the block is found in L1 cache, then the data is read from L1 cache. If the block is not
found in the L1 cache, but present in the L2 cache (An L2 cache hit), then the cache block
is moved from the L2 cache to the L1 cache. If this causes a block to be evicted from L1,
the evicted block is then placed into L2. If the block is not found in either L1 or L2, then
it is read from main memory and placed just in L1 and not in L2. In the exclusive cache
configuration, the only way L2 gets populated is when a block is evicted from L1. Hence,
the L2 cache in this configuration is also called a victim cache for L1.

Sample Run:

The details from Part 1 apply here to the second level L2 cache. Your program gets two
separate configurations (one for level 1 and one for level 2 cache). Both L1 and L2 have the
same block size. Your program should report the total number of memory reads and writes,
followed by cache miss and hit for L1 and L2 cache.

Here is the format for part 2.
./second <L1 cache size><L1 associativity><L1 cache policy><L1 block size><L2 cache
size><L2 associativity><L2 cache policy><trace file>
  
This is an example testcase for part 2.

$./second 32 assoc:2 fifo 4 64 assoc lru trace2.txt
memread:3277
memwrite:2861
l1cachehit:6501
l1cachemiss:3499
l2cachehit:222
l2cachemiss:3277
  
The above example, simulates a 2-way set associate cache of size 32 bytes. bytes with block
size of 4 for L1 cache. Similarly, L2 cache is a fully associate cache of size 64 bytes. Further,
the trace file used for this run is trace2.txt. As you can see, the program outputs the memory
read and memory writes followed by the L1 and L2 cache hits and misses in the order shown
above.
