<div dir="rtl" style="text-align: justify;">

# گزارش تمرین گروهی ۱.۱

گروه ۳
-----

عرشیا اخوان <letmemakenewone@gmail.com>

محمدرضا عبدی <reza_abdi20@yahoo.com>

احمد سلیمی <ahsa9978@gmail.com> 

امیرمهدی نامجو <amirm137878@gmail.com> 

پاس‌دادن آرگومان
============
تغییرات نسبت به سند طراحی اولیه
----------------
- از داده ساختار 
args
 استفاده ای نشده است، 
 به دلیل اینکه بلافاصله پس از 
  tokenize
  کردن نام فایل,
  آن را مستقیما در پشته کاربر ذخیره کرده و نیازی به ساختن آرایه از رشته 
  tokenize
  شده نداشتیم.


داده‌ساختار‌ها
----------------
NULL

نحوه پیاده سازی
----------------
- ابتدا در تابع 
`start_process`،
رشته ورودی را
tokenize
کرده و به تابع
`push_args`
پاس می‌دهیم.
خروجی تابع
`push_args`
پیکانی به آرایه
`argv`
در پشته کاربر می‌باشد.
پس از انجام
stack alignment
مقدار
`argv`
و
`argc`
به سر پشته هل می‌دهیم.

- تابع 
`push_args` 
مطابق جدول زیر، ابتدا
esp
را به مقدار لازم
پایین آورده، سپس با استفاده از
`memcpy`
رشتهٔ
`cmd`
(که
tokenize
شده است و
به جای
space
ها
`NULL`
قرار داده شده است.)
را در پشته ذخیره می‌کنیم.
پس از آن بعنوان آخرین عضو
`argv`
مقدار
`NULL`
در پشته ذخیره می‌کنیم. در ادامه با استفاده از
`argv`
آدرس هر آرگومان را در پشته ذخیره می‌کنیم. سپس پوینتر به اولین
`arg`
را برمی‌گرداند.


<div dir="ltr">

|   Address  |      Name      |    Data    |     Type    |
|:----------:|:--------------:|:----------:|:-----------:|
| 0xbffffffc | argv[3][...]   | bar\0      | char[4]     |
| 0xbffffff8 | argv[2][...]   | foo\0      | char[4]     |
| 0xbffffff5 | argv[1][...]   | -l\0       | char[3]     |
| 0xbfffffed | argv[0][...]   | /bin/ls\0  | char[8]     |
| 0xbfffffec | stack-align    | 0          | uint8_t     |
| 0xbfffffe8 | argv[4]        | 0          | char *      |
| 0xbfffffe4 | argv[3]        | 0xbffffffc | char *      |
| 0xbfffffe0 | argv[2]        | 0xbffffff8 | char *      |
| 0xbfffffdc | argv[1]        | 0xbffffff5 | char *      |
| 0xbfffffd8 | argv[0]        | 0xbfffffed | char *      |
| 0xbfffffd4 | argv           | 0xbfffffd8 | char **     |
| 0xbfffffd0 | argc           | 4          | int         |
| 0xbfffffcc | return address | 0          | void (*) () |

</div>

فراخوانی‌های سیستمی
================
تغییرات نسبت به سند طراحی اولیه
----------------

داده‌ساختار‌ها
----------------

نحوه پیاده سازی برای فراخوانی‌های سیستمی برای کنترل پردازه ها
------------

### 1.exec

### 2.exit

### 3.halt

### 4.wait

### 5.practice

نحوه پیاده سازی برای فراخوانی‌های سیستمی برای عملیات روی پرونده‌ها
------------

### 1.create

### 2.remove

### 3.open

### 4.filesize

### 5.read

### 6.write

### 7.seek

### 8.tell

### 9.close

</div>