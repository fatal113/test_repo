# Session B06 — Worked Example: Chọn và kiểm chứng cấu trúc dữ liệu theo workload

## 🎯 Learning Outcomes liên quan

- `ADVC-H1SD` — chọn và kiểm chứng cấu trúc dữ liệu/thuật toán C17 theo workload, invariant và ownership.

## CASE-B06-01 · Ticket và tiêu chí thành công

Đội record-tool cần một executable C17 nhỏ để chứng minh readiness trước khi xây ADT generic: sort/search records, duyệt list, kiểm tra hierarchy, lookup hash và mô phỏng LIFO/FIFO. Mục tiêu không phải nhồi mọi implementation vào một file; case phải chứng minh cách chọn representation và oracle cho toàn bộ outline, còn asset dùng một tập đại diện dễ audit.

- **LO:** `ADVC-H1SD`.
- **Increment:** `M00-FND-06`.
- **Artifact:** `assets/b06_dsa_demo.c`.
- **Success:** strict build exit `0`, zero warnings; happy/negative oracle đúng từng stream và exit code; không allocation lỗi/leak trong ca đã cho.
- **Ranh giới:** ISO C17 portable; không hardware/embedded. Complexity là model phân tích, không phải timing guarantee.

## Mapping CASE-B06-01 tới toàn bộ leaf

| Outline ref | Quyết định/implementation/evidence |
|---|---|
| `OUT-B06-03` Think, how, classify data structure | Ma trận workload: sequence→array/list, hierarchy→tree, equality key→hash, LIFO/FIFO→stack/queue. |
| `OUT-B06-04` Primitive/non-primitive data | Scalar `int` tạo `record_t`, node, table slot và ADT composite; không serialize raw layout. |
| `OUT-B06-05` Design aspect | Invariant, capacity, ownership, duplicate/full policy và exact status/oracle được công bố. |
| `OUT-B06-07` How to ogranize structure in C | `struct` + hàm operation; designated initializer và callback comparator. |
| `OUT-B06-08` Data orgainize in array | Record array contiguous, sort rồi search. |
| `OUT-B06-09` Data orgainize in linked list | Ba node singly-linked, traversal count/sum. |
| `OUT-B06-10` Data orgainize with hierachy model | BST ba node đại diện rooted hierarchy; ownership/traversal có bound. |
| `OUT-B06-11` Hashing | Open addressing, linear probing, collision và full-table failure. |
| `OUT-B06-13` Collections and Collection operation | Insert/find/traverse/push/pop/enqueue/dequeue có status và postcondition. |
| `OUT-B06-14` Analyzing an Algorithm | Case phân biệt worst/expected/model; oracle không dùng timing. |
| `OUT-B06-16` Arrays | Random access/sorted traversal; count đi cùng pointer. |
| `OUT-B06-17` Linked Lists | Ba object node automatic nối thành singly-linked list; traversal count/sum trong lifetime của self-test. |
| `OUT-B06-18` Binary Tree | BST ba node dựng sẵn; kiểm inorder/node count/height với convention leaf height `1`. |
| `OUT-B06-19` General Tree | Design decision first-child/next-sibling được so với BST representative; không cần thêm implementation vào demo. |
| `OUT-B06-20` Heaps | Design decision min-heap cho priority workload được so với FIFO queue; không đánh đồng memory heap. |
| `OUT-B06-21` Queues | Bounded circular FIFO, dequeue `8` trước `13`. |
| `OUT-B06-22` Stacks | Bounded LIFO, pop `6` sau ba push. |
| `OUT-B06-23` Sorting Algorithms | `qsort` + comparator chống overflow; không khẳng định algorithm/stability/complexity của library. |
| `OUT-B06-24` Searching Algorithms | `bsearch` trên đúng sorted range với cùng comparator. |

## Input, trạng thái ban đầu và constraint

### Dataset và operation

- Records `(key,payload)`: `(42,420),(5,50),(31,310),(7,70),(19,190)`.
- Search key: `31`.
- List gồm ba node automatic có values `3,1,5`, nối theo thứ tự đó và có tổng `9`.
- BST keys: `4,2,6`; convention height tính theo số node, leaf height `1`.
- Hash capacity `7`; keys `5,12,19` cùng home bucket `5`, nên key `19` cần ba probe.
- Stack pushes `2,4,6`; queue enqueues `8,13`.
- Negative mode điền một hash table nhỏ rồi insert key `99`; operation phải từ chối sau tối đa `capacity` probes.

### Invariant bắt buộc

- Mọi `(pointer,count)` chỉ đọc trong `[0,count)`.
- Comparator không dùng phép trừ có thể overflow và cùng được dùng cho `qsort`/`bsearch`.
- List/BST node trong demo có automatic storage duration, không gọi `free`; liên kết chỉ được dùng khi các object còn lifetime.
- Hash probe bounded; stack/queue validate full/empty trước mutation.
- Functional oracle không chứa địa chỉ, thời gian hoặc số comparison nội bộ của C library.

## Phân tích lựa chọn và trade-off

Record array phù hợp vì dataset dense và case cần sort một lần rồi search; array cho locality và random access, đổi lại chèn giữa tốn dịch chuyển. List minh họa node identity/insert đầu nhưng lookup vẫn tuyến tính và có allocation overhead. BST cung cấp ordered hierarchy; bản không cân bằng đơn giản nhưng worst-case có thể thành chain. Hash table tối ưu equality lookup theo expected model, không hỗ trợ sorted traversal và phải xử lý collision/full.

Stack và queue cùng có bounded array nhưng semantics khác: stack là LIFO, queue là FIFO với wrap-around. Nếu workload là “luôn lấy priority nhỏ nhất”, min-heap mới là lựa chọn phù hợp: peek min O(1), push/pop O(log n) trong binary-heap model. Nếu dữ liệu có nhiều child như menu/AST, general tree (child array hoặc first-child/next-sibling) phù hợp hơn binary tree/BST. Hai quyết định này cover `OUT-B06-19/20` mà không làm asset đại diện phình to.

`qsort` được chọn vì là interface ISO C generic. Không được suy từ tên rằng implementation dùng quicksort, stable hoặc O(n log n). Khi stability là acceptance criterion, cần thuật toán/API khác có contract stable rõ ràng. `bsearch` chỉ dùng sau sort bằng comparator nhất quán; với duplicate, case không giả định phần tử matching đầu tiên.

## Đọc implementation theo luồng

1. `record_compare` tạo ordering bằng hai phép so sánh, tránh `left->key - right->key`.
2. Record array được `qsort`; `bsearch` nhận key record cùng comparator.
3. List/BST được dựng từ object automatic có lifetime tới hết self-test; traversal thật tạo count/sum/inorder oracle.
4. Hash insert/lookup trả probe count xác định cho dataset đã chọn.
5. Stack/queue trả status; self-test chỉ in PASS sau khi mọi invariant và expected value đúng.
6. Negative mode kiểm table full mà không loop vô hạn hay overwrite slot.

## Strict build và happy oracle

Từ thư mục Unit:

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  assets/b06_dsa_demo.c -o b06_dsa_demo
./b06_dsa_demo --self-test
```

Build phải exit `0` với `stdout`/`stderr` rỗng. Self-test phải exit `0`, `stderr` rỗng và `stdout` chính xác:

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

Ý nghĩa oracle:

- Sort tạo ordering tăng theo key; oracle không nói algorithm/stability.
- Search tìm đúng payload `310`; oracle không nói số comparison hoặc vị trí khi duplicate.
- List, BST, hash, stack và queue mỗi cấu trúc chứng minh operation/invariant riêng, không chỉ đổi tên cùng một container.

## Negative oracle

```sh
set +e
./b06_dsa_demo --negative >negative.out 2>negative.err
rc=$?
set -e
printf 'exit=%d\n' "$rc"
```

Oracle:

- `rc` chính xác `2`.
- `negative.out` là file rỗng.
- `negative.err` chính xác:

```text
error: hash table full for key 99
```

Nếu chương trình treo, overwrite item hoặc trả success thì full-table invariant đã hỏng. Đối số không phải `--self-test`/`--negative` phải trả usage exit `64`, không tự chọn happy path.

## Complexity evidence đúng phạm vi

| Operation trong case | Phân tích theo model | Caveat |
|---|---|---|
| Array index | O(1) | Không phải giới hạn nanosecond của ISO C. |
| List traversal | O(n) | Allocator/cache làm constant khác. |
| BST lookup | O(h), worst O(n) khi lệch | Demo không cân bằng. |
| Hash lookup | expected O(1), worst O(n) | Phụ thuộc hash/load/input; demo có collision cố ý. |
| Stack/queue bounded operation | O(1) | Khi không có concurrency/blocking. |
| Binary heap push/pop (design alternative) | O(log n) | Đây là binary-heap algorithm, không phải `malloc` heap. |
| `qsort` | Không claim complexity | ISO C không mandate algorithm/stability/cost. |
| `bsearch` | Contract cần sorted input | Không đưa count comparison implementation-specific vào oracle. |

Đo benchmark có thể bổ sung sau, nhưng một vài timing không chứng minh Big-O và timing không thuộc acceptance oracle của Unit.

## Failure modes, chẩn đoán và phòng ngừa

| Dấu hiệu | Nguyên nhân gốc | Chẩn đoán/evidence | Sửa | Phòng ngừa |
|---|---|---|---|---|
| Sort sai ở key cực trị | comparator dùng phép trừ overflow | test `INT_MIN/INT_MAX`, antisymmetry | relational comparator | property test comparator |
| `bsearch` trả `NULL` cho key có mặt | chưa sort hoặc comparator khác | validate adjacent ordering | sort và dùng chung comparator | coupled test |
| Hash treo khi full | probe không bounded | counter vượt capacity | fail/resize sau capacity | full-table negative oracle |
| List leak/use-after-free | owner/destroy chain sai | Memcheck + trace node | lưu `next` trước free | single-owner contract |
| BST inorder không tăng | insert phá ordering | inorder + lower/upper bounds | sửa branch/duplicate policy | mutation chỉ qua API |
| Queue lấy `13` trước `8` | cập nhật head/tail sai | trace `{head,tail,count}` | sửa circular invariant | wrap/full/empty tests |
| Stack size wrap thành số lớn | pop khi empty | kiểm state trước decrement | trả false trước mutate | empty negative test |
| Thiết kế general tree double-free | shared child nhưng contract tree-owner | parent/identity map | clone hoặc graph ownership | validate one-parent/acyclic |
| Gọi queue là heap priority | chọn sai abstraction | đối chiếu required ordering | dùng min-heap | workload matrix trước code |

## Bài học chuyển giao

Không có cấu trúc “tốt nhất” độc lập workload. Một artifact reviewable phải nối được: input/operation → representation → invariant/ownership → algorithm → exact oracle. Complexity claim luôn mang model/assumption; C library contract không được mở rộng bằng suy đoán từ tên hàm.

## Practice Time — không chấm điểm, không có lời giải

Tạo bản sao riêng của asset và thay dữ liệu bằng:

- Records: `(18,180),(3,30),(11,110),(7,70)`; search key `11`.
- List cuối cùng có sequence `2,4,6,8`.
- BST insert `5,1,9`, height convention giữ nguyên.
- Hash capacity `7`, insert `1,8,15`, lookup `15` (ba keys collision ở home bucket `1`).
- Stack push `1,3,5`; queue enqueue `4,6`.
- Negative mode điền table rồi insert key `42`.

Không copy lời giải từ nơi khác; giữ API/status/invariant, comparator relational, probe bounded và cleanup. Happy oracle mới:

```text
array.sorted=3,7,11,18
search.key=11 payload=110
list.count=4 sum=20
tree.nodes=3 height=2 inorder=1,5,9
hash.key=15 value=150 probes=3
stack.pop=5 remaining=2
queue.pop=4 remaining=1
self-test=PASS
```

Happy exit `0`, `stderr` rỗng. Negative exit `2`, `stdout` rỗng và:

```text
error: hash table full for key 42
```

Nộp source bản sao, strict build command, output của hai mode và một design note ngắn: vì sao general tree phù hợp hơn BST cho menu nhiều child, và vì sao min-heap phù hợp hơn FIFO queue cho scheduler theo priority. Không cần hiện thực general tree/heap; phần này cố ý không kèm lời giải.

## Provenance của các case

- **ISO/IEC 9899:2018 — Programming languages — C**, ISO/IEC JTC 1/SC 22, Edition 4 (C17), 2018, truy cập 2026-08-22: https://www.iso.org/standard/74528.html
- **WG14 N2176 — C17 committee draft**, ISO/IEC JTC 1/SC 22/WG14, 2017-10-09, bản draft công khai, truy cập 2026-08-22: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf
- **SEI CERT C Coding Standard**, Carnegie Mellon University Software Engineering Institute, online work-in-progress snapshot, truy cập 2026-08-22: https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/
- **Dictionary of Algorithms and Data Structures**, U.S. National Institute of Standards and Technology, online reference, truy cập 2026-08-22: https://xlinux.nist.gov/dads/

Case/data/oracle là synthetic; complexity và lựa chọn representation là phân tích được gắn giả định, còn claim về `qsort`/`bsearch` giới hạn ở contract ISO C17.
