# B10 — File Handling — Ví dụ thực thi

> **Case:** `CASE-B10-01` · **LO:** `ADVC-H1SD`, `ADVC-H3SD` · **Increment:** `M00-FND-10`  
> **Giới hạn an toàn:** case đọc local files, kiểm schema/checksum và ghi duy nhất một fresh scratch output do người chạy chọn. S-record **không** được flash, không được ghi vào address trong record, không gọi thiết bị, MCU, HAL, RTOS, MMIO hoặc ISR.

## 🎯 Learning Outcomes liên quan

- [x] `OUT-B10-01` — how file is oganized in disk
- [x] `OUT-B10-02` — how OS manage files
- [x] `OUT-B10-03` — how application read/write file
- [x] `OUT-B10-04` — stream in OS, file stream
- [x] `OUT-B10-05` — text stream and binary stream
- [x] `OUT-B10-06` — File pointer
- [x] `OUT-B10-07` — Active file pointer
- [x] `OUT-B10-08` — open and close file
- [x] `OUT-B10-09` — APIs for read, write file
- [x] `OUT-B10-10` — manage Active file pointer
- [x] `OUT-B10-11` — fflush
- [x] `OUT-B10-12` — Overview about srecord format

## CASE-B10-01 — Bounded readiness-file validator

### 1. Ticket và tiêu chí thành công

Gateway team nhận CSV readiness records và S-record text để kiểm integrity, đồng thời cần một workflow write/flush/reopen nhỏ trên local scratch file. Utility chỉ tạo evidence về file I/O/format. Artifact implementation: [b10_file_demo.c](assets/b10_file_demo.c).

Case đạt khi:

1. C17 strict build zero warnings.
2. CSV fixture exit `0`, stderr rỗng, exact stdout `OK csv records=4 value_sum=93 flags_or=0x03`.
3. S-record fixture exit `0`, stderr rỗng, exact stdout `OK srec records=2 data_bytes=4 start=0000`.
4. Bad-checksum fixture exit `2`, stdout rỗng, exact stderr `ERROR S-record checksum mismatch at line 1`.
5. Missing path/open failure được tách khỏi format failure; mọi opened `FILE *` được close đúng một lần.
6. Sanitizer build không diagnostic; tài liệu không suy checksum pass thành quyền flashing.
7. `write-demo` kiểm `fprintf`, `fflush`, `fclose`, reopen và exact 23-byte content; không ghi vào fixture.
8. Empty CSV exit `2` khác initial read I/O error exit `3`; S9-only bị reject vì chưa có S1 data.

### 2. Input và profile

CSV [lab06_records.csv](assets/lab06_records.csv):

```csv
id,value,flags
1,18,0x01
2,21,0x03
3,24,0x00
4,30,0x02
```

S-record [lab06_sample.srec](assets/lab06_sample.srec):

```text
S107001001020304DE
S9030000FC
```

Checksum-negative [lab06_bad_checksum.srec](assets/lab06_bad_checksum.srec) đổi checksum record đầu từ `DE` thành `DF`.

Profile parser được công bố, không phải full S-record implementation:

- line bounded; ASCII hex;
- ít nhất một `S1` có data với address 16-bit phải đứng trước `S9` address 16-bit/zero data;
- exact length phải khớp byte-count;
- low-eight-bit sum của count + address + data + checksum phải bằng `0xFF`;
- S1 rỗng, S9-only và record sau S9 đều bị reject;
- chỉ báo `records`, `data_bytes`, `start`; không sử dụng payload/address cho hardware.

### 3. Cách triển khai và build

Với `csv|srec`, `main` mở read-only text stream, chuyển borrowed `FILE *` cho processor, lưu status và `fclose`. CSV phân biệt initial `ferror` với empty trước khi kiểm header; S-record kiểm syntax, count-length, hex, checksum và S1-before-S9 state. Với `write-demo`, helper exclusive-create scratch path bằng C17 `wx`, kiểm `fprintf`/`fflush`/`fclose`, reopen bằng `r`, `fread` đúng 23 bytes, cấm byte thừa và so exact payload.

Từ thư mục Unit:

```sh
set -eu
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 \
  assets/b10_file_demo.c -o b10_file_demo
```

### 4. Happy oracles

CSV:

```sh
./b10_file_demo csv assets/lab06_records.csv >csv.out 2>csv.err
test ! -s csv.err
printf '%s\n' \
  'OK csv records=4 value_sum=93 flags_or=0x03' >csv.expected
diff -u csv.expected csv.out
```

Vì `18+21+24+30=93` và `0x01|0x03|0x00|0x02=0x03`, exact stdout là:

```text
OK csv records=4 value_sum=93 flags_or=0x03
```

S-record:

```sh
./b10_file_demo srec assets/lab06_sample.srec >srec.out 2>srec.err
test ! -s srec.err
printf '%s\n' \
  'OK srec records=2 data_bytes=4 start=0000' >srec.expected
diff -u srec.expected srec.out
```

S1 count `07` = 2 address bytes + 4 data bytes + 1 checksum byte. Tổng low byte `07+00+10+01+02+03+04+DE = FF`. S9 có start field `0000`. Exact stdout:

```text
OK srec records=2 data_bytes=4 start=0000
```

Đây là oracle định dạng file; không phải firmware-authenticity, address-policy hay deployment oracle.

Write/flush/reopen dùng **fresh scratch directory**, không dùng path của fixture:

```sh
scratch_dir=$(mktemp -d)
./b10_file_demo write-demo "$scratch_dir/readiness.txt" \
  >write.out 2>write.err
test ! -s write.err
printf '%s\n' \
  'OK write-demo bytes=23 flush=ok reopen=match' >write.expected
diff -u write.expected write.out
printf 'status=ready\nrecords=2\n' >"$scratch_dir/content.expected"
cmp "$scratch_dir/content.expected" "$scratch_dir/readiness.txt"
```

`fprintf` phải trả đúng length, `fflush` và cả hai lần `fclose` phải success; sau reopen, `fread` phải thấy đúng 23 bytes và EOF ngay sau payload. Exact stdout:

```text
OK write-demo bytes=23 flush=ok reopen=match
```

`flush=ok` chỉ xác nhận C library chuyển buffered output tới host environment và content đọc lại khớp; không phải physical-durability claim.

### 5. Negative oracles

Checksum sai:

```sh
set +e
./b10_file_demo srec assets/lab06_bad_checksum.srec \
  >bad.out 2>bad.err
bad_rc=$?
set -e
test "$bad_rc" -eq 2
test ! -s bad.out
printf '%s\n' \
  'ERROR S-record checksum mismatch at line 1' >bad.expected
diff -u bad.expected bad.err
```

Missing local file là resource error khác format error:

```sh
set +e
./b10_file_demo csv assets/does-not-exist.csv \
  >missing.out 2>missing.err
missing_rc=$?
set -e
test "$missing_rc" -eq 3
test ! -s missing.out
grep -Fx \
  'ERROR cannot open local file: assets/does-not-exist.csv' missing.err
```

Empty input và initial read error là hai contracts khác nhau:

```sh
: >"$scratch_dir/empty.csv"
set +e
./b10_file_demo csv "$scratch_dir/empty.csv" >empty.out 2>empty.err
empty_rc=$?
set -e
test "$empty_rc" -eq 2
test ! -s empty.out
grep -Fx 'ERROR empty CSV file' empty.err

# Linux/glibc baseline: fopen(directory,"r") thành công nhưng initial fgets lỗi EISDIR.
set +e
./b10_file_demo csv "$scratch_dir" >read_error.out 2>read_error.err
read_error_rc=$?
set -e
test "$read_error_rc" -eq 3
test ! -s read_error.out
grep -Fx 'ERROR reading CSV file' read_error.err
```

Test directory-read được gắn nhãn Linux/glibc; nhánh C portable vẫn dựa trên `ferror(stream)`, không dựa trên mã `EISDIR`.

S9 checksum hợp lệ nhưng thiếu S1 data vẫn không đạt profile:

```sh
printf 'S9030000FC\n' >"$scratch_dir/s9-only.srec"
set +e
./b10_file_demo srec "$scratch_dir/s9-only.srec" \
  >s9_only.out 2>s9_only.err
s9_only_rc=$?
set -e
test "$s9_only_rc" -eq 2
test ! -s s9_only.out
grep -Fx 'ERROR S9 termination precedes S1 data at line 1' s9_only.err
rm -rf "$scratch_dir"
```

Sai mode CLI (`json`) phải exit `64` và in usage ở stderr. Chỉ exact mode `write-demo` mới đi vào output path; `csv|srec` luôn mở `"r"`.

### 6. Sanitizer và công cụ chẩn đoán

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O1 -g \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  assets/b10_file_demo.c -o b10_file_san
./b10_file_san csv assets/lab06_records.csv
./b10_file_san srec assets/lab06_sample.srec
san_dir=$(mktemp -d)
./b10_file_san write-demo "$san_dir/readiness.txt"
printf 'status=ready\nrecords=2\n' >"$san_dir/expected.txt"
cmp "$san_dir/expected.txt" "$san_dir/readiness.txt"
rm -rf "$san_dir"
```

Pass: exact functional output giữ nguyên, exit `0`, không ASan/UBSan diagnostic. Sanitizer không chứng minh parser hỗ trợ mọi S-record type; profile rejection là chủ ý.

### 7. Truy vết quyết định và trade-off

| Outline | Quyết định/evidence trong case |
|---|---|
| `OUT-B10-01..02` | chỉ contract logical records/path/open errors; không giả định disk blocks/descriptors |
| `OUT-B10-03..04` | return-driven bounded read; initial `ferror` khác empty; checked write/flush/reopen, chỉ dùng `FILE` stream |
| `OUT-B10-05` | CSV/S-record là text; CR/LF được trim trước parse; không dùng string API cho binary payload |
| `OUT-B10-06` | `main` sở hữu read stream; processors borrow; write helper sở hữu hai stream lifetime tuần tự |
| `OUT-B10-07` | sequential position tự advance; EOF/error state được phân biệt |
| `OUT-B10-08` | validator dùng `r`; writer dùng exclusive-create `wx` trên fresh scratch path; NULL/close errors đều được kiểm |
| `OUT-B10-09` | `fgets`+explicit parse; `fprintf` exact return và reopened `fread` exact bytes |
| `OUT-B10-10` | single pass thay seek/backtracking, giữ line number/state đồng bộ |
| `OUT-B10-11` | executable kiểm `fflush` output rồi close/reopen; không `fflush` input stream |
| `OUT-B10-12` | bắt buộc nonempty S1 trước S9; count/length/hex/checksum/sequence; file validation only, no flashing |

Line buffers hữu hạn ngăn unbounded memory use nhưng reject record dài. Single pass dùng ít state nhưng không resolve cross-record address policy. Text mode dễ inspect; một binary format sẽ cần `rb`, exact-count block reads và explicit endian schema. Những trade-off này được chấp nhận vì ticket là local bounded validator.

### 8. Failure modes và troubleshooting

| Dấu hiệu | Nguyên nhân | Chẩn đoán | Sửa | Phòng tránh |
|---|---|---|---|---|
| Record cuối lặp/garbage | loop dùng `!feof` | log read return và indicators | loop trên `fgets` return, rồi `ferror` | fixtures empty/no-final-newline |
| Empty bị báo resource error hoặc ngược lại | không kiểm `ferror` sau initial `fgets==NULL` | chạy empty file và Linux directory-read đối chứng | `ferror`→exit `3`, otherwise empty→exit `2` | khóa exact exit/channel oracles |
| Long line thành hai records | không kiểm newline/completeness | fixture tại capacity±1 | reject/consume theo stated policy | fixed cap + boundary tests |
| CSV suffix bị bỏ qua | formatted conversion/parse partial | log unconsumed substring | require exactly two commas và consumed-all fields | malformed fixture matrix |
| Checksum khác theo host | newline/text bytes tính nhầm | dump decoded hex pairs và running sum | trim record boundary; sum decoded bytes | golden good/bad vectors |
| Record sau S9 vẫn pass | thiếu state-machine check | fixture S9 rồi S1 | reject next line | sequencing tests |
| S9-only vẫn pass | chỉ kiểm termination/checksum, không track S1 | fixture chỉ `S9030000FC` | require nonempty S1 before S9 | S9-only + empty-S1 tests |
| Báo write thành công nhưng file sai | bỏ return/flush/close/reopen check | exact length+`cmp`, inspect stderr | fail-fast trên `fprintf`/`fflush`/`fclose`/`fread` | fresh scratch + reopen oracle |
| “File pass nên flash được” | nhầm integrity với authenticity/deployment | review scope/evidence | dừng workflow, thiết kế separate authorized validator | label output/file-validation-only, không hardware APIs |
| Resource leak/double close | ownership mơ hồ | trace open/close, Valgrind | single owner/cleanup | borrowed-stream API contract |

### 9. Kết quả đã tái lập trên baseline

Trong container Ubuntu 22.04 với GCC 11.4:

- strict build: exit `0`, zero warnings;
- CSV happy: exact stdout, exit `0`;
- S-record happy: exact stdout, exit `0`;
- bad checksum: exit `2`, stdout rỗng, exact stderr;
- S9-only: exit `2`, stdout rỗng, exact `ERROR S9 termination precedes S1 data at line 1`;
- empty CSV exit `2`; Linux/glibc initial directory-read error exit `3`, hai stderr khác nhau;
- write-demo: exit `0`, exact `bytes=23 flush=ok reopen=match`, reopened content match;
- ASan+UBSan: không diagnostic thuộc các fixtures.

## Practice Time — độc lập, không chấm điểm

Tạo hai **temporary local input files** mới; không sửa fixtures và không sửa source.

`practice.csv`:

```csv
id,value,flags
10,12,0x01
11,20,0x04
```

Exact oracle:

```text
OK csv records=2 value_sum=32 flags_or=0x05
```

`practice.srec`:

```text
S1050020AA55DB
S9030020DC
```

Exact oracle:

```text
OK srec records=2 data_bytes=2 start=0020
```

Yêu cầu:

1. Strict build và chạy cả hai inputs; xác minh exit `0`, stderr rỗng, stdout exact bằng `diff`.
2. Tạo bản copy S-record đổi checksum `DB` thành `DA`; oracle: exit `2`, stdout rỗng, exact stderr `ERROR S-record checksum mismatch at line 1`.
3. Tạo CSV có field thứ tư; oracle: exit `2`, stdout rỗng, exact stderr `ERROR invalid CSV record at line 2`.
4. Chạy `write-demo` với một fresh scratch path khác; oracle `OK write-demo bytes=23 flush=ok reopen=match`, content exact `status=ready\nrecords=2\n`. Không dùng fixture path.
5. Tạo S9-only input; oracle exit `2`, stdout rỗng, stderr `ERROR S9 termination precedes S1 data at line 1`.
6. Vẽ ownership/state trace cho cả read và write→flush→close→reopen; ghi rõ vì sao không `fflush(stdin)` và vì sao flush không đồng nghĩa physical durability.
7. Viết câu kết luận: evidence này xác nhận gì về file format/local I/O và **không** xác nhận gì về hardware/flashing.

Acceptance chỉ gồm input/oracles/analysis; không có solution implementation mới và không có thao tác thiết bị.

## Provenance của các case

- `SRC-C17-ISO`, `SRC-C17-WG14`: C17 file/stream semantics.
- `SRC-GLIBC235`: stdio behavior trên baseline glibc 2.35.
- `SRC-CERTC`: file/input validation và resource handling guidance.
- `SRC-MOTOROLA-SREC`: S-record field/count/checksum overview; profile Unit nhỏ hơn format đầy đủ.
