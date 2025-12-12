#define THEORY_IMPL
#define THEORY_DUCKTAPE
#define THEORY_SLIM_BUILD
#include <theory/theory.hh>

using namespace UFG;

bool ConvertToBin(void* buffer, s64 bufferSize, const qString& filename)
{
    qChunkFileBuilder chunk_builder;
    if (!chunk_builder.CreateBuilder("PC64", filename)) {
        return 0;
    }

    u32 screen_size = static_cast<u32>(sizeof(UIScreenChunk) + bufferSize);

    auto screen = static_cast<UIScreenChunk*>(qMalloc(screen_size));
    qMemSet(screen, 0, sizeof(UIScreenChunk));

    auto screenName = filename.GetFilenameWithoutExtension();
    screen->mNode.SetUID(screenName.GetStringHashUpper32());
    screen->mTypeUID = RTypeUID_UIScreen;
    screen->SetDebugName(screenName);

    screen->mBufferSize = static_cast<u32>(bufferSize);
    screen->mBuffer.Set(&screen[1]);

    memcpy(screen->mBuffer.Get(), buffer, bufferSize);

    chunk_builder.BeginChunk(ChunkUID_UIScreen, "UIScreenChunk");
    chunk_builder.Write(screen, screen_size);
    chunk_builder.EndChunk(ChunkUID_UIScreen);

    qFree(screen);

    bool dummy;
    chunk_builder.CloseBuilder(&dummy, 0);

    return 1;
}

bool ConvertToSWF(UIScreenChunk* screen, const qString& filename)
{
    auto buffer = screen->mBuffer.Get();
    if (!buffer || !screen->mBufferSize) {
        return 0;
    }

    auto file = qOpen(filename, QACCESS_WRITE);
    if (!file) {
        return 0;
    }

    qWrite(file, buffer, screen->mBufferSize);
    qClose(file);

    return 1;
}

int DetermineFileType(void* data, s64 size)
{
    if (size >= sizeof(UIScreenChunk))
    {
        auto chunk = static_cast<qChunk*>(data);
        auto screen = static_cast<UIScreenChunk*>(chunk->GetData());
        if (chunk->mUID == ChunkUID_UIScreen && screen->mTypeUID == RTypeUID_UIScreen) {
            return 0;
        }
    }

    if (size > 3)
    {
        auto f = static_cast<u8*>(data);
        if ((f[0] == 'S' && f[1] == 'F' && f[2] == 'X') ||
            (f[0] == 'C' && f[1] == 'F' && f[2] == 'X') ||
            (f[0] == 'C' && f[1] == 'W' && f[2] == 'S') ||
            (f[0] == 'G' && f[1] == 'F' && f[2] == 'X'))
        {
            return 1;
        }
    }

    return -1;
}

int main(int argc, char** argv)
{
    qInit(0);

    if (1 >= argc)
    {
        auto filename = qString(argv[0]).GetFilename();

        qPrintf("Missing argument!\nUsage: %s <filename> <outname:optional>\n", filename.mData);
        return 1;
    }

    qString filename = argv[1];
    qString outname = (argc > 2 ? argv[2] : argv[1]);
    if (!qFileExists(filename))
    {
        qPrintf("Error: File doesn't exist!\n");
        return 1;
    }

    char* ext = qStringFindLast(filename, ".");
    if (!ext)
    {
        qPrintf("Error: Filename has no extension, can't decide what to do...\n");
        return 1;
    }

    s64 fileSize;
    auto fileData = StreamFileWrapper::ReadEntireFile(filename, &fileSize);
    if (!fileData || !fileSize)
    {
        qPrintf("Error: Failed to read file or the file is empty!\n");
        return 1;
    }

    int fileType = DetermineFileType(fileData, fileSize);
    if (fileType == -1)
    {
        qPrintf("Error: Loaded file that is not valid SWF or UIScreen!\n");
        return 1;
    }

    const bool isSWF = (fileType == 1);
    outname = outname.ReplaceExtension(isSWF ? ".bin" : ".swf");

    if (isSWF)
    {
        if (!ConvertToBin(fileData, fileSize, outname))
        {
            qPrintf("Error: Failed to convert SWF to UIScreen!\n");
            return 1;
        }
    }
    else
    {
        auto chunk = static_cast<qChunk*>(fileData);
        auto screen = static_cast<UIScreenChunk*>(chunk->GetData());

        if (!ConvertToSWF(screen, outname))
        {
            qPrintf("Error: Failed to convert UIScreen to SWF!\n");
            return 1;
        }
    }

    return 0;
}