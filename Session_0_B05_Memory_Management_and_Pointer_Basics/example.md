# Session B05 — Worked Example: Dynamic vector có ownership kiểm chứng được

## 🎯 Learning Outcomes liên quan

- `ADVC-H1SD` — dùng pointer, allocation và ownership contract để tạo một vector C17 có thể kiểm chứng.

## CASE-B05-01 · Ticket và tiêu chí thành công

Một C systems developer cần bàn giao increment `M00-FND-05`: chương trình C17 nhận tập dữ liệu synthetic cố định, lưu trong vector tự tăng, tính tổng bằng range pointer hợp lệ, rồi giải phóng ownership. Artifact phải có happy path và lỗi overflow xác định; evidence ELF/MAP nếu thu thập phải được tách khỏi kết luận lifetime portable.

- **LO:** `ADVC-H1SD`.
- **Artifact:** `assets/b05_memory_demo.c`.
- **Baseline:** Ubuntu 22.04, GCC 11.4; code lõi chỉ dùng ISO C17.
- **Success:** strict build exit `0` và zero warnings; hai oracle dưới đây khớp `stdout`/`stderr`/exit code; không leak hay invalid access trong ca đã cho.

### Mapping đầy đủ

| Outline ref | Phần hiện thực/bằng chứng trong CASE-B05-01 |
|---|---|
| `OUT-B05-01` Memory layout | Contract lifetime: object vector automatic sở hữu allocation cho tới `vector_destroy`; không suy từ stack/heap. |
| `OUT-B05-02` Variable and memory location | `vector`, `data`, `size`, `capacity`; test nội dung/quan hệ, không test địa chỉ tuyệt đối. |
| `OUT-B05-03` Linker file and memory | Lane evidence riêng với GNU `-Map`/`readelf`; không dùng để chứng minh lifetime. |
| `OUT-B05-04` Pointer variable | `data` là owning pointer; các cursor trong phép tính tổng là non-owning, chỉ đọc. |
| `OUT-B05-05` Assigning values to a pointer | `realloc` vào temporary `next`, chỉ commit vào `data` khi thành công. |
| `OUT-B05-06` Memory allocation for a pointer | growth `0→4→8`, overflow guard, `free`, reset trạng thái. |
| `OUT-B05-07` Pointer arithmetic | traversal half-open `[data, data + size)`; không dereference one-past. |

## Input, trạng thái đầu và ràng buộc

- Happy input được nhúng để self-test tái lập: `{3, -1, 7, 0, 5}`.
- Vector khởi tạo `{ .data = NULL, .size = 0, .capacity = 0 }`.
- Growth nhân đôi, capacity đầu tiên là `4`; mọi phép nhân byte phải qua guard `capacity <= SIZE_MAX / sizeof *data`.
- Owner duy nhất là `int_vector_t.data`; hàm destroy phải trả object về trạng thái rỗng.
- Negative input logic là capacity `SIZE_MAX / sizeof(int) + 1`, phải bị từ chối trước khi gọi allocator.
- Không đưa địa chỉ, tên section hay offset ELF vào functional oracle.

## Phân tích lựa chọn và trade-off

Chọn dynamic array vì workload cần append và scan tuần tự: locality tốt và truy cập theo index hằng số trong mô hình RAM thông thường. Growth nhân đôi cho số lần reallocation ít hơn growth từng phần tử, đổi lại có thể giữ capacity dư. Một linked list tránh di chuyển toàn bộ allocation nhưng tăng overhead mỗi node, locality kém và không có random access trực tiếp.

Checked `realloc` giữ failure atomicity: nếu thất bại, vector cũ còn nguyên. Sau một `realloc` thành công, mọi view cũ vào allocation trước phải được xem là hết hiệu lực; vì vậy case không giữ cursor qua thao tác grow. `vector_destroy` reset cả ba trường để trạng thái hậu điều kiện dễ kiểm tra, nhưng reset một owner không tự sửa các alias bên ngoài—API này không cho alias sống qua destroy.

## Hiện thực cần đọc

Mở `assets/b05_memory_demo.c` và lần theo bốn invariant:

1. `size <= capacity`.
2. `data == NULL` khi capacity bằng `0`; sau khi cấp phát, `data != NULL`.
3. Phép nhân kích thước chỉ xảy ra sau overflow guard.
4. Cursor chỉ được dereference khi nhỏ hơn pointer one-past `data + size`.

`--self-test` triển khai happy path; `--negative` gọi đúng checked-allocation boundary. Mọi đối số khác trả usage error `64`, giúp CLI không im lặng chạy sai mode.

## Build strict và functional oracle portable

Từ thư mục Unit:

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  assets/b05_memory_demo.c -o b05_memory_demo
```

Oracle build: exit `0`, `stdout` rỗng, `stderr` rỗng. Chạy happy path:

```sh
./b05_memory_demo --self-test
```

Oracle chính xác:

- exit code: `0`
- `stderr`: rỗng
- `stdout`:

```text
size=5 capacity=8 sum=14 first=3 last=5
ownership=destroyed data=null size=0 capacity=0
self-test=PASS
```

Chạy và bắt negative path mà không làm mất exit code:

```sh
set +e
./b05_memory_demo --negative >negative.out 2>negative.err
rc=$?
set -e
printf 'exit=%d\n' "$rc"
```

Oracle chính xác:

- `rc` là `2`.
- `negative.out` có kích thước `0` byte.
- `negative.err`:

```text
error: allocation size overflow
```

Lỗi phải xuất hiện trước allocator; không chấp nhận “allocator trả NULL” như oracle thay thế vì điều đó phụ thuộc môi trường.

## Lane evidence GNU/Linux riêng biệt

Phần này quan sát artifact trên baseline GNU/Linux; nó không thay đổi hay chứng minh contract ISO C ở trên.

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  assets/b05_memory_demo.c -Wl,-Map=b05_memory_demo.map \
  -o b05_memory_demo_elf
test -s b05_memory_demo.map
grep -q 'main' b05_memory_demo.map
readelf -h b05_memory_demo_elf | grep -q 'ELF'
readelf -s b05_memory_demo_elf | grep -q '[[:space:]]main$'
```

Oracle cục bộ: từng lệnh exit `0`; map không rỗng; ELF header và symbol `main` được tìm thấy. Không so địa chỉ, section size hoặc offset cố định. Tối ưu hóa, strip, LTO hay toolchain khác có thể đổi evidence dù functional oracle vẫn đúng.

Nếu đúng baseline có Valgrind 3.18.1:

```sh
valgrind --leak-check=full --error-exitcode=99 \
  ./b05_memory_demo --self-test >valgrind.stdout 2>valgrind.stderr
```

Oracle: exit `0`, `valgrind.stdout` bằng happy stdout; báo cáo Memcheck chứa `ERROR SUMMARY: 0 errors from 0 contexts`. Đây là evidence tool-specific bổ sung, không thay cho code review lifetime/ownership.

## Vì sao output đúng

Năm lần append tạo `size=5`. Capacity đi từ `0` lên `4` ở phần tử đầu và lên `8` khi thêm phần tử thứ năm. Range pointer đọc đúng năm giá trị, nên tổng là `3 + (-1) + 7 + 0 + 5 = 14`; first/last là `3`/`5`. Destroy gọi `free`, rồi reset `data`, `size`, `capacity`, tạo dòng ownership xác định.

Negative case tạo capacity vừa lớn hơn thương số an toàn. Guard nhận biết phép nhân với `sizeof(int)` sẽ vượt miền `size_t`, trả trạng thái overflow và in đúng một dòng lỗi; không cấp phát, không leak.

## Failure modes và chẩn đoán

| Dấu hiệu | Nguyên nhân | Bằng chứng chẩn đoán | Sửa | Phòng ngừa |
|---|---|---|---|---|
| Capacity không phải `8` | growth rule bị đổi hoặc increment sai | log sequence capacity | phục hồi rule `0→4`, sau đó nhân đôi có guard | test boundary phần tử 4/5 |
| Tổng sai/invalid read | loop dereference one-past | review điều kiện cursor, Memcheck | dùng `cursor < end` | invariant `[begin,end)` |
| Leak khi grow lỗi | gán `realloc` trực tiếp cho owner | fault injection/Memcheck | temporary rồi commit | helper grow duy nhất |
| Double-free | hai owner hoặc destroy không có contract | trace owner transitions | một owner, reset sau free | document move/destroy rules |
| MAP khác giữa hai build | flags/linker/version khác | so command manifest | đánh giá evidence theo baseline | không dùng address/offset làm oracle |

## Bài học chuyển giao

Thiết kế memory-safe trong C bắt đầu từ lifetime và ownership, rồi mới chọn allocator/tool. Pointer arithmetic chỉ hợp lệ trong miền array; `realloc` là một transaction cần temporary và commit. MAP/ELF giúp quan sát binary cụ thể, còn đúng/sai của dereference phải được lập luận theo ISO C.

## Practice Time — không chấm điểm, không có lời giải

Tạo bản sao riêng của asset và mở rộng self-test bằng bộ input mới `{8, -3, 12, 4}`. Không sửa asset gốc. Yêu cầu:

- Vẫn dùng cùng ownership contract, overflow guard và traversal `[begin,end)`.
- Không in địa chỉ.
- Thêm một negative test độc lập cho capacity `SIZE_MAX / sizeof(int) + 1`.
- Strict build phải exit `0`, zero warnings.

Oracle happy path mới:

```text
size=4 capacity=4 sum=21 first=8 last=4
ownership=destroyed data=null size=0 capacity=0
self-test=PASS
```

Exit `0`, `stderr` rỗng. Negative path phải exit `2`, `stdout` rỗng và `stderr`:

```text
error: allocation size overflow
```

Không dùng MAP/ELF để suy lifetime. Nộp source bản sao, command build/run và ba giá trị oracle (`stdout`, `stderr`, exit code); phần này cố ý không cung cấp code lời giải.

## Provenance của các case

- **ISO/IEC 9899:2018 — Programming languages — C**, ISO/IEC JTC 1/SC 22, Edition 4, 2018, truy cập 2026-08-22: https://www.iso.org/standard/74528.html
- **WG14 N2176 — C17 committee draft**, ISO/IEC JTC 1/SC 22/WG14, 2017-10-09, truy cập 2026-08-22: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf
- **Using LD / GNU Binary Utilities 2.38**, GNU Project/FSF, truy cập 2026-08-22: https://sourceware.org/binutils/docs-2.38/ld/ và https://sourceware.org/binutils/docs-2.38/binutils/
- **Valgrind User Manual — Memcheck**, Valgrind Developers, baseline 3.18.1, truy cập 2026-08-22: https://valgrind.org/docs/manual/mc-manual.html

Case và dữ liệu là synthetic; GNU/Linux evidence được gắn nhãn riêng, còn functional contract là ISO C17.
