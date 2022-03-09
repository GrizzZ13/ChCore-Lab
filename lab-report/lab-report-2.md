# Lab-Report-2

*519021910685 王浩天*

*Grizzy@sjtu.edu.cn*

## 思考题 1：请思考多级⻚表相⽐单级⻚表带来的优势和劣势（如果有的话），并计算在 AArch64 ⻚表中分别以 4KB 粒度和 2MB 粒度映射 0～4GB 地址范围所需的物理内存⼤⼩（或⻚表⻚数量）。

优势：多级页表相对于单级页表，可以显著减少页表的大小，否则一张页表映射4kb全部64位地址空间就需要33554432GB的空间，而多级页表允许页表结构存在空洞，可以部分创建，减少页表占据的空间。

劣势：需要多次访存，复杂度高



若以4KB映射4GB的地址范围，则一共需要映射$2^{20}$页

需要$2^{20}$个三级页表条目，即$2^{11}$个三级页表

需要$2^{11}$个二级页表条目，即$2^2$个二级页表

共计2048 + 4 + 1 + 1 = 2054个页表项



若以2MB粒度映射4GB的地址范围，一共需要映射$2^{11}$页

需要$2^{11}$个二级页表条目，即$2^2$个二级页表

共计4 + 1 + 1 = 6个页表



## 练习题 2：请在 init_boot_pt 函数的 LAB 2 TODO 1 处配置内核⾼地址⻚表（ boot_ttbr1_l0 、 boot_ttbr1_l1 和 boot_ttbr1_l2 ），以 2MB 粒度映射。

```c
/* LAB 2 TODO 1 BEGIN */
/* Step 1: set L0 and L1 page table entry */
vaddr = KERNEL_VADDR;
boot_ttbr1_l0[GET_L0_INDEX(vaddr)] = ((u64)boot_ttbr1_l1) | IS_TABLE
                                     | IS_VALID | NG;
boot_ttbr1_l1[GET_L1_INDEX(vaddr)] = ((u64)boot_ttbr1_l2) | IS_TABLE
                                     | IS_VALID | NG;

/* Step 2: map PHYSMEM_START ~ PERIPHERAL_BASE with 2MB granularity */
vaddr = PHYSMEM_START;
for (; vaddr < PERIPHERAL_BASE; vaddr += SIZE_2M) {
        boot_ttbr1_l2[GET_L2_INDEX(vaddr)] =
                (vaddr)
                | UXN /* Unprivileged execute never */
                | ACCESSED /* Set access flag */
                | NG /* Mark as not global */
                | INNER_SHARABLE /* Sharebility */
                | NORMAL_MEMORY /* Normal memory */
                | IS_VALID;
}

/* Step 2: map PERIPHERAL_BASE ~ PHYSMEM_END with 2MB granularity */
for (vaddr = PERIPHERAL_BASE; vaddr < PHYSMEM_END; vaddr += SIZE_2M) {
        boot_ttbr1_l2[GET_L2_INDEX(vaddr)] =
                (vaddr)
                | UXN /* Unprivileged execute never */
                | ACCESSED /* Set access flag */
                | NG /* Mark as not global */
                | DEVICE_MEMORY /* Device memory */
                | IS_VALID;
}
/* LAB 2 TODO 1 END */
```



## 思考题 3：请思考在 init_boot_pt 函数中为什么还要为低地址配置⻚表，并尝试验证⾃⼰的解释。

## 练习题 4：完成 kernel/mm/buddy.c 中的 split_page 、 buddy_get_pages 、 merge_page 和buddy_free_pages 函数中的 LAB 2 TODO 2 部分，其中 buddy_get_pages ⽤于分配指定阶⼤⼩的连续物理⻚， buddy_free_pages ⽤于释放已分配的连续物理⻚。