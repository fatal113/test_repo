# B09 — Common C Defects — Ví dụ thực thi

> **Case:** `CASE-B09-01` · **LO:** `ADVC-H3SD` · **Increment:** `M00-FND-09`  
> **Ranh giới:** executable chỉ chạy safe checked paths ISO C17. Anti-pattern UB được phân tích chứ không được kích hoạt. GCC/Clang sanitizer và Valgrind là công cụ chẩn đoán theo môi trường local, không phải bảo đảm của chuẩn C.

## 🎯 Learning Outcomes liên quan

- [x] `OUT-B09-01` — Alignment and packing
- [x] `OUT-B09-02` — Macro, function like macro
- [x] `OUT-B09-03` — Overflow and underflow
- [x] `OUT-B09-04` — pointer to const and const pointer
- [x] `OUT-B09-05` — signed and usigned type
- [x] `OUT-B09-06` — string and characters
- [x] `OUT-B09-07` — Precedence
- [x] `OUT-B09-08` — Loosing dynamic allocated memory

## CASE-B09-01 — Harden bộ thu record trước defect dữ liệu và bộ nhớ

### 1. Ticket và tiêu chí thành công

Một utility local nhận số record và label từ CLI, tạo values trên heap, tính tổng, giải mã word little-endian và kiểm flags. Stakeholder cần artifact nhỏ để tái lập policy kiểm dữ liệu và cleanup trước khi ghép vào gateway.

Artifact: [b09_defects_demo.c](assets/b09_defects_demo.c). Case đạt khi:

- strict C17 build không warning;
- input `4 sensor-A` exit `0` và stdout exact;
- count vượt biên hoặc label sai policy exit `2`, stdout rỗng, stderr exact;
- `--overflow-test` thực thi checked-add tại `UINT32_MAX+1`, chứng minh rejection transactional bằng exact oracle;
- ASan+UBSan không diagnostic; Valgrind báo zero error/leak cho happy fixture;
- mỗi mục `OUT-B09-01..08` truy vết được tới một code decision và oracle.

### 2. Thiết kế và quyết định

| Outline | Quyết định trong asset | Evidence |
|---|---|---|
| `OUT-B09-01` | decode hai bytes bằng shift/OR, không cast raw storage | `word=4660` từ `34 12` |
| `OUT-B09-02` | `ARRAY_COUNT` chỉ dùng tại local array | internal contract giữ count `2` |
| `OUT-B09-03` | guard allocation multiplication và `checked_add_u32`; CLI `--overflow-test` gọi đúng helper ở max+1 | exact `overflow-rejected ... output-unchanged=123` |
| `OUT-B09-04` | `const uint32_t * const` cho read-only view | loop không mutate/reseat view |
| `OUT-B09-05` | `strtoul` + consumed-all + range trước cast | `9` bị reject, exit `2` |
| `OUT-B09-06` | label capacity/policy + NUL copy; `<ctype.h>` nhận unsigned char | `sensor-A` pass; `bad_label` fail |
| `OUT-B09-07` | `(flags & 0x01U) == 0U` có grouping rõ | happy output `mask=3` |
| `OUT-B09-08` | một owner, cleanup label, `free` đúng một lần | Valgrind zero leak |

Alternative raw struct cast ngắn hơn nhưng không portable về endian/alignment/layout. Fixed local array bỏ allocation failure nhưng không minh họa ownership; heap được giữ với maximum nhỏ và guard đầy đủ. Silent label truncation bị loại vì hai identifier khác nhau có thể trở thành cùng label.

### 3. Build và happy oracle

Từ thư mục Unit:

```sh
set -eu
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  assets/b09_defects_demo.c -o b09_demo

./b09_demo 4 sensor-A >happy.out 2>happy.err
test ! -s happy.err
printf '%s\n' \
  'OK count=4 sum=100 label=sensor-A word=4660 mask=3' >happy.expected
diff -u happy.expected happy.out
```

Exact result:

```text
OK count=4 sum=100 label=sensor-A word=4660 mask=3
```

`count=4` tạo `10,20,30,40`; checked total là `100`. Bytes `0x34,0x12` được giải mã little-endian thành `0x1234 = 4660`. Flags `3` có bit thấp nên internal contract pass.

### 4. Negative oracles

Count vượt giới hạn:

```sh
set +e
./b09_demo 9 sensor-A >count.out 2>count.err
count_rc=$?
set -e
test "$count_rc" -eq 2
test ! -s count.out
printf '%s\n' 'ERROR count must be 1..8' >count.expected
diff -u count.expected count.err
```

Label vi phạm alphabet:

```sh
set +e
./b09_demo 4 bad_label >label.out 2>label.err
label_rc=$?
set -e
test "$label_rc" -eq 2
test ! -s label.out
printf '%s\n' \
  'ERROR label must be 1..15 alnum-or-dash characters' >label.expected
diff -u label.expected label.err
```

Không negative path nào tiếp tục allocation sau validation failure. Exit `64` được dành cho sai số lượng argument; exit `3` cho lỗi resource/internal sau validation.

Overflow rejection path là self-test có thể chạy, không dựa vào việc happy fixture tình cờ không overflow:

```sh
./b09_demo --overflow-test >overflow.out 2>overflow.err
test ! -s overflow.err
printf '%s\n' \
  'OK overflow-rejected left=4294967295 right=1 output-unchanged=123' \
  >overflow.expected
diff -u overflow.expected overflow.out
```

Exit phải là `0`: self-test pass khi `checked_add_u32` trả rejection và sentinel `123` không bị sửa. Nếu helper nhận phép cộng hoặc ghi output trước khi reject, self-test in `ERROR overflow was not rejected transactionally` ở stderr và exit `3`.

### 5. Diagnostics và memory oracle

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O1 -g \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  assets/b09_defects_demo.c -o b09_san
./b09_san 4 sensor-A
./b09_san 8 node-Z
./b09_san --overflow-test

gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O0 -g \
  assets/b09_defects_demo.c -o b09_valgrind
valgrind --leak-check=full --show-leak-kinds=all \
  --errors-for-leak-kinds=all --error-exitcode=99 \
  ./b09_valgrind 4 sensor-A
```

Pass oracle: hai sanitizer runs giữ exact functional output và không diagnostic; Valgrind process exit `0`, `ERROR SUMMARY: 0 errors`, không “definitely lost”. Tool không thay thế boundary tests: một chương trình có policy sai nhưng truy cập memory hợp lệ vẫn có thể sanitizer-clean.

### 6. Failure modes và troubleshooting

| Dấu hiệu | Nguyên nhân | Chẩn đoán | Sửa | Phòng tránh |
|---|---|---|---|---|
| `word` đổi trên architecture khác | raw cast/endian/layout assumption | xem fixture bytes và decode path | byte-wise decode theo format | golden byte vectors, cấm raw struct serialization |
| Count âm/lớn đi qua validation | mixed signed/unsigned hoặc parser không consumed-all | warning conversion + boundary inputs | parse/range rồi cast | tests `0,1,8,9`, junk suffix |
| Label gây over-read | thiếu capacity/NUL contract | ASan, inspect length | reject oversize, copy `length+1` | pointer+capacity API |
| Mask branch sai | precedence grouping | truth table, warnings/preprocessed expression | `(flags & MASK) == 0U` | parentheses rule cho bitwise+compare |
| Memcheck báo lost block | return/overwrite owner trước free | leak stack trace và path audit | single cleanup/explicit transfer | document owner và failure-injection test |
| O2-only result khác | signed UB, lifetime hoặc unsequenced side effect | UBSan, O0/O2 diff, macro expansion | sửa defined semantics | strict warnings + sanitizer regression |

### 7. Kết quả đã tái lập trên baseline

Trong container Ubuntu 22.04 với GCC 11.4/Valgrind 3.18.1:

- strict build: exit `0`, zero warnings;
- happy `4 sensor-A`: exact stdout, exit `0`;
- negative `9 sensor-A`: exit `2`, stdout rỗng, exact stderr;
- overflow self-test: exit `0`, stderr rỗng, exact stdout `OK overflow-rejected left=4294967295 right=1 output-unchanged=123`;
- ASan+UBSan: không diagnostic;
- Valgrind: exit `0`, zero errors/leaks thuộc fixture.

## Practice Time — độc lập, không chấm điểm

Không thay source và không dùng solution code. Thực hiện audit mới với input `8 node-Z`:

```text
OK count=8 sum=360 label=node-Z word=4660 mask=3
```

Yêu cầu:

1. Strict-build bằng GCC và, nếu có, Clang; cả hai phải zero warnings.
2. Chứng minh exact happy stdout phía trên, stderr rỗng, exit `0`.
3. Dùng input `8 bad_label`; oracle: exit `2`, stdout rỗng, exact stderr `ERROR label must be 1..15 alnum-or-dash characters`.
4. Chạy lại `--overflow-test` dưới sanitizer và đối chiếu exact oracle `output-unchanged=123`; không thay bằng phép signed overflow cố tình.
5. Chạy sanitizer và Valgrind; lưu command, version và exit code.
6. Viết bảng audit tám dòng: mỗi outline chọn một boundary input khác với worked example, nêu defect bị ngăn và evidence pass/fail. Không viết một phiên bản cố tình gây UB để “chứng minh” defect.

Acceptance: exact oracles, diagnostics sạch, mapping đủ tám mục, không có lời giải implementation hoàn chỉnh trong bài nộp.

## Provenance của các case

- `SRC-C17-ISO`, `SRC-C17-WG14`: C17 object, pointer, conversion, string và allocation semantics.
- `SRC-CERTC`: secure-coding rules liên quan integer, string, macro và memory.
- `SRC-GCC11`, `SRC-LLVM14`: warnings/sanitizers theo baseline.
- `SRC-VALGRIND318`: Memcheck workflow và leak classification.
