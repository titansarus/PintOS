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

- `main`, `0xc000e000`

- `idle`

<div dir="ltr">

```log
pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_RUNNING, name = "main", '\000' <repeats 11 times>, stack = 0xc000edec <incomplete sequence \357>, priority = 31, allelem = {prev =                                                                                                                                                                 0c0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020                                                                                                                                                                               ,18 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```

</div>

۷.

<div dir = "ltr">

```log
#0  process_execute (file_name=file_name@entry=0xc0007d50 "do-nothing") at ../../userprog/process.c:36
#1  0xc0020268 in run_task (argv=0xc00357cc <argv+12>) at ../../threads/init.c:288
#2  0xc0020921 in run_actions (argv=0xc00357cc <argv+12>) at ../../threads/init.c:340
#3  main () at ../../threads/init.c:133
```

```c
tid_t
process_execute (const char *file_name)
{
  char *fn_copy;
  tid_t tid;

  sema_init (&temporary, 0);
  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
  if (tid == TID_ERROR)
    palloc_free_page (fn_copy);
  return tid;
}
```

```c
static void
run_task (char **argv)
{
  const char *task = argv[1];

  printf ("Executing '%s':\n", task);
#ifdef USERPROG
  process_wait (process_execute (task));
#else
  run_test (task);
#endif
  printf ("Execution of '%s' complete.\n", task);
}

```

```c
static void
run_actions (char **argv)
{
  /* An action. */
  struct action
    {
      char *name;                       /* Action name. */
      int argc;                         /* # of args, including action name. */
      void (*function) (char **argv);   /* Function to execute action. */
    };

  /* Table of supported actions. */
  static const struct action actions[] =
    {
      {"run", 2, run_task},
#ifdef FILESYS
      {"ls", 1, fsutil_ls},
      {"cat", 2, fsutil_cat},
      {"rm", 2, fsutil_rm},
      {"extract", 1, fsutil_extract},
      {"append", 2, fsutil_append},
#endif
      {NULL, 0, NULL},
    };

  while (*argv != NULL)
    {
      const struct action *a;
      int i;

      /* Find action name. */
      for (a = actions; ; a++)
        if (a->name == NULL)
          PANIC ("unknown action `%s' (use -h for help)", *argv);
        else if (!strcmp (*argv, a->name))
          break;

      /* Check for required arguments. */
      for (i = 1; i < a->argc; i++)
        if (argv[i] == NULL)
          PANIC ("action `%s' requires %d argument(s)", *argv, a->argc - 1);

      /* Invoke action and advance. */
      a->function (argv);
      argv += a->argc;
    }

}
```


```c
int
main (void)
{
  char **argv;

  /* Clear BSS. */
  bss_init ();

  /* Break command line into arguments and parse options. */
  argv = read_command_line ();
  argv = parse_options (argv);

  /* Initialize ourselves as a thread so we can use locks,
     then enable console locking. */
  thread_init ();
  console_init ();

  /* Greet user. */
  printf ("Pintos booting with %'"PRIu32" kB RAM...\n",
          init_ram_pages * PGSIZE / 1024);

  /* Initialize memory system. */
  palloc_init (user_page_limit);
  malloc_init ();
  paging_init ();

  /* Segmentation. */
#ifdef USERPROG
  tss_init ();
  gdt_init ();
#endif

  /* Initialize interrupt handlers. */
  intr_init ();
  timer_init ();
  kbd_init ();
  input_init ();
#ifdef USERPROG
  exception_init ();
  syscall_init ();
#endif

  /* Start thread scheduler and enable interrupts. */
  thread_start ();
  serial_init_queue ();
  timer_calibrate ();

#ifdef FILESYS
  /* Initialize file system. */
  ide_init ();
  locate_block_devices ();
  filesys_init (format_filesys);
#endif

  printf ("Boot complete.\n");

  /* Run actions specified on kernel command line. */
  run_actions (argv);

  /* Finish up. */
  shutdown ();
  thread_exit ();
}
```

</div>

۸.

- `main`, `idle`, `do-nothing`

<div dir="ltr">

```log

pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_BLOCKED, name = "main", '\000' <repeats 11 times>, stack = 0xc000eeac "\001", priority = 31, allelem = {prev = 0xc0035910 <all_list>,
 next = 0xc0104020}, elem = {prev = 0xc0037314 <temporary+4>, next = 0xc003731c <temporary+12>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc010a0
20}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #2: 0xc010a000 {tid = 3, status = THREAD_RUNNING, name = "do-nothing\000\000\000\000\000", stack = 0xc010afd4 "", priority = 31, allelem = {prev = 0xc0104020, next = 0xc00359
18 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}

```

</div>

۹.

<div dir="ltr">

```c
tid_t
process_execute (const char *file_name)
{
  char *fn_copy;
  tid_t tid;

  sema_init (&temporary, 0);
  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
  if (tid == TID_ERROR)
    palloc_free_page (fn_copy);
  return tid;
}
```

</div>

۱۰.

<div dir="ltr">

```x86asm
esp = 0xc0000000
eip = 0x8048754
```

</div>

۱۱.

تابع
`start_process`
بصورت دستی، به پردازنده می‌گوید که از سمت کد کاربر، یک وقفه آمده بوده که
kernel
در حال هندل کردن آن بوده است. (در عمل چنین چیزی وجود نداشته، اما چون
x86
راه مستقیمی برای
context switch
به حالت کاربر ندارد، کرنل به این صورت پردازنده را گول می‌زند.)
در نتیجه استراکت
`ـif`
را می‌سازد و رجیسترها را در آن ذخیره می‌کند. سپس با استفاده از
`asm volatile`
و با توجه به این که آدرس پشته را برابر آدرس این استراکت قرار می‌دهد، تابع
`intr_exit`
را در
`intr-stubs.S`
فراخوانی می‌کند.
این تابع نیز 
user state
را از پشته می‌خواند و با استفاده از
`iret`
که دستوری برای بازگشت از وقفه است، به محیط کاربر باز می‌گردد، و این گونه کد کاربر اجرا می‌شود.


۱۲.

<div dir="ltr">

```x86asm
eax            0x0      0
ecx            0x0      0
edx            0x0      0
ebx            0x0      0
esp            0xc0000000       0xc0000000
ebp            0x0      0x0
esi            0x0      0
edi            0x0      0
eip            0x8048754        0x8048754
eflags         0x202    [ IF ]
cs             0x1b     27
ss             0x23     35
ds             0x23     35
es             0x23     35
fs             0x23     35
gs             0x23     35
```

</div>

این مقادیر مشابه مقادیر
`_if`
هستند. (همانطور که انتظار هم داشتیم.)

۱۳.

<div dir="ltr">

```log
#0  _start (argc=<unavailable>, argv=<unavailable>) at ../../lib/user/entry.c:9
```

</div>


## دیباگ

۱۴.

۱۵.

۱۶.

۱۷.

۱۸.

۱۹.

</div>