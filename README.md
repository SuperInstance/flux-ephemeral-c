# flux-ephemeral-c

Pure C11 library for sandbox task lifecycle management. Zero dependencies, no heap allocation.

## Usage

```c
#include "ephemeral.h"

EphManager mgr;
eph_init(&mgr);

int id = eph_spawn(&mgr, "task", 1, 5000, 10000, 4096, 1);
eph_start(&mgr, id);
eph_complete(&mgr, id, 0, (uint8_t*)"ok", 2);
```

## API

| Function | Description |
|---|---|
| `eph_init` | Initialize manager |
| `eph_spawn` | Create task, returns task_id or -1 |
| `eph_find` | Find task by id |
| `eph_start` | Transition SPAWNED → RUNNING |
| `eph_complete` | Set COMPLETED with output (truncated to 255) |
| `eph_cancel` | Cancel SPAWNED or RUNNING task |
| `eph_tick` | Expire SPAWNED tasks past deadline |
| `eph_report_cpu` | Accumulate CPU, FAIL if over budget |
| `eph_report_mem` | Accumulate memory usage |
| `eph_by_owner` | Filter tasks by owner |
| `eph_by_state` | Filter tasks by state |

## Build

```sh
make test
```

## Constraints

- Max 32 concurrent tasks (`EPH_MAX`)
- Task names: 31 chars max
- Output buffer: 255 bytes max
- Pure C11, no malloc, no deps
