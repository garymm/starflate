# Benchmark results

## Apple MacBook Pro, M1 Pro 16 GB

| name | iterations | real_time (ns) | cpu_time (ns) | comment |
| -    |       -    | -         |         -|        -|
| BM_CodeTable | 588879 | 1174.2 | 1174.12 | [d44ffea96815292b517603334895e58c2cd9ea62](https://github.com/garymm/starflate/tree/d44ffea96815292b517603334895e58c2cd9ea62) (initial impl based on [dahuffman](https://github.com/soxofaan/dahuffman/)) |
| BM_CodeTable | 8931647 |79.1927 | 79.1827 | [52bdee03371059ab181b2af63bbe729598c62928](https://github.com/garymm/starflate/tree/52bdee03371059ab181b2af63bbe729598c62928) (replace merging priority queues with in-place rotations)
| BM_CodeTable | 16829634 |41.5 | 41.5 | [efd83290046d9f7118b7d7563b99c33a604a9626](https://github.com/garymm/starflate/tree/efd83290046d9f7118b7d7563b99c33a604a9626) (use std::array, avoid heap allocation altogether)
