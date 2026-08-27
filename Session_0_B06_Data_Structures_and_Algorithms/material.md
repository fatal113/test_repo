# Session B06: Data Structures and Algorithms — Học liệu cốt lõi

## 🎯 Learning Outcomes

> **Learning Outcome:** ADVC-H1SD — Thiết kế, hiện thực và kiểm chứng mô-đun C17 dùng cấu trúc dữ liệu có ownership/invariant rõ và thuật toán phù hợp workload.  
> **Increment:** M00-FND-06 — executable C17 minh họa array, list, tree, hash, stack, queue cùng `qsort`/`bsearch`, có oracle xác định.

## Kiến thức tiên quyết, mental map và ranh giới

Học viên cần biết `struct`, mảng, pointer/lifetime, dynamic allocation, function pointer/callback, vòng lặp và cách đọc Big-O cơ bản. Build chuẩn: GCC 11.4, `-std=c17 -Wall -Wextra -Wpedantic -Werror -O2`; code lõi chỉ dùng ISO C17.

```text
workload + invariant + ownership
              |
              v
  chọn representation/ADT ------> chọn operation/algorithm
              |                              |
              +---------- đo/kiểm chứng <----+
                    correctness trước, cost sau
```

Ranh giới: complexity trong Unit dùng mô hình phân tích thuật toán thông dụng (RAM/comparison model), không phải cam kết timing của ISO C hay deadline real-time. ISO C17 quy định interface và kết quả sắp xếp/tìm kiếm của `qsort`/`bsearch`, nhưng không quy định `qsort` phải dùng quicksort, ổn định hay có một độ phức tạp cụ thể. Không có MCU, HAL, MMIO, RTOS, ISR, flashing hoặc giả định phần cứng.

## Checklist Content Outlines

- [x] 1 Data structure
- [x] 1.1 Data structure Fundamental
- [x] 1.1.1 Think, how, classify data structure
- [x] 1.1.2 Primitive/non-primitive data
- [x] 1.1.3 Design aspect
- [x] 1.2 Example
- [x] 1.2.1 How to ogranize structure in C
- [x] 1.2.2 Data orgainize in array
- [x] 1.2.3 Data orgainize in linked list
- [x] 1.2.4 Data orgainize with hierachy model
- [x] 1.2.5 Hashing
- [x] 2 Algorithm
- [x] 2.1 Collections and Collection operation
- [x] 2.2 Analyzing an Algorithm
- [x] 2.3 Non-primitive data structures
- [x] 2.3.1 Arrays
- [x] 2.3.2 Linked Lists
- [x] 2.3.3 Binary Tree
- [x] 2.3.4 General Tree
- [x] 2.3.5 Heaps
- [x] 2.3.6 Queues
- [x] 2.3.7 Stacks
- [x] 2.4 Sorting Algorithms
- [x] 2.5 Searching Algorithms

## 3. Nội dung lý thuyết cốt lõi

### 1 Data structure

Data structure là quyết định biểu diễn dữ liệu cùng invariant và operation, không chỉ là một khai báo `struct`.

### 1.1 Data structure Fundamental

#### OUT-B06-03 1.1.1 Think, how, classify data structure

**Mapping:** `OUT-B06-03` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Phân loại theo hình dạng (linear/hierarchical/graph), kích thước (fixed/dynamic), quan hệ lưu trữ (contiguous/linked) và access pattern (sequence/key/priority). Đây là công cụ suy luận, không phải taxonomy bắt buộc của ISO C.

**Vai trò/quyết định.** Bắt đầu từ operation chủ đạo, giới hạn `n`, ownership và yêu cầu thứ tự; không chọn cấu trúc chỉ vì tên quen.

**Cơ chế.** Representation quyết định đường đi tới phần tử: offset cho array, link cho list, path cho tree, hash-to-slot cho table.

**Khi dùng/không dùng/trade-off.** Phân loại giúp shortlist lựa chọn; không dùng nhãn thay benchmark hoặc proof. Cùng dữ liệu có thể cần nhiều index, đổi lại tốn bộ nhớ và đồng bộ invariant.

**Ví dụ/oracle riêng.** Workload `append=1000, lookup_by_key=100000, preserve_order=no` được phân loại `dynamic + keyed`; quyết định hash table. Oracle review chính xác: `class=dynamic,keyed; primary=lookup; candidate=hash-table`.

**Best practice.** Rule: ghi workload trước representation → rationale: tối ưu đúng operation → positive: chọn hash cho lookup key dày → negative: chọn list vì “linh hoạt”, rồi lookup tuyến tính.

**Failure chain.** Dấu hiệu lookup chậm khi `n` tăng → nguyên nhân phân loại bỏ sót access pattern → chẩn đoán đếm operation/n → sửa chọn/index lại → phòng ngừa workload matrix trong design review.

#### OUT-B06-04 1.1.2 Primitive/non-primitive data

**Mapping:** `OUT-B06-04` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Trong taxonomy sư phạm, primitive là scalar nền như integer/floating/pointer; non-primitive/composite gồm array, structure, union và ADT tạo từ chúng. “Primitive/non-primitive” không phải cặp thuật ngữ normative của ISO C; phải nói rõ kiểu C thật.

**Vai trò/quyết định.** Chọn kiểu phần tử đủ miền giá trị và representation composite giữ invariant, padding/alignment và ownership rõ.

**Cơ chế.** `record_t { int key; int payload; }` gom hai scalar thành record; array/list/tree tổ chức nhiều record bằng quan hệ khác nhau.

**Khi dùng/không dùng/trade-off.** Composite tăng tính kết dính nhưng copy cả struct có chi phí và pointer member không tự deep-copy.

**Ví dụ/oracle riêng.** Với `record_t r={.key=7,.payload=70};`, oracle `key=7 payload=70 size_at_least=8`; chỉ kiểm `sizeof r >= 2*sizeof(int)`, không hard-code padding.

**Best practice.** Rule: dùng fixed-width type chỉ khi contract cần đúng width và kiểm availability → rationale: miền/serialization rõ → positive: `int` cho key nội bộ → negative: giả định mọi `int` đúng 32 bit trong file format.

**Failure chain.** Dấu hiệu binary record lệch giữa platform → nguyên nhân serialize raw struct/padding → chẩn đoán so field/`sizeof` → sửa encode từng trường → phòng ngừa tách in-memory type khỏi wire format.

#### OUT-B06-05 1.1.3 Design aspect

**Mapping:** `OUT-B06-05` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Design aspect gồm invariant, ownership/lifetime, capacity, error policy, ordering/equality, iterator invalidation và cost model. API đẹp nhưng thiếu invariant chưa phải ADT an toàn.

**Vai trò/quyết định.** Chốt ai sở hữu node/buffer, khi nào operation thất bại, duplicate key xử lý thế nào và view nào bị invalidated.

**Cơ chế.** Mỗi mutator kiểm precondition, duy trì invariant và công bố postcondition; verifier/test quan sát trạng thái công khai.

**Khi dùng/không dùng/trade-off.** Invariant mạnh dễ audit nhưng có thể tốn kiểm tra; release build có thể bỏ assert nội bộ, không bỏ validation input public.

**Ví dụ/oracle riêng.** Stack capacity `3` có contract `0<=size<=3`; push thứ tư trả false và không đổi state. Oracle: `push4=rejected size=3 top=30`.

**Best practice.** Rule: viết invariant cạnh type/API → rationale: mọi operation cùng duy trì một contract → positive: queue ghi `count<=capacity` → negative: dùng sentinel mơ hồ và không phân biệt full/empty.

**Failure chain.** Dấu hiệu state hỏng sau lỗi → nguyên nhân mutator thay đổi một phần rồi fail → chẩn đoán snapshot trước/sau → sửa validate trước hoặc rollback → phòng ngừa failure-atomic test.

### 1.2 Example

#### OUT-B06-07 1.2.1 How to ogranize structure in C

**Mapping:** `OUT-B06-07` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Tổ chức structure trong C là định nghĩa `struct`, API operation và invariant; `typedef` chỉ đặt alias, không tạo encapsulation tự động.

**Vai trò/quyết định.** Chọn public-by-value cho record đơn giản hoặc opaque handle khi cần che representation/ownership.

**Cơ chế.** Designated initializer gắn field theo tên; hàm `record_compare` áp ordering nhất quán cho sort/search.

**Khi dùng/không dùng/trade-off.** Public struct thuận tiện và zero boilerplate nhưng khóa ABI/layout; opaque type linh hoạt hơn, đổi lại cần lifecycle API.

**Ví dụ/oracle riêng.** `record_t r={.key=12,.payload=120}; r.payload+=1;` cho oracle `record=12:121`.

**Best practice.** Rule: thao tác duy trì invariant qua hàm khi type phức tạp → rationale: một điểm kiểm tra → positive: `queue_push` kiểm full → negative: caller sửa trực tiếp `count`.

**Failure chain.** Dấu hiệu field mâu thuẫn → nguyên nhân mutation rải rác → chẩn đoán tìm write vào field → sửa gom qua API → phòng ngừa hạn chế exposure representation.

#### OUT-B06-08 1.2.2 Data orgainize in array

**Mapping:** `OUT-B06-08` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Array là dãy phần tử cùng kiểu, contiguous theo quy tắc C, kích thước cố định cho array object; dynamic array là ADT quản lý một allocation có size/capacity.

**Vai trò/quyết định.** Chọn array khi cần duyệt tuần tự/locality và random access; quyết định sorted hay insertion order.

**Cơ chế.** Phần tử `i` là `a[i]`; chèn giữa cần dịch phần tử, còn sort cho phép binary search.

**Khi dùng/không dùng/trade-off.** Tốt cho dense data; không tốt khi liên tục chèn/xóa giữa tập lớn và phải giữ pointer ổn định.

**Ví dụ/oracle riêng.** Mảng `{9,2,6}` sau sort tăng là `{2,6,9}`; oracle `array[0]=2 array[2]=9 count=3`.

**Best practice.** Rule: luôn mang length cùng pointer → rationale: C không lưu length khi array decay → positive: `(data,count)` → negative: loop tới khi gặp `0` dù `0` là dữ liệu hợp lệ.

**Failure chain.** Dấu hiệu out-of-bounds → nguyên nhân length sai → chẩn đoán kiểm `i<count` → sửa truyền count chuẩn → phòng ngừa API slice `(ptr,len)`.

#### OUT-B06-09 1.2.3 Data orgainize in linked list

**Mapping:** `OUT-B06-09` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Linked list nối node bằng pointer; singly list chỉ có `next`. Contiguous storage không được bảo đảm và random access vẫn tuyến tính.

**Vai trò/quyết định.** Chọn khi cần insert/remove qua node đã biết và địa chỉ node ổn định; chốt owner của từng node và head.

**Cơ chế.** Prepend đặt `new->next=head`, rồi commit `head=new`; traversal đi từng link tới `NULL`.

**Khi dùng/không dùng/trade-off.** Insert đầu O(1) trong model; đổi lại mỗi node có overhead, allocation và locality kém. Không chọn cho lookup index dày.

**Ví dụ/oracle riêng.** Prepend lần lượt `1,3,5` tạo thứ tự `5,3,1`; oracle `list.count=3 sum=9 head=5`.

**Best practice.** Rule: xác định một owner chain và destroy mọi node → rationale: tránh leak/double-free → positive: loop lưu `next` trước `free` → negative: `free(node); node=node->next` đọc object đã free.

**Failure chain.** Dấu hiệu loop vô hạn → nguyên nhân cycle vô ý → chẩn đoán visited bound/Floyd → sửa link sai → phòng ngừa test acyclic invariant sau mutation.

#### OUT-B06-10 1.2.4 Data orgainize with hierachy model

**Mapping:** `OUT-B06-10` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Hierarchy biểu diễn quan hệ parent–child; tree liên thông, không cycle và có một root trong mô hình rooted tree. Binary tree giới hạn hai child; general tree không.

**Vai trò/quyết định.** Chọn child representation: fixed left/right, first-child/next-sibling, hay dynamic child array; chốt ownership subtree.

**Cơ chế.** Traversal preorder/inorder/postorder biến hierarchy thành sequence; depth/height phụ thuộc path dài nhất theo convention đã nêu.

**Khi dùng/không dùng/trade-off.** Phù hợp AST/menu/org chart; không ép dữ liệu nhiều-parent thành tree—graph phù hợp hơn.

**Ví dụ/oracle riêng.** Root `A` có children `B,C`, `B` có `D`; preorder oracle `A,B,D,C`, node count `4`, height theo số node `3`.

**Best practice.** Rule: công bố convention height và child order → rationale: tránh oracle mơ hồ → positive: “leaf height=1” → negative: test mong `2` nhưng code đếm edge.

**Failure chain.** Dấu hiệu recursion không dừng → nguyên nhân cycle/shared child sai contract → chẩn đoán mark visited → sửa parent/link → phòng ngừa validate acyclic/single-owner trước traversal.

#### OUT-B06-11 1.2.5 Hashing

**Mapping:** `OUT-B06-11` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Hashing ánh key sang bucket; collision là bình thường và cần chaining/open addressing. Expected O(1) chỉ dưới giả định distribution/load phù hợp, không phải worst-case hay lời hứa ISO C.

**Vai trò/quyết định.** Chọn hash/equality nhất quán, collision policy, load threshold, resize và duplicate-key semantics.

**Cơ chế.** Demo dùng `key % capacity` và linear probing tới slot trống/cùng key; lookup phải dùng cùng probe sequence.

**Khi dùng/không dùng/trade-off.** Tốt cho equality lookup, không tự cung cấp sorted traversal/range query; worst-case probing O(n).

**Ví dụ/oracle riêng.** Capacity `7`, keys `5,12,19` cùng home bucket `5`; key `19` nằm sau ba probe. Oracle `hash.key=19 value=190 probes=3`.

**Best practice.** Rule: giới hạn probe và quản lý load factor → rationale: tránh loop vô hạn/cluster → positive: fail sau `capacity` probes → negative: `while(true)` khi table full.

**Failure chain.** Dấu hiệu insert treo → nguyên nhân table full không bound → chẩn đoán đếm probe → sửa trả lỗi/resize → phòng ngừa full-table negative test.

### 2 Algorithm

#### OUT-B06-13 2.1 Collections and Collection operation

**Mapping:** `OUT-B06-13` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Collection là abstraction nhiều phần tử; operation thường có create/destroy, insert/remove, find, iterate, size. ADT contract độc lập representation nhưng C không có interface collection dựng sẵn.

**Vai trò/quyết định.** Xác định semantics của duplicate, missing, full, iteration order và invalidation trước khi code.

**Cơ chế.** Cùng operation `insert` có cơ chế khác trên array/list/hash; caller nên dựa status/postcondition, không dựa internal slot.

**Khi dùng/không dùng/trade-off.** API chung giúp thay representation nhưng generic `void *` giảm type safety; typed API rõ hơn cho Unit.

**Ví dụ/oracle riêng.** Collection set nhận `insert(7), insert(7)` theo replace semantics; oracle `size=1 find(7)=present`.

**Best practice.** Rule: status phải phân biệt success/missing/full → rationale: caller xử lý xác định → positive: enum result → negative: trả `NULL` vừa có nghĩa missing vừa là stored null.

**Failure chain.** Dấu hiệu caller coi full là success → nguyên nhân status mơ hồ → chẩn đoán contract/test branch → sửa typed status → phòng ngừa table oracle từng outcome.

#### OUT-B06-14 2.2 Analyzing an Algorithm

**Mapping:** `OUT-B06-14` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Phân tích tách correctness, time/space growth và case worst/average/amortized. Big-O là upper-growth bound theo mô hình/giả định, không phải số mili-giây hay benchmark.

**Vai trò/quyết định.** Chọn metric phản ánh workload, nêu `n`, operation cơ sở và assumptions; sau đó mới đo implementation.

**Cơ chế.** Một loop qua `n` phần tử có n lần kiểm tra chính; nested full loops thường quadratic; divide-and-conquer cần recurrence.

**Khi dùng/không dùng/trade-off.** Dùng growth analysis cho scale/architecture; benchmark cho constant/cache/toolchain. Không suy Big-O chỉ từ một lần đo.

**Ví dụ/oracle riêng.** Linear search key cuối trong 5 phần tử thực hiện đúng `5` comparisons; oracle `n=5 found=yes comparisons=5`, phân tích worst O(n).

**Best practice.** Rule: ghi rõ case và assumptions → rationale: “O(1)” hash có điều kiện → positive: “expected O(1), worst O(n)” → negative: tuyên bố luôn O(1).

**Failure chain.** Dấu hiệu dự báo scale sai → nguyên nhân trộn average/worst → chẩn đoán adversarial input → sửa model/case → phòng ngừa complexity note cạnh API.

### 2.3 Non-primitive data structures

#### OUT-B06-16 2.3.1 Arrays

**Mapping:** `OUT-B06-16` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Array ADT cung cấp ordered positions; C array object có số phần tử xác định khi tạo. Dynamic vector thêm allocation/growth ngoài bản thân array.

**Vai trò/quyết định.** Chốt capacity, size, bounds và invalidation khi move/realloc.

**Cơ chế.** Index truy cập offset theo element; xóa vị trí giữ order cần shift suffix.

**Khi dùng/không dùng/trade-off.** Random access O(1) trong model; insert đầu O(n). Pointer tới element có thể invalid sau realloc.

**Ví dụ/oracle riêng.** Xóa index `1` khỏi `{4,8,15,16}` và giữ order cho `{4,15,16}`; oracle `size=3 sequence=4,15,16 moves=2`.

**Best practice.** Rule: validate `index < size` trước access → rationale: tránh UB → positive: return false khi index `4` → negative: đọc `data[size]`.

**Failure chain.** Dấu hiệu phần tử lặp sau delete → nguyên nhân move count sai → chẩn đoán snapshot sequence → sửa dịch `size-index-1` phần tử → phòng ngừa test first/middle/last.

#### OUT-B06-17 2.3.2 Linked Lists

**Mapping:** `OUT-B06-17` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** List ADT giữ sequence bằng links; singly/doubly/circular là variants với invariant khác nhau.

**Vai trò/quyết định.** Chọn có tail/prev hay không dựa operation; quyết định node allocation và iterator invalidation.

**Cơ chế.** Remove-after rewires predecessor rồi giải phóng đúng node; không copy payload để giả vờ xóa nếu identity quan trọng.

**Khi dùng/không dùng/trade-off.** O(1) removal khi có predecessor/node phù hợp; tìm predecessor vẫn O(n). Doubly list tốn thêm pointer nhưng xóa node trực tiếp dễ hơn.

**Ví dụ/oracle riêng.** List `10→20→30`, remove node sau head; oracle `sequence=10,30 removed=20 size=2`.

**Best practice.** Rule: update links trước free và giữ `next` cần thiết → rationale: không dereference freed node → positive: `victim=head->next; head->next=victim->next; free(victim)` → negative: free trước khi đọc next.

**Failure chain.** Dấu hiệu lost tail/leak → nguyên nhân quên edge case → chẩn đoán test size 0/1/2 → sửa head/tail invariant → phòng ngừa mutation matrix.

#### OUT-B06-18 2.3.3 Binary Tree

**Mapping:** `OUT-B06-18` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Binary tree có tối đa hai child; binary search tree (BST) thêm ordering invariant. Binary tree không mặc nhiên balanced hay BST.

**Vai trò/quyết định.** Chọn duplicate policy, balance requirement và traversal; worst-case BST lệch có height n.

**Cơ chế.** BST insert so key rồi đi left/right; inorder của BST hợp lệ sinh sequence không giảm theo duplicate policy.

**Khi dùng/không dùng/trade-off.** BST hữu ích cho ordered traversal/range; unbalanced implementation đơn giản nhưng worst lookup O(n).

**Ví dụ/oracle riêng.** Insert `4,2,6`; oracle `tree.nodes=3 height=2 inorder=2,4,6` với leaf height `1`.

**Best practice.** Rule: không gọi mọi binary tree là BST → rationale: search pruning cần ordering invariant → positive: validate inorder → negative: dùng BST search trên arbitrary tree.

**Failure chain.** Dấu hiệu missing key hiện hữu → nguyên nhân invariant bị phá → chẩn đoán inorder + bounds → sửa insert/mutation → phòng ngừa chỉ mutate qua BST API.

#### OUT-B06-19 2.3.4 General Tree

**Mapping:** `OUT-B06-19` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** General rooted tree cho số child tùy ý; biểu diễn phổ biến là child array hoặc first-child/next-sibling. Nó khác graph khi mỗi non-root có đúng một parent và không cycle.

**Vai trò/quyết định.** Child array ưu tiên indexed child/locality; sibling links ưu tiên incremental insertion, với ownership phức tạp hơn.

**Cơ chế.** First-child/next-sibling biến mỗi node thành hai pointer nhưng traversal phải đi cả subtree child và sibling chain.

**Khi dùng/không dùng/trade-off.** Hợp menu/AST; graph phù hợp shared child/cycle. Recursion dễ đọc nhưng depth không tin cậy có thể tràn call stack implementation.

**Ví dụ/oracle riêng.** Tree `root` children `a,b,c`, `b` child `d`; breadth-first oracle `root,a,b,c,d count=5`.

**Best practice.** Rule: xác định owner của sibling/subtree → rationale: destroy đúng một lần → positive: root owns all descendants → negative: hai parent cùng free một child.

**Failure chain.** Dấu hiệu double-free → nguyên nhân shared child trái tree contract → chẩn đoán identity/parent map → sửa clone hoặc chuyển sang graph owner model → phòng ngừa parent invariant validation.

#### OUT-B06-20 2.3.5 Heaps

**Mapping:** `OUT-B06-20` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Binary heap là complete binary tree thường lưu trong array với heap-order; nó không phải vùng dynamic storage thường gọi “heap”.

**Vai trò/quyết định.** Chọn min/max heap, tie policy, capacity và có cần decrease-key/handle ổn định không.

**Cơ chế.** Với zero-based array, children `2i+1`, `2i+2`; push sift-up, pop root đổi với last rồi sift-down.

**Khi dùng/không dùng/trade-off.** Priority queue: peek O(1), push/pop O(log n) trong model; không hỗ trợ tìm arbitrary key nhanh hay sorted iteration trực tiếp.

**Ví dụ/oracle riêng.** Min-heap push `7,2,5`, rồi pop; oracle `pop=2 remaining_root=5 size=2`.

**Best practice.** Rule: kiểm overflow khi tính child index/capacity → rationale: `2*i+1` có thể wrap với size lớn → positive: chỉ tính khi `i <= (size-2)/2` → negative: arithmetic không guard.

**Failure chain.** Dấu hiệu pop không phải min → nguyên nhân sift dừng sớm → chẩn đoán kiểm `parent<=children` mọi index → sửa chọn child nhỏ hơn → phòng ngừa invariant test sau mỗi mutation.

#### OUT-B06-21 2.3.6 Queues

**Mapping:** `OUT-B06-21` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Queue là FIFO ADT; circular buffer là representation bounded bằng array. Nó khác priority queue vì thứ tự lấy theo arrival, không theo priority.

**Vai trò/quyết định.** Chọn bounded/dynamic, full policy (reject/drop/block—Unit dùng reject) và thread-safety ngoài phạm vi.

**Cơ chế.** `tail=(tail+1)%capacity`, `head` tương tự; `count` phân biệt full với empty khi indices trùng.

**Khi dùng/không dùng/trade-off.** Bounded circular queue không allocation sau init, deterministic capacity; đổi lại phải xử lý full.

**Ví dụ/oracle riêng.** Enqueue `8,13`, dequeue một; oracle `queue.pop=8 remaining=1 next=13`.

**Best practice.** Rule: công bố full policy và không overwrite im lặng → rationale: tránh mất dữ liệu → positive: return false giữ state → negative: advance tail và đè phần tử chưa đọc.

**Failure chain.** Dấu hiệu FIFO đảo/mất item → nguyên nhân head/tail wrap sai → chẩn đoán test qua boundary → sửa modulo/count update → phòng ngừa wrap-around oracle.

#### OUT-B06-22 2.3.7 Stacks

**Mapping:** `OUT-B06-22` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Stack là LIFO ADT với push/pop/peek; khác call stack implementation và không nhất thiết dùng dynamic storage.

**Vai trò/quyết định.** Chọn bounded/dynamic và underflow/overflow status; phù hợp undo, DFS, parsing.

**Cơ chế.** Array stack dùng `items[size++]` khi push và `items[--size]` khi pop sau validation.

**Khi dùng/không dùng/trade-off.** Bounded stack đơn giản, cache-friendly; dynamic stack linh hoạt nhưng allocation/failure phức tạp. Không dùng khi cần FIFO.

**Ví dụ/oracle riêng.** Push `2,4,6`, pop; oracle `stack.pop=6 remaining=2 next=4`.

**Best practice.** Rule: kiểm full/empty trước đổi `size` → rationale: tránh underflow/partial state → positive: false và giữ size → negative: `--size` khi zero.

**Failure chain.** Dấu hiệu index rất lớn sau pop empty → nguyên nhân unsigned underflow → chẩn đoán xem `size` → sửa guard trước decrement → phòng ngừa empty negative test.

#### OUT-B06-23 2.4 Sorting Algorithms

**Mapping:** `OUT-B06-23` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Sorting sắp phần tử theo ordering. Insertion/selection/bubble thường quadratic; merge sort thường O(n log n) và cần auxiliary storage; quicksort thường average O(n log n), worst O(n²); heapsort O(n log n) trong comparison model. Đây là thuộc tính thuật toán cụ thể, không được gán tự động cho hàm thư viện C `qsort`.

**Vai trò/quyết định.** Chọn theo n, stability, memory, worst-case và dữ liệu gần sorted; comparator phải nhất quán.

**Cơ chế.** `qsort(base,n,size,cmp)` hoán vị để comparator không báo phần tử sau nhỏ hơn phần tử trước. ISO C không bắt buộc implementation là quicksort, stable hay complexity nào.

**Khi dùng/không dùng/trade-off.** `qsort` portable, generic nhưng callback có overhead/type erasure và stability không được hứa. Custom stable sort cần khi equal-key order quan trọng.

**Ví dụ/oracle riêng.** Sort keys `{42,5,31,7,19}` bằng comparator quan hệ cho oracle `5,7,19,31,42`; equal-order không thuộc oracle.

**Best practice.** Rule: comparator trả `(a>b)-(a<b)`, không `a-b` → rationale: tránh signed overflow → positive: relational compare → negative: subtraction với `INT_MIN/INT_MAX`.

**Failure chain.** Dấu hiệu order thất thường → nguyên nhân comparator overflow/không nhất quán → chẩn đoán test extremes và antisymmetry → sửa relational comparator → phòng ngừa comparator property tests.

#### OUT-B06-24 2.5 Searching Algorithms

**Mapping:** `OUT-B06-24` · LO `ADVC-H1SD` · `M00-FND-06`.

**Định nghĩa/ranh giới.** Linear search không cần sort; binary search cần sorted range theo cùng ordering; hash lookup cần hash/equality; tree search cần invariant. `bsearch` trả một matching element hoặc `NULL`; với duplicate, phần tử matching nào được trả không nên bị giả định.

**Vai trò/quyết định.** Tính tổng cost: sort một lần rồi nhiều query có thể đáng giá; một query nhỏ thường linear đơn giản hơn.

**Cơ chế.** `bsearch(key,base,n,size,cmp)` dùng comparator nhất quán với sort. ISO C quy định contract/result, không cần giả định số comparison hay implementation cụ thể.

**Khi dùng/không dùng/trade-off.** Binary search cho array sorted; hash cho equality workload; tree cho ordered updates/ranges. Không gọi `bsearch` trên unsorted data.

**Ví dụ/oracle riêng.** Sau sort trên, tìm key `31` trả payload `310`; tìm `99` trả `NULL`. Oracle `found31=310 found99=none`.

**Best practice.** Rule: dùng cùng ordering cho `qsort` và `bsearch` → rationale: precondition sorted phải khớp query → positive: một comparator dùng lại → negative: sort tăng nhưng search comparator giảm.

**Failure chain.** Dấu hiệu key hiện hữu nhưng trả `NULL` → nguyên nhân range chưa sort/comparator lệch → chẩn đoán validate adjacent order → sửa sort và comparator → phòng ngừa coupled sort/search test.

## Ví dụ tích hợp và exact oracle

`assets/b06_dsa_demo.c` dùng đại diện array/list/BST/hash/stack/queue cùng `qsort`/`bsearch`. General tree và heap được phân tích ở leaf tương ứng nhưng không được nhồi vào executable; việc chọn representative subset giữ demo audit được mà vẫn cover quyết định cho mọi cấu trúc.

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  assets/b06_dsa_demo.c -o b06_dsa_demo
./b06_dsa_demo --self-test
```

Exit `0`, `stderr` rỗng, `stdout` chính xác:

```text
array.sorted=5,7,19,31,42
search.key=31 payload=310
list.count=3 sum=9
tree.nodes=3 height=2 inorder=2,4,6
hash.key=19 value=190 probes=3
stack.pop=6 remaining=2
queue.pop=8 remaining=1
self-test=PASS
```

`--negative` phải exit `2`, `stdout` rỗng, `stderr`:

```text
error: hash table full for key 99
```

## Lỗi thường gặp

| Dấu hiệu | Nguyên nhân | Chẩn đoán | Sửa | Phòng ngừa |
|---|---|---|---|---|
| `qsort` order sai | comparator overflow/inconsistent | extremes/property test | relational comparator | dùng lại comparator |
| `bsearch` miss | input chưa sorted/ordering khác | validate adjacent pairs | sort trước search | coupled test |
| Hash loop vô hạn | table full, không probe bound | đếm probes | dừng ở capacity | negative full-table |
| Tree traversal treo | cycle/shared child sai contract | visited set/bound | sửa ownership/link | acyclic invariant |
| Queue/stack corrupt | full/empty update sai | boundary trace | validate trước mutate | empty/full/wrap tests |
| Complexity claim quá mạnh | nhầm API với algorithm | đối chiếu standard/model | ghi rõ assumptions | tách contract khỏi cost model |

## Thuật ngữ

- **Invariant:** tính chất phải đúng trước/sau operation công khai.
- **ADT:** kiểu dữ liệu trừu tượng được định nghĩa bởi state/operations/contract.
- **Representation:** cách state được lưu bằng kiểu và object C cụ thể.
- **Stable sort:** các phần tử equal-key giữ thứ tự tương đối; ISO C không hứa `qsort` stable.
- **Load factor:** tỷ lệ slot đã dùng trong hash table.
- **Amortized cost:** cost trung bình trên một chuỗi operation theo proof/model, không phải average input tùy ý.
- **Oracle:** kết quả/điều kiện chính xác dùng để quyết định pass/fail.

## Tự kiểm tra — Quiz 5 câu

1. ISO C có yêu cầu `qsort` dùng quicksort hoặc chạy O(n log n) không?  
   **Đáp án:** Không. Chuẩn quy định interface và kết quả ordering; algorithm, stability và complexity là lựa chọn implementation.

2. Vì sao hash lookup thường nói expected O(1) chứ không phải luôn O(1)?  
   **Đáp án:** Collision, distribution và load factor có thể tạo probe/chain dài; worst-case có thể O(n).

3. Khi nào pointer tới phần tử dynamic array có thể mất hiệu lực?  
   **Đáp án:** Khi operation như `realloc` di chuyển allocation hoặc khi phần tử/allocation bị xóa; API phải công bố invalidation.

4. Binary tree và BST khác gì?  
   **Đáp án:** Binary tree chỉ giới hạn tối đa hai child; BST còn có ordering invariant cho phép loại bỏ nhánh khi search.

5. `bsearch` key trùng có bảo đảm trả phần tử trùng đầu tiên không?  
   **Đáp án:** Không. Chỉ dựa vào một matching element; nếu cần first/last duplicate phải viết boundary search có contract riêng.

## 7. Nguồn tham khảo và provenance phần bổ sung

- **ISO/IEC 9899:2018 — Programming languages — C**, ISO/IEC JTC 1/SC 22, Edition 4 (C17), 2018, truy cập 2026-08-22: https://www.iso.org/standard/74528.html
- **WG14 N2176 — C17 committee draft**, ISO/IEC JTC 1/SC 22/WG14, 2017-10-09, bản draft công khai không phải ấn bản ISO cuối, truy cập 2026-08-22: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf
- **SEI CERT C Coding Standard**, Carnegie Mellon University Software Engineering Institute, online work-in-progress snapshot, truy cập 2026-08-22: https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/
- **Dictionary of Algorithms and Data Structures**, U.S. National Institute of Standards and Technology, online reference, truy cập 2026-08-22: https://xlinux.nist.gov/dads/

Các input/oracle và complexity examples là dữ liệu đào tạo synthetic. Complexity được gắn với model/algorithm đã nêu; normative claim về C library chỉ dựa trên C17.
