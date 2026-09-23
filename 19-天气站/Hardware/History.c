#include "History.h"
#include "W25Q64.h"

/*================================================================
 *  历史记录存储
 *
 *  用的是 W25Q64 的「追加式」写法，不是「存一条擦一次」：
 *
 *    NOR Flash 只能把 1 写成 0，想把 0 变回 1 必须擦掉整个 4KB 扇区，
 *    而每个扇区只有约 10 万次擦写寿命。所以这里从前往后一条一条追加，
 *    整个区写满了才统一擦一次重来。
 *
 *  布局：扇区 1 开始，共 16 个扇区 = 64KB，每条 8 字节 -> 可存 8192 条。
 *        扇区 0 留着以后放配置用。
 *
 *  开机不用额外维护计数器：擦除后的 Flash 是全 0xFF，扫到第一条
 *  timestamp == 0xFFFFFFFF 就知道存到哪了。
 *================================================================*/

#define HIST_FIRST_SECTOR   1
#define HIST_SECTOR_COUNT   16
#define HIST_RECORDS_MAX    (HIST_SECTOR_COUNT * (W25Q64_SECTOR_SIZE / sizeof(History_Record)))

static uint32_t histCount = 0;

static uint32_t RecAddr(uint32_t idx)
{
    return HIST_FIRST_SECTOR * W25Q64_SECTOR_SIZE + idx * sizeof(History_Record);
}

/* 开机扫描：找到第一条空白记录 */
void History_Init(void)
{
    uint32_t i;
    History_Record r;

    histCount = 0;
    for (i = 0; i < HIST_RECORDS_MAX; i++)
    {
        W25Q64_ReadData(RecAddr(i), (uint8_t *)&r, sizeof(r));
        if (r.timestamp == 0xFFFFFFFF) break;
        if (r.timestamp == 0) break;              /* 双保险 */
        histCount++;
    }
}

uint32_t History_Count(void)
{
    return histCount;
}

/* 追加一条记录 */
void History_Add(uint32_t timestamp, int16_t temp_x10, uint8_t light)
{
    uint32_t i;
    History_Record r;

    if (histCount >= HIST_RECORDS_MAX)
    {
        /* 区满了：整体擦除后从头再来（保存最新数据优先） */
        for (i = 0; i < HIST_SECTOR_COUNT; i++)
        {
            W25Q64_SectorErase((HIST_FIRST_SECTOR + i) * W25Q64_SECTOR_SIZE);
        }
        histCount = 0;
    }

    r.timestamp = timestamp;
    r.temp_x10  = temp_x10;
    r.light     = light;
    r.reserved  = 0xFF;        /* 保持擦除态 */

    W25Q64_PageProgram(RecAddr(histCount), (uint8_t *)&r, sizeof(r));
    histCount++;
}

/* 读第 index 条 */
uint8_t History_Get(uint32_t index, History_Record *rec)
{
    if (index >= histCount) return 0;
    W25Q64_ReadData(RecAddr(index), (uint8_t *)rec, sizeof(History_Record));
    return 1;
}

/* 清空 */
void History_Clear(void)
{
    uint32_t i;
    for (i = 0; i < HIST_SECTOR_COUNT; i++)
    {
        W25Q64_SectorErase((HIST_FIRST_SECTOR + i) * W25Q64_SECTOR_SIZE);
    }
    histCount = 0;
}
/*================================================================
 *  系统时间存档（放在扇区 0）
 *
 *  没有 RTC 电池的话，主电源一断 RTC 就归零，开机只能从默认
 *  时间重新走。所以运行中每隔一段时间把"最后一次已知时间"写进
 *  Flash，开机先读回来，时间就能接着走。
 *
 *  同样是追加式写入：每个槽 8 字节，扇区 0 能放 512 个槽，
 *  写满了才擦一次，保护擦写寿命。
 *
 *  槽格式：{ uint32_t time; uint32_t magic; }
 *================================================================*/
#define SYS_TIME_MAGIC   0xA55A1234u
#define SYS_SLOT_SIZE    8
#define SYS_SLOTS_MAX    (W25Q64_SECTOR_SIZE / SYS_SLOT_SIZE)

typedef struct {
    uint32_t time;
    uint32_t magic;
} SysTimeSlot;

typedef char SysTimeSlot_SizeCheck[(sizeof(SysTimeSlot) == 8) ? 1 : -1];

static uint32_t SysTime_NextSlot(void)
{
    uint32_t i;
    SysTimeSlot s;

    for (i = 0; i < SYS_SLOTS_MAX; i++)
    {
        W25Q64_ReadData(i * SYS_SLOT_SIZE, (uint8_t *)&s, sizeof(s));
        if (s.magic == 0xFFFFFFFF) return i;      /* 找到空槽 */
    }
    return SYS_SLOTS_MAX;                          /* 写满了 */
}

void History_SaveTime(uint32_t sec)
{
    SysTimeSlot s;
    uint32_t slot = SysTime_NextSlot();

    if (slot >= SYS_SLOTS_MAX)
    {
        W25Q64_SectorErase(0);                     /* 满了才擦一次 */
        slot = 0;
    }

    s.time  = sec;
    s.magic = SYS_TIME_MAGIC;
    W25Q64_PageProgram(slot * SYS_SLOT_SIZE, (uint8_t *)&s, sizeof(s));
}

uint32_t History_LoadTime(void)
{
    uint32_t i, found = 0;
    SysTimeSlot s;

    for (i = 0; i < SYS_SLOTS_MAX; i++)
    {
        W25Q64_ReadData(i * SYS_SLOT_SIZE, (uint8_t *)&s, sizeof(s));
        if (s.magic == SYS_TIME_MAGIC)          found = s.time;
        else if (s.magic == 0xFFFFFFFF)         break;    /* 到空槽为止 */
    }
    return found;
}