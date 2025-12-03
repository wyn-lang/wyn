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
| `rename_file(old, new)` | ✅ NEW | Returns 0 on success |

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
| `tcp_send(fd, data)` | ✅ Working | Returns bytes sent |
| `tcp_recv(fd, maxlen)` | ✅ Working | Returns string |
| `tcp_listen(port)` | ✅ NEW | Returns server fd |
| `tcp_accept(fd)` | ✅ NEW | Returns client fd |

### System
| Builtin | Status | Notes |
|---------|--------|-------|
| `getenv(name)` | ✅ Working | |
| `system(cmd)` | ✅ Working | |
| `exit(code)` | ✅ Working | |
| `getcwd()` | ✅ NEW | Returns current directory |
| `chdir(path)` | ✅ NEW | Returns 0 on success |
| `getpid()` | ✅ NEW | Returns process ID |

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
| `sqrtf(float)` | ✅ NEW | Hardware sqrt |
| `abs(int)` | ✅ Working | |
| `min(a, b)` | ✅ Working | |
| `max(a, b)` | ✅ Working | |
| `random()` | ✅ Working | |
| `sinf(float)` | ✅ NEW | libm sin |
| `cosf(float)` | ✅ NEW | libm cos |
| `tanf(float)` | ✅ NEW | libm tan |
| `logf(float)` | ✅ NEW | libm log (natural) |
| `expf(float)` | ✅ NEW | libm exp |
| `powf(base, exp)` | ✅ NEW | libm pow |
| `floorf(float)` | ✅ NEW | Hardware floor |
| `ceilf(float)` | ✅ NEW | Hardware ceil |

---

## Missing Builtins (Needed for stdlib)

### Priority 1 - Essential for Stage 1
| Builtin | Needed By | Status |\n|---------|-----------|--------|\n| `str_concat(a, b)` | string.wyn | ✅ Working |\n| `str_substr(s, start, len)` | string.wyn | ✅ Working |\n| `str_split(s, delim)` | string.wyn | ⚠️ Implemented but needs debugging |

### Priority 2 - Useful for stdlib
| Builtin | Needed By | Status |
|---------|-----------|--------|
| `list_dir(path)` | fs.wyn | ❌ Not yet (needs opendir/readdir) |
| `file_size(path)` | fs.wyn | ✅ Working |
| `is_dir(path)` | fs.wyn | ✅ Working |

### Priority 3 - Nice to have
| Builtin | Needed By | Implementation |
|---------|-----------|----------------|
| `asinf(float)` | math.wyn | ✅ libm |
| `acosf(float)` | math.wyn | ✅ libm |
| `atanf(float)` | math.wyn | ✅ libm |
| `atan2f(y, x)` | math.wyn | ✅ libm |
| `log10f(float)` | math.wyn | ✅ libm |

---

## Stdlib Files - Fix Status

| File | Status | Notes |
|------|--------|-------|
| std/fs.wyn | ✅ Fixed | Uses read_file, write_file, etc. |
| std/io.wyn | ⚠️ Partial | Basic I/O works |
| std/net.wyn | ✅ Working | tcp_connect/send/recv/listen/accept work |
| std/http.wyn | ❌ Stubs | Depends on net.wyn |
| std/string.wyn | ✅ Fixed | Uses str_find for contains/index_of |
| std/time.wyn | ⚠️ Partial | time_now works |
| std/os.wyn | ✅ Fixed | Uses getenv, system, time_now, sleep_ms |
| std/math.wyn | ✅ Fixed | Uses sqrtf, sinf, cosf, tanf, logf, expf, powf |
| std/json.wyn | ❌ Stubs | Needs string ops |
| std/crypto.wyn | ❌ Stubs | Needs byte arrays |
| std/test.wyn | ✅ Fixed | Uses conditionals |

---

## Action Plan

### Phase 1: Wire up existing builtins to stdlib
1. Fix std/fs.wyn to use read_file, write_file, etc.
2. Fix std/os.wyn to use getenv, system
3. Fix std/time.wyn to use time_now, sleep_ms

### Phase 2: Add string builtins (mostly complete)
1. ✅ str_concat(a, b) -> str
2. ✅ str_substr(s, start, len) -> str
3. ✅ str_find(haystack, needle) -> int (-1 if not found)
4. ✅ str_cmp(a, b) -> int (0 if equal)
5. ⚠️ str_split(s, delim) -> [str] (implemented but needs debugging)

### Phase 3: Add network builtins ✅ COMPLETE
1. ✅ tcp_send(fd, data) -> int (bytes sent)
2. ✅ tcp_recv(fd, maxlen) -> str
3. ✅ tcp_listen(port) -> int (fd)
4. ✅ tcp_accept(fd) -> int (client fd)

### Phase 4: Add math builtins ✅ COMPLETE
1. ✅ sinf, cosf, tanf (asin, acos, atan still needed)
2. ✅ logf, expf
3. ✅ powf, floorf, ceilf, sqrtf
