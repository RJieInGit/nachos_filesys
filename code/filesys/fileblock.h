//this  module is to store the information of how data store in the disk. the fileblocks are indexed by the filehdr
#ifndef FILE_BLOCK_H
#define FILE_BLOCK_H
#include "main.h"
#include "../machine/disk.h"
#include "bitmap.h"
#include "pbitmap.h"

#define MAX_SECTOR (int)(SectorSize / sizeof(int))
#define EMPTY_BLOCK -1

class IndirectBlock{
    public:
        IndirectBlock();
        int Allocate(PersistentBitmap *bitMap,int numSectors);
        
        void Deallocate(Bitmap *bitmap);

        //get the fileblock from the disk
        void FetchFrom(int sectorNumber);
        void WriteBack(int sectorNumber); // write the fileblock index back to disk

        int ByteToSector(int offset); // calculate the offset to disk sector nubmer

        void Print();

    private: 
        int dataSectors[MAX_SECTOR];

};

#endif 