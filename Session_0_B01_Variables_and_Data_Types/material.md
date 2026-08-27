# Session B01: Variables and Data Types — Học liệu cốt lõi

> **Khóa học:** [NFP] Advanced C Programming · **Ngôn ngữ:** ISO C17 · **Thuộc:** Optional C Basics Refresher

## 🎯 Learning Outcomes

- **ADVC-H1SD** [create] — Thiết kế, hiện thực và kiểm chứng mô-đun C17 không làm lộ ownership hoặc gây lỗi bộ nhớ trong fixture đã cho.

**Tiểu mục tiêu diễn giải (không phải ID Learning Outcome trong kế hoạch):** chọn kiểu/storage/linkage/qualifier theo miền dữ liệu; xây tagged record và chỉ chuyển kiểu sau khi kiểm tra miền giá trị.

## 🧭 Ngữ cảnh nghề nghiệp/dự án và phần tăng trưởng của Unit

**MDB Edge Diagnostics Gateway — Simulated:** học viên đóng vai C systems developer, bổ sung data model cho record telemetry local. Unit tạo milestone **M00-FND-01**: một executable C17 portable có oracle xác định tại `assets/b01_variables_demo.c`. Artifact nhận `id`, loại phép đo và giá trị dạng text; nó phải tạo record đúng kiểu hoặc từ chối input trước khi conversion làm mất dữ liệu. Kết quả được B02 dùng như nền tảng để xử lý nhiều record.

**Biên không nhúng:** mọi ví dụ chạy như process local. Không giả định MCU, thanh ghi, MMIO, ISR, DMA, HAL, RTOS, kích thước word hay endianness cụ thể. `volatile` chỉ được giải thích theo abstract machine của C, không được biến thành lời hứa về atomicity hoặc đồng bộ luồng.

## 📚 Nguồn đầu vào đã map

- **SRC-C17-ISO:** ISO/IEC 9899:2018, *Information technology — Programming languages — C*, Edition 4, ISO/IEC JTC 1/SC 22, 2018; truy cập 2026-08-22: https://www.iso.org/standard/74528.html
- **SRC-C17-WG14:** WG14 N2176, *C17 ballot — ISO/IEC 9899:2017, Programming languages — C*, 2017-10-09; truy cập 2026-08-22: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf
- **SRC-CERTC:** Carnegie Mellon University Software Engineering Institute, *SEI CERT C Coding Standard*, online work-in-progress snapshot constrained to C17; truy cập 2026-08-22: https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/
- **SRC-GCC11:** GNU Project / Free Software Foundation, *GCC 11.4 manuals*, version 11.4.0; phần tùy chọn dialect/diagnostic; truy cập 2026-08-22: https://gcc.gnu.org/onlinedocs/gcc-11.4.0/

---

## 1. Kiến thức tiên quyết và môi trường

### Kiến thức/kỹ năng tiên quyết

- Hoàn thành B00 hoặc biết quy trình preprocess → compile → link → run.
- Biết đọc stdout, stderr và exit code; phân biệt lỗi compile với lỗi nghiệp vụ lúc chạy.
- Biết dùng terminal tại thư mục `Session_0_B01_Variables_and_Data_Types`.

### Môi trường và kiểm tra nhanh

- Baseline course: Ubuntu 22.04/WSL2, GCC 11.4, glibc 2.35; mã nguồn không phụ thuộc Linux API.
- Chế độ bắt buộc: `-std=c17 -Wall -Wextra -Wpedantic -Werror`.
- Kiểm tra: `gcc --version` và `test -f assets/b01_variables_demo.c` phải thành công trước khi học.

## 2. Định vị trong lộ trình (Mental Map)

```text
B00: build/run/oracle
          │
          ▼
B01: value domain → C type → lifetime/linkage → aggregate/tag → checked conversion
          │
          ▼
B02: bounded array + decision + loop invariant
```

Mental model trung tâm: một biến không chỉ là “ô nhớ có tên”. Contract của nó gồm **miền giá trị**, **kiểu**, **lifetime**, **visibility/linkage**, **mutability** và **representation không được suy đoán ngoài điều chuẩn bảo đảm**. Thiết kế bắt đầu từ dữ liệu nghiệp vụ rồi mới chọn công cụ C.

## 3. Nội dung lý thuyết cốt lõi

**📋 Checklist bắt buộc phủ — giữ nguyên văn và đúng thứ tự Content Outlines:**

- [x] Introductory question?
- [x] Basic Data Types
- [x] Store Class
- [x] Key word for variable
- [x] Pointer variable
- [x] Struct Data type
- [x] Structure
- [x] Union
- [x] Enum
- [x] Casting

#### OUT-B01-01 Introductory question?

**Mapping:** Outline `OUT-B01-01 / Introductory question?` · LO `ADVC-H1SD` · Milestone `M00-FND-01`.

##### Định nghĩa và ranh giới

Câu hỏi mở đầu là một decision gate: “Dữ liệu này có miền giá trị, lifetime, ownership và trạng thái hợp lệ nào?” Nó không phải câu hỏi cú pháp “nên dùng `int` hay `long`”, vì cùng một cú pháp có thể sai khi domain khác nhau.

##### Vai trò và quyết định cần đưa ra

Người thiết kế record phải quyết định `id` có âm không, nhiệt độ có cần dấu không, record mang đồng thời hay chỉ một payload, và input text sai sẽ được báo thế nào. Quyết định này định hình public contract và oracle của gateway.

##### Cơ chế và mental model

Đi từ domain → invariant → type → validation boundary. `id ∈ [0, 2^32-1]` dẫn tới `uint32_t`; nhiệt độ milli-degree cần dấu dẫn tới `int32_t`; payload loại trừ lẫn nhau dẫn tới tagged union. Parser giữ giá trị ở kiểu rộng rồi kiểm tra trước khi thu hẹp.

##### Khi dùng / khi không dùng

Dùng gate này trước mọi declaration mang nghĩa nghiệp vụ. Không cần over-engineer biến đếm cục bộ hiển nhiên; trade-off là vài phút thiết kế đổi lấy ít conversion ngầm và ít trạng thái bất khả thi hơn.

##### Ví dụ cụ thể và oracle

- **Context/input:** operator gửi `id=17`, `kind=temp`, `raw=25375`.
- **Decision/artifact:** chọn `uint32_t`, `enum`, tagged `union` và `struct DiagnosticRecord`.
- **Expected/oracle:** chạy `./b01_variables_demo --record 17 temp 25375`; stdout phải đúng một dòng `record id=17 kind=TEMP_C raw=25375 whole=25 processed=1`, exit `0`.

##### Best practice

**Rule:** ghi domain cạnh API và kiểm tra tại biên parse → **rationale:** kiểu C không tự diễn đạt mọi invariant → **positive:** reject RPM `-1` trước cast → **negative:** cast thẳng `-1` sang unsigned tạo số lớn hợp lệ về bit nhưng sai nghiệp vụ.

##### Failure và troubleshooting

**Dấu hiệu:** output có RPM rất lớn từ `-1` → **nguyên nhân:** chọn/cast trước khi xác định domain → **chẩn đoán:** chạy negative oracle và xem exit code → **sửa:** parse kiểu rộng, range-check, rồi cast → **phòng tránh:** review mọi conversion tại trust boundary.

#### OUT-B01-02 Basic Data Types

**Mapping:** Outline `OUT-B01-02 / Basic Data Types` · LO `ADVC-H1SD` · `M00-FND-01`.

##### Định nghĩa và ranh giới

C17 có kiểu số nguyên, floating, `_Bool`, `void` và các derived type. `char`, `short`, `int`, `long` có thứ tự kích thước tối thiểu nhưng không có số bit cố định; `uint32_t` chỉ tồn tại khi implementation có đúng kiểu unsigned 32 bit. `sizeof` trả đơn vị byte C, còn `CHAR_BIT` quyết định số bit mỗi byte.

##### Vai trò và quyết định cần đưa ra

Chọn kiểu theo range, signedness và giao diện. Dùng `<stdint.h>` cho field có width protocol; dùng `size_t` cho kích thước object/mảng; dùng floating chỉ khi domain và sai số cho phép.

##### Cơ chế và mental model

Integer promotions và usual arithmetic conversions có thể đổi signedness trước phép toán. Vì vậy `uint32_t id` không đồng nghĩa mọi biểu thức chứa `id` đều tính trong cùng miền; literal và operand còn lại cũng quan trọng.

##### Khi dùng / khi không dùng

Dùng fixed-width ở record/file/protocol cần đúng số bit. Dùng `int` cho giá trị tự nhiên trong range của `int`; không dùng `uint32_t` cho mọi biến chỉ vì “rõ kích thước”, vì loop/index và phép trừ có thể khó xử lý hơn.

##### Ví dụ cụ thể và oracle

- **Input:** giới hạn hợp lệ lớn nhất `4294967295`.
- **Action:** `checked_u32()` nhận `int64_t`, so với `UINT32_MAX`, rồi gán.
- **Oracle:** `./b01_variables_demo --self-test` in `B01 SELF-TEST PASS checks=6`; `_Static_assert(sizeof(uint32_t) * CHAR_BIT == 32U, ...)` phải compile. `sizeof` tính theo byte C, nên không được giả định một byte luôn có tám bit.

##### Best practice

**Rule:** dùng macro limit/format của chuẩn → **rationale:** tránh giả định ABI → **positive:** `UINT32_MAX`, `PRIu32` → **negative:** `%lu` cho `uint32_t` có thể mismatch và là undefined behavior trong `printf`.

##### Failure và troubleshooting

**Dấu hiệu:** `-Wformat` hoặc số in sai → **nguyên nhân:** format không khớp promoted argument → **chẩn đoán:** bật `-Wformat=2`/warning profile → **sửa:** dùng `<inttypes.h>` → **phòng tránh:** không suy kiểu từ platform đang chạy.

#### OUT-B01-03 Store Class

**Mapping:** Outline `OUT-B01-03 / Store Class` · LO `ADVC-H1SD` · `M00-FND-01`.

##### Định nghĩa và ranh giới

Storage-class specifier gồm `auto`, `register`, `static`, `extern`, `_Thread_local` trong C17. Cần tách ba trục: storage duration, scope và linkage. `static` ở block cho static duration nhưng không tạo external linkage; `static` ở file scope tạo internal linkage.

##### Vai trò và quyết định cần đưa ra

Developer quyết định state tồn tại trong một call, suốt process hay theo thread; đồng thời quyết định symbol có lộ khỏi translation unit không. Đây là quyết định lifetime/API, không phải mẹo “đưa biến vào vùng data”.

##### Cơ chế và mental model

`candidate` trong `make_record()` có automatic duration cho mỗi call. `records_processed` ở file scope với `static` tồn tại suốt process và không được linker export như external symbol. Chạy process mới tạo lại state ban đầu.

##### Khi dùng / khi không dùng

Dùng automatic local làm mặc định. Dùng file-scope `static` cho implementation detail thật sự cần state lâu dài; tránh mutable global khi có thể truyền context vì nó làm test, reentrancy và concurrency khó hơn.

##### Ví dụ cụ thể và oracle

- **Action:** gọi executable hai lần độc lập với cùng input.
- **Oracle:** mỗi lệnh `./b01_variables_demo --record 17 temp 25375` đều kết thúc `processed=1`, chứng minh counter thuộc process, không bền qua lần chạy mới.

##### Best practice

**Rule:** chọn lifetime ngắn nhất đáp ứng contract → **rationale:** giảm alias và stale state → **positive:** `DiagnosticRecord candidate = {0};` local → **negative:** một record global dùng lại có thể rò dữ liệu giữa request.

##### Failure và troubleshooting

**Dấu hiệu:** test phụ thuộc thứ tự → **nguyên nhân:** mutable static state không reset → **chẩn đoán:** chạy test riêng và theo suite → **sửa:** truyền state hoặc cung cấp reset contract → **phòng tránh:** code review storage duration cùng ownership.

#### OUT-B01-04 Key word for variable

**Mapping:** Outline `OUT-B01-04 / Key word for variable` · LO `ADVC-H1SD` · `M00-FND-01`.

##### Định nghĩa và ranh giới

Các keyword ảnh hưởng declaration gồm type specifier, storage-class specifier và qualifier `const`, `volatile`, `restrict`, `_Atomic`. `const` ngăn sửa qua lvalue đó; `volatile` yêu cầu access observable theo implementation nhưng không tạo atomicity hay thread synchronization; `restrict` là promise aliasing có precondition nghiêm ngặt.

##### Vai trò và quyết định cần đưa ra

Chọn qualifier để mô tả quyền sửa và alias contract tại API. Input chỉ đọc nên là `const char *`; output cần ghi nên là pointer không-const. Không thêm `volatile` để “chữa” race.

##### Cơ chế và mental model

Trong `parse_i64(const char *text, int64_t *out)`, callee đọc chuỗi nhưng ghi qua `out`. Compiler kiểm tra mutation sai của `text`; caller vẫn chịu trách nhiệm lifetime của cả hai object.

##### Khi dùng / khi không dùng

Dùng `const` làm contract đọc-only. Chỉ dùng `volatile` cho object thật sự bị thay đổi ngoài luồng abstract-machine theo platform contract; trong refresher portable này không có đối tượng như vậy. Dùng `restrict` chỉ khi chứng minh non-aliasing.

##### Ví dụ cụ thể và oracle

- **Negative snippet:** thêm `text[0] = 'x';` vào `parse_i64`.
- **Oracle:** strict build phải thất bại với diagnostic kiểu “assignment of read-only location”; bản asset nguyên trạng build exit `0`.

##### Best practice

**Rule:** đặt `const` ở data mà function không sở hữu quyền sửa → **rationale:** compiler bắt contract violation sớm → **positive:** `print_record(const DiagnosticRecord *record)` → **negative:** bỏ `const` che giấu mutation ngoài ý muốn.

##### Failure và troubleshooting

**Dấu hiệu:** code thêm `volatile` nhưng race vẫn xảy ra → **nguyên nhân:** nhầm visibility với synchronization → **chẩn đoán:** xem có primitive atomic/thread contract không → **sửa:** dùng `_Atomic` hoặc mutex ở unit concurrency → **phòng tránh:** ghi rõ non-embedded boundary.

#### OUT-B01-05 Pointer variable

**Mapping:** Outline `OUT-B01-05 / Pointer variable` · LO `ADVC-H1SD` · `M00-FND-01`.

##### Định nghĩa và ranh giới

Pointer variable chứa địa chỉ hoặc null pointer và có pointed-to type. Pointer không tự mang length, ownership hay bằng chứng lifetime. `&object` tạo địa chỉ; `*pointer` dereference chỉ hợp lệ khi pointer trỏ tới object sống, đúng alignment/type và có quyền access.

##### Vai trò và quyết định cần đưa ra

Trong B01, pointer tạo output parameter cho parser/converter. API phải quyết định null có hợp lệ không, ai cấp object, ai sở hữu, và state output khi thất bại.

##### Cơ chế và mental model

`checked_u32(value, &id)` nhận địa chỉ `id`. Function kiểm tra `out != NULL`; chỉ ghi sau khi range pass, nên failure-atomic: output không bị ghi nửa chừng.

##### Khi dùng / khi không dùng

Dùng pointer khi callee cần quan sát/sửa object caller hoặc biểu diễn optional object. Không dùng pointer chỉ để tránh copy scalar nhỏ; value parameter đơn giản hơn và loại bỏ null/lifetime state.

##### Ví dụ cụ thể và oracle

- **Input/action:** self-test gọi `checked_u32(4294967296, &converted)`.
- **Expected/oracle:** hàm trả `false`, `--self-test` vẫn in `PASS checks=6`; không có dereference ngoài range/null.

##### Best practice

**Rule:** validate pointer trước dereference và ghi output cuối cùng → **rationale:** tránh null dereference/partial state → **positive:** candidate local rồi `*out = candidate` → **negative:** ghi từng field của `*out` trước khi validate kind để lại record dở dang.

##### Failure và troubleshooting

**Dấu hiệu:** crash tại `*out` → **nguyên nhân:** null/dangling pointer → **chẩn đoán:** warning, debugger backtrace, sanitizer khi có → **sửa:** bổ sung precondition/guard và sửa caller lifetime → **phòng tránh:** document ownership/lifetime cho mọi pointer parameter.

#### OUT-B01-06 Struct Data type

**Mapping:** Outline `OUT-B01-06 / Struct Data type` · LO `ADVC-H1SD` · `M00-FND-01`.

##### Định nghĩa và ranh giới

`struct` type gom các member có tên thành một record; declaration type khác với việc tạo object. `typedef struct { ... } DiagnosticRecord;` tạo alias type, chưa cấp một record cụ thể. Layout có thể chứa padding và phụ thuộc ABI.

##### Vai trò và quyết định cần đưa ra

Chọn member tối thiểu tạo invariant: `id`, `kind`, `value`. Người dùng record cần biết semantic field, không được phụ thuộc raw byte layout.

##### Cơ chế và mental model

Mỗi object `DiagnosticRecord` chứa storage cho mọi member; `kind` quyết định union member hợp lệ. Assignment `*out = candidate` sao chép giá trị struct theo C, kể cả padding không được xem như dữ liệu nghiệp vụ.

##### Khi dùng / khi không dùng

Dùng struct cho dữ liệu cùng tồn tại và cùng lifecycle. Không dùng để overlay packet/file bằng cast raw bytes; cần parser/serializer rõ endianness, padding và range.

##### Ví dụ cụ thể và oracle

- **Input:** `id=18`, kind `rpm`, raw `3200` trong self-test.
- **Artifact/oracle:** `make_record()` tạo object có `id==18`, `kind==READING_RPM`, `value.rpm==3200`; `--self-test` đạt check tương ứng.

##### Best practice

**Rule:** khởi tạo aggregate (`= {0}` hoặc designated initializer) → **rationale:** không đọc member indeterminate → **positive:** `DiagnosticRecord candidate = {0};` → **negative:** object automatic không init rồi in `id` gây undefined behavior.

##### Failure và troubleshooting

**Dấu hiệu:** record có field rác → **nguyên nhân:** thiếu initialization hoặc sai member → **chẩn đoán:** bật `-Wuninitialized`, kiểm tra constructor path → **sửa:** init toàn bộ và chỉ publish sau validation → **phòng tránh:** một creator function giữ invariant.

#### OUT-B01-07 Structure

**Mapping:** Outline `OUT-B01-07 / Structure` · LO `ADVC-H1SD` · `M00-FND-01`.

##### Định nghĩa và ranh giới

“Structure” ở leaf này là cách vận hành object struct: truy cập `object.member`, `pointer->member`, copy và truyền địa chỉ. Nó không đồng nghĩa layout byte ổn định hoặc inheritance như ngôn ngữ hướng đối tượng.

##### Vai trò và quyết định cần đưa ra

Thiết kế luồng tạo → validate → publish → chỉ đọc. `print_record()` nhận pointer const; `make_record()` dùng candidate local để caller không thấy trạng thái chưa hợp lệ.

##### Cơ chế và mental model

`record->id` tương đương `(*record).id` khi pointer hợp lệ. Struct pass-by-value copy toàn record; pointer tránh copy nhưng thêm precondition lifetime/null. B01 chọn pointer const cho printer và output pointer cho creator.

##### Khi dùng / khi không dùng

Truyền value cho record nhỏ, immutable khi ownership đơn giản; truyền pointer khi cần output/mutation hoặc tránh copy object lớn. Không tối ưu theo cảm tính; đo nếu size/speed là vấn đề.

##### Ví dụ cụ thể và oracle

- **Action:** `./b01_variables_demo --record 17 temp 25375`.
- **Oracle:** `print_record(&record)` đọc đúng các member và tính `whole=25`; stdout phải khớp hoàn toàn dòng đã công bố ở OUT-B01-01.

##### Best practice

**Rule:** giữ invariant trong creator/API thay vì để caller sửa tùy ý → **rationale:** giảm tổ hợp state sai → **positive:** caller nhận record chỉ sau `make_record()==true` → **negative:** caller set `kind=RPM` nhưng ghi `milli_celsius`.

##### Failure và troubleshooting

**Dấu hiệu:** kind và payload không khớp → **nguyên nhân:** member bị sửa ngoài creator → **chẩn đoán:** log cả tag và raw input, đặt watchpoint khi cần → **sửa:** centralize construction → **phòng tránh:** API const/read-only sau tạo.

#### OUT-B01-08 Union

**Mapping:** Outline `OUT-B01-08 / Union` · LO `ADVC-H1SD` · `M00-FND-01`.

##### Định nghĩa và ranh giới

`union` cho nhiều member dùng chung storage; tại một thời điểm chương trình phải biết member nào mang giá trị có nghĩa. Union không tự lưu tag và không phải cơ chế portable để type-pun, serialize hay đoán endianness.

##### Vai trò và quyết định cần đưa ra

Payload nhiệt độ và RPM loại trừ nhau, nên union tránh giữ cả hai. Quyết định bắt buộc là ghép union với `ReadingKind` và validate tag trước read.

##### Cơ chế và mental model

Ghi `candidate.value.rpm` thiết lập representation dùng cho member đó; `print_record()` branch theo `kind` rồi mới đọc `rpm` hoặc `milli_celsius`. Nếu tag sai, program đọc representation với nghĩa sai.

##### Khi dùng / khi không dùng

Dùng cho variant có tag/invariant rõ và memory trade-off có ích. Không dùng khi các field đồng thời tồn tại; khi đó struct thường đúng hơn. Không dùng raw union bytes làm external format.

##### Ví dụ cụ thể và oracle

- **Input/action:** `./b01_variables_demo --record 18 rpm 3200`.
- **Expected/oracle:** stdout `record id=18 kind=RPM raw=3200 processed=1`, exit `0`; output không có field nhiệt độ.

##### Best practice

**Rule:** mọi union nghiệp vụ phải có explicit tag và exhaustive access → **rationale:** union không tự track active member → **positive:** `switch (record->kind)` → **negative:** luôn đọc `.rpm` dù tag là temperature tạo semantic corruption.

##### Failure và troubleshooting

**Dấu hiệu:** raw bits hợp lệ nhưng đơn vị/output vô lý → **nguyên nhân:** tag/member mismatch → **chẩn đoán:** log tag tại write và read → **sửa:** tạo qua một constructor → **phòng tránh:** test mỗi variant và invalid tag.

#### OUT-B01-09 Enum

**Mapping:** Outline `OUT-B01-09 / Enum` · LO `ADVC-H1SD` · `M00-FND-01`.

##### Định nghĩa và ranh giới

Enumeration tạo tập tên hằng số kiểu `int`-compatible cho trạng thái/loại. C17 không biến enum thành closed set tại runtime: object enum vẫn có thể nhận giá trị không tương ứng enumerator qua input/cast lỗi. Kích thước representation là implementation-defined.

##### Vai trò và quyết định cần đưa ra

Enum thay magic numbers `1/2` bằng `READING_TEMP_C/READING_RPM`, giúp switch và review diễn đạt domain. Parser vẫn phải từ chối chuỗi kind ngoài tập.

##### Cơ chế và mental model

`parse kind → enumerator → switch`. `kind_name()` có `default` để không dereference payload cho tag lạ; creator chỉ publish hai enumerator hợp lệ.

##### Khi dùng / khi không dùng

Dùng cho tập lựa chọn nhỏ, ổn định trong process. Không ghi raw `sizeof(enum)` ra file/protocol; external format cần width xác định và mapping encode/decode.

##### Ví dụ cụ thể và oracle

- **Input/action:** `./b01_variables_demo --record 7 watts 12`.
- **Expected/oracle:** stderr `error: kind must be 'temp' or 'rpm'`, stdout rỗng, exit `2`.

##### Best practice

**Rule:** validate external value và xử lý `default` → **rationale:** enum không bảo đảm chỉ có enumerator → **positive:** creator reject `watts` → **negative:** cast số tùy ý rồi index array bằng enum có thể out-of-bounds.

##### Failure và troubleshooting

**Dấu hiệu:** `UNKNOWN` hoặc branch không dự kiến → **nguyên nhân:** input/tag chưa validate → **chẩn đoán:** in numeric tag, kiểm tra parser mapping → **sửa:** reject tại boundary → **phòng tránh:** test unknown value và không serialize raw enum.

#### OUT-B01-10 Casting

**Mapping:** Outline `OUT-B01-10 / Casting` · LO `ADVC-H1SD` · `M00-FND-01`.

##### Định nghĩa và ranh giới

Cast yêu cầu conversion rõ cú pháp; nó không chứng minh giá trị representable hay pointer hợp lệ. Narrowing integer có thể đổi giá trị; signed/unsigned conversion theo modulo hoặc implementation rules; pointer/integer cast có portability giới hạn.

##### Vai trò và quyết định cần đưa ra

Boundary CLI parse vào `int64_t`; developer phải range-check trước cast sang field. Cast nằm sau proof, không thay proof.

##### Cơ chế và mental model

`checked_u32()` kiểm `value >= 0` và `(uint64_t)value <= UINT32_MAX`, sau đó mới `(uint32_t)value`. Short-circuit bảo đảm cast sang `uint64_t` chỉ được so trong nhánh đã loại số âm.

##### Khi dùng / khi không dùng

Dùng cast khi conversion đã được chứng minh và muốn làm intent rõ. Không cast để tắt warning pointer/type; warning thường chỉ ra contract mismatch cần sửa.

##### Ví dụ cụ thể và oracle

- **Input/action:** `./b01_variables_demo --record 17 rpm -1`.
- **Expected/oracle:** stderr `error: value '-1' is outside uint32 range for RPM`, stdout rỗng, exit `2`; không xuất hiện `4294967295`.

##### Best practice

**Rule:** check range ở kiểu nguồn rộng rồi cast một lần → **rationale:** sau narrowing không thể phân biệt giá trị gốc bị mất → **positive:** `checked_i32()`/`checked_u32()` → **negative:** `(uint32_t)parsed_value` rồi kiểm `>= 0` là quá muộn.

##### Failure và troubleshooting

**Dấu hiệu:** wrap hoặc sign flip → **nguyên nhân:** narrowing/unsigned conversion trước validation → **chẩn đoán:** test boundary `-1`, `INT32_MAX+1`, `UINT32_MAX+1` → **sửa:** helper checked conversion → **phòng tránh:** bật `-Wconversion` khi audit và giữ boundary tests.

## 4. Ví dụ tích hợp liên khái niệm có thể kiểm chứng

### Input/trạng thái ban đầu

CLI nhận record temperature `17 temp 25375`. `id` và value ban đầu là text chưa tin cậy; process counter bắt đầu `0`.

### Cách thực hiện

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror \
  assets/b01_variables_demo.c -o b01_variables_demo
./b01_variables_demo --self-test
./b01_variables_demo --record 17 temp 25375
```

Parser dùng scalar rộng, output pointer và checked cast; creator ghép enum, tagged union và struct; file-scope static counter chỉ tăng sau khi record hợp lệ.

### Output mong đợi

```text
B01 SELF-TEST PASS checks=6
record id=17 kind=TEMP_C raw=25375 whole=25 processed=1
```

### Cách xác minh

- Build exit `0`, không warning.
- Hai command run exit `0`; stdout phải byte-for-byte như trên, stderr rỗng.
- Negative: `./b01_variables_demo --record 17 rpm -1` phải stderr đúng `error: value '-1' is outside uint32 range for RPM`, exit `2`.

## 5. Lỗi thường gặp, troubleshooting và quy tắc áp dụng

| Dấu hiệu | Nguyên nhân | Chẩn đoán | Sửa | Phòng tránh |
|---|---|---|---|---|
| RPM từ `-1` thành số rất lớn | Cast unsigned quá sớm | Chạy negative oracle, xem điểm cast | Range-check ở `int64_t` | Boundary tests cho min/max ±1 |
| Record đôi lúc có field rác | Automatic object chưa init | `-Wuninitialized`, trace constructor | `= {0}` và publish cuối | Một creator giữ invariant |
| Tag đúng nhưng payload sai đơn vị | Union member/tag mismatch | Log tag tại write/read | Switch exhaustive, constructor tập trung | Test từng variant |
| Test phụ thuộc thứ tự | Mutable static state | Chạy riêng và theo suite | Truyền context/reset rõ | Lifetime ngắn nhất |
| Build qua compiler này, lỗi compiler khác | Giả định width/format/layout | Strict build trên GCC/Clang | `<stdint.h>`, `<inttypes.h>`, không serialize struct thô | Ghi rõ implementation-defined choices |

## 6. Từ điển thuật ngữ và mô hình tư duy

- **Object:** vùng storage có value/type/lifetime theo abstract machine C.
- **Scope:** vùng source nơi identifier nhìn thấy; khác **linkage**, quan hệ giữa declaration ở các scope/translation unit.
- **Storage duration:** khoảng tồn tại của object: automatic, static, thread hoặc allocated.
- **Qualifier:** thuộc tính contract như `const`, `volatile`, `restrict`, `_Atomic`.
- **Aggregate:** struct/array; struct chứa đồng thời các member.
- **Tagged union:** union đi kèm tag enum chỉ rõ member hợp lệ.
- **Narrowing conversion:** conversion sang miền biểu diễn hẹp hơn.
- **Oracle:** output/exit code cụ thể dùng để quyết định pass/fail, không phải nhận xét “trông đúng”.

## 7. Nguồn tham khảo và provenance phần bổ sung

- C17/WG14 N2176: §§6.2 (scope, linkage, storage duration, types), 6.3 (conversions), 6.5.4 (cast), 6.7.2 (struct/union/enum), 7.20 (`<stdint.h>`), 7.8 (`<inttypes.h>`).
- GCC 11.4 manual: C dialect và diagnostic options; GCC chỉ là implementation evidence, không thay chuẩn.
- CERT C: DCL, INT, EXP rules được dùng làm safety rationale; nội dung diễn giải độc lập, không sao chép quy tắc dài.
- Không có giả định embedded hoặc Linux ABI trong data model. Output CLI và exit-code convention là thiết kế simulated của course, không phải yêu cầu của ISO C.

## 8. Self-check Quiz

### Câu hỏi

1. Vì sao `uint32_t rpm = (uint32_t)parsed; if (rpm <= UINT32_MAX)` không phát hiện input `-1`?
2. `static` ở file scope đồng thời ảnh hưởng hai trục nào?
3. Vì sao `volatile` không phải giải pháp cho data race?
4. Khi nào union trong record được đọc an toàn về mặt contract của bài?
5. Có nên ghi `sizeof(DiagnosticRecord)` byte trực tiếp ra file để trao đổi giữa hai máy không?

### Đáp án có giải thích

1. Cast đã chuyển `-1` sang miền unsigned trước kiểm tra; mọi `uint32_t` đương nhiên `<= UINT32_MAX`. Phải kiểm ở kiểu nguồn rộng rồi cast.
2. Object có static storage duration và identifier có internal linkage; scope vẫn là file scope. Ba khái niệm này không đồng nghĩa.
3. `volatile` kiểm soát observable access theo C implementation, không tạo atomic read-modify-write hay happens-before. Cần atomic/mutex theo contract concurrency.
4. Khi tag đã được validate và code chỉ đọc member tương ứng tag; constructor phải duy trì invariant này.
5. Không. Padding, alignment, enum/endianness và representation có thể khác. Hãy encode từng field vào external format được đặc tả.
