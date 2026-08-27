# Session B02: Arrays, Decisions and Loops — Ví dụ và nghiên cứu tình huống

> **Case đã chốt:** `CASE-B02-01` · **Milestone:** `M00-FND-02` · **LO:** `ADVC-H1SD` · **Mức xác thực:** simulated

## 🎯 Learning Outcomes liên quan

Là C developer của **MDB Edge Diagnostics Gateway — Simulated**, học viên cần xử lý một batch số nguyên có giới hạn, chọn phép phân tích bằng CLI và tạo kết quả có thể đối chiếu byte-for-byte. Case tạo đúng artifact trong kế hoạch:

`Phan_0_Optional_C_Basics_Refresher/Session_0_B02_Arrays_Decisions_and_Loops/assets/b02_array_flow_demo.c`

Artifact là chương trình hosted ISO C17, không truy cập thanh ghi, ngắt, DMA, file hay network. Nó buộc capacity, shape, decision và termination condition trở thành contract quan sát được thay vì giả định ngầm.

## Mapping case đã chốt trong course plan

- **id:** CASE-B02-01
- **learning_outcomes:** ADVC-H1SD
- **outline_refs:** OUT-B02-02, OUT-B02-03, OUT-B02-04, OUT-B02-05, OUT-B02-06, OUT-B02-07, OUT-B02-09, OUT-B02-10, OUT-B02-11, OUT-B02-12, OUT-B02-14, OUT-B02-15, OUT-B02-16, OUT-B02-17
- **authenticity:** inherit (simulated)

| Leaf | Bằng chứng trực tiếp trong asset/case |
|---|---|
| OUT-B02-02 1.1 What is Array? | `values` là tập object `int32_t` đồng nhất, truy cập bằng chỉ số. |
| OUT-B02-03 1.2 Multidimensional Arrays | Self-test duyệt `row_major[2][3]` và xác nhận tổng `21`. |
| OUT-B02-04 1.3 Array in memory | Duyệt row-major; số hàng lấy bằng `sizeof row_major / sizeof row_major[0]`. |
| OUT-B02-05 1.4 How to declare an Array? | `int32_t values[MAX_VALUES] = {0}` và mảng `const` có initializer. |
| OUT-B02-06 1.5 How works with array? | Mọi hàm nhận array kèm `count`; chỉ truy cập miền `[0, count)`. |
| OUT-B02-07 1.6 When will using Array? | Batch có tối đa tám giá trị cùng kiểu và cần duyệt tuần tự. |
| OUT-B02-09 2.1 Introduce | Parser, analyzer và presenter tách các điểm quyết định. |
| OUT-B02-10 2.2 How to build an expression? | Biểu thức so sánh, logic, arithmetic dùng operand đã parse và tránh side effect ẩn. |
| OUT-B02-11 2.3 If, else, condition Operator “?:” | `if/else if/else` chọn mode; `?:` chỉ chọn nhãn `band`. |
| OUT-B02-12 2.4 Switch | `switch` ánh xạ enum `AnalysisMode` sang thuật toán/tên. |
| OUT-B02-14 3.1 What is looping in C? | `for`, `while`, `do...while` đều có invariant và termination hữu hạn. |
| OUT-B02-15 3.3 Enter and exit, break looping follow | `continue` bỏ số không dương; `break` dừng ở số dương đầu tiên. |
| OUT-B02-16 3.4 How to use looping? | Parse và analyze dùng vòng lặp đúng với bounds/công việc. |
| OUT-B02-17 3.5 Key for Looping | init → guard → body → progress; self-test kiểm `0` cho `do...while`. |

## Case Study 01: Ticket DGW-FND-02 — Phân tích batch số nguyên có giới hạn

### 1. Ticket và tiêu chí thành công

- **Vai trò/stakeholder:** C systems developer; reviewer cần build log và oracle tái lập được.
- **Vấn đề:** nhận `MODE VALUE...`; hỗ trợ `sum`, `max`, `first-positive`; không đọc quá capacity và không dùng dữ liệu parse lỗi.
- **Ràng buộc:** ISO C17; `1..8` giá trị `int32_t`; zero warnings; không phụ thuộc locale, clock hay platform-specific API.
- **Thành công:** self-test pass; happy path cho đúng hai dòng stdout/exit `0`; token lỗi cho đúng stderr/exit `2` và stdout rỗng.

### 2. Input, trạng thái ban đầu và ràng buộc

Happy fixture:

```text
MODE=max
VALUES=12 7 25 9
```

Negative fixture:

```text
MODE=max
VALUES=12 bad 25
```

Mỗi lần chạy bắt đầu với `values[8]` được zero-initialize; chỉ `count` phần tử đầu tiên trở thành active data. Parser phải hoàn tất từng token trước khi analyzer được gọi.

### 3. Phân tích lựa chọn và trade-off

1. **Sentinel kết thúc mảng:** không phù hợp vì mọi giá trị `int32_t`, kể cả `0` hay `INT32_MIN`, đều có thể là dữ liệu hợp lệ.
2. **Mảng tăng trưởng động:** linh hoạt hơn nhưng thêm allocation/failure path không cần thiết cho capacity đã biết là tám.
3. **Chuỗi `if` chứa cả thuật toán:** chạy được nhưng mode và hành vi dễ lệch nhau khi mở rộng.
4. **Phương án chọn:** fixed-capacity array + explicit `count`; enum được parse một lần; `switch` chọn thuật toán; từng loop có bound rõ.

Trade-off: giới hạn tám phần tử làm CLI dễ kiểm chứng và tránh heap, đổi lại không phù hợp batch tùy ý lớn. `int64_t` được dùng cho kết quả tổng để tránh overflow khi cộng tối đa tám `int32_t`; thiết kế vẫn phải đổi nếu capacity tăng vượt miền đó.

### 4. Cách triển khai

Đọc asset:

```bash
sed -n '1,280p' assets/b02_array_flow_demo.c
```

Luồng chính:

```text
argv → parse mode → kiểm count 1..8 → parse từng int32_t
     → switch(mode) → loop có bound/break/continue → classify band → print
```

Điểm then chốt:

- `count = argc - 3` được kiểm trước khi ghi mảng.
- Vòng parse dùng `i < count`; vị trí lỗi hiển thị theo kiểu người dùng, bắt đầu từ `1`.
- `MODE_MAX` khởi tạo từ `values[0]`, không dùng `0` làm maximum giả.
- `MODE_FIRST_POSITIVE` chỉ trả success nếu `break` xảy ra trước `i == count`.
- `decimal_digits` dùng `do...while`, do đó input `0` cho oracle `1` digit.

### 5. Build và chạy chính xác

Từ thư mục session:

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror \
  assets/b02_array_flow_demo.c -o b02_array_flow_demo
./b02_array_flow_demo --self-test
./b02_array_flow_demo --analyze max 12 7 25 9
```

### 6. Output mong đợi

Self-test, stdout chính xác:

```text
B02 SELF-TEST PASS checks=8
```

Happy path, stdout chính xác:

```text
count=4 accepted=4 rejected=0
mode=MAX result=25 band=HIGH
```

Happy path: stderr rỗng, exit code `0`.

Negative path:

```bash
./b02_array_flow_demo --analyze max 12 bad 25
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
  assets/b02_array_flow_demo.c -o b02_array_flow_demo
test "$(./b02_array_flow_demo --self-test)" = \
  "B02 SELF-TEST PASS checks=8"
test "$(./b02_array_flow_demo --analyze max 12 7 25 9)" = \
  "count=4 accepted=4 rejected=0
mode=MAX result=25 band=HIGH"

set +e
negative_output=$(./b02_array_flow_demo --analyze max 12 bad 25 2>&1 >/dev/null)
negative_status=$?
set -e
test "$negative_status" -eq 2
test "$negative_output" = \
  "error: invalid integer at position 2: bad"
```

Pass khi script exit `0`. Oracle gồm cả thứ tự dòng, stream và exit code.

### 8. Giải thích sâu, failure modes và chuyển giao

- **Symptom:** `max -5 -2` cho `0` → **cause:** khởi tạo maximum bằng `0` → **diagnose:** trace giá trị trước loop → **fix:** dùng `values[0]`, loop từ `1` → **prevent:** fixture toàn số âm.
- **Symptom:** input thứ chín làm hỏng memory → **cause:** ghi trước khi kiểm capacity → **diagnose:** sanitizer/kiểm `count` và chỉ số → **fix:** reject `count > MAX_VALUES` trước loop → **prevent:** test 8 và 9 phần tử.
- **Symptom:** `first-positive -2 0` in kết quả cũ → **cause:** không phân biệt “không tìm thấy” → **diagnose:** theo dõi `i` sau loop → **fix:** trả `false` khi `i == count` → **prevent:** oracle no-match với exit `3`.
- **Giới hạn:** đây là batch nhỏ trong memory; không thay cho streaming, vòng lặp real-time có WCET, hoặc xử lý concurrent.

Nguyên tắc chuyển giao: array luôn đi với capacity/count; branch phải thể hiện policy; loop phải có bound hoặc progress chứng minh được; invalid input không được chảy sang bước tính toán.

## Practice Time — Không tính điểm, không có lời giải

Tạo file riêng `practice_b02_window.c`; không sửa asset của case. Viết CLI lọc các phần tử nằm trong khoảng đóng `[LOW, HIGH]` và báo vị trí phần tử đầu tiên thỏa điều kiện.

Yêu cầu:

- Cú pháp: `--window LOW HIGH VALUE [VALUE ...]`; tối đa 10 giá trị.
- Dùng fixed-capacity array + explicit count, `for`, `if` và `break`; không dùng sentinel.
- Với `--window 10 20 5 10 14 20 21`, stdout chính xác:

```text
count=5 in_range=3 first_index=1
```

- Với `--window 20 10 5 14`, stdout rỗng, stderr chính xác:

```text
error: LOW must be less than or equal to HIGH
```

- Exit lần lượt `0` và `2`; strict build zero warnings; `--self-test` phải kiểm lower boundary, upper boundary, no-match và capacity overflow.

`first_index` là chỉ số mảng bắt đầu từ `0`. Hãy tự thiết kế parser, loop invariant và cách biểu diễn “không tìm thấy”; tài liệu này cố ý không cung cấp solution.

## Provenance của các case

- ISO/IEC 9899:2018 và WG14 N2176: array, selection statement, iteration statement, integer/conversion semantics.
- `SRC-GCC11`, *GCC 11.4 manuals*, version 11.4.0: lựa chọn dialect `-std=c17` và warning options dùng trong lệnh kiểm chứng.
- CLI schema, limits, exit codes, messages và fixtures là dữ liệu synthetic của `CASE-B02-01`, không phải API từ nguồn ngoài.
