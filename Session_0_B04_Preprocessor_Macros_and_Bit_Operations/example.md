# Session B04: Preprocessor Macros and Bit Operations — Worked Example

> **CASE-B04-01 · ADVC-H1SD · M00-FND-04 · simulated · ISO C17**

## 1. Ticket và ranh giới

**Ticket:** tạo một module cấu hình synthetic cho gateway. Module giữ `ENABLE` ở bit 0, `READY` ở bit 2 và `MODE` ở bits 4..6. Maintainer cần build C17 strict, đọc/ghi field không phá bit khác, và nhận lỗi xác định khi mode không fit.

**Ranh giới:** “Peripheral Access Layer” ở đây chỉ là API trên `register_image_t` trong memory. Không có địa chỉ thiết bị, I/O vật lý, driver, HAL hay hành vi embedded. Đây là pure logic có thể unit-test.

**Artifact:** `assets/b04_macro_bits_demo.c`.

## 🎯 Learning Outcomes liên quan

| Outline ref | Cách CASE-B04-01 hiện thực/chứng minh |
|---|---|
| OUT-B04-01 | guard C17, include, expansion và strict translation |
| OUT-B04-03 | `#define` có contract hẹp |
| OUT-B04-04 | `B04_VARIANT`, `MODE_MASK`, `MODE_SHIFT` |
| OUT-B04-05 | macro stringification/declaration; hành vi runtime dùng hàm |
| OUT-B04-07 | include trực tiếp `<stdint.h>`, `<inttypes.h>`, `<stdbool.h>` |
| OUT-B04-08 | `#ifndef B04_VARIANT` và build override |
| OUT-B04-09 | hai `#error` fail-fast |
| OUT-B04-10 | `#`, `##` qua `STRINGIFY` và `DECLARE_FLAG` |
| OUT-B04-12 | unsigned AND/OR/NOT/SHIFT |
| OUT-B04-14 | mode mask `0x70`, shift `4`, range `0..7` |
| OUT-B04-15 | set/clear idempotent |
| OUT-B04-16 | validate-then-commit, read/write field |
| OUT-B04-17 | `register_image_t` và API in-memory; không I/O phần cứng |

## 3. Input, constraint và lựa chọn

- **Happy input:** image `0`; set ENABLE+READY; ghi MODE=`5`; clear READY.
- **Negative input:** image `0xA5`; thử MODE=`8`, vượt field ba bit.
- **Constraints:** exact `uint32_t`, shift `<32`, rejected write giữ nguyên state, không argument macro có side effect, stdout/stderr tách biệt.
- **Quyết định:** macro chỉ xử lý token/hằng build-time; năm thao tác runtime là hàm `static`. Việc này giữ type checking và evaluate-once.
- **Trade-off:** fixed layout rất gọn và nhanh, nhưng field khó mở rộng; module phải document mask/shift. Pure in-memory test được mọi host C17, nhưng không mô hình hóa timing/side effect của thiết bị thật.

## 4. Cơ chế triển khai

`DECLARE_FLAG(ENABLE, 0)` dùng `##` để sinh `ENABLE_MASK`; `STRINGIFY(B04_VARIANT)` dùng hai tầng để mở rộng rồi stringify. `field_write` thực hiện:

1. kiểm `mask != 0`, `shift < 32`;
2. tính `field_max = mask >> shift`;
3. từ chối `field_value > field_max` mà chưa mutate;
4. clear đúng mask và OR giá trị đã shift.

Set/clear chỉ đổi bit thuộc mask. Negative path snapshot state và tự kiểm rejected write không đổi image trước khi phát lỗi nghiệp vụ.

## 5. Build và happy-path oracle

Chạy từ thư mục Unit B04:

```bash
set -eu
rm -f -- b04_macro_bits_demo happy.out happy.err
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  assets/b04_macro_bits_demo.c -o b04_macro_bits_demo
./b04_macro_bits_demo --self-test >happy.out 2>happy.err
test ! -s happy.err
test "$(cat happy.out)" = "$(printf '%s\n' \
  'config=portable_c17' \
  'initial=0x00000000' \
  'after_set=0x00000005' \
  'after_mode=0x00000055 mode=5' \
  'after_clear=0x00000051 enabled=1' \
  'self-test=PASS')"
```

**Oracle:** compile exit `0` và zero warnings; chương trình exit `0`; stderr rỗng; stdout chính xác sáu dòng:

```text
config=portable_c17
initial=0x00000000
after_set=0x00000005
after_mode=0x00000055 mode=5
after_clear=0x00000051 enabled=1
self-test=PASS
```

Để kiểm conditional compilation:

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  -DB04_VARIANT=training assets/b04_macro_bits_demo.c -o b04_training
test "$(./b04_training --self-test | head -n1)" = 'config=training'
```

## 6. Negative-path oracle

```bash
set +e
./b04_macro_bits_demo --negative >negative.out 2>negative.err
rc=$?
set -e
test "$rc" -eq 2
test ! -s negative.out
test "$(cat negative.err)" = \
  'error: mode value 8 does not fit mask 0x00000070'
```

**Oracle:** exit `2`; stdout đúng `0` byte; stderr chính xác:

```text
error: mode value 8 does not fit mask 0x00000070
```

Lỗi usage là contract khác: không argv hoặc argv lạ phải exit `64` và stderr `usage: b04_macro_bits_demo --self-test|--negative`.

## 7. Vì sao kết quả đúng và failure modes

- `0x00 | 0x01 | 0x04 = 0x05`.
- Ghi mode: `(0x05 & ~0x70) | ((5 << 4) & 0x70) = 0x55`.
- Clear READY: `0x55 & ~0x04 = 0x51`.
- `8 > (0x70 >> 4)`, nên negative path fail trước assignment.

| Dấu hiệu | Nguyên nhân | Bằng chứng | Fix/prevent |
|---|---|---|---|
| mode 8 trở thành 0 | mask cắt bit cao | negative oracle không còn exit 2 | range-check trước shift/mask |
| bit ENABLE mất khi ghi MODE | overwrite cả word | expected `0x55`, actual `0x50` | read-modify-write |
| side effect chạy hai lần | macro runtime lặp argument | preprocessed output | đổi thành `static inline`/hàm |
| build C11 vẫn qua | thiếu guard/dialect | compile command `-std=c11` | giữ `#error` và profile C17 |

## 8. Practice Time — tự làm, không chấm điểm, không có lời giải

Mở rộng một bản sao của demo bằng trường `RATE` ở bits 8..11 và flag `LOCKED` ở bit 15.

- **Input mới:** image ban đầu `0x00000001`; ghi RATE=`10`; set LOCKED; đọc lại RATE; sau đó thử RATE=`16`.
- **Constraint:** không đổi code gốc trong `assets`; tạo file scratch; không địa chỉ thiết bị; invalid write giữ nguyên state; không dùng macro evaluate argument nhiều lần.
- **Happy oracle chính xác:** stdout của scratch phải là:

```text
after_rate=0x00000A01 rate=10
after_lock=0x00008A01 locked=1
practice=PASS
```

- **Negative oracle:** RATE=`16` phải exit `2`, stdout rỗng, stderr chính xác `error: rate value 16 does not fit mask 0x00000F00`; image vẫn `0x00008A01` theo assertion nội bộ.
- **Build oracle:** `gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2` exit `0`, không warning.

Không có skeleton hay implementation mẫu; mục tiêu là chuyển giao quy tắc validate-then-commit sang mask mới.

## Provenance của các case

- [ISO/IEC 9899:2018 — Programming languages — C](https://www.iso.org/standard/74528.html), ISO/IEC JTC 1/SC 22, Edition 4 (C17), 2018, truy cập 2026-08-22; clauses về preprocessing và bitwise expressions.
- [WG14 N2176 — C17 committee draft](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf), ISO/IEC JTC 1/SC 22/WG14, 2017-10-09, bản draft công khai không phải ấn bản ISO cuối, truy cập 2026-08-22.
- [SEI CERT C Coding Standard](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/), Carnegie Mellon University Software Engineering Institute, snapshot online, truy cập 2026-08-22; PRE/EXP/INT guidance.
- [GCC 11.4 manuals](https://gcc.gnu.org/onlinedocs/gcc-11.4.0/), GNU Project/Free Software Foundation, version 11.4.0, truy cập 2026-08-22; C dialect, preprocessor và warning options.

`[BỔ SUNG — nguồn: các nguồn trên]` Case, fixture và oracles là dữ liệu synthetic được biên soạn mới; không sao chép code từ chuẩn/tài liệu.
