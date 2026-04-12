# Maintenance

## Build
```sh
make clean && make test
```

## Adding features
- Keep zero-malloc constraint
- State transitions must be valid (see enum)
- Max tasks is EPH_MAX (32) — increase in ephemeral.h if needed

## Known limitations
- `eph_tick` only expires SPAWNED tasks (not RUNNING)
- No reaping / slot reuse after completion
- Not thread-safe

## Testing
19 tests cover: init, spawn, find, start, complete, output truncation, cancel, tick expiry, CPU budget enforcement, memory tracking, owner/state filtering, capacity limits, sandboxed flag.
