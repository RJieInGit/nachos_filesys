// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"
#include "fileblock.h"

/*
constructor , initial file size would be 0
*/
FileHeader::FileHeader(){
    for(int i=0;i<NumDirect;i++){
        dataSectors[i]=EMPTY_BLOCK;
        numBytes=0;
        numSectors=0;
    }
}


//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	the initial file size is 0 and we dynamically extend it when we write to 
//  the file later
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------


bool
FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
{ 
    DEBUG('f',"now start file header allocation\n");
   
    int needSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
	return FALSE;		// not enough space
    DEBUG('f',"enough space for the file\n");
    IndirectBlock *block;
    int allocated=0;
    for (int i = 0; i < numSectors && allocated<needSectors; i++) {
        block = new IndirectBlock();
        if(dataSectors[i] ==EMPTY_BLOCK)
            dataSectors[i]= freeMap->Find();
        else
            block->FetchFrom(dataSectors[i]);
        ASSERT(dataSectors[i] != EMPTY_BLOCK);     // if the sector did not store a block, it should have one after we create assign one to it
        int sectorsInBlock = block(freeMap,needSectors- allocated);
        ASSERT(sectorsInBlock != -1);
        block->WriteBack(dataSectors[i]);          // store the block to disk
        allocated+= sectorsInBlock;
        delete block;                              //we already write it to disk, free the memory
    }
    ASSERT(needSectors < =allocated);
     numBytes += fileSize;
     numSectors  += divRoundUp(fileSize, SectorSize);
    DEBUG('f', "file header allocated\n");
    return true;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated in the blocks allocated in the filehdr
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(PersistentBitmap *freeMap)
{
    DEBUG('f', "beginning filehdr deallocation\n");
    IndirectBlock *block;
    for (int i = 0; i < numSectors; i++) {
        int blockSector = dataSectors[i];
        if(blockSector==EMPTY_BLOCK)
            continue;                    // do nothing if we dont have a block here
        ASSERT(freeMap->Test(sector));   // assert the sector do occupied
        block=new IndirectBlock();
        block->FetchFrom(blockSector);
        block->Deallocate(freeMap);
        ASSERT(freeMap->Test(sector));   // to be deleted
        freeMap->Clear(sector);
        dataSectors[i] = EMPTY_BLOCK;    // why github guy dont do this?????
        delete block;

    }
    DEBUG('f', "finished filehdr deallocation\n");
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    kernel->synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    kernel->synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    int vBlock = offset/SectorSize;
    IndirectBlock *block =new IndirectBlock();
    block->FetchFrom(dataSectors[vBlock/MAX_SECTOR]);
    int pBlock= block->ByteToSector((vBlock%MAX_SECTOR) *SectorSize);
    ASSERT(pBLock>=0 && pBlock < NumSectors);
    delete block;
    return pBlock;

}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
	printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	kernel->synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}
