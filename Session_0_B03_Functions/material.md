# Session B03: Functions — Học liệu cốt lõi

> **Khóa học:** [NFP] Advanced C Programming · **Ngôn ngữ:** ISO C17 · **Thuộc:** Optional C Basics Refresher

## 🎯 Learning Outcomes

- **ADVC-H1SD** [create] — Hiện thực và kiểm chứng mô-đun C17 có function contract, ownership/bounds và failure behavior quan sát được.

**Tiểu mục tiêu diễn giải (không phải ID Learning Outcome trong kế hoạch):** thiết kế prototype/linkage; chọn function, `static inline`, macro hay variadic API có chủ đích; truyền dữ liệu/multiple output an toàn; giới hạn recursion; thay GNU nested function bằng callback + explicit context portable.

## 🧭 Ngữ cảnh nghề nghiệp/dự án và phần tăng trưởng của Unit

Trong **MDB Edge Diagnostics Gateway — Simulated**, B03 tạo milestone **M00-FND-03**: tách pipeline parse–analyze–present thành các function có precondition, output và lỗi rõ. Artifact kế hoạch là `assets/b03_functions_demo.c`; nó tổng hợp một batch bằng prototype, file-local helper, `static inline`, output pointer, array + length, variadic function, recursion có bound và callback có context.

**Biên không nhúng:** input là `argv`; artifact không chạy trong ISR, không truy cập MMIO, không đo WCET/stack thực và không bảo đảm thread safety. ISO C mô tả behavior của function nhưng không bắt buộc implementation phải dùng một “stack” vật lý. Kỹ thuật contract và bound chuyển giao được sang embedded; budget thời gian/bộ nhớ phải được đo lại trên target/toolchain thật.

## 📚 Nguồn đầu vào đã map

- **SRC-C17-ISO:** ISO/IEC 9899:2018, *Information technology — Programming languages — C*, Edition 4, ISO/IEC JTC 1/SC 22, 2018; truy cập 2026-08-22: https://www.iso.org/standard/74528.html
- **SRC-C17-WG14:** WG14 N2176, *C17 ballot — ISO/IEC 9899:2017, Programming languages — C*, 2017-10-09; truy cập 2026-08-22: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf
- **SRC-GCC11:** GNU Project / Free Software Foundation, *GCC 11.4 manuals*, version 11.4.0; xem chính xác *An Inline Function is As Fast As a Macro* tại https://gcc.gnu.org/onlinedocs/gcc-11.4.0/gcc/Inline.html và *Nested Functions* tại https://gcc.gnu.org/onlinedocs/gcc-11.4.0/gcc/Nested-Functions.html; truy cập 2026-08-22.
- **SRC-GLIBC235:** GNU Project, *GNU C Library Reference Manual*, glibc 2.35; xem Appendix A.2 *How Variadic Functions are Defined and Used* trong PDF https://sourceware.org/glibc/manual/2.35/pdf/libc.pdf; truy cập 2026-08-22.
- **SRC-CERTC:** Carnegie Mellon University Software Engineering Institute, *SEI CERT C Coding Standard*, online snapshot constrained to C17; truy cập 2026-08-22: https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/

---

## 1. Kiến thức tiên quyết và môi trường

### Kiến thức/kỹ năng tiên quyết

- Hoàn thành B01–B02; hiểu type, pointer, array + count, decision và loop bounds.
- Biết phân biệt stdout/stderr/exit code và chạy strict compiler gate.
- Nhận biết undefined behavior, translation unit và external/internal linkage ở mức nhập môn.

### Môi trường và kiểm tra nhanh

- Baseline khóa: Ubuntu 22.04/WSL2, GCC 11.4; source chỉ cần hosted ISO C17.
- Build gate: `gcc -std=c17 -Wall -Wextra -Wpedantic -Werror`.
- Asset gate: `./b03_functions_demo --self-test` phải in đúng `B03 SELF-TEST PASS checks=9`.
- `-Wpedantic -Werror` là portability gate quan trọng: một nested function kiểu GNU phải bị loại khỏi artifact ISO C17.

## 2. Định vị trong lộ trình (Mental Map)

```text
B01 type + pointer ─┐
B02 array + flow ───┴→ B03 function contract
                         ├→ declaration/linkage
                         ├→ value, pointer, array + length, multiple output
                         ├→ inline/macro/varargs decision
                         └→ callback + context / bounded recursion
                                      ↓
                           B04 interface/preprocessor
```

Function boundary là nơi biến giả định thành contract: input nào hợp lệ, object nào được phép sửa, output nào chỉ có giá trị khi success, và resource nào bị giới hạn. Prototype là bằng chứng compiler kiểm được; test oracle là bằng chứng runtime kiểm được.

## 3. Nội dung lý thuyết cốt lõi

**📋 Checklist bắt buộc phủ — giữ nguyên văn và đúng thứ tự Content Outlines:**

- [x] 1 What is function
- [x] 1.1 Syntax
- [x] 1.2 Declaration and function prototype
- [x] 1.3 Global function, Local function, function in single translate unit
- [x] 2 Inline keyword, inline function
- [x] 2.1 Compiler optimization use-case
- [x] 2.2 Different in size, speed analysis
- [x] 3 Phân biệt macro-like function và function
- [x] 4 Variable argument list
- [x] 5 Function argument, function return
- [x] 5.1 Passing argument as value
- [x] 5.2 Passing argument as reference
- [x] 5.3 example
- [x] Multiple input/multiple output function
- [x] Passing array as argument
- [x] return/passing pointer to diffrent kind of object
- [x] nested function
- [x] 6 Recursion
- [x] 6.1 introduction to stack
- [x] 6.2 how to create a recursion function

### OUT-B03-01 1 What is function

Nhóm này định nghĩa function qua signature, contract và linkage. “Local function” trong phần dưới được hiểu là function helper có internal linkage/file scope; ISO C17 không cho định nghĩa function bên trong block.

#### OUT-B03-02 1.1 Syntax

**Mapping:** `OUT-B03-02 / 1.1 Syntax` · LO `ADVC-H1SD` · Milestone `M00-FND-03`.

**Định nghĩa/ranh giới:** Function definition gồm return type, declarator, parameter list và compound statement; lời gọi tạo một lần thực thi function. `return` phải phù hợp return type. Trong declaration C17, `f()` không cung cấp prototype nêu rõ “không tham số”; dùng `f(void)`.

**Vai trò/quyết định:** Tách một phép biến đổi nhỏ khi nó có tên nghiệp vụ, contract và test riêng. Giữ một function làm một trách nhiệm thay vì chia vụn theo số dòng.

**Cơ chế:** `static inline int32_t clamp_upper(int32_t value, int32_t limit) { return value > limit ? limit : value; }` nhận hai giá trị và trả một giá trị, không sửa object bên ngoài.

**Dùng/không dùng/trade-off:** Dùng function để có type checking và một điểm implementation. Không tạo wrapper vô nghĩa chỉ gọi một function khác; abstraction tăng readability nhưng thêm boundary cần đặt tên/contract.

**Ví dụ + exact oracle:** `clamp_upper(11, 10)` trả chính xác `10`; `clamp_upper(3, 10)` trả `3`.

**Best practice:** **Rule:** viết parameter list và return contract cụ thể → **rationale:** compiler bắt mismatch sớm → **positive:** `int self_test(void)` → **negative:** `int self_test()` để ý nghĩa parameter không rõ trong C17.

**Lỗi:** dấu hiệu compile/call mismatch → nguyên nhân signature mơ hồ → chẩn đoán đọc declaration/definition/call cùng nhau → sửa dùng prototype đầy đủ → phòng tránh bật `-Wstrict-prototypes` khi policy cho phép.

#### OUT-B03-03 1.2 Declaration and function prototype

**Mapping:** `OUT-B03-03 / 1.2 Declaration and function prototype` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** Declaration giới thiệu identifier/type; prototype còn nêu type từng parameter để compiler kiểm và thực hiện conversion đúng. Definition đồng thời là declaration. Các declaration tương ứng phải compatible.

**Vai trò/quyết định:** Đặt prototype trước caller hoặc trong header khi chia translation unit; không dựa implicit declaration—C17 không cho phép nó.

**Cơ chế:** Asset định nghĩa helper trước nơi gọi, nên definition cung cấp prototype. API nhiều file nên có header được include bởi cả implementation và client.

**Dùng/không dùng/trade-off:** Header chung giảm drift nhưng tạo dependency cần quản lý include guard. Prototype riêng lẻ copy vào nhiều `.c` nhanh lúc đầu nhưng dễ bất tương thích.

**Ví dụ + exact oracle:** với `int32_t parsed=0`, `parse_i32("42", &parsed)` phải trả `true` và `parsed == 42`; `parse_i32("42x", &parsed)` phải trả `false`.

**Best practice:** **Rule:** declaration chỉ có một nguồn sự thật → **rationale:** compiler so definition với header → **positive:** implementation include chính header của nó → **negative:** client tự chép prototype rồi đổi `size_t` thành `int`.

**Lỗi:** dấu hiệu crash/giá trị return sai giữa module → nguyên nhân declaration không compatible → chẩn đoán strict-build toàn project và kiểm header → sửa thống nhất prototype → phòng tránh cấm hand-written duplicate declarations.

#### OUT-B03-04 1.3 Global function, Local function, function in single translate unit

**Mapping:** `OUT-B03-04 / 1.3 Global function, Local function, function in single translate unit` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** Function ở file scope mặc định có external linkage; thêm `static` cho internal linkage, chỉ dùng trong translation unit. ISO C17 chỉ cho function definition ở file scope; “local function” ở đây là file-local helper, không phải nested definition.

**Vai trò/quyết định:** Chỉ export entry point cần cho module; giữ parser/analyzer helper `static` để giảm surface và tránh collision.

**Cơ chế:** `parse_i32`, `find_range`, `visit_values` và `add_clamped` đều `static`; `main` có external linkage theo hosted program contract.

**Dùng/không dùng/trade-off:** `static` tốt cho implementation detail và tối ưu cục bộ; không dùng nếu function là API liên translation unit. Internal linkage giảm coupling nhưng test black-box thay vì gọi helper từ module khác.

**Ví dụ + exact oracle:** strict-build asset đơn lẻ exit `0`; mọi helper chỉ được gọi bên trong file và `./b03_functions_demo --self-test` in `B03 SELF-TEST PASS checks=9`.

**Best practice:** **Rule:** default helper về `static`, export có chủ đích qua header → **rationale:** interface nhỏ dễ review → **positive:** `static bool find_range(...)` → **negative:** để tất cả helper external và hy vọng tên không va chạm.

**Lỗi:** dấu hiệu duplicate symbol khi link → nguyên nhân cùng helper external ở nhiều file → chẩn đoán linker map/`nm` → sửa `static` hoặc thiết kế một API duy nhất → phòng tránh review danh sách exported symbols.

### OUT-B03-05 2 Inline keyword, inline function

`inline` thuộc semantics ngôn ngữ/linkage, đồng thời là gợi ý có thể hỗ trợ optimization; nó không phải lệnh bắt compiler thay call bằng body.

#### OUT-B03-06 2.1 Compiler optimization use-case

**Mapping:** `OUT-B03-06 / 2.1 Compiler optimization use-case` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** Inline expansion là optimization; keyword `inline` không bảo đảm expansion, và compiler có thể inline function không mang keyword. Trong C, các dạng `inline`/`extern inline` còn ảnh hưởng linkage; asset chọn `static inline` để tránh yêu cầu external definition.

**Vai trò/quyết định:** Chỉ cân nhắc cho helper nhỏ, gọi nóng sau khi profiler chỉ ra; correctness không được phụ thuộc việc compiler inline.

**Cơ chế:** `clamp_upper` là `static inline`; compiler tự quyết định tạo call hay thay body theo optimization, target và debug options.

**Dùng/không dùng/trade-off:** Có ích cho header/file-local helper nhỏ; không dùng như thuốc chữa performance hoặc cho body lớn. Có thể giảm call overhead nhưng tăng code size/I-cache pressure.

**Ví dụ + exact oracle:** build ở `-O0` và `-O2`, rồi chạy `--summarize 10 3 7 11`; cả hai binary phải cho cùng dòng đầu `count=3 sum=21 min=3 max=11 clipped_sum=20` và exit `0`.

**Best practice:** **Rule:** xem `inline` là design/linkage choice, đo optimization riêng → **rationale:** as-if rule cho compiler tự chọn → **positive:** `static inline` helper thuần nhỏ → **negative:** assert rằng keyword chắc chắn loại bỏ call.

**Lỗi:** dấu hiệu undefined reference sau tách header → nguyên nhân hiểu sai C inline linkage → chẩn đoán xem declaration ở mọi translation unit và symbol table → sửa dùng pattern `static inline` hoặc external definition có chủ đích → phòng tránh document linkage pattern.

#### OUT-B03-07 2.2 Different in size, speed analysis

**Mapping:** `OUT-B03-07 / 2.2 Different in size, speed analysis` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** Size là số byte/section của artifact; speed là metric trên workload/target xác định. Source code hay sự hiện diện của `inline` không đủ kết luận; debug/release, LTO, CPU và input làm kết quả thay đổi.

**Vai trò/quyết định:** Chốt correctness oracle trước, sau đó so cùng compiler/flags/target/dataset bằng `size`, map file và benchmark lặp; ghi uncertainty.

**Cơ chế:** tạo hai build chỉ khác optimization, xác minh output giống nhau, rồi đo `.text` và thời gian trên workload nội bộ `--benchmark`. Mode này chạy 10.000.000 iteration trong một process, biến đổi state unsigned xác định và in checksum để công việc không bị loại bỏ im lặng. Mỗi build warm-up một lần, đo năm lần bằng `date +%s%N` của GNU Coreutils, lấy median; nếu khác nhiều biến cùng lúc, không gán nguyên nhân cho inline.

**Dùng/không dùng/trade-off:** Đo khi size/latency là requirement; không microbenchmark CLI startup để kết luận helper nhanh hơn. Inlining có thể tăng speed và size, hoặc giúp tối ưu liên thủ tục làm cả hai tốt hơn.

**Ví dụ + exact oracle:** cả build `-O0` và `-O2` chạy `--benchmark 10000000` phải stdout đúng `BENCH iterations=10000000 checksum=-230967616032`, stderr rỗng, exit `0`. Evidence hợp lệ có compiler/OS fingerprint, đúng năm elapsed samples và median cho mỗi build; số giây/size không có ngưỡng portable và không bắt buộc `-O2` nhanh hơn.

**Best practice:** **Rule:** một comparison thay một biến và giữ oracle → **rationale:** tránh kết luận do nhiễu → **positive:** `-O2` vs `-O2 -fno-inline` cùng input → **negative:** so debug x86 với release ARM rồi quy mọi chênh lệch cho keyword.

**Lỗi:** dấu hiệu benchmark “nhanh” nhưng output thiếu → nguyên nhân optimizer loại công việc/fixture khác → chẩn đoán kiểm output và command line → sửa giữ observable result/dataset → phòng tránh lưu script, binary size và checksum fixture.

#### OUT-B03-08 3 Phân biệt macro-like function và function

**Mapping:** `OUT-B03-08 / 3 Phân biệt macro-like function và function` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** Function-like macro là token substitution ở preprocessing, không phải function call và không có type checking. Function đánh giá mỗi argument expression một lần theo call semantics; macro có thể lặp, bỏ hoặc thay đổi precedence nếu viết sai.

**Vai trò/quyết định:** Chọn function/`static inline` cho phép tính typed. Chỉ chọn macro khi thật sự cần token/stringification, conditional compilation hoặc generic facility được review kỹ.

**Cơ chế:** macro `#define DOUBLE(x) ((x) + (x))` thay `x` hai lần; function `double_i32(x)` nhận giá trị đã evaluate một lần.

**Dùng/không dùng/trade-off:** Macro không tạo call và làm việc trên token/type khác nhau nhưng diagnostic/debug khó, side-effect risk cao. Function rõ contract và symbol/debug tốt hơn.

**Ví dụ + exact oracle:** `double_i32(3)` phải trả `6`. Không chạy `DOUBLE(i++)`: nó sửa `i` nhiều lần không được sequencing, dẫn đến undefined behavior nên **không tồn tại oracle hợp lệ**.

**Best practice:** **Rule:** không truyền expression có side effect vào function-like macro và ưu tiên typed function → **rationale:** tránh multiple evaluation → **positive:** `clamp_upper(reading, limit)` → **negative:** `MAX(read_sensor(), threshold)` nếu macro có thể gọi sensor hai lần.

**Lỗi:** dấu hiệu counter tăng hai lần → nguyên nhân macro evaluate argument lặp → chẩn đoán xem preprocessed output `gcc -E` → sửa temporary + function → phòng tránh macro naming/review rule và tests side-effect-free.

#### OUT-B03-09 4 Variable argument list

**Mapping:** `OUT-B03-09 / 4 Variable argument list` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** Variadic function có ít nhất một named parameter rồi `...`; callee dùng `<stdarg.h>`, `va_list`, `va_start`, `va_arg`, `va_end`. Không có runtime type metadata; caller/callee phải chia sẻ count/format và type chính xác sau default argument promotions.

**Vai trò/quyết định:** Hợp lý cho logging/format family hoặc adapter hẹp. Fixed typed array thường tốt hơn cho dữ liệu nghiệp vụ vì length/type kiểm được.

**Cơ chế:** `checked_sum_varargs(size_t count, int64_t *out, ...)` đọc đúng `count` đối số với `va_arg(arguments, int)` và luôn gọi `va_end`.

**Dùng/không dùng/trade-off:** Dùng khi API intrinsically heterogeneous/format-driven; không dùng để né struct/array. Call gọn nhưng mismatch count/type là undefined behavior và khó phân tích tĩnh.

**Ví dụ + exact oracle:** `checked_sum_varargs(3U, &total, 3, 7, 11)` phải trả `true`, đặt `total == 21`; chính check này nằm trong self-test.

**Best practice:** **Rule:** có protocol explicit cho số lượng/type và gọi `va_end` trên mọi path sau `va_start` → **rationale:** callee không tự khám phá arguments → **positive:** count `3`, ba `int` → **negative:** count `4` nhưng chỉ truyền ba giá trị hoặc đọc `double` như `int`.

**Lỗi:** dấu hiệu total ngẫu nhiên/crash → nguyên nhân count/type lệch → chẩn đoán so call với từng `va_arg` → sửa protocol/typed container → phòng tránh wrapper typed và test zero/max count.

### OUT-B03-10 5 Function argument, function return

C luôn truyền argument theo giá trị. Muốn callee sửa object caller, giá trị được truyền là một pointer trỏ tới object đó; pointer vẫn được copy.

#### OUT-B03-11 5.1 Passing argument as value

**Mapping:** `OUT-B03-11 / 5.1 Passing argument as value` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** Parameter là object local khởi tạo từ argument value. Gán lại parameter scalar/pointer không gán lại biến argument của caller; object mà pointer trỏ tới vẫn có thể bị sửa nếu type cho phép.

**Vai trò/quyết định:** Truyền scalar nhỏ theo value khi function chỉ cần snapshot và không cần báo mutation.

**Cơ chế:** `clamp_upper(value, limit)` thao tác trên hai bản sao; caller giữ nguyên input.

**Dùng/không dùng/trade-off:** Tốt cho scalar/enum; aggregate lớn có thể truyền `const T *`. Value semantics dễ suy luận, đổi lại copy aggregate có thể tốn chi phí.

**Ví dụ + exact oracle:** với caller `int32_t x=11`, gọi `clamp_upper(x,10)` cho return `10` và sau call `x` vẫn chính xác `11`.

**Best practice:** **Rule:** mặc định input-only scalar theo value, đặt tên return rõ → **rationale:** giảm alias/mutation → **positive:** `clamp_upper(value, limit)` → **negative:** nhận `int32_t *` rồi sửa input chỉ để trả một scalar.

**Lỗi:** dấu hiệu caller không đổi sau `value=0` trong callee → nguyên nhân chỉ sửa bản sao parameter → chẩn đoán theo địa chỉ/object identity → sửa return value hoặc output pointer → phòng tránh document input/output direction.

#### OUT-B03-12 5.2 Passing argument as reference

**Mapping:** `OUT-B03-12 / 5.2 Passing argument as reference` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** C không có reference parameter như C++; cách gọi thường được mô tả “by reference” là truyền **giá trị pointer**. Callee dereference pointer để sửa object caller; lifetime, nullability và writable state phải hợp lệ.

**Vai trò/quyết định:** Dùng output pointer cho kết quả bổ sung hoặc object lớn; trả `bool`/status để phân biệt success với output value.

**Cơ chế:** `parse_i32(text, &value)` kiểm `out != NULL`, parse vào local rồi gán `*out` khi success.

**Dùng/không dùng/trade-off:** Dùng khi mutation/output là contract; không dùng pointer chỉ vì “C nhanh hơn”. Cho nhiều output nhưng thêm alias/null/lifetime risk.

**Ví dụ + exact oracle:** `parse_i32("-7", &value)` trả `true`, `value == -7`; `parse_i32("-7", NULL)` trả `false` và không dereference.

**Best practice:** **Rule:** null-check, nêu ownership và chỉ publish output sau validation → **rationale:** failure không để state nửa chừng → **positive:** local candidate rồi `*out=candidate` → **negative:** ghi từng phần output trước khi phát hiện token lỗi.

**Lỗi:** dấu hiệu segfault tại `*out` → nguyên nhân null/dangling/read-only pointer → chẩn đoán call site + lifetime → sửa contract/guard → phòng tránh annotations, negative null tests và không giữ borrowed pointer quá lifetime.

#### OUT-B03-13 5.3 example

**Mapping:** `OUT-B03-13 / 5.3 example` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** Một call site hoàn chỉnh phải chuẩn bị input, truyền đúng type/address, kiểm status rồi mới dùng output. Ví dụ không chỉ minh họa syntax mà chứng minh failure contract.

**Vai trò/quyết định:** Parser là boundary lý tưởng: text input-only, numeric output writable, status tách khỏi numeric domain.

**Cơ chế:** `int32_t limit; if (!parse_i32(argv[2], &limit)) { ... return 2; }` ngăn invalid value chảy vào pipeline.

**Dùng/không dùng/trade-off:** Pattern status + output áp dụng khi mọi giá trị của output type đều hợp lệ nên không có sentinel. Verbose hơn direct return nhưng không mất một giá trị để báo lỗi.

**Ví dụ + exact oracle:** `./b03_functions_demo --summarize bad 3` phải stdout rỗng, stderr đúng `error: limit is not an int32 value: bad`, exit `2`.

**Best practice:** **Rule:** luôn kiểm status trước đọc output → **rationale:** output có thể không được ghi khi failure → **positive:** branch ngay sau `parse_i32` → **negative:** in `limit` dù parser trả `false`.

**Lỗi:** dấu hiệu output rác ở invalid input → nguyên nhân caller bỏ qua status → chẩn đoán trace return value → sửa early return → phòng tránh compiler/static-analysis rule cho ignored-result wrapper.

#### OUT-B03-14 Multiple input/multiple output function

**Mapping:** `OUT-B03-14 / Multiple input/multiple output function` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** C function chỉ có một return value trực tiếp; nhiều output dùng struct return hoặc output pointers. Outputs phải có contract về null, alias và validity-on-failure. Contract của `find_range` yêu cầu `minimum` và `maximum` đều khác null **và trỏ tới hai object khác nhau**; alias `minimum == maximum` bị từ chối.

**Vai trò/quyết định:** `find_range` nhận array/count và tạo minimum/maximum. Status `bool` cho empty/invalid; hai pointer chứa outputs.

**Cơ chế:** sau toàn bộ guard, function seed hai candidate local bằng phần tử đầu, update candidates trong loop, rồi mới publish cả hai outputs. Nó không dùng sentinel `INT32_MAX/MIN` để đại diện empty; mọi failure, kể cả alias, để outputs không đổi.

**Dùng/không dùng/trade-off:** Struct return tốt khi outputs luôn đi cùng nhau; output pointers hữu ích với caller-owned storage/API hiện hữu. Pointer version linh hoạt nhưng nhiều null/alias cases hơn.

**Ví dụ + exact oracle:** `values={3,7,11}`, `find_range(values,3,&min,&max)` trả `true`, `min==3`, `max==11`. Với `int32_t same=1234`, `find_range(values,3,&same,&same)` phải trả `false` và `same` vẫn chính xác `1234`; count `0` cũng trả `false`.

**Best practice:** **Rule:** status, alias policy và validity-on-failure của từng output phải explicit → **rationale:** hai kết quả không thể cùng tồn tại trong một object và caller không phải suy đoán sentinel/state dở dang → **positive:** hai pointer distinct + candidates rồi publish → **negative:** nhận `&same,&same`, ghi minimum rồi ghi đè bằng maximum.

**Lỗi:** dấu hiệu chỉ còn maximum khi caller dùng một biến → nguyên nhân hai output pointer alias → chẩn đoán so địa chỉ `minimum == maximum` và kiểm state trước/sau failure → sửa reject trước mọi write → phòng tránh tests null/alias/empty/1/n và failure-atomic oracle.

#### OUT-B03-15 Passing array as argument

**Mapping:** `OUT-B03-15 / Passing array as argument` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** Trong function parameter, `T a[]` được điều chỉnh thành `T *a`; array length không được truyền tự động. `const T values[]` bảo vệ phần tử qua pointer đó nhưng không chứng minh lifetime/count.

**Vai trò/quyết định:** Mọi analyzer/visitor nhận `values` cùng `count`; caller chịu trách nhiệm vùng có ít nhất `count` phần tử sống.

**Cơ chế:** `find_range(const int32_t values[], size_t count, ...)` index `[0,count)`; `recursive_sum` thu nhỏ view bằng `values+1,count-1`.

**Dùng/không dùng/trade-off:** Pointer + length là API portable cho contiguous sequence. Không dùng `sizeof parameter` để đo length; slice explicit dễ reuse nhưng caller/callee phải duy trì invariant.

**Ví dụ + exact oracle:** với `{3,7,11}` và count `2`, range phải là `min=3,max=7`; giá trị `11` không được đọc.

**Best practice:** **Rule:** đặt pointer và length cạnh nhau, kiểm trước dereference → **rationale:** parameter decay mất bounds → **positive:** `(values,count)` → **negative:** loop tới `MAX_VALUES` dù caller chỉ có ba phần tử.

**Lỗi:** dấu hiệu ASan out-of-bounds → nguyên nhân count vượt object → chẩn đoán call site và loop guard → sửa truyền count thực/capacity → phòng tránh API naming + boundary test.

#### OUT-B03-16 return/passing pointer to diffrent kind of object

**Mapping:** `OUT-B03-16 / return/passing pointer to diffrent kind of object` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** Pointer tới object có thể chuyển sang/từ `void *` theo ISO C, nhưng callee phải biết actual type/alignment trước dereference. Không được trả pointer tới automatic local sau khi function kết thúc. Quy tắc object pointer không đồng nghĩa function pointer có thể portably đi qua `void *`.

**Vai trò/quyết định:** Callback generic nhận `void *context`; caller tạo `SumContext`, callback cast lại đúng type trong lifetime của call.

**Cơ chế:** `visit_values(..., add_clamped, &context)` copy object pointer vào `void *`; `add_clamped` dùng `SumContext *state = context` rồi cập nhật state.

**Dùng/không dùng/trade-off:** Context pointer phù hợp callback generic; API typed tốt hơn khi chỉ có một context type. Genericity đổi lấy compile-time type safety thấp hơn và cần lifetime discipline.

**Ví dụ + exact oracle:** với context `{limit=10,sum=0,accepted=0}` và `{3,7,11}`, visitor trả `true`, state cuối `sum=20,accepted=3`.

**Best practice:** **Rule:** owner giữ context sống suốt call; callback cast đúng documented type; không trả địa chỉ local → **rationale:** tránh dangling/misaligned access → **positive:** `&context` của caller dùng đồng bộ → **negative:** `return &local;` hoặc truyền `int *` rồi cast `SumContext *`.

**Lỗi:** dấu hiệu callback crash/field vô lý → nguyên nhân context sai type/lifetime → chẩn đoán kiểm call, cast, address/lifetime → sửa typed wrapper/context owner → phòng tránh một typedef callback + contract context thống nhất.

#### OUT-B03-17 nested function

**Mapping:** `OUT-B03-17 / nested function` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** Định nghĩa function bên trong function khác **không thuộc ISO C17**. GCC hỗ trợ nested functions như **GNU-only extension**; code có thể cần trampoline và mang caveat toolchain/security. Clang/strict compiler khác không bắt buộc hỗ trợ.

**Vai trò/quyết định:** Artifact portable không dùng nested function. State lexical được thay bằng file-scope `static` callback + explicit context pointer.

**Cơ chế:** `add_clamped` ở file scope nhận `(value, void *context)`; `SumContext` mang `limit/sum/accepted`; `visit_values` gọi callback. Closure trở thành dữ liệu explicit.

**Dùng/không dùng/trade-off:** Chỉ dùng GNU nested function khi project chủ ý khóa GCC GNU dialect và đã review executable-stack/trampoline/lifetime; không dùng trong ISO C17 portable deliverable. Callback+context verbose hơn nhưng portable, testable và lifetime thấy được.

**Ví dụ + exact oracle:** `visit_values({3,7,11},3,add_clamped,&context)` cho `sum=20,accepted=3`; `visit_values(...,NULL,&context)` trả `false`. Strict C17 build phải exit `0` mà không có nested definition.

**Best practice:** **Rule:** portable baseline dùng file-scope callback + context → **rationale:** ISO C17 không có closure/nested definition → **positive:** `VisitFn` + `void *context` → **negative:** đặt `bool visit(...)` bên trong `run_summary` và compile bằng mặc định GNU.

**Lỗi:** dấu hiệu `ISO C forbids nested functions` dưới `-Wpedantic` hoặc binary đòi executable stack → nguyên nhân GNU nested function/trampoline → chẩn đoán strict build và linker warning → sửa lift callback + context → phòng tránh CI `-std=c17 -Wpedantic -Werror`.

### OUT-B03-18 6 Recursion

Recursion là một function trực tiếp/gián tiếp gọi lại chính nó. Correctness cần base case và bước tiến; production còn cần resource bound.

#### OUT-B03-19 6.1 introduction to stack

**Mapping:** `OUT-B03-19 / 6.1 introduction to stack` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** Mỗi active invocation có parameters/automatic objects/control state riêng; implementation thường dùng call stack, nhưng ISO C không quy định cấu trúc vật lý hay dung lượng. Vì vậy không tuyên bố số byte/depth portable nếu chưa đo target.

**Vai trò/quyết định:** Dùng mental model frame để trace recursion; áp application bound trước khi gọi để ngăn depth không kiểm soát.

**Cơ chế:** `recursive_sum_impl` tăng `depth`, giảm `count`; `MAX_RECURSION_DEPTH=16`. Base frame count `0`, rồi unwinding cộng ngược.

**Dùng/không dùng/trade-off:** Mental stack hữu ích debug. Với hard real-time/stack nhỏ, iteration thường dễ budget hơn; recursion diễn đạt tree/divide-and-conquer tự nhiên nhưng resource proof khó hơn.

**Ví dụ + exact oracle:** `recursive_sum(values,17,&total)` trả chính xác `false` trước khi xử lý vì vượt bound `16`; self-test tính đây là một check pass.

**Best practice:** **Rule:** có depth/input bound được test và đo stack trên target → **rationale:** standard không bảo đảm tài nguyên → **positive:** reject count `17` → **negative:** recurse theo input user không giới hạn.

**Lỗi:** dấu hiệu stack overflow/reset → nguyên nhân depth không bounded hoặc frame lớn → chẩn đoán input/depth counter + target stack watermark → sửa iteration/bound/giảm locals → phòng tránh resource analysis và max-depth test.

#### OUT-B03-20 6.2 how to create a recursion function

**Mapping:** `OUT-B03-20 / 6.2 how to create a recursion function` · LO `ADVC-H1SD` · `M00-FND-03`.

**Định nghĩa/ranh giới:** Recursion đúng cần base case reachable, recursive step tiến về base, kết hợp kết quả và failure propagation. Tail-call optimization không được ISO C đảm bảo.

**Vai trò/quyết định:** Asset dùng recursive sum để học mechanism; loop là lựa chọn production đơn giản hơn cho array tuyến tính.

**Cơ chế:** count `0` đặt `*out=0`; count `n` gọi trên `values+1,n-1`, rồi `*out=values[0]+tail`. Failure từ lời gọi sâu được truyền lên trước khi publish output.

**Dùng/không dùng/trade-off:** Dùng khi cấu trúc bài toán recursive và depth bounded; không dùng chỉ để thay loop tuyến tính. Code có thể gần định nghĩa toán hơn, đổi lại call/resource overhead.

**Ví dụ + exact oracle:** `recursive_sum(values,3,&total)` với `values={3,7,11}` trả `true`, `total==21`; gọi lại với chính pointer hợp lệ đó và count `0` trả `true`, `total==0`.

**Best practice:** **Rule:** viết/test base case trước, chứng minh measure giảm mỗi call → **rationale:** bảo đảm termination → **positive:** `count-1` sau guard `count==0` → **negative:** gọi lại cùng `count` và chờ runtime dừng.

**Lỗi:** dấu hiệu hang/overflow → nguyên nhân base case không tới hoặc unsigned underflow → chẩn đoán trace `count/depth` → sửa guard trước `count-1` → phòng tránh proof variant + tests 0/1/max/max+1.

## 4. Ví dụ tích hợp của Unit

`assets/b03_functions_demo.c` kết nối toàn bộ contract qua fixture:

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror \
  assets/b03_functions_demo.c -o b03_functions_demo
./b03_functions_demo --summarize 10 3 7 11
```

Input: limit `10`, array `{3,7,11}`. Parser tạo scalar; `find_range` xuất min/max; callback+context tính clipped sum; recursion tính total. Exact oracle:

```text
count=3 sum=21 min=3 max=11 clipped_sum=20
recursive_sum=21
```

stderr rỗng, exit `0`. `./b03_functions_demo --self-test` phải in `B03 SELF-TEST PASS checks=9`.

## 5. Những cách làm chưa hiệu quả và cải tiến

| Cách làm | Vì sao yếu | Cải tiến có contract |
|---|---|---|
| Export mọi helper | Link surface lớn, dễ collision/coupling | File-local helper `static`, API qua header |
| Dùng `inline` như performance guarantee | Compiler không buộc inline; size có thể tăng | Chốt correctness, profile/measure cùng flags |
| Variadic API không count/format | Callee không biết số lượng/type | Protocol explicit hoặc typed array/struct |
| Gọi array parameter rồi dùng `sizeof` lấy length | Parameter đã thành pointer | Truyền `values,count` |
| Trả pointer tới local | Lifetime kết thúc khi return | Caller-owned output, allocated/static object theo contract |
| GNU nested function trong C17 | Extension, portability/trampoline risk | File-scope callback + explicit context |
| Recursion không bound | Không có portable stack guarantee | Input/depth bound hoặc iteration |

## 6. Lỗi thường gặp theo chuỗi chẩn đoán

- **Triệu chứng:** link lỗi/mismatch ABI → **nguyên nhân:** prototype giữa declaration và definition lệch → **chẩn đoán:** strict-build và so header → **khắc phục:** một declaration nguồn sự thật → **phòng tránh:** implementation include header.
- **Triệu chứng:** output pointer crash → **nguyên nhân:** null/dangling/sai type → **chẩn đoán:** inspect lifetime và actual object → **khắc phục:** guard + typed contract → **phòng tránh:** negative tests/nullability note.
- **Triệu chứng:** min bị ghi đè bằng max → **nguyên nhân:** hai output pointers alias → **chẩn đoán:** so địa chỉ và state trước/sau failure → **khắc phục:** reject alias trước mọi write → **phòng tránh:** self-test failure-atomic với sentinel `1234`.
- **Triệu chứng:** macro gọi I/O hai lần → **nguyên nhân:** multiple evaluation → **chẩn đoán:** `gcc -E` → **khắc phục:** typed function/temporary → **phòng tránh:** side-effect-free macro arguments.
- **Triệu chứng:** variadic sum ngẫu nhiên → **nguyên nhân:** count/type mismatch sau promotions → **chẩn đoán:** đối chiếu call với `va_arg` → **khắc phục:** đúng protocol/typed array → **phòng tránh:** wrapper và boundary tests.
- **Triệu chứng:** strict C17 báo nested function hoặc executable-stack warning → **nguyên nhân:** GNU-only nested definition/trampoline → **chẩn đoán:** `-std=c17 -Wpedantic` + linker log → **khắc phục:** callback+context → **phòng tránh:** portability gate CI.
- **Triệu chứng:** reset/stack overflow với input lớn → **nguyên nhân:** recursion không bound → **chẩn đoán:** depth counter/stack watermark → **khắc phục:** bound hoặc loop → **phòng tránh:** resource budget trên target.

### Thuật ngữ nhanh

- **Function contract:** precondition, effect, output, failure và resource boundary của function.
- **Prototype:** declaration nêu type parameters, cho compiler kiểm call.
- **Translation unit:** source sau preprocessing; ranh giới của internal linkage.
- **Internal linkage:** identifier `static` file scope chỉ liên kết trong translation unit.
- **Inline expansion:** optimization thay call bằng body; không đồng nghĩa keyword chắc chắn được thực hiện.
- **Variadic function:** function nhận số argument thay đổi qua `...` và `<stdarg.h>`.
- **Default argument promotions:** promotions áp cho unnamed variadic arguments; callee phải đọc đúng promoted type.
- **Output pointer:** pointer tới caller-owned object mà callee được phép ghi theo contract.
- **Callback/context:** function pointer cùng object pointer mang state explicit.
- **Nested function:** function definition nằm trong function khác; GNU extension, không phải ISO C17.
- **Recursion depth:** số invocation active trên chuỗi gọi; cần bound/resource proof.

## 7. Nguồn tham khảo và provenance phần bổ sung

- `SRC-C17-ISO` và `SRC-C17-WG14`: function declarator/definition/call, linkage, pointer, array parameter adjustment, `inline`, `<stdarg.h>` và recursion semantics.
- `SRC-GCC11`, *GCC 11.4 manuals*, version 11.4.0, *An Inline Function is As Fast As a Macro*: keyword/optimization và GNU inline notes; asset dùng pattern portable `static inline`.
- `SRC-GCC11`, *GCC 11.4 manuals*, version 11.4.0, *Nested Functions*: nested definition là GNU extension; asset ISO C17 không dùng extension đó.
- `SRC-GLIBC235`, *GNU C Library Reference Manual*, glibc 2.35, Appendix A.2 *How Variadic Functions are Defined and Used*: workflow `va_list`/`va_start`/`va_arg`/`va_end`.
- `[BỔ SUNG — nguồn: các tài liệu trên]` Portability boundary, default promotions, internal linkage và callback-context rules.
- `[SUY DIỄN — từ case simulated]` CLI schema, capacity `16`, recursion bound `16`, benchmark capacity/workload/checksum, exact messages/fixtures và practice oracles là thiết kế sư phạm của `CASE-B03-01`, không phải API external.

## 8. Quiz tự kiểm tra

### Câu 1

Trong ISO C17, `int f()` có phải prototype bảo đảm function không nhận tham số không?

**Đáp án:** Không. `int f(void)` mới nêu rõ không có parameter. `f()` trong C17 để parameter information không được chỉ định, làm giảm kiểm tra call.

### Câu 2

Thêm `inline` có bảo đảm binary nhỏ hơn và function call biến mất không?

**Đáp án:** Không. Compiler tự quyết định expansion; inline có thể tăng/giảm size hoặc không xảy ra. So cùng target/flags/workload và giữ correctness oracle.

### Câu 3

Vì sao `find_range(values, count, &min, &max)` vẫn là pass-by-value?

**Đáp án:** C copy giá trị của các pointers vào parameters. Callee sửa objects `min/max` bằng dereference; nó không nhận C++ reference.

### Câu 4

Nested function viết trong GCC có portable dưới `-std=c17` không? Phương án thay thế nào dùng trong asset?

**Đáp án:** Không; đó là GNU-only extension. Asset dùng file-scope callback `add_clamped` và truyền `SumContext` qua `void *context`.

### Câu 5

Một recursive function có base case rồi đã đủ an toàn cho production chưa?

**Đáp án:** Chưa. Còn phải chứng minh mỗi call tiến về base, giới hạn depth/input, propagate failure và đo stack trên target; ISO C không bảo đảm tail-call optimization hay dung lượng stack.
