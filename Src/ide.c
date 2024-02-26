#include "ide.h"

IDEChannelRegisters_t channels[2];
ide_device_t ide_devices[4];
uint8_t ide_buf[SECTOR_SIZE] = {0};
uint8_t package[2] = {0};

//the most of the code is taken from the tutorial at https://wiki.osdev.org/PCI_IDE_Controller

//these two functions are used to access registers on the ata drive
uint8_t ide_read_from_registers(uint8_t channel, uint8_t reg) {
    uint8_t result;
    if (reg > 0x07 && reg < 0x0C)
        ide_write_to_registers(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        result = port_byte_in(channels[channel].base + reg - 0x00);
    else if (reg < 0x0C)
        result = port_byte_in(channels[channel].base  + reg - 0x06);
    else if (reg < 0x0E)
        result = port_byte_in(channels[channel].ctrl  + reg - 0x0A);
    else if (reg < 0x16)
        result = port_byte_in(channels[channel].bmide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        ide_write_to_registers(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    return result;
}

void ide_write_to_registers(uint8_t channel, uint8_t reg, uint8_t data) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write_to_registers(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        port_byte_out(channels[channel].base  + reg - 0x00, data);
    else if (reg < 0x0C)
        port_byte_out(channels[channel].base  + reg - 0x06, data);
    else if (reg < 0x0E)
        port_byte_out(channels[channel].ctrl  + reg - 0x0A, data);
    else if (reg < 0x16)
        port_byte_out(channels[channel].bmide + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        ide_write_to_registers(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}


// read long word from reg port for quads times
void insl(uint16_t reg, uint32_t *buffer, uint32_t quads)
{
    int index;
    for (index = 0; index < quads; index++)
    {
        asm volatile ("inl %%dx, %%eax" : "=a" (buffer[index]) : "dN" (reg));
    }
}


// write long word to reg port for quads times
void outsl(uint16_t reg, uint32_t *buffer, uint32_t quads)
{
  int index;
  for (index = 0; index < quads; index++)
  {
    asm volatile ("outl %%eax, %%dx" : : "dN" (reg), "a" (buffer[index]));
  }
}

void ide_read_buffer(uint8_t channel, uint8_t reg, uint32_t buffer, uint32_t quads) 
{
    if (reg > 0x07 && reg < 0x0C)
        ide_write_to_registers(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    asm("pushw %es; movw %ds, %ax; movw %ax, %es");
    if (reg < 0x08)
        insl(channels[channel].base  + reg - 0x00, buffer, quads);
    else if (reg < 0x0C)
        insl(channels[channel].base  + reg - 0x06, buffer, quads);
    else if (reg < 0x0E)
        insl(channels[channel].ctrl  + reg - 0x0A, buffer, quads);
    else if (reg < 0x16)
        insl(channels[channel].bmide + reg - 0x0E, buffer, quads);
    asm("popw %es;");
    if (reg > 0x07 && reg < 0x0C)
        ide_write_to_registers(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

uint8_t ide_polling(uint8_t channel, uint32_t advanced_check) {
    // (I) Delay 400 nanosecond for BSY to be set:
    // -------------------------------------------------
    for(uint8_t i = 0; i < 4; i++)
        ide_read_from_registers(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.
 
    // (II) Wait for BSY to be cleared:
    // -------------------------------------------------
    while (ide_read_from_registers(channel, ATA_REG_STATUS) & ATA_SR_BSY); // Wait for BSY to be zero.
 
    if (advanced_check) {
        uint8_t state = ide_read_from_registers(channel, ATA_REG_STATUS); // Read Status Register.
 
        // (III) Check For Errors:
        // -------------------------------------------------
        if (state & ATA_SR_ERR)
            return 2; // Error.
 
        // (IV) Check If Device fault:
        // -------------------------------------------------
        if (state & ATA_SR_DF)
            return 1; // Device Fault.
 
        // (V) Check DRQ:
        // -------------------------------------------------
        // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
        if ((state & ATA_SR_DRQ) == 0)
            return 3; // DRQ should be set
 
    }
   return 0; // No Error.
}

uint8_t ide_print_error(uint32_t drive, uint8_t err) {
    if (err == 0)
        return err;
 
    kprintf("IDE:");
    if (err == 1) {kprintf("- Device Fault\n     "); err = 19;}
    else if (err == 2) {
        uint8_t st = ide_read_from_registers(ide_devices[drive].Channel, ATA_REG_ERROR);
        if (st & ATA_ER_AMNF)   {kprintf("- No Address Mark Found\n     ");   err = 7;}
        if (st & ATA_ER_TK0NF)   {kprintf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_ABRT)   {kprintf("- Command Aborted\n     ");      err = 20;}
        if (st & ATA_ER_MCR)   {kprintf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_IDNF)   {kprintf("- ID mark not Found\n     ");      err = 21;}
        if (st & ATA_ER_MC)   {kprintf("- No Media or Media Error\n     ");   err = 3;}
        if (st & ATA_ER_UNC)   {kprintf("- Uncorrectable Data Error\n     ");   err = 22;}
        if (st & ATA_ER_BBK)   {kprintf("- Bad Sectors\n     ");       err = 13;}
    }
    else  if (err == 3)           {kprintf("- Reads Nothing\n     "); err = 23;}
    else  if (err == 4)  {kprintf("- Write Protected\n     "); err = 8;}
    kprintf("- [%s %s] %s\n",
        (const char *[]){"Primary", "Secondary"}[ide_devices[drive].Channel], // Use the channel as an index into the array
        (const char *[]){"Master", "Slave"}[ide_devices[drive].Drive], // Same as above, using the drive
        ide_devices[drive].Model);
    return err;
}

void ide_initialize(uint32_t BAR0, uint32_t BAR1, uint32_t BAR2, uint32_t BAR3, uint32_t BAR4) 
{
 
    int i, j, k, count = 0;
 
    // 1- Detect I/O Ports which interface IDE Controller:
    channels[ATA_PRIMARY].base  = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
    channels[ATA_PRIMARY].ctrl  = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
    channels[ATA_SECONDARY].base  = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
    channels[ATA_SECONDARY].ctrl  = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
    channels[ATA_PRIMARY].bmide = (BAR4 & 0xFFFFFFFC) + 0; // Bus Master IDE
    channels[ATA_SECONDARY].bmide = (BAR4 & 0xFFFFFFFC) + 8; // Bus Master IDE

      // 2- Disable IRQs:
    ide_write_to_registers(ATA_PRIMARY  , ATA_REG_CONTROL, 2);
    ide_write_to_registers(ATA_SECONDARY, ATA_REG_CONTROL, 2);

    // 3- Detect ATA-ATAPI Devices:
    for (i = 0; i < 2; i++)
        for (j = 0; j < 2; j++) {
    
            uint8_t err = 0, type = IDE_ATA, status;
            ide_devices[count].Reserved = 0; // Assuming that no drive here.
    
            // (I) Select Drive:
            ide_write_to_registers(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
    
            // (II) Send ATA Identify Command:
            ide_write_to_registers(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    
            // (III) Polling:
            if (ide_read_from_registers(i, ATA_REG_STATUS) == 0) continue; // If Status = 0, No Device.
    
            while(1) {
                status = ide_read_from_registers(i, ATA_REG_STATUS);
                if ((status & ATA_SR_ERR)) {err = 1; break;} // If Err, Device is not ATA.
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything is right.
            }
    
            // (IV) Probe for ATAPI Devices:
    
            if (err != 0) {
                uint8_t cl = ide_read_from_registers(i, ATA_REG_LBA1);
                uint8_t ch = ide_read_from_registers(i, ATA_REG_LBA2);
    
                if (cl == 0x14 && ch == 0xEB)
                type = IDE_ATAPI;
                else if (cl == 0x69 && ch == 0x96)
                type = IDE_ATAPI;
                else
                continue; // Unknown Type (may not be a device).
    
                ide_write_to_registers(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
            }
    
            // (V) Read Identification Space of the Device:
            ide_read_buffer(i, ATA_REG_DATA, (uint32_t) ide_buf, 128);

            // (VI) Read Device Parameters:
            ide_devices[count].Reserved     = 1;
            ide_devices[count].Type         = type;
            ide_devices[count].Channel      = i;
            ide_devices[count].Drive        = j;
            ide_devices[count].Signature    = *((uint16_t*)(ide_buf + ATA_IDENT_DEVICETYPE));
            ide_devices[count].Capabilities = *((uint16_t*)(ide_buf + ATA_IDENT_CAPABILITIES));
            ide_devices[count].CommandSets  = *((uint32_t*)(ide_buf + ATA_IDENT_COMMANDSETS));

            // (VII) Get Size:
            if (ide_devices[count].CommandSets & (1 << 26))
                // Device uses 48-Bit Addressing:
                ide_devices[count].Size   = *((uint32_t*)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
            else
                // Device uses CHS or 28-bit Addressing:
                ide_devices[count].Size   = *((uint32_t*)(ide_buf + ATA_IDENT_MAX_LBA));
            
            // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
            for(k = 0; k < 40; k += 2) {
                ide_devices[count].Model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
                ide_devices[count].Model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];}
            ide_devices[count].Model[40] = 0; // Terminate String.
            count++;
        }
}

uint8_t ide_ata_access(uint8_t direction, uint8_t drive, uint32_t lba, uint8_t numsects, uint32_t buffer_to_use)
{
    uint8_t lba_mode /* 0: CHS, 1:LBA28, 2: LBA48 */, dma /* 0: No DMA, 1: DMA */, cmd;
    uint8_t lba_io[6];
    uint32_t channel      = ide_devices[drive].Channel; // Read the Channel.
    uint32_t slavebit      = ide_devices[drive].Drive; // Read the Drive [Master/Slave]
    uint32_t bus = channels[channel].base; // Bus Base, like 0x1F0 which is also data port.
    uint32_t words = 256; // Almost every ATA drive has a sector-size of 512-byte.
    uint16_t cyl, i;
    uint8_t head, sect, err;

    // (I) Select one from LBA28, LBA48 or CHS;
    if (lba >= 0x10000000) { // Sure Drive should support LBA in this case, or you are
                                // giving a wrong LBA.
        // LBA48:
        lba_mode  = 2;
        lba_io[0] = (lba & 0x000000FF) >> 0;
        lba_io[1] = (lba & 0x0000FF00) >> 8;
        lba_io[2] = (lba & 0x00FF0000) >> 16;
        lba_io[3] = (lba & 0xFF000000) >> 24;
        lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        head      = 0; // Lower 4-bits of HDDEVSEL are not used here.
    } else if (ide_devices[drive].Capabilities & 0x200)  { // Drive supports LBA?
        // LBA28:
        lba_mode  = 1;
        lba_io[0] = (lba & 0x00000FF) >> 0;
        lba_io[1] = (lba & 0x000FF00) >> 8;
        lba_io[2] = (lba & 0x0FF0000) >> 16;
        lba_io[3] = 0; // These Registers are not used here.
        lba_io[4] = 0; // These Registers are not used here.
        lba_io[5] = 0; // These Registers are not used here.
        head      = (lba & 0xF000000) >> 24;
    } else {
        // CHS:
        lba_mode  = 0;
        sect      = (lba % 63) + 1;
        cyl       = (lba + 1  - sect) / (16 * 63);
        lba_io[0] = sect;
        lba_io[1] = (cyl >> 0) & 0xFF;
        lba_io[2] = (cyl >> 8) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head      = (lba + 1  - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
    }

    // (II) See if drive supports DMA or not;
    dma = 0; // We don't support DMA

    // (III) Wait if the drive is busy;
    while (ide_read_from_registers(channel, ATA_REG_STATUS) & ATA_SR_BSY){   
    
    } // Wait if busy.
    
    // (IV) Select Drive from the controller;
    if (lba_mode == 0)
        ide_write_to_registers(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS.
    else
        ide_write_to_registers(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA

    // (V) Write Parameters to registers;
    if (lba_mode == 2) {
        ide_write_to_registers(channel, ATA_REG_SECCOUNT1,   0);
        ide_write_to_registers(channel, ATA_REG_LBA3,   lba_io[3]);
        ide_write_to_registers(channel, ATA_REG_LBA4,   lba_io[4]);
        ide_write_to_registers(channel, ATA_REG_LBA5,   lba_io[5]);
    }
    ide_write_to_registers(channel, ATA_REG_SECCOUNT0,   numsects);
    ide_write_to_registers(channel, ATA_REG_LBA0,   lba_io[0]);
    ide_write_to_registers(channel, ATA_REG_LBA1,   lba_io[1]);
    ide_write_to_registers(channel, ATA_REG_LBA2,   lba_io[2]);

    // (VI) Select the command and send it;
    // Routine that is followed:
    // If ( DMA & LBA48)   DO_DMA_EXT;
    // If ( DMA & LBA28)   DO_DMA_LBA;
    // If ( DMA & LBA28)   DO_DMA_CHS;
    // If (!DMA & LBA48)   DO_PIO_EXT;
    // If (!DMA & LBA28)   DO_PIO_LBA;
    // If (!DMA & !LBA#)   DO_PIO_CHS;

    if (lba_mode == 0 && dma == 0 && direction == ATA_READ) cmd = ATA_CMD_READ_PIO;
    if (lba_mode == 1 && dma == 0 && direction == ATA_READ) cmd = ATA_CMD_READ_PIO;   
    if (lba_mode == 2 && dma == 0 && direction == ATA_READ) cmd = ATA_CMD_READ_PIO_EXT;   
    if (lba_mode == 0 && dma == 1 && direction == ATA_READ) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 1 && dma == 1 && direction == ATA_READ) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 2 && dma == 1 && direction == ATA_READ) cmd = ATA_CMD_READ_DMA_EXT;
    if (lba_mode == 0 && dma == 0 && direction == ATA_WRITE) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 1 && dma == 0 && direction == ATA_WRITE) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 2 && dma == 0 && direction == ATA_WRITE) cmd = ATA_CMD_WRITE_PIO_EXT;
    if (lba_mode == 0 && dma == 1 && direction == ATA_WRITE) cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 1 && dma == 1 && direction == ATA_WRITE) cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 2 && dma == 1 && direction == ATA_WRITE) cmd = ATA_CMD_WRITE_DMA_EXT;
    ide_write_to_registers(channel, ATA_REG_COMMAND, cmd);               // Send the Command.

    if (dma)
        if (direction == 0);
            // DMA Read.
        else;
            // DMA Write.
   else
        if (direction == 0)
            // PIO Read.
        for (i = 0; i < numsects; i++) {
            if (err = ide_polling(channel, 1))
                return err; // Polling, set error and exit if there is.
            asm("pushw %es");
            asm("rep insw" : : "c"(words), "d"(bus), "D"(buffer_to_use)); // Receive Data.
            asm("popw %es");
            buffer_to_use += (words*2);
        } 
        else {
            // PIO Write.
            for (i = 0; i < numsects; i++) {
                ide_polling(channel, 0); // Polling.
                asm("pushw %ds");
                asm("rep outsw"::"c"(words), "d"(bus), "S"(buffer_to_use)); // Send Data
                asm("popw %ds");
                buffer_to_use += (words*2);
            }
            ide_write_to_registers(channel, ATA_REG_COMMAND, (char []) {   ATA_CMD_CACHE_FLUSH,
                        ATA_CMD_CACHE_FLUSH,
                        ATA_CMD_CACHE_FLUSH_EXT}[lba_mode]);
            ide_polling(channel, 0); // Polling.
        }
 
   return 0;
}


void ide_read_sectors(uint8_t drive, uint8_t numsects, uint32_t lba, uint32_t buffer_to_read_to) 
{
    // 1: Check if the drive presents:
    // ==================================
    if (drive > 3 || ide_devices[drive].Reserved == 0)
    {
        package[0] = 0x1;      // Drive Not Found!
    }
    // 2: Check if inputs are valid:
    // ==================================
    else if (((lba + numsects) > ide_devices[drive].Size) && (ide_devices[drive].Type == IDE_ATA))
    {
        package[0] = 0x2;                     // Seeking to invalid position.
    }
    
    // 3: Read in PIO Mode through Polling & IRQs:
    // ============================================
    else 
    {
        uint8_t err;
        if (ide_devices[drive].Type == IDE_ATA)
            err = ide_ata_access(ATA_READ, drive, lba, numsects, buffer_to_read_to);
        else if (ide_devices[drive].Type == IDE_ATAPI)
         
        package[0] = ide_print_error(drive, err);
    }
}
// package[0] is an entry of an array. It contains the Error Code.

void ide_write_sectors(uint8_t drive, uint8_t numsects, uint32_t lba, uint32_t buffer_to_write_from)
{
 
    // 1: Check if the drive presents:
    // ==================================
    if (drive > 3 || ide_devices[drive].Reserved == 0)
        package[0] = 0x1;      // Drive Not Found!
    // 2: Check if inputs are valid:
    // ==================================
    else if (((lba + numsects) > ide_devices[drive].Size) && (ide_devices[drive].Type == IDE_ATA))
        package[0] = 0x2;                     // Seeking to invalid position.
    // 3: Read in PIO Mode through Polling & IRQs:
    // ============================================
    else {
        uint8_t err;
        if (ide_devices[drive].Type == IDE_ATA)
            err = ide_ata_access(ATA_WRITE, drive, lba, numsects, buffer_to_write_from);
        else if (ide_devices[drive].Type == IDE_ATAPI)
            err = 4; // Write-Protected.
        package[0] = ide_print_error(drive, err);
    }
}

uint16_t bytes_in_current_sector(uint32_t current_address, uint32_t remaining_size)
{
  const uint32_t address_in_sector = current_address % SECTOR_SIZE;
  uint16_t relevant_bytes = remaining_size;

  if(address_in_sector + remaining_size > SECTOR_SIZE)
  {
    relevant_bytes = SECTOR_SIZE - address_in_sector;
  }

  return relevant_bytes;
}


void ide_access(uint8_t drive, uint32_t address, uint32_t size, uint32_t buffer, uint8_t write)
{
    uint16_t sector = address / SECTOR_SIZE;
    while (size > 0)
    {
        ide_read_sectors(drive, 1, sector, (uint32_t)ide_buf);
        const uint16_t bytes_in_current = bytes_in_current_sector(sector, size);
        if (write)
        {
            memcpy(ide_buf + (address % SECTOR_SIZE), buffer, bytes_in_current);
            ide_write_sectors(drive, 1, sector, (uint32_t)ide_buf);
        }
        else
        {
            memcpy(buffer, ide_buf + (address % SECTOR_SIZE), bytes_in_current);
        }
        buffer += bytes_in_current;
        size -= bytes_in_current;
        address += bytes_in_current;
        sector += 1;
    }
    memset(ide_buf, 0, SECTOR_SIZE);
}