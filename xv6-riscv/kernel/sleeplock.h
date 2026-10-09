// Long-term locks for processes
struct sleeplock {
  uint locked;        // Is the lock held?
#ifdef __cplusplus
  mutable 
#endif
  struct spinlock lk; // spinlock protecting this sleep lock

  // For debugging:
  const char *name; // Name of lock.
  int pid;    // Process holding lock
};
