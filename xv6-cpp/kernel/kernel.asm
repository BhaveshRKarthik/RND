
kernel/kernel:     file format elf64-littleriscv


Disassembly of section .text:

0000000080000000 <_entry>:

        # kernel_stack_base is declared in start.cpp,
        # with a KSTACKSIZE-byte stack per CPU.
        # sp = (kernel_stack_base + KSTACKSIZE) + (hartid  * KSTACKSIZE)

        la sp, kernel_stack_base + KSTACKSIZE # top of stack for cpu0
    80000000:	00004117          	auipc	sp,0x4
    80000004:	90010113          	addi	sp,sp,-1792 # 80003900 <kernel_stack_base+0x1000>
        li a0, KSTACKSIZE
    80000008:	6505                	lui	a0,0x1
        csrr a1, mhartid
    8000000a:	f14025f3          	csrr	a1,mhartid
        mul a0, a0, a1
    8000000e:	02b50533          	mul	a0,a0,a1
        add sp, sp, a0
    80000012:	912a                	add	sp,sp,a0

        # jump to start() in start.c
        call start
    80000014:	006000ef          	jal	8000001a <start>

0000000080000018 <spin>:

        # catch the return from start() and spin.
spin:
        j spin
    80000018:	a001                	j	80000018 <spin>

000000008000001a <start>:

        // ask for the very first timer interrupt.
        riscv::stimecmp = riscv::time + 1000000;
    }

    extern "C" void start() {
    8000001a:	1141                	addi	sp,sp,-16
    8000001c:	e422                	sd	s0,8(sp)
    8000001e:	0800                	addi	s0,sp,16
            __asm volatile("csrs %0, %1" : : "i"(csr_addr), "rK"(static_cast<uint64_t>(mask)) : "memory");
        }

        // csr &= mask  (clear every bit NOT in mask)
        void operator&=(flag_type mask) const requires(detail::csr::writable<csr_addr>) {
            __asm volatile("csrc %0, %1" : : "i"(csr_addr), "rK"(~static_cast<uint64_t>(mask)) : "memory");
    80000020:	6789                	lui	a5,0x2
    80000022:	80078793          	addi	a5,a5,-2048 # 1800 <_entry-0x7fffe800>
    80000026:	3007b073          	csrc	mstatus,a5
            __asm volatile("csrs %0, %1" : : "i"(csr_addr), "rK"(static_cast<uint64_t>(mask)) : "memory");
    8000002a:	6785                	lui	a5,0x1
    8000002c:	80078793          	addi	a5,a5,-2048 # 800 <_entry-0x7ffff800>
    80000030:	3007a073          	csrs	mstatus,a5
            __asm volatile("csrw %0, %1" : : "i"(csr_addr), "rK"(static_cast<uint64_t>(value)) : "memory");
    80000034:	00000797          	auipc	a5,0x0
    80000038:	59e78793          	addi	a5,a5,1438 # 800005d2 <_ZN3xv65kmainEv>
    8000003c:	34179073          	csrw	mepc,a5
    80000040:	18005073          	csrwi	satp,0
    80000044:	67c1                	lui	a5,0x10
    80000046:	17fd                	addi	a5,a5,-1 # ffff <_entry-0x7fff0001>
    80000048:	30279073          	csrw	medeleg,a5
    8000004c:	30379073          	csrw	mideleg,a5
            __asm volatile("csrs %0, %1" : : "i"(csr_addr), "rK"(static_cast<uint64_t>(mask)) : "memory");
    80000050:	22000793          	li	a5,544
    80000054:	1047a073          	csrs	sie,a5
            __asm volatile("csrw %0, %1" : : "i"(csr_addr), "rK"(static_cast<uint64_t>(value)) : "memory");
    80000058:	57fd                	li	a5,-1
    8000005a:	00a7d713          	srli	a4,a5,0xa
    8000005e:	3b071073          	csrw	pmpaddr0,a4
    80000062:	3a07d073          	csrwi	pmpcfg0,15
            __asm volatile("csrs %0, %1" : : "i"(csr_addr), "rK"(static_cast<uint64_t>(mask)) : "memory");
    80000066:	4705                	li	a4,1
    80000068:	1776                	slli	a4,a4,0x3d
    8000006a:	30a72073          	csrs	0x30a,a4
    8000006e:	17fe                	slli	a5,a5,0x3f
    80000070:	30a7a073          	csrs	0x30a,a5
    80000074:	30616073          	csrsi	mcounteren,2
            __asm volatile("csrr %0, %1" : "=r"(value) : "i"(csr_addr));
    80000078:	c01027f3          	rdtime	a5
        riscv::stimecmp = riscv::time + 1000000;
    8000007c:	000f4737          	lui	a4,0xf4
    80000080:	24070713          	addi	a4,a4,576 # f4240 <_entry-0x7ff0bdc0>
    80000084:	97ba                	add	a5,a5,a4
            __asm volatile("csrw %0, %1" : : "i"(csr_addr), "rK"(static_cast<uint64_t>(value)) : "memory");
    80000086:	14d79073          	csrw	stimecmp,a5
            __asm volatile("csrr %0, %1" : "=r"(value) : "i"(csr_addr));
    8000008a:	f14027f3          	csrr	a5,mhartid
            uint64_t x;
            __asm volatile("mv %0, tp" : "=r"(x));
            return x;
        }
        void operator=(uint64_t value) const {
            __asm volatile("mv tp, %0" : : "r"(value)); // rK not valid for mv
    8000008e:	823e                	mv	tp,a5

        // keep each CPU's hartid in its tp register, for cpuid().
        riscv::tp = riscv::mhartid;

        // switch to supervisor mode and jump to main().
        __asm volatile("mret");
    80000090:	30200073          	mret
    }
    80000094:	6422                	ld	s0,8(sp)
    80000096:	0141                	addi	sp,sp,16
    80000098:	8082                	ret

000000008000009a <swtch>:
#       Save current registers in from_ctx.
#       Load register values from to_ctx.	

.globl swtch
swtch:
        sd ra, 0(a0)
    8000009a:	00153023          	sd	ra,0(a0) # 1000 <_entry-0x7ffff000>
        sd sp, 8(a0)
    8000009e:	00253423          	sd	sp,8(a0)
        sd s0, 16(a0)
    800000a2:	e900                	sd	s0,16(a0)
        sd s1, 24(a0)
    800000a4:	ed04                	sd	s1,24(a0)
        sd s2, 32(a0)
    800000a6:	03253023          	sd	s2,32(a0)
        sd s3, 40(a0)
    800000aa:	03353423          	sd	s3,40(a0)
        sd s4, 48(a0)
    800000ae:	03453823          	sd	s4,48(a0)
        sd s5, 56(a0)
    800000b2:	03553c23          	sd	s5,56(a0)
        sd s6, 64(a0)
    800000b6:	05653023          	sd	s6,64(a0)
        sd s7, 72(a0)
    800000ba:	05753423          	sd	s7,72(a0)
        sd s8, 80(a0)
    800000be:	05853823          	sd	s8,80(a0)
        sd s9, 88(a0)
    800000c2:	05953c23          	sd	s9,88(a0)
        sd s10, 96(a0)
    800000c6:	07a53023          	sd	s10,96(a0)
        sd s11, 104(a0)
    800000ca:	07b53423          	sd	s11,104(a0)

        ld ra, 0(a1)
    800000ce:	0005b083          	ld	ra,0(a1)
        ld sp, 8(a1)
    800000d2:	0085b103          	ld	sp,8(a1)
        ld s0, 16(a1)
    800000d6:	6980                	ld	s0,16(a1)
        ld s1, 24(a1)
    800000d8:	6d84                	ld	s1,24(a1)
        ld s2, 32(a1)
    800000da:	0205b903          	ld	s2,32(a1)
        ld s3, 40(a1)
    800000de:	0285b983          	ld	s3,40(a1)
        ld s4, 48(a1)
    800000e2:	0305ba03          	ld	s4,48(a1)
        ld s5, 56(a1)
    800000e6:	0385ba83          	ld	s5,56(a1)
        ld s6, 64(a1)
    800000ea:	0405bb03          	ld	s6,64(a1)
        ld s7, 72(a1)
    800000ee:	0485bb83          	ld	s7,72(a1)
        ld s8, 80(a1)
    800000f2:	0505bc03          	ld	s8,80(a1)
        ld s9, 88(a1)
    800000f6:	0585bc83          	ld	s9,88(a1)
        ld s10, 96(a1)
    800000fa:	0605bd03          	ld	s10,96(a1)
        ld s11, 104(a1)
    800000fe:	0685bd83          	ld	s11,104(a1)
        
        ret
    80000102:	8082                	ret

0000000080000104 <_ZN3xv63cpu7currentEv>:
            __asm volatile("csrr %0, %1" : "=r"(value) : "i"(csr_addr));
    80000104:	100027f3          	csrr	a5,sstatus
        XV6_FLAG_ENUM_OPERATORS_CLASS(flag_type);
    80000108:	8b89                	andi	a5,a5,2

    std::array<cpu, NCPU> cpu::m_cpus = {};

    cpu& cpu::current() {
        // The m_cpus array is indexed by riscv::tp, which is guaranteed to be unique for each CPU.
        if (riscv::intr_enabled()) panic("CPU::current::intr_enabled");
    8000010a:	eb89                	bnez	a5,8000011c <_ZN3xv63cpu7currentEv+0x18>
            __asm volatile("mv %0, tp" : "=r"(x));
    8000010c:	8792                	mv	a5,tp
      [[__nodiscard__]]
      _GLIBCXX17_CONSTEXPR reference
      operator[](size_type __n) noexcept
      {
	__glibcxx_requires_subscript(__n);
	return _M_elems[__n];
    8000010e:	079e                	slli	a5,a5,0x7
        return m_cpus[riscv::tp];
    }
    80000110:	00002517          	auipc	a0,0x2
    80000114:	3b050513          	addi	a0,a0,944 # 800024c0 <_ZN3xv63cpu6m_cpusE>
    80000118:	953e                	add	a0,a0,a5
    8000011a:	8082                	ret
    cpu& cpu::current() {
    8000011c:	1141                	addi	sp,sp,-16
    8000011e:	e406                	sd	ra,8(sp)
    80000120:	e022                	sd	s0,0(sp)
    80000122:	0800                	addi	s0,sp,16
        if (riscv::intr_enabled()) panic("CPU::current::intr_enabled");
    80000124:	00002517          	auipc	a0,0x2
    80000128:	eec50513          	addi	a0,a0,-276 # 80002010 <__init_array_end+0x8>
    8000012c:	230000ef          	jal	8000035c <_ZN3xv65panicEPKc>

0000000080000130 <_ZN3xv63cpu13push_intr_offEv>:

    void cpu::push_intr_off() {
    80000130:	1141                	addi	sp,sp,-16
    80000132:	e422                	sd	s0,8(sp)
    80000134:	0800                	addi	s0,sp,16
            __asm volatile("csrrc %0, %1, %2" : "=r"(old_value) : "i"(csr_addr), "rK"(~static_cast<uint64_t>(mask)) : "memory");
    80000136:	100176f3          	csrrci	a3,sstatus,2
            __asm volatile("mv %0, tp" : "=r"(x));
    8000013a:	8792                	mv	a5,tp
        // exchange_and returns the old sstatus, so we learn the previous SIE state atomically.

        const bool was_on = riscv::sstatus.exchange_and(~riscv::sstatus::sie) & riscv::sstatus::sie;

        cpu& c = m_cpus[riscv::tp];
        if (c.m_noff == 0) c.m_intena = was_on;
    8000013c:	00779613          	slli	a2,a5,0x7
    80000140:	00002717          	auipc	a4,0x2
    80000144:	38070713          	addi	a4,a4,896 # 800024c0 <_ZN3xv63cpu6m_cpusE>
    80000148:	9732                	add	a4,a4,a2
    8000014a:	5f38                	lw	a4,120(a4)
    8000014c:	eb19                	bnez	a4,80000162 <_ZN3xv63cpu13push_intr_offEv+0x32>
    8000014e:	85b2                	mv	a1,a2
    80000150:	00002617          	auipc	a2,0x2
    80000154:	37060613          	addi	a2,a2,880 # 800024c0 <_ZN3xv63cpu6m_cpusE>
    80000158:	962e                	add	a2,a2,a1
        const bool was_on = riscv::sstatus.exchange_and(~riscv::sstatus::sie) & riscv::sstatus::sie;
    8000015a:	8285                	srli	a3,a3,0x1
    8000015c:	8a85                	andi	a3,a3,1
        if (c.m_noff == 0) c.m_intena = was_on;
    8000015e:	06d60e23          	sb	a3,124(a2)
        c.m_noff++;
    80000162:	079e                	slli	a5,a5,0x7
    80000164:	00002697          	auipc	a3,0x2
    80000168:	35c68693          	addi	a3,a3,860 # 800024c0 <_ZN3xv63cpu6m_cpusE>
    8000016c:	97b6                	add	a5,a5,a3
    8000016e:	2705                	addiw	a4,a4,1
    80000170:	dfb8                	sw	a4,120(a5)
    }
    80000172:	6422                	ld	s0,8(sp)
    80000174:	0141                	addi	sp,sp,16
    80000176:	8082                	ret

0000000080000178 <_ZN3xv63cpu12pop_intr_offEv>:

    void cpu::pop_intr_off() {
    80000178:	1141                	addi	sp,sp,-16
    8000017a:	e406                	sd	ra,8(sp)
    8000017c:	e022                	sd	s0,0(sp)
    8000017e:	0800                	addi	s0,sp,16
            __asm volatile("csrr %0, %1" : "=r"(value) : "i"(csr_addr));
    80000180:	100027f3          	csrr	a5,sstatus
        XV6_FLAG_ENUM_OPERATORS_CLASS(flag_type);
    80000184:	8b89                	andi	a5,a5,2
        if (riscv::intr_enabled()) panic("cpu::pop_intr_off::interruptible");
    80000186:	e3a1                	bnez	a5,800001c6 <_ZN3xv63cpu12pop_intr_offEv+0x4e>
            __asm volatile("mv %0, tp" : "=r"(x));
    80000188:	8712                	mv	a4,tp
        cpu& c = m_cpus[riscv::tp];
        if (c.m_noff < 1) panic("cpu::pop_intr_off::unbalanced");
    8000018a:	00771693          	slli	a3,a4,0x7
    8000018e:	00002797          	auipc	a5,0x2
    80000192:	33278793          	addi	a5,a5,818 # 800024c0 <_ZN3xv63cpu6m_cpusE>
    80000196:	97b6                	add	a5,a5,a3
    80000198:	5fbc                	lw	a5,120(a5)
    8000019a:	cf85                	beqz	a5,800001d2 <_ZN3xv63cpu12pop_intr_offEv+0x5a>
        c.m_noff--;
    8000019c:	37fd                	addiw	a5,a5,-1
    8000019e:	0007861b          	sext.w	a2,a5
    800001a2:	00771593          	slli	a1,a4,0x7
    800001a6:	00002697          	auipc	a3,0x2
    800001aa:	31a68693          	addi	a3,a3,794 # 800024c0 <_ZN3xv63cpu6m_cpusE>
    800001ae:	96ae                	add	a3,a3,a1
    800001b0:	debc                	sw	a5,120(a3)
        if (c.m_noff == 0 && c.m_intena) riscv::sstatus |= riscv::sstatus::sie;
    800001b2:	e611                	bnez	a2,800001be <_ZN3xv63cpu12pop_intr_offEv+0x46>
    800001b4:	07c6c783          	lbu	a5,124(a3)
    800001b8:	c399                	beqz	a5,800001be <_ZN3xv63cpu12pop_intr_offEv+0x46>
            __asm volatile("csrs %0, %1" : : "i"(csr_addr), "rK"(static_cast<uint64_t>(mask)) : "memory");
    800001ba:	10016073          	csrsi	sstatus,2
    }
    800001be:	60a2                	ld	ra,8(sp)
    800001c0:	6402                	ld	s0,0(sp)
    800001c2:	0141                	addi	sp,sp,16
    800001c4:	8082                	ret
        if (riscv::intr_enabled()) panic("cpu::pop_intr_off::interruptible");
    800001c6:	00002517          	auipc	a0,0x2
    800001ca:	e6a50513          	addi	a0,a0,-406 # 80002030 <__init_array_end+0x28>
    800001ce:	18e000ef          	jal	8000035c <_ZN3xv65panicEPKc>
        if (c.m_noff < 1) panic("cpu::pop_intr_off::unbalanced");
    800001d2:	00002517          	auipc	a0,0x2
    800001d6:	e8650513          	addi	a0,a0,-378 # 80002058 <__init_array_end+0x50>
    800001da:	182000ef          	jal	8000035c <_ZN3xv65panicEPKc>

00000000800001de <_ZNK3xv68spinlock7holdingEv>:
        // holder before the lock reads as free; on RISC-V it is a `fence rw,w` and a plain store.
        __atomic_store_n(&m_locked, lock_status::unlocked, __ATOMIC_RELEASE);
        cpu::pop_intr_off();
    }

    bool spinlock::holding() const {
    800001de:	1141                	addi	sp,sp,-16
    800001e0:	e422                	sd	s0,8(sp)
    800001e2:	0800                	addi	s0,sp,16
        // If another hart holds the lock, m_holder is that hart's id and never ours.
        return (__atomic_load_n(&m_locked, __ATOMIC_RELAXED) == lock_status::locked) && (__atomic_load_n(&m_holder, __ATOMIC_RELAXED) == riscv::tp);
    800001e4:	411c                	lw	a5,0(a0)
    800001e6:	2781                	sext.w	a5,a5
    800001e8:	4705                	li	a4,1
    800001ea:	00e78663          	beq	a5,a4,800001f6 <_ZNK3xv68spinlock7holdingEv+0x18>
    800001ee:	4501                	li	a0,0
    }
    800001f0:	6422                	ld	s0,8(sp)
    800001f2:	0141                	addi	sp,sp,16
    800001f4:	8082                	ret
        return (__atomic_load_n(&m_locked, __ATOMIC_RELAXED) == lock_status::locked) && (__atomic_load_n(&m_holder, __ATOMIC_RELAXED) == riscv::tp);
    800001f6:	4148                	lw	a0,4(a0)
    800001f8:	2501                	sext.w	a0,a0
            __asm volatile("mv %0, tp" : "=r"(x));
    800001fa:	8792                	mv	a5,tp
    800001fc:	1502                	slli	a0,a0,0x20
    800001fe:	9101                	srli	a0,a0,0x20
    80000200:	8d1d                	sub	a0,a0,a5
    80000202:	00153513          	seqz	a0,a0
    80000206:	b7ed                	j	800001f0 <_ZNK3xv68spinlock7holdingEv+0x12>

0000000080000208 <_ZN3xv68spinlock7acquireEv>:
    void spinlock::acquire() {
    80000208:	1101                	addi	sp,sp,-32
    8000020a:	ec06                	sd	ra,24(sp)
    8000020c:	e822                	sd	s0,16(sp)
    8000020e:	e426                	sd	s1,8(sp)
    80000210:	1000                	addi	s0,sp,32
    80000212:	84aa                	mv	s1,a0
        cpu::push_intr_off(); // disable interrupts to avoid deadlock with an interrupt handler taking this lock
    80000214:	f1dff0ef          	jal	80000130 <_ZN3xv63cpu13push_intr_offEv>
        if (holding()) panic("spinlock::acquire::already_held");
    80000218:	8526                	mv	a0,s1
    8000021a:	fc5ff0ef          	jal	800001de <_ZNK3xv68spinlock7holdingEv>
        while (__atomic_exchange_n(&m_locked, lock_status::locked, __ATOMIC_ACQUIRE) != lock_status::unlocked) {
    8000021e:	4705                	li	a4,1
        if (holding()) panic("spinlock::acquire::already_held");
    80000220:	c911                	beqz	a0,80000234 <_ZN3xv68spinlock7acquireEv+0x2c>
    80000222:	00002517          	auipc	a0,0x2
    80000226:	e5650513          	addi	a0,a0,-426 # 80002078 <__init_array_end+0x70>
    8000022a:	132000ef          	jal	8000035c <_ZN3xv65panicEPKc>
            while (__atomic_load_n(&m_locked, __ATOMIC_RELAXED) != lock_status::unlocked);
    8000022e:	409c                	lw	a5,0(s1)
    80000230:	2781                	sext.w	a5,a5
    80000232:	fff5                	bnez	a5,8000022e <_ZN3xv68spinlock7acquireEv+0x26>
        while (__atomic_exchange_n(&m_locked, lock_status::locked, __ATOMIC_ACQUIRE) != lock_status::unlocked) {
    80000234:	87ba                	mv	a5,a4
    80000236:	0cf4a7af          	amoswap.w.aq	a5,a5,(s1)
    8000023a:	2781                	sext.w	a5,a5
    8000023c:	fbed                	bnez	a5,8000022e <_ZN3xv68spinlock7acquireEv+0x26>
    8000023e:	8792                	mv	a5,tp
        __atomic_store_n(&m_holder, riscv::tp, __ATOMIC_RELAXED);
    80000240:	00448713          	addi	a4,s1,4
    80000244:	08f7202f          	amoswap.w	zero,a5,(a4)
    }
    80000248:	60e2                	ld	ra,24(sp)
    8000024a:	6442                	ld	s0,16(sp)
    8000024c:	64a2                	ld	s1,8(sp)
    8000024e:	6105                	addi	sp,sp,32
    80000250:	8082                	ret

0000000080000252 <_ZN3xv68spinlock7releaseEv>:
    void spinlock::release() {
    80000252:	1101                	addi	sp,sp,-32
    80000254:	ec06                	sd	ra,24(sp)
    80000256:	e822                	sd	s0,16(sp)
    80000258:	e426                	sd	s1,8(sp)
    8000025a:	1000                	addi	s0,sp,32
    8000025c:	84aa                	mv	s1,a0
        if (!holding()) panic("spinlock::release::not_held");
    8000025e:	f81ff0ef          	jal	800001de <_ZNK3xv68spinlock7holdingEv>
    80000262:	c10d                	beqz	a0,80000284 <_ZN3xv68spinlock7releaseEv+0x32>
        __atomic_store_n(&m_holder, -1, __ATOMIC_RELAXED);
    80000264:	57fd                	li	a5,-1
    80000266:	00448713          	addi	a4,s1,4
    8000026a:	08f7202f          	amoswap.w	zero,a5,(a4)
        __atomic_store_n(&m_locked, lock_status::unlocked, __ATOMIC_RELEASE);
    8000026e:	0f50000f          	fence	iorw,ow
    80000272:	0804a02f          	amoswap.w	zero,zero,(s1)
        cpu::pop_intr_off();
    80000276:	f03ff0ef          	jal	80000178 <_ZN3xv63cpu12pop_intr_offEv>
    }
    8000027a:	60e2                	ld	ra,24(sp)
    8000027c:	6442                	ld	s0,16(sp)
    8000027e:	64a2                	ld	s1,8(sp)
    80000280:	6105                	addi	sp,sp,32
    80000282:	8082                	ret
        if (!holding()) panic("spinlock::release::not_held");
    80000284:	00002517          	auipc	a0,0x2
    80000288:	e1450513          	addi	a0,a0,-492 # 80002098 <__init_array_end+0x90>
    8000028c:	0d0000ef          	jal	8000035c <_ZN3xv65panicEPKc>

0000000080000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>:
    volatile unsigned char* const raw_thr = reinterpret_cast<volatile unsigned char*>(UART0_BASE);

    void raw_putc(char c) {
        *raw_thr = static_cast<unsigned char>(c);
    }
    void raw_puts(const char* s) {
    80000290:	1141                	addi	sp,sp,-16
    80000292:	e422                	sd	s0,8(sp)
    80000294:	0800                	addi	s0,sp,16
        while (*s) raw_putc(*s++);
    80000296:	00054783          	lbu	a5,0(a0)
    8000029a:	cb89                	beqz	a5,800002ac <_ZN12_GLOBAL__N_18raw_putsEPKc+0x1c>
        *raw_thr = static_cast<unsigned char>(c);
    8000029c:	10000737          	lui	a4,0x10000
        while (*s) raw_putc(*s++);
    800002a0:	0505                	addi	a0,a0,1
        *raw_thr = static_cast<unsigned char>(c);
    800002a2:	00f70023          	sb	a5,0(a4) # 10000000 <_entry-0x70000000>
        while (*s) raw_putc(*s++);
    800002a6:	00054783          	lbu	a5,0(a0)
    800002aa:	fbfd                	bnez	a5,800002a0 <_ZN12_GLOBAL__N_18raw_putsEPKc+0x10>
    }
    800002ac:	6422                	ld	s0,8(sp)
    800002ae:	0141                	addi	sp,sp,16
    800002b0:	8082                	ret

00000000800002b2 <_ZN12_GLOBAL__N_15checkEPKcb>:
    // makes the compiler unable to turn this into constant initialization.
    volatile uint32_t ctor_probe;
    bool ctor_ran = (ctor_probe = 1, true);

    int failures = 0;
    void check(const char* what, bool ok) {
    800002b2:	1101                	addi	sp,sp,-32
    800002b4:	ec06                	sd	ra,24(sp)
    800002b6:	e822                	sd	s0,16(sp)
    800002b8:	e426                	sd	s1,8(sp)
    800002ba:	1000                	addi	s0,sp,32
    800002bc:	84aa                	mv	s1,a0
        raw_puts(ok ? "[PASS] " : "[FAIL] ");
    800002be:	e99d                	bnez	a1,800002f4 <_ZN12_GLOBAL__N_15checkEPKcb+0x42>
    800002c0:	00002517          	auipc	a0,0x2
    800002c4:	df850513          	addi	a0,a0,-520 # 800020b8 <__init_array_end+0xb0>
    800002c8:	fc9ff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
        raw_puts(what);
    800002cc:	8526                	mv	a0,s1
    800002ce:	fc3ff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
        *raw_thr = static_cast<unsigned char>(c);
    800002d2:	100007b7          	lui	a5,0x10000
    800002d6:	4729                	li	a4,10
    800002d8:	00e78023          	sb	a4,0(a5) # 10000000 <_entry-0x70000000>
        raw_putc('\n');
        if (!ok) failures++;
    800002dc:	00002717          	auipc	a4,0x2
    800002e0:	5e470713          	addi	a4,a4,1508 # 800028c0 <_ZN12_GLOBAL__N_18failuresE>
    800002e4:	431c                	lw	a5,0(a4)
    800002e6:	2785                	addiw	a5,a5,1
    800002e8:	c31c                	sw	a5,0(a4)
    }
    800002ea:	60e2                	ld	ra,24(sp)
    800002ec:	6442                	ld	s0,16(sp)
    800002ee:	64a2                	ld	s1,8(sp)
    800002f0:	6105                	addi	sp,sp,32
    800002f2:	8082                	ret
        raw_puts(ok ? "[PASS] " : "[FAIL] ");
    800002f4:	00002517          	auipc	a0,0x2
    800002f8:	dcc50513          	addi	a0,a0,-564 # 800020c0 <__init_array_end+0xb8>
    800002fc:	f95ff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
        raw_puts(what);
    80000300:	8526                	mv	a0,s1
    80000302:	f8fff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
        *raw_thr = static_cast<unsigned char>(c);
    80000306:	100007b7          	lui	a5,0x10000
    8000030a:	4729                	li	a4,10
    8000030c:	00e78023          	sb	a4,0(a5) # 10000000 <_entry-0x70000000>
        if (!ok) failures++;
    80000310:	bfe9                	j	800002ea <_ZN12_GLOBAL__N_15checkEPKcb+0x38>

0000000080000312 <_ZN12_GLOBAL__N_110raw_puthexEm>:
    void raw_puthex(uint64_t v) {
    80000312:	1101                	addi	sp,sp,-32
    80000314:	ec06                	sd	ra,24(sp)
    80000316:	e822                	sd	s0,16(sp)
    80000318:	e426                	sd	s1,8(sp)
    8000031a:	1000                	addi	s0,sp,32
    8000031c:	84aa                	mv	s1,a0
        raw_puts("0x");
    8000031e:	00002517          	auipc	a0,0x2
    80000322:	daa50513          	addi	a0,a0,-598 # 800020c8 <__init_array_end+0xc0>
    80000326:	f6bff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
        for (int i = 60; i >= 0; i -= 4) raw_putc("0123456789abcdef"[(v >> i) & 0xf]);
    8000032a:	03c00713          	li	a4,60
    8000032e:	00002597          	auipc	a1,0x2
    80000332:	da258593          	addi	a1,a1,-606 # 800020d0 <__init_array_end+0xc8>
        *raw_thr = static_cast<unsigned char>(c);
    80000336:	10000637          	lui	a2,0x10000
        for (int i = 60; i >= 0; i -= 4) raw_putc("0123456789abcdef"[(v >> i) & 0xf]);
    8000033a:	56f1                	li	a3,-4
    8000033c:	00e4d7b3          	srl	a5,s1,a4
    80000340:	8bbd                	andi	a5,a5,15
    80000342:	97ae                	add	a5,a5,a1
    80000344:	0007c783          	lbu	a5,0(a5)
        *raw_thr = static_cast<unsigned char>(c);
    80000348:	00f60023          	sb	a5,0(a2) # 10000000 <_entry-0x70000000>
        for (int i = 60; i >= 0; i -= 4) raw_putc("0123456789abcdef"[(v >> i) & 0xf]);
    8000034c:	3771                	addiw	a4,a4,-4
    8000034e:	fed717e3          	bne	a4,a3,8000033c <_ZN12_GLOBAL__N_110raw_puthexEm+0x2a>
    }
    80000352:	60e2                	ld	ra,24(sp)
    80000354:	6442                	ld	s0,16(sp)
    80000356:	64a2                	ld	s1,8(sp)
    80000358:	6105                	addi	sp,sp,32
    8000035a:	8082                	ret

000000008000035c <_ZN3xv65panicEPKc>:
    }
} // namespace

namespace xv6 {
    // weak, so a real panic() in your tree wins at link time
    [[noreturn]] __attribute__((weak)) void panic(const char* m) {
    8000035c:	1101                	addi	sp,sp,-32
    8000035e:	ec06                	sd	ra,24(sp)
    80000360:	e822                	sd	s0,16(sp)
    80000362:	e426                	sd	s1,8(sp)
    80000364:	1000                	addi	s0,sp,32
    80000366:	84aa                	mv	s1,a0
        raw_puts("PANIC: ");
    80000368:	00002517          	auipc	a0,0x2
    8000036c:	d8050513          	addi	a0,a0,-640 # 800020e8 <__init_array_end+0xe0>
    80000370:	f21ff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
        raw_puts(m);
    80000374:	8526                	mv	a0,s1
    80000376:	f1bff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
        *raw_thr = static_cast<unsigned char>(c);
    8000037a:	100007b7          	lui	a5,0x10000
    8000037e:	4729                	li	a4,10
    80000380:	00e78023          	sb	a4,0(a5) # 10000000 <_entry-0x70000000>
        *reinterpret_cast<volatile uint32_t*>(0x100000UL) = ok ? 0x5555u : ((1u << 16) | 0x3333u);
    80000384:	67cd                	lui	a5,0x13
    80000386:	33378793          	addi	a5,a5,819 # 13333 <_entry-0x7ffecccd>
    8000038a:	00100737          	lui	a4,0x100
    8000038e:	c31c                	sw	a5,0(a4)
        for (;;) __asm volatile("wfi");
    80000390:	10500073          	wfi
    80000394:	bff5                	j	80000390 <_ZN3xv65panicEPKc+0x34>

0000000080000396 <_Z5sleepPvRN3xv68spinlockE>:
// ---- hooks that uart.h expects from the rest of the kernel -------------------------------------------

// Single-hart stand-in for xv6's sleep(): drop the lock, wait for a wakeup(), re-take the lock.
// The sequence number is sampled while the lock is held (interrupts off), so a wakeup that arrives
// right after the release cannot be lost. Times out instead of hanging if the interrupt never comes.
void sleep(void*, xv6::spinlock& lk) {
    80000396:	1101                	addi	sp,sp,-32
    80000398:	ec06                	sd	ra,24(sp)
    8000039a:	e822                	sd	s0,16(sp)
    8000039c:	e426                	sd	s1,8(sp)
    8000039e:	e04a                	sd	s2,0(sp)
    800003a0:	1000                	addi	s0,sp,32
    800003a2:	892e                	mv	s2,a1
    if (!lk.holding()) xv6::panic("sleep() called without holding the lock");
    800003a4:	852e                	mv	a0,a1
    800003a6:	e39ff0ef          	jal	800001de <_ZNK3xv68spinlock7holdingEv>
    800003aa:	c939                	beqz	a0,80000400 <_Z5sleepPvRN3xv68spinlockE+0x6a>
    sleep_calls        = sleep_calls + 1;
    800003ac:	00002717          	auipc	a4,0x2
    800003b0:	54470713          	addi	a4,a4,1348 # 800028f0 <_ZN12_GLOBAL__N_111sleep_callsE>
    800003b4:	631c                	ld	a5,0(a4)
    800003b6:	0785                	addi	a5,a5,1
    800003b8:	e31c                	sd	a5,0(a4)
    const uint64_t seq = wake_seq;
    800003ba:	00002497          	auipc	s1,0x2
    800003be:	53e4b483          	ld	s1,1342(s1) # 800028f8 <_ZN12_GLOBAL__N_18wake_seqE>
    lk.release();
    800003c2:	854a                	mv	a0,s2
    800003c4:	e8fff0ef          	jal	80000252 <_ZN3xv68spinlock7releaseEv>
            __asm volatile("csrr %0, %1" : "=r"(value) : "i"(csr_addr));
    800003c8:	100027f3          	csrr	a5,sstatus
        XV6_FLAG_ENUM_OPERATORS_CLASS(flag_type);
    800003cc:	8b89                	andi	a5,a5,2
    if (!riscv::intr_enabled()) xv6::panic("sleep() with interrupts off: the wakeup could never arrive");
    800003ce:	cf9d                	beqz	a5,8000040c <_Z5sleepPvRN3xv68spinlockE+0x76>
            __asm volatile("csrr %0, %1" : "=r"(value) : "i"(csr_addr));
    800003d0:	c0102773          	rdtime	a4
    const uint64_t deadline = now() + 20'000'000; // 2 s
    800003d4:	013137b7          	lui	a5,0x1313
    800003d8:	d0078793          	addi	a5,a5,-768 # 1312d00 <_entry-0x7eced300>
    800003dc:	973e                	add	a4,a4,a5
    while (wake_seq == seq) {
    800003de:	00002697          	auipc	a3,0x2
    800003e2:	51a68693          	addi	a3,a3,1306 # 800028f8 <_ZN12_GLOBAL__N_18wake_seqE>
    800003e6:	629c                	ld	a5,0(a3)
    800003e8:	02979863          	bne	a5,s1,80000418 <_Z5sleepPvRN3xv68spinlockE+0x82>
    800003ec:	c01027f3          	rdtime	a5
        if (now() > deadline) xv6::panic("sleep() timed out: no wakeup, the UART TX interrupt never arrived");
    800003f0:	fef77be3          	bgeu	a4,a5,800003e6 <_Z5sleepPvRN3xv68spinlockE+0x50>
    800003f4:	00002517          	auipc	a0,0x2
    800003f8:	d6450513          	addi	a0,a0,-668 # 80002158 <__init_array_end+0x150>
    800003fc:	f61ff0ef          	jal	8000035c <_ZN3xv65panicEPKc>
    if (!lk.holding()) xv6::panic("sleep() called without holding the lock");
    80000400:	00002517          	auipc	a0,0x2
    80000404:	cf050513          	addi	a0,a0,-784 # 800020f0 <__init_array_end+0xe8>
    80000408:	f55ff0ef          	jal	8000035c <_ZN3xv65panicEPKc>
    if (!riscv::intr_enabled()) xv6::panic("sleep() with interrupts off: the wakeup could never arrive");
    8000040c:	00002517          	auipc	a0,0x2
    80000410:	d0c50513          	addi	a0,a0,-756 # 80002118 <__init_array_end+0x110>
    80000414:	f49ff0ef          	jal	8000035c <_ZN3xv65panicEPKc>
    }
    lk.acquire();
    80000418:	854a                	mv	a0,s2
    8000041a:	defff0ef          	jal	80000208 <_ZN3xv68spinlock7acquireEv>
}
    8000041e:	60e2                	ld	ra,24(sp)
    80000420:	6442                	ld	s0,16(sp)
    80000422:	64a2                	ld	s1,8(sp)
    80000424:	6902                	ld	s2,0(sp)
    80000426:	6105                	addi	sp,sp,32
    80000428:	8082                	ret

000000008000042a <_Z6wakeupPv>:

void wakeup(void*) {
    8000042a:	1141                	addi	sp,sp,-16
    8000042c:	e422                	sd	s0,8(sp)
    8000042e:	0800                	addi	s0,sp,16
    wakeup_calls = wakeup_calls + 1;
    80000430:	00002717          	auipc	a4,0x2
    80000434:	4b870713          	addi	a4,a4,1208 # 800028e8 <_ZN12_GLOBAL__N_112wakeup_callsE>
    80000438:	631c                	ld	a5,0(a4)
    8000043a:	0785                	addi	a5,a5,1
    8000043c:	e31c                	sd	a5,0(a4)
    wake_seq     = wake_seq + 1;
    8000043e:	00002717          	auipc	a4,0x2
    80000442:	4ba70713          	addi	a4,a4,1210 # 800028f8 <_ZN12_GLOBAL__N_18wake_seqE>
    80000446:	631c                	ld	a5,0(a4)
    80000448:	0785                	addi	a5,a5,1
    8000044a:	e31c                	sd	a5,0(a4)
}
    8000044c:	6422                	ld	s0,8(sp)
    8000044e:	0141                	addi	sp,sp,16
    80000450:	8082                	ret

0000000080000452 <_Z11consoleintri>:

void consoleintr(int c) {
    80000452:	1141                	addi	sp,sp,-16
    80000454:	e422                	sd	s0,8(sp)
    80000456:	0800                	addi	s0,sp,16
    if (rx_count < sizeof(rx_buf)) rx_buf[rx_count] = static_cast<char>(c);
    80000458:	00002717          	auipc	a4,0x2
    8000045c:	47873703          	ld	a4,1144(a4) # 800028d0 <_ZN12_GLOBAL__N_18rx_countE>
    80000460:	03f00793          	li	a5,63
    80000464:	00e7ed63          	bltu	a5,a4,8000047e <_Z11consoleintri+0x2c>
    80000468:	00002717          	auipc	a4,0x2
    8000046c:	46873703          	ld	a4,1128(a4) # 800028d0 <_ZN12_GLOBAL__N_18rx_countE>
    80000470:	0000a797          	auipc	a5,0xa
    80000474:	49078793          	addi	a5,a5,1168 # 8000a900 <_ZN12_GLOBAL__N_16rx_bufE>
    80000478:	97ba                	add	a5,a5,a4
    8000047a:	00a78023          	sb	a0,0(a5)
    rx_count = rx_count + 1;
    8000047e:	00002717          	auipc	a4,0x2
    80000482:	45270713          	addi	a4,a4,1106 # 800028d0 <_ZN12_GLOBAL__N_18rx_countE>
    80000486:	631c                	ld	a5,0(a4)
    80000488:	0785                	addi	a5,a5,1
    8000048a:	e31c                	sd	a5,0(a4)
}
    8000048c:	6422                	ld	s0,8(sp)
    8000048e:	0141                	addi	sp,sp,16
    80000490:	8082                	ret

0000000080000492 <kerneltrap>:

extern "C" void kernelvec(); 

// ---- S-mode trap handler: external interrupts go to the UART, everything else is a test failure ------
extern "C" void kerneltrap() {
    80000492:	7179                	addi	sp,sp,-48
    80000494:	f406                	sd	ra,40(sp)
    80000496:	f022                	sd	s0,32(sp)
    80000498:	ec26                	sd	s1,24(sp)
    8000049a:	1800                	addi	s0,sp,48
    8000049c:	142024f3          	csrr	s1,scause
    const uint64_t cause    = riscv::scause;
    const bool is_interrupt = (cause >> 63) != 0;
    const uint64_t code     = cause & 0xff;

    if (is_interrupt && code == 9) {              // supervisor external interrupt
    800004a0:	0c04de63          	bgez	s1,8000057c <kerneltrap+0xea>
    800004a4:	0ff4f793          	zext.b	a5,s1
    800004a8:	4725                	li	a4,9
    800004aa:	00e78d63          	beq	a5,a4,800004c4 <kerneltrap+0x32>
            stray_irqs = stray_irqs + 1;
        }
        if (irq != 0) *plic_reg(0x201004) = irq; // complete
        return;
    }
    if (is_interrupt && code == 5) { // supervisor timer: silence it and carry on
    800004ae:	4715                	li	a4,5
    800004b0:	0ce79663          	bne	a5,a4,8000057c <kerneltrap+0xea>
            __asm volatile("csrw %0, %1" : : "i"(csr_addr), "rK"(static_cast<uint64_t>(value)) : "memory");
    800004b4:	57fd                	li	a5,-1
    800004b6:	14d79073          	csrw	stimecmp,a5
    raw_puthex(riscv::sepc);
    raw_puts(" stval=");
    raw_puthex(riscv::stval);
    raw_putc('\n');
    finish(false);
}
    800004ba:	70a2                	ld	ra,40(sp)
    800004bc:	7402                	ld	s0,32(sp)
    800004be:	64e2                	ld	s1,24(sp)
    800004c0:	6145                	addi	sp,sp,48
    800004c2:	8082                	ret
        const uint32_t irq = *plic_reg(0x201004); // claim
    800004c4:	0c2017b7          	lui	a5,0xc201
    800004c8:	43c4                	lw	s1,4(a5)
    800004ca:	2481                	sext.w	s1,s1
        if (irq == UART_IRQ) {
    800004cc:	47a9                	li	a5,10
    800004ce:	00f48e63          	beq	s1,a5,800004ea <kerneltrap+0x58>
        } else if (irq != 0) {
    800004d2:	d4e5                	beqz	s1,800004ba <kerneltrap+0x28>
            stray_irqs = stray_irqs + 1;
    800004d4:	00002717          	auipc	a4,0x2
    800004d8:	40c70713          	addi	a4,a4,1036 # 800028e0 <_ZN12_GLOBAL__N_110stray_irqsE>
    800004dc:	631c                	ld	a5,0(a4)
    800004de:	0785                	addi	a5,a5,1 # c201001 <_entry-0x73dfefff>
    800004e0:	e31c                	sd	a5,0(a4)
        if (irq != 0) *plic_reg(0x201004) = irq; // complete
    800004e2:	0c2017b7          	lui	a5,0xc201
    800004e6:	c3c4                	sw	s1,4(a5)
    800004e8:	bfc9                	j	800004ba <kerneltrap+0x28>
            uart_irqs = uart_irqs + 1;
    800004ea:	00002717          	auipc	a4,0x2
    800004ee:	3ee70713          	addi	a4,a4,1006 # 800028d8 <_ZN12_GLOBAL__N_19uart_irqsE>
    800004f2:	631c                	ld	a5,0(a4)
    800004f4:	0785                	addi	a5,a5,1 # c201001 <_entry-0x73dfefff>
    800004f6:	e31c                	sd	a5,0(a4)

    template<uintptr_t Addr, uart_access_type Access, detail::uart::valid_fields_type Fields = detail::uart::no_fields>
    struct Reg : Fields, private utils::immovable<Reg<Addr, Access, Fields>> {
        using flag_type = typename Fields::flag_type;

        flag_type read() noexcept requires(Access != uart_access_type::write_only) { return static_cast<flag_type>(*ptr()); }
    800004f8:	100007b7          	lui	a5,0x10000
    800004fc:	0789                	addi	a5,a5,2 # 10000002 <_entry-0x6ffffffe>
    800004fe:	0007c783          	lbu	a5,0(a5)
        const char* m_name; // Name of lock.
    };

    class spinlock_guard : private utils::immovable<spinlock_guard> {
    public:
        explicit spinlock_guard(spinlock& l) : m_lock(l) { m_lock.acquire(); }
    80000502:	0000a517          	auipc	a0,0xa
    80000506:	44650513          	addi	a0,a0,1094 # 8000a948 <_ZN8ns16550a5uart0E+0x8>
    8000050a:	cffff0ef          	jal	80000208 <_ZN3xv68spinlock7acquireEv>
    8000050e:	100007b7          	lui	a5,0x10000
    80000512:	0795                	addi	a5,a5,5 # 10000005 <_entry-0x6ffffffb>
    80000514:	0007c783          	lbu	a5,0(a5)
        }

        void intr() {
            (void)ISR.read();
            with_spinlock(m_tx_lock) {
                if (LSR.read() & LSR::tx_idle) {
    80000518:	0207f793          	andi	a5,a5,32
    8000051c:	eb95                	bnez	a5,80000550 <kerneltrap+0xbe>
        ~spinlock_guard() { m_lock.release(); }
    8000051e:	0000a517          	auipc	a0,0xa
    80000522:	42a50513          	addi	a0,a0,1066 # 8000a948 <_ZN8ns16550a5uart0E+0x8>
    80000526:	d2dff0ef          	jal	80000252 <_ZN3xv68spinlock7releaseEv>
        flag_type read() noexcept requires(Access != uart_access_type::write_only) { return static_cast<flag_type>(*ptr()); }
    8000052a:	100007b7          	lui	a5,0x10000
    8000052e:	0795                	addi	a5,a5,5 # 10000005 <_entry-0x6ffffffb>
    80000530:	0007c783          	lbu	a5,0(a5)
            if (LSR.read() & LSR::rx_ready) return RHR.read();
    80000534:	8b85                	andi	a5,a5,1
    80000536:	d7d5                	beqz	a5,800004e2 <kerneltrap+0x50>
    80000538:	e84a                	sd	s2,16(sp)
    8000053a:	e44e                	sd	s3,8(sp)
        flag_type read() noexcept requires(Access != uart_access_type::write_only) { return static_cast<flag_type>(*ptr()); }
    8000053c:	100007b7          	lui	a5,0x10000
    80000540:	0007c503          	lbu	a0,0(a5) # 10000000 <_entry-0x70000000>
    80000544:	10000937          	lui	s2,0x10000
    80000548:	0915                	addi	s2,s2,5 # 10000005 <_entry-0x6ffffffb>
    8000054a:	100009b7          	lui	s3,0x10000
    8000054e:	a831                	j	8000056a <kerneltrap+0xd8>
                    m_tx_busy = false;
    80000550:	0000a797          	auipc	a5,0xa
    80000554:	3e078823          	sb	zero,1008(a5) # 8000a940 <_ZN8ns16550a5uart0E>
                    wakeup(&m_tx_chan);
    80000558:	0000a517          	auipc	a0,0xa
    8000055c:	3e950513          	addi	a0,a0,1001 # 8000a941 <_ZN8ns16550a5uart0E+0x1>
    80000560:	ecbff0ef          	jal	8000042a <_Z6wakeupPv>
    80000564:	bf6d                	j	8000051e <kerneltrap+0x8c>
        flag_type read() noexcept requires(Access != uart_access_type::write_only) { return static_cast<flag_type>(*ptr()); }
    80000566:	0009c503          	lbu	a0,0(s3) # 10000000 <_entry-0x70000000>
                }
            }

            for (int c = getc(); c != -1; c = getc()) consoleintr(c);
    8000056a:	ee9ff0ef          	jal	80000452 <_Z11consoleintri>
        flag_type read() noexcept requires(Access != uart_access_type::write_only) { return static_cast<flag_type>(*ptr()); }
    8000056e:	00094783          	lbu	a5,0(s2)
            if (LSR.read() & LSR::rx_ready) return RHR.read();
    80000572:	8b85                	andi	a5,a5,1
    80000574:	fbed                	bnez	a5,80000566 <kerneltrap+0xd4>
    80000576:	6942                	ld	s2,16(sp)
    80000578:	69a2                	ld	s3,8(sp)
    8000057a:	b7a5                	j	800004e2 <kerneltrap+0x50>
    raw_puts("TRAP scause=");
    8000057c:	00002517          	auipc	a0,0x2
    80000580:	c2450513          	addi	a0,a0,-988 # 800021a0 <__init_array_end+0x198>
    80000584:	d0dff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
    raw_puthex(cause);
    80000588:	8526                	mv	a0,s1
    8000058a:	d89ff0ef          	jal	80000312 <_ZN12_GLOBAL__N_110raw_puthexEm>
    raw_puts(" sepc=");
    8000058e:	00002517          	auipc	a0,0x2
    80000592:	c2250513          	addi	a0,a0,-990 # 800021b0 <__init_array_end+0x1a8>
    80000596:	cfbff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
            __asm volatile("csrr %0, %1" : "=r"(value) : "i"(csr_addr));
    8000059a:	14102573          	csrr	a0,sepc
    raw_puthex(riscv::sepc);
    8000059e:	d75ff0ef          	jal	80000312 <_ZN12_GLOBAL__N_110raw_puthexEm>
    raw_puts(" stval=");
    800005a2:	00002517          	auipc	a0,0x2
    800005a6:	c1650513          	addi	a0,a0,-1002 # 800021b8 <__init_array_end+0x1b0>
    800005aa:	ce7ff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
    800005ae:	14302573          	csrr	a0,stval
    raw_puthex(riscv::stval);
    800005b2:	d61ff0ef          	jal	80000312 <_ZN12_GLOBAL__N_110raw_puthexEm>
        *raw_thr = static_cast<unsigned char>(c);
    800005b6:	100007b7          	lui	a5,0x10000
    800005ba:	4729                	li	a4,10
    800005bc:	00e78023          	sb	a4,0(a5) # 10000000 <_entry-0x70000000>
        *reinterpret_cast<volatile uint32_t*>(0x100000UL) = ok ? 0x5555u : ((1u << 16) | 0x3333u);
    800005c0:	67cd                	lui	a5,0x13
    800005c2:	33378793          	addi	a5,a5,819 # 13333 <_entry-0x7ffecccd>
    800005c6:	00100737          	lui	a4,0x100
    800005ca:	c31c                	sw	a5,0(a4)
        for (;;) __asm volatile("wfi");
    800005cc:	10500073          	wfi
    800005d0:	bff5                	j	800005cc <kerneltrap+0x13a>

00000000800005d2 <_ZN3xv65kmainEv>:
            __asm volatile("mv %0, tp" : "=r"(x));
    800005d2:	8792                	mv	a5,tp

namespace xv6 {
    void kmain() {
        const uint64_t hart = riscv::tp;
        if (hart != 0) park();
    800005d4:	20079163          	bnez	a5,800007d6 <_ZN3xv65kmainEv+0x204>
    void kmain() {
    800005d8:	7129                	addi	sp,sp,-320
    800005da:	fe06                	sd	ra,312(sp)
    800005dc:	fa22                	sd	s0,304(sp)
    800005de:	0280                	addi	s0,sp,320

        if (!ctor_ran) run_init_array(); // uart0 is a global with a constructor
    800005e0:	00002797          	auipc	a5,0x2
    800005e4:	2e47c783          	lbu	a5,740(a5) # 800028c4 <_ZN12_GLOBAL__N_18ctor_ranE>
    800005e8:	1e078a63          	beqz	a5,800007dc <_ZN3xv65kmainEv+0x20a>
    800005ec:	f626                	sd	s1,296(sp)
    800005ee:	f24a                	sd	s2,288(sp)
    800005f0:	ee4e                	sd	s3,280(sp)
    800005f2:	ea52                	sd	s4,272(sp)
    800005f4:	e656                	sd	s5,264(sp)
            __asm volatile("csrw %0, %1" : : "i"(csr_addr), "rK"(static_cast<uint64_t>(value)) : "memory");
    800005f6:	00000797          	auipc	a5,0x0
    800005fa:	40a78793          	addi	a5,a5,1034 # 80000a00 <kernelvec>
    800005fe:	10579073          	csrw	stvec,a5
    80000602:	57fd                	li	a5,-1
    80000604:	14d79073          	csrw	stimecmp,a5
            __asm volatile("csrs %0, %1" : : "i"(csr_addr), "rK"(static_cast<uint64_t>(mask)) : "memory");
    80000608:	20000793          	li	a5,512
    8000060c:	1047a073          	csrs	sie,a5
        *plic_reg(UART_IRQ * 4) = 1;              // priority > threshold, otherwise it never fires
    80000610:	0c0007b7          	lui	a5,0xc000
    80000614:	4705                	li	a4,1
    80000616:	d798                	sw	a4,40(a5)
        *plic_reg(0x2080)       = 1u << UART_IRQ; // S-mode enable bits, hart 0
    80000618:	0c0027b7          	lui	a5,0xc002
    8000061c:	08078793          	addi	a5,a5,128 # c002080 <_entry-0x73ffdf80>
    80000620:	40000713          	li	a4,1024
    80000624:	c398                	sw	a4,0(a5)
        *plic_reg(0x201000)     = 0;              // S-mode priority threshold, hart 0
    80000626:	0c2017b7          	lui	a5,0xc201
    8000062a:	0007a023          	sw	zero,0(a5) # c201000 <_entry-0x73dff000>
    8000062e:	10016073          	csrsi	sstatus,2
        riscv::stimecmp = ~0ULL; // no timer interrupts during the test
        riscv::sie |= riscv::sie::seie;
        plic_init_hart0();
        riscv::intr_enable();

        raw_puts("== uart test ==\n");
    80000632:	00002517          	auipc	a0,0x2
    80000636:	bbe50513          	addi	a0,a0,-1090 # 800021f0 <__init_array_end+0x1e8>
    8000063a:	c57ff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>

        // T1: polled output
        raw_puts("-- T1 putc_sync, next line must read: putc_sync ok\n");
    8000063e:	00002517          	auipc	a0,0x2
    80000642:	bca50513          	addi	a0,a0,-1078 # 80002208 <__init_array_end+0x200>
    80000646:	c4bff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
        for (const char* s = "putc_sync ok\n"; *s; ++s) ns16550a::uart0.putc_sync(*s);
    8000064a:	00002617          	auipc	a2,0x2
    8000064e:	b7660613          	addi	a2,a2,-1162 # 800021c0 <__init_array_end+0x1b8>
    80000652:	07000693          	li	a3,112
        flag_type read() noexcept requires(Access != uart_access_type::write_only) { return static_cast<flag_type>(*ptr()); }
    80000656:	10000737          	lui	a4,0x10000
    8000065a:	0715                	addi	a4,a4,5 # 10000005 <_entry-0x6ffffffb>
        void write(flag_type value) noexcept requires(Access != uart_access_type::read_only) { *ptr() = static_cast<uint8_t>(value); }
    8000065c:	100005b7          	lui	a1,0x10000
        flag_type read() noexcept requires(Access != uart_access_type::write_only) { return static_cast<flag_type>(*ptr()); }
    80000660:	00074783          	lbu	a5,0(a4)
            while (!(LSR.read() & LSR::tx_idle));
    80000664:	0207f793          	andi	a5,a5,32
    80000668:	dfe5                	beqz	a5,80000660 <_ZN3xv65kmainEv+0x8e>
        void write(flag_type value) noexcept requires(Access != uart_access_type::read_only) { *ptr() = static_cast<uint8_t>(value); }
    8000066a:	00d58023          	sb	a3,0(a1) # 10000000 <_entry-0x70000000>
    8000066e:	0605                	addi	a2,a2,1
    80000670:	00064683          	lbu	a3,0(a2)
    80000674:	f6f5                	bnez	a3,80000660 <_ZN3xv65kmainEv+0x8e>

        // T2: interrupt-driven transmit through sleep()/wakeup()
        raw_puts("-- T2 write(), next line must read: write ok\n");
    80000676:	00002517          	auipc	a0,0x2
    8000067a:	bca50513          	addi	a0,a0,-1078 # 80002240 <__init_array_end+0x238>
    8000067e:	c13ff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
        {
            static constexpr char msg[]  = "write ok\n";
            constexpr size_t len         = sizeof(msg) - 1;
            const uint64_t sleeps_before = sleep_calls;
    80000682:	00002a97          	auipc	s5,0x2
    80000686:	26ea8a93          	addi	s5,s5,622 # 800028f0 <_ZN12_GLOBAL__N_111sleep_callsE>
    8000068a:	000aba03          	ld	s4,0(s5)
            const uint64_t irqs_before   = uart_irqs;
    8000068e:	00002917          	auipc	s2,0x2
    80000692:	24a90913          	addi	s2,s2,586 # 800028d8 <_ZN12_GLOBAL__N_19uart_irqsE>
    80000696:	00093983          	ld	s3,0(s2)
            __asm volatile("csrr %0, %1" : "=r"(value) : "i"(csr_addr));
    8000069a:	100024f3          	csrr	s1,sstatus
      template<contiguous_iterator _It>
	requires __is_compatible_ref<iter_reference_t<_It>>::value
	constexpr explicit(extent != dynamic_extent)
	span(_It __first, size_type __count)
	noexcept
	: _M_ptr(std::to_address(__first)), _M_extent(__count)
    8000069e:	00002597          	auipc	a1,0x2
    800006a2:	dfa58593          	addi	a1,a1,-518 # 80002498 <_ZZN3xv65kmainEvE3msg>
    800006a6:	f8b43823          	sd	a1,-112(s0)
	: _M_extent_value(__extent)
    800006aa:	47a5                	li	a5,9
    800006ac:	f8f43c23          	sd	a5,-104(s0)
            const bool intr_before       = riscv::intr_enabled();

            ns16550a::uart0.write(std::span<const char>(msg, len));
    800006b0:	4625                	li	a2,9
    800006b2:	0000a517          	auipc	a0,0xa
    800006b6:	28e50513          	addi	a0,a0,654 # 8000a940 <_ZN8ns16550a5uart0E>
    800006ba:	2be000ef          	jal	80000978 <_ZN8ns16550a4UARTILm268435456EE5writeESt4spanIKcLm18446744073709551615EE>
    800006be:	100025f3          	csrr	a1,sstatus
        return static_cast<satp::flag_type>(satp::mode_sv39 | (pagetable_addr >> 12));
    }

    // are device supervisor interrupts enabled?
    inline bool intr_enabled() {
        return sstatus & sstatus::sie;
    800006c2:	8185                	srli	a1,a1,0x1
    800006c4:	0014d793          	srli	a5,s1,0x1

            check("T2 write() leaves the interrupt state as it found it", riscv::intr_enabled() == intr_before);
    800006c8:	8985                	andi	a1,a1,1
    800006ca:	8b85                	andi	a5,a5,1
    800006cc:	8d9d                	sub	a1,a1,a5
    800006ce:	0015b593          	seqz	a1,a1
    800006d2:	00002517          	auipc	a0,0x2
    800006d6:	b9e50513          	addi	a0,a0,-1122 # 80002270 <__init_array_end+0x268>
    800006da:	bd9ff0ef          	jal	800002b2 <_ZN12_GLOBAL__N_15checkEPKcb>
            // every character after the first waits for the TX-idle interrupt
            check("T2 write() slept once per extra character (TX interrupt path used)", sleep_calls - sleeps_before >= len - 1);
    800006de:	000ab583          	ld	a1,0(s5)
    800006e2:	414585b3          	sub	a1,a1,s4
    800006e6:	0085b593          	sltiu	a1,a1,8
    800006ea:	0015c593          	xori	a1,a1,1
    800006ee:	00002517          	auipc	a0,0x2
    800006f2:	bba50513          	addi	a0,a0,-1094 # 800022a8 <__init_array_end+0x2a0>
    800006f6:	bbdff0ef          	jal	800002b2 <_ZN12_GLOBAL__N_15checkEPKcb>
            check("T2 UART interrupts were delivered through the PLIC", uart_irqs > irqs_before);
    800006fa:	00093583          	ld	a1,0(s2)
    800006fe:	00b9b5b3          	sltu	a1,s3,a1
    80000702:	00002517          	auipc	a0,0x2
    80000706:	bee50513          	addi	a0,a0,-1042 # 800022f0 <__init_array_end+0x2e8>
    8000070a:	ba9ff0ef          	jal	800002b2 <_ZN12_GLOBAL__N_15checkEPKcb>
        }

        // T3: more characters than any FIFO holds, nothing may be dropped or reordered
        raw_puts("-- T3 long write, next line must be 200 digits cycling 0-9\n");
    8000070e:	00002517          	auipc	a0,0x2
    80000712:	c1a50513          	addi	a0,a0,-998 # 80002328 <__init_array_end+0x320>
    80000716:	b7bff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
        {
            char big[201];
            for (int i = 0; i < 200; i++) big[i] = char('0' + i % 10);
    8000071a:	ec040693          	addi	a3,s0,-320
    8000071e:	4781                	li	a5,0
    80000720:	45a9                	li	a1,10
    80000722:	0c800613          	li	a2,200
    80000726:	02b7e73b          	remw	a4,a5,a1
    8000072a:	0307071b          	addiw	a4,a4,48
    8000072e:	00e68023          	sb	a4,0(a3)
    80000732:	2785                	addiw	a5,a5,1
    80000734:	0685                	addi	a3,a3,1
    80000736:	fec798e3          	bne	a5,a2,80000726 <_ZN3xv65kmainEv+0x154>
            big[200] = '\n';
    8000073a:	47a9                	li	a5,10
    8000073c:	f8f40423          	sb	a5,-120(s0)
	: _M_ptr(std::to_address(__first)), _M_extent(__count)
    80000740:	ec040593          	addi	a1,s0,-320
    80000744:	fab43023          	sd	a1,-96(s0)
	: _M_extent_value(__extent)
    80000748:	0c900793          	li	a5,201
    8000074c:	faf43423          	sd	a5,-88(s0)
            ns16550a::uart0.write(std::span<const char>(big, sizeof(big)));
    80000750:	0000a497          	auipc	s1,0xa
    80000754:	1f048493          	addi	s1,s1,496 # 8000a940 <_ZN8ns16550a5uart0E>
    80000758:	0c900613          	li	a2,201
    8000075c:	8526                	mv	a0,s1
    8000075e:	21a000ef          	jal	80000978 <_ZN8ns16550a4UARTILm268435456EE5writeESt4spanIKcLm18446744073709551615EE>
            check("T3 long write() returned", true);
    80000762:	4585                	li	a1,1
    80000764:	00002517          	auipc	a0,0x2
    80000768:	c0450513          	addi	a0,a0,-1020 # 80002368 <__init_array_end+0x360>
    8000076c:	b47ff0ef          	jal	800002b2 <_ZN12_GLOBAL__N_15checkEPKcb>
	: _M_ptr(std::to_address(__first)), _M_extent(__count)
    80000770:	00002597          	auipc	a1,0x2
    80000774:	d3858593          	addi	a1,a1,-712 # 800024a8 <_ZZN3xv65kmainEvE3msg_0>
    80000778:	fab43823          	sd	a1,-80(s0)
	: _M_extent_value(__extent)
    8000077c:	47c1                	li	a5,16
    8000077e:	faf43c23          	sd	a5,-72(s0)
        }

        // T4: write() again right after a write(): the busy flag must not be stuck
        {
            static constexpr char msg[] = "second write ok\n";
            ns16550a::uart0.write(std::span<const char>(msg, sizeof(msg) - 1));
    80000782:	4641                	li	a2,16
    80000784:	8526                	mv	a0,s1
    80000786:	1f2000ef          	jal	80000978 <_ZN8ns16550a4UARTILm268435456EE5writeESt4spanIKcLm18446744073709551615EE>
            check("T4 back-to-back write() did not get stuck", true);
    8000078a:	4585                	li	a1,1
    8000078c:	00002517          	auipc	a0,0x2
    80000790:	bfc50513          	addi	a0,a0,-1028 # 80002388 <__init_array_end+0x380>
    80000794:	b1fff0ef          	jal	800002b2 <_ZN12_GLOBAL__N_15checkEPKcb>
            char rb[8];
            check("T5 getc() returns -1 when no input is pending", ns16550a::uart0.getc() == -1);
            check("T5 read() returns 0 when no input is pending", ns16550a::uart0.read(rb) == 0);
        }
#else
        raw_puts("-- T5 rx: type or pipe ");
    80000798:	00002517          	auipc	a0,0x2
    8000079c:	c2050513          	addi	a0,a0,-992 # 800023b8 <__init_array_end+0x3b0>
    800007a0:	af1ff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
        *raw_thr = static_cast<unsigned char>(c);
    800007a4:	100007b7          	lui	a5,0x10000
    800007a8:	03500713          	li	a4,53
    800007ac:	00e78023          	sb	a4,0(a5) # 10000000 <_entry-0x70000000>
        raw_putdec(RX_COUNT);
        raw_puts(" characters into the serial console\n");
    800007b0:	00002517          	auipc	a0,0x2
    800007b4:	c2050513          	addi	a0,a0,-992 # 800023d0 <__init_array_end+0x3c8>
    800007b8:	ad9ff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
            __asm volatile("csrr %0, %1" : "=r"(value) : "i"(csr_addr));
    800007bc:	c0102673          	rdtime	a2
        {
            const uint64_t deadline = now() + 100'000'000; // 10 s
    800007c0:	05f5e7b7          	lui	a5,0x5f5e
    800007c4:	10078793          	addi	a5,a5,256 # 5f5e100 <_entry-0x7a0a1f00>
    800007c8:	963e                	add	a2,a2,a5
            while (rx_count < RX_COUNT && now() < deadline) {
    800007ca:	00002697          	auipc	a3,0x2
    800007ce:	10668693          	addi	a3,a3,262 # 800028d0 <_ZN12_GLOBAL__N_18rx_countE>
    800007d2:	4711                	li	a4,4
    800007d4:	a891                	j	80000828 <_ZN3xv65kmainEv+0x256>
        for (;;) __asm volatile("wfi");
    800007d6:	10500073          	wfi
    800007da:	bff5                	j	800007d6 <_ZN3xv65kmainEv+0x204>
    800007dc:	f626                	sd	s1,296(sp)
    800007de:	f24a                	sd	s2,288(sp)
    800007e0:	ee4e                	sd	s3,280(sp)
    800007e2:	ea52                	sd	s4,272(sp)
    800007e4:	e656                	sd	s5,264(sp)
        for (auto p = &__init_array_start; p < &__init_array_end; ++p) (*p)();
    800007e6:	00002717          	auipc	a4,0x2
    800007ea:	81a70713          	addi	a4,a4,-2022 # 80002000 <__init_array_start>
    800007ee:	00002797          	auipc	a5,0x2
    800007f2:	81a78793          	addi	a5,a5,-2022 # 80002008 <__init_array_end>
    800007f6:	00f77f63          	bgeu	a4,a5,80000814 <_ZN3xv65kmainEv+0x242>
    800007fa:	84ba                	mv	s1,a4
    800007fc:	893e                	mv	s2,a5
    800007fe:	609c                	ld	a5,0(s1)
    80000800:	9782                	jalr	a5
    80000802:	04a1                	addi	s1,s1,8
    80000804:	ff24ede3          	bltu	s1,s2,800007fe <_ZN3xv65kmainEv+0x22c>
        if (!ctor_ran) xv6::panic("global constructors did not run");
    80000808:	00002797          	auipc	a5,0x2
    8000080c:	0bc7c783          	lbu	a5,188(a5) # 800028c4 <_ZN12_GLOBAL__N_18ctor_ranE>
    80000810:	de0793e3          	bnez	a5,800005f6 <_ZN3xv65kmainEv+0x24>
    80000814:	00002517          	auipc	a0,0x2
    80000818:	9bc50513          	addi	a0,a0,-1604 # 800021d0 <__init_array_end+0x1c8>
    8000081c:	b41ff0ef          	jal	8000035c <_ZN3xv65panicEPKc>
    80000820:	c01027f3          	rdtime	a5
            while (rx_count < RX_COUNT && now() < deadline) {
    80000824:	00c7f563          	bgeu	a5,a2,8000082e <_ZN3xv65kmainEv+0x25c>
    80000828:	629c                	ld	a5,0(a3)
    8000082a:	fef77be3          	bgeu	a4,a5,80000820 <_ZN3xv65kmainEv+0x24e>
            }
            check("T5 characters reached consoleintr() through the RX interrupt", rx_count >= RX_COUNT);
    8000082e:	00002497          	auipc	s1,0x2
    80000832:	0a248493          	addi	s1,s1,162 # 800028d0 <_ZN12_GLOBAL__N_18rx_countE>
    80000836:	608c                	ld	a1,0(s1)
    80000838:	0055b593          	sltiu	a1,a1,5
    8000083c:	0015c593          	xori	a1,a1,1
    80000840:	00002517          	auipc	a0,0x2
    80000844:	bb850513          	addi	a0,a0,-1096 # 800023f8 <__init_array_end+0x3f0>
    80000848:	a6bff0ef          	jal	800002b2 <_ZN12_GLOBAL__N_15checkEPKcb>
            raw_puts("   received: ");
    8000084c:	00002517          	auipc	a0,0x2
    80000850:	bec50513          	addi	a0,a0,-1044 # 80002438 <__init_array_end+0x430>
    80000854:	a3dff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
            const size_t n = rx_count < sizeof(rx_buf) ? static_cast<size_t>(rx_count) : sizeof(rx_buf);
    80000858:	6098                	ld	a4,0(s1)
    8000085a:	03f00793          	li	a5,63
    8000085e:	04e7e763          	bltu	a5,a4,800008ac <_ZN3xv65kmainEv+0x2da>
    80000862:	00002617          	auipc	a2,0x2
    80000866:	06e63603          	ld	a2,110(a2) # 800028d0 <_ZN12_GLOBAL__N_18rx_countE>
            for (size_t i = 0; i < n; i++) raw_putc(rx_buf[i] >= 32 && rx_buf[i] < 127 ? rx_buf[i] : '?');
    8000086a:	e239                	bnez	a2,800008b0 <_ZN3xv65kmainEv+0x2de>
        *raw_thr = static_cast<unsigned char>(c);
    8000086c:	100007b7          	lui	a5,0x10000
    80000870:	4729                	li	a4,10
    80000872:	00e78023          	sb	a4,0(a5) # 10000000 <_entry-0x70000000>
            raw_putc('\n');
        }
#endif

        check("no stray PLIC interrupts", stray_irqs == 0);
    80000876:	00002597          	auipc	a1,0x2
    8000087a:	06a5b583          	ld	a1,106(a1) # 800028e0 <_ZN12_GLOBAL__N_110stray_irqsE>
    8000087e:	0015b593          	seqz	a1,a1
    80000882:	00002517          	auipc	a0,0x2
    80000886:	bc650513          	addi	a0,a0,-1082 # 80002448 <__init_array_end+0x440>
    8000088a:	a29ff0ef          	jal	800002b2 <_ZN12_GLOBAL__N_15checkEPKcb>

        raw_puts(failures == 0 ? "ALL PASS\n" : "SOME CHECKS FAILED\n");
    8000088e:	00002797          	auipc	a5,0x2
    80000892:	0327a783          	lw	a5,50(a5) # 800028c0 <_ZN12_GLOBAL__N_18failuresE>
    80000896:	cba1                	beqz	a5,800008e6 <_ZN3xv65kmainEv+0x314>
    80000898:	00002517          	auipc	a0,0x2
    8000089c:	be050513          	addi	a0,a0,-1056 # 80002478 <__init_array_end+0x470>
    800008a0:	9f1ff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
        *reinterpret_cast<volatile uint32_t*>(0x100000UL) = ok ? 0x5555u : ((1u << 16) | 0x3333u);
    800008a4:	67cd                	lui	a5,0x13
    800008a6:	33378793          	addi	a5,a5,819 # 13333 <_entry-0x7ffecccd>
    800008aa:	a0b9                	j	800008f8 <_ZN3xv65kmainEv+0x326>
            const size_t n = rx_count < sizeof(rx_buf) ? static_cast<size_t>(rx_count) : sizeof(rx_buf);
    800008ac:	04000613          	li	a2,64
    800008b0:	0000a797          	auipc	a5,0xa
    800008b4:	05078793          	addi	a5,a5,80 # 8000a900 <_ZN12_GLOBAL__N_16rx_bufE>
    800008b8:	963e                	add	a2,a2,a5
            for (size_t i = 0; i < n; i++) raw_putc(rx_buf[i] >= 32 && rx_buf[i] < 127 ? rx_buf[i] : '?');
    800008ba:	05e00513          	li	a0,94
    800008be:	03f00813          	li	a6,63
        *raw_thr = static_cast<unsigned char>(c);
    800008c2:	100005b7          	lui	a1,0x10000
    800008c6:	a031                	j	800008d2 <_ZN3xv65kmainEv+0x300>
    800008c8:	00d58023          	sb	a3,0(a1) # 10000000 <_entry-0x70000000>
            for (size_t i = 0; i < n; i++) raw_putc(rx_buf[i] >= 32 && rx_buf[i] < 127 ? rx_buf[i] : '?');
    800008cc:	0785                	addi	a5,a5,1
    800008ce:	f8f60fe3          	beq	a2,a5,8000086c <_ZN3xv65kmainEv+0x29a>
    800008d2:	0007c683          	lbu	a3,0(a5)
    800008d6:	fe06871b          	addiw	a4,a3,-32
    800008da:	0ff77713          	zext.b	a4,a4
    800008de:	fee575e3          	bgeu	a0,a4,800008c8 <_ZN3xv65kmainEv+0x2f6>
    800008e2:	86c2                	mv	a3,a6
    800008e4:	b7d5                	j	800008c8 <_ZN3xv65kmainEv+0x2f6>
        raw_puts(failures == 0 ? "ALL PASS\n" : "SOME CHECKS FAILED\n");
    800008e6:	00002517          	auipc	a0,0x2
    800008ea:	b8250513          	addi	a0,a0,-1150 # 80002468 <__init_array_end+0x460>
    800008ee:	9a3ff0ef          	jal	80000290 <_ZN12_GLOBAL__N_18raw_putsEPKc>
        *reinterpret_cast<volatile uint32_t*>(0x100000UL) = ok ? 0x5555u : ((1u << 16) | 0x3333u);
    800008f2:	6795                	lui	a5,0x5
    800008f4:	55578793          	addi	a5,a5,1365 # 5555 <_entry-0x7fffaaab>
    800008f8:	00100737          	lui	a4,0x100
    800008fc:	c31c                	sw	a5,0(a4)
        for (;;) __asm volatile("wfi");
    800008fe:	10500073          	wfi
    80000902:	bff5                	j	800008fe <_ZN3xv65kmainEv+0x32c>

0000000080000904 <_GLOBAL__sub_I__ZN8ns16550a5uart0E>:
        finish(failures == 0);
    }
} // namespace xv6
    80000904:	1141                	addi	sp,sp,-16
    80000906:	e422                	sd	s0,8(sp)
    80000908:	0800                	addi	s0,sp,16
    bool ctor_ran = (ctor_probe = 1, true);
    8000090a:	4785                	li	a5,1
    8000090c:	00002717          	auipc	a4,0x2
    80000910:	faf72e23          	sw	a5,-68(a4) # 800028c8 <_ZN12_GLOBAL__N_110ctor_probeE>
    80000914:	00002717          	auipc	a4,0x2
    80000918:	faf70823          	sb	a5,-80(a4) # 800028c4 <_ZN12_GLOBAL__N_18ctor_ranE>
        UART(const char* uartname) : m_tx_lock(uartname) {
    8000091c:	0000a797          	auipc	a5,0xa
    80000920:	fe478793          	addi	a5,a5,-28 # 8000a900 <_ZN12_GLOBAL__N_16rx_bufE>
    80000924:	04078023          	sb	zero,64(a5)
    80000928:	040780a3          	sb	zero,65(a5)
        explicit spinlock(const char* name) : m_name(name) {}
    8000092c:	0407a423          	sw	zero,72(a5)
    80000930:	577d                	li	a4,-1
    80000932:	c7f8                	sw	a4,76(a5)
    80000934:	00002717          	auipc	a4,0x2
    80000938:	b5c70713          	addi	a4,a4,-1188 # 80002490 <__init_array_end+0x488>
    8000093c:	ebb8                	sd	a4,80(a5)
        void write(flag_type value) noexcept requires(Access != uart_access_type::read_only) { *ptr() = static_cast<uint8_t>(value); }
    8000093e:	100007b7          	lui	a5,0x10000
    80000942:	000780a3          	sb	zero,1(a5) # 10000001 <_entry-0x6fffffff>
    80000946:	10000737          	lui	a4,0x10000
    8000094a:	f8000693          	li	a3,-128
    8000094e:	00d701a3          	sb	a3,3(a4) # 10000003 <_entry-0x6ffffffd>
    80000952:	468d                	li	a3,3
    80000954:	10000637          	lui	a2,0x10000
    80000958:	00d60023          	sb	a3,0(a2) # 10000000 <_entry-0x70000000>
    8000095c:	000780a3          	sb	zero,1(a5)
    80000960:	00d701a3          	sb	a3,3(a4)
    80000964:	10000737          	lui	a4,0x10000
    80000968:	461d                	li	a2,7
    8000096a:	00c70123          	sb	a2,2(a4) # 10000002 <_entry-0x6ffffffe>
    8000096e:	00d780a3          	sb	a3,1(a5)
} // namespace xv6
    80000972:	6422                	ld	s0,8(sp)
    80000974:	0141                	addi	sp,sp,16
    80000976:	8082                	ret

0000000080000978 <_ZN8ns16550a4UARTILm268435456EE5writeESt4spanIKcLm18446744073709551615EE>:
        void write(std::span<const char> buf) {
    80000978:	711d                	addi	sp,sp,-96
    8000097a:	ec86                	sd	ra,88(sp)
    8000097c:	e8a2                	sd	s0,80(sp)
    8000097e:	e4a6                	sd	s1,72(sp)
    80000980:	e0ca                	sd	s2,64(sp)
    80000982:	f852                	sd	s4,48(sp)
    80000984:	f05a                	sd	s6,32(sp)
    80000986:	1080                	addi	s0,sp,96
    80000988:	84aa                	mv	s1,a0
    8000098a:	fab43023          	sd	a1,-96(s0)
    8000098e:	fac43423          	sd	a2,-88(s0)
    80000992:	8a2e                	mv	s4,a1
            with_spinlock(m_tx_lock) {
    80000994:	00850913          	addi	s2,a0,8
        explicit spinlock_guard(spinlock& l) : m_lock(l) { m_lock.acquire(); }
    80000998:	854a                	mv	a0,s2
    8000099a:	86fff0ef          	jal	80000208 <_ZN3xv68spinlock7acquireEv>
      { return iterator(this->_M_ptr); }

      [[nodiscard]]
      constexpr iterator
      end() const noexcept
      { return iterator(this->_M_ptr + this->size()); }
    8000099e:	fa843b03          	ld	s6,-88(s0)
    800009a2:	9b52                	add	s6,s6,s4
                for (char c : buf) {
    800009a4:	054b0263          	beq	s6,s4,800009e8 <_ZN8ns16550a4UARTILm268435456EE5writeESt4spanIKcLm18446744073709551615EE+0x70>
    800009a8:	fc4e                	sd	s3,56(sp)
    800009aa:	f456                	sd	s5,40(sp)
    800009ac:	ec5e                	sd	s7,24(sp)
    800009ae:	e862                	sd	s8,16(sp)
                        sleep(&m_tx_chan, m_tx_lock);
    800009b0:	00148993          	addi	s3,s1,1
        void write(flag_type value) noexcept requires(Access != uart_access_type::read_only) { *ptr() = static_cast<uint8_t>(value); }
    800009b4:	10000c37          	lui	s8,0x10000
                    m_tx_busy = true;
    800009b8:	4b85                	li	s7,1
                for (char c : buf) {
    800009ba:	000a4a83          	lbu	s5,0(s4)
                    while (m_tx_busy) {
    800009be:	0004c783          	lbu	a5,0(s1)
    800009c2:	cb81                	beqz	a5,800009d2 <_ZN8ns16550a4UARTILm268435456EE5writeESt4spanIKcLm18446744073709551615EE+0x5a>
                        sleep(&m_tx_chan, m_tx_lock);
    800009c4:	85ca                	mv	a1,s2
    800009c6:	854e                	mv	a0,s3
    800009c8:	9cfff0ef          	jal	80000396 <_Z5sleepPvRN3xv68spinlockE>
                    while (m_tx_busy) {
    800009cc:	0004c783          	lbu	a5,0(s1)
    800009d0:	fbf5                	bnez	a5,800009c4 <_ZN8ns16550a4UARTILm268435456EE5writeESt4spanIKcLm18446744073709551615EE+0x4c>
        void write(flag_type value) noexcept requires(Access != uart_access_type::read_only) { *ptr() = static_cast<uint8_t>(value); }
    800009d2:	015c0023          	sb	s5,0(s8) # 10000000 <_entry-0x70000000>
                    m_tx_busy = true;
    800009d6:	01748023          	sb	s7,0(s1)
                for (char c : buf) {
    800009da:	0a05                	addi	s4,s4,1
    800009dc:	fd4b1fe3          	bne	s6,s4,800009ba <_ZN8ns16550a4UARTILm268435456EE5writeESt4spanIKcLm18446744073709551615EE+0x42>
    800009e0:	79e2                	ld	s3,56(sp)
    800009e2:	7aa2                	ld	s5,40(sp)
    800009e4:	6be2                	ld	s7,24(sp)
    800009e6:	6c42                	ld	s8,16(sp)
        ~spinlock_guard() { m_lock.release(); }
    800009e8:	854a                	mv	a0,s2
    800009ea:	869ff0ef          	jal	80000252 <_ZN3xv68spinlock7releaseEv>
        }
    800009ee:	60e6                	ld	ra,88(sp)
    800009f0:	6446                	ld	s0,80(sp)
    800009f2:	64a6                	ld	s1,72(sp)
    800009f4:	6906                	ld	s2,64(sp)
    800009f6:	7a42                	ld	s4,48(sp)
    800009f8:	7b02                	ld	s6,32(sp)
    800009fa:	6125                	addi	sp,sp,96
    800009fc:	8082                	ret
	...

0000000080000a00 <kernelvec>:
.globl kerneltrap
.globl kernelvec
.align 4
kernelvec:
        # make room to save registers.
        addi sp, sp, -256
    80000a00:	7111                	addi	sp,sp,-256

        # save caller-saved registers.
        sd ra, 0(sp)
    80000a02:	e006                	sd	ra,0(sp)
        # sd sp, 8(sp)
        sd gp, 16(sp)
    80000a04:	e80e                	sd	gp,16(sp)
        # sd tp, 24(sp)
        sd t0, 32(sp)
    80000a06:	f016                	sd	t0,32(sp)
        sd t1, 40(sp)
    80000a08:	f41a                	sd	t1,40(sp)
        sd t2, 48(sp)
    80000a0a:	f81e                	sd	t2,48(sp)
        sd a0, 72(sp)
    80000a0c:	e4aa                	sd	a0,72(sp)
        sd a1, 80(sp)
    80000a0e:	e8ae                	sd	a1,80(sp)
        sd a2, 88(sp)
    80000a10:	ecb2                	sd	a2,88(sp)
        sd a3, 96(sp)
    80000a12:	f0b6                	sd	a3,96(sp)
        sd a4, 104(sp)
    80000a14:	f4ba                	sd	a4,104(sp)
        sd a5, 112(sp)
    80000a16:	f8be                	sd	a5,112(sp)
        sd a6, 120(sp)
    80000a18:	fcc2                	sd	a6,120(sp)
        sd a7, 128(sp)
    80000a1a:	e146                	sd	a7,128(sp)
        sd t3, 216(sp)
    80000a1c:	edf2                	sd	t3,216(sp)
        sd t4, 224(sp)
    80000a1e:	f1f6                	sd	t4,224(sp)
        sd t5, 232(sp)
    80000a20:	f5fa                	sd	t5,232(sp)
        sd t6, 240(sp)
    80000a22:	f9fe                	sd	t6,240(sp)

        # call the C trap handler in trap.c
        call kerneltrap
    80000a24:	a6fff0ef          	jal	80000492 <kerneltrap>

        # restore registers.
        ld ra, 0(sp)
    80000a28:	6082                	ld	ra,0(sp)
        # ld sp, 8(sp)
        ld gp, 16(sp)
    80000a2a:	61c2                	ld	gp,16(sp)
        # not tp (contains hartid), in case we moved CPUs
        ld t0, 32(sp)
    80000a2c:	7282                	ld	t0,32(sp)
        ld t1, 40(sp)
    80000a2e:	7322                	ld	t1,40(sp)
        ld t2, 48(sp)
    80000a30:	73c2                	ld	t2,48(sp)
        ld a0, 72(sp)
    80000a32:	6526                	ld	a0,72(sp)
        ld a1, 80(sp)
    80000a34:	65c6                	ld	a1,80(sp)
        ld a2, 88(sp)
    80000a36:	6666                	ld	a2,88(sp)
        ld a3, 96(sp)
    80000a38:	7686                	ld	a3,96(sp)
        ld a4, 104(sp)
    80000a3a:	7726                	ld	a4,104(sp)
        ld a5, 112(sp)
    80000a3c:	77c6                	ld	a5,112(sp)
        ld a6, 120(sp)
    80000a3e:	7866                	ld	a6,120(sp)
        ld a7, 128(sp)
    80000a40:	688a                	ld	a7,128(sp)
        ld t3, 216(sp)
    80000a42:	6e6e                	ld	t3,216(sp)
        ld t4, 224(sp)
    80000a44:	7e8e                	ld	t4,224(sp)
        ld t5, 232(sp)
    80000a46:	7f2e                	ld	t5,232(sp)
        ld t6, 240(sp)
    80000a48:	7fce                	ld	t6,240(sp)

        addi sp, sp, 256
    80000a4a:	6111                	addi	sp,sp,256

        # return to whatever we were doing in the kernel.
        sret
    80000a4c:	10200073          	sret
	...

0000000080001000 <_trampoline>:
    80001000:	14051073          	csrw	sscratch,a0
    80001004:	4501                	li	a0,0
    80001006:	02153423          	sd	ra,40(a0)
    8000100a:	02253823          	sd	sp,48(a0)
    8000100e:	02353c23          	sd	gp,56(a0)
    80001012:	04453023          	sd	tp,64(a0)
    80001016:	04553423          	sd	t0,72(a0)
    8000101a:	04653823          	sd	t1,80(a0)
    8000101e:	04753c23          	sd	t2,88(a0)
    80001022:	f120                	sd	s0,96(a0)
    80001024:	f524                	sd	s1,104(a0)
    80001026:	fd2c                	sd	a1,120(a0)
    80001028:	e150                	sd	a2,128(a0)
    8000102a:	e554                	sd	a3,136(a0)
    8000102c:	e958                	sd	a4,144(a0)
    8000102e:	ed5c                	sd	a5,152(a0)
    80001030:	0b053023          	sd	a6,160(a0)
    80001034:	0b153423          	sd	a7,168(a0)
    80001038:	0b253823          	sd	s2,176(a0)
    8000103c:	0b353c23          	sd	s3,184(a0)
    80001040:	0d453023          	sd	s4,192(a0)
    80001044:	0d553423          	sd	s5,200(a0)
    80001048:	0d653823          	sd	s6,208(a0)
    8000104c:	0d753c23          	sd	s7,216(a0)
    80001050:	0f853023          	sd	s8,224(a0)
    80001054:	0f953423          	sd	s9,232(a0)
    80001058:	0fa53823          	sd	s10,240(a0)
    8000105c:	0fb53c23          	sd	s11,248(a0)
    80001060:	11c53023          	sd	t3,256(a0)
    80001064:	11d53423          	sd	t4,264(a0)
    80001068:	11e53823          	sd	t5,272(a0)
    8000106c:	11f53c23          	sd	t6,280(a0)
    80001070:	140022f3          	csrr	t0,sscratch
    80001074:	06553823          	sd	t0,112(a0)
    80001078:	00853103          	ld	sp,8(a0)
    8000107c:	02053203          	ld	tp,32(a0)
    80001080:	01053283          	ld	t0,16(a0)
    80001084:	00053303          	ld	t1,0(a0)
    80001088:	12000073          	sfence.vma
    8000108c:	18031073          	csrw	satp,t1
    80001090:	12000073          	sfence.vma
    80001094:	9282                	jalr	t0

0000000080001096 <userret>:
    80001096:	12000073          	sfence.vma
    8000109a:	18051073          	csrw	satp,a0
    8000109e:	12000073          	sfence.vma
    800010a2:	4501                	li	a0,0
    800010a4:	02853083          	ld	ra,40(a0)
    800010a8:	03053103          	ld	sp,48(a0)
    800010ac:	03853183          	ld	gp,56(a0)
    800010b0:	04053203          	ld	tp,64(a0)
    800010b4:	04853283          	ld	t0,72(a0)
    800010b8:	05053303          	ld	t1,80(a0)
    800010bc:	05853383          	ld	t2,88(a0)
    800010c0:	7120                	ld	s0,96(a0)
    800010c2:	7524                	ld	s1,104(a0)
    800010c4:	7d2c                	ld	a1,120(a0)
    800010c6:	6150                	ld	a2,128(a0)
    800010c8:	6554                	ld	a3,136(a0)
    800010ca:	6958                	ld	a4,144(a0)
    800010cc:	6d5c                	ld	a5,152(a0)
    800010ce:	0a053803          	ld	a6,160(a0)
    800010d2:	0a853883          	ld	a7,168(a0)
    800010d6:	0b053903          	ld	s2,176(a0)
    800010da:	0b853983          	ld	s3,184(a0)
    800010de:	0c053a03          	ld	s4,192(a0)
    800010e2:	0c853a83          	ld	s5,200(a0)
    800010e6:	0d053b03          	ld	s6,208(a0)
    800010ea:	0d853b83          	ld	s7,216(a0)
    800010ee:	0e053c03          	ld	s8,224(a0)
    800010f2:	0e853c83          	ld	s9,232(a0)
    800010f6:	0f053d03          	ld	s10,240(a0)
    800010fa:	0f853d83          	ld	s11,248(a0)
    800010fe:	10053e03          	ld	t3,256(a0)
    80001102:	10853e83          	ld	t4,264(a0)
    80001106:	11053f03          	ld	t5,272(a0)
    8000110a:	11853f83          	ld	t6,280(a0)
    8000110e:	7928                	ld	a0,112(a0)
    80001110:	10200073          	sret
	...
