# Runtime/Builtin Status

## Current Builtins (Implemented in stage0.c)

### I/O
| Builtin | Status | Notes |
|---------|--------|-------|
| `print(str)` | ✅ Working | Uses printf |
| `println(str)` | ✅ Working | Uses puts |
| `print_float(float)` | ✅ Working | |
| `print_char(int)` | ✅ Working | |
| `print_newline()` | ✅ Working | |
| `flush()` | ✅ Working | fflush(stdout) |
| `read_line()` | ✅ Working | fgets from stdin |

### File System
| Builtin | Status | Notes |
|---------|--------|-------|
| `read_file(path)` | ✅ Working | Returns str or 0 |
| `write_file(path, content)` | ✅ Working | |
| `append_file(path, content)` | ✅ Working | |
| `file_exists(path)` | ✅ Working | Uses access() |
| `delete_file(path)` | ✅ Working | Uses remove() |
| `mkdir(path)` | ✅ Working | |
| `rmdir(path)` | ✅ Working | |

### Time
| Builtin | Status | Notes |
|---------|--------|-------|
| `time_now()` | ✅ Working | Unix timestamp |
| `sleep_ms(ms)` | ✅ Working | Uses usleep |
| `clock()` | ✅ Working | CPU time |

### Network
| Builtin | Status | Notes |
|---------|--------|-------|
| `tcp_connect(host, port)` | ✅ Working | Returns fd |
| `tcp_close(fd)` | ✅ Working | |
| `tcp_send(fd, data)` | ✅ NEW | Returns bytes sent |
| `tcp_recv(fd, maxlen)` | ✅ NEW | Returns string |

### System
| Builtin | Status | Notes |
|---------|--------|-------|
| `getenv(name)` | ✅ Working | |
| `system(cmd)` | ✅ Working | |
| `exit(code)` | ✅ Working | |

### String/Conversion
| Builtin | Status | Notes |
|---------|--------|-------|
| `len(arr/str)` | ✅ Working | |
| `ord(str)` | ✅ Working | First char ASCII |
| `chr(int)` | ✅ Working | Int to char |
| `char_at(str, idx)` | ✅ Working | |
| `int_to_str(int)` | ✅ Working | |
| `str_to_int(str)` | ✅ Working | |
| `int_to_float(int)` | ✅ Working | |
| `float_to_int(float)` | ✅ Working | |
| `str_find(haystack, needle)` | ✅ NEW | Returns index or -1 |
| `str_cmp(a, b)` | ✅ NEW | Returns 0 if equal |

### Math
| Builtin | Status | Notes |
|---------|--------|-------|
| `sqrt(float)` | ✅ Working | |
| `abs(int)` | ✅ Working | |
| `min(a, b)` | ✅ Working | |
| `max(a, b)` | ✅ Working | |
| `random()` | ✅ Working | |

---

## Missing Builtins (Needed for stdlib)

### Priority 1 - Essential for Stage 1
| Builtin | Needed By | Implementation |
|---------|-----------|----------------|
| `tcp_send(fd, data)` | net.wyn | write() syscall |
| `tcp_recv(fd, len)` | net.wyn | read() syscall |
| `tcp_listen(port)` | net.wyn | socket/bind/listen |
| `tcp_accept(fd)` | net.wyn | accept() |
| `str_concat(a, b)` | string.wyn | malloc + strcpy |
| `str_substr(s, start, len)` | string.wyn | malloc + memcpy |
| `str_find(haystack, needle)` | string.wyn | strstr |
| `str_split(s, delim)` | string.wyn | Returns array |
| `str_eq(a, b)` | string.wyn | strcmp |

### Priority 2 - Useful for stdlib
| Builtin | Needed By | Implementation |
|---------|-----------|----------------|
| `list_dir(path)` | fs.wyn | opendir/readdir |
| `file_size(path)` | fs.wyn | stat() |
| `is_dir(path)` | fs.wyn | stat() |
| `rename(old, new)` | fs.wyn | rename() |
| `getcwd()` | os.wyn | getcwd() |
| `chdir(path)` | os.wyn | chdir() |
| `getpid()` | os.wyn | getpid() |

### Priority 3 - Nice to have
| Builtin | Needed By | Implementation |
|---------|-----------|----------------|
| `sin(float)` | math.wyn | libm |
| `cos(float)` | math.wyn | libm |
| `tan(float)` | math.wyn | libm |
| `log(float)` | math.wyn | libm |
| `exp(float)` | math.wyn | libm |
| `pow(base, exp)` | math.wyn | libm |
| `floor(float)` | math.wyn | libm |
| `ceil(float)` | math.wyn | libm |

---

## Stdlib Files - Fix Status

| File | Status | Notes |
|------|--------|-------|
| std/fs.wyn | ✅ Fixed | Uses read_file, write_file, etc. |
| std/io.wyn | ⚠️ Partial | Basic I/O works |
| std/net.wyn | ⚠️ Partial | tcp_connect/send/recv work |
| std/http.wyn | ❌ Stubs | Depends on net.wyn |
| std/string.wyn | ✅ Fixed | Uses str_find for contains/index_of |
| std/time.wyn | ⚠️ Partial | time_now works |
| std/os.wyn | ✅ Fixed | Uses getenv, system, time_now, sleep_ms |
| std/math.wyn | ⚠️ Partial | Basic ops work, needs trig |
| std/json.wyn | ❌ Stubs | Needs string ops |
| std/crypto.wyn | ❌ Stubs | Needs byte arrays |
| std/test.wyn | ✅ Fixed | Uses conditionals |

---

## Action Plan

### Phase 1: Wire up existing builtins to stdlib
1. Fix std/fs.wyn to use read_file, write_file, etc.
2. Fix std/os.wyn to use getenv, system
3. Fix std/time.wyn to use time_now, sleep_ms

### Phase 2: Add string builtins
1. str_concat(a, b) -> str
2. str_substr(s, start, len) -> str
3. str_find(haystack, needle) -> int (-1 if not found)
4. str_eq(a, b) -> bool

### Phase 3: Add network builtins
1. tcp_send(fd, data) -> int (bytes sent)
2. tcp_recv(fd, maxlen) -> str
3. tcp_listen(port) -> int (fd)
4. tcp_accept(fd) -> int (client fd)

### Phase 4: Add math builtins
1. sin, cos, tan, asin, acos, atan
2. log, log10, exp
3. pow, floor, ceil, round
