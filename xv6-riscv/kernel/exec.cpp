#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"

// Prototypes this file expects in defs.h (const-correct versions):
//   int      kexec(const char *path, const char *const *argv);
//   struct inode *namei(const char *path);
//   int      copyout(pagetable_t, uint64 dstva, const char *src, uint64 len);
//   void     panic(const char *) __attribute__((noreturn));
//   char    *safestrcpy(char *, const char *, int);

namespace {

// ELF p_flags bits.
constexpr int ELF_FLAG_EXEC = 0x1;
constexpr int ELF_FLAG_WRITE = 0x2;

// Map ELF segment permissions to PTE permission bits.
// (uvmalloc() adds PTE_R | PTE_U itself.)
constexpr int
flags2perm(int flags)
{
  return ((flags & ELF_FLAG_EXEC) ? PTE_X : 0) |
         ((flags & ELF_FLAG_WRITE) ? PTE_W : 0);
}

// Owns the new user page table until exec() commits to it.
// If we bail out early, the destructor frees it.
struct NewImage {
  pagetable_t pagetable = nullptr;
  uint64 sz = 0;

  NewImage() = default;
  NewImage(const NewImage &) = delete;
  NewImage &operator=(const NewImage &) = delete;

  ~NewImage()
  {
    if (pagetable)
      proc_freepagetable(pagetable, sz);
  }

  // Give up ownership (called when committing to the new image).
  pagetable_t release()
  {
    pagetable_t pt = pagetable;
    pagetable = nullptr;
    return pt;
  }
};

// Holds the log transaction and the inode lock for the executable.
// release() is idempotent, so the happy path can drop both early.
class InodeGuard {
public:
  explicit InodeGuard(struct inode *ip) : ip_(ip) {}
  InodeGuard(const InodeGuard &) = delete;
  InodeGuard &operator=(const InodeGuard &) = delete;
  ~InodeGuard() { release(); }

  void release()
  {
    if (ip_) {
      iunlockput(ip_);
      end_op();
      ip_ = nullptr;
    }
  }

private:
  struct inode *ip_;
};

// Read one fixed-size object from the inode at byte offset off.
template <typename T>
bool
read_obj(struct inode *ip, uint off, T &obj)
{
  return readi(ip, 0, reinterpret_cast<uint64>(&obj), off, sizeof(T)) ==
         static_cast<int>(sizeof(T));
}

// Load an ELF program segment into pagetable at virtual address va.
// va must be page-aligned, and the pages from va to va+sz must already
// be mapped.
bool
loadseg(pagetable_t pagetable, uint64 va, struct inode *ip, uint offset,
        uint sz)
{
  for (uint i = 0; i < sz; i += PGSIZE) {
    uint64 pa = walkaddr(pagetable, va + i);
    if (pa == 0)
      panic("loadseg: address should exist");
    uint n = (sz - i < PGSIZE) ? sz - i : PGSIZE;
    if (readi(ip, 0, pa, offset + i, n) != static_cast<int>(n))
      return false;
  }
  return true;
}

// Validate the ELF file and map all of its PT_LOAD segments into img.
bool
load_elf(struct proc *p, struct inode *ip, NewImage &img, uint64 &entry)
{
  elfhdr elf;
  if (!read_obj(ip, 0, elf))
    return false;
  if (elf.magic != ELF_MAGIC)
    return false;

  img.pagetable = proc_pagetable(p);
  if (img.pagetable == nullptr)
    return false;

  uint off = static_cast<uint>(elf.phoff);
  for (int i = 0; i < elf.phnum; i++, off += sizeof(proghdr)) {
    proghdr ph;
    if (!read_obj(ip, off, ph))
      return false;
    if (ph.type != ELF_PROG_LOAD)
      continue;
    if (ph.memsz < ph.filesz)
      return false;
    if (ph.vaddr + ph.memsz < ph.vaddr)
      return false;
    if (ph.vaddr % PGSIZE != 0)
      return false;

    uint64 newsz = uvmalloc(img.pagetable, img.sz, ph.vaddr + ph.memsz,
                            flags2perm(ph.flags));
    if (newsz == 0)
      return false;
    img.sz = newsz;

    if (!loadseg(img.pagetable, ph.vaddr, ip, ph.off, ph.filesz))
      return false;
  }

  entry = elf.entry;
  return true;
}

// Allocate the user stack above the image and push argv onto it.
// On success, argc and sp are the values for the new process.
bool
push_args(NewImage &img, const char *const *argv, uint64 &argc, uint64 &sp_out)
{
  // Allocate some pages at the next page boundary.
  // Make the first inaccessible as a stack guard.
  // Use the rest as the user stack.
  img.sz = PGROUNDUP(img.sz);
  uint64 newsz =
    uvmalloc(img.pagetable, img.sz, img.sz + (USERSTACK + 1) * PGSIZE, PTE_W);
  if (newsz == 0)
    return false;
  img.sz = newsz;
  uvmclear(img.pagetable, img.sz - (USERSTACK + 1) * PGSIZE);

  uint64 sp = img.sz;
  const uint64 stackbase = sp - USERSTACK * PGSIZE;

  // Copy argument strings onto the new stack and remember their
  // addresses in ustack[]. One extra slot for the terminating null.
  uint64 ustack[MAXARG + 1];
  uint64 n = 0;
  for (; argv[n]; n++) {
    if (n >= MAXARG)
      return false;
    const uint64 len = strlen(argv[n]) + 1;
    sp -= len;
    sp -= sp % 16; // riscv sp must be 16-byte aligned
    if (sp < stackbase)
      return false;
    if (copyout(img.pagetable, sp, argv[n], len) < 0)
      return false;
    ustack[n] = sp;
  }
  ustack[n] = 0;

  // Push a copy of ustack[], the array of argv[] pointers.
  sp -= (n + 1) * sizeof(uint64);
  sp -= sp % 16;
  if (sp < stackbase)
    return false;
  if (copyout(img.pagetable, sp, reinterpret_cast<const char *>(ustack),
              (n + 1) * sizeof(uint64)) < 0)
    return false;

  argc = n;
  sp_out = sp;
  return true;
}

// Final path component, for p->name.
const char *
basename_of(const char *path)
{
  const char *last = path;
  for (const char *s = path; *s; s++)
    if (*s == '/')
      last = s + 1;
  return last;
}

} // namespace

//
// the implementation of the exec() system call
//
int
kexec(const char *path, const char *const *argv)
{
  struct proc *p = myproc();

  begin_op();
  struct inode *ip = namei(path);
  if (ip == nullptr) {
    end_op();
    return -1;
  }
  ilock(ip);
  InodeGuard file(ip); // declared before img: img is freed first on failure
  NewImage img;

  uint64 entry;
  if (!load_elf(p, ip, img, entry))
    return -1;
  file.release();

  uint64 argc, sp;
  if (!push_args(img, argv, argc, sp))
    return -1;

  // a0 and a1 contain arguments to user main(argc, argv).
  // argc is returned via the system call return value, which goes in a0.
  p->trapframe->a1 = sp;

  // Save program name for debugging.
  safestrcpy(p->name, basename_of(path), sizeof(p->name));

  // Commit to the user image.
  pagetable_t oldpagetable = p->pagetable;
  uint64 oldsz = p->sz;
  p->sz = img.sz;
  p->pagetable = img.release();
  p->trapframe->epc = entry; // initial program counter = ulib.c:start()
  p->trapframe->sp = sp;     // initial stack pointer
  proc_freepagetable(oldpagetable, oldsz);

  return static_cast<int>(argc); // ends up in a0, the first argument to main
}
