<div dir="rtl" style="text-align: justify;">

تمرین گروهی ۱/۰ - آشنایی با pintos
======================

شماره گروه:
-----
> نام و آدرس پست الکترونیکی اعضای گروه را در این قسمت بنویسید.

عرشیا اخوان <letmemakenewone@gmail.com>

محمدرضا عبدی <reza_abdi20@yahoo.com>

احمد سلیمی <ahsa9978@gmail.com> 

امیرمهدی نامجو <amirm137878@gmail.com> 

مقدمات
----------
> اگر نکات اضافه‌ای در مورد تمرین یا برای دستیاران آموزشی دارید در این قسمت بنویسید.


> لطفا در این قسمت تمامی منابعی (غیر از مستندات Pintos، اسلاید‌ها و دیگر منابع  درس) را که برای تمرین از آن‌ها استفاده کرده‌اید در این قسمت بنویسید.

آشنایی با pintos
============
>  در مستند تمرین گروهی ۱۹ سوال مطرح شده است. پاسخ آن ها را در زیر بنویسید.


## یافتن دستور معیوب

<div dir="ltr">

```diff
FAIL
Test output failed to match any acceptable form.

Acceptable output:
  do-nothing: exit(162)
Differences in `diff -u' format:

- do-nothing: exit(162)
+ Page fault at 0xc0000008: rights violation error reading page in user context.
+ do-nothing: dying due to interrupt 0x0e (#PF Page-Fault Exception).
+ Interrupt 0x0e (#PF Page-Fault Exception) at eip=0x8048757
+  cr2=c0000008 error=00000005
+  eax=00000000 ebx=00000000 ecx=00000000 edx=00000000
+  esi=00000000 edi=00000000 esp=bfffffe4 ebp=00000000
+  cs=001b ds=0023 es=0023 ss=0023
```

</div>

۱. `Page fault at 0xc0000008`

۲.`eip=0x8048757`

۳. `_start`

<div dir="ltr">

```x86asm
mov    0x24(%esp),%eax
```
</div>

۴. 

## کد C

<div dir="ltr">

```c 
#include <syscall.h>

int main (int, char *[]);
void _start (int argc, char *argv[]);

void 
_start (int argc, char *argv[])
{
    exit (main (argc, argv));
}
```

</div>

## کد Assembly


<div dir="ltr">

```x86asm
8048754 <_start>:
8048754:	83 ec 1c             	sub    $0x1c,%esp
8048757:	8b 44 24 24          	mov    0x24(%esp),%eax
804875b:	89 44 24 04          	mov    %eax,0x4(%esp)
804875f:	8b 44 24 20          	mov    0x20(%esp),%eax
8048763:	89 04 24             	mov    %eax,(%esp)
8048766:	e8 35 f9 ff ff       	call   80480a0 <main>
804876b:	89 04 24             	mov    %eax,(%esp)
804876e:	e8 49 1b 00 00       	call   804a2bc <exit>
```

</div>


- این دستور پشته را به اندازه مورد نیاز رزرو می‌کند. ۸ بایت برای ورودی‌های
callee
ها، و ۱۶ باید برای دستور
call
و ۴ بایت برای
align
کردن پشته مورد نیاز است.
(باید توجه کرد که پیش از فراخوانی تابع، باید ۱۶ بایت خالی در نظر بگیریم.)
<div dir="ltr">

```x86asm
sub    $0x1c,%esp
```
</div>

- این دستور پارامتر
argc
را در رجیستر
eax%
ذخیره می‌کند.

<div dir="ltr">

```x86asm
mov    0x24(%esp),%eax
```
</div>

- این دستور رجیستر
eax%
را به عنوان پارامتر
argc
تابع
main
در پشته ذخیره می‌کند.

<div dir="ltr">

```x86asm
mov    %eax,0x4(%esp)
```
</div>

- این دستور پارامتر
argv
را در رجیستر
eax%
ذخیره می‌کند.

<div dir="ltr">

```x86asm
mov    0x20(%esp),%eax
```
</div>

- این دستور رجیستر
eax%
را به عنوان پارامتر
argv
تابع
main
در پشته ذخیره می‌کند.

<div dir="ltr">

```x86asm
mov    %eax,(%esp)
```
</div>

- این دستور تابع
main
را فراخوانی می‌کند و
return address
را در پشته ذخیره می‌کند و به اولین دستور 
callee
پرش می کند.
<div dir="ltr">

```x86asm
call   80480a0 <main>
```
</div>


- تابع
main
خروجی خود را در رجیستر
eax%
ذخیره می‌کند. این دستور، این مقدار را بعنوان پارامتر تابع
exit
در پشته ذخیره می‌کند.

<div dir="ltr">

```x86asm
mov    %eax,(%esp)
```
</div>

- این دستور تابع
exit
 فراخوانی می‌کند.

<div dir="ltr">

```x86asm
call   804a2bc <exit>
```
</div>

۵.
این دسترسی به خاطر این می‌باشد که موقع اجرا تابع 
`start_`
با هیچ ورودی‌ای صدا زده نمی‌شود. 
به همین جهت مقادیر argc و argv در پشته وجود ندارد و 
هنگامی که به دستور
`mov    0x24(%esp),%eax`
می‌رسیم, تلاش می‌کنیم تا از پشته به اندازه سایز argc + argv بخوانیم و از آنجایی که argc و argv در پشته وجود ندارند, باعث می‌شود تا از آدرس
`0xc0000008`
(که دامنه
user mode page
(برگهٔ حالت کاربر)
است.)
خارج شویم و با خطای write violation 
(تجاوز به راستی ها :))
مواجه شویم.



## به سوی crash

۶.

۷.

۸.

۹.

۱۰.

۱۱.

۱۲.

۱۳.


## دیباگ

۱۴.

۱۵.

۱۶.

۱۷.

۱۸.

۱۹.

</>