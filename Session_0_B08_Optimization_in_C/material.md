# B08 — Optimization in C — Học liệu cốt lõi

> **Khóa học:** [NFP] Advanced C Programming · **Thời lượng:** 90 phút · **LO:** `ADVC-H3SD`  
> **Ranh giới:** source là ISO C17 portable. GCC/Clang, linker MAP, `gprof` và `perf` là công cụ đo theo môi trường, không phải đặc tính bảo đảm bởi C. Không MCU, HAL, RTOS, MMIO, ISR hoặc flashing. Không tuyên bố “`-O2` luôn nhanh hơn” hay một speedup phổ quát.

## 🎯 Learning Outcomes

- Đánh giá tối ưu bằng baseline tái lập, output/checksum bất biến, compiler/linker/profile evidence và quyết định có giới hạn.
- **Increment `M00-FND-08`:** [b08_optimization_demo.c](assets/b08_optimization_demo.c) cho cùng workload ở `-O0`/`-O2`; correctness phải pass trước khi đọc timing.

## 1. Kiến thức tiên quyết và môi trường

- Hoàn thành B07; hiểu integer unsigned, function/loop và shell exit code.
- Baseline: Ubuntu 22.04, GCC 11.4, Binutils 2.38, `gprof` 2.38; `perf` có thể bị host chặn PMU.
- Strict flags: `-std=c17 -Wall -Wextra -Wpedantic -Werror`; profile build tách khỏi sanitizer build.

```sh
gcc --version
size --version
gprof --version
```

## 2. Mental map

`business/latency-size problem → fixed workload → correctness oracle → baseline build → profile hotspot → chọn transformation → build so sánh → output/checksum invariant → đo lặp lại → quyết định giữ/bỏ có phạm vi`

Optimizer được phép thay cách thực thi nhưng phải giữ observable behavior của chương trình có semantics xác định. Nếu `-O0` và `-O2` khác output, hãy nghi undefined behavior, data race, uninitialized state hoặc oracle không ổn định trước khi nói về hiệu năng.

## 3. Nội dung lý thuyết cốt lõi

- [x] why to optimize code
- [x] trade off between speed and size
- [x] Local optimization and global optimazation
- [x] Common sub-expression elimination
- [x] Constant propagation
- [x] Copy propagation
- [x] Dead code elimination
- [x] Global register allocation
- [x] Inline calls
- [x] Instruction scheduling
- [x] Lifetime analysis
- [x] Loop invariant expressions (code motion)
- [x] Loop unrolling
- [x] Strength reduction
- [x] Using compiler options
- [x] Analyze code distribution in memory using Linker MAP file
- [x] Profile the code

#### OUT-B08-01 — why to optimize code

**Mapping.** `OUT-B08-01` → `ADVC-H3SD` → `M00-FND-08`: chỉ tối ưu sau khi một mục tiêu đo được và correctness oracle đã khóa.

**Định nghĩa/ranh giới.** Tối ưu là thay đổi build/code/layout để cải thiện metric như latency, throughput, memory hoặc energy trong workload xác định mà không phá contract. “Code trông nhanh” hoặc một lần chạy ngắn không phải bằng chứng.

**Vai trò/quyết định.** Stakeholder phải chọn metric, workload, budget và regression guard. Nếu profile không cho thấy hotspot đáng kể, quyết định đúng có thể là không đổi code.

**Cơ chế.** Measurement so baseline/candidate trên cùng input, environment fingerprint và output checksum. Nhiễu scheduler/cache làm timing phân tán; correctness check đứng trước thống kê.

**Khi dùng/không dùng/trade-off.** Dùng khi có bottleneck hoặc budget thật; không tối ưu speculative ở code hiếm chạy. Tối ưu có thể tăng độ phức tạp, build time và maintenance cost.

**Ví dụ/oracle.** Workload `n=1000` của asset phải luôn in `OK n=1000 checksum=7BD0A72C`; chỉ sau `diff` exit `0` giữa O0/O2 mới thu profile. Metric timing không có ngưỡng phổ quát.

**Best practice.** **Rule:** measure → change one hypothesis → remeasure. **Rationale:** giữ quan hệ nhân-quả. **Positive:** lưu command, host fingerprint, checksum. **Negative:** đổi nhiều vòng lặp rồi chọn lần chạy nhanh nhất, không thể quy kết cải thiện.

**Failure/troubleshooting.** Timing dao động → workload quá ngắn/nhiễu host → kiểm distribution và tăng bounded repetitions → report median/range → phòng tránh bằng fixed input và warm-up công bố.

#### OUT-B08-02 — trade off between speed and size

**Mapping.** `OUT-B08-02` → `ADVC-H3SD` → `M00-FND-08`: quyết định build profile theo constraint thay vì mặc định một optimization level.

**Định nghĩa/ranh giới.** Speed và code/data size có thể xung đột: inline/unroll có thể giảm call/branch nhưng tăng `.text` và cache pressure. `size` đo section của binary cụ thể, không phải memory footprint toàn hệ thống.

**Vai trò/quyết định.** Chọn ưu tiên theo deployment: latency-sensitive, storage-constrained hay balanced. Không đánh đổi correctness/security để lấy số nhỏ hơn.

**Cơ chế.** Compiler chọn transformation theo flags; linker gom section; CPU cache/branch behavior khiến binary lớn hơn có thể nhanh hơn hoặc chậm hơn tùy workload.

**Khi dùng/không dùng/trade-off.** So `-O2`, `-Os` khi size là constraint; không suy từ kích thước file filesystem sang resident memory. Trade-off phải ghi cả metric và regression risk.

**Ví dụ/oracle.** Build O2/Os, chạy cùng `1000`; oracle bắt buộc cho cả hai là `checksum=7BD0A72C` và `diff` rỗng. `size` numbers được ghi nguyên trạng nhưng không có expected winner.

**Best practice.** **Rule:** tách correctness gate khỏi speed/size comparison. **Rationale:** metric tốt không hợp thức hóa behavior sai. **Positive:** bảng output hash + `.text/.data/.bss`. **Negative:** chọn binary nhỏ nhất dù negative input không còn exit `2`.

**Failure/troubleshooting.** Binary nhỏ nhưng chậm → instruction/cache trade-off → đo hotspot/counters nếu có → thử candidate riêng → phòng tránh bằng workload đại diện, không dùng size làm proxy duy nhất.

#### OUT-B08-03 — Local optimization and global optimazation

**Mapping.** `OUT-B08-03` → `ADVC-H3SD` → `M00-FND-08`: phân biệt transformation trong basic block/function với phân tích vượt function/translation unit.

**Định nghĩa/ranh giới.** Local optimization dùng phạm vi nhỏ; global optimization xét control/data flow rộng hơn. Interprocedural/LTO còn vượt translation unit và phụ thuộc toolchain/options, không phải semantics ISO C.

**Vai trò/quyết định.** Developer quyết định scope nhỏ nhất giải quyết hotspot; global/LTO cần regression mạnh hơn vì ảnh hưởng rộng và build/debug khác.

**Cơ chế.** Data-flow chứng minh expression/value/live range; optimizer chỉ biến đổi khi tin observable behavior được giữ theo language model.

**Khi dùng/không dùng/trade-off.** Local change dễ review; global option có thể phát hiện cơ hội lớn nhưng tăng compile/link time và khó map source/assembly.

**Ví dụ/oracle.** `mix_value` là scope function; O2 có thể tối ưu cả loop `compute_checksum`. Với `n=8`, cả O0/O2 phải in `OK n=8 checksum=6E7527A0`.

**Best practice.** **Rule:** bật scope rộng từng bước, giữ binary/flags/evidence. **Rationale/cơ chế:** mỗi lần chỉ đổi một phạm vi giúp quy biến đổi và regression cho đúng data-flow scope; optimization toàn cục tác động nhiều function/path hơn local optimization. **Positive:** O2 rồi LTO là hai candidate riêng. **Negative:** bật O3+LTO và sửa source cùng lúc, không biết nguyên nhân regression.

**Failure/troubleshooting.** Stack trace khác hẳn → inline/LTO → giữ debug candidate, xem flags/map → thu hẹp transformation → phòng tránh bằng reproducible build profiles.

#### OUT-B08-04 — Common sub-expression elimination

**Mapping.** `OUT-B08-04` → `ADVC-H3SD` → `M00-FND-08`: loại tính lại chỉ khi operands không thay đổi và expression không có side effect/volatile semantics.

**Định nghĩa/ranh giới.** CSE tái sử dụng kết quả expression tương đương. Nó không hợp lệ qua write có thể alias, function có side effect hoặc volatile access cần quan sát.

**Vai trò/quyết định.** Source nên diễn đạt intent rõ; không tự cache mọi expression nếu optimizer đã làm và cache làm stale data.

**Cơ chế.** Optimizer dùng value numbering/data-flow để chứng minh cùng value. `shared` trong `mix_value` làm quan hệ rõ mà vẫn cho compiler tối ưu.

**Khi dùng/không dùng/trade-off.** Dùng biến intermediate khi tăng clarity hoặc computation đắt; không cache read từ I/O/volatile. Thêm temporary có thể tăng live range.

**Ví dụ/oracle.** Expression `(scaled ^ 0xA5A5A5A5) + (scaled >> 3)` được tính một lần; `n=1` cho exact `OK n=1 checksum=96335EB2`.

**Best practice.** **Rule:** ưu tiên semantics/alias rõ, kiểm assembly khi metric yêu cầu. **Rationale/cơ chế:** CSE chỉ tái sử dụng expression khi value numbering chứng minh operands không bị alias/side effect thay đổi; contract rõ vừa an toàn vừa mở cơ hội tối ưu. **Positive:** const local `shared`. **Negative:** cache `*pointer` rồi function khác sửa pointee nhưng vẫn dùng cache.

**Failure/troubleshooting.** CSE không xuất hiện → alias/volatile/flags cản → xem optimized assembly → cải thiện contract const/restrict chỉ khi đúng → không thêm `restrict` sai để ép optimizer.

#### OUT-B08-05 — Constant propagation

**Mapping.** `OUT-B08-05` → `ADVC-H3SD` → `M00-FND-08`: theo dõi hằng compile-time qua expression để đơn giản hóa path.

**Định nghĩa/ranh giới.** Constant propagation thay value đã chứng minh bằng hằng; khác constant folding ở chỗ value được lan qua data flow. Input runtime `count` không phải hằng chỉ vì test dùng một giá trị.

**Vai trò/quyết định.** Dùng named constants cho contract; không hardcode fixture để “tối ưu” làm mất tính tổng quát.

**Cơ chế.** `UINT32_C(17)`, bias `3` và FNV multiplier là constants; compiler lan chúng trong `mix_value`/loop.

**Khi dùng/không dùng/trade-off.** Hữu ích cho configuration compile-time ổn định; runtime configuration cần giữ input. Specialization tăng code size.

**Ví dụ/oracle.** Dù constants được propagate, input `1003` vẫn được xử lý runtime và output phải là `OK n=1003 checksum=AC6DA027`.

**Best practice.** **Rule:** hằng phải có tên/type đúng miền. **Rationale/cơ chế:** propagation giữ semantics của type; suffix/type sai có thể kích hoạt promotion hoặc signed overflow khác với phép wrap `uint32_t` chủ ý. **Positive:** `UINT32_C(...)`. **Negative:** macro đổi type/signedness ngoài ý muốn, tạo checksum khác giữa platforms.

**Failure/troubleshooting.** Candidate sai ở O2 → signed overflow/UB bị lộ → UBSan và compare O0/O2 → chuyển arithmetic intended-wrap sang `uint32_t` → test boundary.

#### OUT-B08-06 — Copy propagation

**Mapping.** `OUT-B08-06` → `ADVC-H3SD` → `M00-FND-08`: loại copy scalar thừa khi value và alias relation cho phép.

**Định nghĩa/ranh giới.** Copy propagation thay use của biến copy bằng nguồn tương đương. Nó không cho phép bỏ copy khi snapshot cũ có ý nghĩa hoặc object có volatile/alias effect.

**Vai trò/quyết định.** Không “tối ưu” bằng cách gộp biến làm mất tên domain. Compiler có thể loại machine copy trong khi source vẫn rõ.

**Cơ chế.** SSA/data-flow theo definition-use chain; register allocation sau đó có thể map nhiều tên source vào cùng register.

**Khi dùng/không dùng/trade-off.** Tin compiler cho scalar; refactor source khi copy che intent hoặc object lớn. Snapshot deliberate phải được giữ/test.

**Ví dụ/oracle.** `checksum = compute_checksum(count)` có thể propagate return vào `printf`; source vẫn giữ tên. Oracle `n=1000` không đổi: `7BD0A72C`.

**Best practice.** **Rule:** đo generated code trước micro-refactor. **Rationale/cơ chế:** SSA/copy propagation thường xóa machine copy nhưng vẫn giữ tên source; refactor không có evidence có thể làm mất snapshot semantics mà không giảm instruction. **Positive:** giữ `checksum` cho review/debug. **Negative:** xóa biến snapshot rồi đọc mutable source lần hai, behavior đổi.

**Failure/troubleshooting.** Assembly vẫn có moves → ABI/debug/live range → inspect `-S` đúng flags → chấp nhận nếu không hotspot → tránh source khó đọc vì một move chưa chứng minh tốn kém.

#### OUT-B08-07 — Dead code elimination

**Mapping.** `OUT-B08-07` → `ADVC-H3SD` → `M00-FND-08`: loại computation không ảnh hưởng observable behavior, không loại validation/error path cần contract.

**Định nghĩa/ranh giới.** Dead code không reachable hoặc result không được dùng. Volatile access, I/O và function side effect là observable; “hiếm chạy” không đồng nghĩa dead.

**Vai trò/quyết định.** Xóa feature/config cũ ở source khi có thể; không dựa vào optimizer để che code nguy hiểm hoặc secret.

**Cơ chế.** Liveness/control-flow chứng minh result/path không tác động output/state. Checksum được in nên loop không dead.

**Khi dùng/không dùng/trade-off.** Compiler DCE tốt cho temporary; source cleanup giảm maintenance. Giữ diagnostic path dù happy benchmark không chạy nó.

**Ví dụ/oracle.** Với input `0`, parser vẫn phải exit `2`, stdout rỗng, stderr `ERROR item-count must be 1..1000000`; O2 không được “tối ưu mất” negative contract.

**Best practice.** **Rule:** regression cả happy và negative sau optimization. **Rationale/cơ chế:** DCE được phép bỏ code không observable, nên stderr/exit-code oracle chứng minh validation path vẫn là behavior cần giữ chứ không phải dead path. **Positive:** checksum + invalid-input oracle. **Negative:** benchmark chỉ happy rồi bỏ validation vì “không dùng”.

**Failure/troubleshooting.** Code bị bỏ ngoài ý muốn → result không observable/UB → inspect optimized assembly và output → tạo legitimate sink/test, sửa UB → không dùng volatile giả làm benchmark sink nếu không cần semantics đó.

#### OUT-B08-08 — Global register allocation

**Mapping.** `OUT-B08-08` → `ADVC-H3SD` → `M00-FND-08`: hiểu compiler gán live values vào register xuyên control flow; keyword `register` không bảo đảm register vật lý.

**Định nghĩa/ranh giới.** Register allocation là bước backend giải bài toán live ranges/spilling theo target ABI. Kết quả phụ thuộc architecture, flags và surrounding code.

**Vai trò/quyết định.** Developer giảm memory traffic qua data layout/algorithm khi profile chứng minh; không hardcode giả định register count portable.

**Cơ chế.** Lifetime analysis tạo interference; allocator chọn registers và spill. Call boundaries có caller/callee-saved constraints.

**Khi dùng/không dùng/trade-off.** Xem allocation khi hotspot instruction-level; không đánh giá từ source alone. Giảm live variables có thể giúp nhưng làm code khó đọc.

**Ví dụ/oracle.** Loop giữ `index`, `checksum`, `scaled/shared`; O2 assembly có thể giữ registers. Oracle vẫn là `n=1000 checksum=7BD0A72C`, không phải “phải dùng register X”.

**Best practice.** **Rule:** profile spill cost trước refactor. **Rationale/cơ chế:** allocator quyết định register/spill từ live ranges, ABI và target; chỉ spill trong hotspot đo được mới biện minh cho việc rút ngắn lifetime hoặc đổi algorithm. **Positive:** inspect annotated assembly. **Negative:** thêm `register` và tuyên bố speedup không đo.

**Failure/troubleshooting.** Nhiều stack load/store → register pressure → profiler/disassembly → thu hẹp live range nếu rõ lợi ích → retest output và performance distribution.

#### OUT-B08-09 — Inline calls

**Mapping.** `OUT-B08-09` → `ADVC-H3SD` → `M00-FND-08`: đánh giá trade-off call overhead/optimization context với code growth và debugability.

**Định nghĩa/ranh giới.** Inlining thay call bằng body về mặt generated code; `inline` trong C còn liên quan linkage và không bắt compiler inline. Compiler có thể inline function không ghi `inline`.

**Vai trò/quyết định.** Chỉ force/tool-specific inline khi profile và source policy cho phép; đây không phải lựa chọn portable mặc định.

**Cơ chế.** Sau inline, optimizer thấy constants/data flow rộng hơn; đổi lại duplicate body tăng `.text`.

**Khi dùng/không dùng/trade-off.** Function nhỏ/hot có thể lợi; function lớn/cold có thể hại I-cache. Debug stack ít rõ hơn.

**Ví dụ/oracle.** `mix_value` được gọi mỗi iteration; O2 có thể inline. Cả O0 và O2 với `n=8` phải cho `6E7527A0`.

**Best practice.** **Rule:** để optimizer quyết định trước, kiểm optimization report/map. **Rationale/cơ chế:** cost model cân call overhead với code growth và context mới sau inline; ép bằng macro bỏ type/single-evaluation contract mà vẫn có thể làm I-cache xấu hơn. **Positive:** source function nhỏ, pure. **Negative:** macro hóa function để ép inline, gây multi-evaluation/type defect.

**Failure/troubleshooting.** Binary phình → over-inlining → compare `size`/profile → remove force-inline hoặc split cold path → giữ output invariant.

#### OUT-B08-10 — Instruction scheduling

**Mapping.** `OUT-B08-10` → `ADVC-H3SD` → `M00-FND-08`: hiểu backend sắp instruction để giảm dependency stalls mà không đổi behavior.

**Định nghĩa/ranh giới.** Scheduling reorder machine instructions trong giới hạn dependency và memory model. Không được reorder vượt observable volatile/atomic rules; performance target-specific.

**Vai trò/quyết định.** Developer chủ yếu cung cấp dependency/data layout rõ; manual scheduling ở C hiếm portable.

**Cơ chế.** Backend model latency/resources, sắp independent operations quanh pipeline constraints. Alias uncertainty hạn chế reordering.

**Khi dùng/không dùng/trade-off.** Inspect khi profile cho thấy backend stalls; không viết source rối để đoán scheduler. Target khác có optimum khác.

**Ví dụ/oracle.** O2 có thể reorder XOR/shift/multiply trong checksum nhưng `n=1` vẫn phải `96335EB2` và invalid input vẫn exit `2`.

**Best practice.** **Rule:** giữ defined behavior và dùng target evidence. **Rationale/cơ chế:** scheduler dựa trên dependency/resource model của target, thứ mà thứ tự dòng C không mô tả; side effects hoặc sequencing mơ hồ vừa cản scheduling vừa có thể tạo UB. **Positive:** independent pure arithmetic. **Negative:** dựa vào thứ tự evaluation không được C bảo đảm hoặc nhét side effects vào một expression.

**Failure/troubleshooting.** O0/O2 khác → không kết luận scheduler bug trước → sanitizer/static review sequencing/UB → sửa source contract → so lại exact output.

#### OUT-B08-11 — Lifetime analysis

**Mapping.** `OUT-B08-11` → `ADVC-H3SD` → `M00-FND-08`: dùng live range để hiểu register/spill và đồng thời bảo toàn object lifetime của C.

**Định nghĩa/ranh giới.** Compiler liveness nói value còn được dùng; C object lifetime nói khi object tồn tại. Hai khái niệm liên quan nhưng không giống nhau; optimizer giả định code không dùng object ngoài lifetime.

**Vai trò/quyết định.** Thu hẹp scope khi tăng rõ ràng; không trả pointer tới local hoặc dùng uninitialized value để “giảm copy”.

**Cơ chế.** Definition/use xác định live range; scope/lifetime cho phép reuse storage/register. UB cho optimizer quyền loại/reorder theo assumption hợp lệ.

**Khi dùng/không dùng/trade-off.** Scope nhỏ giúp review và register pressure; scope quá vụn có thể giảm readability.

**Ví dụ/oracle.** `scaled` và `shared` chỉ sống trong `mix_value`; sanitizer run `n=1000` phải không diagnostic và output `7BD0A72C`.

**Best practice.** **Rule:** initialize tại nơi dùng và không để alias thoát lifetime. **Rationale/cơ chế:** liveness chỉ tối ưu value có definition hợp lệ; access trước initialization hoặc sau object lifetime là UB và làm mọi kết luận register/spill không đáng tin. **Positive:** const locals trong helper. **Negative:** callback giữ địa chỉ local rồi dùng sau return.

**Failure/troubleshooting.** O2-only corruption → uninitialized/dangling likely → ASan/UBSan + warnings → sửa lifetime/init → test nhiều optimization levels.

#### OUT-B08-12 — Loop invariant expressions (code motion)

**Mapping.** `OUT-B08-12` → `ADVC-H3SD` → `M00-FND-08`: đưa computation không đổi ra loop khi không có alias/side-effect cản trở.

**Định nghĩa/ranh giới.** Loop invariant có cùng value mọi iteration; code motion hợp lệ chỉ khi evaluation timing/exception/side effect không đổi observable behavior.

**Vai trò/quyết định.** Compiler thường làm; source hoisting dùng khi tăng clarity hoặc profile chứng minh và precondition cho phép tính một lần.

**Cơ chế.** Dominance/data-flow chứng minh operands không đổi. Volatile read, function side effect hoặc alias write làm expression không invariant.

**Khi dùng/không dùng/trade-off.** Hoist constants/bounds ổn định; không hoist validation phụ thuộc iteration. Hoisting có thể tính expression dù loop zero lần, nên cần chú ý lỗi/divide.

**Ví dụ/oracle.** Scale/multiplier là invariant của checksum loop; input `1003` vẫn phải `AC6DA027`.

**Best practice.** **Rule:** chứng minh invariant và zero-iteration behavior. **Rationale/cơ chế:** code motion đổi thời điểm evaluation; dù operands không đổi, hoist một operation có trap/side effect có thể tạo behavior ở path mà loop vốn chạy zero lần. **Positive:** const config trước loop. **Negative:** đưa division ra loop khiến divide-by-zero xảy ra dù loop không chạy.

**Failure/troubleshooting.** Candidate crash ở count zero → hoist đổi evaluation → negative test → đặt guard trước computation → phòng tránh boundary 0/1/max.

#### OUT-B08-13 — Loop unrolling

**Mapping.** `OUT-B08-13` → `ADVC-H3SD` → `M00-FND-08`: giảm loop-control overhead có kiểm soát và xử lý remainder đúng.

**Định nghĩa/ranh giới.** Unrolling lặp body nhiều lần mỗi iteration. Nó không tự bảo đảm nhanh hơn; tăng code size/register pressure và cần tail/remainder path.

**Vai trò/quyết định.** Dùng compiler flags trước manual unroll; manual chỉ khi hotspot và vectorization/report giải thích được.

**Cơ chế.** Ít branch/index updates hơn, nhiều independent operations hơn; dependency checksum tuần tự có thể hạn chế lợi ích.

**Khi dùng/không dùng/trade-off.** Hợp loop trip count lớn/đơn giản; không hợp code body lớn hoặc count nhỏ/không đều.

**Ví dụ/oracle.** Candidate unroll-factor 4 phải xử lý remainder của `n=1003`; exact output vẫn `OK n=1003 checksum=AC6DA027`.

**Best practice.** **Rule:** test counts `0,1,k-1,k,k+1`. **Rationale/cơ chế:** unrolling xử lý `k` phần tử mỗi body và cần remainder/control path riêng; các biên quanh `k` phơi lỗi bỏ sót hoặc đọc quá cuối. **Positive:** remainder loop rõ. **Negative:** chỉ benchmark multiple-of-4, bỏ ba phần tử cuối.

**Failure/troubleshooting.** Checksum sai ở 1003 nhưng đúng 1000 → remainder bug → compare first divergent index → sửa tail → giữ boundary regression.

#### OUT-B08-14 — Strength reduction

**Mapping.** `OUT-B08-14` → `ADVC-H3SD` → `M00-FND-08`: thay operation đắt bằng operation tương đương chỉ khi miền số học giữ semantics.

**Định nghĩa/ranh giới.** Ví dụ compiler thay multiply index bằng induction addition hoặc division power-of-two bằng shift khi signedness/rounding cho phép. Không phải mọi phép thay đều tương đương với số âm/overflow.

**Vai trò/quyết định.** Để compiler xử lý arithmetic thường; source transformation phải chứng minh range/type.

**Cơ chế.** Induction-variable analysis nhận linear recurrence. Trong demo, arithmetic `uint32_t` có wrap modulo xác định.

**Khi dùng/không dùng/trade-off.** Hợp loop numeric hot; không thay `/ 2` signed bằng `>> 1` nếu behavior với số âm chưa được định nghĩa trong contract.

**Ví dụ/oracle.** Dù compiler biến `index * 17 + 3`, `n=2500` phải in `OK n=2500 checksum=F2163101`.

**Best practice.** **Rule:** khóa type/range rồi compare output. **Rationale/cơ chế:** strength reduction chỉ tương đương trong miền số học đã chứng minh; promotions, rounding và signed overflow có thể làm shift/add khác phép nhân source. **Positive:** `uint32_t` constants. **Negative:** dùng signed overflow như modulo, O2 có thể tối ưu theo assumption không overflow.

**Failure/troubleshooting.** Chỉ sai ở input lớn → overflow domain khác → UBSan/range analysis → dùng checked/specified unsigned arithmetic → boundary tests.

#### OUT-B08-15 — Using compiler options

**Mapping.** `OUT-B08-15` → `ADVC-H3SD` → `M00-FND-08`: mỗi option set là một build profile có purpose và evidence riêng.

**Định nghĩa/ranh giới.** `-O0`, `-O2`, `-Os`, `-g`, `-pg`, sanitizer flags thay codegen/instrumentation. GCC-specific flags không thuộc ISO C; không trộn profiler/sanitizer rồi so timing như cùng baseline.

**Vai trò/quyết định.** Chốt compiler version, complete flags và link flags. Debug, release, profile và sanitizer là artifacts khác nhau.

**Cơ chế.** Frontend semantics → optimization pipeline → backend/link; instrumentation làm thay workload/timing.

**Khi dùng/không dùng/trade-off.** `-O0 -g` dễ debug; `-O2` candidate release; `-pg` cho gprof; sanitizers cho defect evidence. Không dùng `-ffast-math` khi floating contract chưa cho phép.

**Ví dụ/oracle.** Hai command O0/O2 phải tạo stdout byte-for-byte giống `OK n=1000 checksum=7BD0A72C`; `diff -u` exit `0`.

**Best practice.** **Rule:** record full command/tool version. **Rationale/cơ chế:** option set điều khiển passes, code generation và debug/profile fidelity; thiếu command/version khiến binary và evidence không thể tái tạo hay quy kết. **Positive:** distinct `/tmp/b08_o0`, `/tmp/b08_o2`. **Negative:** overwrite binary và không biết output thuộc flags nào.

**Failure/troubleshooting.** Không tái lập → flags/compiler drift → capture `gcc --version` và command → rebuild clean → version-pin baseline.

#### OUT-B08-16 — Analyze code distribution in memory using Linker MAP file

**Mapping.** `OUT-B08-16` → `ADVC-H3SD` → `M00-FND-08`: dùng MAP để truy vết section/symbol placement của đúng binary, không suy diễn runtime speed.

**Định nghĩa/ranh giới.** GNU ld MAP ghi input sections, addresses, sizes và symbol resolution của một link. Nó không chứng minh cache hit, peak RSS hoặc source correctness.

**Vai trò/quyết định.** Dùng để tìm code/data growth, duplicate objects, unexpected section/symbol; giữ MAP cùng binary/hash/flags.

**Cơ chế.** Linker layout các input sections theo script/default; `-Wl,-Map,file` xuất report. Optimization/inlining có thể làm symbol biến mất.

**Khi dùng/không dùng/trade-off.** Dùng khi size/layout là constraint; không đọc MAP cũ sau rebuild. Report lớn cần filter nhưng phải giữ bản gốc.

**Ví dụ/oracle.** Link O2 với `-Wl,-Map,/tmp/b08.map`; oracle: link exit `0`, `/tmp/b08.map` nonempty, executable vẫn in `7BD0A72C`. Không đặt ngưỡng address/size portable.

**Best practice.** **Rule:** bind MAP to build ID/hash. **Rationale/cơ chế:** linker sinh MAP từ đúng set objects/options tại một lần link; build khác có section/symbol placement khác nên evidence ghép chéo dẫn tới quyết định sai. **Positive:** lưu compiler command + SHA-256. **Negative:** ghép MAP O0 với timing O2.

**Failure/troubleshooting.** Không thấy `mix_value` → có thể inline/DCE → xem optimization/disassembly → dùng profile build khi cần symbol → không kết luận source bị mất.

#### OUT-B08-17 — Profile the code

**Mapping.** `OUT-B08-17` → `ADVC-H3SD` → `M00-FND-08`: xác định hotspot bằng profile của workload đại diện và giới hạn claim theo host/tool.

**Định nghĩa/ranh giới.** Profiling thu sample/call counts/time/counters. `gprof -pg` cần instrumented build và normal exit; `perf` phụ thuộc kernel/permission/PMU. Report `0.00s` ở workload ngắn không có nghĩa code “miễn phí”.

**Vai trò/quyết định.** Chọn tool theo câu hỏi: call distribution, samples hay counters. Nếu PMU bị từ chối, ghi `BLOCKED-PMU`, không sửa sysctl/sudo.

**Cơ chế.** Instrumentation hoặc sampling quy attribution; overhead/resolution làm lệch measurement. Checksum giữ workload không bị bỏ.

**Khi dùng/không dùng/trade-off.** Profile sau correctness; không dùng một microbenchmark làm claim production. Instrumented result dùng để tìm hotspot, không làm release binary mặc định.

**Ví dụ/oracle.** Build `-pg -O0`, chạy `1000000`, `gprof` report phải nonempty và nhắc `mix_value`/`compute_checksum`; stdout vẫn có fixed checksum của binary hiện tại. Không yêu cầu phần trăm/time tối thiểu.

**Best practice.** **Rule:** profile cùng fixed workload, lưu raw report và decision. **Rationale/cơ chế:** sampling/instrumentation phân bổ cost theo lần chạy cụ thể và chịu noise/resolution; raw report + workload nối observation với decision có giới hạn. **Positive:** “giữ code vì không có hotspot đủ lớn”. **Negative:** tuyên bố 2× từ một lần chạy hoặc khi timer hiển thị 0.00.

**Failure/troubleshooting.** Không có `gmon.out` → build thiếu `-pg` hoặc abnormal exit/wrong cwd → kiểm compile+link và run → tạo profile tách biệt → tự động assert report nonempty, không assert speedup.

## 4. Quy trình kiểm chứng chuẩn

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O0 -g \
  assets/b08_optimization_demo.c -o /tmp/b08_o0
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  assets/b08_optimization_demo.c -Wl,-Map,/tmp/b08.map -o /tmp/b08_o2
/tmp/b08_o0 1000 >/tmp/b08_o0.out
/tmp/b08_o2 1000 >/tmp/b08_o2.out
diff -u /tmp/b08_o0.out /tmp/b08_o2.out
test -s /tmp/b08.map
```

Chỉ khi các lệnh trên pass mới đo. Không dùng sanitizer binary để benchmark; chạy sanitizer riêng cho safety evidence.

## 5. Thuật ngữ

| Thuật ngữ | Nghĩa |
| --- | --- |
| baseline/candidate | Build trước/sau thay đổi được so trong cùng contract |
| observable behavior | I/O, volatile/access và hiệu ứng được language model coi quan sát được |
| hotspot | Vùng chiếm phần đáng kể theo profile cụ thể |
| live range/spill | Khoảng value cần tồn tại / lưu tạm ra memory khi thiếu register |
| MAP file | Báo cáo layout/resolution do linker sinh |
| profile | Bằng chứng phân bố theo workload/tool, không phải universal truth |

## 6. Self-check Quiz (5 câu)

1. **O2 chạy nhanh hơn một lần đã đủ kết luận chưa?**  
   **Đáp án:** Chưa; cần output invariant, repetitions/distribution, environment fingerprint và workload đại diện.
2. **MAP file chứng minh điều gì?**  
   **Đáp án:** Layout/section/symbol của đúng link; không chứng minh runtime speed hay RSS.
3. **Vì sao O0/O2 khác checksum là blocker?**  
   **Đáp án:** Observable behavior đã đổi; có thể có UB/uninitialized/order defect, nên performance comparison không còn cùng chương trình contract.
4. **Loop unroll factor 4 cần test count nào?**  
   **Đáp án:** Ít nhất `0,1,3,4,5` và một count lớn không chia hết để kiểm zero/remainder paths.
5. **Khi `perf` bị permission denied nên làm gì?**  
   **Đáp án:** Lưu exact diagnostic và ghi `BLOCKED-PMU`; không sudo hay đổi system-wide setting. Có thể dùng evidence khác nhưng không giả PASS cho counter chưa chạy.

## 7. Nguồn tham khảo và provenance phần bổ sung

- `SRC-USER-CREF`: outline đã được người dùng phê duyệt.
- [GCC 11.4 manuals](https://gcc.gnu.org/onlinedocs/gcc-11.4.0/) (`SRC-GCC11`).
- [GNU ld 2.38](https://sourceware.org/binutils/docs-2.38/ld/) và [Binutils 2.38](https://sourceware.org/binutils/docs-2.38/binutils/) (`SRC-LD238`, `SRC-BINUTILS238`).
- [GNU gprof](https://sourceware.org/binutils/docs-2.38/gprof/) (`SRC-GPROF238`).
- [Linux 5.15 documentation](https://docs.kernel.org/5.15/index.html) (`SRC-LINUX-PERF515`).
- [LLVM/Clang 14 documentation](https://releases.llvm.org/14.0.0/tools/clang/docs/index.html) (`SRC-LLVM14`).

**[BỔ SUNG — nguồn: `SRC-USER-CREF`]** Workload/checksum là synthetic training fixture. Mọi performance conclusion phải được đo lại trên môi trường delivery thật.
