# Session B03: Functions — Ví dụ và nghiên cứu tình huống

> **Case đã chốt:** `CASE-B03-01` · **Milestone:** `M00-FND-03` · **LO:** `ADVC-H1SD` · **Mức xác thực:** simulated

## 🎯 Learning Outcomes liên quan

Là C systems developer của **MDB Edge Diagnostics Gateway — Simulated**, học viên tái cấu trúc xử lý batch thành các function contract kiểm chứng được. Case tạo đúng artifact đã lên kế hoạch:

`Phan_0_Optional_C_Basics_Refresher/Session_0_B03_Functions/assets/b03_functions_demo.c`

Artifact là hosted ISO C17. Đặc biệt, nó **không** dùng nested function: định nghĩa function trong function khác là GNU-only, không portable ISO C17. File-scope callback + explicit context tạo cùng khả năng mang state mà vẫn qua strict portability gate.

## Mapping case đã chốt trong course plan

- **id:** CASE-B03-01
- **learning_outcomes:** ADVC-H1SD
- **outline_refs:** OUT-B03-02, OUT-B03-03, OUT-B03-04, OUT-B03-06, OUT-B03-07, OUT-B03-08, OUT-B03-09, OUT-B03-11, OUT-B03-12, OUT-B03-13, OUT-B03-14, OUT-B03-15, OUT-B03-16, OUT-B03-17, OUT-B03-19, OUT-B03-20
- **authenticity:** inherit (simulated)

| Leaf | Bằng chứng trực tiếp trong asset/case |
|---|---|
| OUT-B03-02 1.1 Syntax | Named return/parameter types và `main(int argc, char **argv)`. |
| OUT-B03-03 1.2 Declaration and function prototype | Definition/prototype xuất hiện trước call; signature đầy đủ. |
| OUT-B03-04 1.3 Global function, Local function, function in single translate unit | Helpers `static` ở file scope; `main` là entry point external. |
| OUT-B03-06 2.1 Compiler optimization use-case | `static inline clamp_upper`; correctness không phụ thuộc expansion. |
| OUT-B03-07 2.2 Different in size, speed analysis | `--benchmark` có workload/checksum cố định; build `-O0/-O2`, năm timing samples và median. |
| OUT-B03-08 3 Phân biệt macro-like function và function | Typed inline function thay function-like calculation macro. |
| OUT-B03-09 4 Variable argument list | `checked_sum_varargs` dùng count, `va_list`, `va_start/va_arg/va_end`. |
| OUT-B03-11 5.1 Passing argument as value | `limit`, `value`, `count` được truyền theo value. |
| OUT-B03-12 5.2 Passing argument as reference | C truyền giá trị pointer; parser ghi caller-owned output sau validation. |
| OUT-B03-13 5.3 example | `run_summary` kiểm status của từng call trước dùng output. |
| OUT-B03-14 Multiple input/multiple output function | `find_range` yêu cầu hai output distinct, reject alias failure-atomic và ghi min/max khi success. |
| OUT-B03-15 Passing array as argument | Mọi sequence API nhận `values` + `count`. |
| OUT-B03-16 return/passing pointer to diffrent kind of object | `void *context` được cast lại đúng `SumContext *`; không trả pointer local. |
| OUT-B03-17 nested function | Comment + implementation ghi rõ GNU-only; alternative là `add_clamped` + context. |
| OUT-B03-19 6.1 introduction to stack | Recursion có depth/input bound `16`; không giả định stack byte portable. |
| OUT-B03-20 6.2 how to create a recursion function | Base case `count==0`, step `values+1,count-1`, failure propagation. |

## Case Study 01: Ticket DGW-FND-03 — Xây function pipeline portable

### 1. Ticket và tiêu chí thành công

- **Vai trò/stakeholder:** C developer; maintainer cần interface nhỏ, tester cần oracle deterministic, reviewer cần portability evidence.
- **Vấn đề:** nhận `LIMIT VALUE...`, tính sum/min/max, clipped sum và một recursive checksum mà không gom toàn bộ logic vào `main`.
- **Ràng buộc:** ISO C17; tối đa 16 values; recursion tối đa 16; zero warnings; không GNU nested function; invalid token không tạo output một phần.
- **Thành công:** self-test pass; happy fixture đúng hai dòng stdout/exit `0`; negative fixture đúng stderr/exit `2`; strict compiler gate pass.

### 2. Input, trạng thái ban đầu và ràng buộc

Happy fixture:

```text
LIMIT=10
VALUES=3 7 11
```

Negative fixture:

```text
LIMIT=10
VALUES=3 bad 11
```

Trạng thái mỗi process bắt đầu độc lập. `values[16]` được zero-initialize nhưng chỉ prefix `count` là input. `SumContext` bắt đầu `{limit=10,sum=0,accepted=0}`. Không có clock, random, file, network hoặc environment-dependent output.

### 3. Phân tích lựa chọn và trade-off

Các boundary được chọn theo trách nhiệm:

1. `parse_i32`: text → typed scalar, status + output pointer.
2. `find_range`: array + length → status + hai output pointers non-null/distinct; failure không sửa output.
3. `visit_values`: traversal policy → callback + explicit context.
4. `add_clamped`: operation file-local; `static inline clamp_upper` cho phép helper typed.
5. `recursive_sum`: ví dụ recursion có bound và wrapper che `depth` nội bộ.
6. `run_summary`: orchestration; dừng ngay khi một contract fail.

Không chọn macro để tính clamp vì function bảo đảm mỗi argument được evaluate một lần và có type checking. Không chọn variadic list cho batch chính vì typed array + count an toàn hơn; variadic helper chỉ được giới hạn trong self-test để minh họa protocol. Không chọn GNU nested callback vì khóa yêu cầu portable ISO C17.

Trade-off: callback + `void *` làm traversal tái sử dụng được nhưng mất một phần type safety ở context cast. Recursion gần định nghĩa toán nhưng với array tuyến tính, loop tiết kiệm call frames hơn; case giữ recursion chỉ để học mechanism và áp bound `16`.

### 4. Cách triển khai

Đọc asset:

```bash
sed -n '1,340p' assets/b03_functions_demo.c
```

Luồng chính:

```text
argv
  → parse_i32(limit, values[])
  → find_range(values,count,&min,&max)
  → visit_values(values,count,add_clamped,&context)
  → recursive_sum(values,count,&total)
  → print only after every contract succeeds
```

Các điểm then chốt:

- Helper implementation details có internal linkage nhờ `static`.
- `static inline` không được coi là optimization guarantee; chỉ output/behavior là contract.
- `find_range` từ chối null, empty và `minimum == maximum` trước mọi write; tính vào candidates rồi mới publish.
- `checked_sum_varargs` đọc đúng số argument kiểu `int` và luôn `va_end`.
- `VisitFn` là function pointer typed; `context` chỉ dành cho object pointer.
- `add_clamped` ở file scope, không nằm trong `run_summary`; đây là portable alternative cho closure/nested function.
- Wrapper reject count trên bound trước recursion; base case đứng trước phép `count - 1U`.

### 5. Build và chạy chính xác

Từ thư mục session:

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror \
  assets/b03_functions_demo.c -o b03_functions_demo
./b03_functions_demo --self-test
./b03_functions_demo --summarize 10 3 7 11
```

`-Wpedantic -Werror` phải được giữ; bỏ nó có thể che việc vô tình đưa GNU extension trở lại source.

### 6. Output mong đợi

Self-test, stdout chính xác:

```text
B03 SELF-TEST PASS checks=9
```

Happy path, stdout chính xác:

```text
count=3 sum=21 min=3 max=11 clipped_sum=20
recursive_sum=21
```

Happy path: stderr rỗng, exit code `0`.

Negative path:

```bash
./b03_functions_demo --summarize 10 3 bad 11
```

stderr chính xác:

```text
error: invalid integer at position 2: bad
```

Negative path: stdout rỗng, exit code `2`.

### 7. Cách xác minh

```bash
set -eu
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror \
  assets/b03_functions_demo.c -o b03_functions_demo
test "$(./b03_functions_demo --self-test)" = \
  "B03 SELF-TEST PASS checks=9"
test "$(./b03_functions_demo --summarize 10 3 7 11)" = \
  "count=3 sum=21 min=3 max=11 clipped_sum=20
recursive_sum=21"

set +e
negative_output=$(./b03_functions_demo --summarize 10 3 bad 11 2>&1 >/dev/null)
negative_status=$?
set -e
test "$negative_status" -eq 2
test "$negative_output" = \
  "error: invalid integer at position 2: bad"
```

Pass khi script exit `0`. Oracle bao gồm stdout/stderr separation, line order và exit status; “output gần giống” không pass.

Để tạo **timing evidence thực sự lặp lại được**, dùng một process cho toàn workload thay vì timing hàng triệu lần khởi động CLI. `--benchmark` sinh ba giá trị từ recurrence unsigned xác định ở mỗi iteration, gọi các function contract và in checksum observable. Trên Ubuntu 22.04/WSL2, dùng đồng hồ nanosecond của GNU Coreutils `date`:

```bash
set -eu
export LC_ALL=C
bench_dir=$(mktemp -d)
trap 'rm -rf -- "$bench_dir"' EXIT

gcc --version | head -n 1 | tee "$bench_dir/compiler.txt"
uname -a | tee "$bench_dir/platform.txt"
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O0 \
  assets/b03_functions_demo.c -o "$bench_dir/b03_O0"
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  assets/b03_functions_demo.c -o "$bench_dir/b03_O2"
printf '%s\n' 'BENCH iterations=10000000 checksum=-230967616032' \
  > "$bench_dir/expected.txt"

for opt in O0 O2; do
  bin="$bench_dir/b03_$opt"
  "$bin" --benchmark 10000000 > "$bench_dir/$opt.warmup.out"
  cmp "$bench_dir/expected.txt" "$bench_dir/$opt.warmup.out"
  : > "$bench_dir/$opt.elapsed_ns"
  run=1
  while [ "$run" -le 5 ]; do
    start_ns=$(date +%s%N)
    "$bin" --benchmark 10000000 > "$bench_dir/$opt.$run.out"
    end_ns=$(date +%s%N)
    cmp "$bench_dir/expected.txt" "$bench_dir/$opt.$run.out"
    elapsed_ns=$((end_ns - start_ns))
    test "$elapsed_ns" -ge 0
    printf '%s\n' "$elapsed_ns" >> "$bench_dir/$opt.elapsed_ns"
    run=$((run + 1))
  done
  test "$(wc -l < "$bench_dir/$opt.elapsed_ns")" -eq 5
  median_ns=$(sort -n "$bench_dir/$opt.elapsed_ns" | sed -n '3p')
  printf '%s median_elapsed_ns=%s\n' "$opt" "$median_ns"
done
size "$bench_dir/b03_O0" "$bench_dir/b03_O2"
```

**Oracle chức năng chính xác cho warm-up và cả 10 measured runs:**

```text
BENCH iterations=10000000 checksum=-230967616032
```

Boundary argv của mode đo cũng cố định: `--benchmark 0` phải stdout rỗng, stderr đúng `error: iterations must be in range 1..50000000`, exit `2`; không đưa negative run này vào timing samples.

Mỗi process phải stderr rỗng và exit `0`; mỗi file `.elapsed_ns` phải có đúng năm số không âm, median là dòng thứ ba sau sort. `compiler.txt`, `platform.txt`, hai median và bảng `size` là evidence cần lưu. Không có oracle portable cho nanosecond/số byte và không đặt điều kiện `O2 < O0`: scheduler, CPU frequency, thermal state, linker và toolchain gây nhiễu. Chỉ so hai build ngay trong cùng lượt, cùng source/input/máy; lặp toàn protocol nếu coefficient of variation hoặc samples dao động lớn.

**Evidence tham chiếu đã chạy ngày 2026-08-22** trong image khóa `codex/adv-c-jammy:20260821` (WSL2 x86-64), GCC `11.4.0`, glibc `2.35`; đây là quan sát của đúng fingerprint đó, **không phải acceptance threshold**:

```text
O0 samples_ns=531438336,522181104,546777034,526796922,527606579 median_ns=527606579
O2 samples_ns=42571267,42788132,42949265,42010435,43256584 median_ns=42788132
   text   data    bss    dec    hex filename
   6419    656     16   7091   1bb3 b03_O0
   4353    640     16   5009   1391 b03_O2
```

Evidence chỉ cho phép kết luận “trong lượt đo này, trên fingerprint này” về chênh lệch quan sát; nó không chứng minh `inline` là nguyên nhân duy nhất và không được suy rộng sang target embedded.

### 8. Giải thích sâu, failure modes và chuyển giao

- **Symptom:** giá trị parse lỗi vẫn xuất hiện → **cause:** caller bỏ qua status/output nửa chừng → **diagnose:** trace return từng function → **fix:** early return trước presenter → **prevent:** negative fixture kiểm stdout rỗng.
- **Symptom:** `min` biến mất và chỉ còn `max` → **cause:** caller truyền cùng một object cho hai outputs → **diagnose:** so địa chỉ hai pointers và state trước/sau failure → **fix:** `find_range` reject alias trước mọi write → **prevent:** self-test `same=1234` phải giữ nguyên khi return `false`.
- **Symptom:** callback crash ở `state->sum` → **cause:** context null/sai actual type/hết lifetime → **diagnose:** xem call site và cast → **fix:** caller-owned live `SumContext` + null guard → **prevent:** một callback typedef và contract context.
- **Symptom:** strict build báo nested function → **cause:** GNU-only definition trong block → **diagnose:** tìm function body bên trong function khác → **fix:** lift ra file scope, truyền state qua context → **prevent:** CI giữ `-std=c17 -Wpedantic -Werror`.
- **Symptom:** recursion reset/overflow với batch lớn → **cause:** thiếu bound/không tiến về base → **diagnose:** log count/depth và đo stack target → **fix:** reject bound hoặc chuyển loop → **prevent:** tests `0,1,16,17` và resource review.
- **Symptom:** variadic total ngẫu nhiên → **cause:** count/type không khớp default promotions → **diagnose:** đối chiếu caller với `va_arg` → **fix:** đúng protocol hoặc typed array → **prevent:** wrapper typed; không dùng varargs cho batch chính.
- **Giới hạn:** asset đồng bộ, single-process, small batch; không chứng minh thread safety, ABI giữa compiler, WCET hay stack byte trên target embedded.

Nguyên tắc chuyển giao: interface nhỏ nhưng contract đầy đủ; sequence luôn có length; mutation qua pointer là explicit; extension phải được gắn nhãn và có alternative; optimization/resource claim phải đo trên đúng target.

## Practice Time — Không tính điểm, không có lời giải

Tạo file độc lập `practice_b03_gcd.c`; không sửa asset case. Viết CLI tính ước số chung lớn nhất bằng Euclid recursive có bound và báo số invocation.

Yêu cầu:

- Cú pháp: `--gcd A B`; cả hai là `uint32_t` dương.
- Có prototype đầy đủ, parser status + output pointer, recursive helper file-local, base case và bước tiến. Đặt `MAX_GCD_CALLS = 48`: cho phép tối đa 48 invocation; lời gọi định tạo invocation thứ 49 phải return failure trước khi ghi result. Trước recursion phải normalize để operand lớn đứng trước, không tính bước này là invocation. Bound bao phủ toàn miền hai operand `uint32_t` dương: cặp Fibonacci hợp lệ lớn nhất bên dưới `2^32` cần 46 invocation sau normalization, còn `F48` đã vượt `UINT32_MAX`.
- Không dùng macro phép tính, biến global hoặc nested function; nếu dùng callback/context để trace thì callback phải ở file scope.
- `--gcd 84 30` phải stdout đúng:

```text
gcd=6 calls=4
```

- `--gcd 84 -30` phải stdout rỗng, stderr đúng:

```text
error: operands must be positive uint32 values
```

- Boundary workload `--gcd 2971215073 1836311903` phải stdout đúng:

```text
gcd=1 calls=46
```

- Exit của happy/negative/boundary lần lượt `0`, `2`, `0`; strict build zero warnings.
- `--self-test` phải kiểm `(84,30)`, hai số bằng nhau, một operand `1`, invalid `0`, cặp boundary Fibonacci ở **cả hai thứ tự** đều normalize và cho đúng 46 calls, cùng guard nội bộ: khi 48 invocation đã được dùng, yêu cầu tạo invocation thứ 49 trả failure, giữ output không đổi.

`calls` tính cả invocation đầu tiên và invocation base case: `gcd(84,30) → gcd(30,24) → gcd(24,6) → gcd(6,0)` là chính xác bốn. Normalization bắt buộc ở wrapper không được tính là một recursive invocation. Hãy tự chọn signature/status/output design và viết oracle script; tài liệu này không cung cấp solution.

## Provenance của các case

- ISO/IEC 9899:2018 và WG14 N2176: function declaration/call, linkage, pointer/array parameters, `inline`, variadic facilities và recursion behavior.
- `SRC-GCC11`, *GCC 11.4 manuals*, version 11.4.0, *Nested Functions*: nested function là GNU extension; case dùng callback + context portable thay thế.
- `SRC-GCC11`, *GCC 11.4 manuals*, version 11.4.0, *An Inline Function is As Fast As a Macro*: `inline` không phải correctness/performance guarantee.
- `SRC-GLIBC235`, *GNU C Library Reference Manual*, glibc 2.35, Appendix A.2: lifecycle của `va_list`.
- CLI schema, bounds, messages, fixtures và Practice Time là dữ liệu synthetic của `CASE-B03-01`, không phải external API.
