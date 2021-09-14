<div dir="rtl" style="text-align: justify;">

# گزارش تمرین گروهی ۳

گروه ۳
-----

عرشیا اخوان <letmemakenewone@gmail.com>

محمدرضا عبدی <reza_abdi20@yahoo.com>

احمد سلیمی <ahsa9978@gmail.com> 

امیرمهدی نامجو <amirm137878@gmail.com>

بافر کش
====================
برای تست‌های جدید، تعدادی فراخوانی سیستمی به سیستم‌عامل اضافه شده است:

`sys_cache_inv`
این فراخوانی، ابتدا تمامی بلوک‌هایی که در کش کثیف هستند را در دیسک می‌نویسد، سپس آنها را غیر معتبر می‌کند.

`sys_cache_spec`
این فراخوانی با توجه به پرچم‌های داده شده، اطلاعاتی نظیر تعداد hit ها، تعداد miss ها، تعداد کل خواندن از دیسک و تعداد کل نوشتن روی دیسک به ما می‌دهد.

همچنین مجموعه‌ای از توابع را برای استفاده از فراخوانی‌های سیستمی ذکر شده، پیاده سازی کرده‌ایم.


 تابع 
`cache_flush`
نیز پیاده‌سازی شده است.
این تابع تمامی بلوک‌های کثیف کش را در دیسک می‌نویسد
تا هنگام خاموش شدن سیستم، از پایداری داده‌ها در دیسک مطمئن شویم.

تابع 
`cache_invalidate`
نیز پیاده‌سازی شده است.
این تابع، ابتدا
`cache_flush`
را صدا می‌زند سپس تمامی بلوک‌ها را غیر معتبر می‌کند.

> یکی از سنجه‌های خوب برای آزمودن عملکرد کش، تست
`multi-oom`
است. این تست قبل از پیاده سازی کش، بیش از 160 ثانیه به طول می‌انجامید. اما اکنون حدود 5 ثانیه زمان می‌برد.


پرونده‌های توسعه پذیر
====================
در این بخش تغییری نسبت به سند طراحی وجود ندارد.


پوشه
====
به توصیفگر پرونده هر ریسه، ساختار 
`struct dir`
را نیز اضافه کردیم، تا بتوانیم عملیات‌های مرتبط با پوشه را مدیریت کنیم.

یکسری توابع جدید مانند
`split_path`
و
`open_dir_path`
به فایل 
`directory.c`
اضافه شده‌اند که به ترتیب برای جدا کردن نام پرونده از مسیر آن و باز کردن یک پوشه در یک مسیر تو در تو می‌باشد.

فرایند تخصیص 
working directory
به هر ریسه، در تابع 
`start_process`
و از روی 
working directory
ریسه پدر هنگام ساخت صورت می‌گیرد.

تست‌های افزون بر طراحی
====================

> توضیح دهید تست شما دقیقا چه چیزی را تست می کند.

۲ تست نوشته شده، به ترتیب تست‌های توصیف شده در سند فاز ۳ را تست می‌کنند:

 _در این سناریو اثربخشی حافظه نهان بافر خود را با استفاده از حساب کردن rate hit می سنجید. ابتدا حافظه نهان بافر را خالی
کنید سپس یک فایل را باز کنید و به صورت ترتیبی آن را بخوانید.تا مقدار rate hit را برای یک حافظه نهان بافر خالی بدست
آورید.پس از آن فایل را ببنید و دوباره آن را باز کرده و به همان صورت بخوانید تا مطمئن شوید rate hit بهبود یافته است._

 _در این سناریو توانایی حافظه نهان بافر خود را در ادغام و یکی کردن تغییرات بر روی یک sector را ارزیابی می کنید. به این صورت
که هر block حافظه دو شمارش گر cnt_read و cnt_write را نگه داری می کند. حال شروع به تولید و نوشتن یک فایل بزرگ به
صورت بایت به بایت کنید(حجم فایل تولید شده بیش از ۶۴ کیلوبایت باشد که دوبرابر اندازه بیشینه حافظه نهان بافر است). سپس
فایل را به صورت بایت به بایت بخوانید در صورت صحت کارکرد حافظه نهان بافر تعداد نوشتن بر روی دیسک باید در حدود ۱۲۸
مورد باشد(بدلیل اینکه ۶۴ کیلوبایت دارای ۱۲۸ block است.)_

> توضیح دهید چگونه این امر را تست می کنید و همینطور به صورت کیفی بیان کنید که خروجی مورد نظر باید چگونه باشد تا تست
پاس شود.

 در تست نخست، یک مجموعه عملیات خواندن از روی دیسک را دوبار انجام داده و در هر مرحله، 
 hit rate
 را محاسبه می‌کند و بررسی می‌کند که آیا بهتر شده است یا خیر.

 در تست دوم، به تعداد 
 64K،
 بایت در دیسک می نویسیم.
 از آنجایی که این بایت‌ها پشت سر هم قرار دارند و هر 
 512
 بایت در یک بلاک کش قرار گرفته است، تعداد عملیات‌های نوشتن روی دیسک برابر با تعداد بلوک‌های استفاده شده در کش بوده
 (128)
 و پس از دریافت دفعات نوشتن روی دیسک، باید با عددی حدود 
 128
 مواجه شویم.

> خروجی خود هسته وقتی تست اجرا می شود و همچنین خروجی تست را در این بخش بیاورید.

خروجی مربوط به تست اول
(`cache-hit`):

<div dir="ltr">

```log
Copying tests/filesys/extended/cache-hit to scratch partition...
Copying tests/filesys/extended/tar to scratch partition...
qemu-system-i386 -device isa-debug-exit -hda /tmp/7YikWgDwLn.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading............
Kernel command line: -q -f extract run cache-hit
Pintos booting with 3,968 kB RAM...
367 pages available in kernel pool.
367 pages available in user pool.
Calibrating timer...  838,860,800 loops/s.
hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
hda1: 192 sectors (96 kB), Pintos OS kernel (20)
hda2: 242 sectors (121 kB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'cache-hit' into the file system...
Putting 'tar' into the file system...
Erasing ustar archive...
Executing 'cache-hit':
(cache-hit) begin
(cache-hit) create "cache"
(cache-hit) open "cache"
(cache-hit) creating cache
(cache-hit) close "tmp"
(cache-hit) invalidating cache
(cache-hit) open "cache"
(cache-hit) read tmp
(cache-hit) close "cache"
(cache-hit) open "cache"
(cache-hit) read "cache"
(cache-hit) close "cache"
(cache-hit) new hit rate > old hit rate
(cache-hit) end
cache-hit: exit(0)
Execution of 'cache-hit' complete.
Timer: 80 ticks
Thread: 4 idle ticks, 73 kernel ticks, 3 user ticks
hdb1 (filesys): 629 reads, 541 writes
hda2 (scratch): 241 reads, 2 writes
Console: 1316 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
```
</div>

نتیجه تست اول:

<div dir="ltr">

```log
PASS
```
</div>

خروجی مربوط به تست دوم
(`cache-write`):

<div dir="ltr">

```log
Copying tests/filesys/extended/cache-write to scratch partition...
Copying tests/filesys/extended/tar to scratch partition...
qemu-system-i386 -device isa-debug-exit -hda /tmp/web9GKsrec.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading............
Kernel command line: -q -f extract run cache-write
Pintos booting with 3,968 kB RAM...
367 pages available in kernel pool.
367 pages available in user pool.
Calibrating timer...  754,483,200 loops/s.
hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
hda1: 192 sectors (96 kB), Pintos OS kernel (20)
hda2: 240 sectors (120 kB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'cache-write' into the file system...
Putting 'tar' into the file system...
Erasing ustar archive...
Executing 'cache-write':
(cache-write) begin
(cache-write) create "cache"
(cache-write) open "cache" for writing
(cache-write) writing 64kB to "cache"
(cache-write) close "cache" after writing
(cache-write) open "cache" for read
(cache-write) total number of writes is close to 128
(cache-write) end
cache-write: exit(0)
Execution of 'cache-write' complete.
Timer: 349 ticks
Thread: 23 idle ticks, 67 kernel ticks, 259 user ticks
hdb1 (filesys): 787 reads, 618 writes
hda2 (scratch): 239 reads, 2 writes
Console: 1249 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
```
</div>

نتیجه تست دوم:

<div dir="ltr">

```log
PASS
```
</div>


> دو ایراد بالقوه و غیر بدیهی هسته را بیان کنید و نشان دهید که در صورت بروز این خطا تست شما چه خروجی خواهد داشت.گزارش
شما باید به این صورت باشد:
“اگر هسته X را به جای Y انجام دهد آنگاه خروجی تست Z خواهد بود.“
شما باید برای هر تست دو ایراد مجزا پیدا کنید اما ایراد های دو تست می تواند یکسان باشد یا اشتراک داشته باشند.لازم به ذکر
است که ایراد بیان شده باید مربوط به سناریو تست شما باشد به عنوان مثال سناریو زیر قابل قبول نیست: اگر هسته خطای نحوی
۱۳ داشته باشد آنگاه تست اجرا نمی شود.

ایرادات مرتبط با تست اول (`cache_hit`):

1. اگر پیاده‌سازی سامانه پرونده‌ها به نادرستی صورت گیرد، ممکن است فراخوانی‌های سیستمی 
`read`
و
`write`
با مشکل مواجه شده و تعداد بایت‌های درستی را خروجی ندهند که در این صورت تست ما رد می‌شود.

2. در صورت نبود کش و یا عملکرد نادرست آن، عدد بدست آمده برای اولین 
hit rate
از دومین
hit rate
کمتر نخواهد بود.

ایرادات مرتبط با تست دوم (`cache_write`):

1. اگر پیاده‌سازی سامانه پرونده‌ها به نادرستی صورت گیرد، ممکن است فراخوانی‌های سیستمی 
`read`
و
`write`
با مشکل مواجه شده و تعداد بایت‌های درستی را خروجی ندهند که در این صورت تست ما رد می‌شود.

2. اگر رویکرد کش در هسته برای نوشتن روی دیسک 
write through
باشد، 
به ازای هر فراخوانی سیستمی
`write`
که با ورودی یک بایت فراخوانی می‌شود، ما یک عملیات نوشتن روی دیسک خواهیم داشت که تعداد کل عملیات‌های نوشتن روی دیسک را از 
128
به
128 * 512
تغییر خواهد داد.
بنابراین تست ما رد خواهد شد.

سوالات افزون بر طراحی
====================

- بیان کنید که هر فرد گروه دقیقا چه بخشی را انجام داد؟ آیا این کار را به صورت مناسب انجام دادید و چه کارهایی برای بهبود
عملکردتان می توانید انجام دهید.
  > همه کار ها به صورت جلسات ۴ نفره در
  vscode live share
   انجام شد و همه افراد گروه در تمام مسیر پیشرفت پروژه دخیل بوده‌اند.

- آیا کد شما مشکل بزرگی امنیتی در بخش حافظه دارد
(به طور خاص رشته‌ها در زبان C؟ )
memory leak
 و نحوه مدیریت ضعیف خطاها نیز بررسی خواهد شد.
  > کدهای اضافه شده توسط گروه هیچ مشکل بزرگ امنیتی جدیدی اضافه نکرده است.
  > در جواب سوال دوم نیز باید گفت که تست multi-oom را پاس کرده‌ایم. این تست نیاز به این دارد که فضاهای اضافی را که گرفته‌ایم تماماْ آزاد کرده باشیم. چون این تست سعی‌ می‌کند که ده بار، تعداد حداقل ۳۰ ترد را بسازد و انتظار دارد که هر دفعه بتواند دقیقا به اندازه دفعه قبلی کار را ادامه بدهد. بدین ترتیب اطمینان حاصل می‌کند که هیچ فضای اضافی از قبل آزاد نشده باقی نمانده باشد. کد ما می‌تواند تا ۶۰ ترد در هر بار اجرا را پشتیبانی کند. ضمن این که این تردها فایل هم باز می‌کنند و مشکلی پیش نمی‌آید. همچنین برای اجرای درست این تست باید امن ریسه بودن فایل‌ها هم تضمین بشود. همچنین در صورتی که یک کد page-fault بخورد باید قفل فایل سیستم را در صورتی که در اختیار دارد آزاد کند. کد ما همه این موارد را رعایت کرده است.
  
- آیا از یک Style Code واحد استفاده کردید؟ آیا style مورد استفاده توسط شما با pintos هم خوانی دارد؟(از نظر
فرورفتگی و نحوه نام گذاری)

  > بله

- آیا کد شما ساده و قابل درک است؟

  > بله، تلاش کرده‌ایم تا سطح سادگی و قابل فهم بودن کد در تمام کدبیس 
  pintos
  یکسان باشد.

- آیا کد پیچیده ای در بخشی از کدهای خود دارید؟ در صورت وجود آیا با قرار دادن توضیحات مناسب آن را قابل فهم کردید؟

  > خیر

- آیا کد Comment شده ای در کد نهایی خود دارید؟
آیا کدی دارید که کپی کرده باشید؟

  > خیر

- آیا الگوریتم های list linked را خودتان پیاده سازی کردید یا از پیاده سازی موجود استفاده کردید؟

  > خیر، از پیاده‌سازی موجود استفاده کردیم.

- آیا طول خط کدهای شما بیش از حد زیاد است؟ (۱۰۰ کاراکتر)

  > خیر

- آیا در git خودتان پرونده های binary حضور دارند؟(پرونده های binary و پرونده های log را commit نکنید!)
  > خیر


</div>