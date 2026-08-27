# Session B05: Memory Management and Pointer Basics — Học liệu cốt lõi

## 🎯 Learning Outcomes

> **Learning Outcome:** ADVC-H1SD — Thiết kế, hiện thực và kiểm chứng mô-đun C17 dùng con trỏ và bộ nhớ động mà không làm lộ ownership hoặc gây lỗi bộ nhớ trong các ca kiểm thử đã cho.  
> **Increment:** M00-FND-05 — dynamic vector có ownership, checked allocation, pointer arithmetic hợp lệ và bằng chứng chạy lại được.

## Kiến thức tiên quyết, môi trường và ranh giới

Học viên cần biết khai báo biến, mảng, hàm, `sizeof`, điều kiện, vòng lặp và cách đọc `stdout`, `stderr`, exit code. Môi trường chuẩn là Ubuntu 22.04, GCC 11.4, glibc 2.35, GNU Binutils 2.38 và Valgrind 3.18.1; phần lõi phải biên dịch bằng ISO C17 với `-Wall -Wextra -Wpedantic -Werror`.

Mental map của Unit:

```text
object + storage duration -> lifetime -> pointer hợp lệ
                                  |
                                  v
                      ownership + checked allocation
                                  |
                                  v
                      dynamic vector kiểm chứng được

ISO C17: ngữ nghĩa object/lifetime/pointer  |  GNU/Linux: ELF/MAP là bằng chứng cục bộ
```

Ranh giới bắt buộc: ISO C không quy định một sơ đồ tiến trình phổ quát gồm “code/data/BSS/heap/stack”. Các tên đó có thể mô tả một implementation, nhưng không thay thế khái niệm storage duration và lifetime của C. ELF, linker map, `readelf` và Valgrind trong bài là quan sát riêng của GNU/Linux; địa chỉ hay section cụ thể không phải oracle portable. Unit không dùng MCU, linker script phần cứng, HAL, MMIO, RTOS, ISR hoặc flashing.

## Checklist Content Outlines

- [x] 1 Memory layout
- [x] 2 Variable and memory location
- [x] 3 Linker file and memory
- [x] 4 Pointer variable
- [x] 5 Assigning values to a pointer
- [x] 6 Memory allocation for a pointer
- [x] 7 Pointer arithmetic

## 3. Nội dung lý thuyết cốt lõi

### Các chủ đề theo syllabus

#### 1 Memory layout

**Mapping:** Outline `OUT-B05-01` · LO `ADVC-H1SD` · Increment `M00-FND-05`.

**Định nghĩa và ranh giới.** Trong mô hình trừu tượng ISO C, object là vùng lưu trữ có kiểu và giá trị; storage duration xác định khoảng thời gian lưu trữ tồn tại, còn lifetime xác định lúc object có thể được truy cập theo quy tắc của ngôn ngữ. Static, thread, automatic và allocated storage duration là các phạm trù của C17 (thread storage có điều kiện hỗ trợ). “Stack” và “heap” là cách triển khai thường gặp, không phải lời hứa của ISO C.

**Vai trò/quyết định.** Người thiết kế vector phải quyết định dữ liệu sống bao lâu và ai kết thúc lifetime; không được suy ownership từ vị trí địa chỉ hoặc tên section.

**Cơ chế.** Object automatic thường bắt đầu lifetime khi vào block và hết khi rời block; object static tồn tại suốt execution; vùng do `malloc` cấp tồn tại cho tới `free` thành công đối với vùng đó. Pointer không kéo dài lifetime của object mà nó trỏ tới.

**Khi dùng / không dùng / trade-off.** Dùng mô hình lifetime để chứng minh truy cập hợp lệ. Chỉ dùng sơ đồ tiến trình để giải thích một toolchain đã nêu rõ; sơ đồ dễ hình dung nhưng kém portable và có thể bị tối ưu/linker biến đổi.

**Ví dụ và oracle riêng.** `int answer = 42; int *view = &answer; printf("%d\n", *view);` trong cùng block phải in chính xác `42`. Trả `view` ra khỏi hàm chứa `answer` thì không có oracle giá trị hợp lệ vì lifetime đã kết thúc.

**Best practice.** Rule: mô tả contract bằng lifetime/ownership, không bằng “nằm trên stack/heap” → rationale: contract còn đúng trên implementation khác → positive: “caller giữ buffer tới khi hàm trả về” → negative: “địa chỉ thấp nên chắc còn sống”, có thể dẫn tới dangling pointer.

**Failure chain.** Dấu hiệu: giá trị lúc đúng lúc sai sau khi hàm trả về → nguyên nhân: dùng pointer tới object automatic đã hết lifetime → chẩn đoán: lần theo scope/lifetime, bật warning và Memcheck nếu phù hợp → sửa: copy giá trị hoặc cấp storage với lifetime đủ dài → phòng ngừa: ghi rõ owner và lifetime trong API.

#### 2 Variable and memory location

**Mapping:** Outline `OUT-B05-02` · LO `ADVC-H1SD` · Increment `M00-FND-05`.

**Định nghĩa và ranh giới.** Biến là identifier dùng để chỉ object; biểu thức `&object` tạo pointer tới object khi toán hạng cho phép. “Địa chỉ” là giá trị pointer có ngữ nghĩa theo C, không mặc nhiên là số nguyên tuyến tính có thể so sánh tùy ý. Giá trị của object và địa chỉ của object là hai thứ khác nhau.

**Vai trò/quyết định.** Khi truyền dữ liệu, chọn truyền giá trị nếu callee chỉ cần bản sao; truyền pointer khi cần quan sát/sửa cùng object hoặc tránh copy lớn, đồng thời phải công bố nullability và lifetime.

**Cơ chế.** `int sample = 27; int *p = &sample;` làm `p` trỏ tới `sample`; `*p` là lvalue chỉ cùng object. Gán qua `*p` thay đổi `sample`, còn gán lại `p` chỉ đổi object đích.

**Khi dùng / không dùng / trade-off.** Pointer cho phép chia sẻ/mutate nhưng tăng coupling và rủi ro alias/lifetime. Không truyền pointer khi bản sao nhỏ, bất biến và rõ ràng hơn.

**Ví dụ và oracle riêng.** Với `int sample = 27; int *p = &sample; *p += 5; printf("sample=%d view=%d\n", sample, *p);`, oracle là `sample=32 view=32`; địa chỉ cụ thể không thuộc oracle.

**Best practice.** Rule: kiểm chứng nội dung và quan hệ alias thay vì hard-code địa chỉ → rationale: ASLR, linker và lần chạy có thể đổi địa chỉ → positive: assert `p == &sample` rồi kiểm tra `*p` → negative: mong đợi `p == (int *)0x1234`, không portable và có thể invalid.

**Failure chain.** Dấu hiệu: test thất bại chỉ vì địa chỉ khác → nguyên nhân: đưa địa chỉ implementation-specific vào oracle → chẩn đoán: tìm format `%p`/hằng địa chỉ trong assertion → sửa: kiểm tra giá trị hoặc quan hệ pointer hợp lệ → phòng ngừa: loại địa chỉ tuyệt đối khỏi acceptance criteria.

#### 3 Linker file and memory

**Mapping:** Outline `OUT-B05-03` · LO `ADVC-H1SD` · Increment `M00-FND-05`.

**Định nghĩa và ranh giới.** GNU `ld` có thể sinh map file mô tả cách input sections/symbols được bố trí trong một executable ELF; `readelf` đọc metadata ELF. Đây là bằng chứng của artifact được build bằng GNU/Linux toolchain đã chốt, không phải định nghĩa lifetime, ownership hoặc hành vi portable của ISO C. Linker *file* trong outline được xử lý như map output; Unit không viết linker script cho phần cứng.

**Vai trò/quyết định.** Dùng map/ELF khi cần trả lời “toolchain này đã tạo artifact nào?”; dùng quy tắc C để trả lời “dereference này có hợp lệ không?”. Hai câu hỏi không thay thế nhau.

**Cơ chế.** Tùy chọn `-Wl,-Map=b05_memory_demo.map` chuyển yêu cầu cho linker; `test -s` chứng minh file tồn tại và không rỗng; `readelf -h` xác nhận ELF; `readelf -s` cho symbol table nếu symbol còn được giữ. Tối ưu hóa, LTO, stripping và linker script có thể thay đổi evidence.

**Khi dùng / không dùng / trade-off.** Dùng trong debug footprint/linking trên đúng baseline. Không dùng section `.bss`/`.data` để suy rằng một pointer còn hợp lệ. Map chi tiết nhưng phụ thuộc toolchain và không nên là functional oracle portable.

**Ví dụ và oracle riêng.** Build asset bằng `gcc ... -Wl,-Map=b05_memory_demo.map ...`; chạy `test -s b05_memory_demo.map && grep -q 'main' b05_memory_demo.map`. Oracle cục bộ là exit `0`; không kiểm tra offset/address cố định.

**Best practice.** Rule: gắn nhãn mọi bằng chứng MAP/ELF bằng compiler, linker, version và flags → rationale: layout thay đổi theo build → positive: “GNU ld 2.38, `-O2`, map có `main`” → negative: “C17 yêu cầu `main` ở địa chỉ X”, là kết luận sai phạm vi.

**Failure chain.** Dấu hiệu: map khác sau khi đổi `-O0` sang `-O2` nhưng code vẫn đúng → nguyên nhân: optimization/dead-section/linker placement → chẩn đoán: so flags, version và symbol visibility → sửa: điều chỉnh tiêu chí evidence, không sửa lifetime code vô cớ → phòng ngừa: lưu build manifest cùng map.

#### 4 Pointer variable

**Mapping:** Outline `OUT-B05-04` · LO `ADVC-H1SD` · Increment `M00-FND-05`.

**Định nghĩa và ranh giới.** Pointer variable là object có kiểu pointer, có thể giữ null pointer, pointer tới object/function phù hợp hoặc một số giá trị đặc biệt khác do phép toán hợp lệ tạo ra. Khai báo pointer không tự tạo object đích; `NULL` biểu thị không trỏ tới object và không được dereference.

**Vai trò/quyết định.** Mỗi pointer trong vector cần vai trò rõ: owning (`data`) hay non-owning view; nullable hay required; mutable hay pointer-to-const.

**Cơ chế.** `int *p` mô tả pointer tới `int`; `const int *view` không cho sửa `int` qua `view`; `int *const slot` không cho gán lại chính `slot`. Dereference chỉ hợp lệ khi pointer trỏ tới object còn lifetime và alignment/type access phù hợp.

**Khi dùng / không dùng / trade-off.** Dùng pointer cho cấu trúc động, optional value và sharing có contract. Không dùng pointer chỉ để “tăng hiệu năng” khi chưa đo; indirection có thể làm API khó hiểu và locality kém.

**Ví dụ và oracle riêng.** `int x = 7; int *p = &x; *p = 12; printf("x=%d\n", x);` có oracle `x=12`. `int *p; *p = 12;` không có hành vi xác định vì giá trị pointer indeterminate.

**Best practice.** Rule: khởi tạo pointer tại khai báo và dùng `const` cho view chỉ đọc → rationale: thu hẹp trạng thái bất hợp lệ → positive: `const int *view = &x;` → negative: pointer chưa khởi tạo rồi dereference.

**Failure chain.** Dấu hiệu: crash hoặc corruption tại `*p` → nguyên nhân: null/uninitialized/dangling pointer → chẩn đoán: kiểm tra nguồn gốc, lifetime, owner và chạy Memcheck trên baseline → sửa contract/khởi tạo/giữ lifetime → phòng ngừa: invariant “pointer null iff size/capacity bằng 0” khi phù hợp.

#### 5 Assigning values to a pointer

**Mapping:** Outline `OUT-B05-05` · LO `ADVC-H1SD` · Increment `M00-FND-05`.

**Định nghĩa và ranh giới.** Gán cho pointer (`p = &a`) thay đổi đích mà `p` tham chiếu; gán qua pointer (`*p = value`) thay đổi object đích. Conversion phải tuân kiểu và qualifier; ép một số nguyên tùy ý thành pointer không tạo ra object hợp lệ để truy cập.

**Vai trò/quyết định.** Phân biệt rebind với mutation giúp review ownership: rebind owner có thể làm mất vùng cấp phát cũ, còn mutation tác động dữ liệu được chia sẻ.

**Cơ chế.** Giá trị từ `malloc` có thể gán cho object pointer phù hợp trong C mà không cần cast. Khi reassign owner, phải giữ đường dẫn tới allocation cũ cho tới khi `free` hoặc chuyển ownership có chủ ý.

**Khi dùng / không dùng / trade-off.** Rebind view là bình thường; rebind owning pointer chỉ sau khi đã giải quyết allocation cũ. Cast có thể cần ở boundary đặc thù nhưng không nên che cảnh báo kiểu.

**Ví dụ và oracle riêng.** `int a=4,b=9; int *selected=&a; selected=&b; printf("a=%d b=%d selected=%d\n",a,b,*selected);` in `a=4 b=9 selected=9`.

**Best practice.** Rule: không cast kết quả `malloc` trong C và không overwrite owner trước khi kiểm tra allocation mới → rationale: cast có thể che thiếu prototype; overwrite gây leak → positive: `int *next = realloc(data, bytes); if (next) data = next;` → negative: `data = realloc(data, bytes);` làm mất pointer cũ khi thất bại.

**Failure chain.** Dấu hiệu: leak sau lỗi cấp phát → nguyên nhân: gán trực tiếp kết quả `realloc` vào owner → chẩn đoán: xem nhánh `NULL`, dùng fault injection/Memcheck → sửa: dùng temporary pointer → phòng ngừa: chuẩn hóa checked-reallocation helper.

#### 6 Memory allocation for a pointer

**Mapping:** Outline `OUT-B05-06` · LO `ADVC-H1SD` · Increment `M00-FND-05`.

**Định nghĩa và ranh giới.** `malloc`/`realloc` tạo hoặc thay đổi allocated storage; `free` kết thúc quyền truy cập tới allocation. Pointer là handle, không phải vùng nhớ. Zero-size allocation có các khả năng được chuẩn cho phép và không nên dùng để suy ra một object phần tử.

**Vai trò/quyết định.** Vector cần owner duy nhất, capacity growth có overflow guard, failure atomicity và destructor đưa object về trạng thái rỗng xác định.

**Cơ chế.** Trước `count * sizeof *data`, kiểm tra `count <= SIZE_MAX / sizeof *data`. `realloc` thành công có thể đổi địa chỉ và làm pointer/view cũ mất hiệu lực; thất bại trả `NULL` và allocation cũ còn nguyên. `free(NULL)` hợp lệ.

**Khi dùng / không dùng / trade-off.** Dynamic storage phù hợp khi kích thước runtime hoặc lifetime vượt scope. Mảng có giới hạn cố định đơn giản/deterministic hơn. Tăng capacity theo cấp số nhân giảm số lần cấp phát nhưng giữ dư bộ nhớ.

**Ví dụ và oracle riêng.** Asset thêm `{3,-1,7,0,5}`, tăng capacity `0→4→8`, rồi destroy. Oracle chính xác là `size=5 capacity=8 sum=14 first=3 last=5`, sau đó `ownership=destroyed data=null size=0 capacity=0`.

**Best practice.** Rule: check overflow trước allocation, dùng temporary cho `realloc`, và có đúng một hàm destroy idempotent theo contract → rationale: tránh wrap, leak và double-free → positive: asset giữ `data` cũ nếu grow thất bại → negative: nhân size không kiểm tra rồi ghi `count` phần tử vào allocation quá nhỏ.

**Failure chain.** Dấu hiệu: heap corruption hoặc allocation nhỏ bất thường với count lớn → nguyên nhân: overflow phép nhân → chẩn đoán: test boundary `SIZE_MAX / sizeof(T) + 1`, Memcheck → sửa: guard trước phép nhân → phòng ngừa: helper checked-size và negative oracle bắt buộc.

#### 7 Pointer arithmetic

**Mapping:** Outline `OUT-B05-07` · LO `ADVC-H1SD` · Increment `M00-FND-05`.

**Định nghĩa và ranh giới.** C cho phép cộng/trừ số nguyên với pointer trong một array object (một object đơn lẻ được xem như array một phần tử cho quy tắc này) và tạo pointer one-past. Dereference one-past hoặc tạo pointer vượt miền được phép là sai; trừ/so thứ tự pointer không cùng array không cho kết quả portable.

**Vai trò/quyết định.** Duyệt vector bằng index hoặc `[begin,end)` phải giữ invariant `begin <= cursor <= end` trong cùng allocation và chỉ dereference khi `cursor < end`.

**Cơ chế.** `p + i` dịch theo đơn vị `sizeof *p`, không theo byte. `end - begin` cho số phần tử khi cả hai thuộc cùng array/one-past và kết quả biểu diễn được bởi `ptrdiff_t`.

**Khi dùng / không dùng / trade-off.** Range `[begin,end)` tạo loop gọn và không cần sentinel data. Index thường dễ audit overflow/bounds hơn; không dùng arithmetic để “đi” giữa hai object độc lập.

**Ví dụ và oracle riêng.** Với `int a[]={2,7,1};` và vòng `for (const int *p=a; p<a+3; ++p) sum+=*p;`, oracle là `sum=10 distance=3`; `a+3` được so/trừ nhưng không dereference.

**Best practice.** Rule: ràng buộc pointer arithmetic trong một allocation và dùng điều kiện `< end` trước dereference → rationale: one-past chỉ là mốc → positive: vòng lặp trên → negative: `*(a + 3)` đọc ngoài mảng.

**Failure chain.** Dấu hiệu: invalid read ở cuối loop → nguyên nhân: dùng `<= end` hoặc dereference one-past → chẩn đoán: in index/distance, kiểm tra loop invariant, Memcheck → sửa thành `< end` → phòng ngừa: review range theo mô hình half-open.

## Ví dụ tích hợp và oracle

Asset `assets/b05_memory_demo.c` kết hợp owner vector, checked `realloc`, range traversal và destructor. Lệnh chuẩn:

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  assets/b05_memory_demo.c -o b05_memory_demo
./b05_memory_demo --self-test
```

`stdout` phải đúng từng byte (ngoài newline cuối):

```text
size=5 capacity=8 sum=14 first=3 last=5
ownership=destroyed data=null size=0 capacity=0
self-test=PASS
```

Exit code là `0`, `stderr` rỗng. Với `--negative`, exit code là `2`, `stdout` rỗng và `stderr` chính xác:

```text
error: allocation size overflow
```

## Lỗi thường gặp

| Dấu hiệu | Nguyên nhân gốc | Chẩn đoán | Sửa | Phòng ngừa |
|---|---|---|---|---|
| Leak khi `realloc` thất bại | overwrite owner | xem nhánh `NULL` | temporary pointer | checked-grow helper |
| Use-after-free | view sống lâu hơn owner | lần theo lifetime/Memcheck | bỏ view hoặc kéo dài owner | ghi ownership contract |
| Invalid read cuối mảng | dereference one-past | audit `[begin,end)` | dùng `< end` | boundary test |
| Test địa chỉ không ổn định | oracle phụ thuộc ASLR/linker | tìm `%p`/hằng địa chỉ | assert quan hệ/giá trị | tách portable oracle khỏi ELF evidence |
| Map được dùng để “chứng minh” lifetime | trộn hai tầng mô hình | đối chiếu nguồn quy tắc | dùng ISO C cho lifetime | nhãn mọi evidence cục bộ |

## Thuật ngữ

- **Object:** vùng lưu trữ có nội dung biểu diễn một giá trị theo kiểu C.
- **Storage duration:** khoảng thời gian lưu trữ của object được dành riêng.
- **Lifetime:** khoảng execution mà object tồn tại và có thể được truy cập theo chuẩn.
- **Ownership:** quy ước API xác định thành phần chịu trách nhiệm giải phóng; đây là discipline thiết kế, không phải từ khóa C.
- **Alias/view:** pointer không sở hữu cùng tham chiếu tới object.
- **One-past:** pointer ngay sau phần tử cuối, dùng làm mốc nhưng không được dereference.
- **ELF/MAP evidence:** metadata của artifact GNU/Linux cụ thể, không phải ngữ nghĩa portable.

## Tự kiểm tra — Quiz 5 câu

1. ISO C17 có bắt buộc object automatic phải nằm trên hardware/process stack không?  
   **Đáp án:** Không. C17 quy định storage duration/lifetime; “stack” là lựa chọn implementation. Vì vậy oracle portable không dùng địa chỉ hay section để chứng minh lifetime.

2. Vì sao `data = realloc(data, bytes);` nguy hiểm?  
   **Đáp án:** Khi thất bại, `realloc` trả `NULL` còn allocation cũ vẫn tồn tại; gán trực tiếp làm mất handle và gây leak. Dùng temporary rồi commit khi khác `NULL`.

3. Pointer `end = array + count` có hợp lệ không, và có được đọc `*end` không?  
   **Đáp án:** Có thể tạo/so/trừ `end` theo quy tắc cùng array; không được dereference one-past.

4. Map file chứng minh được điều gì trong Unit này?  
   **Đáp án:** Nó chứng minh cách GNU linker cụ thể bố trí artifact với đúng version/flags đã ghi. Nó không chứng minh ownership hoặc lifetime theo ISO C.

5. Sau `free(p)`, gán `p = NULL` có làm các alias khác an toàn không?  
   **Đáp án:** Không. Nó chỉ đổi `p`; mọi alias tới allocation cũ vẫn dangling. Contract phải quản lý toàn bộ view trước khi owner giải phóng.

## 7. Nguồn tham khảo và provenance phần bổ sung

- **ISO/IEC 9899:2018 — Programming languages — C**, ISO/IEC JTC 1/SC 22, Edition 4 (C17), 2018, truy cập 2026-08-22: https://www.iso.org/standard/74528.html
- **WG14 N2176 — C17 committee draft**, ISO/IEC JTC 1/SC 22/WG14, 2017-10-09, bản draft công khai không phải ấn bản ISO cuối, truy cập 2026-08-22: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf
- **SEI CERT C Coding Standard**, Carnegie Mellon University Software Engineering Institute, online work-in-progress snapshot, truy cập 2026-08-22: https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/
- **The GNU C Library Reference Manual**, GNU Project, glibc 2.35, truy cập 2026-08-22: https://sourceware.org/glibc/manual/2.35/pdf/libc.pdf
- **Using LD, GNU Binutils 2.38** và **GNU Binary Utilities 2.38**, GNU Project/FSF, truy cập 2026-08-22: https://sourceware.org/binutils/docs-2.38/ld/ và https://sourceware.org/binutils/docs-2.38/binutils/
- **Valgrind User Manual — Memcheck**, Valgrind Developers, baseline 3.18.1, truy cập 2026-08-22: https://valgrind.org/docs/manual/mc-manual.html

Các ví dụ, input và oracle của Unit là dữ liệu đào tạo synthetic. Các nhận định về ELF/MAP/Valgrind chỉ áp dụng cho baseline GNU/Linux được nêu; nội dung portable dựa trên C17.
