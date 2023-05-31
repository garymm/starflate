# Benchmark results

## Apple MacBook Pro, M1 Pro 16 GB

| name | iterations | real_time | cpu_time | comment |
| -    |       -    | -         |         -|        -|
| BM_CodeTable | 588879 | 1174.2 | 1174.12 | [d44ffea96815292b517603334895e58c2cd9ea62](https://github.com/garymm/gpu-deflate/tree/d44ffea96815292b517603334895e58c2cd9ea62) (initial impl based on [dahuffman](https://github.com/soxofaan/dahuffman/)) |
| BM_CodeTable | 8931647 |79.1927 | 79.1827 | [52bdee03371059ab181b2af63bbe729598c62928](https://github.com/garymm/gpu-deflate/tree/52bdee03371059ab181b2af63bbe729598c62928) (replace merging priority queues with in-place rotations)
