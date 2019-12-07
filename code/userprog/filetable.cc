#include "filetable.h"
#include "main.h"

FileTable::FileTable()
{
    length = MAX_OPEN_FILE_ID;
    table = new FileTableEntry *[length];
}

FileTable::~FileTable()
{
    delete[] table;
}

FileTableEntry *
FileTable::ReadEntry(int id) {
	ASSERT(id >= 0 && id < MAX_OPEN_FILE_ID);
	return table[id]; 
}

void
FileTable::WriteEntry(int id, OpenFile *f) {
	ASSERT(id >= 0 && id < MAX_OPEN_FILE_ID);
	table[id] = new FileTableEntry(f);
	return ;
}

void
FileTable::ClearEntry(int id) {
	ASSERT(id >= 0 && id < MAX_OPEN_FILE_ID);
	FileTableEntry *entry = table[id];
	table[id] = NULL;
	delete entry;
	return ;
}

void
FileTable::ClearTable() {
	for(int i = 0; i < length; ++i)
		ClearEntry(i);
}

int
FileTable::Insert(OpenFile *file)
{
    for (int i = 0; i < length; i++)
    {
        if (ReadEntry(i) == NULL)
        {
            WriteEntry(i, file);
            return i;
        }
    }

    return -1;
}

OpenFile *
FileTable::Resolve(int id)
{
    if(id < 0 || id >= length)
		return NULL;

	FileTableEntry *entry = ReadEntry(id);
    if(entry == NULL)
		return NULL;
	
	return entry->GetFile();
}

int FileTable::Remove(int id)
{
	if(id < 0 || id >= length)
		return -1;

	FileTableEntry *entry = ReadEntry(id);
	if(entry == NULL)
		return -1;

	entry->DecreaseReference();
	if(entry->GetReferenceCount() <= 0)
		ClearEntry(id);

	return 0;
}

int FileTable::AddReference(int id)
{
	if(id < 0 || id >= length)			// invalid file id
		return -1;

	FileTableEntry *entry = ReadEntry(id);			// get TableEntry
	if(entry == NULL)		// entry does not exist
		return -1;

	entry->IncreaseReference();
	return 0;
}


FileVector::FileVector() {
	length = MAX_OPEN_FILE_ID;
	idVector = new int[length];

	idVector[0] = 0; // console in
	idVector[1] = 1; // console out
	kernel->globalFileTable->AddReference(0);
	kernel->globalFileTable->AddReference(1);

	for(int i = 2; i < length; ++i)					// iterate over vector, settting each value to FREE
		idVector[i] = -1;
}

FileVector::~FileVector() {
	for(int i = 0; i < length; ++i)
		kernel->globalFileTable->Remove(idVector[i]);

	delete[] idVector;
}

int
FileVector::Insert(OpenFile *f) {
	int globalId = kernel->globalFileTable->Insert(f);
	int localId = -1;
	for(int i = 2; i < length; ++i) {
		if(idVector[i] == -1) {
			idVector[i] = globalId;
			localId = i;
			break;
		}
	}

	return localId;
}

OpenFile *
FileVector::Resolve(int id) {
	if(id < 0 || id >= length)
		return NULL;

	return kernel->globalFileTable->Resolve(idVector[id]);
}

int
FileVector::Remove(int id) {
	if(id < 0 || id >= length)
		return -1;

	kernel->globalFileTable->Remove(idVector[id]);
	idVector[id] = -1;
	return 0;
}