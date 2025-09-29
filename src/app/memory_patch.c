#include "microtbx.h"

// Переопределяем функции пула памяти, чтобы они напрямую использовали
// стандартную кучу FreeRTOS через обертки tbx_heap.
// Это обходит потенциальные баги в кастомном менеджере памяти mempool.

void * TbxMemPoolAllocateAuto(size_t size)
{
  return TbxHeapAllocate(size);
}

void TbxMemPoolRelease(void * memPtr)
{
  TbxHeapFree(memPtr);
}

// Оставляем пустую заглушку, чтобы компоновщик не ругался
uint8_t TbxMemPoolCreate(size_t numBlocks, size_t blockSize)
{
  (void)numBlocks;
  (void)blockSize;
  return TBX_OK;
}