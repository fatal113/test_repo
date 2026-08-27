# Hands-on Lab LAB-P00-01 — Portable Record Analyzer CLI

> **Phần:** Optional C Basics Refresher · **Vị trí thực hiện:** sau B10 · **Chuẩn:** ISO C17 · **Thời lượng:** 180 phút · **Hình thức:** guided with fading · **Đánh giá:** không chấm điểm

## 🎯 Learning Outcomes được thực hành

Sau Lab, học viên có thể tích hợp các increment B01–B10 thành một CLI xử lý record synthetic và tạo bằng chứng tái lập cho hai Learning Outcome:

- `ADVC-H1SD`: public contract, ownership, collection, sort/search và callback không gây lỗi bộ nhớ trong bộ fixture đã công bố.
- `ADVC-H3SD`: strict build, negative tests, sanitizer/Valgrind, linker MAP và profiling evidence dẫn tới một quyết định có căn cứ.

Lab tạo evidence cho milestone `M00-FND-10` và readiness gate `M00-RDY`. Nó không phải Assignment, không thay đổi `Assignment_01..09`, và không cung cấp implementation hoàn chỉnh.

## Ranh giới portable C

CLI chỉ đọc starter file local, chỉ ghi các output mới dưới `lab06_scratch`, và chỉ xử lý dữ liệu synthetic; ba fixture trong `assets/` luôn read-only. Không có microcontroller, peripheral register, địa chỉ phần cứng, HAL, RTOS, ISR, memory-mapped I/O hoặc flashing. ISO C17 quy định behavior của chương trình; GCC, GNU ld, GDB, Valgrind và gprof chỉ là lớp công cụ GNU/Linux dùng để thu bằng chứng. Linker MAP không phải một phần của chuẩn C.

## 📋 Lab outline và Definition of Done

Một nhóm phát triển cần công cụ `record_tool` để:

1. đọc record từ CSV có header `id,value,flags`;
2. lưu record trong collection sở hữu bộ nhớ;
3. sắp xếp, tìm kiếm và lọc bằng callback có context;
4. kiểm tra một profile Motorola S-record giới hạn;
5. cho cùng observable output ở build `-O0` và `-O2`;
6. xuất evidence đủ để reviewer phân biệt lỗi chương trình với khác biệt công cụ.

Definition of Done:

- bốn checkpoint `CP-P00-10..40` đều có command, exit code, output và file evidence hiện tại;
- strict build không warning;
- valid và invalid fixture khớp đúng contract;
- AddressSanitizer/UndefinedBehaviorSanitizer và Valgrind không báo finding trên ít nhất một path được chấp nhận và một path bị từ chối đúng contract;
- output `-O0` và `-O2` giống byte-for-byte trên cùng workload;
- không có thao tác ghi/xóa ngoài `lab06_scratch`;
- mọi claim về performance có measurement kèm điều kiện chạy, không suy rộng thành quy luật phổ quát.

## Điều kiện bắt đầu

Đã học B01–B10 hoặc chứng minh năng lực tương đương. Cần Ubuntu 22.04/WSL2 hoặc Linux tương thích với:

```bash
gcc --version
gdb --version
valgrind --version
size --version
gprof --version
cmake --version
sha256sum --version
cmp --version
```

Profile biên dịch bắt buộc:

```bash
-std=c17 -Wall -Wextra -Wpedantic -Werror
```

Không tiếp tục nếu compiler không nhận `-std=c17`, starter asset thiếu, hoặc working directory không phải folder Unit B10.

## Starter assets và smoke check

Các asset hoàn chỉnh của B01–B10 là ví dụ tham khảo hành vi, không phải lời giải tích hợp. Lab cấp ba fixture:

- `assets/lab06_records.csv`: bốn record hợp lệ;
- `assets/lab06_sample.srec`: một S1 data record và một S9 termination record có checksum đúng;
- `assets/lab06_bad_checksum.srec`: cùng payload nhưng checksum dòng đầu sai.

Khóa baseline trước khi tạo scratch:

```bash
mkdir -p lab06_scratch/evidence
sha256sum assets/lab06_records.csv \
  assets/lab06_sample.srec \
  assets/lab06_bad_checksum.srec \
  > lab06_scratch/evidence/starter.sha256
sha256sum -c lab06_scratch/evidence/starter.sha256
```

Expected: ba dòng `OK`, command exit `0`. Nếu hash check fail ở lần chạy sau, recopy đúng file được nêu tên; không sửa fixture gốc.

## Contract dữ liệu

### CSV profile

- Dòng đầu chính xác `id,value,flags`.
- Tối đa 64 data rows; mỗi physical line tối đa 127 byte không tính `\0`.
- `id`: `uint32_t`, lớn hơn 0 và duy nhất.
- `value`: `int`, trong `[-1000,1000]`.
- `flags`: cú pháp integer được `strtoul(..., 0)` chấp nhận, nhưng phải nằm trong `uint32_t`.
- Không chấp nhận field rỗng, ký tự dư, dòng bị cắt, duplicate ID hoặc trailing comma.
- Parser là failure-atomic ở cấp collection: khi toàn file lỗi, caller không được nhận collection được báo là thành công một phần.

Record public model tối thiểu phải biểu đạt được:

```c
typedef struct {
    uint32_t id;
    int value;
    uint32_t flags;
} record_t;
```

Đây là contract kiểu, không phải chỉ định layout tuần tự hóa. Padding và byte order không được dùng để đọc/ghi CSV hay S-record bằng cách dump raw `struct`.

### S-record profile của Lab

Lab chỉ yêu cầu ASCII uppercase, không whitespace, tối đa 78 ký tự mỗi dòng, và hai record type:

- `S1`: address 16-bit, có thể chứa data;
- `S9`: address 16-bit, kết thúc file và không có data.

`count` đếm address bytes + data bytes + checksum byte. Tổng modulo 256 của `count`, address, data và checksum phải bằng `0xFF`. Profile yêu cầu **ít nhất một S1 có ít nhất một data byte**, sau đó đúng một `S9` nằm cuối file. Vì vậy S9-only và S1 rỗng đều là input lỗi. Checksum chỉ phát hiện lỗi truyền ngẫu nhiên; nó không phải chữ ký hoặc cryptographic integrity.

Fixture hợp lệ:

```text
S107001001020304DE
S9030000FC
```

Dòng S1 có `count=0x07`, address `0x0010`, bốn data byte `01 02 03 04`, checksum `DE`. Tổng byte từ count đến checksum là `0xFF` modulo 256.

## Public behavior cần đạt

Happy CSV command:

```bash
./lab06_scratch/build/record_tool \
  --csv assets/lab06_records.csv --min 20 --flag 0x01
```

Expected exit `0`, stderr rỗng, stdout chính xác:

```text
OK loaded=4 selected=1 min=21 max=21 mean=21.00 flags=0x03
```

Happy S-record command:

```bash
./lab06_scratch/build/record_tool --srec assets/lab06_sample.srec
```

Expected exit `0`, stderr rỗng:

```text
OK srec records=2 data_bytes=4 entry=0x00000000
```

Bad-checksum command phải exit `3`, stdout rỗng và stderr chính xác:

```text
ERROR srec line=1 code=checksum
```

Sai CSV syntax/range/duplicate/overlong phải exit `2`, stdout rỗng và dùng một mã lỗi ổn định, ví dụ `ERROR csv line=3 code=duplicate-id`. Không in summary từ dữ liệu parse dở.

## Timeline và mức hướng dẫn

| Khoảng | Mức hỗ trợ | Kết quả |
|---|---|---|
| 0–15 phút | Guided | preflight, hash, folder và behavior contract được khóa |
| 15–55 phút | Guided | CP-P00-10: typed model, parser, function/flag contract |
| 55–100 phút | Guided with fading | CP-P00-20: ownership, collection, algorithms, callback |
| 100–145 phút | Fading | CP-P00-30: file/S-record/error/memory evidence |
| 145–180 phút | Independent handoff | CP-P00-40: O0/O2, MAP, profile, decision và reset |

## CP-P00-10 — Type, array/control, function và bit-flag contract

**LO:** `ADVC-H1SD`.

**Action.** Tạo `include/record.h`, `src/record.c`, `src/main.c` và runner `tests/verify_foundation.sh`. Chọn `size_t` cho count/index, fixed-width unsigned type cho bit flags, `enum` cho status, input pointer có `const`, và output pointer cho dữ liệu trả về cùng status.

**Ràng buộc thiết kế.** Không dùng function-like macro để tính biểu thức có side effect. Nếu dùng macro mask, mọi operand/result phải được ngoặc hóa và call site không truyền `index++`. Duyệt mảng với invariant `0 <= index < count`; capacity luôn được truyền riêng qua function boundary.

**Hints giảm dần.**

1. Bắt đầu từ prototype và error/output-on-failure policy trước body.
2. Parse bằng `strtoul`/`strtol` với `errno`, end pointer và range check trước cast.
3. Chỉ commit row vào collection sau khi cả ba field hợp lệ.
4. Khi một negative fixture tạo partial stdout, truy vết nơi summary được gọi thay vì chỉ xóa dòng in.

**Verify.** Runner phải tự build strict, chạy happy fixture, ít nhất bốn invalid classes và so sánh exact stream/exit code.

```bash
sh lab06_scratch/tests/verify_foundation.sh \
  | tee lab06_scratch/evidence/foundation.log
```

**Expected.** Exit `0`; log ghi `foundation: PASS cases=5`; compiler warning count `0`. Runner đồng thời ghi command/diagnostic build vào `lab06_scratch/evidence/build.log`; evidence gồm happy output và bốn negative result. Nếu runner chỉ báo “pass” mà không lưu expected/actual, checkpoint chưa đạt.

## CP-P00-20 — Ownership, structure, algorithm và callback

**LO:** `ADVC-H1SD`.

**Action.** Thay storage cố định bằng vector owned có `data/size/capacity`; thêm sort theo `value,id`, lookup theo `id`, và filter callback nhận một context bất biến gồm `min_value` và `required_flags`.

Contract callback gợi ý ở mức interface:

```c
typedef int (*record_predicate_fn)(const record_t *record,
                                   const void *context);
```

Interface không tiết lộ cách triển khai vector. Học viên tự quyết định API reserve/push/free và viết ownership table: ai tạo, ai sở hữu, khi nào chuyển giao, khi nào giải phóng. Muốn kiểm failure-atomic growth, test build có thể inject một allocator wrapper được cấu hình fail ở lần gọi xác định; không sửa global allocator của process.

**Hints giảm dần.**

1. `realloc` phải đi qua temporary pointer; chỉ cập nhật `data/capacity` sau success.
2. Comparator của `qsort` trả giá trị âm/0/dương bằng so sánh có nhánh, không dùng `left->value - right->value` vì có thể overflow.
3. `bsearch` chỉ đúng khi mảng đã được sắp theo cùng comparator/key contract.
4. Context callback phải sống lâu hơn mọi lần gọi; callback không được lưu pointer tới local sắp hết lifetime.

**Verify.**

```bash
sh lab06_scratch/tests/verify_collection.sh \
  | tee lab06_scratch/evidence/collection.log
```

**Expected.** Exit `0`; log ghi `collection: PASS cases=7`. Bảy case phải gồm empty, one, max 64, duplicate ID, filter none/many và injected allocation failure. Sau failure, hash của collection trước/sau giống nhau; cleanup gọi đúng một lần cho mỗi allocation owned.

## CP-P00-30 — File/error hardening và memory-tool evidence

**LO:** `ADVC-H1SD`, `ADVC-H3SD`.

**Action.** Hoàn thiện đọc CSV bằng `fgets`, phát hiện line truncation, phân biệt EOF với I/O error, đóng mọi `FILE *`, viết validator cho S1/S9 profile, và thêm text/binary write→flush→close→reopen verification chỉ dưới scratch. Tạo matrix nối input class → status → stdout/stderr → cleanup path.

**Quy tắc stream.** `fopen` success mới tạo stream; mỗi stream được `fclose` đúng một lần. `feof`/`ferror` chỉ được hỏi sau khi read không trả dữ liệu như mong muốn. Không gọi `fflush` trên input stream để “xóa input”; ISO C không định nghĩa cách dùng đó. Nếu ghi evidence file, kiểm cả lỗi `fprintf`, `fflush` và `fclose` vì lỗi có thể xuất hiện muộn khi buffer được đẩy xuống hệ thống.

**Verify functional.**

```bash
sh lab06_scratch/tests/verify_files_and_memory.sh \
  | tee lab06_scratch/evidence/files-memory.log
```

Runner phải kiểm đúng 13 case: valid CSV, malformed field, overlong line, duplicate ID, valid S-record, bad hex, bad count, bad checksum, missing S9, S9-only, empty-S1, data-after-S9 và write-roundtrip. Kết thúc thành công, runner exit `0` và chỉ in `files: PASS cases=13` ra stdout. Mỗi rejected input phải có stdout rỗng và exact status/diagnostic sau:

| Input class | Exit | Exact stderr |
|---|---:|---|
| malformed CSV field | 2 | `ERROR csv line=2 code=syntax` |
| overlong CSV line | 2 | `ERROR csv line=2 code=line-too-long` |
| duplicate CSV ID | 2 | `ERROR csv line=3 code=duplicate-id` |
| bad S-record hex | 3 | `ERROR srec line=1 code=hex` |
| bad S-record count | 3 | `ERROR srec line=1 code=count` |
| bad S-record checksum | 3 | `ERROR srec line=1 code=checksum` |
| missing S9 | 3 | `ERROR srec line=EOF code=missing-s9` |
| S9-only | 3 | `ERROR srec line=1 code=missing-s1` |
| empty S1 | 3 | `ERROR srec line=1 code=empty-s1` |
| data after S9 | 3 | `ERROR srec line=3 code=data-after-s9` |

Write-roundtrip là phần bắt buộc của case thứ 13. Thêm mode `--write-evidence <text-path> <binary-path>`; runner chỉ truyền hai path mới dưới `lab06_scratch/output/`. Mode phải dùng `fprintf` với text stream, `fwrite` với binary stream, kiểm `fflush` và `fclose`, reopen bằng `r`/`rb`, rồi so đúng nội dung trước khi báo success. Text payload chính xác là `status=ready\nrecords=4\n` (23 byte); binary payload là bốn byte `43 31 37 00`. Exact stdout là `OK write-roundtrip text_bytes=23 binary_bytes=4 flush=ok reopen=match`, stderr rỗng, exit `0`. Không ghi đè starter fixture và không gọi `fflush` trên input stream.

**Verify sanitizer.**

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror \
  -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I lab06_scratch/include lab06_scratch/src/*.c \
  -o lab06_scratch/build/record_tool_asan
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
  ./lab06_scratch/build/record_tool_asan \
  --csv assets/lab06_records.csv --min 20 --flag 0x01 \
  >lab06_scratch/evidence/asan.out \
  2>lab06_scratch/evidence/asan.err
```

Expected exit `0`, `asan.err` rỗng và stdout đúng happy oracle.

Chạy thêm sanitizer trên bad-checksum fixture, bắt riêng application status:

```bash
set +e
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
  ./lab06_scratch/build/record_tool_asan \
  --srec assets/lab06_bad_checksum.srec \
  >lab06_scratch/evidence/asan-invalid.out \
  2>lab06_scratch/evidence/asan-invalid.err
asan_invalid_rc=$?
set -e
test "$asan_invalid_rc" -eq 3
test ! -s lab06_scratch/evidence/asan-invalid.out
grep -Fx 'ERROR srec line=1 code=checksum' \
  lab06_scratch/evidence/asan-invalid.err
! grep -Eq 'AddressSanitizer:|runtime error:' \
  lab06_scratch/evidence/asan-invalid.err
```

Expected: application rejection vẫn đúng exit/output; log không có `AddressSanitizer:` hoặc `runtime error:`. Diagnostic nghiệp vụ được phép xuất hiện và không được gọi nhầm là sanitizer finding.

**Verify Valgrind.**

```bash
valgrind --tool=memcheck --leak-check=full \
  --errors-for-leak-kinds=definite,indirect \
  --error-exitcode=99 \
  ./lab06_scratch/build/record_tool \
  --srec assets/lab06_sample.srec \
  >lab06_scratch/evidence/valgrind.out \
  2>lab06_scratch/evidence/valgrind.log
```

Expected application exit `0`, summary stdout đúng, Valgrind exit `0`, `ERROR SUMMARY: 0 errors`, và `in use at exit: 0 bytes in 0 blocks`. Đây chỉ chứng minh các fixture đã chạy, không chứng minh chương trình đúng cho mọi input.

Chạy lại Valgrind trên bad-checksum fixture; dùng `set +e` để giữ application exit `3`:

```bash
set +e
valgrind --tool=memcheck --leak-check=full \
  --errors-for-leak-kinds=definite,indirect \
  --error-exitcode=99 \
  ./lab06_scratch/build/record_tool \
  --srec assets/lab06_bad_checksum.srec \
  >lab06_scratch/evidence/valgrind-invalid.out \
  2>lab06_scratch/evidence/valgrind-invalid.log
valgrind_invalid_rc=$?
set -e
test "$valgrind_invalid_rc" -eq 3
test ! -s lab06_scratch/evidence/valgrind-invalid.out
grep -F 'ERROR srec line=1 code=checksum' \
  lab06_scratch/evidence/valgrind-invalid.log
grep -F 'ERROR SUMMARY: 0 errors' \
  lab06_scratch/evidence/valgrind-invalid.log
grep -F 'in use at exit: 0 bytes in 0 blocks' \
  lab06_scratch/evidence/valgrind-invalid.log
```

Như vậy CP-P00-30 có memory evidence cho cả một accepted path và một rejected path.

## CP-P00-40 — Optimization evidence, handoff và reset

**LO:** `ADVC-H3SD`.

**Action.** Tạo hai binary từ cùng source và input, khóa observable output trước khi đo:

```bash
set -eu
rm -f -- \
  lab06_scratch/evidence/o0.out \
  lab06_scratch/evidence/o2.out \
  lab06_scratch/evidence/output.diff \
  lab06_scratch/evidence/record_tool_o0.map \
  lab06_scratch/evidence/record_tool_o2.map \
  lab06_scratch/evidence/binaries.sha256

gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O0 -g \
  -I lab06_scratch/include lab06_scratch/src/*.c \
  -Wl,-Map=lab06_scratch/evidence/record_tool_o0.map \
  -o lab06_scratch/build/record_tool_o0

gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 -g \
  -I lab06_scratch/include lab06_scratch/src/*.c \
  -Wl,-Map=lab06_scratch/evidence/record_tool_o2.map \
  -o lab06_scratch/build/record_tool_o2

lab06_scratch/build/record_tool_o0 \
  --csv assets/lab06_records.csv --min 20 --flag 0x01 \
  >lab06_scratch/evidence/o0.out
lab06_scratch/build/record_tool_o2 \
  --csv assets/lab06_records.csv --min 20 --flag 0x01 \
  >lab06_scratch/evidence/o2.out
printf '%s\n' \
  'OK loaded=4 selected=1 min=21 max=21 mean=21.00 flags=0x03' \
  >lab06_scratch/evidence/happy.expected
cmp -s lab06_scratch/evidence/happy.expected \
       lab06_scratch/evidence/o0.out
cmp -s lab06_scratch/evidence/happy.expected \
       lab06_scratch/evidence/o2.out
if cmp -s lab06_scratch/evidence/o0.out \
          lab06_scratch/evidence/o2.out; then
  printf '%s\n' 'MATCH o0=o2' \
    >lab06_scratch/evidence/output.diff
else
  diff -u lab06_scratch/evidence/o0.out \
          lab06_scratch/evidence/o2.out \
    >lab06_scratch/evidence/output.diff
  exit 1
fi
test -s lab06_scratch/evidence/record_tool_o0.map
test -s lab06_scratch/evidence/record_tool_o2.map
size lab06_scratch/build/record_tool_o0 \
     lab06_scratch/build/record_tool_o2 \
  >lab06_scratch/evidence/size.txt
```

Expected: cả hai process exit `0`; hai output khớp happy oracle; `output.diff` chứa chính xác `MATCH o0=o2`; hai MAP file không rỗng. Kích thước có thể tăng hoặc giảm tùy toolchain; chỉ ghi số đã đo.

Profile là evidence bắt buộc của checkpoint này. Build riêng với `-pg`, chạy workload synthetic đã khóa, rồi tạo report từ đúng binary vừa chạy:

```bash
set -eu
rm -f -- \
  lab06_scratch/gmon.out \
  lab06_scratch/build/record_tool_pg \
  lab06_scratch/evidence/profile.expected \
  lab06_scratch/evidence/profile.txt \
  lab06_scratch/evidence/profile_workload.out
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 -g -pg \
  -I lab06_scratch/include lab06_scratch/src/*.c \
  -o lab06_scratch/build/record_tool_pg

(
  set -eu
  cd lab06_scratch
  ./build/record_tool_pg \
    --csv ../assets/lab06_records.csv --min 20 --flag 0x01 \
    >evidence/profile_workload.out
  printf '%s\n' \
    'OK loaded=4 selected=1 min=21 max=21 mean=21.00 flags=0x03' \
    >evidence/profile.expected
  cmp -s evidence/profile.expected evidence/profile_workload.out
  test -s gmon.out
  gprof build/record_tool_pg gmon.out >evidence/profile.txt
  test -s evidence/profile.txt
  grep -Eq '(^|[[:space:]])main([[:space:]]|$)' evidence/profile.txt
)

sha256sum \
  lab06_scratch/build/record_tool_o0 \
  lab06_scratch/build/record_tool_o2 \
  lab06_scratch/build/record_tool_pg \
  lab06_scratch/evidence/record_tool_o0.map \
  lab06_scratch/evidence/record_tool_o2.map \
  lab06_scratch/evidence/profile.txt \
  >lab06_scratch/evidence/binaries.sha256
sha256sum -c lab06_scratch/evidence/binaries.sha256
```

Expected: workload exit `0` và khớp happy oracle; `gmon.out` cùng `profile.txt` không rỗng; report có symbol ứng dụng `main`; sáu dòng hash đều được `sha256sum -c` xác nhận `OK`. Cột thời gian có thể bằng `0.00` nếu workload quá ngắn, vì vậy chỉ dùng report này để chứng minh quy trình profiling và phân bố quan sát được, không tuyên bố speedup.

`optimization_decision.md` phải ghi compiler/CPU/input, số lần chạy, metric, correctness check, observation và quyết định giữ/không giữ thay đổi. Không được kết luận “`-O2` luôn nhanh hơn” hoặc diễn giải số từ một lần chạy như guarantee.

**Verify.**

```bash
sh lab06_scratch/tests/verify_handoff.sh \
  | tee lab06_scratch/evidence/handoff.log
```

Expected exit `0`; log ghi `handoff: PASS`; output equivalence, current binary hash, MAP/profile và decision note đều resolve.

## Troubleshooting matrix

| Dấu hiệu | Nguyên nhân thường gặp | Bằng chứng cần xem | Sửa và phòng tránh |
|---|---|---|---|
| Build chỉ pass khi bỏ `-Werror` | mismatch type/format hoặc extension GNU | compiler diagnostic đầy đủ | sửa contract kiểu; không cast để che warning; giữ strict profile trong CI |
| Invalid CSV vẫn in summary | commit dữ liệu/output trước khi toàn input hợp lệ | stdout và call trace | parse vào temporary collection; chỉ swap/commit khi success |
| `realloc` fail làm mất buffer cũ | gán trực tiếp return vào owner pointer | injected-failure test và pointer trace | dùng temporary pointer; commit sau success |
| `bsearch` không tìm thấy ID tồn tại | sort/search dùng comparator khác nhau | dump order và key comparator | khóa một comparator contract; test zero/one/many |
| S-record valid bị báo checksum | count bao gồm sai trường hoặc hex decode lỗi | log từng byte và running sum | count address+data+checksum; assert modulo sum `0xFF` |
| `feof` khiến bỏ dòng cuối | kiểm EOF trước read | iteration trace | gọi read trước; phân loại sau khi read fail |
| O0/O2 output khác | UB, uninitialized data hoặc unstable comparator | sanitizer, diff và warning | dừng đo; sửa correctness rồi chạy lại từ clean build |
| Valgrind báo still reachable | cleanup path không chạy hoặc library giữ state | allocation backtrace | phân loại finding; sửa owner cleanup; không blanket-suppress |

## Deliverables

- `lab06_scratch/include/record.h` và source dưới `lab06_scratch/src/`;
- bốn runner `verify_foundation.sh`, `verify_collection.sh`, `verify_files_and_memory.sh`, `verify_handoff.sh`;
- input fixtures mới của học viên, khác ba starter fixtures nhưng theo cùng profile;
- `lab06_scratch/evidence/` chứa build, functional, negative, sanitizer, Valgrind, MAP, size và profile evidence;
- ownership table, error-path matrix và `optimization_decision.md`;
- `README.md` có build, run, test và reset command.

## 🧪 Final validation

Reviewer xác nhận bằng evidence, không dựa vào lời mô tả:

1. mỗi checkpoint map tới ít nhất một LO và một command có expected result;
2. example assets chỉ được tham khảo, không bị sửa để làm giả evidence;
3. mọi negative case phân biệt expected rejection với crash/hang;
4. memory-tool claim nêu đúng fixture đã chạy;
5. MAP/profile thuộc binary có hash được ghi trong cùng lần chạy;
6. Practice/Lab không chứa secret, PII hoặc hardware dependency;
7. evidence có timestamp/tool version và có thể tái tạo từ scratch.

## ♻️ Reset, cleanup và khả năng chạy lại

Chỉ reset trong folder đã xác minh. Lệnh sau cố ý xóa toàn bộ build, output, profile và evidence sinh ra trong **đúng** `lab06_scratch`; hãy copy evidence cần bàn giao ra nơi đã duyệt trước khi chạy:

```bash
set -eu
test -d lab06_scratch
test -d lab06_scratch/build
test -d lab06_scratch/evidence
test ! -L lab06_scratch
test "$(basename "$PWD")" = 'Session_0_B10_File_Handling'
cmake -E remove_directory lab06_scratch/build
cmake -E remove_directory lab06_scratch/evidence
if test -d lab06_scratch/output; then
  cmake -E remove_directory lab06_scratch/output
fi
rm -f -- lab06_scratch/gmon.out
mkdir -p lab06_scratch/build
mkdir -p lab06_scratch/evidence
mkdir -p lab06_scratch/output
test ! -e lab06_scratch/gmon.out
test -z "$(find lab06_scratch/build -mindepth 1 -maxdepth 1 -print -quit)"
test -z "$(find lab06_scratch/evidence -mindepth 1 -maxdepth 1 -print -quit)"
test -z "$(find lab06_scratch/output -mindepth 1 -maxdepth 1 -print -quit)"
printf '%s\n' 'reset: PASS build=empty evidence=empty output=empty gmon=absent'
```

Expected sau reset: block exit `0` và in chính xác `reset: PASS build=empty evidence=empty output=empty gmon=absent`; ba folder tồn tại/rỗng và `lab06_scratch/gmon.out` không tồn tại. Chạy lại smoke check rồi bốn runner sẽ tái tạo evidence mới. Không dùng biến rỗng, glob rộng hoặc đường dẫn cha. Giữ/copy `lab06_scratch/evidence` khi bàn giao trước reset; sau khi trainer xác nhận, người học có thể xóa đúng folder scratch bằng công cụ quản lý file của workspace. Ba fixture trong `assets/` là immutable baseline và không bị cleanup.

## Nguồn và provenance

- `SRC-USER-CREF`: outline và quyết định phân loại Practice Time do người dùng duyệt ngày 2026-08-22.
- `SRC-C17-ISO` và `SRC-C17-WG14`: semantics của object, pointer, function, preprocessing, allocation và stream trong C17.
- `SRC-CERTC`: quy tắc an toàn cho integer, array, string, allocation, I/O và error handling.
- `SRC-GLIBC235`: hành vi thư viện C trong baseline Ubuntu của khóa; chỉ facility ISO C được coi là portable.
- `SRC-MOTOROLA-SREC`: cấu trúc S-record cổ điển; Lab dùng một profile S1/S9 giới hạn và không tuyên bố hỗ trợ mọi biến thể công cụ.

Toàn bộ scenario/data là simulated và synthetic. Lab không đại diện dự án khách hàng thật.
