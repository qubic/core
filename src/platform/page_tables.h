#pragma once

// x86-64 four-level page-table access for dirty-page tracking of large contract states under UEFI.
//
// The firmware identity-maps RAM (linear == physical) with the largest pages it can. For a tracked
// range we split those into 4 KiB PTEs once at start-up, before any application processor runs, so no
// TLB shootdown is ever needed. Afterwards the writing core can consume the hardware Dirty bit (PTE
// bit 6): scan the PTEs of the range, report the set ones, clear them and invlpg each cleared page.
// The invlpg is mandatory: a TLB entry that already carries D=1 stops the CPU from setting the bit
// again in memory. Because scan and clear run on the single writing core, every write between two
// collections is observed.
//
// Everything returns false on anything unexpected (5-level paging, unmapped entries); the caller then
// falls back to treating the whole range as dirty.

#ifndef NO_UEFI

#include <lib/platform_efi/uefi.h>
#include <lib/platform_common/qintrin.h>
#include "memory.h"

struct PageTables
{
    static constexpr unsigned long long PRESENT = 1ULL << 0;
    static constexpr unsigned long long RW = 1ULL << 1;
    static constexpr unsigned long long US = 1ULL << 2;
    static constexpr unsigned long long DIRTY = 1ULL << 6;
    static constexpr unsigned long long PS = 1ULL << 7;          // PDPTE/PDE: large page; PTE: PAT
    static constexpr unsigned long long PAT_LARGE = 1ULL << 12;  // PAT position in a large-page entry
    static constexpr unsigned long long ADDR_MASK = 0x000FFFFFFFFFF000ULL;
    static constexpr unsigned long long FLAGS_MASK = 0x8000000000000FFFULL; // low 12 flag bits + NX
    static constexpr unsigned long long CR0_WP = 1ULL << 16;
    static constexpr unsigned long long CR4_LA57 = 1ULL << 12;
    static constexpr unsigned long long EFLAGS_IF = 1ULL << 9;
    static constexpr unsigned long long PAGE = 4096;
    static constexpr unsigned long long LARGE_PAGE = 1ULL << 21;

    bool ready = false;
    unsigned long long tablesAllocated = 0; // 4 KiB page-table pages allocated by splitting

    // Must be called on the core whose CR3 will be used for all later calls (the BSP at init; all APs
    // inherit its tables). Fails on 5-level paging, which this walker does not handle.
    bool init()
    {
        ready = !(readCr4() & CR4_LA57);
        return ready;
    }

    // Split every large page covering [base, base + size) into 4 KiB PTEs. Call before starting APs.
    bool splitRange(const void* base, unsigned long long size)
    {
        if (!ready)
        {
            return false;
        }
        const unsigned long long begin = (unsigned long long)base & ~(PAGE - 1);
        const unsigned long long end = (unsigned long long)base + size;
        for (unsigned long long va = begin; va < end; va += LARGE_PAGE)
        {
            if (!pte(va, true))
            {
                return false;
            }
        }
        return true;
    }

    // On the writing core: report every 4 KiB page of [base, base + size) whose Dirty bit is set as
    // onDirty(pageIndex), then clear the bit and flush that page from this core's TLB. Pages that were
    // never written cost one PTE read. Returns false (with the range possibly half consumed) if a page
    // is unmapped or not split; the caller must then invalidate its cache.
    template <class F>
    bool collectDirty(const void* base, unsigned long long size, F onDirty)
    {
        if (!ready)
        {
            return false;
        }
        const unsigned long long begin = (unsigned long long)base;
        const unsigned long long end = begin + size;
        WriteProtectGuard guard; // page-table pages may be mapped read-only with CR0.WP set
        for (unsigned long long va2m = begin & ~(LARGE_PAGE - 1); va2m < end; va2m += LARGE_PAGE)
        {
            unsigned long long* table = pageTableOf(va2m);
            if (!table)
            {
                return false;
            }
            for (unsigned int k = 0; k < 512; k++)
            {
                const unsigned long long va = va2m + k * PAGE;
                if (va < begin)
                {
                    continue;
                }
                if (va >= end)
                {
                    break;
                }
                volatile unsigned long long* p = &table[k];
                if (!(*p & PRESENT))
                {
                    return false;
                }
                if (*p & DIRTY)
                {
                    onDirty((va - begin) / PAGE);
                    atomicAnd(p, ~DIRTY);
                    invlpg((const void*)va);
                }
            }
        }
        return true;
    }

private:
    // RAII: lift CR0.WP on this core while editing page-table pages that the firmware mapped read-only.
    struct WriteProtectGuard
    {
        unsigned long long cr0;
        unsigned long long eflags;
        WriteProtectGuard()
        {
            eflags = readEflags();
            disableInterrupts();
            cr0 = readCr0();
            if (cr0 & CR0_WP)
            {
                writeCr0(cr0 & ~CR0_WP);
            }
        }
        ~WriteProtectGuard()
        {
            if (cr0 & CR0_WP)
            {
                writeCr0(cr0);
            }
            if (eflags & EFLAGS_IF)
            {
                enableInterrupts();
            }
        }
    };

    static unsigned long long* tableAt(unsigned long long entry)
    {
        return (unsigned long long*)(entry & ADDR_MASK);
    }

    bool allocTable(unsigned long long** table)
    {
        EFI_PHYSICAL_ADDRESS address = 0;
        if (bs->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, 1, &address) != EFI_SUCCESS)
        {
            return false;
        }
        *table = (unsigned long long*)address;
        setMem(*table, PAGE, 0);
        tablesAllocated++;
        return true;
    }

    // Replace the large-page entry *e, covering 1 << shift bytes, by a table of 512 entries that map
    // the same bytes with the same flags one level down.
    bool splitEntry(unsigned long long* e, unsigned int shift)
    {
        const unsigned long long big = *e;
        const unsigned long long base = big & ADDR_MASK & ~((1ULL << shift) - 1);
        const unsigned long long flags = big & FLAGS_MASK & ~PS;
        const bool pat = (big & PAT_LARGE) != 0;
        const unsigned long long sub = 1ULL << (shift - 9);
        unsigned long long* table;
        if (!allocTable(&table))
        {
            return false;
        }
        for (unsigned int i = 0; i < 512; i++)
        {
            unsigned long long entry = (base + i * sub) | flags;
            if (sub == PAGE)
            {
                entry |= pat ? PS : 0; // bit 7 is PAT in a PTE
            }
            else
            {
                entry |= PS | (pat ? PAT_LARGE : 0);
            }
            table[i] = entry;
        }
        {
            WriteProtectGuard guard;
            *e = (unsigned long long)table | PRESENT | RW | (big & US);
        }
        for (unsigned long long off = 0; off < (1ULL << shift); off += PAGE)
        {
            invlpg((const void*)(base + off));
        }
        return true;
    }

    // The 4 KiB PTE mapping va. With split set, large pages on the way are split; otherwise nullptr
    // is returned for them. nullptr also for any non-present entry.
    unsigned long long* pte(unsigned long long va, bool split)
    {
        unsigned long long* table = pageTableOfImpl(va, split);
        return table ? &table[(va >> 12) & 511] : nullptr;
    }

    unsigned long long* pageTableOf(unsigned long long va)
    {
        return pageTableOfImpl(va, false);
    }

    unsigned long long* pageTableOfImpl(unsigned long long va, bool split)
    {
        unsigned long long* pml4 = tableAt(readCr3());
        unsigned long long* e = &pml4[(va >> 39) & 511];
        if (!(*e & PRESENT))
        {
            return nullptr;
        }
        unsigned long long* pdpt = tableAt(*e);
        e = &pdpt[(va >> 30) & 511];
        if (!(*e & PRESENT))
        {
            return nullptr;
        }
        if ((*e & PS) && (!split || !splitEntry(e, 30)))
        {
            return nullptr;
        }
        unsigned long long* pd = tableAt(*e);
        e = &pd[(va >> 21) & 511];
        if (!(*e & PRESENT))
        {
            return nullptr;
        }
        if ((*e & PS) && (!split || !splitEntry(e, 21)))
        {
            return nullptr;
        }
        return tableAt(*e);
    }

#if defined(_MSC_VER) && !defined(__clang__)
    static unsigned long long readCr0() { return __readcr0(); }
    static void writeCr0(unsigned long long v) { __writecr0(v); }
    static unsigned long long readCr3() { return __readcr3(); }
    static unsigned long long readCr4() { return __readcr4(); }
    static unsigned long long readEflags() { return __readeflags(); }
    static void disableInterrupts() { _disable(); }
    static void enableInterrupts() { _enable(); }
    static void invlpg(const void* va) { __invlpg((void*)va); }
    static void atomicAnd(volatile unsigned long long* p, unsigned long long mask) { _InterlockedAnd64((volatile long long*)p, (long long)mask); }
#else
    static unsigned long long readCr0() { unsigned long long v; __asm__ volatile("mov %%cr0, %0" : "=r"(v)); return v; }
    static void writeCr0(unsigned long long v) { __asm__ volatile("mov %0, %%cr0" : : "r"(v) : "memory"); }
    static unsigned long long readCr3() { unsigned long long v; __asm__ volatile("mov %%cr3, %0" : "=r"(v)); return v; }
    static unsigned long long readCr4() { unsigned long long v; __asm__ volatile("mov %%cr4, %0" : "=r"(v)); return v; }
    static unsigned long long readEflags() { unsigned long long v; __asm__ volatile("pushfq; popq %0" : "=r"(v)); return v; }
    static void disableInterrupts() { __asm__ volatile("cli" : : : "memory"); }
    static void enableInterrupts() { __asm__ volatile("sti" : : : "memory"); }
    static void invlpg(const void* va) { __asm__ volatile("invlpg (%0)" : : "r"(va) : "memory"); }
    static void atomicAnd(volatile unsigned long long* p, unsigned long long mask) { __sync_fetch_and_and(p, mask); }
#endif
};

#endif // NO_UEFI
