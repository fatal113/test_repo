# B10 — File Handling — Học liệu cốt lõi

> **Khóa học:** [NFP] Advanced C Programming · **Thời lượng:** 120 phút · **LO:** `ADVC-H1SD`, `ADVC-H3SD`  
> **Ranh giới bắt buộc:** phần portable dùng ISO C17 `stdio`. Mô tả descriptor/filesystem là mental model OS, không phải API portable. S-record ở Unit này **chỉ là kiểm tra định dạng file và checksum trên local file**; không flashing, không ghi bộ nhớ thiết bị, không MCU/HAL/RTOS/MMIO/ISR và không kết nối phần cứng.

## 🎯 Learning Outcomes

- Thiết kế pipeline đọc file bounded và write→`fflush`→close→reopen verification, kiểm đầy đủ error/EOF/position/cleanup và tạo oracle xác định (`ADVC-H1SD`).
- Đánh giá record text và S-record profile bằng exact diagnostics/checksum evidence (`ADVC-H3SD`).
- **Increment `M00-FND-10`:** [b10_file_demo.c](assets/b10_file_demo.c), fixtures [lab06_records.csv](assets/lab06_records.csv), [lab06_sample.srec](assets/lab06_sample.srec), [lab06_bad_checksum.srec](assets/lab06_bad_checksum.srec).

## 1. Kiến thức tiên quyết và mental map

- Hoàn thành B09; hiểu array, pointer, bounded string, integer conversion, ownership và cleanup.
- Strict baseline: `-std=c17 -Wall -Wextra -Wpedantic -Werror`; input chỉ là local path do người chạy cung cấp.
- Mental map: `persistent bytes ↔ OS filesystem/name/metadata ↔ open handle ↔ C FILE stream + buffering/state → bounded read/checked write → parse hoặc flush/reopen verify → exact oracle → close exactly once`.
- Một file có thể hợp lệ ở tầng bytes nhưng sai schema/checksum; mở file thành công không chứng minh nội dung hợp lệ.

## 3. Nội dung lý thuyết cốt lõi

- [x] how file is oganized in disk
- [x] how OS manage files
- [x] how application read/write file
- [x] stream in OS, file stream
- [x] text stream and binary stream
- [x] File pointer
- [x] Active file pointer
- [x] open and close file
- [x] APIs for read, write file
- [x] manage Active file pointer
- [x] fflush
- [x] Overview about srecord format

#### OUT-B10-01 — how file is oganized in disk

**Mapping.** `OUT-B10-01` → `ADVC-H1SD` → `M00-FND-10`: tách logical byte sequence mà C đọc khỏi cách filesystem/storage đặt blocks.

**Định nghĩa/ranh giới.** Ở mức chương trình portable, file là sequence dữ liệu được truy cập qua stream; directory entry, inode/MFT, extents, blocks, cache và device sectors là implementation/OS concepts. Spelling plan được giữ nguyên; thuật ngữ đúng là **organized**. C17 không cho phép suy physical contiguity hoặc atomicity từ `FILE *`.

**Vai trò/quyết định.** Application developer quyết định format/schema/limits, không tự quản block placement. Nếu cần durability/atomic rename/sparse-file guarantees phải chọn và ghi rõ OS API ngoài portable scope.

**Cơ chế.** Tên path được OS resolve tới object; filesystem ánh xạ logical offsets sang storage và cache. C library chuyển stream operations thành host services; hardware placement có thể đổi mà logical bytes vẫn như cũ.

**Khi dùng/không dùng/trade-off.** Mental model logical file đủ cho bounded parser. Không dựa vào “các dòng nằm liên tiếp trên disk” để tối ưu; OS-specific tuning chỉ dùng sau profile. Abstraction tăng portability nhưng che một số durability details.

**Ví dụ/oracle.** CSV fixture có 5 logical text lines; parser không quan tâm sectors. Oracle `OK csv records=4 value_sum=93 flags_or=0x03` chứng minh logical content, không chứng minh block layout.

**Best practice.** **Rule:** contract file bằng bytes/records/encoding, không bằng physical layout. **Rationale:** filesystem có quyền phân mảnh/cache. **Positive:** header `id,value,flags` và line cap `96`. **Negative:** giả định một `fread` luôn tương ứng một disk sector hay một record.

**Failure/troubleshooting.** Chỉ lỗi trên filesystem/máy khác → phụ thuộc layout/line-ending/path → thu fixture bytes, stat/OS info và parser trace → sửa format-level assumptions → phòng tránh bằng portable fixtures và explicit encoding/newline policy.

#### OUT-B10-02 — how OS manage files

**Mapping.** `OUT-B10-02` → `ADVC-H1SD` → `M00-FND-10`: hiểu pathname, permissions, open resource, offset và errors đứng dưới C stream.

**Định nghĩa/ranh giới.** OS quản namespace, metadata, permissions, open handles/descriptors, cache và concurrency. Descriptor/POSIX semantics không đồng nhất với `FILE *`; Unit chỉ dùng chúng để giải thích, không gọi `open(2)`.

**Vai trò/quyết định.** Developer phải xử lý path không tồn tại, permission/resource failures và concurrent replacement theo contract. Không rò path nhạy cảm trong production logs; demo local in path để oracle dễ học.

**Cơ chế.** `fopen` yêu cầu runtime/OS resolve path và cấp resource; nhiều stream/handle có thể tham chiếu cùng file nhưng có state/buffering khác. Rename/unlink/concurrent writes có behavior theo host.

**Khi dùng/không dùng/trade-off.** `stdio` đủ cho portable sequential parser; OS descriptor API cần khi contract đòi flags/locking/nonblocking/durability cụ thể. Portability đổi lấy ít quyền kiểm soát host semantics.

**Ví dụ/oracle.** Path không tồn tại làm `fopen` trả `NULL`; executable exit `3`, stderr bắt đầu `ERROR cannot open local file:` và không dereference stream.

**Best practice.** **Rule:** coi open là operation có thể fail và giữ resource owner rõ. **Rationale:** path hợp lệ về syntax vẫn có thể fail permission/resource. **Positive:** kiểm `stream==NULL` ngay. **Negative:** gọi `fgets` sau `fopen` mà không kiểm.

**Failure/troubleshooting.** “Cannot open” → path/permission/resource chứ chưa phải parse error → ghi exact path trong môi trường học, kiểm working directory/permissions → sửa path/quyền → phòng tránh bằng absolute fixture path trong automation và phân loại exit code.

#### OUT-B10-03 — how application read/write file

**Mapping.** `OUT-B10-03` → `ADVC-H1SD` → `M00-FND-10`: application mở, transfer bounded, phân biệt EOF/error, validate, rồi close.

**Định nghĩa/ranh giới.** Read/write API trao đổi units theo stream; một call có thể trả ít hơn yêu cầu hoặc gặp EOF/error. Text parsing còn có schema/range constraints ngoài I/O success.

**Vai trò/quyết định.** Caller chọn record boundary, maximum, retry/partial-write policy và error propagation. Hai validation modes chỉ đọc fixtures; `write-demo` chỉ ghi vào **scratch output path do người chạy cung cấp**, rồi flush, close, reopen và so exact bytes.

**Cơ chế.** `fgets` đọc tối đa capacity-1 và thêm NUL nếu thành công; absence newline khi chưa EOF cho thấy line bị cắt bởi buffer. Ngay cả lần đọc header đầu tiên, `NULL + ferror!=0` là resource error exit `3`, còn `NULL + !ferror` là empty input exit `2`. Write path kiểm `fprintf`, `fflush`, `fclose`, rồi `fread` exact length sau reopen.

**Khi dùng/không dùng/trade-off.** Line I/O phù hợp bounded CSV/S-record; block I/O phù hợp binary fixed/length-prefixed data. Line API dễ debug nhưng phải xử lý long line và newline variants.

**Ví dụ/oracle.** CSV line vượt 95 chars bị reject `ERROR CSV line too long at line N`, exit `2`. `write-demo <fresh-scratch-path>` phải tạo đúng 23 bytes `status=ready\nrecords=2\n`, exit `0`, stderr rỗng và stdout `OK write-demo bytes=23 flush=ok reopen=match`.

**Best practice.** **Rule:** kiểm return trước buffer use và kiểm completeness/error sau transfer. **Rationale:** EOF, partial record và I/O error là trạng thái khác nhau. **Positive:** `fgets`→newline/EOF check→parse→`ferror`. **Negative:** `while (!feof(f)) { fgets(...); parse(...); }` dùng stale buffer.

**Failure/troubleshooting.** Record lặp cuối file hoặc empty bị báo nhầm I/O error → loop/first-read không xét đúng return+indicators → trace `fgets`, `feof`, `ferror` → loop trên return và phân nhánh `ferror` trước empty → phòng tránh bằng exact tests empty exit `2` và Linux directory-read error exit `3`.

#### OUT-B10-04 — stream in OS, file stream

**Mapping.** `OUT-B10-04` → `ADVC-H1SD` → `M00-FND-10`: phân biệt conceptual byte channel/OS handle với C `FILE` stream có buffer và indicators.

**Định nghĩa/ranh giới.** “Stream” là chuỗi dữ liệu theo thứ tự; C file stream là object thư viện gắn external file, có buffer, orientation, EOF/error và position state. OS handle/descriptor là resource tầng dưới, không được thay thế lẫn nhau tùy ý.

**Vai trò/quyết định.** Chọn một abstraction owner cho mỗi pipeline. Trộn descriptor I/O và stdio trên cùng underlying file cần synchronization rules ngoài case và dễ làm offsets/buffers bất nhất.

**Cơ chế.** `fgets` có thể lấy bytes từ user-space buffer dù OS offset đã đi xa hơn; `fclose` flush output cần thiết và giải phóng stream. Vì buffer, “đã gọi write” và “đã durable trên disk” không giống nhau.

**Khi dùng/không dùng/trade-off.** C stream tốt cho formatted/line I/O portable; descriptor API tốt cho OS-specific flags/readiness. Buffering giảm calls nhưng thêm state và visibility timing.

**Ví dụ/oracle.** Asset không trộn descriptor với stdio. CSV/SREC dùng borrowed input stream; `write-demo` dùng một output stream, close, rồi tạo input stream mới để verify. Exact `flush=ok reopen=match` chứng minh workflow tầng stream, không chứng minh physical durability.

**Best practice.** **Rule:** không mix I/O abstractions trên cùng resource nếu chưa có synchronization contract. **Rationale:** hai lớp giữ state/buffer riêng. **Positive:** mọi read qua `fgets`. **Negative:** `read(fileno(fp),...)` xen `fgets(fp,...)` tùy ý.

**Failure/troubleshooting.** Thiếu/duplicate bytes khi mix APIs → offsets và buffers lệch → minimal reproduction chỉ một abstraction → bỏ mix hoặc dùng documented coordination → phòng tránh bằng wrapper ownership và code-review boundary.

#### OUT-B10-05 — text stream and binary stream

**Mapping.** `OUT-B10-05` → `ADVC-H1SD` → `M00-FND-10`: dùng text mode cho ASCII fixtures, nhưng format/checksum được định nghĩa trên ký tự hex sau newline trim.

**Định nghĩa/ranh giới.** C phân biệt text và binary stream; text mode có thể translate line endings/representation theo host, binary giữ bytes đọc/ghi. Trên một số systems chúng giống nhau, nhưng portable code không giả định vậy.

**Vai trò/quyết định.** Format owner chọn mode: human-readable line format dùng text; opaque bytes/exact offsets dùng binary. S-record là ASCII line format, không phải “binary firmware image” dù nó biểu diễn byte values.

**Cơ chế.** `fopen(path,"r")` tạo text input stream; `trim_newline` bỏ CR/LF ở record boundary. Hex pairs được decode sau đó. Với binary `rb`, application tự quản exact byte count và không dùng string functions trên arbitrary data.

**Khi dùng/không dùng/trade-off.** Text dễ inspect/diff nhưng parsing/locale/newline cần policy; binary compact/exact nhưng cần schema/version/endian và hex dump tooling.

**Ví dụ/oracle.** `S107001001020304DE` là text record; validator tính checksum từ decoded pairs, không flash payload `01 02 03 04`. Oracle `data_bytes=4`.

**Best practice.** **Rule:** mode phải khớp representation contract. **Rationale:** text translation và binary byte identity khác nhau. **Positive:** `r` + bounded lines cho CSV/SREC. **Negative:** dùng `strlen` trên binary buffer hoặc coi S-record line là lệnh thiết bị.

**Failure/troubleshooting.** Offset/checksum khác theo OS → mode/newline included sai → hex dump fixture, kiểm trim và bytes tham gia checksum → chọn mode/normalize đúng spec → phòng tránh bằng golden fixtures CRLF/LF và explicit checksum domain.

#### OUT-B10-06 — File pointer

**Mapping.** `OUT-B10-06` → `ADVC-H1SD` → `M00-FND-10`: `FILE *stream` là opaque handle được kiểm NULL, truyền theo borrow và đóng bởi owner `main`.

**Định nghĩa/ranh giới.** `FILE *` trỏ tới object thư viện C đại diện stream; layout là opaque, không dereference fields. Nó không phải buffer data, filename hay raw OS descriptor.

**Vai trò/quyết định.** CSV/SREC processors nhận `FILE *` mượn và không close; `main` là owner. `run_write_demo` tự sở hữu từng output/input stream theo hai lifetime không chồng nhau và close trước khi reopen.

**Cơ chế.** Runtime giữ buffer/indicators trong object. Sau `fclose`, pointer không còn là stream hợp lệ; dùng lại là lifetime violation.

**Khi dùng/không dùng/trade-off.** Truyền `FILE *` giúp processor test được với local/temp stream; truyền path khi function phải sở hữu policy open. Borrowed stream linh hoạt nhưng ownership cần ghi rõ.

**Ví dụ/oracle.** `process_csv(stream)` không close; `main` gọi `fclose` dù processor trả `2`. Trong `write-demo`, output `FILE *` không được dùng sau close và input stream mới đọc đúng 23 bytes; sanitizer run không báo lỗi thuộc fixture.

**Best practice.** **Rule:** mỗi `FILE *` có một close owner, NULL/lifetime được kiểm. **Rationale:** double-close/use-after-close là UB, quên close rò resource. **Positive:** open→process→single close. **Negative:** helper close rồi caller tiếp tục `ferror(stream)`.

**Failure/troubleshooting.** Bad descriptor/double free sau helper → ownership mơ hồ → trace open/close sites → hợp nhất owner và document borrow → phòng tránh bằng wrapper pattern và cleanup tests.

#### OUT-B10-07 — Active file pointer

**Mapping.** `OUT-B10-07` → `ADVC-H1SD` → `M00-FND-10`: giải thích “active file pointer” trong outline là **file position indicator** đang chọn byte/logical position cho lần I/O kế tiếp.

**Định nghĩa/ranh giới.** Mỗi C stream có file position indicator (khi stream/file hỗ trợ positioning) cùng EOF/error state. Đây không phải biến `FILE *` và không nhất thiết là physical disk address; text-stream positions chỉ portable khi dùng giá trị từ `ftell` để quay lại bằng `fseek` phù hợp.

**Vai trò/quyết định.** Sequential parser để library tự advance; random-access code phải lưu/restore positions và handle failure. Không dùng position làm record count nếu text translation tồn tại.

**Cơ chế.** Successful read chuyển indicator theo dữ liệu đã đọc; EOF indicator được set sau attempt không đọc được. `rewind` đưa về đầu và clear EOF/error; `clearerr` chỉ clear indicators, không reposition.

**Khi dùng/không dùng/trade-off.** Sequential pass đơn giản, ít state; seek hữu ích cho index/header nhưng có thể không được hỗ trợ ở non-seekable stream. Asset không seek vì checksum/aggregation chỉ cần một pass.

**Ví dụ/oracle.** Sau header CSV, bốn `fgets` lần lượt đọc records 1..4; records=`4`. Không gọi `rewind`, nên không duplicate header/records.

**Best practice.** **Rule:** gọi position APIs và kiểm return; không đoán offset text. **Rationale:** stream/host có thể không seekable và text positions opaque. **Positive:** lưu `long p=ftell(fp)`, kiểm `p!=-1L`, restore `fseek`. **Negative:** cộng byte count thủ công rồi seek text stream.

**Failure/troubleshooting.** Đọc lại thiếu/duplicate record → position bị đổi/không reset EOF → log `ftell` khi hợp lệ và indicators → `rewind`/checked `fseek`, clear state đúng → phòng tránh bằng sequential design hoặc centralized position owner.

#### OUT-B10-08 — open and close file

**Mapping.** `OUT-B10-08` → `ADVC-H1SD` → `M00-FND-10`: mode `r` cho validator và C17 `wx` exclusive-create cho scratch `write-demo`, immediate NULL check, processing, rồi checked `fclose` trên mọi success path.

**Định nghĩa/ranh giới.** `fopen` tạo stream theo mode và có thể fail; `fclose` flush output còn buffer, đóng associated file và giải phóng stream, cũng có thể fail. Sau failed open không close; sau close không reuse pointer.

**Vai trò/quyết định.** Chọn least privilege: validator read-only không mở output mode. `write-demo` dùng C17 `wx`, nên existing path bị reject thay vì truncate; close failure được report trước khi báo verified.

**Cơ chế.** Read modes lưu processor status rồi close; close fail sau success trả `3`. Write mode kiểm `fprintf`→`fflush`→`fclose`, reopen bằng `r`, `fread` exact bytes, kiểm EOF/error và close lần hai.

**Khi dùng/không dùng/trade-off.** `r` cho existing text, `wx` cho exclusive-create scratch output, `rb` cho exact binary. Exclusive create yêu cầu caller cấp path mới nhưng ngăn overwrite; C17 cần cleanup explicit.

**Ví dụ/oracle.** Missing read file: exit `3`, `ERROR cannot open local file: <path>`. Fresh scratch write: `OK write-demo bytes=23 flush=ok reopen=match`; file bytes phải match exact payload.

**Best practice.** **Rule:** open với mode tối thiểu, kiểm ngay, close exactly once và kiểm close khi output/resource integrity quan trọng. **Rationale:** mode sai có thể phá data; close có thể phát hiện buffered-write failure. **Positive:** validator `fopen(path,"r")`; writer dùng `fopen(path,"wx")` dưới scratch directory. **Negative:** output mode truncating không exclusive có thể làm mất dữ liệu trước validation.

**Failure/troubleshooting.** File đột ngột zero bytes → destructive mode → inspect open mode/audit logs → restore fixture và đổi `r`/`rb` → phòng tránh bằng read-only mounts và negative test missing file.

#### OUT-B10-09 — APIs for read, write file

**Mapping.** `OUT-B10-09` → `ADVC-H1SD` → `M00-FND-10`: chọn `fgets` cho bounded line records; biết khi nào `fread`/`fwrite`, `fprintf`/`fputs` phù hợp.

**Định nghĩa/ranh giới.** Character/line/formatted/block APIs có units và return contracts khác nhau. `fread`/`fwrite` trả số elements, không phải boolean; formatted input có ambiguity/range pitfalls nếu contract không chặt.

**Vai trò/quyết định.** API phải khớp record format và error strategy. CSV parser dùng line rồi explicit parse để giữ line-number diagnostic; `write-demo` dùng checked `fprintf`, sau reopen dùng `fread` exact byte count.

**Cơ chế.** `fgets` NUL-terminates khi success; `strchr` kiểm delimiters; `strtoul` kiểm consumed-all/range. `fprintf` trả số ký tự hoặc giá trị âm; `fread` trả item count; asset kiểm cả hai, extra byte, `ferror`, flush và close.

**Khi dùng/không dùng/trade-off.** `fgets`+parse dễ tạo deterministic error; `fscanf` ngắn nhưng whitespace/partial conversion khó audit. Block I/O nhanh/gọn cho byte data nhưng schema/checksum vẫn là trách nhiệm application.

**Ví dụ/oracle.** CSV field thừa (`1,18,0x01,extra`) bị `ERROR invalid CSV record at line 2`, exit `2`. Write path chỉ pass nếu `fprintf` ghi 23 chars và reopened `fread` thấy đúng 23 bytes, không thiếu/thừa.

**Best practice.** **Rule:** kiểm exact return và consumed input, không chỉ “khác EOF”. **Rationale:** partial conversion/read có thể để state nửa hợp lệ. **Positive:** parse line rồi yêu cầu `*end=='\0'`. **Negative:** `fscanf(fp,"%u",&x)` rồi bỏ qua return/suffix.

**Failure/troubleshooting.** Parser chấp nhận record cắt cụt → return/count không được kiểm → capture exact line + API result → require complete delimiters/elements → phòng tránh bằng malformed/truncated fixture matrix.

#### OUT-B10-10 — manage Active file pointer

**Mapping.** `OUT-B10-10` → `ADVC-H1SD` → `M00-FND-10`: quản file position bằng sequential ownership hoặc `fgetpos/fsetpos`, `ftell/fseek`, `rewind` có kiểm soát.

**Định nghĩa/ranh giới.** Position APIs thay/đọc logical indicator; `fgetpos/fsetpos` dùng `fpos_t`, `ftell/fseek` dùng contract offset theo stream mode, `rewind` về đầu và clear error. Không phải mọi stream seekable.

**Vai trò/quyết định.** Parser architect chọn single pass khi có thể; nếu backtracking, một component phải sở hữu position và reset parser state cùng stream state.

**Cơ chế.** Seek thành công làm input tiếp theo lấy từ position mới và có effects trên EOF; update streams còn cần sequencing giữa read/write. Asset single-pass tránh backtracking và giữ line_number đồng bộ.

**Khi dùng/không dùng/trade-off.** Seek cho file index/random record; không dùng với pipe/terminal hoặc giả định success. Single pass dùng memory nhỏ nhưng không thuận tiện cross-record references.

**Ví dụ/oracle.** S-record parser reject ngay record sau S9 tại đúng line; nó không seek quay lại và không reset `saw_termination`, nên diagnostic deterministic `ERROR record follows S9 termination at line N`.

**Best practice.** **Rule:** centralize position changes, kiểm return và đồng bộ logical parser state. **Rationale:** seek riêng lẻ có thể làm line counter/cache/parser diverge. **Positive:** helper `seek_record` cập nhật cả position và record index. **Negative:** nhiều helpers `fseek` cùng stream không contract.

**Failure/troubleshooting.** Error line sai sau rewind → line counter không reset cùng position → log position+logical state → reset atomically hoặc single pass → phòng tránh bằng state-machine ownership tests.

#### OUT-B10-11 — fflush

**Mapping.** `OUT-B10-11` → `ADVC-H1SD` → `M00-FND-10`: xác định đúng mục đích flush và tránh anti-pattern `fflush(stdin)` trong ISO C17 portable code.

**Định nghĩa/ranh giới.** Với output/update stream có dữ liệu output chưa ghi, `fflush` chuyển buffered output tới host environment và trả status. Nó không bảo đảm physical durability như một OS sync primitive. Trong portable ISO C, không gọi `fflush` trên input-only stream để “xóa bàn phím”.

**Vai trò/quyết định.** Flush khi cần visibility/order trước khi tiếp tục hoặc chuyển output→input trên update stream; luôn kiểm return khi correctness phụ thuộc write. CSV/SREC input không flush; `write-demo` có executable `fflush` gate trước close/reopen.

**Cơ chế.** Buffering gom writes; flush yêu cầu library chuyển chúng xuống tầng host. `fclose` cũng flush output nhưng explicit flush cho phép phát hiện lỗi trước và giữ stream mở. Update-stream direction changes có sequencing rules riêng.

**Khi dùng/không dùng/trade-off.** Dùng cho protocol prompt/log visibility có contract; không flush mỗi byte nếu không cần vì tăng I/O overhead. Không dùng như disk durability hoặc input discard portable.

**Ví dụ/oracle.** `write-demo` kiểm `fflush(out)`, close, reopen và compare payload; exact stdout `OK write-demo bytes=23 flush=ok reopen=match`. `fflush(stdin)` không xuất hiện. Oracle chỉ nói bytes được chuyển tới host environment và đọc lại được, không nói đã durable trên media.

**Best practice.** **Rule:** flush output có mục đích, kiểm status; không `fflush(stdin)`. **Rationale:** input behavior không portable và output failure có thể xuất hiện muộn. **Positive:** `if (fflush(out)==EOF) handle_error();`. **Negative:** `fflush(stdin)` để bỏ dòng hoặc cho rằng `fflush(out)` đồng nghĩa dữ liệu đã nằm bền vững trên disk.

**Failure/troubleshooting.** `ERROR flushing local output file` hoặc reopen mismatch → buffered write/resource/content failure → kiểm return của `fprintf`/`fflush`/`fclose`, file length và scratch permissions → dừng trước khi báo success → phòng tránh bằng fresh scratch path và exact reopen oracle; durability cần OS facility ngoài scope.

#### OUT-B10-12 — Overview about srecord format

**Mapping.** `OUT-B10-12` → `ADVC-H1SD`, `ADVC-H3SD` → `M00-FND-10`: validate bounded S1/S9 text profile và checksum; **không thực hiện flashing/hardware**.

**Định nghĩa/ranh giới.** Motorola S-record là ASCII records bắt đầu `S` + type, byte-count, address, optional data và checksum. Byte-count đếm address+data+checksum sau count. Checksum là ones' complement của low byte tổng count/address/data; tương đương low-byte sum gồm checksum bằng `0xFF`. Unit profile chỉ chấp nhận ít nhất một `S1` **có data** (16-bit address) trước một `S9` cuối (16-bit start, zero data); các record type khác bị reject.

**Vai trò/quyết định.** File-ingestion developer chỉ quyết định “format hợp lệ theo profile hay không”, tổng data bytes và start field. Không ghi address/data vào process memory, device memory hoặc cổng giao tiếp.

**Cơ chế.** Parser giới hạn line 519 chars, decode mỗi hex pair, xác minh exact length từ count, cộng modulo 256, kiểm `0xFF`, cộng data bytes có overflow guard, track `saw_data_record`, reject S9 trước S1, reject S1 rỗng và cấm record sau S9.

**Khi dùng/không dùng/trade-off.** Profile nhỏ phù hợp bài học/checksum gate; không dùng để xác nhận toàn bộ S-record variants, memory ranges, firmware authenticity/signature hay deployment safety. Full production parser cần spec profile/version, address policy, record types và security review riêng.

**Ví dụ/oracle.** `S107001001020304DE` rồi `S9030000FC` cho exact `OK srec records=2 data_bytes=4 start=0000`. Đổi checksum `DE`→`DF` cho mismatch. File chỉ có `S9030000FC` phải exit `2`, stdout rỗng, exact stderr `ERROR S9 termination precedes S1 data at line 1`.

**Best practice.** **Rule:** validate syntax→count/length→hex→checksum→S1-data-before-S9 sequencing trước khi dùng metadata. **Rationale:** checksum hợp lệ không chứng minh sequence/profile đầy đủ. **Positive:** reject S9-only, S1 rỗng, type ngoài profile và record sau S9. **Negative:** thấy checksum S9 đúng rồi chấp nhận file không có data record hoặc flash bytes.

**Failure/troubleshooting.** Checksum mismatch → byte bị sửa, count/domain tính sai hoặc newline bị đưa vào → dump decoded bytes và running sum → sửa fixture/parser theo spec → phòng tránh bằng known-good/bad golden vectors. Nhắc lại: kết quả pass chỉ nói **file-format profile hợp lệ**, không cho phép flashing.

## 3. Ví dụ tích hợp

Build asset rồi chạy hai read modes và một write/flush mode trên fresh scratch path:

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  assets/b10_file_demo.c -o b10_file_demo
./b10_file_demo csv assets/lab06_records.csv
./b10_file_demo srec assets/lab06_sample.srec
scratch_dir=$(mktemp -d)
./b10_file_demo write-demo "$scratch_dir/readiness.txt"
```

Exact output:

```text
OK csv records=4 value_sum=93 flags_or=0x03
OK srec records=2 data_bytes=4 start=0000
OK write-demo bytes=23 flush=ok reopen=match
```

Checksum-negative fixture:

```sh
./b10_file_demo srec assets/lab06_bad_checksum.srec
# exit 2; stdout rỗng; stderr:
ERROR S-record checksum mismatch at line 1
```

File write phải có đúng 23 bytes và nội dung exact; scratch được xóa sau khi so:

```sh
printf 'status=ready\nrecords=2\n' >"$scratch_dir/expected.txt"
cmp "$scratch_dir/expected.txt" "$scratch_dir/readiness.txt"
rm -rf "$scratch_dir"
```

## 4. Quiz tự kiểm tra (5 câu)

1. **Một `fread` thành công một phần có phải boolean success không?**  
   **Đáp án:** Không. Nó trả số elements đã đọc; caller so với count yêu cầu rồi phân biệt EOF/error theo contract.

2. **Vì sao `while (!feof(fp))` thường sai?**  
   **Đáp án:** EOF chỉ được set sau một read attempt thất bại; loop có thể xử lý stale buffer. Loop trên return của read, rồi kiểm `ferror`.

3. **`FILE *` có phải file position không?**  
   **Đáp án:** Không. Nó là pointer tới opaque stream object; position indicator là state bên trong stream và thay đổi theo I/O/seek.

4. **`fflush(stdin)` có phải cách portable để bỏ input dư?**  
   **Đáp án:** Không trong ISO C17. Hãy đọc/discard bounded theo input policy; `fflush` dùng đúng contract output/update stream.

5. **S-record checksum pass có chứng minh image an toàn để nạp thiết bị?**  
   **Đáp án:** Không. Nó chỉ chứng minh integrity arithmetic của record theo profile. Authenticity, allowed addresses, device compatibility và flashing đều ngoài scope Unit.

## 5. Từ điển thuật ngữ

- **Filesystem/logical file:** cơ chế OS tổ chức namespace/storage / sequence dữ liệu application thấy.
- **`FILE *`:** handle opaque của C library cho stream.
- **Buffering:** lưu tạm dữ liệu để giảm host I/O calls.
- **EOF/error indicator:** trạng thái end-of-file / lỗi của stream, không phải loop condition trước read.
- **File position indicator:** vị trí logical dùng cho operation kế tiếp nếu stream hỗ trợ.
- **Text/binary mode:** mode có thể translate text / giữ byte identity.
- **S1/S9:** S-record data với address 16-bit / termination với start field 16-bit trong profile này.
- **Byte-count/checksum:** số bytes sau count / ones' complement kiểm tổng record.
- **Oracle:** stdout, stderr, exit code và diagnostics chính xác dùng quyết định pass/fail.

## 7. Nguồn tham khảo và provenance phần bổ sung

- `SRC-USER-CREF` — outline Part 0 đã được duyệt.
- `SRC-C17-ISO` — ISO/IEC 9899:2018 metadata: <https://www.iso.org/standard/74528.html>.
- `SRC-C17-WG14` — public committee draft N2176: <https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf>.
- `SRC-GLIBC235` — GNU C Library Reference Manual 2.35: <https://sourceware.org/glibc/manual/2.35/pdf/libc.pdf>.
- `SRC-CERTC` — SEI CERT C Coding Standard: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/>.
- `SRC-MOTOROLA-SREC` — Motorola S-record description, Appendix A: <https://www.nxp.com/docs/en/user-guide/M68332BCC.pdf>.
