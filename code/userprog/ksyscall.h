/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__ 
#define __USERPROG_KSYSCALL_H__ 

#include "kernel.h"
#include "thread.h"
#include "list.h"
#define MAX_STRING_LENGTH 128  //max length 
char *
LoadStringFromMemory(int vAddr) {

    // printf("start load\n");
    bool terminated = false;
    char *buffer = new(std::nothrow) char[MAX_STRING_LENGTH];
    
    //memLock->Acquire();
    // printf("ld str acquire\n");
    for(int i = 0; i < MAX_STRING_LENGTH; ++i) {                   // iterate until max string length
        int pAddr = kernel->currentThread->space->V2P(vAddr + i);
        if((buffer[i] = kernel->machine->mainMemory[pAddr]) == '\0') {      // break if string ended
          terminated = true;
          break;
        }
    }

    //memLock->Release();
    // printf("end load\n");

    if(!terminated) {                         // if the string is longer than the max string length, return NULL
        delete [] buffer;
        return NULL;
    }

    buffer[MAX_STRING_LENGTH - 1] = '\0';                       // truncate string to protect against overflow
    return buffer;
}

void
SaveStringToMemory(char* buffer, int numRead, int vAddr) {
    // printf("start read\n");
    //memLock->Acquire();
    for(int i = 0; i < numRead; ++i) {                   // iterate over the amount of bytes read
        int pAddr = kernel->currentThread->space->V2P(vAddr + i);
        kernel->machine->mainMemory[pAddr] = buffer[i];     // copy buffer from kernel-land back to user-land
    }
    // printf("end read\n");
    //memLock->Release();

    return ;
}

void SysHalt()
{
  kernel->interrupt->Halt();
}


int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

int SysCreate(int name,int protection){
 char *filename = LoadStringFromMemory(name);     // grab filename argument from register
    if(filename == NULL)    // cant load filename string, so error
        return 0;

    DEBUG('a', "filename: %s\n", filename);
    kernel->fileSystem->Create(filename, 0, kernel->currentThread->space->wdSector);                    // attempt to create a new file

    delete [] filename;
    return 0;
}

int SysRemove(int addr){
char *filename = LoadStringFromMemory(addr); 
kernel->fileSystem->Remove(filename,kernel->currentThread->wdSector);
}

//mode is a int. &1 &2 &4 represent read, write ,executable
OpenFileId SysOpen(int addr, int mode){
  char *filename = LoadStringFromMemory(addr);     // grab filename argument from register    
    if(filename == NULL)   // cant load filename string, so error
        return -1;
    
    OpenFile *f = kernel->fileSystem->Open(filename, kernel->currentThread->space->wdSector);
  
    delete [] filename;
    if(f == NULL)        // cant open file, so error
        return -1;

    OpenFileId id = currentThread->space->fileVector->Insert(f);
    return id;  
}

int SysWrite(int addr, int size, OpenFileId id){
  // printf("start write\n");
    char *buffer = LoadStringFromMemory(addr);     // grab buffer argument from register    
    if(buffer == NULL)     // error bad input
        return 0;
    if(id == ConsoleOutput) {                                       // if we want to Write to ConsoleOutput, use the SynchConsole
        
        //ioLock->Acquire();
        char *curChar = buffer;                                     // iterate over the writebuffer and write out each character to ConsoleOutput
        while(size-- > 0)                         
            kernel->synchConsoleOut->WriteChar(*curChar++);
        //ioLock->Release();
    } 
    else {                                                           // else we are trying to write to an OpenFile
        
        //ioLock->Acquire();
        OpenFile *f = currentThread->space->fileVector->Resolve(id);   // resolve the fileid to an OpenFile Object using the OpenFileTable
        if(f == NULL) {     // trying to read from bad fileid
            delete [] buffer;
            ioLock->Release();
            return 0;
        }

        f->Write(buffer, size);                                       // write the buffer to the OpenFile object                         
    }

    // printf("end write\n");
    delete [] buffer;
    return 0;
}

int SysRead(int addr, int size, OpenFileId id){
  char *buffer = getStringFromAddr(kernel->machine->ReadRegister(4));
  if (id == ConsoleInputID)
  {
    int total = 0;
    while (size > 0)
    {
      buffer[total] = kernel->synchConsoleIn->GetChar();
      total++;
      size--;
    }
    // write back for console
    int addr = kernel->machine->ReadRegister(4);
    int i = 0;
    int value;
    do
    {
      value = buffer[i];
      if (!kernel->machine->WriteMem(addr + i, 1, value))
      {
        kernel->machine->WriteMem(addr + i, 1, value);
      }
      i++;
    } while ((char)value != '\0');
    return total;
  }
  OpenFile *file = kernel->currentThread->fileVector->Resolve(id);
  if (file == NULL)
    return -1;
  int result = file->Read(buffer, size);
  return result;
}

int SysSeek(int pos, OpenFileId id){
  OpenFile *file = kernel->currentThread->fileVector->Resolve(id);
  if (file == NULL)
    return -1;
  file->Seek(pos);
  return 0;
}

int SysClose(OpenFileId id){                  // grab fileid to close
    currentThread->space->fileVector->Remove(id);                // decrement a reference count to that OpenFile object in the OpenFileTable
    return 0;
}

int SC_JOIN(SpaceId id){
   Thread *child = NULL;
  ListIterator<Thread *> *iter = new ListIterator<Thread *>(kernel->currentThread->childList);
  while (!iter->IsDone())
  {
    if (iter->Item()->PID == id)
    {
      child = iter->Item();
      break;
    }
    iter->Next();
  }
  delete iter;
  if (child == NULL || kernel->currentThread->waitingFor != -1)
    return -1;
  else if (kernel->currentThread->childrenResult->find(id) == kernel->currentThread->childrenResult->end())
  {
    IntStatus oldlevel = kernel->interrupt->SetLevel(IntOff);
    kernel->currentThread->waitingFor = id;
    kernel->waitingChildrenList->Append(kernel->currentThread);
    kernel->currentThread->Sleep(false);
    kernel->interrupt->SetLevel(oldlevel);
  }
  return kernel->currentThread->childList->at(id);
}



int SysPwd()
{
  
  int workSector;
  if (kernel->currentThread->father != NULL)
    workSector = kernel->currentThread->father->wdSector;
  else
    workSector = kernel->currentThread->wdSector;
  kernel->fileSystem->PrintFullPath(workSector);
  return 0;
}

void SysLsDir()
{
  // since exec thread joined from father thread, use father workSector
  int workSector;
  if (kernel->currentThread->father != NULL)
    workSector = kernel->currentThread->father->wdSector;
  else
    workSector = kernel->currentThread->wdSector;
  kernel->fileSystem->List(workSector);
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
