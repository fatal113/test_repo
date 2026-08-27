# Session B02: Arrays Decisions and Loops — Học liệu cốt lõi

> **Khóa học:** [NFP] Advanced C Programming · **Ngôn ngữ:** ISO C17 · **Thuộc:** Optional C Basics Refresher

## 🎯 Learning Outcomes

- **ADVC-H1SD** [create] — Hiện thực và kiểm chứng mô-đun C17 có bounds, invariant và failure behavior quan sát được.

**Tiểu mục tiêu diễn giải (không phải ID Learning Outcome trong kế hoạch):** chọn/duyệt mảng có bounds; xây decision và loop từ predicate, invariant và termination condition rõ.

## 🧭 Ngữ cảnh nghề nghiệp/dự án và phần tăng trưởng của Unit

Trong **MDB Edge Diagnostics Gateway — Simulated**, B02 tạo milestone **M00-FND-02**: phân tích tối đa tám sample bằng array bounded, decision và loop. Artifact kế hoạch là `assets/b02_array_flow_demo.c`; nó nhận mode `sum`, `max` hoặc `first-positive`, từ chối input sai bằng stderr/exit code cố định và không dùng heap.

**Biên không nhúng:** dữ liệu đến từ `argv`, không từ ADC, DMA, ISR, ring buffer phần cứng hay MMIO. Row-major array là quy tắc ngôn ngữ C; cache line, bus width và timing không được giả định. Bài học về bounds/invariant chuyển giao được sang embedded nhưng artifact hiện tại là hosted C17 local.

## 📚 Nguồn đầu vào đã map

- **SRC-C17-ISO:** ISO/IEC 9899:2018, *Information technology — Programming languages — C*, Edition 4, ISO/IEC JTC 1/SC 22, 2018; truy cập 2026-08-22: https://www.iso.org/standard/74528.html
- **SRC-C17-WG14:** WG14 N2176, *C17 ballot — ISO/IEC 9899:2017, Programming languages — C*, 2017-10-09; truy cập 2026-08-22: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf
- **SRC-CERTC:** Carnegie Mellon University Software Engineering Institute, *SEI CERT C Coding Standard*, ARR/EXP/MSC guidance from the online work-in-progress snapshot constrained to C17; truy cập 2026-08-22: https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/
- **SRC-GCC11:** GNU Project / Free Software Foundation, *GCC 11.4 manuals*, version 11.4.0; phần C dialect/diagnostic dùng cho build gate; truy cập 2026-08-22: https://gcc.gnu.org/onlinedocs/gcc-11.4.0/

---

## 1. Kiến thức tiên quyết và môi trường

### Kiến thức/kỹ năng tiên quyết

- Hoàn thành B01; hiểu scalar, `size_t`, pointer cơ bản và checked parsing.
- Biết build/run, phân biệt stdout/stderr và lấy exit code.
- Nhận biết undefined behavior không bắt buộc tạo crash hay diagnostic.

### Môi trường và kiểm tra nhanh

- Baseline: Ubuntu 22.04/WSL2, GCC 11.4; source chỉ cần hosted ISO C17.
- Build gate: `gcc -std=c17 -Wall -Wextra -Wpedantic -Werror`.
- Asset gate: `./b02_array_flow_demo --self-test` phải in đúng `B02 SELF-TEST PASS checks=8`.

## 2. Định vị trong lộ trình (Mental Map)

```text
B01 typed record
   ↓
B02 bounded sequence → predicate → branch → loop invariant → deterministic result
   ↓
B03 function contract cho array + length và nhiều output
```

Một mảng trả lời “các phần tử ở đâu”; bounds trả lời “được phép chạm phần tử nào”; decision chọn đường đi; loop lặp bước chuyển trạng thái. Trước mỗi loop, viết invariant và termination condition để dự đoán behavior thay vì dò bằng thử nghiệm.

## 3. Nội dung lý thuyết cốt lõi

**📋 Checklist bắt buộc phủ — giữ nguyên văn và đúng thứ tự Content Outlines:**

- [x] 1 Array in C
- [x] 1.1 What is Array?
- [x] 1.2 Multidimensional Arrays
- [x] 1.3 Array in memory
- [x] 1.4 How to declare an Array?
- [x] 1.5 How works with array?
- [x] 1.6 When will using Array?
- [x] 2 Decision in C
- [x] 2.1 Introduce
- [x] 2.2 How to build an expression?
- [x] 2.3 If, else, condition Operator “?:”
- [x] 2.4 Switch
- [x] 3 Looping In C
- [x] 3.1 What is looping in C?
- [x] 3.3 Enter and exit, break looping follow
- [x] 3.4 How to use looping?
- [x] 3.5 Key for Looping

### OUT-B02-01 1 Array in C

Nhóm này đi từ type/layout tới declaration, access và tiêu chí lựa chọn. Mọi leaf giữ chung invariant của case: `0 <= count <= MAX_VALUES` và chỉ `values[0]..values[count-1]` đã được khởi tạo.

#### OUT-B02-02 1.1 What is Array?

**Mapping:** `OUT-B02-02 / 1.1 What is Array?` · LO `ADVC-H1SD` · Milestone `M00-FND-02`.

##### Định nghĩa và ranh giới

Array là object gồm số phần tử cố định của cùng một element type, được đánh chỉ số từ `0`. Array không phải pointer; trong đa số expression nó được chuyển thành pointer tới phần tử đầu, làm mất thông tin số phần tử.

##### Vai trò và quyết định

Case cần buffer tối đa tám `int32_t`. Developer chọn fixed array vì capacity nhỏ, đã biết và không cần allocation failure; contract vẫn phải mang `count` riêng.

##### Cơ chế

Declaration `int32_t values[MAX_VALUES]` cấp storage cho tám phần tử. Chỉ prefix dài `count` là initialized input; `sizeof values` chỉ cho toàn array trong scope declaration, không dùng được sau parameter adjustment.

##### Khi dùng / không dùng / trade-off

Dùng array fixed khi element đồng nhất và upper bound nhỏ/rõ. Không dùng khi dữ liệu unbounded hoặc cần grow; dynamic array phù hợp hơn nhưng thêm ownership/allocation. Fixed capacity đơn giản, đổi lại phải reject phần vượt giới hạn.

##### Ví dụ và exact oracle

Chạy `./b02_array_flow_demo --analyze max 12 7 25 9`; prefix có `count=4`. Dòng đầu phải đúng `count=4 accepted=4 rejected=0`; exit `0`.

##### Best practice

**Rule:** truyền array cùng length/capacity → **rationale:** pointer decay không mang bounds → **positive:** `analyze(values, count, mode, &result)` → **negative:** callee đo `sizeof(values)/sizeof(values[0])` trên parameter và nhận kích thước pointer.

##### Failure/troubleshooting

**Dấu hiệu:** đọc giá trị rác cuối buffer → **nguyên nhân:** loop tới capacity thay vì count → **chẩn đoán:** log `i,count`, sanitizer nếu có → **sửa:** bound `i < count` → **phòng tránh:** invariant prefix initialized.

#### OUT-B02-03 1.2 Multidimensional Arrays

**Mapping:** `OUT-B02-03 / 1.2 Multidimensional Arrays` · LO `ADVC-H1SD` · `M00-FND-02`.

##### Định nghĩa và ranh giới

`T a[R][C]` là array `R` phần tử, mỗi phần tử là array `C` phần tử; nó không phải pointer-to-pointer. Trong parameter, mọi dimension trừ dimension đầu cần biết để tính offset.

##### Vai trò và quyết định

Matrix nhỏ dùng cho bảng sample theo channel/time. Developer phải chốt column count trong type hoặc truyền representation phẳng kèm stride; `int **` không thay thế được contiguous 2D array.

##### Cơ chế

C lưu row-major: `a[r][c]` tương đương `*(*(a+r)+c)` và offset phần tử là `r*C+c`. Asset self-test cộng `{{1,2,3},{4,5,6}}` bằng loop lồng.

##### Khi dùng / không dùng / trade-off

Dùng khi cả shape và row-major access rõ. Với shape runtime/ảnh lớn, dùng flat buffer + dimensions/stride; trade-off là index verbose nhưng API không khóa column compile-time.

##### Ví dụ và exact oracle

Input matrix `2x3` trong self-test có tổng `21` và row count `2`. `./b02_array_flow_demo --self-test` phải in `B02 SELF-TEST PASS checks=8`; đổi `6` thành `7` làm check matrix-sum fail và stderr `B02 SELF-TEST FAIL ...`.

##### Best practice

**Rule:** ghi shape trong type/parameters → **rationale:** compiler cần stride → **positive:** `const int32_t grid[2][3]` → **negative:** cast `int32_t[2][3]` thành `int32_t **` rồi index, dẫn tới invalid pointer access.

##### Failure/troubleshooting

**Dấu hiệu:** row đầu đúng, row sau crash → **nguyên nhân:** sai stride/type → **chẩn đoán:** kiểm declaration parameter và `sizeof row` → **sửa:** đúng column dimension hoặc flat indexing → **phòng tránh:** test phần tử biên mỗi row.

#### OUT-B02-04 1.3 Array in memory

**Mapping:** `OUT-B02-04 / 1.3 Array in memory` · LO `ADVC-H1SD` · `M00-FND-02`.

##### Định nghĩa và ranh giới

Các phần tử array liên tiếp, không có padding giữa hai element; có thể có alignment/padding bên trong element struct. Pointer arithmetic hợp lệ trong cùng array object và vị trí one-past, nhưng không được dereference one-past.

##### Vai trò và quyết định

Hiểu layout để viết loop tuyến tính và tính index; không suy địa chỉ vật lý, cache behavior hay endianness từ quy tắc này.

##### Cơ chế

`&a[i] == a + i`; subtraction của hai pointer trong cùng array trả khoảng cách element, không phải byte. 2D array nối các row theo row-major.

##### Khi dùng / không dùng / trade-off

Dùng pointer/index traversal khi bounds đã chứng minh. Không dùng arithmetic để đi qua hai object rời; không dereference `a+count`. Index dễ review, pointer walk đôi khi gọn nhưng precondition khó nhìn hơn.

##### Ví dụ và exact oracle

Với `int a[3]={10,20,30};`, biểu thức `printf("%td\n", &a[2]-&a[0]);` phải in `2`. Đây là khoảng cách hai element; không phụ thuộc `sizeof(int)`.

##### Best practice

**Rule:** biểu diễn range bằng base + count → **rationale:** one-past chỉ làm sentinel, không phải object → **positive:** `for (i=0; i<count; ++i)` → **negative:** `i<=count` đọc one-past.

##### Failure/troubleshooting

**Dấu hiệu:** lỗi chỉ ở phần tử cuối → **nguyên nhân:** off-by-one → **chẩn đoán:** xem điều kiện `<=`, test count `0/1/capacity` → **sửa:** half-open range `[0,count)` → **phòng tránh:** dùng cùng convention toàn module.

#### OUT-B02-05 1.4 How to declare an Array?

**Mapping:** `OUT-B02-05 / 1.4 How to declare an Array?` · LO `ADVC-H1SD` · `M00-FND-02`.

##### Định nghĩa và ranh giới

Declaration gắn element type và bound: `int32_t values[8]`. Initializer có thể suy bound (`int a[] = {1,2}`) hoặc zero-initialize phần còn lại. VLA là feature tùy chọn trong C17 và không dùng trong asset portable này.

##### Vai trò và quyết định

Chọn named constant `MAX_VALUES=8`, tránh magic bound rải rác. Phân biệt capacity với current count và init storage trước khi có đường đọc.

##### Cơ chế

`int32_t values[MAX_VALUES] = {0};` khởi tạo tất cả phần tử bằng zero. `sizeof values / sizeof values[0]` dùng được tại nơi `values` vẫn là array expression.

##### Khi dùng / không dùng / trade-off

Suy bound tốt cho lookup table literal; explicit capacity tốt cho buffer. Không dựa VLA nếu target có `__STDC_NO_VLA__` hoặc stack budget không rõ.

##### Ví dụ và exact oracle

Snippet `int a[] = {4,5,6,7}; printf("%zu\n", sizeof a/sizeof a[0]);` phải in `4`. Trong asset, input thứ chín bị từ chối trước ghi ngoài buffer.

##### Best practice

**Rule:** một nguồn chân lý cho capacity → **rationale:** declaration và guard không lệch → **positive:** enum `MAX_VALUES` dùng cả array/validation → **negative:** array `[8]` nhưng guard `count<=10` gây overflow.

##### Failure/troubleshooting

**Dấu hiệu:** capacity check pass nhưng memory corrupt → **nguyên nhân:** magic numbers lệch → **chẩn đoán:** search mọi literal bound → **sửa:** named constant/derived count → **phòng tránh:** boundary self-test capacity và capacity+1.

#### OUT-B02-06 1.5 How works with array?

**Mapping:** `OUT-B02-06 / 1.5 How works with array?` · LO `ADVC-H1SD` · `M00-FND-02`.

##### Định nghĩa và ranh giới

Làm việc với array gồm populate, validate, traverse, reduce/search và không vượt initialized range. Array assignment toàn bộ không tồn tại trong C; cần loop hoặc library function với precondition đúng.

##### Vai trò và quyết định

Case populate từng argv đã parse, rồi reduce theo mode. Quyết định fail-fast tại token sai giữ prefix không bị dùng như kết quả hoàn chỉnh.

##### Cơ chế

Loop parse duy trì invariant: trước iteration `i`, `values[0..i)` hợp lệ. Loop max duy trì `result == max(values[0..i))` sau mỗi bước.

##### Khi dùng / không dùng / trade-off

Dùng một pass khi operation associative/streamable. Tách validate và analyze giúp contract rõ nhưng đi qua data hai lần; ở capacity 8, clarity quan trọng hơn micro-optimization.

##### Ví dụ và exact oracle

Input `12 7 25 9`, mode `max`; exact stdout thứ hai là `mode=MAX result=25 band=HIGH`. Nếu loop bắt đầu `i=0` với result `values[0]` vẫn đúng nhưng lặp thừa; asset bắt đầu `1`.

##### Best practice

**Rule:** viết invariant trước loop → **rationale:** chứng minh init/update/termination → **positive:** max khởi tạo từ phần tử đầu sau `count>0` → **negative:** `result=0` trả sai cho mảng toàn số âm.

##### Failure/troubleshooting

**Dấu hiệu:** max của `[-5,-2]` thành `0` → **nguyên nhân:** sentinel không thuộc data → **chẩn đoán:** test all-negative → **sửa:** init từ `values[0]` → **phòng tránh:** fixture theo partition domain.

#### OUT-B02-07 1.6 When will using Array?

**Mapping:** `OUT-B02-07 / 1.6 When will using Array?` · LO `ADVC-H1SD` · `M00-FND-02`.

##### Định nghĩa và ranh giới

Leaf này là decision rule: array phù hợp khi element đồng type, order/index có nghĩa, capacity/length quản lý được và contiguous storage hữu ích. Nó không tự cung cấp grow, bounds check hay ownership metadata.

##### Vai trò và quyết định

Gateway batch synthetic giới hạn tám sample, nên stack array tránh heap và có failure rule đơn giản. Workload unbounded phải dùng chunk/stream/dynamic container ở unit sau.

##### Cơ chế

CLI tính `count=argc-3`; guard `1..MAX_VALUES` chạy trước populate. Capacity overflow trở thành lỗi nghiệp vụ, không thành memory overwrite.

##### Khi dùng / không dùng / trade-off

Dùng cho lookup/bounded batch. Không dùng fixed stack array khổng lồ hoặc dữ liệu không có upper bound. Trade-off: deterministic memory vs giới hạn cứng/rejection.

##### Ví dụ và exact oracle

Gọi mode với chín values: `./b02_array_flow_demo --analyze sum 1 2 3 4 5 6 7 8 9`; stderr phải `error: expected 1..8 values, got 9`, stdout rỗng, exit `2`.

##### Best practice

**Rule:** capacity là public failure contract → **rationale:** caller cần biết overflow behavior → **positive:** reject rõ `got 9` → **negative:** silently drop phần tử thứ chín làm kết quả có vẻ hợp lệ nhưng thiếu data.

##### Failure/troubleshooting

**Dấu hiệu:** result thiếu sample không báo lỗi → **nguyên nhân:** truncation ngầm → **chẩn đoán:** so count input/accepted → **sửa:** reject hoặc report dropped count theo spec → **phòng tránh:** exact capacity+1 oracle.

### OUT-B02-08 2 Decision in C

Nhóm decision biến predicate scalar thành một nhánh điều khiển. Predicate phải diễn đạt business rule, không chứa side effect khó thấy và phải có policy cho mọi input class.

#### OUT-B02-09 2.1 Introduce

**Mapping:** `OUT-B02-09 / 2.1 Introduce` · LO `ADVC-H1SD` · `M00-FND-02`.

##### Định nghĩa và ranh giới

C xem scalar bằng `0` là false, khác `0` là true. Decision gồm condition và các đường control-flow; nó khác validation ở chỗ validation là policy dùng decision để chấp nhận/từ chối input.

##### Vai trò và quyết định

Case quyết định mode hợp lệ, token parse được, count trong bounds, có positive value hay không và result thuộc band nào.

##### Cơ chế

Mỗi guard giảm state space: invalid mode dừng trước array; invalid count dừng trước parse; invalid token dừng tại vị trí; analysis failure dùng exit `3` thay vì input syntax exit `2`.

##### Khi dùng / không dùng / trade-off

Dùng guard clauses khi failure độc lập và giúp happy path phẳng. Không phân nhánh theo condition lặp lại có thể mâu thuẫn; enum+switch hoặc table phù hợp hơn cho nhiều mode.

##### Ví dụ và exact oracle

`./b02_array_flow_demo --analyze median 1 2` phải stderr `error: mode must be sum, max, or first-positive`, stdout rỗng, exit `2`.

##### Best practice

**Rule:** mỗi branch có observable policy → **rationale:** tránh silent fallthrough/default → **positive:** invalid mode có message/exit → **negative:** map unknown mode sang SUM khiến typo không được phát hiện.

##### Failure/troubleshooting

**Dấu hiệu:** typo mode vẫn cho result → **nguyên nhân:** default success → **chẩn đoán:** negative unknown-mode test → **sửa:** explicit reject → **phòng tránh:** partition test mọi enum/input class.

#### OUT-B02-10 2.2 How to build an expression?

**Mapping:** `OUT-B02-10 / 2.2 How to build an expression?` · LO `ADVC-H1SD` · `M00-FND-02`.

##### Định nghĩa và ranh giới

Expression kết hợp operand/operator tạo value và có evaluation rules. Precedence không thay thế clarity; `&&`/`||` short-circuit từ trái sang phải, nhưng order của nhiều subexpression khác có thể không được chỉ định.

##### Vai trò và quyết định

Guard phải kiểm pointer trước dereference và range trước cast/index. Developer chọn expression dễ audit thay vì nhồi assignment/increment vào condition.

##### Cơ chế

`(text == NULL) || (out == NULL) || (*text == '\0')` short-circuit nên `*text` chỉ chạy sau `text != NULL`. Relational result được dùng làm predicate.

##### Khi dùng / không dùng / trade-off

Dùng short-circuit để bảo vệ access phụ thuộc. Tách expression phức tạp thành named booleans/guards; thêm dòng code nhưng giảm lỗi precedence/side effect.

##### Ví dụ và exact oracle

Với snippet `const char *p=NULL; puts((p!=NULL && *p!='\0') ? "data" : "blocked");`, exact output là `blocked` và không dereference null.

##### Best practice

**Rule:** không sửa cùng scalar nhiều lần trong một full expression → **rationale:** unsequenced side effects có thể UB → **positive:** increment ở statement riêng → **negative:** `a[i] = i++` có evaluation khó/không portable theo dạng dùng.

##### Failure/troubleshooting

**Dấu hiệu:** crash chỉ khi null → **nguyên nhân:** check đặt sau dereference hoặc dùng `|` thay `||` → **chẩn đoán:** inspect operator/order → **sửa:** guard trái trước access → **phòng tránh:** parentheses và tests null/empty.

#### OUT-B02-11 2.3 If, else, condition Operator “?:”

**Mapping:** `OUT-B02-11 / 2.3 If, else, condition Operator “?:”` · LO `ADVC-H1SD` · `M00-FND-02`.

##### Định nghĩa và ranh giới

`if/else` chọn statement; conditional operator `?:` là expression tạo value. `?:` không phải thay thế tốt cho multi-step branch hoặc nested policy khó đọc.

##### Vai trò và quyết định

Asset dùng `if` cho update max/validation và `?:` cho mapping đơn giản `result >= 20 ? HIGH : NORMAL`.

##### Cơ chế

Chỉ một trong operand thứ hai/thứ ba của `?:` được evaluate. Với if-else chain, thứ tự condition quyết định branch đầu tiên khớp.

##### Khi dùng / không dùng / trade-off

Dùng `?:` cho lựa chọn value ngắn, cùng ý nghĩa/type; dùng `if` khi có nhiều statement/error path. Nested ternary tiết kiệm dòng nhưng tăng ambiguity.

##### Ví dụ và exact oracle

Happy result `25` làm predicate true; output chính xác `mode=MAX result=25 band=HIGH`. Với `--analyze max 12 7`, output band phải `NORMAL`.

##### Best practice

**Rule:** branch phải mutually understandable và có braces cho statement blocks → **rationale:** tránh dangling-else/edit bug → **positive:** explicit guard + return → **negative:** nested `?:` gán status/message khó review.

##### Failure/troubleshooting

**Dấu hiệu:** boundary `20` vào sai band → **nguyên nhân:** dùng `>` thay `>=` → **chẩn đoán:** test 19/20/21 → **sửa:** align predicate với requirement → **phòng tránh:** boundary-value table trước code.

#### OUT-B02-12 2.4 Switch

**Mapping:** `OUT-B02-12 / 2.4 Switch` · LO `ADVC-H1SD` · `M00-FND-02`.

##### Định nghĩa và ranh giới

`switch` chọn case theo integral/enum controlling expression. Không có implicit break: execution tiếp tục sang case kế nếu không `break`, `return` hoặc control transfer khác.

##### Vai trò và quyết định

`AnalysisMode` có ba operation; switch làm mapping mode→algorithm rõ và có `default` reject invalid enum.

##### Cơ chế

Mỗi case trong `analyze()` `return` sau khi hoàn tất, nên không fallthrough. `default` trả false để caller tạo error thay vì dùng result chưa init.

##### Khi dùng / không dùng / trade-off

Dùng cho tập enum nhỏ. Function table phù hợp khi nhiều strategy/extensibility, nhưng thêm function pointers. Không switch trên string; parse string sang enum trước.

##### Ví dụ và exact oracle

`./b02_array_flow_demo --analyze sum 12 7 25 9` phải có dòng `mode=SUM result=53 band=HIGH`; cùng input mode max cho `25`, chứng minh case khác nhau.

##### Best practice

**Rule:** xử lý mọi enumerator và invalid value; annotate intentional fallthrough nếu có → **rationale:** enum có thể chứa giá trị ngoài set → **positive:** `default: return false` → **negative:** thiếu break vô tình chạy algorithm kế tiếp.

##### Failure/troubleshooting

**Dấu hiệu:** SUM result bị MAX ghi đè → **nguyên nhân:** fallthrough → **chẩn đoán:** warning `-Wimplicit-fallthrough`, trace case → **sửa:** return/break → **phòng tránh:** mỗi case nhỏ, compile warnings as errors.

### OUT-B02-13 3 Looping In C

Nhóm loop bao gồm `while`, `do...while`, `for`, cùng `break`/`continue`. Mọi loop phải có init, invariant, progress và termination condition; “chạy đúng fixture” chưa phải bằng chứng termination tổng quát.

#### OUT-B02-14 3.1 What is looping in C?

**Mapping:** `OUT-B02-14 / 3.1 What is looping in C?` · LO `ADVC-H1SD` · `M00-FND-02`.

##### Định nghĩa và ranh giới

Loop lặp statement khi condition cho phép. `while` kiểm trước, `do...while` chạy ít nhất một lần, `for` gom init/test/update. Chúng tương đương về khả năng biểu đạt, khác về intent và vị trí kiểm.

##### Vai trò và quyết định

Chọn `for` cho traversal có index; `while` cho số bước theo state; `do...while` cho action phải chạy ít nhất một lần như đếm chữ số `0`.

##### Cơ chế

Loop max: init result, `i=1`; test `i<count`; update result invariant; increment `i`. Khi exit, invariant suy ra max toàn prefix.

##### Khi dùng / không dùng / trade-off

Không dùng recursion cho linear scan chỉ để “gọn”; loop có stack usage hằng. Chọn form diễn đạt termination rõ nhất.

##### Ví dụ và exact oracle

`for` duyệt `[12,7,25,9]` và trả `25`; exact output `mode=MAX result=25 band=HIGH`. Count `1` vẫn đúng vì body max không chạy.

##### Best practice

**Rule:** ghi variant tiến gần termination → **rationale:** bắt infinite loop → **positive:** `i++` với `i<count` → **negative:** quên update `i` trong while làm process treo.

##### Failure/troubleshooting

**Dấu hiệu:** CPU cao, không output → **nguyên nhân:** condition không đổi → **chẩn đoán:** debugger interrupt/xem `i` → **sửa:** update trên mọi path → **phòng tránh:** variant/invariant review và timeout test.

#### OUT-B02-15 3.3 Enter and exit, break looping follow

**Mapping:** `OUT-B02-15 / 3.3 Enter and exit, break looping follow` · LO `ADVC-H1SD` · `M00-FND-02`.

##### Định nghĩa và ranh giới

`break` thoát loop gần nhất; `continue` bỏ phần còn lại của iteration rồi tới update/test. `return` thoát function. Chúng không tự khôi phục resource hay invariant.

##### Vai trò và quyết định

Search first-positive dùng `continue` bỏ non-positive và `break` ngay khi tìm thấy. Caller phân biệt “không có kết quả” với result `0` bằng boolean.

##### Cơ chế

Sau `break`, `i<count` chứng minh đã tìm thấy. Nếu loop kết thúc tự nhiên, `i==count` và function trả false; output parameter không được dùng.

##### Khi dùng / không dùng / trade-off

Dùng early exit cho search. Tránh nhiều break/continue khiến invariant phân mảnh; có thể tách helper. Với resource, cleanup path phải rõ.

##### Ví dụ và exact oracle

`./b02_array_flow_demo --analyze first-positive -2 0 7 11` phải in `mode=FIRST_POSITIVE result=7 band=NORMAL`. Input `-2 0` phải stderr `error: no value satisfies mode FIRST_POSITIVE`, exit `3`.

##### Best practice

**Rule:** nêu postcondition cho mỗi exit → **rationale:** tránh dùng output chưa gán → **positive:** chỉ print khi `analyze()==true` → **negative:** break condition sai rồi in stale result.

##### Failure/troubleshooting

**Dấu hiệu:** trả phần tử positive thứ hai/không trả → **nguyên nhân:** continue/break sai vị trí → **chẩn đoán:** trace index `[negative,zero,positive]` → **sửa:** guard non-positive trước assignment → **phòng tránh:** fixture đặt match ở đầu/giữa/cuối/không có.

#### OUT-B02-16 3.4 How to use looping?

**Mapping:** `OUT-B02-16 / 3.4 How to use looping?` · LO `ADVC-H1SD` · `M00-FND-02`.

##### Định nghĩa và ranh giới

Sử dụng loop đúng nghĩa là thiết kế init, condition, body, progress và postcondition cùng bounds; không chỉ chọn keyword. Loop parse còn phải có failure-atomic policy.

##### Vai trò và quyết định

Case parse argv theo thứ tự và fail tại token đầu sai. Prefix đã parse không được analyze vì toàn request bị reject.

##### Cơ chế

Invariant trước iteration `i`: `0<=i<=count` và `values[0..i)` hợp lệ. Parse failure return `2`; success ghi `values[i]`, rồi `i++`. Exit tự nhiên chứng minh toàn prefix hợp lệ.

##### Khi dùng / không dùng / trade-off

Dùng two-phase parse/analyze khi cần all-or-nothing. Streaming có thể xử lý một pass nhưng cần policy rollback/partial result rõ.

##### Ví dụ và exact oracle

`./b02_array_flow_demo --analyze max 12 bad 25` phải stderr `error: invalid integer at position 2: bad`, stdout rỗng, exit `2`; value `25` không được xử lý.

##### Best practice

**Rule:** kiểm bound trước write và validate trước publish → **rationale:** ngăn OOB/partial output → **positive:** capacity guard trước parse loop → **negative:** ghi `values[i]` rồi mới kiểm `i<MAX_VALUES`.

##### Failure/troubleshooting

**Dấu hiệu:** crash với input dài hoặc output partial khi token sai → **nguyên nhân:** guard/publish sai thứ tự → **chẩn đoán:** capacity+1 và bad-middle tests → **sửa:** precondition + two-phase → **phòng tránh:** assertions/invariants trong review.

#### OUT-B02-17 3.5 Key for Looping

**Mapping:** `OUT-B02-17 / 3.5 Key for Looping` · LO `ADVC-H1SD` · `M00-FND-02`.

##### Định nghĩa và ranh giới

Các keyword liên quan là `for`, `while`, `do`, `break`, `continue`; `goto` tồn tại nhưng không cần cho traversal này. Keyword không quyết định chất lượng; invariant và exit policy mới quyết định.

##### Vai trò và quyết định

Asset dùng đủ ba loop form có vai trò riêng: `for` cho arrays, `while` cho row index trong self-test, `do...while` cho decimal digits; `continue/break` cho search.

##### Cơ chế

`decimal_digits(0)` phải chạy body một lần nên do-while tự nhiên. Với while/for, condition false ban đầu sẽ chạy zero lần.

##### Khi dùng / không dùng / trade-off

Dùng form làm proof ngắn nhất; không ép mọi loop về một style. `goto cleanup` có thể hợp lý trong resource-heavy C, nhưng không cần và không được dạy như loop control thay thế tại đây.

##### Ví dụ và exact oracle

Self-test kiểm `decimal_digits(0)==1` và `decimal_digits(2026)==4`; exact aggregate oracle là `B02 SELF-TEST PASS checks=8`.

##### Best practice

**Rule:** update/progress phải dễ thấy, kể cả trước `continue` → **rationale:** continue có thể bỏ update ở while → **positive:** for-loop update expression vẫn chạy → **negative:** while `continue` trước `i++` gây infinite loop.

##### Failure/troubleshooting

**Dấu hiệu:** treo chỉ khi branch continue → **nguyên nhân:** progress bị skip → **chẩn đoán:** breakpoint tại continue và xem index → **sửa:** update trước continue hoặc dùng for → **phòng tránh:** review mọi control transfer trong loop.

## 4. Ví dụ tích hợp liên khái niệm có thể kiểm chứng

### Input/trạng thái ban đầu

Mode `max`; bốn sample `12 7 25 9`; capacity `8`; chưa có phần tử nào được parse.

### Cách thực hiện

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror \
  assets/b02_array_flow_demo.c -o b02_array_flow_demo
./b02_array_flow_demo --self-test
./b02_array_flow_demo --analyze max 12 7 25 9
```

### Output mong đợi

```text
B02 SELF-TEST PASS checks=8
count=4 accepted=4 rejected=0
mode=MAX result=25 band=HIGH
```

### Cách xác minh

- Build exit `0`, zero warnings; mỗi successful run stderr rỗng.
- Negative `--analyze max 12 bad 25` phải stderr `error: invalid integer at position 2: bad`, stdout rỗng, exit `2`.
- Test thêm empty, one element, capacity và capacity+1 để kiểm half-open bounds.

## 5. Lỗi thường gặp, troubleshooting và quy tắc áp dụng

| Dấu hiệu | Nguyên nhân | Chẩn đoán | Sửa | Phòng tránh |
|---|---|---|---|---|
| Lỗi ở phần tử cuối | `i <= count` | Test count 0/1/8, xem index | Dùng `[0,count)` | Half-open range convention |
| Max all-negative bằng 0 | Sentinel sai | Fixture `-5 -2` | Init từ phần tử đầu | Invariant ghi rõ |
| 2D row sau crash | Cast thành `T **`/sai stride | Xem parameter type | Đúng column count hoặc flat buffer | Shape trong API |
| Loop treo ở value đặc biệt | Continue bỏ progress | Interrupt debugger | Di chuyển update/dùng for | Review mọi exit/continue |
| Input sai vẫn có output | Analyze prefix partial | Bad-middle fixture | Parse toàn bộ trước analyze | All-or-nothing contract |

## 6. Từ điển thuật ngữ và mô hình tư duy

- **Element type:** kiểu chung của phần tử array.
- **Bound/capacity:** số slot được cấp; khác **count/length** số element hợp lệ hiện tại.
- **Pointer decay/adjustment:** array expression/parameter chuyển thành pointer trong các ngữ cảnh chuẩn định.
- **Row-major:** các element của một row đứng liên tiếp rồi tới row sau.
- **Half-open range:** `[begin,end)` gồm begin, không gồm end; có size `end-begin`.
- **Predicate:** expression scalar dùng làm điều kiện.
- **Loop invariant:** mệnh đề đúng trước/sau mỗi iteration.
- **Variant/progress measure:** đại lượng tiến về termination.
- **Fallthrough:** switch tiếp tục sang case kế khi không control-transfer.

## 7. Nguồn tham khảo và provenance phần bổ sung

- C17/WG14 N2176: §§6.5.2.1 array subscripting, 6.7.6.2 array declarators, 6.8.4 selection, 6.8.5 iteration, 6.8.6 jump statements.
- CERT C ARR/EXP/MSC: bounds, expression side effect và loop safety rationale.
- GCC manual: strict diagnostic invocation; output semantics vẫn theo ISO C17.
- Capacity `8`, modes, band threshold `20`, messages và exit codes là fixture synthetic của course.

## 8. Self-check Quiz

### Câu hỏi

1. Vì sao function nhận `int a[]` không thể suy length bằng `sizeof a`?
2. `int matrix[2][3]` có phải `int **` không?
3. Invariant đúng cho loop max sau khi đã xử lý prefix là gì?
4. Khi nào `do...while` phù hợp hơn `while` trong asset?
5. Vì sao capacity+1 phải là một negative test bắt buộc?

### Đáp án có giải thích

1. Trong parameter declaration, array được điều chỉnh thành pointer; `sizeof a` là kích thước pointer. Phải truyền length riêng.
2. Không. Nó là array của hai array ba `int`, contiguous row-major; `int **` là pointer tới pointer và có representation khác.
3. `result` bằng maximum của các phần tử trong prefix đã xử lý; init phải từ phần tử thật để đúng với all-negative input.
4. Đếm chữ số của `0` phải thực hiện body ít nhất một lần; do-while thể hiện trực tiếp điều này.
5. Nó chứng minh guard chạy trước write và policy overflow là reject thay vì overwrite/truncate im lặng.
