# B09 — Common C Defects — Học liệu cốt lõi

> **Khóa học:** [NFP] Advanced C Programming · **Thời lượng:** 120 phút · **LO:** `ADVC-H3SD`  
> **Ranh giới:** mã minh họa là ISO C17 portable chạy local. Sanitizer, Valgrind và warning flags là bằng chứng theo toolchain, không thuộc semantics C. Không MCU, HAL, RTOS, MMIO, ISR hoặc phần cứng.

## 🎯 Learning Outcomes

- Nhận diện defect trước khi nó thành memory corruption hoặc sai dữ liệu; chuyển mỗi nhận định thành oracle tái lập.
- **Increment `M00-FND-09`:** [b09_defects_demo.c](assets/b09_defects_demo.c) dùng checked paths cho parsing, byte decoding, macro, arithmetic, const, string và cleanup.

## 1. Kiến thức tiên quyết và mental map

- Hoàn thành B08; hiểu object, array, pointer, integer conversion, dynamic allocation và exit code.
- Baseline: GCC 11.4 hoặc Clang 14; strict flags `-std=c17 -Wall -Wextra -Wpedantic -Werror`.
- Mental map: `external bytes/text → validate representation and bounds → convert in a stated domain → establish ownership/lifetime → perform operation → verify value + exit/stdout/stderr → cleanup on every path`.
- Warning/sanitizer sạch không chứng minh không còn defect; chúng bổ sung review contract và boundary tests.

## 3. Nội dung lý thuyết cốt lõi

- [x] Alignment and packing
- [x] Macro, function like macro
- [x] Overflow and underflow
- [x] pointer to const and const pointer
- [x] signed and usigned type
- [x] string and characters
- [x] Precedence
- [x] Loosing dynamic allocated memory

#### OUT-B09-01 — Alignment and packing

**Mapping.** `OUT-B09-01` → `ADVC-H3SD` → `M00-FND-09`: giải mã một word little-endian từ byte buffer mà không giả định layout/alignment của struct.

**Định nghĩa/ranh giới.** Alignment là yêu cầu địa chỉ hợp lệ cho một type; padding là khoảng compiler có thể chèn trong/between members; packing là extension/tool directive làm thay đổi layout. C17 không bảo đảm một struct in-memory trùng wire/file format, cũng không bảo đảm cast địa chỉ byte bất kỳ sang `uint16_t *` được căn chỉnh hay tránh strict-aliasing UB.

**Vai trò/quyết định.** Người thiết kế parser phải quyết định byte order và chuyển từng field rõ ràng. Chỉ dùng packed struct khi ABI/protocol, compiler và target đã chốt, có static assertions và test layout; portable file parser không cần packed struct.

**Cơ chế.** `decode_u16_le` đọc hai `unsigned char`, promote/cast sang `uint16_t`, shift byte cao rồi OR. Mọi object representation có thể được quan sát qua character type; chuyển byte rõ ràng không dereference pointer lệch alignment.

**Khi dùng/không dùng/trade-off.** Dùng byte-wise decode cho file/network portable; dùng layout-native khi chỉ serializing trong cùng ABI và contract cho phép. Byte decode dài hơn nhưng rõ endian/alignment; packed access có thể chậm, fault hoặc lệ thuộc compiler.

**Ví dụ/oracle.** Bytes `{0x34,0x12}` phải tạo decimal `4660`; asset với `4 sensor-A` in `word=4660`. Oracle: stdout exact `OK count=4 sum=100 label=sensor-A word=4660 mask=3`, exit `0`.

**Best practice.** **Rule:** parse external representation field-by-field. **Rationale:** tách format khỏi padding/endian/alignment ABI. **Positive:** `bytes[0] | bytes[1] << 8`. **Negative:** `*(const uint16_t *)(buffer + 1)`, có thể lệch alignment, alias sai và đảo endian.

**Failure/troubleshooting.** Chỉ crash trên một CPU → unaligned/ABI assumption → bật alignment sanitizer, xem address/layout và fixture bytes → thay cast bằng decode/memcpy có endian conversion → phòng tránh bằng format spec, golden vectors và không serialize raw struct.

#### OUT-B09-02 — Macro, function like macro

**Mapping.** `OUT-B09-02` → `ADVC-H3SD` → `M00-FND-09`: macro `ARRAY_COUNT` chỉ áp dụng cho array thật trong cùng scope.

**Định nghĩa/ranh giới.** Macro là token substitution trước type checking; function-like macro trông như call nhưng argument có thể được mở rộng nhiều lần. `ARRAY_COUNT(a)` tính `sizeof(a)/sizeof(a[0])` đúng cho array object, sai khi `a` đã decay thành pointer.

**Vai trò/quyết định.** Dùng macro cho compile-time expression nhỏ khi C17 không có generic function phù hợp; dùng function `static inline` khi cần single evaluation, type checking hoặc debug call site.

**Cơ chế.** `SQUARE(i++)` kiểu `((i++)*(i++))` đánh giá argument hai lần và có thể tạo unsequenced modification; `MAX(a,b)` cũng có side effects lặp. Parentheses giảm precedence bug nhưng không giải quyết multi-evaluation.

**Khi dùng/không dùng/trade-off.** Macro constants/count trong scope phù hợp có zero runtime cost; không đưa expression có side effect vào macro không bảo đảm single evaluation. Inline function có rules linkage nhưng an toàn type/evaluation hơn.

**Ví dụ/oracle.** `static const unsigned char encoded_word[]` có `ARRAY_COUNT(encoded_word)==2`; contract fail nếu khác. Oracle happy giữ `word=4660`; không dùng count macro sau khi truyền array vào function.

**Best practice.** **Rule:** parenthesize parameters/result và document evaluation; ưu tiên inline function cho behavior. **Rationale:** preprocessor không hiểu type/side effect. **Positive:** `#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))` dùng ngay với local array. **Negative:** `#define TWICE(x) ((x)+(x))` rồi `TWICE(i++)`.

**Failure/troubleshooting.** Giá trị tăng hai lần/chỉ lỗi release → macro multi-evaluation hoặc precedence → xem preprocessed output `gcc -E`, warning và call site → thay macro bằng function/temp → phòng tránh bằng cấm side-effect arguments và test compile-time use boundary.

#### OUT-B09-03 — Overflow and underflow

**Mapping.** `OUT-B09-03` → `ADVC-H3SD` → `M00-FND-09`: cộng `uint32_t` có precondition được kiểm trước khi ghi output.

**Định nghĩa/ranh giới.** Unsigned arithmetic wrap modulo `2^N`; signed overflow là undefined behavior. “Wrap được định nghĩa” không đồng nghĩa đúng nghiệp vụ: count, size và money thường phải reject overflow. Underflow unsigned cũng wrap về giá trị lớn.

**Vai trò/quyết định.** API owner phải chọn reject, saturate hay modulo và ghi thành contract. Parser/allocation phải reject trước phép nhân/cộng nếu wrap có thể làm cấp phát thiếu.

**Cơ chế.** Với addition, kiểm `UINT32_MAX - left < right` trước `left + right`. Với allocation, kiểm `count > SIZE_MAX / sizeof(*values)` trước multiplication. Không tính expression nguy hiểm rồi mới so.

**Khi dùng/không dùng/trade-off.** Modulo phù hợp checksum/hash đã công bố; checked arithmetic phù hợp sizes/totals. Check thêm branch nhưng ngăn memory corruption và ambiguity.

**Ví dụ/oracle.** Asset tạo `10,20,30,40`; checked sum cho `100`. Executable path `--overflow-test` gọi chính `checked_add_u32(UINT32_MAX,1,&sentinel)`: phải exit `0`, stderr rỗng và in chính xác `OK overflow-rejected left=4294967295 right=1 output-unchanged=123`. Oracle đồng thời chứng minh helper trả failure **trước** phép cộng và không sửa output khi reject.

**Best practice.** **Rule:** validate trước operation trong đúng integer domain. **Rationale:** sau signed UB không còn recovery đáng tin; sau unsigned wrap có thể mất evidence. **Positive:** `UINT32_MAX-left < right`. **Negative:** `sum=left+right; if(sum<left)` được dùng bừa cho signed type.

**Failure/troubleshooting.** Allocation nhỏ bất thường hoặc total quay về 0 → integer wrap/conversion → UBSan, boundary fixtures, log operands/types → dùng checked helper/size guard → phòng tránh bằng domain types, maximums và tests ở max±1.

#### OUT-B09-04 — pointer to const and const pointer

**Mapping.** `OUT-B09-04` → `ADVC-H3SD` → `M00-FND-09`: `const uint32_t * const read_only_view` khóa cả sửa pointee qua view và đổi view.

**Định nghĩa/ranh giới.** `const T *p` là pointer tới object chỉ đọc qua `p`; `T *const p` là pointer không thể reseat; `const T *const p` có cả hai. `const` không tự làm object immutable nếu alias non-const hợp lệ khác còn tồn tại, cũng không bảo đảm thread safety.

**Vai trò/quyết định.** Signature dùng pointee-const để công bố function không sửa input qua pointer đó; const pointer local giúp giữ invariant ownership/location. Không cast away const để gọi API viết.

**Cơ chế.** Qualifier áp vào declarator gần nó: đọc từ tên ra ngoài. Compiler chặn assignment vi phạm tại compile time; cast có thể che warning nhưng write vào object vốn const là UB.

**Khi dùng/không dùng/trade-off.** Dùng const cho read-only views/config; không dùng nó thay synchronization hoặc ownership model. Const-correct API có thể cần chỉnh nhiều signatures nhưng giảm mutation surface.

**Ví dụ/oracle.** Loop sum chỉ đọc `values` qua `read_only_view`; input `4` cho `sum=100` và pointer được giải phóng bằng owner `values` sau use.

**Best practice.** **Rule:** thêm const ở API boundary sớm và giữ owner riêng. **Rationale:** compiler thực thi một phần mutation contract. **Positive:** `size_t f(const uint8_t *data)`. **Negative:** cast `(uint8_t *)data` rồi ghi vì signature bất tiện.

**Failure/troubleshooting.** Compiler báo discarded qualifier hoặc dữ liệu “read-only” đổi → signature/alias contract sai → tìm mọi alias/write, không chỉ thêm cast → sửa API/ownership → phòng tránh bằng const-correct headers và code review declarators.

#### OUT-B09-05 — signed and usigned type

**Mapping.** `OUT-B09-05` → `ADVC-H3SD` → `M00-FND-09`: parse external count thành miền có giới hạn rồi convert có kiểm soát sang `size_t`.

**Định nghĩa/ranh giới.** Khi signed và unsigned trộn, usual arithmetic conversions có thể đổi số âm thành unsigned rất lớn. `size_t` là unsigned type cho kích thước; spelling trong outline được giữ nguyên nhưng thuật ngữ đúng là **unsigned**.

**Vai trò/quyết định.** Chọn type theo miền dữ liệu và API; tại boundary, parse rộng, kiểm end pointer/range rồi cast. Không chỉ đổi toàn bộ loop indices sang signed hay unsigned mà thiếu contract.

**Cơ chế.** So `int index=-1` với `size_t count=8` có thể convert `-1` thành giá trị unsigned lớn. `strtoul` cần kiểm `errno`, có digit, consumed-all và upper/lower bounds; dấu âm phải bị policy reject qua range/lexical checks phù hợp.

**Khi dùng/không dùng/trade-off.** `size_t` cho array count/index; fixed-width types cho wire data; signed cho miền thật sự chứa số âm. Conversion explicit tăng verbosity nhưng lộ boundary decision.

**Ví dụ/oracle.** Input `9 sensor-A` vượt `MAX_VALUES=8`, phải exit `2`, stdout rỗng, stderr exact `ERROR count must be 1..8`; không được wrap/truncate rồi cấp phát.

**Best practice.** **Rule:** tránh mixed-sign comparisons và kiểm trước cast. **Rationale:** conversion diễn ra trước comparison. **Positive:** parse, range-check `1..8`, sau đó `(size_t)parsed`. **Negative:** `if (atoi(s) < sizeof(array))` nhận `-1` ngoài ý muốn.

**Failure/troubleshooting.** Negative input đi qua check hoặc warning sign-compare → implicit conversion → bật `-Wsign-conversion` trong audit build, inspect types/value → chuẩn hóa domain/check trước cast → phòng tránh bằng typed boundaries và max fixtures.

#### OUT-B09-06 — string and characters

**Mapping.** `OUT-B09-06` → `ADVC-H3SD` → `M00-FND-09`: label C string có capacity 16, policy ký tự và NUL terminator được xác minh trước copy.

**Định nghĩa/ranh giới.** C string là sequence `char` kết thúc bởi `\0` trong storage đủ lớn; character classification functions yêu cầu argument là `EOF` hoặc giá trị biểu diễn được bởi `unsigned char`. Byte string không mặc định là UTF-8 text hợp lệ.

**Vai trò/quyết định.** Parser owner định nghĩa max length, alphabet và encoding. API nhận buffer phải biết capacity; binary data dùng pointer+length thay vì `strlen`.

**Cơ chế.** `strlen` chỉ dùng sau khi biết source là C string do `argv` cung cấp; reject length `0` hoặc `>=16`, cast từng byte sang `unsigned char` trước `isalnum`, rồi copy `length+1` để gồm NUL.

**Khi dùng/không dùng/trade-off.** C string phù hợp CLI text nhỏ; pointer+length phù hợp untrusted/binary buffers. Bounded copy có policy rõ nhưng truncation im lặng không được dùng nếu label là identifier.

**Ví dụ/oracle.** `sensor-A` hợp lệ. `bad_label` chứa `_`, phải exit `2`, stdout rỗng và stderr exact `ERROR label must be 1..15 alnum-or-dash characters`.

**Best practice.** **Rule:** validate capacity và termination trước use; cast `unsigned char` cho `<ctype.h>`. **Rationale:** over-read và negative-char argument có thể là UB. **Positive:** reject oversize, copy đủ `length+1`. **Negative:** `strcpy(dst, argv[2])` hoặc `isalnum((char)0xFF)`.

**Failure/troubleshooting.** Log rác/ASan over-read → thiếu NUL hoặc vượt capacity → inspect length/capacity, ASan fixture max±1 → reject/copy bounded → phòng tránh bằng length-carrying APIs và exact boundary tests.

#### OUT-B09-07 — Precedence

**Mapping.** `OUT-B09-07` → `ADVC-H3SD` → `M00-FND-09`: bit-mask check được parenthesize để intent không phụ thuộc người đọc nhớ bảng precedence.

**Định nghĩa/ranh giới.** Precedence quyết định grouping cú pháp, associativity xử lý operators cùng mức; cả hai không tự quy định mọi thứ tự evaluation. `flags & MASK == 0` được parse khác `(flags & MASK) == 0` vì equality bind chặt hơn bitwise AND.

**Vai trò/quyết định.** Reviewer cần nhìn thấy grouping domain, nhất là bitwise, comparison, assignment và macro. Parentheses làm intent auditable nhưng không sửa unsequenced side effects.

**Cơ chế.** Asset dùng `(flags & UINT32_C(0x01)) == 0U`, với `flags=3`; mask low bit có giá trị 1 nên contract pass và output `mask=3`.

**Khi dùng/không dùng/trade-off.** Parenthesize khi trộn operator families; không thêm parentheses vô nghĩa che expression quá phức tạp—tách biến named intermediate tốt hơn.

**Ví dụ/oracle.** Với `flags=3`, đúng check không đi error path; stdout happy kết thúc `mask=3`. Biểu thức sai `flags & 1U == 0U` thực chất kiểm `flags & 0U`, luôn false.

**Best practice.** **Rule:** parenthesize operands của bitwise comparison và tách side effects. **Rationale:** giảm review ambiguity và macro expansion surprise. **Positive:** `(flags & MASK) != 0U`. **Negative:** `a & b == 0` hoặc `array[i++] = i`.

**Failure/troubleshooting.** Branch không bao giờ chạy dù mask đổi → grouping sai → compiler warning, AST/preprocessed output, bảng truth fixtures → thêm grouping/tách intermediate → phòng tránh bằng strict warnings và code-review rule.

#### OUT-B09-08 — Loosing dynamic allocated memory

**Mapping.** `OUT-B09-08` → `ADVC-H3SD` → `M00-FND-09`: một owner `values`, một cleanup label, `free` trên mọi path sau allocation. Spelling plan được giữ; thuật ngữ đúng là **losing/leaking dynamically allocated memory**.

**Định nghĩa/ranh giới.** Leak xảy ra khi allocated object còn sống nhưng không còn reachable owner để `free`; dangling pointer xảy ra sau `free` mà alias vẫn được dùng. `free(NULL)` hợp lệ; đặt local owner về `NULL` không tự vô hiệu hóa alias khác.

**Vai trò/quyết định.** Mỗi allocation cần owner, transfer rule và cleanup responsibility. Với resource lifetime ngắn, một cleanup block thường dễ audit hơn nhiều early returns.

**Cơ chế.** Asset kiểm size, `malloc`, xử lý; mọi internal failure sau đó `goto cleanup`, gọi `free(values)`, gán `NULL`, trả status. Parse failures trước allocation có thể return trực tiếp.

**Khi dùng/không dùng/trade-off.** Heap dùng khi size/runtime lifetime cần; local fixed array phù hợp maximum nhỏ và stack budget cho phép. Heap hỗ trợ dynamic size nhưng thêm failure path, fragmentation và ownership burden.

**Ví dụ/oracle.** `valgrind --leak-check=full --errors-for-leak-kinds=all --error-exitcode=99 ./b09_demo 4 sensor-A` phải exit `0` và báo `ERROR SUMMARY: 0 errors`; negative input trước allocation exit `2`.

**Best practice.** **Rule:** chỉ định owner và viết cleanup đồng thời với allocation. **Rationale:** từng return mới là một leak opportunity. **Positive:** initialize NULL, one cleanup, `free` once. **Negative:** overwrite `values=malloc(...)` lần hai hoặc return giữa path mà không free.

**Failure/troubleshooting.** RSS tăng/Valgrind “definitely lost” → ownership path mất `free` → chạy fixture qua Memcheck, trace allocations/returns → gom cleanup và sửa transfer → phòng tránh bằng failure-injection tests, sanitizer/Valgrind gate và API ownership docs.

## 3. Ví dụ tích hợp

Asset nối cả tám defect class thành một boundary pipeline: parse `count`, kiểm label, kiểm allocation size, allocate, tạo values, đọc bằng const view, checked-sum, decode byte word, kiểm parenthesized mask và cleanup. Hai oracle tối thiểu:

```text
$ ./b09_demo 4 sensor-A
OK count=4 sum=100 label=sensor-A word=4660 mask=3
```

```text
$ ./b09_demo 9 sensor-A
ERROR count must be 1..8
# exit 2; stdout rỗng
```

Overflow rejection là executable contract riêng, không phải đoạn pseudo-code:

```text
$ ./b09_demo --overflow-test
OK overflow-rejected left=4294967295 right=1 output-unchanged=123
# exit 0; stderr rỗng
```

Material này dùng safe checked paths để học chẩn đoán; nó không cố tình chạy UB. Các anti-pattern được phân tích, không được đưa vào executable production-style.

## 4. Quiz tự kiểm tra (5 câu)

1. **Có nên cast `buffer+1` sang `uint32_t *` để đọc file nhanh hơn?**  
   **Đáp án:** Không trong parser portable. Địa chỉ có thể lệch alignment, representation có endian khác và aliasing có thể sai; decode bytes hoặc `memcpy` rồi đổi endian theo format.

2. **Vì sao `#define DOUBLE(x) ((x)+(x))` vẫn nguy hiểm dù đủ ngoặc?**  
   **Đáp án:** Argument được đánh giá hai lần. `DOUBLE(i++)` có side effects lặp/unsequenced; ngoặc chỉ sửa grouping, không tạo single evaluation.

3. **Unsigned overflow đã defined thì luôn chấp nhận được?**  
   **Đáp án:** Không. Modulo phù hợp một số checksum, nhưng size/count/total thường phải reject vì wrap có thể cấp phát thiếu hoặc làm sai nghiệp vụ.

4. **`const uint8_t *p` có nghĩa object không ai sửa được?**  
   **Đáp án:** Không. Nó chỉ cấm sửa object qua `p`; alias non-const hợp lệ khác có thể sửa. Const cũng không thay thế synchronization.

5. **Đặt `p=NULL` sau `free(p)` có chấm dứt mọi use-after-free?**  
   **Đáp án:** Không. Chỉ local `p` đổi; alias khác vẫn dangling. Cần ownership/borrow lifetime rõ và không dùng alias sau owner cleanup.

## 5. Từ điển thuật ngữ

- **Alignment:** bội địa chỉ mà implementation yêu cầu cho một type.
- **Padding/packing:** byte chèn bởi layout / cơ chế giảm padding, thường tool/ABI-specific.
- **Integer promotion/usual arithmetic conversions:** rules đưa operands về type chung trước operation.
- **C string:** dãy `char` có NUL terminator trong storage hợp lệ.
- **Precedence/associativity:** grouping cú pháp; không đồng nghĩa thứ tự evaluation tổng quát.
- **Owner/borrow:** quy ước ai giải phóng object / ai chỉ dùng trong lifetime cho phép.
- **Leak/dangling pointer:** allocation mất owner / pointer còn trỏ tới lifetime đã kết thúc.
- **Oracle:** kết quả pass/fail chính xác gồm value, exit code và output channels.

## 7. Nguồn tham khảo và provenance phần bổ sung

- `SRC-USER-CREF` — outline Part 0 đã được duyệt.
- `SRC-C17-ISO` — ISO/IEC 9899:2018, metadata: <https://www.iso.org/standard/74528.html>.
- `SRC-C17-WG14` — public committee draft N2176: <https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf>.
- `SRC-CERTC` — SEI CERT C Coding Standard: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/>.
- `SRC-GCC11` — GCC 11.4 manuals: <https://gcc.gnu.org/onlinedocs/gcc-11.4.0/>.
- `SRC-LLVM14` — Clang 14 documentation: <https://releases.llvm.org/14.0.0/tools/clang/docs/index.html>.
- `SRC-VALGRIND318` — Memcheck manual: <https://valgrind.org/docs/manual/mc-manual.html>.
