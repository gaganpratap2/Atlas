## Day 3 — Binary Market-Data Decoder

### Workload

- Component: `atlas::market_data::Decoder`
- Message type: Trade
- Message size: 31 bytes
- Iterations per run: 1,000,000
- Build: Release
- Platform: Windows

### Results

| Run | Elapsed time | Avg decode time | Checksum |
|---|---:|---:|---:|
| 1 | 17,071,200 ns | 17.0712 ns/event | 100,000,000 |
| 2 | 17,560,400 ns | 17.5604 ns/event | 100,000,000 |

### Interpretation

Both runs successfully decoded 1,000,000 Trade messages with the expected
checksum.

The observed average was approximately 17–18 ns/event across these two runs.
This is a local development baseline for the current decoder implementation,
not a production HFT latency claim.

### Reproducibility

```text
.\build\cpp\Release\atlas_decoder_benchmark.exe