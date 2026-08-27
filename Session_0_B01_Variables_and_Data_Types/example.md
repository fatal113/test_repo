# Session B01: Variables and Data Types — Ví dụ và nghiên cứu tình huống

> **Case đã chốt:** `CASE-B01-01` · **Milestone:** `M00-FND-01` · **LO:** `ADVC-H1SD` · **Mức xác thực:** simulated

## 🎯 Learning Outcomes liên quan

Là developer của **MDB Edge Diagnostics Gateway — Simulated**, học viên phải chuyển input CLI chưa tin cậy thành record telemetry có type/invariant rõ. Case tạo đúng artifact đã lên kế hoạch:

`Phan_0_Optional_C_Basics_Refresher/Session_0_B01_Variables_and_Data_Types/assets/b01_variables_demo.c`

Artifact là ISO C17 hosted, không dùng API Linux hay extension compiler. Nó minh họa scalar, storage class, qualifier, pointer, struct, union, enum và checked cast trong cùng một đường chạy có oracle.

## Mapping case đã chốt trong course plan

- **id:** CASE-B01-01
- **learning_outcomes:** ADVC-H1SD
- **outline_refs:** OUT-B01-01, OUT-B01-02, OUT-B01-03, OUT-B01-04, OUT-B01-05, OUT-B01-06, OUT-B01-07, OUT-B01-08, OUT-B01-09, OUT-B01-10
- **authenticity:** inherit (simulated)

| Leaf | Bằng chứng trực tiếp trong asset/case |
|---|---|
| OUT-B01-01 Introductory question? | Domain `id/kind/raw`, success/failure contract và oracle CLI được chốt trước type. |
| OUT-B01-02 Basic Data Types | `uint32_t`, `int32_t`, `int64_t`, `bool`, `size_t`, macro `PRI*`. |
| OUT-B01-03 Store Class | automatic `candidate`; file-scope `static records_processed`; function `static`. |
| OUT-B01-04 Key word for variable | input pointer `const`; `_Static_assert`; không lạm dụng `volatile`. |
| OUT-B01-05 Pointer variable | output parameters, `&id`, null guard và `const DiagnosticRecord *`. |
| OUT-B01-06 Struct Data type | typedef `DiagnosticRecord`. |
| OUT-B01-07 Structure | init, member access, pointer `->`, failure-atomic struct assignment. |
| OUT-B01-08 Union | `ReadingValue` chứa một trong temperature/RPM. |
| OUT-B01-09 Enum | `ReadingKind`, parser và exhaustive `switch`. |
| OUT-B01-10 Casting | `checked_i32`/`checked_u32` kiểm miền trước explicit cast. |

## Case Study 01: Ticket DGW-FND-01 — Tạo typed diagnostic record

### 1. Ticket và tiêu chí thành công

- **Vai trò/stakeholder:** C systems developer; reviewer cần evidence chạy lại được.
- **Vấn đề:** input `ID KIND VALUE` là text. `temp` cho phép `int32_t` có dấu; `rpm` chỉ cho phép `uint32_t`. Record phải mang đúng một payload.
- **Ràng buộc:** ISO C17; không giả định `long` rộng bao nhiêu; zero warnings; lỗi không tạo stdout hay record dở dang.
- **Thành công:** self-test pass; happy path khớp stdout/exit `0`; invalid range khớp stderr/exit `2`.

### 2. Input, trạng thái ban đầu và ràng buộc

Happy fixture:

```text
ID=17
KIND=temp
VALUE=25375
```

Negative fixture:

```text
ID=17
KIND=rpm
VALUE=-1
```

State bắt đầu: `records_processed == 0` trong process mới. Không có file, network, clock hay environment variable ảnh hưởng output.

### 3. Phân tích lựa chọn và trade-off

1. **`int` cho mọi field:** code ngắn nhưng không ghi rõ width và không chứa hết `uint32_t` trên implementation có `int` 32 bit.
2. **Struct chứa đồng thời temperature và RPM:** dễ đọc nhưng cho phép cả hai payload cùng tồn tại, tạo state vô nghĩa.
3. **Raw union không tag:** tiết kiệm storage nhưng reader không biết active member.
4. **Phương án chọn:** fixed-width scalar + enum tag + union payload trong struct; parse sang `int64_t`, range-check, cast cuối.

Trade-off: tagged union cần branch và invariant giữa tag/payload, nhưng mô hình hóa đúng variant và buộc parser kiểm tra kind. File-scope counter được giữ để minh họa storage class; production concurrent nên chuyển thành context/atomic tùy contract, không dùng counter này như thiết kế thread-safe.

### 4. Cách triển khai

Đọc asset:

```bash
sed -n '1,260p' assets/b01_variables_demo.c
```

Luồng chính:

```text
argv text
  → parse_i64 (syntax/range của kiểu rộng)
  → checked_u32 hoặc checked_i32
  → make_record (candidate local + tag/payload invariant)
  → publish whole struct
  → increment counter
  → print bằng PRIu32/PRId32
```

Các điểm then chốt:

- `parse_i64` kiểm `errno`, end pointer và chuỗi rỗng.
- `make_record` chỉ gán `*out` sau khi mọi điều kiện pass.
- Printer branch theo tag trước khi đọc union.
- Không kiểm `sizeof(DiagnosticRecord)` theo một con số vì padding là ABI-dependent.

### 5. Build và chạy chính xác

Từ thư mục session:

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror \
  assets/b01_variables_demo.c -o b01_variables_demo
./b01_variables_demo --self-test
./b01_variables_demo --record 17 temp 25375
```

### 6. Output mong đợi

Self-test, stdout chính xác:

```text
B01 SELF-TEST PASS checks=6
```

Happy path, stdout chính xác:

```text
record id=17 kind=TEMP_C raw=25375 whole=25 processed=1
```

Happy path: stderr rỗng, exit code `0`.

Negative path:

```bash
./b01_variables_demo --record 17 rpm -1
```

stderr chính xác:

```text
error: value '-1' is outside uint32 range for RPM
```

Negative path: stdout rỗng, exit code `2`.

### 7. Cách xác minh

```bash
set -eu
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror \
  assets/b01_variables_demo.c -o b01_variables_demo
test "$(./b01_variables_demo --self-test)" = \
  "B01 SELF-TEST PASS checks=6"
test "$(./b01_variables_demo --record 17 temp 25375)" = \
  "record id=17 kind=TEMP_C raw=25375 whole=25 processed=1"

set +e
negative_output=$(./b01_variables_demo --record 17 rpm -1 2>&1 >/dev/null)
negative_status=$?
set -e
test "$negative_status" -eq 2
test "$negative_output" = \
  "error: value '-1' is outside uint32 range for RPM"
```

Pass khi toàn bộ script exit `0`. Không chấp nhận “gần giống” vì oracle chủ ý cố định.

### 8. Giải thích sâu, failure modes và chuyển giao

- **Symptom:** negative fixture in record với RPM lớn → **cause:** cast trước range-check → **diagnose:** đặt breakpoint ở `checked_u32`, xem giá trị nguồn → **fix:** kiểm ở `int64_t` → **prevent:** min/max ±1 tests.
- **Symptom:** temperature được in như RPM → **cause:** tag/union mismatch → **diagnose:** trace `make_record` và switch printer → **fix:** chỉ tạo record qua creator → **prevent:** mỗi variant có test.
- **Symptom:** warning format chỉ xuất hiện trên ABI khác → **cause:** `%lu`/`%d` đoán kiểu → **diagnose:** strict cross-compiler build → **fix:** `PRI*` → **prevent:** `<inttypes.h>` cho fixed-width.
- **Giới hạn:** counter không thread-safe; integer division `25375 / 1000` chỉ tạo phần nguyên, không phải format nhiệt độ có ba chữ số thập phân; record không phải wire format.

Nguyên tắc chuyển giao: conversion chỉ là bước cuối sau validation; aggregate chỉ hợp lệ khi creator duy trì invariant; representation trong memory không được coi là protocol.

## Practice Time — Không tính điểm, không có lời giải

Tạo file riêng `practice_b01_voltage.c`; không sửa oracle của case. Bổ sung variant `volt` cho điện áp nguyên millivolt trong miền `0..5000`.

Yêu cầu:

- Giữ mô hình enum + tagged union + struct và checked conversion.
- `--record 42 volt 3300` phải stdout đúng:

```text
record id=42 kind=VOLT_MV raw=3300 processed=1
```

- `--record 42 volt 5001` phải stdout rỗng, stderr đúng:

```text
error: value '5001' is outside allowed range 0..5000 for VOLT_MV
```

- Exit lần lượt `0` và `2`; strict build zero warnings; thêm `--self-test` có ít nhất bốn boundary checks (`0`, `5000`, `-1`, `5001`).

Không copy nguyên constructor của case rồi đổi tên; hãy tự quyết định type của payload, vị trí validation và failure-atomic output. Nộp source cùng transcript build/run, không có đáp án mẫu trong tài liệu này.

## Provenance của các case

- ISO/IEC 9899:2018 và WG14 N2176: object/type/storage/linkage, aggregate, union/enum, conversion, `<stdint.h>`, `<inttypes.h>`.
- GCC 11.4 manual: strict dialect/diagnostic invocation.
- CLI schema, exit codes, messages và fixture là dữ liệu synthetic của `CASE-B01-01`, không phải API từ nguồn ngoài.
