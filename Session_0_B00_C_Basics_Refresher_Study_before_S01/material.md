# B00 — C Basics Refresher (Study before S01) — Học liệu ôn tập C17

> **Khóa học:** [NFP] Advanced C Programming · **Tính chất:** học liệu tự học tùy chọn trước S01 · **Thời lượng gợi ý:** 180 phút · **Chuẩn ngôn ngữ:** ISO C17
>
> **Trạng thái phạm vi:** **[BỔ SUNG — nguồn: `SRC-USER-BASIC`, yêu cầu trực tiếp đã được người dùng phê duyệt ngày 2026-08-21]**. Bốn chủ đề dưới đây được suy ra từ nhóm prerequisite “C cơ bản” của S01 để học viên tự kiểm tra nền tảng. Phần này hỗ trợ LO `ADVC-H1SD`, không tạo LO chính thức, Assignment hay nội dung đánh giá mới trong syllabus.

## 🎯 Learning Outcomes

Sau phần tự học, học viên có thể đọc, viết, biên dịch và kiểm tra một chương trình C17 nhỏ dùng kiểu dữ liệu, control flow, hàm, mảng, chuỗi và con trỏ cơ bản; đồng thời phân biệt lỗi compile, lỗi logic và lỗi runtime trước khi vào nội dung con trỏ nâng cao của S01.

- **LO được hỗ trợ:** `ADVC-H1SD` — vai trò prerequisite support; bằng chứng ở mức ôn tập là asset C17 build warning-free và qua hai oracle đầu vào.
- **Readiness increment:** `M00-RDY` — tạo một analyzer sample bounded cùng build/run/debug evidence trước khi nhận ticket M01 của dự án mô phỏng MDB Edge Diagnostics Gateway. Increment này là tùy chọn, không chấm điểm và không thay đổi các ASM hoặc graded milestone hiện có.
- **Dữ liệu:** toàn bộ sample là synthetic, không chứa dữ liệu thiết bị hoặc khách hàng thật.

## Kiến thức tiên quyết và môi trường

- Biết tạo và lưu một file văn bản có đuôi `.c`.
- Biết mở terminal trong Ubuntu 22.04, WSL2 hoặc môi trường Linux tương đương của khóa.
- Baseline của khóa: GCC 11.4, GDB 12.1, C17. Phần ôn tập chỉ cần GCC và GDB; không yêu cầu CMake.
- Warning profile dùng xuyên suốt: `-std=c17 -Wall -Wextra -Wpedantic -Werror -O0 -g`.

Lệnh dựng asset:

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O0 -g \
  assets/basic_refresher_demo.c -o /tmp/basic_refresher_demo
```

Build đạt khi compiler exit `0` và không có warning. Cờ `-O0 -g` giữ luồng và thông tin debug dễ quan sát; đây là profile học tập, không phải quyết định tối ưu hóa production.

## Mental map

Một chương trình C nhỏ đi qua chuỗi sau:

`source → compiler kiểm cú pháp/kiểu → executable → input → control flow → hàm → mảng/chuỗi → con trỏ tới dữ liệu → output/exit code → debugger khi oracle sai`

Kiểu dữ liệu quy định cách diễn giải giá trị; control flow chọn đường chạy; hàm tạo contract; scope giới hạn tên và lifetime; mảng giữ các phần tử liên tiếp; chuỗi C là mảng `char` kết thúc bằng `\0`; con trỏ cho phép tham chiếu object có địa chỉ hợp lệ. Compiler và debugger cung cấp bằng chứng khác nhau: compiler bắt một lớp lỗi trước khi chạy, còn GDB giúp quan sát trạng thái của một lần chạy cụ thể.

## 3. Nội dung lý thuyết cốt lõi

- [x] Cấu trúc chương trình C17, kiểu dữ liệu, toán tử và luồng điều khiển
- [x] Hàm, prototype, phạm vi và vòng đời biến cơ bản
- [x] Mảng, chuỗi ký tự và thao tác dữ liệu có giới hạn
- [x] Con trỏ cơ bản, const-correctness và quy trình biên dịch/gỡ lỗi với GCC

### Nhóm kiến thức nền tảng trước S01

#### OUT-B00-01 — Cấu trúc chương trình C17, kiểu dữ liệu, toán tử và luồng điều khiển

**Mapping:** `OUT-B00-01` → `ADVC-H1SD` (prerequisite support) → `M00-RDY`: đọc và kiểm soát luồng xử lý sample synthetic trước M01.

**Định nghĩa và ranh giới.** Một translation unit C thường gồm chỉ thị tiền xử lý, khai báo kiểu/hàm và định nghĩa hàm; chương trình hosted bắt đầu ở `main`. Kiểu như `int`, `double`, `size_t` xác định miền giá trị và phép toán hợp lệ. Toán tử tạo biểu thức; `if`/`else`, `switch`, `for` và `while` chọn hoặc lặp đường chạy. Khối `{...}` nhóm câu lệnh. Chủ đề này chỉ ôn scalar type, conversion có chủ đích và control flow tuần tự; chưa đi vào bit-field, atomics, macro generic hay undefined behavior nâng cao.

**Vấn đề, vai trò và quyết định.** Khi xử lý danh sách sample, developer phải quyết định kiểu nào biểu diễn số lượng, tổng và trung bình; điều kiện nào từ chối input; vòng lặp dừng ở đâu; exit code nào phân biệt thành công với thất bại. Chọn kiểu hoặc điều kiện sai có thể làm output trông hợp lý nhưng vi phạm oracle. Quyết định đúng là dùng `size_t` cho count/index, giới hạn sample trước khi chuyển từ `long` sang `int`, và tách nhánh lỗi khỏi nhánh in kết quả.

**Cơ chế và mental model.** Compiler kiểm biểu thức theo type; promotion/conversion có thể đổi miền giá trị. `=` gán, còn `==` so sánh. `&&` và `||` đánh giá từ trái sang phải và short-circuit, nên điều kiện bảo vệ phải đứng trước phép dùng dữ liệu. Vòng `for (index = 0U; index < count; ++index)` duyệt đúng các chỉ số hợp lệ `0..count-1`. `return` từ `main` truyền status cho shell; stdout dành cho kết quả bình thường, stderr dành cho chẩn đoán.

**Khi dùng, khi không dùng và trade-off.** Dùng `if` khi điều kiện không phải một tập giá trị rời rạc đơn giản; dùng `switch` khi dispatch trên tập hằng số nguyên rõ ràng; dùng `for` khi init/condition/update tạo một invariant duyệt; dùng `while` khi số vòng phụ thuộc dữ liệu. Không dùng một biểu thức quá nhiều side effect để rút ngắn code vì thứ tự đánh giá dễ bị hiểu sai. Không chọn `double` cho mọi dữ liệu chỉ để tránh nghĩ về integer division; hãy giữ count/sample ở integer và cast rõ tại phép tính trung bình.

**Ví dụ nghề nghiệp có oracle riêng.** Reviewer cần tóm tắt bốn sample synthetic `18,21,24,30`, mỗi sample phải nằm trong `[-50,150]`, tối đa tám phần tử. Developer dùng `size_t` cho count/index, `int` cho sample và total đã được giới hạn, `double` cho mean; vòng lặp cập nhật min/max/total. Artifact là [basic_refresher_demo.c](assets/basic_refresher_demo.c). Kết quả mong đợi là `count=4`, `min=18`, `max=30`, `mean=23.25`. Oracle: build không warning, process exit `0`, stdout chính xác `OK count=4 min=18 max=30 mean=23.25`.

##### Ví dụ code cụ thể — kiểu dữ liệu và vòng lặp có biên

Lưu block sau thành `/tmp/b00_flow.c`. Ví dụ cố ý dùng một bộ dữ liệu nhỏ khác asset tích hợp để chỉ làm rõ type, conversion và loop invariant:

```c
#include <stddef.h>
#include <stdio.h>

int main(void)
{
    const int samples[] = {12, 18, 27};
    const size_t count = sizeof samples / sizeof samples[0];
    int total = 0;

    for (size_t index = 0U; index < count; ++index) {
        total += samples[index];
    }

    const double mean = (double)total / (double)count;
    printf("count=%zu total=%d mean=%.2f\n", count, total, mean);
    return 0;
}
```

Chạy `gcc -std=c17 -Wall -Wextra -Wpedantic -Werror /tmp/b00_flow.c -o /tmp/b00_flow && /tmp/b00_flow`. Oracle là exit `0`, không có warning và stdout đúng `count=3 total=57 mean=19.00`. `index < count` giữ truy cập trong mảng; hai cast sang `double` ngăn integer division làm mất phần thập phân.

**Best practices.**

- **Rule:** chọn kiểu theo ý nghĩa và miền giá trị. **Rationale:** type là một phần của contract và quyết định conversion/format. **Positive:** count/index dùng `size_t`, `printf` dùng `%zu`. **Negative/hậu quả:** dùng `int` cho size nhưng in `%zu` tạo format mismatch và có thể cho output sai.
- **Rule:** đặt điều kiện bảo vệ trước thao tác phụ thuộc. **Rationale:** short-circuit ngăn đọc dữ liệu chưa hợp lệ. **Positive:** kiểm parse/range rồi mới cast và lưu vào mảng. **Negative/hậu quả:** cast trước khi kiểm range có thể làm mất thông tin và chấp nhận giá trị sai.
- **Rule:** vòng lặp phải có invariant và biên trên rõ. **Rationale:** `index < count` khớp miền chỉ số của mảng. **Positive:** duyệt `0` đến `count - 1`. **Negative/hậu quả:** dùng `index <= count` đọc one-past và có thể crash hoặc làm sai kết quả.

**Failure và troubleshooting.** Dấu hiệu compiler báo “comparison between signed and unsigned” thường do index/count khác họ kiểu: xem declaration và warning, đổi index/count sang `size_t` thay vì cast che warning. Dấu hiệu mean là `23.00` thay vì `23.25` do integer division: đặt breakpoint trước phép chia hoặc in total/count, rồi cast ít nhất một toán hạng sang `double`; phòng tránh bằng test có kết quả không nguyên. Dấu hiệu vòng lặp chạy thừa một lần do điều kiện `<=`: quan sát `index` tại lần dừng, sửa thành `<`, và luôn gắn loop condition với invariant `index là chỉ số hợp lệ`.

#### OUT-B00-02 — Hàm, prototype, phạm vi và vòng đời biến cơ bản

**Mapping:** `OUT-B00-02` → `ADVC-H1SD` (prerequisite support) → `M00-RDY`: tách parse và summarize thành contract nhỏ trước khi học callback/API ở S01.

**Định nghĩa và ranh giới.** Hàm là đơn vị có prototype, tham số, return type và body. Prototype cho compiler biết contract kiểu tại điểm gọi. Parameter và biến khai báo trong block có block scope; tên `static` function ở file scope có internal linkage, chỉ dùng trong translation unit đó. Scope trả lời “tên dùng được ở đâu”; storage duration/lifetime trả lời “object tồn tại bao lâu” — hai khái niệm liên quan nhưng không đồng nhất. Chủ đề này chưa đi vào function pointer hoặc linkage nhiều translation unit; đó là nội dung S01/S05.

**Vấn đề, vai trò và quyết định.** Một `main` làm cả parse, validation, tính toán và in output rất khó test từng lỗi. Developer cần quyết định input/output contract của mỗi hàm, nhánh nào trả failure, và tên nào cần lộ ra ngoài file. Trong asset, `parse_samples` nhận text, buffer/capacity và output count; `summarize_samples` nhận mảng/count cùng ba output pointer cho minimum, maximum và total; cả hai trả `1` hoặc `0`. Các helper là `static` để không tạo API ngoài ý muốn.

**Cơ chế và mental model.** C truyền đối số theo giá trị. Muốn callee cập nhật `sample_count` của caller, caller truyền địa chỉ `&sample_count`, callee ghi qua `*out_count`. Mỗi lần gọi tạo automatic objects riêng; các biến local `minimum`, `maximum`, `total` hết lifetime khi hàm trả về, nên hàm chép từng giá trị sang object của caller qua output pointer trước `return`. `return` sớm trên precondition failure giữ happy path ngắn và ngăn caller dùng kết quả chưa hoàn tất.

**Khi dùng, khi không dùng và trade-off.** Tách hàm khi có contract có thể đặt tên, tái sử dụng hoặc kiểm riêng; giữ inline logic rất nhỏ khi tách ra chỉ làm che luồng. Dùng output pointer khi cần vừa trả status vừa trả dữ liệu, nhưng phải nêu null policy và output-on-failure. Không trả địa chỉ của biến automatic local; object đó hết lifetime sau `return`. `static` helper giảm namespace và coupling, đổi lại không thể gọi trực tiếp từ translation unit test khác nếu không test qua public behavior.

**Ví dụ nghề nghiệp có oracle riêng.** Input `18,xx,24` đến parser của tool preflight. Constraint là không cập nhật count thành công một phần và không gọi summarizer với dữ liệu lỗi. `parse_samples` đặt `*out_count = 0U`, phát hiện `end == cursor` ở `xx`, trả `0`; `main` in lỗi và dừng. Artifact là cùng executable nhưng oracle riêng cho contract hàm: exit `2`, stdout rỗng, stderr chính xác `ERROR invalid sample list`. Bằng chứng này chứng minh error path được truyền qua return value thay vì caller dùng output chưa hợp lệ.

##### Ví dụ code cụ thể — prototype và output parameter

Lưu block sau thành `/tmp/b00_function.c`:

```c
#include <stddef.h>
#include <stdio.h>

static int total_of(const int *values, size_t count, int *out_total)
{
    if (values == NULL || count == 0U || out_total == NULL) {
        return 0;
    }

    int total = 0;
    for (size_t index = 0U; index < count; ++index) {
        total += values[index];
    }
    *out_total = total;
    return 1;
}

int main(void)
{
    const int values[] = {20, 21, 22};
    int total = 0;
    const int ok = total_of(values, 3U, &total);
    printf("status=%s total=%d\n", ok != 0 ? "ok" : "error", total);
    return ok != 0 ? 0 : 1;
}
```

Chạy `gcc -std=c17 -Wall -Wextra -Wpedantic -Werror /tmp/b00_function.c -o /tmp/b00_function && /tmp/b00_function`. Oracle là exit `0` và stdout đúng `status=ok total=63`. `values` là input-only nhờ `const`; `out_total` là output có precondition rõ; `static` giữ helper trong translation unit này.

**Best practices.**

- **Rule:** prototype phải diễn tả đúng input-only và output. **Rationale:** `const` giúp compiler/reviewer thấy hàm không sửa input qua pointer. **Positive:** `summarize_samples(const int *values, size_t count, int *out_minimum, int *out_maximum, int *out_total)`. **Negative/hậu quả:** bỏ `const` làm contract rộng hơn cần thiết và cho phép mutation input ngoài ý muốn.
- **Rule:** validate mọi pointer/count precondition trước khi dereference. **Rationale:** C không tự kiểm null hoặc bounds. **Positive:** từ chối `values == NULL`, `count == 0U` hoặc bất kỳ output pointer nào null. **Negative/hậu quả:** gán `*out_total` trước kiểm null có thể gây runtime fault.
- **Rule:** giữ helper private bằng `static` khi không phải public API. **Rationale:** internal linkage tránh collision và giảm surface cần duy trì. **Positive:** `static int parse_samples(...)`. **Negative/hậu quả:** helper external không cần thiết có thể trùng symbol khi dự án lớn lên.
- **Rule:** không trả pointer tới automatic local. **Rationale:** lifetime kết thúc khi hàm trả về. **Positive:** chép scalar local qua output pointer hợp lệ của caller. **Negative/hậu quả:** trả `&total` tạo dangling pointer và use-after-return.

**Failure và troubleshooting.** Dấu hiệu linker báo `multiple definition` có thể do helper không `static` được định nghĩa ở nhiều file: xem symbol/file trong linker output, chuyển implementation private vào một file và dùng internal linkage. Dấu hiệu caller thấy count cũ do truyền `sample_count` thay vì `&sample_count`: compiler sẽ báo type mismatch nếu prototype đúng; sửa call và không cast để che lỗi. Dấu hiệu dữ liệu thay đổi sau hàm lỗi do mutation trước validation: đặt watchpoint lên output, xác định lần ghi đầu, chuyển validation lên trước và đặt output-on-error rõ; phòng tránh bằng invalid-input oracle.

#### OUT-B00-03 — Mảng, chuỗi ký tự và thao tác dữ liệu có giới hạn

**Mapping:** `OUT-B00-03` → `ADVC-H1SD` (prerequisite support) → `M00-RDY`: biểu diễn bounded synthetic samples và command text trước M01.

**Định nghĩa và ranh giới.** Mảng là dãy cố định các phần tử cùng kiểu nằm liên tiếp; với `T a[N]`, chỉ số hợp lệ là `0..N-1`. `sizeof a / sizeof a[0]` cho số phần tử chỉ tại nơi `a` còn là array object. Khi truyền vào hàm, parameter khai báo `T a[]` được điều chỉnh thành `T *`, nên hàm không biết capacity nếu caller không truyền riêng. Chuỗi C là mảng `char` có byte kết thúc `\0`; length không gồm terminator, còn capacity phải chứa cả terminator. Chủ đề này chỉ dùng mảng bounded và chuỗi command-line chỉ đọc; chưa dạy allocation động hoặc encoding Unicode.

**Vấn đề, vai trò và quyết định.** Parser phải chứa tối đa tám sample và biết input kết thúc ở đâu. Developer quyết định capacity cố định, policy cho danh sách rỗng/quá dài/dấu phẩy cuối, và không được suy ra capacity từ pointer. Asset dùng `int samples[MAX_SAMPLES]`, truyền cả `MAX_SAMPLES`, và duyệt text đến `\0`. Danh sách phần tử thứ chín hoặc `18,21,` bị từ chối thay vì ghi quá mảng hoặc tự bỏ token rỗng.

**Cơ chế và mental model.** `samples[index]` tương đương truy cập phần tử thứ `index` trong vùng liên tiếp, nhưng chỉ có nghĩa khi index hợp lệ. `argv[1]` là pointer tới chuỗi null-terminated do môi trường chạy cung cấp khi `argc >= 2`. `strtol` nhận con trỏ đầu vào và trả end pointer; `end == cursor` nghĩa là không đọc được chữ số. Parser kiểm `*end`: `\0` kết thúc chuỗi, `,` chuyển sang token kế, ký tự khác là lỗi. Mảng không tự mang length; `sample_count` mới là số phần tử đã khởi tạo.

**Khi dùng, khi không dùng và trade-off.** Mảng fixed-capacity phù hợp fixture nhỏ có giới hạn rõ và tránh allocation trong bài ôn tập. Không phù hợp input không giới hạn hoặc cần tăng trưởng; khi đó phải có bounded policy hoặc cấu trúc động được dạy sau. Dùng string library khi contract length/capacity được biết; không dùng `strlen` hay copy trên buffer chưa chứng minh có `\0`. Không dùng `sizeof(pointer) / sizeof(pointer[0])` trong callee để tìm số phần tử.

**Ví dụ nghề nghiệp có oracle riêng.** Tool nhận `-5,0,150`, ba sample synthetic nằm đúng biên cho phép. Input text null-terminated, output buffer capacity tám. Parser quyết định chấp nhận dấu âm, từ chối whitespace hoặc hậu tố không được toàn bộ token tiêu thụ, và trả count ba. Artifact là `samples` cùng `sample_count`; kết quả mong đợi `OK count=3 min=-5 max=150 mean=48.33`. Oracle riêng: exit `0` và stdout khớp; ca `1,2,3,4,5,6,7,8,9` phải exit `2` và không ghi phần tử thứ chín.

##### Ví dụ code cụ thể — sao chép chuỗi có capacity

Lưu block sau thành `/tmp/b00_array_string.c`:

```c
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static int set_label(char *destination, size_t capacity, const char *source)
{
    const size_t length = strlen(source);
    if (length + 1U > capacity) {
        return 0;
    }
    (void)memcpy(destination, source, length + 1U);
    return 1;
}

int main(void)
{
    char label[8];
    const int accepted = set_label(label, sizeof label, "READY");
    const int rejected = !set_label(label, sizeof label, "TOO-LONG");
    printf("label=%s rejected=%d\n", label, rejected);
    return accepted != 0 && rejected != 0 ? 0 : 1;
}
```

Chạy `gcc -std=c17 -Wall -Wextra -Wpedantic -Werror /tmp/b00_array_string.c -o /tmp/b00_array_string && /tmp/b00_array_string`. Oracle là exit `0` và stdout đúng `label=READY rejected=1`. Kiểm `length + 1U` tính cả `\0`; nhánh quá dài trả lỗi trước `memcpy`, nên buffer vẫn chứa chuỗi hợp lệ trước đó.

**Best practices.**

- **Rule:** luôn truyền length/capacity cùng pointer mảng qua biên hàm. **Rationale:** array parameter đã thành pointer và mất extent. **Positive:** `parse_samples(text, samples, MAX_SAMPLES, &count)`. **Negative/hậu quả:** callee dùng `sizeof values` chỉ lấy kích thước pointer, dẫn đến vòng lặp sai.
- **Rule:** phân biệt capacity với initialized count. **Rationale:** đọc phần tử chưa khởi tạo là lỗi dù vẫn nằm trong storage của mảng. **Positive:** summarizer chỉ duyệt `sample_count`. **Negative/hậu quả:** duyệt `MAX_SAMPLES` trộn dữ liệu rác vào min/mean.
- **Rule:** chuỗi phải có terminator trong vùng hợp lệ trước thao tác string. **Rationale:** hàm string tìm `\0`; thiếu terminator làm đọc vượt buffer. **Positive:** dùng `argv` hoặc buffer được cấp đủ `length + 1`. **Negative/hậu quả:** mảng `char name[3] = {'a','b','c'}` không phải chuỗi hợp lệ cho `%s`.
- **Rule:** reject input vượt capacity thay vì truncate im lặng. **Rationale:** truncation làm artifact khác dữ liệu caller tưởng đã gửi. **Positive:** phần tử thứ chín trả lỗi. **Negative/hậu quả:** bỏ sample cuối nhưng vẫn báo thành công tạo kết quả thống kê sai.

**Failure và troubleshooting.** Dấu hiệu output thay đổi giữa các lần chạy do duyệt phần tử chưa khởi tạo: quan sát `count` và `index` trong GDB, chỉ duyệt initialized count, khởi tạo nơi cần thiết. Dấu hiệu ASan/GDB cho thấy access ở `samples[8]` do kiểm capacity sau lần ghi: chuyển `count == capacity` lên trước `values[count] = ...`; phòng tránh bằng fixture chín phần tử. Dấu hiệu `%s` in tiếp dữ liệu lạ do thiếu `\0`: kiểm length và byte terminator, sửa allocation/copy contract; không chữa bằng cách in ít ký tự tùy ý nếu contract vẫn mơ hồ.

#### OUT-B00-04 — Con trỏ cơ bản, const-correctness và quy trình biên dịch/gỡ lỗi với GCC

**Mapping:** `OUT-B00-04` → `ADVC-H1SD` (prerequisite support) → `M00-RDY`: dùng địa chỉ, output parameter và evidence loop đúng trước pointer deep-dive của S01.

**Định nghĩa và ranh giới.** Con trỏ object có kiểu lưu địa chỉ của object phù hợp hoặc null; `&object` lấy địa chỉ, `*pointer` truy cập object được trỏ tới khi pointer hợp lệ. `pointer->member` là cách viết gọn của `(*pointer).member`. Pointer không tự mang ownership, lifetime, bounds hay nullability. Biên dịch chuyển source thành executable và có thể dừng ở lỗi cú pháp/kiểu/link; debugging quan sát một execution để tìm nơi actual state lệch expected state. Chủ đề này chỉ ôn address/dereference, null check và output pointer; pointer arithmetic nâng cao, pointer-to-pointer, function pointer, aliasing và ownership động thuộc các session sau.

**Vấn đề, vai trò và quyết định.** Hai helper cần ghi kết quả vào object của caller nhưng vẫn trả status. Developer quyết định khi nào pointer được phép null, object phải sống bao lâu, callee được đọc hay ghi, và evidence nào tách compile failure, invalid-input behavior và logic failure. Asset truyền `&sample_count`, `&minimum`, `&maximum`, `&total`; callee kiểm null trước `*out...`; compiler warning là gate, còn GDB được dùng khi oracle output/exit code không khớp.

**Cơ chế và mental model.** `sample_count` là object trong `main`; `&sample_count` trỏ tới object đó trong suốt lần gọi. Parameter `out_count` là bản sao của địa chỉ; `*out_count = count` thay đổi object gốc. `const int *values` cho phép đọc các phần tử nhưng không sửa chúng qua pointer đó. Quy trình debug có vòng lặp: tái hiện bằng input nhỏ → ghi expected/actual → build `-O0 -g` → breakpoint gần lần sai đầu tiên → xem arguments/locals/control flow → sửa nguyên nhân → rebuild sạch → chạy lại cả happy và invalid oracle.

**Khi dùng, khi không dùng và trade-off.** Dùng pointer khi cần tham chiếu object của caller, xử lý mảng hoặc biểu diễn optional/null theo contract. Không dereference trước null/lifetime/bounds check; không giữ địa chỉ của automatic local sau khi lifetime kết thúc. Dùng compiler trước vì type/warning evidence rẻ và xác định; dùng GDB khi executable chạy nhưng state/control flow sai hoặc crash. Debugger không chứng minh absence of memory bugs, và một lần chạy pass không thay thế boundary fixtures hay sanitizer ở S01.

**Ví dụ nghề nghiệp có oracle riêng.** Ca hợp lệ truyền địa chỉ buffer/count/minimum/maximum/total qua các helper; ca lỗi `18,xx,24` làm `strtol` không tiến tại `xx`. Developer build warning-free, chạy lại lỗi, đặt breakpoint tại `parse_samples`, quan sát `cursor`, `end`, `errno` và return path. Artifact là executable debug cùng command log. Expected: happy exit `0`/stdout đúng; invalid exit `2`/stderr đúng. Oracle tổng hợp:

```bash
/tmp/basic_refresher_demo '18,21,24,30'
# OK count=4 min=18 max=30 mean=23.25

/tmp/basic_refresher_demo '18,xx,24'
# stderr: ERROR invalid sample list
# exit code: 2
```

##### Ví dụ code cụ thể — dereference có precondition và quan sát bằng GDB

Lưu block sau thành `/tmp/b00_pointer.c`:

```c
#include <stdio.h>

static int scale_reading(const int *input, int *output)
{
    if (input == NULL || output == NULL) {
        return 0;
    }
    *output = *input * 2;
    return 1;
}

int main(void)
{
    const int raw = 21;
    int scaled = 0;
    if (!scale_reading(&raw, &scaled)) {
        return 2;
    }
    printf("scaled=%d\n", scaled);
    return 0;
}
```

Build và chạy bằng `gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O0 -g /tmp/b00_pointer.c -o /tmp/b00_pointer && /tmp/b00_pointer`; oracle là exit `0` và stdout `scaled=42`. Để nhìn pointee trước phép ghi, chạy `gdb -q -batch -ex 'break scale_reading' -ex run -ex 'print *input' -ex continue /tmp/b00_pointer`; bằng chứng cần có `$1 = 21`, rồi chương trình thoát bình thường. `const int *input` chặn ghi qua input pointer, còn null check đứng trước cả hai lần dereference.

**Best practices.**

- **Rule:** kiểm pointer và precondition trước lần dereference đầu tiên. **Rationale:** null/dangling/out-of-bounds access không được C tự chặn. **Positive:** helper trả `0` khi output pointer null. **Negative/hậu quả:** ghi `*out_count = 0U` trước kiểm null có thể fault.
- **Rule:** dùng `const` cho pointer chỉ đọc. **Rationale:** compiler ngăn mutation không thuộc contract và signature dễ review. **Positive:** `summarize_samples(const int *values, ...)`. **Negative/hậu quả:** pointer writable không cần thiết làm caller không biết hàm có đổi input hay không.
- **Rule:** coi warning là failure và không cast để che mismatch. **Rationale:** warning type/format thường là bằng chứng sớm của UB hoặc output sai. **Positive:** sửa declaration/format đúng loại. **Negative/hậu quả:** ép cast làm build xanh nhưng pointer vẫn trỏ sai kiểu.
- **Rule:** debug từ lần state lệch đầu tiên, không từ nơi crash cuối. **Rationale:** crash thường là hậu quả muộn của dữ liệu đã hỏng. **Positive:** breakpoint ở parser ngay token lỗi và xem `end == cursor`. **Negative/hậu quả:** chỉ nhìn stack frame cuối rồi thêm null check ngẫu nhiên có thể che nguyên nhân.

**Failure và troubleshooting.** Dấu hiệu compiler báo incompatible pointer type: đọc cả expected/actual type, sửa prototype/call; không cast. Dấu hiệu GDB in “Cannot access memory” khi dereference: xem pointer value, stack frame và lifetime, quay lại nơi pointer được tạo; sửa ownership/lifetime hoặc precondition. Dấu hiệu invalid input vẫn exit `0`: kiểm return value ở `main` và `$?`, đặt breakpoint tại nhánh lỗi, trả exit code khác zero; phòng tránh bằng script/oracle kiểm cả stream lẫn exit code.

## Ví dụ tích hợp và cách tự kiểm

Ví dụ đầy đủ ở [example.md](example.md) và source tại [basic_refresher_demo.c](assets/basic_refresher_demo.c). Hai lệnh tối thiểu:

```bash
/tmp/basic_refresher_demo '18,21,24,30'
/tmp/basic_refresher_demo '18,xx,24'; test "$?" -eq 2
```

Chỉ coi readiness increment `M00-RDY` đạt khi build warning-free, happy oracle đúng, invalid oracle đúng stream và exit code. Nếu chưa đạt, dùng GDB theo workflow ở `OUT-B00-04` rồi chạy lại cả hai ca để tránh regression.

## Thuật ngữ nhanh

- **Declaration / definition:** khai báo giới thiệu tên/kiểu; định nghĩa cấp body hoặc storage theo trường hợp.
- **Scope / lifetime:** vùng source dùng được tên / khoảng thời gian object tồn tại.
- **Array length / capacity:** số phần tử đang có ý nghĩa / số phần tử storage chứa được.
- **Null-terminated string:** dãy `char` có `\0` trong vùng hợp lệ.
- **Pointer / pointee:** giá trị địa chỉ / object được địa chỉ đó tham chiếu.
- **Oracle:** kết quả quan sát được dùng quyết định pass/fail, gồm output, exit code và warning/finding.

## 7. Nguồn tham khảo và provenance phần bổ sung

### Phạm vi được người dùng phê duyệt

- `SRC-USER-BASIC` — Yêu cầu trực tiếp “Add thêm phần basic để học cho học viên ôn tập”, nhận ngày 2026-08-21. Yêu cầu này phê duyệt việc bổ sung đúng bốn leaf nền tảng trước S01; phạm vi không xuất phát từ content outline chính thức và không thay đổi syllabus/LO/assessment hiện có.
- Nhóm chủ đề được suy ra từ prerequisite hiện có của S01: data types, control flow, functions, arrays, basic pointers và khả năng biên dịch/chạy chương trình C đơn giản. Việc gộp thành bốn leaf là quyết định biên tập được phê duyệt theo yêu cầu bổ sung; không tuyên bố đây là nội dung nguyên văn của syllabus.

### Nguồn kỹ thuật được dùng

- `SRC-C17-ISO` — ISO/IEC 9899:2018, Programming Languages — C, Edition 4 (C17), metadata/clause reference only: https://www.iso.org/standard/74528.html
- `SRC-C17-WG14` — WG14 N2176 proposed C17 committee draft, tài liệu công khai để đối chiếu semantics; không tuyên bố là bản ISO final: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf
- `SRC-GNUC` — GNU C Language Manual, dùng cho diễn giải nhập môn và được giới hạn về C17: https://www.gnu.org/software/c-intro-and-ref/manual/html_node/index.html
- `SRC-GCC11` — GCC 11.4 manuals, dùng cho compiler/debug option baseline: https://gcc.gnu.org/onlinedocs/gcc-11.4.0/
- `SRC-CERTC` — SEI CERT C Coding Standard, dùng cho quy tắc kiểm miền giá trị, mảng, chuỗi và pointer: https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/

Các mô tả kỹ thuật là bản diễn giải nguyên gốc cho khóa; asset code và dữ liệu đều synthetic. Không sao chép code bên ngoài, không dùng secret, endpoint, PII hay dữ liệu khách hàng.

## Học tiếp

Khi hai oracle của `M00-RDY` đều đạt, chuyển sang [S01 — Kick-off and advanced pointers](../../Phan_1_Unit_01__Advanced_Pointers__Memory_Management/Session_1_S01_Session_01__Kick-off_and_advanced_pointers/material.md).
