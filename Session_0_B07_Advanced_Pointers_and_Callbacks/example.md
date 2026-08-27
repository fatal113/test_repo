# B07 — Advanced Pointers and Callbacks — Worked Example

> **Case theo plan:** `CASE-B07-01` · **LO:** `ADVC-H1SD` · **Validation:** executable  
> **Boundary:** ISO C17 portable, local synthetic input; không hardware/embedded API.

## 🎯 Learning Outcomes liên quan

`CASE-B07-01` phủ, đúng thứ tự:

1. `OUT-B07-01` — Assigning pointer to address
2. `OUT-B07-02` — Wrong using pointer
3. `OUT-B07-03` — Pointer essence
4. `OUT-B07-04` — Function pointer
5. `OUT-B07-05` — Callback function

Artifact: [assets/b07_callbacks_demo.c](assets/b07_callbacks_demo.c). Mức xác thực kế thừa project mô phỏng; case không phải Assignment và không chấm điểm.

## Case CASE-B07-01 — Callback filter có context và output ownership

### 1. Ticket và tiêu chí thành công

Một CLI nhận threshold cùng tối đa 16 số nguyên phân tách bằng dấu phẩy. Nó phải dùng callback để chọn các giá trị lớn hơn hoặc bằng threshold, rồi trả summary được cấp phát qua pointer-to-pointer.

Tiêu chí:

- strict C17 build exit `0`, không warning;
- happy input tạo đúng count/sum/first;
- malformed token bị từ chối trước callback, stdout rỗng;
- whitespace ở threshold hoặc list bị từ chối bằng diagnostic riêng trước `strtol`;
- summary chỉ được caller sở hữu sau khi callee commit thành công;
- ASan/UBSan không báo finding trên hai path đã công bố.

### 2. Input, baseline và constraint

- Happy: threshold `8`, list `3,8,13,5`.
- Negative: threshold `8`, list `3,x,13`.
- Whitespace-negative: threshold `8`, list `3, 8,13`.
- Không whitespace/token rỗng/trailing comma; `contains_whitespace` kiểm cả hai CLI fields **trước** `strtol`; capacity 16; parse phải tiến và nằm trong miền `int`.
- Callback đồng bộ, context borrowed, không global mutable state.

### 3. Phân tích thiết kế

`contains_whitespace` duyệt từng byte dưới dạng `unsigned char` trước parse, nên khoảng trắng mà `strtol` vốn có thể bỏ qua vẫn bị policy reject. `parse_list` giữ pointer cursor trong chuỗi và luôn kiểm `end != cursor`, separator và capacity trước lần ghi. `value_predicate` khóa signature `int(int, const void *)`; `at_least_threshold` đọc `struct threshold_context` nhưng không giữ nó. `select_values` nhận pointer+count, kiểm mọi output pointer và overflow của tổng.

`build_summary` minh họa pointer-to-pointer: đặt `*out_summary = NULL`, cấp phát buffer tạm, kiểm `snprintf`, rồi mới gán output. Vì vậy caller có rule đơn giản: chỉ `free(summary)` khi call thành công.

Trade-off: `void *context` cho phép nhiều policy dùng chung traversal nhưng mất type cụ thể tại boundary; callback tự cast về context đã thống nhất. Một API chỉ có một policy cố định có thể nhận threshold trực tiếp và đơn giản hơn.

### 4. Implementation walkthrough theo outline

- `OUT-B07-01`: `&context`, `&selected`, `&summary` đều lấy địa chỉ object còn sống trong `main`.
- `OUT-B07-02`: mọi pointer được kiểm trước dereference; parser không ghi quá `MAX_VALUES`; summary được free một lần.
- `OUT-B07-03`: dãy luôn đi cùng `value_count`; không dùng `sizeof` trên array parameter.
- `OUT-B07-04`: `value_predicate` ngăn callback sai signature tại compile time.
- `OUT-B07-05`: callback nhận context per-call; traversal và policy tách rời; output ownership commit atomically.

### 5. Build và happy oracle

Từ thư mục Unit:

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O0 -g \
  assets/b07_callbacks_demo.c -o /tmp/b07_callbacks_demo
/tmp/b07_callbacks_demo 8 '3,8,13,5'
```

Expected:

```text
OK threshold=8 selected=2 sum=21 first=8
```

Exit `0`, stderr rỗng. `8` và `13` được chọn theo đúng thứ tự input; `first=8` chứng minh callback không sort hoặc thay đổi dãy.

### 6. Negative oracle

```sh
set +e
/tmp/b07_callbacks_demo 8 '3,x,13' >/tmp/b07_bad.out 2>/tmp/b07_bad.err
status=$?
set -e
test "$status" -eq 2
test ! -s /tmp/b07_bad.out
test "$(cat /tmp/b07_bad.err)" = 'ERROR invalid integer list'
```

Malformed token là expected rejection, không phải crash. Callback không được gọi với partial list.

Whitespace có oracle riêng và phải bị reject nhất quán ở threshold lẫn list:

```sh
set +e
/tmp/b07_callbacks_demo 8 '3, 8,13' >/tmp/b07_space.out 2>/tmp/b07_space.err
space_status=$?
set -e
test "$space_status" -eq 2
test ! -s /tmp/b07_space.out
test "$(cat /tmp/b07_space.err)" = 'ERROR whitespace is not allowed'

set +e
/tmp/b07_callbacks_demo ' 8' '3,8,13' >/tmp/b07_threshold_space.out 2>/tmp/b07_threshold_space.err
threshold_space_status=$?
set -e
test "$threshold_space_status" -eq 2
test ! -s /tmp/b07_threshold_space.out
test "$(cat /tmp/b07_threshold_space.err)" = 'ERROR whitespace is not allowed'
```

### 7. Sanitizer evidence

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O1 -g \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  assets/b07_callbacks_demo.c -o /tmp/b07_callbacks_san
/tmp/b07_callbacks_san 8 '3,8,13,5'
```

Expected vẫn là exact happy output và không có sanitizer diagnostic. Đây chỉ là bằng chứng cho fixture đã chạy; review lifetime/extent vẫn bắt buộc.

### 8. Failure modes và troubleshooting

| Dấu hiệu | Nguyên nhân khả dĩ | Chẩn đoán | Sửa / phòng tránh |
| --- | --- | --- | --- |
| warning incompatible pointer type | callback sai signature | đối chiếu typedef và prototype | viết adapter type-correct; không cast |
| selected đúng nhưng sum sai | overflow hoặc callback có side effect | log input/selected, chạy UBSan | checked-add; callback chỉ đọc context |
| leak summary | caller bỏ quên ownership | ASan/Valgrind allocation stack | một cleanup path, free đúng một lần |
| crash khi invalid token | dùng partial output | breakpoint tại parser/caller | parser commit count chỉ khi toàn input hợp lệ |
| input có space vẫn được nhận | dựa trực tiếp vào whitespace-tolerant `strtol` | chạy exact whitespace-negative oracle | pre-check cả threshold/list bằng `isspace((unsigned char)c)` |

## Practice Time — độc lập, không chấm điểm

Không sửa asset gốc. Copy vào workspace riêng và bổ sung một predicate mới “nằm trong inclusive range” dùng context `{minimum, maximum}`; traversal không được biết policy cụ thể.

- Input mới: range `[10,20]`, list `1,10,12,21,20`.
- Oracle: exit `0`, stderr rỗng, stdout chính xác `OK range=10..20 selected=3 sum=42 first=10`.
- Negative: range `20..10` phải exit `2`, stdout rỗng, stderr `ERROR invalid range`.
- Evidence: source diff, strict-build log, happy/negative stdout-stderr-exit record và sanitizer summary.

Phần này chỉ nêu contract/oracle, không cung cấp implementation hay chuỗi bước giải. Hoàn thành Practice Time không tạo điểm và không thay thế bất kỳ Assignment nào.

## Bài học chuyển giao

Pointer an toàn không đến từ syntax riêng lẻ mà từ hợp đồng nguồn gốc, extent, lifetime và ownership. Function pointer/callback mở rộng policy chỉ khi signature và context lifetime được khóa; pointer-to-pointer chỉ nên commit output sau thành công.

## Provenance của các case

- `SRC-USER-CREF`, `SRC-C17-ISO`, `SRC-C17-WG14`, `SRC-CERTC` như registry trong `_course_plan.json`.
- Scenario, input và output là synthetic, **[BỔ SUNG — nguồn: `SRC-USER-CREF`]**.
