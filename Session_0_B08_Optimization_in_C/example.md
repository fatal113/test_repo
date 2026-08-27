# B08 — Optimization in C — Ví dụ thực thi

> **Case:** `CASE-B08-01` · **LO:** `ADVC-H3SD` · **Increment:** `M00-FND-08`  
> **Ranh giới:** đây là chương trình ISO C17 chạy local. Kết quả đo phụ thuộc máy, toolchain và workload; case **không** khẳng định `-O2` luôn nhanh hơn `-O0`. Correctness là cổng bắt buộc trước mọi nhận xét hiệu năng.

## 🎯 Learning Outcomes liên quan

- [x] `OUT-B08-01` — why to optimize code
- [x] `OUT-B08-02` — trade off between speed and size
- [x] `OUT-B08-03` — Local optimization and global optimazation
- [x] `OUT-B08-04` — Common sub-expression elimination
- [x] `OUT-B08-05` — Constant propagation
- [x] `OUT-B08-06` — Copy propagation
- [x] `OUT-B08-07` — Dead code elimination
- [x] `OUT-B08-08` — Global register allocation
- [x] `OUT-B08-09` — Inline calls
- [x] `OUT-B08-10` — Instruction scheduling
- [x] `OUT-B08-11` — Lifetime analysis
- [x] `OUT-B08-12` — Loop invariant expressions (code motion)
- [x] `OUT-B08-13` — Loop unrolling
- [x] `OUT-B08-14` — Strength reduction
- [x] `OUT-B08-15` — Using compiler options
- [x] `OUT-B08-16` — Analyze code distribution in memory using Linker MAP file
- [x] `OUT-B08-17` — Profile the code

## CASE-B08-01 — Đánh giá một candidate tối ưu mà không đánh đổi correctness

### 1. Ticket và tiêu chí thành công

Nhóm gateway cần biết có nên dùng build tối ưu cho workload checksum hay không. Artifact là [b08_optimization_demo.c](assets/b08_optimization_demo.c). Chương trình nhận `item-count`, tạo chuỗi tính toán xác định và in checksum để ngăn việc lấy timing của một chương trình đã sai hoặc bị loại bỏ toàn bộ.

Case đạt khi:

1. GCC 11.4 biên dịch `-O0` và `-O2` ở C17 strict, không warning.
2. Hai binary nhận `1000` cùng in đúng `OK n=1000 checksum=7BD0A72C` và `diff` exit `0`.
3. Cả hai nhận `0` đều exit `2`, stdout rỗng, stderr đúng một dòng `ERROR item-count must be 1..1000000`.
4. Linker MAP không rỗng; profile report nhận diện được workload, nhưng không dùng một con số timing đơn lẻ để tuyên bố speedup phổ quát.
5. Sanitizer build không phát diagnostic với fixture hợp lệ.
6. GCC decision evidence có oracle bounded cho IRA register assignment/live ranges, post-reload scheduling, LIM, unroll decision và IV strength-reduction analysis; kết luận chỉ mô tả điều report thực sự cho thấy.

### 2. Input, trạng thái ban đầu và ràng buộc

- Toolchain baseline: Ubuntu 22.04, GCC 11.4, Binutils 2.38.
- Input happy: `1000`; negative: `0`; workload tối đa có giới hạn `1..1000000`.
- Phép wrap là chủ ý trên `uint32_t`; không chuyển sang signed arithmetic có overflow undefined.
- `-O0`, `-O2`, `-Os`, profile và sanitizer là các build profile riêng. Không trộn số đo giữa chúng.

### 3. Cách triển khai

Từ thư mục Unit:

```sh
set -eu
CC=gcc
COMMON='-std=c17 -Wall -Wextra -Wpedantic -Werror'

$CC $COMMON -O0 assets/b08_optimization_demo.c -Wl,-Map,b08_O0.map -o b08_O0
$CC $COMMON -O2 assets/b08_optimization_demo.c -Wl,-Map,b08_O2.map -o b08_O2
$CC $COMMON -Os assets/b08_optimization_demo.c -o b08_Os

./b08_O0 1000 > o0.out
./b08_O2 1000 > o2.out
./b08_Os 1000 > os.out
diff -u o0.out o2.out
diff -u o0.out os.out
test -s b08_O0.map
test -s b08_O2.map
```

Oracle stdout của cả ba build:

```text
OK n=1000 checksum=7BD0A72C
```

Negative path được kiểm cả ba kênh:

```sh
set +e
./b08_O2 0 >negative.out 2>negative.err
rc=$?
set -e
test "$rc" -eq 2
test ! -s negative.out
printf '%s\n' 'ERROR item-count must be 1..1000000' >negative.expected
diff -u negative.expected negative.err
```

Sanitizer là correctness evidence, không phải performance build:

```sh
$CC $COMMON -O1 -g -fsanitize=address,undefined \
  -fno-omit-frame-pointer assets/b08_optimization_demo.c -o b08_san
./b08_san 1000
```

Profile build và linker evidence:

```sh
$CC $COMMON -O0 -g -pg assets/b08_optimization_demo.c \
  -Wl,-Map,b08_profile.map -o b08_profile
./b08_profile 1000000 >/dev/null
gprof ./b08_profile gmon.out >b08_gprof.txt
test -s b08_profile.map
test -s b08_gprof.txt
grep -E 'mix_value|compute_checksum' b08_gprof.txt
grep -E '\.(text|data|bss)' b08_O2.map | head
size b08_O0 b08_O2 b08_Os
```

`gprof` có thể báo `0.00` giây nếu sample quá ít. Đó là “chưa đủ độ phân giải”, không phải bằng chứng code mất zero time. Khi được phép trên host, có thể thu thêm `perf stat` nhiều lần; nếu PMU/paranoid policy chặn thì ghi rõ limitation thay vì bịa số.

Compiler decision evidence trên baseline GCC 11.4 được tạo trong thư mục scratch, tách khỏi source:

```sh
mkdir -p b08_evidence
cd b08_evidence

$CC $COMMON -O2 -fno-inline \
  -fopt-info-all=b08.opt \
  -fdump-tree-lim-details \
  -fdump-tree-cunrolli-details \
  -fdump-tree-cunroll-details \
  -fdump-tree-ivopts-details \
  -fdump-rtl-ira \
  -fdump-rtl-sched2 \
  -S ../assets/b08_optimization_demo.c -o b08.s

test -s b08.opt
grep -Eq 'optimized:|missed:' b08.opt
grep -q 'assign reg' ./*r.ira
grep -q 'live ranges' ./*r.ira
grep -q 'basic block' ./*r.sched2
grep -Eq 'Created preheader block|Basic block .*loop' ./*t.lim*
grep -Eq 'Not unrolling|[Uu]nroll' ./*t.cunroll*
grep -q '<Candidate Costs>' ./*t.ivopts

sed -n '/^mix_value:/,/^[.]size[[:space:]]*mix_value/p' b08.s >mix_value.s
test -s mix_value.s
grep -Eq 'sal[lq]?[[:space:]]+[$]4' mix_value.s
grep -Eq 'lea[lq]?' mix_value.s
cd ..
```

Oracle cố ý kiểm **loại evidence và file không rỗng**, không khóa tên register, số basic block, cost number hay toàn bộ assembly. Những chi tiết đó có thể đổi khi compiler/target đổi; baseline/version phải đi kèm report.

| Outline | Evidence GCC 11.4 quan sát được | Giới hạn diễn giải |
|---|---|---|
| `OUT-B08-08` | IRA dump có `assign reg` | chứng minh allocator đã gán; không chứng minh không spill hay speedup |
| `OUT-B08-10` | `sched2` có các `basic block ... after reload` | chứng minh scheduling pass phát report; không khẳng định một thứ tự tối ưu phổ quát |
| `OUT-B08-11` | IRA report có compression của `live ranges` | đây là compiler value live ranges; C object lifetime vẫn được kiểm riêng bằng review/sanitizer |
| `OUT-B08-12` | LIM report tạo preheader và phân tích loop | evidence pass/CFG cụ thể; không tuyên bố mọi invariant đã được hoist |
| `OUT-B08-13` | CUNROLL report ghi `Not unrolling loop` trên workload động | đây là quyết định **không unroll**, không được viết thành “unrolling đã tăng tốc” |
| `OUT-B08-14` | IVOPTS có candidates/cost; x86-64 baseline assembly dùng shift `4` + `lea` cho `index*17+3` | transformation target-specific; correctness vẫn là checksum O0/O2 |

Report `b08.opt` đồng thời cho thấy quyết định inline/missed-inline (`OUT-B08-09`). Nếu một pattern không xuất hiện ở compiler/target khác, kết quả là “evidence profile khác”, không phải lý do nới oracle bằng một tuyên bố không quan sát được.

### 4. Phân tích cơ chế và trade-off

- **Mục tiêu, speed/size (`OUT-B08-01..02`).** Checksum khóa behavior; `size` mô tả sections của binary cụ thể. Candidate chỉ được giữ khi metric đã chọn cải thiện trong workload đại diện và không phá positive/negative oracle.
- **Phạm vi optimizer (`OUT-B08-03..07`).** Compiler có thể CSE biểu thức `shared`, lan constants, bỏ copies/dead temporaries và phân tích xuyên function. Validation path vẫn observable qua stderr/exit code nên không được mất.
- **Backend (`OUT-B08-08..11`).** IRA/sched2 reports cung cấp evidence thật về assignment, live-range analysis và scheduled basic blocks. Không có oracle kiểu “phải dùng thanh ghi X”; exact register/count không bị hard-code.
- **Loop (`OUT-B08-12..14`).** LIM/CUNROLL/IVOPTS cho thấy pass và quyết định thật: preheader được tạo, loop này không unroll, strength-reduction candidates được cost và baseline assembly dùng shift+LEA. Không biến một quyết định “không unroll” thành claim tối ưu thành công.
- **Tool evidence (`OUT-B08-15..17`).** Flags là một phần artifact; MAP trả lời symbol/section được đặt ở đâu; profiler trả lời thời gian/sample tập trung ở đâu. Không công cụ nào tự quyết định business trade-off.

### 5. Failure modes và cách xử lý

| Dấu hiệu | Nguyên nhân khả dĩ | Chẩn đoán | Khắc phục | Phòng tránh |
|---|---|---|---|---|
| O0/O2 khác checksum | UB, uninitialized state hoặc arithmetic sai miền | sanitizer, warnings, giảm input, so output | sửa semantics trước khi đo | dùng fixed-width unsigned cho wrap chủ ý và regression oracle |
| Timing dao động mạnh | workload ngắn, scheduler/cache noise | chạy lặp, ghi host/load, xem phân bố | tăng bounded workload/repetitions | lưu command, environment và thống kê thay vì một lần chạy |
| Binary nhỏ hơn nhưng chậm hơn | inline/layout/cache trade-off | `size`, MAP và profiler cùng workload | chọn profile theo constraint thật | không dùng file size làm proxy duy nhất |
| Profile rỗng/0.00 | sampling chưa đủ hoặc build sai `-pg` | kiểm `gmon.out`, command và report | workload dài hơn; tool phù hợp hơn | tách profile build và smoke-check report |
| MAP không thấy symbol helper | helper đã inline/local/garbage-collected | xem flags, symbol table, disassembly | dùng debug/profile build phù hợp | không đặt oracle vào việc symbol local luôn tồn tại |

### 6. Kết quả đã tái lập trên baseline

Trong container Ubuntu 22.04 có GCC 11.4/Binutils 2.38:

- C17 strict `-O0` và `-O2`: exit `0`, zero warnings.
- `diff` O0/O2 với `1000`: exit `0`; exact stdout `OK n=1000 checksum=7BD0A72C`.
- Negative `0`: exit `2`, stdout rỗng, exact stderr như ticket.
- ASan+UBSan: exit `0`, không diagnostic thuộc fixture.
- MAP O2: không rỗng; `gprof` report không rỗng và chứa `mix_value`/`compute_checksum`. Thời gian sample ngắn không được diễn giải thành speedup.
- GCC dumps: IRA `assign reg`/`live ranges`, sched2 basic blocks, LIM preheader, explicit `Not unrolling`, IVOPTS candidate costs đều match bounded oracle; x86-64 assembly của `mix_value` có shift+LEA cho nhân 17.

## Practice Time — độc lập, không chấm điểm

Bạn chịu trách nhiệm đánh giá một input mới, **không sửa source và không xem lời giải hoàn chỉnh**.

1. Build ba candidate `-O0`, `-O2`, `-Os` bằng strict flags; mỗi candidate có tên riêng.
2. Chạy input `2500`; exact oracle cho mọi candidate là:

   ```text
   OK n=2500 checksum=F2163101
   ```

3. Chứng minh ba output giống nhau bằng `diff`; chạy thêm invalid input `1000001`, yêu cầu exit `2`, stdout rỗng và exact stderr `ERROR item-count must be 1..1000000`.
4. Thu `size` và ít nhất 9 lần timing cho mỗi candidate trên cùng máy. Báo median cùng min/max; nêu rõ noise và environment.
5. Viết quyết định tối đa 120 từ: candidate nào phù hợp nếu `.text` là constraint, candidate nào đáng benchmark thêm nếu latency là constraint, và vì sao dữ liệu này **không** chứng minh một optimization level luôn thắng.

Acceptance: build sạch, exact oracles pass, bảng dữ liệu có command/environment, kết luận có giới hạn; không có solution code hoặc ngưỡng speedup được cho sẵn.

## Provenance của các case

- `SRC-C17-ISO`, `SRC-C17-WG14`: semantics C17 và observable behavior.
- `SRC-GCC11`: optimization/compiler options GCC 11.4.
- `SRC-LD238`, `SRC-BINUTILS238`: linker MAP, `size` và symbol/section tools.
- `SRC-GPROF238`, `SRC-LINUX-PERF515`: profiling workflow và giới hạn công cụ.
- `SRC-LLVM14`: sanitizer/toolchain đối chiếu.
