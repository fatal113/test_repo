# Session B04: Preprocessor Macros and Bit Operations — Học liệu cốt lõi

> **Khóa học:** [NFP] Advanced C Programming · **Thời lượng:** 90 phút · **Bối cảnh:** MDB Edge Diagnostics Gateway — Simulated

## 🎯 Learning Outcomes

- **ADVC-H1SD [create]:** thiết kế, hiện thực và kiểm chứng mô-đun C17 không làm lộ ownership hoặc gây lỗi bộ nhớ trong fixture đã cho.
- **Increment M00-FND-04:** tạo API cờ và trường bit trên một **ảnh thanh ghi `uint32_t` nằm trong bộ nhớ của chương trình**; artifact là `assets/b04_macro_bits_demo.c`.
- **Bằng chứng đạt:** build `-std=c17 -Wall -Wextra -Wpedantic -Werror`, happy path đúng từng byte stdout, negative path đúng stderr/exit code.

## 1. Tiên quyết, mental map và ranh giới

### Tiên quyết

Bạn cần biết khai báo biến/hàm, biểu thức, `uint32_t`, compile và đọc stdout/stderr/exit code. B04 kế thừa contract hàm của B03, chuẩn bị representation và invariant cho B05–B07.

### Mental map

`source.c` → **tiền xử lý token** (`#include`, `#define`, `#if`) → translation unit C → **toán tử bit trên giá trị unsigned** → ảnh cấu hình có invariant → oracle.

Hai tầng phải tách biệt: preprocessor biến đổi token trước khi compiler kiểm kiểu; toán tử bit chạy trên giá trị khi chương trình thực thi. Macro không phải hàm, và mask không phải địa chỉ.

### Ranh giới portable/non-embedded bắt buộc

“Peripheral Access Layer” trong outline được **diễn giải lại** thành API thao tác một `register_image_t { uint32_t value; }` thuần bộ nhớ. Unit này không có hằng địa chỉ thiết bị, truy cập thanh ghi vật lý, `volatile` phần cứng, MMIO, HAL, ISR, RTOS hay flashing. Kết quả chỉ mô phỏng cách đóng gói cờ/trường bit để học C17 portable; không được dùng nó làm bằng chứng rằng truy cập thiết bị thật là đúng.

## 2. Content Outline — giữ nguyên thứ tự và trạng thái

- [x] 1 C Preprocessor Overview
- [x] 2 Macro:
- [x] 2.1 Macro definition
- [x] 2.2 Object-like Macros
- [x] 2.3 Function like Macros
- [x] 3 C Preprocessor Directives
- [x] 3.1 Inclusion directive
- [x] 3.2 Conditional compilation
- [x] 3.3 Diagnostics Directive
- [x] 3.4 Preprocessor Operators
- [x] 4 Bit Operations
- [x] 4.1 And, or, xor, not, shift
- [x] 4.2 User case
- [x] 4.2.1 MASK, SHIFT
- [x] 4.2.2 SET, CLEAR
- [x] 4.2.3 Read/write bit MASK.
- [x] 4.2.4 Example about Peripheral Access Layer

## 3. Nội dung lý thuyết cốt lõi

#### OUT-B04-01 — 1 C Preprocessor Overview

**Mapping:** `OUT-B04-01` · `ADVC-H1SD` · `M00-FND-04` — kiểm soát translation unit của demo.

**Định nghĩa/ranh giới:** preprocessor xử lý preprocessing token và directive trước pha dịch C. Nó có thể chèn header, thay macro và chọn token có điều kiện; nó không hiểu lifetime, type safety hay ownership như compiler.

**Vai trò/quyết định:** người viết phải quyết định việc nào thật sự cần biến đổi trước compile và việc nào nên là hằng có kiểu hoặc `static inline`. Cơ chế cốt lõi là “token vào → token sau tiền xử lý”; vì vậy lỗi macro có thể nhân bản biểu thức trước khi compiler thấy mã.

**Cơ chế:** implementation nhận diện directive theo dòng, mở rộng/rescan macro và tạo chuỗi token của translation unit trước khi phân tích kiểu C.

**Khi dùng/không dùng/trade-off:** dùng cho include, guard, feature build-time và kiểm điều kiện dịch. Không dùng thay luồng điều khiển runtime hoặc hàm có kiểu. Lợi ích là zero runtime cost; đổi lại debugging và diagnostic có thể trỏ vào mã sau mở rộng.

**Ví dụ riêng và oracle:** với `#define RETRIES 3` rồi `int n = RETRIES;`, chạy `gcc -std=c17 -E -P sample.c` phải có dòng `int n = 3;`. Đây là oracle token, khác với oracle chạy chương trình.

**Best practice:** **Rule:** xem output `-E` khi hành vi macro khó hiểu. **Rationale:** output cho thấy token compiler thực nhận. **Positive:** kiểm `int n = 3;`. **Negative:** chỉ đọc `#define` rồi đoán, bỏ sót macro bị header khác định nghĩa lại.

**Failure → xử lý:** dấu hiệu `undeclared` sau include → nhánh tiền xử lý đã loại declaration → chạy `gcc -E -dD` và tìm token/định nghĩa → sửa điều kiện hoặc include → phòng tránh bằng test cả cấu hình build được hỗ trợ.

### 2 Macro:

#### OUT-B04-03 — 2.1 Macro definition

**Mapping:** `OUT-B04-03` · `ADVC-H1SD` · `M00-FND-04` — định nghĩa macro có contract hẹp.

**Định nghĩa/ranh giới:** macro được tạo bằng `#define identifier replacement-list` hoặc dạng có tham số. Replacement list là token; không phải biến và không tạo object có địa chỉ/lifetime.

**Vai trò/quyết định:** chọn tên, phạm vi header và replacement có an toàn khi mở rộng không. Khi gặp macro, mental model là thay token rồi rescan cho tới khi không còn macro áp dụng.

**Cơ chế:** `#define` thêm association tên–replacement vào trạng thái tiền xử lý; mỗi lần tên đủ điều kiện xuất hiện, replacement được chèn rồi rescan theo quy tắc macro.

**Khi dùng/không dùng/trade-off:** dùng cho cấu hình build hoặc thao tác preprocessor. Với hằng có kiểu, ưu tiên `enum`, `static const`; với hành vi runtime, ưu tiên hàm. Macro tiện nhưng namespace toàn translation unit và khó kiểm kiểu.

**Ví dụ riêng và oracle:** `#define BUILD_LEVEL 2` cùng `printf("level=%d\n", BUILD_LEVEL);` phải in chính xác `level=2` và exit `0`.

**Best practice:** **Rule:** mỗi macro public có prefix và contract. **Rationale:** macro không có namespace. **Positive:** `B04_BUILD_LEVEL`. **Negative:** `LEVEL`, dễ va chạm header khác và tạo warning/redefinition hoặc hành vi sai.

**Failure → xử lý:** dấu hiệu `macro redefined` → hai nguồn cùng tên → dùng `gcc -E -dM` xác định định nghĩa → đổi prefix hoặc bỏ định nghĩa trùng → guard header và không dùng tên chung.

#### OUT-B04-04 — 2.2 Object-like Macros

**Mapping:** `OUT-B04-04` · `ADVC-H1SD` · `M00-FND-04` — cấu hình build mặc định.

**Định nghĩa/ranh giới:** object-like macro không có danh sách tham số, ví dụ `#define B04_VARIANT portable_c17`. Nó thay mọi token tên đó trong vùng hiệu lực tiền xử lý; nó không phải object C.

**Vai trò/quyết định:** quyết định giá trị là token build-time hay dữ liệu runtime. Rescan cho phép macro A mở rộng sang macro B, nên thứ tự định nghĩa và `#undef` tác động kết quả.

**Cơ chế:** object-like macro thay identifier bằng replacement list mà không nhận argument; expansion tiếp tục qua các macro lồng cho tới khi rescan hoàn tất.

**Khi dùng/không dùng/trade-off:** phù hợp với cờ compile và literal cần cho `#if`; không phù hợp khi cần type, debugger symbol hoặc địa chỉ. `static const uint32_t` an toàn kiểu hơn nhưng không dùng trực tiếp trong `#if`.

**Ví dụ riêng và oracle:** asset định nghĩa mặc định `B04_VARIANT portable_c17`; `./b04 --self-test | head -n1` phải là `config=portable_c17`.

**Best practice:** **Rule:** bao literal số bằng macro tạo hằng chuẩn khi cần, như `UINT32_C(1)`. **Rationale:** tránh suy luận kiểu không mong muốn. **Positive:** `UINT32_C(1) << 31`. **Negative:** `1 << 31`, có thể dịch vào bit dấu của `int` và gây undefined behavior.

**Failure → xử lý:** cấu hình runtime không đổi dù sửa tham số → giá trị đã khóa lúc compile → kiểm command line và `gcc -dM -E` → rebuild với `-D...` hoặc chuyển thành argv → ghi rõ build-time/runtime trong contract.

#### OUT-B04-05 — 2.3 Function like Macros

**Mapping:** `OUT-B04-05` · `ADVC-H1SD` · `M00-FND-04` — nhận diện precedence và double evaluation.

**Định nghĩa/ranh giới:** function-like macro chỉ mở rộng khi tên theo sau bởi `(`; tham số là token, không có kiểu và có thể xuất hiện nhiều lần. Nó trông giống lời gọi hàm nhưng không có call frame hay quy tắc evaluate-once.

**Vai trò/quyết định:** quyết định liệu biểu thức có side effect hoặc cần type checking; nếu có, chuyển sang hàm. Parenthesize từng tham số và toàn replacement chỉ xử lý precedence, không xử lý double evaluation.

**Cơ chế:** preprocessor ghép argument token vào từng vị trí tham số trong replacement; nếu tham số xuất hiện hai lần thì biểu thức runtime tương ứng cũng có thể xuất hiện hai lần sau expansion.

**Khi dùng/không dùng/trade-off:** dùng cho stringification/token-pasting hoặc pattern không biểu diễn được bằng hàm. Không dùng kiểu `MAX(i++, j++)`. Macro có thể inline mọi nơi nhưng hàm `static inline` cũng thường được tối ưu và an toàn hơn.

**Ví dụ riêng và oracle:** `#define SQUARE_SAFE(x) ((x) * (x))`; với literal `4`, output là `square=16`. Không gọi macro này bằng `i++`.

**Best practice:** **Rule:** argument runtime chỉ được evaluate một lần; dùng `static inline`. **Rationale:** side effect phải xác định. **Positive:** `square_u32(value)`. **Negative:** `SQUARE_SAFE(i++)`, tăng `i` hai lần và còn có thể gây hành vi không xác định do sequencing.

**Failure → xử lý:** counter tăng hai lần → tham số lặp trong replacement → xem `gcc -E` → thay macro bằng hàm → thêm review rule cấm side-effect argument cho macro.

### 3 C Preprocessor Directives

#### OUT-B04-07 — 3.1 Inclusion directive

**Mapping:** `OUT-B04-07` · `ADVC-H1SD` · `M00-FND-04` — tạo translation unit có declaration nhất quán.

**Định nghĩa/ranh giới:** `#include <...>` tìm header hệ thống theo implementation; `#include "..."` bắt đầu theo quy tắc tìm file dự án rồi fallback theo implementation. Directive chèn nội dung header ở mức preprocessing.

**Vai trò/quyết định:** public header chứa declaration và guard; `.c` chứa definition. Include dependency phải tối thiểu nhưng đầy đủ, không dựa vào include bắc cầu.

**Cơ chế:** directive tìm resource theo dạng dấu ngoặc đã dùng, thay directive bằng nội dung header rồi tiếp tục tiền xử lý token được chèn.

**Khi dùng/không dùng/trade-off:** dùng header chuẩn đúng chức năng (`<stdint.h>` cho `uint32_t`, `<limits.h>` cho `CHAR_BIT`, `<inttypes.h>` cho macro format số nguyên). Không include `.c`. Nhiều include làm chậm build, nhưng thiếu include trực tiếp tạo phụ thuộc mong manh.

**Ví dụ riêng và oracle:** file include trực tiếp cả `<stdint.h>` và `<limits.h>`, rồi kiểm `sizeof(uint32_t) * CHAR_BIT`; output chính xác `u32_bits=32` trên implementation có `uint32_t`.

**Best practice:** **Rule:** header phải self-contained và có guard. **Rationale:** thứ tự include không được đổi nghĩa. **Positive:** consumer chỉ include header đó vẫn compile. **Negative:** header dùng `uint32_t` nhưng không include `<stdint.h>`, chỉ tình cờ compile theo thứ tự khác.

**Failure → xử lý:** `unknown type name uint32_t` → thiếu dependency trực tiếp → compile một translation unit chỉ include header → thêm include cần thiết → CI kiểm header self-contained.

#### OUT-B04-08 — 3.2 Conditional compilation

**Mapping:** `OUT-B04-08` · `ADVC-H1SD` · `M00-FND-04` — quản lý cấu hình demo portable.

**Định nghĩa/ranh giới:** `#if/#ifdef/#ifndef/#elif/#else/#endif` chọn token tại thời điểm dịch. Nhánh bị loại không tồn tại trong translation unit, khác `if` runtime vẫn được parse/type-check theo C.

**Vai trò/quyết định:** giới hạn số cấu hình, đặt mặc định rõ và test từng cấu hình. Dùng `defined(NAME)` để kiểm sự hiện diện, không nhầm “không định nghĩa” với “định nghĩa bằng 0”.

**Cơ chế:** biểu thức `#if` được đánh giá trong miền tiền xử lý; chỉ token của nhánh được chọn đi tiếp vào translation unit, các nhánh khác bị loại.

**Khi dùng/không dùng/trade-off:** dùng cho khác biệt build thật sự hoặc feature availability; không dùng để che lỗi hay nhân bản logic nghiệp vụ. Mỗi nhánh thêm một biến thể cần build/test.

**Ví dụ riêng và oracle:** build `gcc ... -DB04_VARIANT=training ...`; dòng đầu happy path phải là `config=training`. Build không `-D` phải là `config=portable_c17`.

**Best practice:** **Rule:** mỗi nhánh supported phải có oracle CI. **Rationale:** compiler không kiểm nhánh bị loại. **Positive:** build cả default/training. **Negative:** nhánh `#ifdef LEGACY` không build nhiều tháng rồi hỏng cú pháp.

**Failure → xử lý:** chỉ cấu hình production fail → nhánh chưa được compile trong test → lưu command và preprocessed output → sửa nhánh → lập matrix build nhỏ, tránh tổ hợp cờ bùng nổ.

#### OUT-B04-09 — 3.3 Diagnostics Directive

**Mapping:** `OUT-B04-09` · `ADVC-H1SD` · `M00-FND-04` — fail-fast khi môi trường sai contract.

**Định nghĩa/ranh giới:** `#error` yêu cầu translation thất bại và đưa preprocessing token sau nó vào diagnostic. C17 chuẩn hóa `#error`; `#warning` không phải directive ISO C17.

**Vai trò/quyết định:** dùng diagnostic compile-time cho prerequisite có thể kiểm bằng macro, ví dụ version chuẩn hoặc kiểu bắt buộc; không dùng làm validation dữ liệu runtime.

**Cơ chế:** khi nhánh chứa `#error` được chọn, preprocessing translation unit phát diagnostic và translation không thể hoàn tất thành executable hợp lệ.

**Khi dùng/không dùng/trade-off:** dùng để ngăn binary không thỏa precondition. Không lạm dụng cho mọi platform; có thể làm giảm khả năng port nếu điều kiện quá cứng.

**Ví dụ riêng và oracle:** `gcc -std=c11 ... b04_macro_bits_demo.c` phải exit khác `0`, stderr chứa chính xác chuỗi `b04_macro_bits_demo.c requires ISO C17 or newer`.

**Best practice:** **Rule:** diagnostic nêu requirement và cách sửa. **Rationale:** người dùng cần hành động được. **Positive:** “requires ISO C17 or newer”. **Negative:** `#error bad`, không cho biết flag nào sai.

**Failure → xử lý:** build dừng dù compiler hỗ trợ C17 → command không chọn đúng dialect → xem `__STDC_VERSION__` bằng `gcc -dM -E` → thêm `-std=c17` → khóa warning/build profile trong tài liệu.

#### OUT-B04-10 — 3.4 Preprocessor Operators

**Mapping:** `OUT-B04-10` · `ADVC-H1SD` · `M00-FND-04` — tạo tên cờ và chuỗi cấu hình có kiểm soát.

**Định nghĩa/ranh giới:** `#` stringify spelling của argument; `##` nối token. Argument liên quan có quy tắc expansion đặc biệt, vì vậy thường cần macro hai tầng để stringify giá trị đã mở rộng.

**Vai trò/quyết định:** chỉ dùng khi tên/chuỗi phải sinh ở compile time; tránh API “ma thuật” làm khó search/debug. `STRINGIFY_INNER` + `STRINGIFY` trong asset cho phép mở rộng rồi stringify.

**Cơ chế:** `#` biến spelling token của argument thành string literal; `##` ghép token lân cận rồi kết quả hợp lệ được rescan, nên macro hai tầng kiểm soát thời điểm expansion.

**Khi dùng/không dùng/trade-off:** tốt cho declaration family nhỏ và build label; không dùng để sinh hàng chục API khác hành vi. Ít lặp code nhưng diagnostic và IDE navigation kém hơn.

**Ví dụ riêng và oracle:** `DECLARE_FLAG(READY, 2)` tạo identifier `READY_MASK`; kết hợp stringify cấu hình cho output `config=portable_c17`.

**Best practice:** **Rule:** token-pasting phải tạo identifier hợp lệ, có prefix và được compile-test. **Rationale:** lỗi chỉ lộ sau expansion. **Positive:** `name##_MASK`. **Negative:** nối dữ liệu người dùng hoặc token rỗng, tạo identifier khó đoán.

**Failure → xử lý:** diagnostic nhắc identifier lạ → `##` sinh token sai → xem `gcc -E -P` → tách macro hai tầng/đổi argument → giữ macro sinh tên ngắn và có test symbol.

### 4 Bit Operations

#### OUT-B04-12 — 4.1 And, or, xor, not, shift

**Mapping:** `OUT-B04-12` · `ADVC-H1SD` · `M00-FND-04` — biểu diễn cờ trên unsigned integer.

**Định nghĩa/ranh giới:** `&`, `|`, `^`, `~`, `<<`, `>>` thao tác bit sau integer promotions. Toán tử logic `&&`, `||`, `!` cho kết quả logic và không thay thế toán tử bit.

**Vai trò/quyết định:** chọn unsigned width cố định, bảo đảm shift count nhỏ hơn width, và xác định mask trước khi thao tác. Với `uint32_t`, coi 32 ô bit được đánh số 0..31.

**Cơ chế:** toán hạng trải qua integer promotions/conversions rồi operator áp dụng từng bit; shift trái/phải di chuyển bit theo count hợp lệ trong width của kiểu đã promote.

**Khi dùng/không dùng/trade-off:** dùng cho flags, packed field, checksum đơn giản. Không dùng packing khi readability quan trọng hơn hoặc dữ liệu phải serialize đa nền tảng mà chưa định nghĩa byte order. Gọn bộ nhớ nhưng tăng rủi ro precedence/width.

**Ví dụ riêng và oracle:** với `uint8_t x=0xA5`, `y=0x0F`, sau cast phù hợp: AND=`0x05`, OR=`0xAF`, XOR=`0xAA`, NOT(x)=`0x5A`; `UINT32_C(3)<<4`=`0x30`, rồi dịch phải 4 được `3`.

**Best practice:** **Rule:** dùng toán hạng unsigned và kiểm shift. **Rationale:** shift âm/quá width là undefined; shift signed dễ phụ thuộc representation/range. **Positive:** `UINT32_C(1) << 31`. **Negative:** `1 << 31`.

**Failure → xử lý:** bit cao sai chỉ ở optimization/platform khác → signed promotion/shift invalid → bật `-Wshift-overflow`, xem type → đổi sang hằng unsigned và validate count → API nhận `uint32_t` và range-check.

### 4.2 User case

#### OUT-B04-14 — 4.2.1 MASK, SHIFT

**Mapping:** `OUT-B04-14` · `ADVC-H1SD` · `M00-FND-04` — định vị trường MODE ba bit.

**Định nghĩa/ranh giới:** mask chọn tập bit; shift là vị trí bit thấp nhất của trường. Với `MODE_MASK=0x70`, `MODE_SHIFT=4`, miền giá trị là `0..7`.

**Vai trò/quyết định:** phải quyết định width/position không chồng lấn và giá trị có fit hay không. Đọc: `(image & mask) >> shift`; ghi: `(image & ~mask) | ((value << shift) & mask)`.

**Cơ chế:** AND giữ bit thuộc field, shift chuẩn hóa field về bit 0; khi ghi, AND-NOT xóa field cũ và OR đưa field mới đã dịch vào đúng vị trí.

**Khi dùng/không dùng/trade-off:** dùng cho schema bit cố định, được document. Không dùng mask rời rạc với công thức field liên tục nếu chưa chứng minh. Packing nhanh/gọn nhưng thay schema có thể phá tương thích.

**Ví dụ riêng và oracle:** image `0xA5` đọc MODE được `2`; ghi MODE=`5` tạo image `0xD5`. Giá trị `8` phải bị từ chối và image giữ nguyên.

**Best practice:** **Rule:** validate `value <= mask >> shift` trước shift. **Rationale:** masking âm thầm cắt mất bit cao. **Positive:** reject 8 cho field 3 bit. **Negative:** `(8<<4)&0x70` thành `0`, che lỗi input.

**Failure → xử lý:** ghi `8` nhưng đọc lại `0` → truncation do mask → log input/mask/shift và so field_max → trả lỗi không mutate → test boundary `0`, `max`, `max+1`.

#### OUT-B04-15 — 4.2.2 SET, CLEAR

**Mapping:** `OUT-B04-15` · `ADVC-H1SD` · `M00-FND-04` — cập nhật flag độc lập.

**Định nghĩa/ranh giới:** set dùng `value |= mask`; clear dùng `value &= ~mask`. Hai thao tác giữ nguyên bit ngoài mask nếu các toán hạng có width đúng.

**Vai trò/quyết định:** xác định mask nào được phép đổi và có cần thao tác nguyên tử trong bối cảnh concurrent không. Unit này single-thread, ảnh thường trong memory nên không tuyên bố atomicity.

**Cơ chế:** OR với mask buộc bit chọn thành `1`; AND với complement của mask buộc chúng thành `0`, trong khi algebra bit giữ các vị trí ngoài mask.

**Khi dùng/không dùng/trade-off:** dùng khi nhiều cờ cùng word và API bảo vệ invariant. Không dùng read-modify-write này cho shared concurrent state nếu thiếu synchronization. Gọn nhưng coupling các cờ vào một word.

**Ví dụ riêng và oracle:** bắt đầu `0x00`, set `ENABLE_MASK=0x01` và `READY_MASK=0x04` → `0x05`; clear READY → `0x01`.

**Best practice:** **Rule:** clear bằng `&= ~mask`, không XOR. **Rationale:** XOR toggle phụ thuộc trạng thái cũ. **Positive:** clear hai lần vẫn `0`. **Negative:** `value ^= mask` lần hai bật cờ trở lại.

**Failure → xử lý:** lệnh “clear” thỉnh thoảng bật bit → dùng XOR → trace trước/sau → đổi sang AND-NOT → test tính idempotent của set/clear.

#### OUT-B04-16 — 4.2.3 Read/write bit MASK.

**Mapping:** `OUT-B04-16` · `ADVC-H1SD` · `M00-FND-04` — API đọc/ghi không phá bit lân cận.

**Định nghĩa/ranh giới:** read trích field, write là read-modify-write có mask. Nó không đồng nghĩa đọc/ghi byte từ file hay thiết bị; object duy nhất ở đây là `uint32_t` trong tiến trình.

**Vai trò/quyết định:** contract phải nêu nullability, mask/shift hợp lệ, failure có mutate không. Asset chọn “trả `false`, không thay image” khi field không hợp lệ.

**Cơ chế:** helper tính miền `mask >> shift`, validate input, tạo word ứng viên bằng clear-then-OR và chỉ commit assignment sau khi mọi precondition đạt.

**Khi dùng/không dùng/trade-off:** dùng khi cần update một field trong word chứa nhiều trường. Không dùng nếu schema có endian/serialization chưa chốt. API helper thêm code nhưng gom validation một nơi.

**Ví dụ riêng và oracle:** image `0x81`, ghi field MODE=`3` tạo `0xB1`, đọc lại `3`, đồng thời bit 0 và bit 7 vẫn bằng `1`.

**Best practice:** **Rule:** compute giá trị mới rồi commit một lần sau validation. **Rationale:** failure phải giữ state cũ. **Positive:** asset kiểm `field_value` trước assignment. **Negative:** clear field trước, sau đó mới phát hiện input quá lớn, làm mất state.

**Failure → xử lý:** negative path trả lỗi nhưng image đổi → mutation trước validation → so snapshot before/after → reorder validate-then-commit → regression test state unchanged.

#### OUT-B04-17 — 4.2.4 Example about Peripheral Access Layer

**Mapping:** `OUT-B04-17` · `ADVC-H1SD` · `M00-FND-04` — mô phỏng layer cờ hoàn toàn portable.

**Định nghĩa/ranh giới:** trong Unit này, “layer” là tập hàm `flags_set`, `flags_clear`, `flags_test`, `field_write`, `field_read` nhận con trỏ tới `register_image_t`. Nó **không** đại diện địa chỉ ngoại vi hay giao tiếp phần cứng.

**Vai trò/quyết định:** maintainer nhận một API có invariant: mode `0..7`, flag không chồng trường, rejected write không đổi state. Cơ chế chỉ là phép toán trên `uint32_t` object có lifetime bình thường.

**Cơ chế:** caller truyền địa chỉ của object `register_image_t`; các hàm đọc/sửa member `value` bằng mask thuần, không tạo hoặc dereference bất kỳ địa chỉ thiết bị nào.

**Khi dùng/không dùng/trade-off:** dùng làm fixture unit test cho logic encode/decode trước khi tích hợp một adapter riêng. Không copy sang driver thật và không suy ra ordering/side-effect của thiết bị. Tách pure logic giúp test dễ; adapter phần cứng nếu có về sau cần chuẩn/platform riêng.

**Ví dụ riêng và oracle:** chạy asset happy path phải lần lượt có `after_set=0x00000005`, `after_mode=0x00000055 mode=5`, `after_clear=0x00000051 enabled=1`, cuối cùng `self-test=PASS`. Negative dùng mode `8`, exit `2`, stdout rỗng, stderr đúng một dòng đã công bố trong Example.

**Best practice:** **Rule:** giữ core encode/decode thuần và truyền state bằng con trỏ object. **Rationale:** test không phụ thuộc môi trường và boundary rõ. **Positive:** `register_image_t image={0}`. **Negative:** cast số nguyên thành con trỏ rồi dereference; hành vi không thuộc Unit và có thể invalid/undefined.

**Failure → xử lý:** demo chạy khác theo máy hoặc gây access violation → code đã đưa địa chỉ/extension vào core → search cast integer-to-pointer và dependency platform → trả về in-memory object → CI strict C17, không macro địa chỉ.

## 4. Ví dụ tích hợp và oracle

Artifact: `assets/b04_macro_bits_demo.c`. Nó nối preprocessing, `#error`, `#`, `##`, object/function-like macro và API mask trên ảnh `uint32_t`.

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  assets/b04_macro_bits_demo.c -o b04_macro_bits_demo
./b04_macro_bits_demo --self-test
```

Output chính xác:

```text
config=portable_c17
initial=0x00000000
after_set=0x00000005
after_mode=0x00000055 mode=5
after_clear=0x00000051 enabled=1
self-test=PASS
```

Negative oracle và command đầy đủ nằm trong `example.md`.

## 5. Lỗi thường gặp

| Dấu hiệu | Nguyên nhân | Chẩn đoán | Sửa | Phòng tránh |
|---|---|---|---|---|
| Argument macro chạy hai lần | replacement lặp tham số | `gcc -E -P` | đổi thành `static inline` | cấm side-effect argument |
| Field lân cận bị mất | ghi thẳng thay vì read-modify-write | so hex before/after | dùng clear-mask rồi OR | test bit không thuộc mask |
| Bit cao sai | literal signed/shift quá width | warning + kiểm type/count | `UINT32_C`, range-check | strict warnings và boundary test |
| Chỉ một build variant lỗi | nhánh `#if` chưa được compile | build matrix, `-E` | sửa nhánh | test mọi variant supported |
| Negative path đổi state | mutate trước validate | snapshot state | validate rồi commit | oracle state-unchanged |

## 6. Thuật ngữ

- **Preprocessing token:** đơn vị preprocessor biến đổi trước khi dịch C.
- **Replacement list:** dãy token thay cho tên macro.
- **Object-like/function-like macro:** macro không/có danh sách tham số.
- **Mask:** word có bit `1` tại vị trí cần chọn.
- **Shift:** số vị trí dịch; phải nhỏ hơn width toán hạng promoted.
- **Register image:** object `uint32_t` mô phỏng bố cục cờ, không phải thanh ghi thiết bị.
- **Read-modify-write:** đọc state cũ, tạo state mới có bảo toàn bit ngoài mask, rồi commit.

## 7. Quiz tự kiểm tra (5 câu, không chấm điểm)

1. Vì sao `SQUARE(i++)` nguy hiểm dù macro đã có ngoặc đầy đủ?
2. `#if FEATURE` khác `if (feature)` ở thời điểm nào và hệ quả kiểm kiểu ra sao?
3. Với mask `0x70`, shift `4`, giá trị lớn nhất hợp lệ là bao nhiêu?
4. Vì sao clear cờ không nên dùng XOR?
5. B04 gọi “Peripheral Access Layer” nhưng artifact được phép thao tác đối tượng nào?

### Đáp án có giải thích

1. Tham số xuất hiện hai lần nên side effect có thể xảy ra hai lần; ngoặc chỉ sửa precedence. Dùng hàm evaluate-once.
2. `#if` loại token trước compiler; `if` là câu lệnh C runtime và cả mã nguồn liên quan vẫn phải parse/type-check. Vì vậy mọi nhánh preprocessor supported cần build riêng.
3. `0x70 >> 4 == 7`; miền là `0..7`. Giá trị `8` phải bị từ chối trước khi mutate.
4. XOR là toggle: clear lần thứ hai sẽ bật lại. `value &= ~mask` mới có tính idempotent cho clear.
5. Chỉ `register_image_t` chứa `uint32_t` trong memory của chương trình; không có địa chỉ thiết bị hay I/O phần cứng.

## 7. Nguồn tham khảo và provenance phần bổ sung

- `SRC-C17-ISO`: [ISO/IEC 9899:2018](https://www.iso.org/standard/74528.html), ISO/IEC JTC 1/SC 22, Edition 4 (C17), truy cập 2026-08-22; dùng metadata/clause reference, không sao chép chuẩn.
- `SRC-C17-WG14`: [WG14 N2176](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf), 2017-10-09, committee document; các mục 5.1.1.2, 6.5.7, 6.10 được diễn giải.
- `SRC-CERTC`: [SEI CERT C Coding Standard](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/), Carnegie Mellon University Software Engineering Institute, snapshot online truy cập 2026-08-22; tham chiếu PRE/EXP/INT.
- `SRC-GCC11`: [GCC 11.4 manuals](https://gcc.gnu.org/onlinedocs/gcc-11.4.0/), GNU Project/Free Software Foundation, version 11.4.0, truy cập 2026-08-22; dùng `-E`, `-dM`, dialect/warning options.
- `SRC-USER-CREF`: outline portable C refresher do người dùng phê duyệt 2026-08-22.

**Phần bổ sung:** `[BỔ SUNG — nguồn: ISO C17, CERT C và GCC 11.4 manuals]` các oracle nhỏ và troubleshooting được biên soạn mới để dạy cách kiểm chứng; không tuyên bố là trích nguyên văn. Không có nội dung thiết bị/embedded bổ sung.
