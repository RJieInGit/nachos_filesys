#include "fileblock.h"
#include "filehdr.h"
#include "kernel.h"
#include "main.h"

IndirectBlock:: IndirectBlock(){
    for(int i=0;i<MAX_SECTOR;i++){
        dataSectors[i]=EMPTY_BLOCK;
    }
}

int IndirectBlock::Allocate(PersistentBitmap *freeMap,itn numbSectors){
    DEBUG('f',"now allocating single indirct block\n");
    if(numSectors<0 || freeMap->NumClear()<numbSectors)
        return -1;
    DEBUG('f',"find enough sectors in disk successfully\n");
    int allocated=0;

    for(int i=0;i<MAX_SECTOR&&allocated<numSectors;i++){
        if(dataSectors[i]!=EMPTY_BLOCK)
        continue;
        dataSectors[i]=freeMap->Find();
        ASSERT(datsSectors[i]!=EMPTY_BLOCK);
        allocated++;
    }
    DEBUG('f',"block sectors allocated successfully\n");
    return allocated;

}

void IndirectBlock::Deallocate(PersistentBitmap *freeMap){
    DEBUG('f',"start to deallocate indirect block\n");
    for(int i=0;i<MAX_SECTOR;i++){
        int sector= dataSectors[i];
        if(sector==EMPTY_BLOCK)
        continue;
        ASSERT(freeMap->Test(sector));
        freeMap->Clear(sector);    
    }
    DEBUG('f',"indirect block deallocation done")
    
}

void IndirectBlock::WriteBack(int sector){
    kernel->synchDisk->WriteSector(sector,(char *)this);
}

void IndirectBlock::FetchFrom(int sector){
    kernel->synchDisk->ReadSector(sector,(char *)this);
}

int IndirectBlock::ByteToSector(int offset){
    int vBlock = offset/SectorSize;
    ASSERT(vBlock<MAX_SECTOR);
    int pBlock= dataSectors[vBlock];
    return pBlock;
}