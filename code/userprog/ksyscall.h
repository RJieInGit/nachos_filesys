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

char *
LoadStringFromMemory(int vAddr) {

    // printf("start load\n");
    bool terminated = false;
    char *buffer = new(std::nothrow) char[MAX_STRING_LENGTH];
    
    //memLock->Acquire();
    // printf("ld str acquire\n");
    for(int i = 0; i < MAX_STRING_LENGTH; ++i) {                   // iterate until max string length
        int pAddr = currentThread->space->V2P(vAddr + i);
        if((buffer[i] = machine->mainMemory[pAddr]) == '\0') {      // break if string ended
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
        int pAddr = currentThread->space->V2P(vAddr + i);
        machine->mainMemory[pAddr] = buffer[i];     // copy buffer from kernel-land back to user-land
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

int SysCreate(char *name,int protection){
 char *filename = LoadStringFromMemory(name);     // grab filename argument from register
    if(filename == NULL)    // cant load filename string, so error
        return ;

    DEBUG('a', "filename: %s\n", filename);
    fileSystem->Create(filename, 0, currentThread->space->wdSector);                    // attempt to create a new file

    delete [] filename;
    return ;
}

int SysRemove(char *name){
char *filename = LoadStringFromMemory(name); 
kernel->fileSystem->Remove(name,kernel->currentThread->wdSector);
}

//mode is a int. &1 &2 &4 represent read, write ,executable
OpenFileId SysOpen(char *name, int mode){
  char *filename = LoadStringFromMemory(name);     // grab filename argument from register    
    if(filename == NULL)   // cant load filename string, so error
        return -1;
    
    OpenFile *f = fileSystem->Open(filename, currentThread->space->wdSector);
  
    delete [] filename;
    if(f == NULL)        // cant open file, so error
        return -1;

    OpenFileId id = currentThread->space->fileVector->Insert(f);
    return id;  
}

int SysWrite(char *buffer, int size, OpenFileId id){
  // printf("start write\n");
    char *buffer = LoadStringFromMemory(machine->ReadRegister(4));     // grab buffer argument from register    
    if(buffer == NULL)     // error bad input
        return ;

    int size = machine->ReadRegister(5);                              // grab number of bytes to write
    OpenFileId id = machine->ReadRegister(6);                         // grab fileid of the file we want to write to


    if(id == ConsoleOutput) {                                       // if we want to Write to ConsoleOutput, use the SynchConsole
        
        ioLock->Acquire();
        char *curChar = buffer;                                     // iterate over the writebuffer and write out each character to ConsoleOutput
        while(size-- > 0)                         
            synchConsole->WriteChar(*curChar++);
        ioLock->Release();
    } 
    else {                                                           // else we are trying to write to an OpenFile
        
        ioLock->Acquire();
        OpenFile *f = currentThread->space->fileVector->Resolve(id);   // resolve the fileid to an OpenFile Object using the OpenFileTable
        if(f == NULL) {     // trying to read from bad fileid
            delete [] buffer;
            ioLock->Release();
            return ;
        }

        f->Write(buffer, size);                                       // write the buffer to the OpenFile object                         
        ioLock->Release();
    }

    // printf("end write\n");
    delete [] buffer;
    return ;
}

int SysRead(char *buffer, int size, OpenFileId id){

}

int SysSeek(int pos, OpenFileId id){

}

int SysClose(OpenFileId id){                  // grab fileid to close
    currentThread->space->fileVector->Remove(id);                // decrement a reference count to that OpenFile object in the OpenFileTable
    return ;
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
  return kernel->currentThread->childrenResult->at(id);
}



int SysPwd()
{
  
  int workSector;
  if (kernel->currentThread->father != NULL)
    workSector = kernel->currentThread->father->workSector;
  else
    workSector = kernel->currentThread->workSector;
  kernel->fileSystem->PrintFullPath(workSector);
  return 0;
}

void SysLsDir()
{
  // since exec thread joined from father thread, use father workSector
  int workSector;
  if (kernel->currentThread->father != NULL)
    workSector = kernel->currentThread->father->workSector;
  else
    workSector = kernel->currentThread->workSector;
  kernel->fileSystem->List(workSector);
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
