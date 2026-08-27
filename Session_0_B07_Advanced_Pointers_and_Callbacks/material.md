# B07 — Advanced Pointers and Callbacks — Học liệu cốt lõi

> **Khóa học:** [NFP] Advanced C Programming · **Thời lượng:** 90 phút · **Chuẩn:** ISO C17  
> **Phạm vi:** chương trình C portable chạy local. Không dùng MCU, HAL, RTOS, ISR, MMIO, địa chỉ ngoại vi hoặc flashing. GNU/Linux chỉ là môi trường build và lấy bằng chứng, không phải semantics của ngôn ngữ.

## 🎯 Learning Outcomes

- **ADVC-H1SD:** thiết kế và kiểm chứng hợp đồng con trỏ, function pointer và callback có context mà không làm mơ hồ lifetime hay ownership.
- **Project increment `M00-FND-07`:** tạo executable [b07_callbacks_demo.c](assets/b07_callbacks_demo.c) với callback type-correct và pointer-to-pointer output.

## 1. Kiến thức tiên quyết và môi trường

- Hoàn thành B06 hoặc chứng minh được readiness tương đương về array, function, allocation và exit code.
- Biết phân biệt object, địa chỉ, giá trị con trỏ và giá trị sau dereference.
- Môi trường chuẩn: Ubuntu 22.04/WSL2, GCC 11.4; warning profile `-std=c17 -Wall -Wextra -Wpedantic -Werror`.
- Dữ liệu đều synthetic; không có secret, PII hay endpoint thật.

Preflight:

```sh
gcc --version
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O0 -g \
  assets/b07_callbacks_demo.c -o /tmp/b07_callbacks_demo
```

## 2. Mental map

`object có lifetime → lấy địa chỉ → pointer có type → kiểm precondition → dereference → function pointer khóa signature → callback nhận context → caller giữ ownership output`

Con trỏ không tự mang thông tin “còn sống”, “có bao nhiêu phần tử” hoặc “ai phải free”. Những thông tin đó thuộc hợp đồng API. Function pointer thêm một trục nữa: địa chỉ code chỉ hợp lệ khi signature khớp; callback phải nhận đủ context mà không dựa vào global mutable state.

## 3. Nội dung lý thuyết cốt lõi

- [x] Assigning pointer to address
- [x] Wrong using pointer
- [x] Pointer essence
- [x] Function pointer
- [x] Callback function

#### OUT-B07-01 — Assigning pointer to address

**Mapping.** `OUT-B07-01` → `ADVC-H1SD` → `M00-FND-07`: tạo pointer chỉ sau khi xác định object và lifetime của object đó.

**Định nghĩa và ranh giới.** Phép `&object` tạo địa chỉ của một object có thể lấy địa chỉ; phép gán `T *pointer = &object` lưu địa chỉ phù hợp type. Gán địa chỉ không chuyển ownership, không kéo dài lifetime và không đồng nghĩa với dereference an toàn. Không lấy địa chỉ của bit-field; không giữ pointer tới automatic object sau khi block kết thúc.

**Vai trò và quyết định.** Developer phải quyết định pointer có thể null không, trỏ tới một object hay một dãy, có quyền sửa hay chỉ đọc, và lifetime do ai bảo đảm. Trong B07, `values` do `main` sở hữu; callback chỉ mượn `const int *` trong thời gian gọi.

**Cơ chế.** Type của pointer quy định cách compiler kiểm phép dereference và pointer arithmetic. `&value` có type tương ứng với `value`; `*pointer` truy cập object đích chỉ khi địa chỉ hợp lệ, được căn chỉnh đúng và lifetime còn hiệu lực.

**Khi dùng / không dùng / trade-off.** Dùng pointer khi API cần tham chiếu object, output parameter hoặc dãy kèm length. Dùng giá trị trực tiếp khi copy nhỏ và không cần chia sẻ mutation. Pointer tránh copy nhưng tăng yêu cầu về lifetime, aliasing và null policy.

**Ví dụ cụ thể và oracle.** Với `int reading = 41; int *target = &reading; *target += 1;`, artifact là state của `reading`; oracle là `reading == 42` và `target == &reading`. Nếu đổi `target` sang `NULL`, nhánh phải từ chối trước dereference thay vì crash.

**Best practice.** **Rule:** khởi tạo pointer ngay khi khai báo và ghi rõ null/ownership contract. **Rationale:** giảm trạng thái indeterminate và giúp reviewer kiểm lifetime. **Positive:** `const int *view = samples;` kèm `count`. **Negative:** `int *p; *p = 7;` dereference giá trị indeterminate, hành vi không xác định.

**Failure/troubleshooting.** Dấu hiệu: crash hoặc sanitizer báo invalid write → nguyên nhân thường là pointer chưa khởi tạo/hết lifetime → chẩn đoán bằng warning, ASan và backtrace tại dereference → sửa bằng khởi tạo từ object còn sống và kiểm null → phòng tránh bằng API pointer+length và strict warnings.

#### OUT-B07-02 — Wrong using pointer

**Mapping.** `OUT-B07-02` → `ADVC-H1SD` → `M00-FND-07`: nhận diện null, dangling, one-past, wrong-type và double-free trước khi callback chạy.

**Định nghĩa và ranh giới.** Dùng pointer sai gồm dereference null/indeterminate/dangling, vượt biên array, vi phạm alignment/effective type, free không đúng ownership hoặc dùng lại sau free. One-past pointer có thể dùng để so sánh/làm sentinel trong cùng array nhưng không được dereference.

**Vai trò và quyết định.** Reviewer phải tách lỗi “địa chỉ sai” khỏi lỗi “giá trị dữ liệu sai”. Quyết định sửa phải phục hồi invariant nguồn gốc, extent và lifetime, không chỉ thêm cast hoặc tắt warning.

**Cơ chế.** Compiler không luôn biết runtime lifetime. Allocator có thể tái sử dụng vùng đã free; code dường như chạy đúng ở một build nhưng hỏng ở build khác. Sanitizer quan sát path đã chạy, còn code review kiểm contract trên path chưa chạy.

**Khi dùng / không dùng / trade-off.** Null là trạng thái hợp lệ chỉ khi API công bố và caller xử lý được. Không dùng sentinel pointer mơ hồ khi enum status hoặc length diễn đạt rõ hơn. Defensive check tăng nhánh nhưng đổi lại error path xác định.

**Ví dụ cụ thể và oracle.** Hàm `write_if_present(int *out, int value)` trả `0` khi `out == NULL`, không ghi memory; với `int x = 0`, gọi `write_if_present(&x, 9)` trả `1` và `x == 9`. Hai oracle phân biệt expected rejection với crash.

**Best practice.** **Rule:** validate pointer trước lần dereference đầu và invalidate pointer sở hữu sau `free`. **Rationale:** lỗi được chặn ở boundary và tránh reuse vô tình. **Positive:** `free(buffer); buffer = NULL;`. **Negative:** giữ alias rồi đọc qua alias sau free; gán riêng owner về null không cứu alias dangling.

**Failure/troubleshooting.** Dấu hiệu: ASan `heap-use-after-free` → nguyên nhân alias sống lâu hơn allocation → chẩn đoán theo allocation/free/use stack → sửa ownership và thứ tự cleanup → phòng tránh bằng một owner, borrowed lifetime ngắn và regression fixture.

#### OUT-B07-03 — Pointer essence

**Mapping.** `OUT-B07-03` → `ADVC-H1SD` → `M00-FND-07`: xem pointer như capability có type, provenance, extent và quyền truy cập, không phải “một số nguyên chứa địa chỉ”.

**Định nghĩa và ranh giới.** Bản chất hữu ích của pointer trong C là giá trị cho phép định danh object/function theo các phép được chuẩn cho phép. Pointer arithmetic chỉ có nghĩa trong một array object và one-past. Chuyển pointer tùy tiện sang số nguyên rồi quay lại không tạo ra object hay lifetime mới.

**Vai trò và quyết định.** Khi thiết kế API, phải truyền extent riêng, dùng `const` để biểu đạt quyền đọc và chọn output parameter khi cần status tách khỏi dữ liệu. Không suy ra độ dài array từ pointer parameter bằng `sizeof`.

**Cơ chế.** Array expression thường decay thành pointer tới phần tử đầu; thông tin số phần tử mất tại boundary hàm. `pointer[index]` tương đương `*(pointer + index)`, vì vậy precondition `index < count` là điều kiện an toàn thật sự.

**Khi dùng / không dùng / trade-off.** Dùng view `const T *data, size_t count` cho dãy borrowed. Dùng struct chứa pointer+length nếu cặp này đi qua nhiều tầng. Không dùng pointer arithmetic giữa hai allocation khác nhau; abstraction rõ hơn làm API dài hơn nhưng giảm defect.

**Ví dụ cụ thể và oracle.** Với `int data[] = {2, 4, 6};` và hàm `sum(data, 3U)`, output phải là `12`; gọi với count `4` vi phạm contract dù binary có thể chưa crash. Artifact kiểm chứng là test boundary `count=0`, `count=3`, không phải quan sát tình cờ.

**Best practice.** **Rule:** truyền pointer cùng extent và giữ arithmetic trong `[begin, end]`. **Rationale:** pointer không mang capacity. **Positive:** loop `index < count`. **Negative:** `sizeof(parameter) / sizeof(parameter[0])` trong hàm trả tỷ lệ kích thước pointer, dẫn tới đọc sai biên.

**Failure/troubleshooting.** Dấu hiệu: kết quả thiếu/thừa phần tử → nguyên nhân mất extent hoặc off-by-one → chẩn đoán log `index/count` và boundary test → sửa contract pointer+count → phòng tránh bằng type view và test 0/1/max capacity.

#### OUT-B07-04 — Function pointer

**Mapping.** `OUT-B07-04` → `ADVC-H1SD` → `M00-FND-07`: khai báo một signature duy nhất cho strategy được dispatch gián tiếp.

**Định nghĩa và ranh giới.** Function pointer trỏ tới function có return type và parameter types tương thích. `typedef int (*value_predicate)(int, const void *);` đặt tên signature; function pointer không phải object pointer và không được chuyển đổi tùy tiện giữa các signature để “ép chạy”.

**Vai trò và quyết định.** Designer chọn signature tối thiểu đủ biểu đạt input, context, status và constness. Nếu function có signature khác, viết adapter type-correct thay vì cast che lỗi.

**Cơ chế.** Tên function trong hầu hết expression chuyển thành pointer tới function; lời gọi `predicate(value, context)` dispatch tới implementation được truyền. Compiler kiểm type ở assignment/call nếu không bị cast phá vỡ.

**Khi dùng / không dùng / trade-off.** Dùng cho strategy table, comparator, visitor và callback đồng bộ. Không dùng khi một `switch` nhỏ rõ hơn hoặc lifetime context khó bảo đảm. Dispatch gián tiếp tăng mở rộng nhưng có thể khó trace và cản inline; correctness ưu tiên trước micro-optimization.

**Ví dụ cụ thể và oracle.** Hai function `add(a,b)` và `maximum(a,b)` cùng signature `int(int,int)` được chọn qua table. Input `(4,7)` cho oracle `add=11`, `max=7`; gán function nhận `double` phải bị compiler từ chối, không cast.

**Best practice.** **Rule:** dùng `typedef`, không cast function pointer và kiểm pointer khác null trước gọi. **Rationale:** signature là contract ABI/type. **Positive:** `value_predicate predicate = at_least_threshold;`. **Negative:** cast `void (*)(void)` sang signature khác có thể làm sai cách truyền/đọc tham số.

**Failure/troubleshooting.** Dấu hiệu: warning incompatible-pointer-type hoặc crash tại indirect call → nguyên nhân signature/calling contract sai → chẩn đoán từ declaration và backtrace → sửa typedef/adapter → phòng tránh bằng `-Werror` và unit test từng entry trong dispatch table.

#### OUT-B07-05 — Callback function

**Mapping.** `OUT-B07-05` → `ADVC-H1SD` → `M00-FND-07`: kết hợp function pointer với context borrowed và output ownership rõ ràng.

**Định nghĩa và ranh giới.** Callback là function được caller cung cấp để callee gọi tại một điểm đã định. Context pointer mang state riêng cho lần gọi; trong demo callback đồng bộ, context chỉ cần sống đến khi `select_values` trả về. Callback bất đồng bộ cần lifetime dài hơn và không thuộc phạm vi Unit này.

**Vai trò và quyết định.** API phải công bố thời điểm/số lần gọi, null policy, khả năng re-enter, quyền sửa context và cách truyền lỗi. Demo chọn predicate thuần đọc: callback trả true/false, không giữ pointer và không dùng global.

**Cơ chế.** `select_values` duyệt array, gọi `predicate(value, context)`, rồi tổng hợp phần tử đạt. `build_summary(..., char **out_summary)` cấp phát và chỉ commit pointer khi hoàn tất; caller sở hữu và `free` đúng một lần.

**Khi dùng / không dùng / trade-off.** Dùng callback để tách traversal khỏi policy. Không dùng callback giữ địa chỉ stack sau return hoặc callback bí mật sửa global. Context làm API verbose hơn nhưng cho phép nhiều instance độc lập và test xác định.

**Ví dụ cụ thể và oracle.** Asset chạy threshold `8` với `3,8,13,5`; callback chọn `8,13`, output chính xác `OK threshold=8 selected=2 sum=21 first=8`. Input `3,x,13` bị parser từ chối trước callback với `ERROR invalid integer list`. Input `3, 8,13` bị pre-check whitespace từ chối, exit `2`, stdout rỗng, exact stderr `ERROR whitespace is not allowed`; `strtol` không được phép âm thầm chấp nhận khoảng trắng.

**Best practice.** **Rule:** callback nhận typed/opaque context với lifetime công bố; output pointer đặt null trước khi làm việc và commit atomically. **Rationale:** tránh global state và partial ownership. **Positive:** `*out_summary = NULL` rồi gán sau `snprintf` thành công. **Negative:** gán output sớm, thất bại sau đó khiến caller không biết có phải free hay không.

**Failure/troubleshooting.** Dấu hiệu: kết quả thay đổi giữa lần chạy hoặc double-free → nguyên nhân context/global state hoặc output commit một phần → chẩn đoán trace số lần gọi và ownership table → sửa context per-call, single commit/single free → phòng tránh bằng happy/zero-match/invalid/allocation-failure tests.

## 4. Biên portable và quy trình kiểm chứng

- Semantics của pointer/function pointer theo ISO C17; warning/sanitizer là bằng chứng cho path đã chạy, không phải proof cho mọi input.
- Demo không chuyển object pointer thành function pointer, không giả lập địa chỉ thiết bị và không dùng extension GNU trong source.
- Chạy thêm sanitizer khi toolchain hỗ trợ:

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O1 -g \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  assets/b07_callbacks_demo.c -o /tmp/b07_callbacks_san
/tmp/b07_callbacks_san 8 '3,8,13,5'
set +e
/tmp/b07_callbacks_san 8 '3, 8,13' >/tmp/b07_space.out 2>/tmp/b07_space.err
status=$?
set -e
test "$status" -eq 2
test ! -s /tmp/b07_space.out
test "$(cat /tmp/b07_space.err)" = 'ERROR whitespace is not allowed'
```

## 5. Thuật ngữ

| Thuật ngữ | Nghĩa trong Unit |
| --- | --- |
| pointee | Object/function được pointer định danh hợp lệ |
| borrowed pointer | Pointer không sở hữu; lifetime do bên khác bảo đảm |
| extent | Số phần tử/byte hợp lệ đi cùng pointer |
| function pointer | Pointer có signature tới function |
| callback | Function được truyền để callee gọi theo contract |
| context | State per-call đi cùng callback, thường qua `const void *` |
| pointer-to-pointer | Output parameter cho phép callee cập nhật pointer của caller |

## 6. Self-check Quiz (5 câu)

1. **`int *p = &x` có chuyển ownership của `x` cho `p` không?**  
   **Đáp án:** Không. Đây chỉ là alias; lifetime của `x` vẫn theo storage duration của `x`.
2. **One-past pointer có thể dereference không?**  
   **Đáp án:** Không. Nó chỉ hợp lệ cho một số phép arithmetic/so sánh trong cùng array; dereference là ngoài biên.
3. **Vì sao không cast để ép function pointer khác signature?**  
   **Đáp án:** Lời gọi sẽ không còn contract type-correct; cách truyền/đọc tham số và return có thể không tương thích, dẫn tới undefined behavior.
4. **Context callback đồng bộ phải sống đến khi nào?**  
   **Đáp án:** Ít nhất đến khi callee gọi callback lần cuối và trả về; callback không được giữ địa chỉ stack cho lần dùng sau.
5. **Vì sao `char **out` nên được đặt `NULL` trước khi cấp phát?**  
   **Đáp án:** Caller nhận trạng thái ownership xác định khi thất bại; chỉ pointer được commit sau thành công mới cần `free`.

## 7. Nguồn tham khảo và provenance phần bổ sung

- `SRC-USER-CREF`: outline portable C do người dùng phê duyệt ngày 2026-08-22.
- `SRC-C17-ISO`: [ISO/IEC 9899:2018 metadata](https://www.iso.org/standard/74528.html).
- `SRC-C17-WG14`: [WG14 N2176 public committee draft](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf), dùng để đối chiếu khái niệm, không thay bản chuẩn cuối.
- `SRC-CERTC`: [SEI CERT C Coding Standard](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/), dùng làm tham chiếu defensive contract.

**[BỔ SUNG — nguồn: `SRC-USER-CREF`]** Scenario record-tool, fixture và oracle là dữ liệu mô phỏng phục vụ học tập; không đại diện hệ thống khách hàng thật.
