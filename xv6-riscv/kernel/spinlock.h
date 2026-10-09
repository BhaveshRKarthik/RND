// Mutual exclusion lock.
struct spinlock {
  uint locked; // Is the lock held?

  // For debugging:
  const char *name;      // Name of lock.
  struct cpu *cpu; // The cpu holding the lock.
};
