# ifndef FILETABLE_H
# define FILETABLE_H
#include "filesys.h"

#define MAX_OPEN_FILE_ID 100

class FileTableEntry
{
private:
    OpenFile *file;
    int refCount;

public:
    FileTableEntry(OpenFile *f)
    {
        file = f;
        refCount = 1;
    };
    void IncreaseReference()
    {
        refCount++;
    };
    void DecreaseReference()
    {
        refCount--;
    };
    int GetReferenceCount()
    {
        return refCount;
    };
    OpenFile * GetFile()
    {
        return file;
    }
    ~FileTableEntry()
    {
        delete file;
    };
};


class FileTable
{
private:
    FileTableEntry **table;
    int length;
    FileTableEntry * ReadEntry(int id);
    void WriteEntry(int id, OpenFile *f);
    void ClearEntry(int id);
    void ClearTable();
public:
    FileTable();
    int Insert(OpenFile *file);
    OpenFile *Resolve(int id);
    int Remove(int id);
    int AddReference(int id);
    ~FileTable();
};

class FileVector
{
private:
    int *idVector;
    int length;
public:
    FileVector(/* args */);
    int Insert(OpenFile *f);
    OpenFile *Resolve(int id);
    int Remove(int id);
    ~FileVector();
};


# endif