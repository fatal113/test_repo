# B00 — C Basics Refresher (Study before S01) — Worked Example

> **Khóa học:** [NFP] Advanced C Programming · **Tính chất:** ví dụ tự học tùy chọn trước S01 · **Mức xác thực:** simulated
>
> **Trạng thái phạm vi:** **[BỔ SUNG — nguồn: `SRC-USER-BASIC`, yêu cầu trực tiếp đã được người dùng phê duyệt ngày 2026-08-21]**. Case này minh họa đúng bốn leaf `OUT-B00-01..04`, hỗ trợ prerequisite cho `ADVC-H1SD` và tạo optional readiness increment `M00-RDY`; case không thay đổi ASM, graded milestone, starter hay lời giải của ASM-A01.

## 🎯 Learning Outcomes liên quan

- **LO hỗ trợ:** `ADVC-H1SD` — prerequisite support.
- **Outline refs:** `OUT-B00-01`, `OUT-B00-02`, `OUT-B00-03`, `OUT-B00-04`.
- **Project increment:** `M00-RDY` — một C systems developer chứng minh mình đọc/validate input, tách hàm, quản lý bounded array/output pointer và tái hiện lỗi bằng compiler/debugger trước khi nhận ticket M01.
- **Artifact:** [basic_refresher_demo.c](assets/basic_refresher_demo.c).
- **Validation method:** executable với happy oracle và invalid-input oracle.

## Case CASE-B00-01 — Tóm tắt sample synthetic có validation

### 1. Ticket và tiêu chí thành công

Một công cụ preflight nhận đúng một command-line argument gồm các số nguyên phân tách bằng dấu phẩy. Tool phải chấp nhận tối đa tám sample trong miền `[-50,150]`, tính count/min/max/mean, và từ chối toàn bộ danh sách nếu có token không hợp lệ. Dữ liệu chỉ là fixture synthetic.

Tiêu chí thành công:

- Source build ở strict C17 với warning count `0`.
- Input `18,21,24,30` exit `0`, stdout đúng golden text.
- Input `18,xx,24` exit `2`, stderr đúng golden text và stdout rỗng.
- GDB có thể dừng ở parser và cho thấy `end == cursor` tại token `xx`.

### 2. Input, baseline và ràng buộc

- **Happy input:** `18,21,24,30`.
- **Invalid input:** `18,xx,24`.
- **Boundary:** một đến tám phần tử; mỗi phần tử từ `-50` đến `150`; không chấp nhận token rỗng, dấu phẩy cuối, hậu tố ký tự hoặc phần tử thứ chín.
- **Environment:** Ubuntu 22.04/WSL2 của khóa, GCC 11.4 baseline, GDB 12.1, language mode C17.
- **Code constraints:** không allocation động; không mutable global; không dùng extension ngoài C17; output pointer phải được kiểm trước dereference.

### 3. Phân tích quyết định

Một parser dùng `atoi` ngắn hơn nhưng không cung cấp end pointer để phân biệt `18` với `18x`, và không có error contract đủ rõ. Case chọn `strtol`: caller đặt `errno = 0`, kiểm `ERANGE`, kiểm parser có tiến (`end != cursor`), kiểm range trước khi cast và kiểm ký tự kế tiếp là `,` hoặc `\0`.

Mảng cố định tám phần tử phù hợp fixture bounded và tránh đưa allocation động vào phần prerequisite. Trade-off là input dài hơn bị từ chối; đó là policy quan sát được, không phải lỗi. Parser nhận `capacity` riêng vì sau khi mảng đi qua parameter, compiler chỉ còn pointer và không giữ array extent.

Tool tách ba trách nhiệm:

1. `parse_samples` chuyển chuỗi thành mảng/count và trả status.
2. `summarize_samples` tính aggregate và chép minimum, maximum, total qua ba output pointer.
3. `mean_of` nhận total/count đã hợp lệ và thực hiện phép chia có conversion rõ.

Thiết kế này cho thấy function contract, scope, pointer output và control flow mà không lẫn với callback/function pointer của S01.

### 4. Implementation walkthrough

**Cấu trúc/kiểu/control flow — `OUT-B00-01`.** Các hằng `#define` khóa capacity/range/exit code. `size_t` dùng cho count/index, `int` cho sample/total sau khi range đã được chặn, `double` cho mean. `if` xử lý precondition/parse failure; `while` duyệt token; `for` duyệt đúng các phần tử đã khởi tạo.

**Hàm/scope — `OUT-B00-02`.** Helper là `static`, nên không lộ symbol ra ngoài translation unit. Các scalar local được chép qua output pointer hợp lệ trước khi hết lifetime. Status tách khỏi dữ liệu để caller không dùng output của nhánh lỗi.

**Mảng/chuỗi — `OUT-B00-03`.** `samples` có capacity tám nhưng `sample_count` mới là length hợp lệ. Parser dừng ở `\0`, không dùng `sizeof` trên array parameter và kiểm capacity trước lần ghi.

**Con trỏ/compile/debug — `OUT-B00-04`.** `argv[1]` trỏ tới input string; `&sample_count`, `&minimum`, `&maximum`, `&total` cho helper ghi vào object của `main`; `const int *values` khóa input chỉ đọc; `end` chứng minh parser đã tiêu thụ token nào. Null/precondition checks đứng trước dereference.

Source đầy đủ: [assets/basic_refresher_demo.c](assets/basic_refresher_demo.c).

### 5. Build và happy oracle

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O0 -g \
  assets/basic_refresher_demo.c -o /tmp/basic_refresher_demo
/tmp/basic_refresher_demo '18,21,24,30'
```

Expected:

- Compile exit `0`, warning count `0`.
- Run exit `0`.
- Stdout chính xác:

```text
OK count=4 min=18 max=30 mean=23.25
```

- Stderr rỗng.

Lý do mean là `23.25`: total `93` được cast sang `double` trước phép chia cho count `4`; integer division sẽ cho oracle sai.

### 6. Invalid-input oracle

Chạy riêng để ghi đúng exit code và hai stream:

```bash
set +e
/tmp/basic_refresher_demo '18,xx,24' \
  >/tmp/basic_refresher_invalid.out \
  2>/tmp/basic_refresher_invalid.err
status=$?
set -e

test "$status" -eq 2
test ! -s /tmp/basic_refresher_invalid.out
test "$(cat /tmp/basic_refresher_invalid.err)" = "ERROR invalid sample list"
```

Expected:

- Exit code `2`.
- Stdout rỗng.
- Stderr chính xác:

```text
ERROR invalid sample list
```

Đây là expected negative behavior của ứng dụng, không phải compiler/runtime crash. Nếu bất kỳ lệnh `test` nào fail, case chưa đạt.

### 7. Debug workflow cho input lỗi

Chỉ dùng GDB sau khi build đã warning-free và lỗi tái hiện ổn định:

```bash
gdb -q /tmp/basic_refresher_demo
```

Trong GDB:

```gdb
break parse_samples
run '18,xx,24'
print text
list parse_samples
break basic_refresher_demo.c:36
continue
print cursor
continue
print cursor
print end
print *cursor
print errno
backtrace
quit
```

Breakpoint dòng `36` là điều kiện ngay sau `strtol` trong asset hiện tại: lần `continue` đầu dừng ở token `18`, lần kế tiếp dừng ở token `xx`. Nếu source được định dạng lại, dùng `list parse_samples`, chọn dòng `if` ngay sau `strtol`, rồi đặt breakpoint tại dòng đó. Evidence cần quan sát là ở token `xx`, `end == cursor`, `*cursor == 'x'`, parser trả `0`, `main` chọn nhánh in lỗi và trả `2`. GDB hỗ trợ định vị control/data state; oracle cuối vẫn là output và exit code ở mục 6.

### 8. Boundary checks mở rộng

Các lệnh sau dùng cùng executable nhưng kiểm contract khác:

```bash
/tmp/basic_refresher_demo '-5,0,150'
# OK count=3 min=-5 max=150 mean=48.33

/tmp/basic_refresher_demo '18,21,'
# ERROR invalid sample list; exit 2

/tmp/basic_refresher_demo '1,2,3,4,5,6,7,8,9'
# ERROR invalid sample list; exit 2

/tmp/basic_refresher_demo '151'
# ERROR invalid sample list; exit 2
```

Boundary fixtures quan trọng hơn việc thêm nhiều input ngẫu nhiên: chúng kiểm dấu phẩy cuối, capacity trước lần ghi và range trước lần cast.

### 9. Failure modes và cách sửa

- **Dấu hiệu:** compile báo format mismatch ở `%zu`. **Nguyên nhân:** count không phải `size_t` hoặc format không khớp. **Chẩn đoán:** đọc expected/actual type trong warning. **Sửa:** đồng bộ type và format; không cast để tắt warning. **Phòng tránh:** giữ `-Werror` trong build oracle.
- **Dấu hiệu:** happy case in mean `23.00`. **Nguyên nhân:** integer division. **Chẩn đoán:** GDB `print total`, `print sample_count`, xem biểu thức. **Sửa:** convert có chủ đích sang `double` trước chia. **Phòng tránh:** golden input có mean không nguyên.
- **Dấu hiệu:** invalid input vẫn sinh summary một phần. **Nguyên nhân:** caller bỏ qua return status hoặc parser commit output quá sớm. **Chẩn đoán:** breakpoint tại return của parser và watch `sample_count`. **Sửa:** chỉ gọi summarizer khi parse thành công; output-on-error là count `0`. **Phòng tránh:** invalid oracle kiểm cả stdout và exit code.
- **Dấu hiệu:** phần tử thứ chín làm crash hoặc đổi output. **Nguyên nhân:** capacity check sau array write. **Chẩn đoán:** theo dõi `count` trước `values[count]`. **Sửa:** reject khi `count == capacity` trước lần ghi. **Phòng tránh:** giữ fixture chín phần tử.

### 10. Trade-off và bài học chuyển giao

Fixed array, linear parse và linear summary tối ưu cho khả năng đọc/kiểm ở bài ôn tập, không tối ưu cho stream lớn. Tool chỉ chấp nhận grammar đơn giản và locale-independent; không xử lý whitespace, đơn vị đo hoặc Unicode. Thêm những khả năng đó cần contract và oracle mới, không nên mở rộng ngầm.

Khi vào S01, học viên sẽ nâng mental model này lên pointer-to-pointer, const-correctness sâu hơn, function pointer, callback table và `void *` generic API. Điều cần giữ nguyên là thói quen ghi precondition, lifetime/bounds, status/output-on-error và oracle có thể chạy lại.

## Provenance của các case

- `SRC-USER-BASIC` — yêu cầu trực tiếp của người dùng ngày 2026-08-21 phê duyệt phần ôn tập tùy chọn gồm bốn leaf trước S01. Case và cách nhóm là nội dung bổ sung ngoài outline chính thức, không thay đổi syllabus/LO/assessment.
- Chuẩn/nguồn kỹ thuật: ISO C17 metadata, WG14 N2176 public draft, GNU C Language Manual, GCC 11.4 manuals và SEI CERT C Coding Standard; chi tiết liên kết và giới hạn sử dụng được ghi trong [material.md](material.md).
- Source code và fixture là nội dung nguyên gốc, synthetic; không chứa secret, PII, endpoint hoặc dữ liệu khách hàng.
